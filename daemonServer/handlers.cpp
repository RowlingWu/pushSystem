#include "handlers.h"

namespace daemon_server
{

map<uint64_t, ServerInfo> gSvrId2SvrInfo;
mutex gSvrInfoMutex;
CompletionQueue gCQ;
ProducerCaller producerCaller(grpc::CreateChannel("localhost:50053", grpc::InsecureChannelCredentials()), &gCQ);

ClientRegisterCallData::ClientRegisterCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void ClientRegisterCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestClientRegister(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new ClientRegisterCallData(service_, cq_);

        reply_.set_err(common::SUCCESS);
        gSvrInfoMutex.lock();
        if (gSvrId2SvrInfo.empty())
        {
            reply_.set_server_id(1001);
        }
        else
        {
            reply_.set_server_id(gSvrId2SvrInfo.rbegin()->first + 1);
        }
        gSvrId2SvrInfo[reply_.server_id()] =
            ServerInfo(parseAndGetIp(ctx_.peer()) + ":"
                    + request_.listening_port(),
                    request_.proc_name(),
                    request_.group_id(),
                    time(NULL));

        gSvrInfoMutex.unlock();

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

HeartBeatCallData::HeartBeatCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void HeartBeatCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestHeartBeat(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new HeartBeatCallData(service_, cq_);

        reply_.set_err(common::SUCCESS);
        gSvrInfoMutex.lock();
        auto it = gSvrId2SvrInfo.find(request_.server_id());
        if (gSvrId2SvrInfo.end() == it)
        {
            reply_.set_err(common::ERR);
            cout << "unknown svrId:" << request_.server_id() << endl;
        }
        else
        {
            it->second.timestamp = time(NULL);
        }

        gSvrInfoMutex.unlock();

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

BeginPushCallData::BeginPushCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void BeginPushCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestBeginPush(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new BeginPushCallData(service_, cq_);

        cout << "get beginPush call\n";

        // TODO: rebalance policy
        // TODO: call producer
        ProduceMsgRequest req;
        req.set_msg_id(request_.msg_id());
        req.set_start_uid(request_.start_uid());
        req.set_end_uid(request_.end_uid());
        producerCaller.ProduceMsg(req);

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

string parseAndGetIp(const string& peer)
{
    vector<string> result;
    boost::split(result, peer, [](char c) { return ':' == c; });
    if (result.size() == 3 && !result[1].empty())
    {
        return result[1];
    }
    return "";
}

ProducerCaller::ProducerCaller(shared_ptr<Channel> channel, CompletionQueue* cq) :
    stub_(Producer::NewStub(channel)),
    cq_(cq)
{
}

void ProducerCaller::ProduceMsg(ProduceMsgRequest& req)
{
    ProduceMsgAsyncCall* call = new ProduceMsgAsyncCall;
    call->response_reader = stub_->PrepareAsyncProduceMsg(&call->context, req, cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void ProduceMsgAsyncCall::OnGetResponse(void* ptr)
{
    ServerImpl& daemonServer = *((ServerImpl*)ptr);

    cout << "produce msg reply: err:" << reply.err() << endl;
}

}; //namespace daemon_server
