#include "daemonServer.h"

namespace daemon_server
{

map<uint64_t, ServerInfo> gSvrId2SvrInfo;
mutex gSvrInfoMutex;

map<double, uint64_t> gRank2SvrId;
unordered_map<uint64_t, ProducerState> gSvrId2ProducerState;
set<uint64_t> gSetIdleProducer;

CompletionQueue gCQ;
atomic<uint32_t> sendingCount(0);

const uint64_t UID_COUNT_PER_TIME = 4096;

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
            double score = calLoadBalanceScore(0.0);
            iter2Ok.first->second.score = score;

            ProducerState producerState(ProducerCaller(
                grpc::CreateChannel(
                    addr,
                    grpc::InsecureChannelCredentials()), &gCQ),
                0);
            gSvrId2ProducerState[reply_.server_id()] =
                producerState;
            gSetIdleProducer.insert(reply_.server_id());

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

        reply_.set_err(common::SUCCESS);
        uint64_t serverId = request_.server_id();
        double score;
        stringstream ssScore;
        ssScore << request_.score();
        ssScore >> score;
cout << "[LoadBalanceCallData::" << __func__
    << "]svrId:" << serverId
    << " score:" << score << endl;

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
        cout << "get beginPush call\n";
        ProduceMsgRequest req;
        req.set_msg_id(request_.msg_id());
        req.set_start_uid(request_.start_uid());
        req.set_end_uid(request_.end_uid());
        serverImpl->SelectProducerAndSend(req);

        status_ = FINISH;
        reply_.set_err(common::SUCCESS);
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

void ServerImpl::SelectProducerAndSend(const ProduceMsgRequest& request)
{
    for (uint64_t curUid = request.start_uid();
            curUid <= (uint64_t)request.end_uid();)
    {
        ProduceMsgRequest req;
        req.set_msg_id(request.msg_id());
        req.set_start_uid(curUid);
        req.set_end_uid((curUid + UID_COUNT_PER_TIME - 1
                > (uint64_t)request.end_uid()) ?
                request.end_uid() :
                curUid + UID_COUNT_PER_TIME - 1);

        gSvrInfoMutex.lock();
        uint32_t producerSize = gSvrId2ProducerState.size();
        if (!gSetIdleProducer.empty())
        {
            uint64_t svrId = *gSetIdleProducer.begin();
            gSetIdleProducer.erase(gSetIdleProducer.begin());
            ++sendingCount;
            ProducerState& producerState =
                gSvrId2ProducerState[svrId];
            ++producerState.curTaskCount;
            producerState.producerCaller.ProduceMsg(
                    svrId, req);
            gSvrInfoMutex.unlock();
            curUid += UID_COUNT_PER_TIME;
cout << "[" << __func__ << "(idle)]curUid:" << req.start_uid()
    << " endUid:" << (uint64_t)req.end_uid()
    << " svrId:" << svrId << endl;
        }
        else if (sendingCount.load() <=
                3 * producerSize &&
                producerSize != 0)
        {
            ++sendingCount;
            static const uint32_t factor = 10000;
            mt19937 rng;
            rng.seed(random_device()());
            uniform_int_distribution<mt19937::result_type> dist(0, factor);
            double randomRank = dist(rng) /
                (double)factor *
                gRank2SvrId.rbegin()->first;
            uint64_t svrId = gRank2SvrId.lower_bound(randomRank)->second;//lower_bound(key): not less than key

            ProducerState& producerState =
                gSvrId2ProducerState[svrId];
            ++producerState.curTaskCount;
            producerState.producerCaller.ProduceMsg(
                    svrId, req);
            gSvrInfoMutex.unlock();
            curUid += UID_COUNT_PER_TIME;
cout << "[" << __func__ << "]curUid:" << req.start_uid()
    << " endUid:" << req.end_uid() << " svrId:"
    << svrId << " randomRank:" << randomRank << endl;
        }
        else
        {
            gSvrInfoMutex.unlock();
            usleep(10000);
        }
    }
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

void ProducerCaller::ProduceMsg(const uint64_t svrId, const ProduceMsgRequest& req)
{
    ProduceMsgAsyncCall* call =
        new ProduceMsgAsyncCall(svrId);
    call->request.CopyFrom(req);
    call->response_reader = stub_->PrepareAsyncProduceMsg(&call->context, req, cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void ProduceMsgAsyncCall::OnGetResponse(void* ptr)
{
    DecreaseProducerTask(svrId);
    ServerImpl& daemonServer = *((ServerImpl*)ptr);

    if (reply.err() == common::SUCCESS)
    {
        cout << "produce msg reply success(startUid:"
            << request.start_uid() << "), svrId:"
            << svrId << endl;
    }
    else
    {
        cout << "get errCode, svrId:" << svrId
            << ". Re-send, startUid:"
            << request.start_uid() << endl;
        sleep(1);
        daemonServer.SelectProducerAndSend(request);
    }
}

void ProduceMsgAsyncCall::OnResponseFail(void* ptr)
{
    DecreaseProducerTask(svrId);
    ServerImpl& daemonServer = *((ServerImpl*)ptr);
    cout << "send msg fail, svrId:" << svrId
        << ". Re-send, startUid:" << request.start_uid()
        << endl;
    sleep(1);
    daemonServer.SelectProducerAndSend(request);
}

void DecreaseProducerTask(uint64_t svrId)
{
    --sendingCount;
    gSvrInfoMutex.lock();
    uint32_t curTask =
        --(gSvrId2ProducerState[svrId].curTaskCount);
    if (0 >= curTask)
    {
        gSetIdleProducer.insert(svrId);
    }
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

void ServerImpl::HandleRpcs()
{
    checkProcAliveThread_ = thread(&ServerImpl::CheckProcAlive, this);
    loadBalanceThread_ = thread(&ServerImpl::ReBalanceTimer, this);
    handleCallBackThread_ = thread(&common::AsyncCompleteRpc, this, &gCQ); // daemon will send req to servers(etc. producers), and should handle replies from these svrs
    checkProcAliveThread_.detach();
    loadBalanceThread_.detach();
    handleCallBackThread_.detach();

    new ClientRegisterCallData(&service_, cq_.get());
    new HeartBeatCallData(&service_, cq_.get());
    new BeginPushCallData(&service_, cq_.get(), this);
    new LoadBalanceCallData(&service_, cq_.get());

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
            uint64_t svrId = it->first;
            cout << "[" << __func__ << "]svrId:" << svrId
                << " addr:" << it->second.address
                << " procName:" << it->second.procName
                << endl;
            if (difftime(now, it->second.timestamp) > ALIVE_DURATION)
            {
                cout << "server not alive, svrId:"
                    << svrId << endl;
                string procName = it->second.procName;
                it = gSvrId2SvrInfo.erase(it);
                if ("producer" == procName)
                {
                    gSvrId2ProducerState.erase(svrId);
                    gSetIdleProducer.erase(svrId);
                    Rebalance();
                }
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
    gRank2SvrId.clear();

    double rank = 0.0;
    for (auto p = gSvrId2SvrInfo.begin();
            p != gSvrId2SvrInfo.end(); ++p)
    {
        uint64_t svrId = p->first;
        double score = p->second.score;

        rank += score;
cout << "[" << __func__ << "]svrId:"
    << svrId << " score:" << score
    << " rank:" << rank << " curTask:"
    << gSvrId2ProducerState[p->first].curTaskCount
    << endl;
        gRank2SvrId[rank] = svrId;
    }
}

}; //namespace daemon_server
