#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <thread>

#include "gen-cpp/daemonServer.grpc.pb.h"
#include "../errCode.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using namespace daemon_server;

namespace daemon_client
{

struct AsyncCall
{
    ClientContext context;
    Status status;
    virtual void OnGetResponse() = 0;
};

struct ClientRegisterAsyncCall : public AsyncCall
{
    ClientRegisterReply reply;
    std::unique_ptr<ClientAsyncResponseReader<ClientRegisterReply>> response_reader;
    void OnGetResponse();
};

struct TestAsyncCall : public AsyncCall
{
    TestReply reply;
    std::unique_ptr<ClientAsyncResponseReader<TestReply>> response_reader;
    void OnGetResponse();
};


class DaemonClientImpl
{
public:
    explicit DaemonClientImpl(std::shared_ptr<Channel> channel);
    void AsyncCompleteRpc();

    void ClientRegister(ClientRegisterRequest& req);
    void Test(TestRequest& req);

private:
    std::unique_ptr<DaemonServer::Stub> stub_;
    CompletionQueue cq_;
};

}; // namespace daemon_client
