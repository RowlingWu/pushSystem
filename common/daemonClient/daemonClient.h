#ifndef COMMON_DAEMON_CLIENT
#define COMMON_DAEMON_CLIENT

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "gen-cpp/daemonServer.grpc.pb.h"
#include "../errCode.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using namespace daemon_server;
using namespace std;

namespace daemon_client
{

struct AsyncCall
{
    ClientContext context;
    Status status;
    virtual void OnGetResponse(void*) = 0;
    virtual void OnResponseFail(void*);
};

struct ClientRegisterAsyncCall : public AsyncCall
{
    ClientRegisterReply reply;
    unique_ptr<ClientAsyncResponseReader<ClientRegisterReply>> response_reader;
    void OnGetResponse(void*);
};

struct HeartBeatAsyncCall : public AsyncCall
{
    HeartBeatReply reply;
    unique_ptr<ClientAsyncResponseReader<HeartBeatReply>> response_reader;
    void OnGetResponse(void*);
};

enum ClientStatus { NO_SERVER, REGISTERED };

class DaemonClientImpl
{
public:
    explicit DaemonClientImpl(shared_ptr<Channel> channel, string port, string procName, uint32_t groupId);
    void AsyncCompleteRpc();
    void ClientRegister(ClientRegisterRequest& req);
    void HeartBeat(HeartBeatRequest& req);

    void SetServerId(uint64_t serverId);
    void SetClientStatus(ClientStatus status);

private:
    void ClientStatusHandler();

private:
    unique_ptr<DaemonServer::Stub> stub_;
    CompletionQueue cq_;
    thread clientStatusThread_;

    uint64_t serverId_;
    string listeningPort_;
    string procName_;
    uint32_t groupId_;
    ClientStatus clientStatus_;
    mutex clientStatusMtx_;
};

}; // namespace daemon_client

#endif
