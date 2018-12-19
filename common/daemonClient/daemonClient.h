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

struct HeartBeatAsyncCall : public AsyncCall
{
    HeartBeatReply reply;
    std::unique_ptr<ClientAsyncResponseReader<HeartBeatReply>> response_reader;
    void OnGetResponse();
};


class DaemonClientImpl
{
public:
    explicit DaemonClientImpl(std::shared_ptr<Channel> channel);
    void AsyncCompleteRpc();

    void ClientRegister(ClientRegisterRequest& req);

private:
    std::unique_ptr<DaemonServer::Stub> stub_;
    CompletionQueue cq_;
};

}; // namespace daemon_client
