#ifndef DAEMON_SERVER_HANDLERS
#define DAEMON_SERVER_HANDLERS

#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <ctime>
#include <unistd.h>
#include <map>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <boost/algorithm/string.hpp>

#include "gen-cpp/daemonServer.grpc.pb.h"
#include "gen-cpp/producer.grpc.pb.h"
#include "../common/errCode.h"
#include "../common/handler_interface.h"
#include "../common/daemonClient/daemonClient.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using namespace std;
using namespace producer;

namespace daemon_server
{
class ServerImpl;

struct ServerInfo
{
    string address;
    string procName;
    uint32_t groupId;
    time_t timestamp;
    ServerInfo(): groupId(0), timestamp(0) {}
    ServerInfo(string addr, string proc, uint32_t grp, time_t ts): address(addr), procName(proc), groupId(grp), timestamp(ts) {}
};

extern map<uint64_t, ServerInfo> gSvrId2SvrInfo;
extern mutex gSvrInfoMutex;

extern CompletionQueue gCQ;

extern uint64_t curUid;
extern atomic<uint32_t> sendingCount;

extern const uint64_t UID_COUNT_PER_TIME;


class ClientRegisterCallData : public common::CallData
{
public:
    ClientRegisterCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq);
    void Proceed();

private:
    DaemonServer::AsyncService* service_;
    ClientRegisterRequest request_;
    ClientRegisterReply reply_;
    ServerAsyncResponseWriter<ClientRegisterReply> responder_;
};

class HeartBeatCallData : public common::CallData
{
public:
    HeartBeatCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq);
    void Proceed();

private:
    DaemonServer::AsyncService* service_;
    HeartBeatRequest request_;
    HeartBeatReply reply_;
    ServerAsyncResponseWriter<HeartBeatReply> responder_;
};

class ServerImpl
{
public:
    virtual ~ServerImpl();
    void Run();
    void RebalanceAndSend(const ProduceMsgRequest& req);
    void DecreaseSendingCount();

private:
    void HandleRpcs();
    void CheckProcAlive();

private:
    class BeginPushCallData : public common::CallData
    {
    public:
        BeginPushCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq, ServerImpl* ptr);
        void Proceed();

    private:
        DaemonServer::AsyncService* service_;
        BeginPushRequest request_;
        BeginPushReply reply_;
        ServerAsyncResponseWriter<BeginPushReply> responder_;
        ServerImpl* serverImpl;
    };

private:
    unique_ptr<ServerCompletionQueue> cq_;
    DaemonServer::AsyncService service_;
    unique_ptr<Server> server_;

    thread checkProcAliveThread_;
    thread handleCallBackThread_;

private:
    static const double ALIVE_DURATION; // sec
};

class ProducerCaller
{
public:
    ProducerCaller();
    explicit ProducerCaller(shared_ptr<Channel> channel, CompletionQueue* cq);
    void ProduceMsg(const ProduceMsgRequest& req);
    ProducerCaller& operator=(ProducerCaller& other);

private:
    unique_ptr<Producer::Stub> stub_;
    CompletionQueue* cq_;
};

extern map<uint64_t, ProducerCaller> gSvrId2ProducerCaller;


struct ProduceMsgAsyncCall : public daemon_client::AsyncCall
{
    ProduceMsgRequest request;
    ProduceMsgReply reply;
    unique_ptr<ClientAsyncResponseReader<ProduceMsgReply>> response_reader;
    void OnGetResponse(void*);
    void OnResponseFail(void*);
};


string parseAndGetIp(const string& peer);

}; // namespace daemon_server

#endif

