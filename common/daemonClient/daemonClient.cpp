#include "daemonClient.h"

namespace daemon_client
{

DaemonClientImpl::DaemonClientImpl(std::shared_ptr<Channel> channel) : stub_(DaemonServer::NewStub(channel))
{
}

void DaemonClientImpl::AsyncCompleteRpc()
{
    void* got_tag;
    bool ok = false;
    while (cq_.Next(&got_tag, &ok))
    {
        AsyncCall* call = static_cast<AsyncCall*>(got_tag);
        GPR_ASSERT(ok);
        if (call->status.ok())
        {
            call->OnGetResponse();
        }
        else
        {
            std::cout << "RPC failed\n";
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

void ClientRegisterAsyncCall::OnGetResponse()
{
    std::cout << "Register errCode:" << reply.err() << std::endl;
}

void HeartBeatAsyncCall::OnGetResponse()
{
}

}; // namespace daemon_client
