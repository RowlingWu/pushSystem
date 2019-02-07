#include "startPush.h"

namespace start_push
{

std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
std::chrono::time_point<std::chrono::high_resolution_clock> endTime;

StartPush::StartPush(shared_ptr<Channel> channel, CompletionQueue* cq) :
    stub_(DaemonServer::NewStub(channel)),
    cq_(cq)
{
}

void StartPush::BeginPush(BeginPushRequest& req)
{
    startTime = chrono::high_resolution_clock::now();

    BeginPushAsyncCall* call = new BeginPushAsyncCall;
    call->response_reader = stub_->PrepareAsyncBeginPush(&call->context, req, cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void BeginPushAsyncCall::OnGetResponse(void*)
{
    cout << "err:" << ((reply.err() == common::SUCCESS)
        ? "SUCCESS" : "ERR") << endl;

    endTime = chrono::high_resolution_clock::now();
    cout << "Total time used:" << chrono::duration<double, milli>(endTime - startTime).count() << " ms\n";
}

}; // namespace start_push
