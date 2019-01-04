#include "daemonClient.h"

namespace daemon_client
{

void AsyncCall::OnResponseFail(void*)
{
    cout << "RPC fail!\n";
}

DaemonClientImpl::DaemonClientImpl(shared_ptr<Channel> channel, string port, string procName, uint32_t groupId) :
    stub_(DaemonServer::NewStub(channel)),
    listeningPort_(port),
    procName_(procName),
    groupId_(groupId),
    clientStatus_(NO_SERVER)
{
}

void DaemonClientImpl::AsyncCompleteRpc()
{
    clientStatusThread_ = thread(&DaemonClientImpl::ClientStatusHandler, this);

    void* got_tag;
    bool ok = false;
    while (cq_.Next(&got_tag, &ok))
    {
        AsyncCall* call = static_cast<AsyncCall*>(got_tag);
        GPR_ASSERT(ok);
        if (call->status.ok())
        {
            call->OnGetResponse(this);
        }
        else
        {
            cout << "RPC failed\n";
        }
        delete call;
    }
}

void DaemonClientImpl::ClientRegister(ClientRegisterRequest& req)
{
    ClientRegisterAsyncCall* call = new ClientRegisterAsyncCall;
    call->response_reader = stub_->PrepareAsyncClientRegister(&call->context, req, &cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void DaemonClientImpl::HeartBeat(HeartBeatRequest& req)
{
    HeartBeatAsyncCall* call = new HeartBeatAsyncCall;
    call->response_reader = stub_->PrepareAsyncHeartBeat(&call->context, req, &cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void DaemonClientImpl::SetServerId(uint64_t serverId)
{
    serverId_ = serverId;
    clientStatusMtx_.lock();
    clientStatus_ = REGISTERED;
    clientStatusMtx_.unlock();
}

void DaemonClientImpl::SetClientStatus(ClientStatus status)
{
    clientStatusMtx_.lock();
    clientStatus_ = status;
    clientStatusMtx_.unlock();
}

void DaemonClientImpl::ClientStatusHandler()
{
    while (true)
    {
        clientStatusMtx_.lock();
        ClientStatus status = clientStatus_;
        clientStatusMtx_.unlock();
        if (NO_SERVER == status)
        {
            cout << "Register.\n";
            ClientRegisterRequest req;
            req.set_listening_port(listeningPort_);
            req.set_proc_name(procName_);
            req.set_group_id(groupId_);
            ClientRegister(req);
        }
        else
        {
            cout << "Heart beat.\n";
            HeartBeatRequest req;
            req.set_server_id(serverId_);
            HeartBeat(req);
        }
        sleep(2);
    }
}

void ClientRegisterAsyncCall::OnGetResponse(void* ptr)
{
    DaemonClientImpl& client = *((DaemonClientImpl*)ptr);
    if (common::SUCCESS == reply.err())
    {
        cout << "Register success.\n";
        client.SetServerId(reply.server_id());
    }
    else
    {
        cout << "Register fail.\n";
        client.SetClientStatus(NO_SERVER);
    }
}

void HeartBeatAsyncCall::OnGetResponse(void* ptr)
{
    DaemonClientImpl& client = *((DaemonClientImpl*)ptr);
    if (common::ERR == reply.err())
    {
        cout << "Send heart beat fail.\n";
        client.SetClientStatus(NO_SERVER);
    }
    else
    {
        cout << "Send heart beat success.\n";
    }
}

}; // namespace daemon_client
