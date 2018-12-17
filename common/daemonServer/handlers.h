#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "gen-cpp/daemonServer.grpc.pb.h"
#include "../errCode.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

namespace daemon_server
{
    
class CallData
{
public:
    CallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
        service_(service), cq_(cq), status_(CREATE)
    {
    }
    virtual void Proceed() = 0;

protected:
    enum CallStatus { CREATE, PROCESS, FINISH };

protected:
    DaemonServer::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    CallStatus status_;
};

class ClientRegisterCallData : public CallData
{
public:
    ClientRegisterCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
        CallData(service, cq), responder_(&ctx_)
    {
        Proceed();
    }

    void Proceed()
    {
        if (status_ == CREATE)
        {
            status_ = PROCESS;
            service_->RequestClientRegister(&ctx_, &request_, &responder_, cq_, cq_, this);
        }
        else if (status_ == PROCESS)
        {
            new ClientRegisterCallData(service_, cq_);

            std::cout << "addr:" << request_.address()
                << " procName:" << request_.proc_name()
                << " grpId:" << request_.group_id()
                << std::endl;
            reply_.set_err(common::SUCCESS);

            status_ = FINISH;
            responder_.Finish(reply_, Status::OK, this);
        }
        else
        {
            GPR_ASSERT(status_ == FINISH);
            delete this;
        }
    }

private:
    ClientRegisterRequest request_;
    ClientRegisterReply reply_;
    ServerAsyncResponseWriter<ClientRegisterReply> responder_;
};

}; // namespace daemon_server
