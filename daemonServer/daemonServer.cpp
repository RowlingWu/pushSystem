#include "daemonServer.h"

namespace daemon_server
{

const double ServerImpl::ALIVE_DURATION = 20; // sec

ServerImpl::~ServerImpl()
{
    server_->Shutdown();
    cq_->Shutdown();
}

void ServerImpl::Run()
{
    string serverAddress("0.0.0.0:50051");

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
    gSvrInfoMutex.lock();
    uint32_t times = (id++) % gSvrId2ProducerCaller.size();
    auto p = gSvrId2ProducerCaller.begin();
    for (uint32_t i = 0; i < times; ++i, ++p)
    {}
    p->ProduceMsg(req);
    gSvrInfoMutex.unlock();
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
        sleep(10); // sleep 20 sec
    }
}

}; // daemon_server
