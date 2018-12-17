#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "handlers.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

namespace daemon_server
{

class ServerImpl final
{
public:
    ~ServerImpl();
    void Run();

private:
    void HandleRpcs();

private:
    std::unique_ptr<ServerCompletionQueue> cq_;
    DaemonServer::AsyncService service_;
    std::unique_ptr<Server> server_;
};

};  // namespace daemon_server
