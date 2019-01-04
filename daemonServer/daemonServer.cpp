#include "daemonServer.h"

namespace daemon_server
{

map<uint64_t, ServerInfo> gSvrId2SvrInfo;
map<uint64_t, ProducerCaller> gSvrId2ProducerCaller;
mutex gSvrInfoMutex;
CompletionQueue gCQ;
uint64_t curUid = 0;
atomic<uint32_t> sendingCount(0);

const uint64_t UID_COUNT_PER_TIME = 1024;

const double ServerImpl::ALIVE_DURATION = 20; // sec

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
        gSvrId2SvrInfo[reply_.server_id()] =
            ServerInfo(addr,
                    request_.proc_name(),
                    request_.group_id(),
                    time(NULL));

        if ("producer" == request_.proc_name())
        {
            ProducerCaller producerCaller(
                grpc::CreateChannel(
                addr,
                grpc::InsecureChannelCredentials()),
                &gCQ);
            gSvrId2ProducerCaller[reply_.server_id()] =
                producerCaller;
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

        cout << "get beginPush call\n";

        for (curUid = request_.start_uid();
                curUid <= (uint64_t)request_.end_uid();)
        {
            gSvrInfoMutex.lock();
            uint32_t producerSize = gSvrId2ProducerCaller.size();
            if (sendingCount.load() <= 3 * producerSize
                    && producerSize > 0)
            {
cout << "curUid:" << curUid << " endUid:" << (uint64_t)request_.end_uid() << endl;
                ProduceMsgRequest req;
                req.set_msg_id(request_.msg_id());
                req.set_start_uid(curUid);
                req.set_end_uid(curUid + UID_COUNT_PER_TIME - 1);
                ++sendingCount;
                serverImpl->RebalanceAndSend(req);
                gSvrInfoMutex.unlock();
                curUid += UID_COUNT_PER_TIME;
            }
            else
            {
                gSvrInfoMutex.unlock();
                usleep(10000);
            }
        }

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
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
        daemonServer.RebalanceAndSend(request);
        gSvrInfoMutex.unlock();
    }
}

void ProduceMsgAsyncCall::OnResponseFail(void* ptr)
{
    ServerImpl& daemonServer = *((ServerImpl*)ptr);
    cout << "send msg fail. re-send.\n";
    sleep(1);
    gSvrInfoMutex.lock();
    daemonServer.RebalanceAndSend(request);
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

void ServerImpl::RebalanceAndSend(const ProduceMsgRequest& req)
{
    // Select a proper ProducerCaller
    static uint32_t id = 0;
    uint32_t producerSize = gSvrId2ProducerCaller.size();
    if (producerSize)
    {
        uint32_t times = (id++) % producerSize;
        auto p = gSvrId2ProducerCaller.begin();
cout << "Prodc size:" << gSvrId2ProducerCaller.size()
    << " times:" << times << endl;
        for (uint32_t i = 0; i < times; ++i, ++p)
        {}
        p->second.ProduceMsg(req);
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
    checkProcAliveThread_ = thread(&ServerImpl::CheckProcAlive, this);

    handleCallBackThread_ = thread(&common::AsyncCompleteRpc, this, &gCQ); // daemon will send req to servers(etc. producers), and should handle replies from these svrs

    void* tag;
    bool ok;
    while (true)
    {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        static_cast<common::CallData*>(tag)->Proceed();
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
                gSvrId2ProducerCaller.erase(it->first);
                it = gSvrId2SvrInfo.erase(it);
                continue;
            }
            ++it;
        }

        gSvrInfoMutex.unlock();
        sleep(10); // sleep x sec
    }
}

}; //namespace daemon_server
