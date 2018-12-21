#ifndef COMMON_CLIENT_SERVER
#define COMMON_CLIENT_SERVER

#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using namespace std;

namespace daemon_client
{

template<typename T>
class ServerImpl
{
public:
    ~ServerImpl()
    {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void Run(const string& addr)
    {
        ServerBuilder builder;
        builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
        cout << "server listening on " << addr << endl;

        HandleRpcs();
    }

private:
    void HandleRpcs()
    {
        cout << "Err! No template matched!\n";
    }

private:
    typename T::AsyncService service_;
    unique_ptr<ServerCompletionQueue> cq_;
    unique_ptr<Server> server_;
};

};  // namespace daemon_client

#endif
