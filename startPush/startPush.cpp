#include "startPush.h"

namespace start_push
{

StartPush::StartPush(shared_ptr<Channel> channel, CompletionQueue* cq) :
    stub_(DaemonServer::NewStub(channel)),
    cq_(cq)
{
}

void StartPush::BeginPush(BeginPushRequest& req)
{
    BeginPushAsyncCall* call = new BeginPushAsyncCall;
    call->response_reader = stub_->PrepareAsyncBeginPush(&call->context, req, cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void BeginPushAsyncCall::OnGetResponse(void*)
{
    cout << "err:" << ((reply.err() == common::SUCCESS)
        ? "SUCCESS" : "ERR") << endl;
}

}; // namespace start_push
