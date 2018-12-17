#include "daemonServer.h"

namespace daemon_server
{

ServerImpl::~ServerImpl()
{
    server_->Shutdown();
    cq_->Shutdown();
}

void ServerImpl::Run()
{
    std::string serverAddress("0.0.0.0:50051");

    ServerBuilder builder;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    std::cout << "server listening on " << serverAddress << std::endl;

    HandleRpcs();
}

void ServerImpl::HandleRpcs()
{
    new ClientRegisterCallData(&service_, cq_.get());
    new TestCallData(&service_, cq_.get());

    void* tag;
    bool ok;
    while (true)
    {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        static_cast<CallData*>(tag)->Proceed();
    }
}

}; // daemon_server
