#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

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

class ServerImpl
{
public:
    virtual ~ServerImpl();
    void Run();

private:
    void HandleRpcs();
    void CheckProcAlive();

private:
    unique_ptr<ServerCompletionQueue> cq_;
    DaemonServer::AsyncService service_;
    unique_ptr<Server> server_;

    thread checkProcAliveThread_;

private:
    static const double ALIVE_DURATION; // sec
};

};  // namespace daemon_server
