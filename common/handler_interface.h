#ifndef COMMON_HANDLER_INTERFACE
#define COMMON_HANDLER_INTERFACE

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

#include "daemonClient/daemonClient.h"

namespace common
{

class CallData
{
public:
    CallData(ServerCompletionQueue* cq) :
        cq_(cq), status_(CREATE)
    {}
    virtual ~CallData() {}
    virtual void Proceed() = 0;

protected:
    enum CallStatus { CREATE, PROCESS, FINISH };

protected:
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    CallStatus status_;
};

static void HandleAsyncCall(void* got_tag, bool ok, void* ptr)
{
    daemon_client::AsyncCall* call = static_cast<daemon_client::AsyncCall*>(got_tag);
    GPR_ASSERT(ok);
    if (call->status.ok())
    {
        call->OnGetResponse(ptr);
    }
    else
    {
        cout << "error_code:"
            << call->status.error_code()
            << ", error_msg:"
            << call->status.error_message() << endl;
        call->OnResponseFail(ptr);
    }
    delete call;
}

static void AsyncCompleteRpc(void* ptr, CompletionQueue* cq_)
{
    void* got_tag;
    bool ok = false;
    while (cq_->Next(&got_tag, &ok))
    {
        thread t = thread(&HandleAsyncCall, got_tag, ok, ptr);
        t.detach();
    }
}

}; // namespace common

#endif

