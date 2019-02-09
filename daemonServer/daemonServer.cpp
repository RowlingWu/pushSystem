#include "daemonServer.h"

namespace daemon_server
{

map<uint64_t, ServerInfo> gSvrId2SvrInfo;
map<double, ProducerCaller> gScore2ProducerCaller;
map<double, double> gRank2Score;
mutex gSvrInfoMutex;
CompletionQueue gCQ;
uint64_t curUid = 0;
atomic<uint32_t> sendingCount(0);

const uint64_t UID_COUNT_PER_TIME = 1024;

const double ServerImpl::ALIVE_DURATION = 10; // sec

ClientRegisterCallData::ClientRegisterCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void ClientRegisterCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestClientRegister(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new ClientRegisterCallData(service_, cq_);

        reply_.set_err(common::SUCCESS);
        gSvrInfoMutex.lock();
        if (gSvrId2SvrInfo.empty())
        {
            reply_.set_server_id(1001);
        }
        else
        {
            reply_.set_server_id(gSvrId2SvrInfo.rbegin()->first + 1);
        }
        string addr = parseAndGetIp(ctx_.peer()) + ":"
                    + request_.listening_port();
        auto iter2Ok = gSvrId2SvrInfo.insert(make_pair(
            reply_.server_id(),
            ServerInfo(addr,
                    request_.proc_name(),
                    request_.group_id(),
                    time(NULL))
        ));

        if ("producer" == request_.proc_name())
        {
            ProducerCaller producerCaller(
                grpc::CreateChannel(
                addr,
                grpc::InsecureChannelCredentials()),
                &gCQ);
            double score = 10000.0;
            if (!gScore2ProducerCaller.empty())
            {
                gScore2ProducerCaller.crbegin()->first;
                score = gScore2ProducerCaller.crbegin()->first + 0.01;
            }
            iter2Ok.first->second.score = score;
            auto score2ProducerCaller2Ok =
                gScore2ProducerCaller.insert(make_pair(
                        score, producerCaller
            ));
            iter2Ok.first->second.pScore2Producer = score2ProducerCaller2Ok.first;
            Rebalance();
        }

        gSvrInfoMutex.unlock();

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

LoadBalanceCallData::LoadBalanceCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void LoadBalanceCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestLoadBalance(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new LoadBalanceCallData(service_, cq_);
cout << "[LoadBalanceCallData::" << __func__
    << "]svrId:" << request_.server_id()
    << " score:" << request_.score() << endl;

        reply_.set_err(common::SUCCESS);
        uint64_t serverId = request_.server_id();
        double score;
        stringstream ssScore;
        ssScore << request_.score();
        ssScore >> score;

        gSvrInfoMutex.lock();
        auto pSvrId2SvrInfo = gSvrId2SvrInfo.find(serverId);
        if (gSvrId2SvrInfo.end() == pSvrId2SvrInfo)
        {
            reply_.set_err(common::ERR);
        }
        else
        {
            pSvrId2SvrInfo->second.score = score;
        }
        gSvrInfoMutex.unlock();

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

HeartBeatCallData::HeartBeatCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void HeartBeatCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestHeartBeat(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new HeartBeatCallData(service_, cq_);

cout << "Receive heartbeat from svrId " << request_.server_id() << endl;
        reply_.set_err(common::SUCCESS);
        gSvrInfoMutex.lock();
        auto it = gSvrId2SvrInfo.find(request_.server_id());
        if (gSvrId2SvrInfo.end() == it)
        {
            reply_.set_err(common::ERR);
            cout << "unknown svrId:" << request_.server_id() << endl;
        }
        else
        {
            it->second.timestamp = time(NULL);
        }

        gSvrInfoMutex.unlock();

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

ServerImpl::BeginPushCallData::BeginPushCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq, ServerImpl* ptr) :
    CallData(cq), service_(service), responder_(&ctx_),
    serverImpl(ptr)
{
    Proceed();
}

void ServerImpl::BeginPushCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestBeginPush(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new BeginPushCallData(service_, cq_, serverImpl);
        NotifyProducers();
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

void ServerImpl::BeginPushCallData::NotifyProducers()
{
    cout << "get beginPush call\n";

    for (curUid = request_.start_uid();
            curUid <= (uint64_t)request_.end_uid();)
    {
        gSvrInfoMutex.lock();
        uint32_t producerSize = gScore2ProducerCaller.size();
        gSvrInfoMutex.unlock();
        if (sendingCount.load() <= 3 * producerSize)
        {
cout << "curUid:" << curUid << " endUid:" << (uint64_t)request_.end_uid() << endl;
            ProduceMsgRequest req;
            req.set_msg_id(request_.msg_id());
            req.set_start_uid(curUid);
            req.set_end_uid(curUid + UID_COUNT_PER_TIME - 1);
            ++sendingCount;
            serverImpl->SelectProducerAndSend(req);
            curUid += UID_COUNT_PER_TIME;
        }
        else
        {
            usleep(10000);
        }
    }

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
}

string parseAndGetIp(const string& peer)
{
    vector<string> result;
    boost::split(result, peer, [](char c) { return ':' == c; });
    if (result.size() == 3 && !result[1].empty())
    {
        return result[1];
    }
    return "";
}

ProducerCaller::ProducerCaller()
{
}

ProducerCaller::ProducerCaller(ProducerCaller& other)
{
    *this = other;
}

ProducerCaller::ProducerCaller(const ProducerCaller& other) : cq_(other.cq_)
{
    stub_.reset(other.stub_.release());
}

ProducerCaller::ProducerCaller(shared_ptr<Channel> channel, CompletionQueue* cq) :
    stub_(Producer::NewStub(channel)),
    cq_(cq)
{
}

ProducerCaller& ProducerCaller::operator=(ProducerCaller& other)
{
    stub_ = std::move(other.stub_);
    cq_ = other.cq_;
    return *this;
}

void ProducerCaller::ProduceMsg(const ProduceMsgRequest& req)
{
    ProduceMsgAsyncCall* call = new ProduceMsgAsyncCall;
    call->request.CopyFrom(req);
    call->response_reader = stub_->PrepareAsyncProduceMsg(&call->context, req, cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void ProduceMsgAsyncCall::OnGetResponse(void* ptr)
{
    ServerImpl& daemonServer = *((ServerImpl*)ptr);

    cout << "produce msg reply(startUid:"
        << request.start_uid() << "), err:"
        << reply.err() << endl;
    if (reply.err() == common::SUCCESS)
    {
        daemonServer.DecreaseSendingCount();
    }
    else
    {
        cout << "get errCode. re-send.\n";
        sleep(1);
        gSvrInfoMutex.lock();
        daemonServer.SelectProducerAndSend(request);
        gSvrInfoMutex.unlock();
    }
}

void ProduceMsgAsyncCall::OnResponseFail(void* ptr)
{
    ServerImpl& daemonServer = *((ServerImpl*)ptr);
    cout << "send msg fail. re-send.\n";
    sleep(1);
    gSvrInfoMutex.lock();
    daemonServer.SelectProducerAndSend(request);
    gSvrInfoMutex.unlock();
}


ServerImpl::~ServerImpl()
{
    server_->Shutdown();
    cq_->Shutdown();
}

void ServerImpl::Run()
{
    string serverAddress("192.168.99.100:50051");

    ServerBuilder builder;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    cout << "server listening on " << serverAddress << endl;

    HandleRpcs();
}

void ServerImpl::SelectProducerAndSend(const ProduceMsgRequest& req)
{
    while (true)
    {
        gSvrInfoMutex.lock();
        uint64_t producerSize = gScore2ProducerCaller.size();
        if (producerSize)
        {
            // Load balance policy
            static const uint32_t factor = 1000000;
            mt19937 rng;
            rng.seed(random_device()());
            uniform_int_distribution<mt19937::result_type> dist(0, factor);
            double randomRank = dist(rng) /
                (double)factor *
                gRank2Score.rbegin()->first;
            double& score = gRank2Score.lower_bound(randomRank)->second;//lower_bound(key): not less than key
cout << "[debug]score:" << score << endl;
if (gRank2Score.lower_bound(randomRank) != gRank2Score.end())
{
    cout << "[debug]not gRank2Score.end\n";
}
            gScore2ProducerCaller[score].ProduceMsg(req);
            gSvrInfoMutex.unlock();
            return;
        }
        gSvrInfoMutex.unlock();
        sleep(1);
    }
}

void ServerImpl::DecreaseSendingCount()
{
    --sendingCount;
}

void ServerImpl::HandleRpcs()
{
    new ClientRegisterCallData(&service_, cq_.get());
    new HeartBeatCallData(&service_, cq_.get());
    new BeginPushCallData(&service_, cq_.get(), this);
    new LoadBalanceCallData(&service_, cq_.get());
    checkProcAliveThread_ = thread(&ServerImpl::CheckProcAlive, this);
    loadBalanceThread_ = thread(&ServerImpl::ReBalanceTimer, this);

    handleCallBackThread_ = thread(&common::AsyncCompleteRpc, this, &gCQ); // daemon will send req to servers(etc. producers), and should handle replies from these svrs

    void* tag;
    bool ok;
    while (true)
    {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        thread t = thread(&common::CallData::Proceed, static_cast<common::CallData*>(tag));
        t.detach();
    }
}

void ServerImpl::CheckProcAlive()
{
    while (true)
    {
        time_t now = time(NULL);
        gSvrInfoMutex.lock();

        for (auto it = gSvrId2SvrInfo.begin();
                it != gSvrId2SvrInfo.end();)
        {
            cout << "[" << __func__ << "]svrId:" << it->first
                << " addr:" << it->second.address
                << " procName:" << it->second.procName
                << " grpId:" << it->second.groupId
                << " timestamp:" << it->second.timestamp
                << endl;
            if (difftime(now, it->second.timestamp) > ALIVE_DURATION)
            {
                cout << "server not alive, svrId:"
                    << it->first << endl;
                gScore2ProducerCaller.erase(
                        it->second.pScore2Producer);
                it = gSvrId2SvrInfo.erase(it);
                Rebalance();
                continue;
            }
            ++it;
        }

        gSvrInfoMutex.unlock();
        sleep(10); // sleep x sec
    }
}

void ServerImpl::ReBalanceTimer()
{
    while (true)
    {
        gSvrInfoMutex.lock();
        Rebalance();
        gSvrInfoMutex.unlock();

        sleep(10);
    }
}

void Rebalance()
{
    gRank2Score.clear();

cout << "[" << __func__ << "]";
    double rank = 0.0;
    for (auto p = gSvrId2SvrInfo.begin();
            p != gSvrId2SvrInfo.end(); ++p)
    {
cout << "svrId:" << p->first << " score:"
    << p->second.score << endl;

        ServerInfo& svrInfo = p->second;
        rank += svrInfo.score;// deal with gRank2Score
        gRank2Score[rank] = svrInfo.score;

        if (abs(svrInfo.pScore2Producer->first -
                svrInfo.score) > 0.0001)
        {                 // deal with gScore2Producer
            auto p = gScore2ProducerCaller.insert(
                make_pair(svrInfo.score,
                svrInfo.pScore2Producer->second));
            gScore2ProducerCaller.erase(
                    svrInfo.pScore2Producer);
            svrInfo.pScore2Producer = p.first;
        }
    }
}

}; //namespace daemon_server
