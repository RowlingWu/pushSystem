#ifndef START_PUSH
#define START_PUSH

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "gen-cpp/daemonServer.grpc.pb.h"
#include "../common/daemonClient/daemonClient.h"
#include "../common/errCode.h"
#include "../common/handler_interface.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using namespace daemon_server;

namespace start_push
{

extern std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
extern std::chrono::time_point<std::chrono::high_resolution_clock> endTime;

class StartPush
{
public:
    explicit StartPush(shared_ptr<Channel> channel, CompletionQueue* cq);
    void BeginPush(daemon_server::BeginPushRequest& req);

private:
    unique_ptr<DaemonServer::Stub> stub_;
    CompletionQueue* cq_;
};

struct BeginPushAsyncCall : public daemon_client::AsyncCall
{
    BeginPushReply reply;
    unique_ptr<ClientAsyncResponseReader<BeginPushReply>> response_reader;
    void OnGetResponse(void*);
};

}; // namespace start_push

#endif
