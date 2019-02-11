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
#include <unordered_map>
#include <set>
#include <cmath>
#include <random>
#include <sstream>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <boost/algorithm/string.hpp>

#include "gen-cpp/daemonServer.grpc.pb.h"
#include "gen-cpp/producer.grpc.pb.h"
#include "../common/errCode.h"
#include "../common/handler_interface.h"
#include "../common/daemonClient/daemonClient.h"
#include "../common/common.h"

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

/*
 * ProducerCaller acts as a grpc client which makes 
 * grpc requests and sends them to producers
 */
class ProducerCaller
{
public:
    ProducerCaller();
    ProducerCaller(ProducerCaller& other);
    ProducerCaller(const ProducerCaller& other);
    explicit ProducerCaller(shared_ptr<Channel> channel, CompletionQueue* cq);
    void ProduceMsg(const uint64_t svrId, const ProduceMsgRequest& req);
    ProducerCaller& operator=(ProducerCaller& other);

private:
    unique_ptr<Producer::Stub> stub_;
    CompletionQueue* cq_;
};

struct ServerInfo
{
    string address;
    string procName;
    uint32_t groupId;
    time_t timestamp;
    double score;  // load balance info
    ServerInfo(): groupId(0), timestamp(0), score(0.0) {}
    ServerInfo(string addr, string proc, uint32_t grp, time_t ts): address(addr), procName(proc), groupId(grp), timestamp(ts), score(0.0) {}
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

class LoadBalanceCallData : public common::CallData
{
public:
    LoadBalanceCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq);
    void Proceed();

private:
    DaemonServer::AsyncService* service_;
    LoadBalanceRequest request_;
    LoadBalanceReply reply_;
    ServerAsyncResponseWriter<LoadBalanceReply> responder_;
};

class ServerImpl
{
public:
    virtual ~ServerImpl();
    void Run();
    void SelectProducerAndSend(const ProduceMsgRequest& req);

private:
    void HandleRpcs();
    void CheckProcAlive();
    void ReBalanceTimer();

private:
    class BeginPushCallData : public common::CallData
    {
    public:
        BeginPushCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq, ServerImpl* ptr);
        void Proceed();
        void NotifyProducers();

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
    thread loadBalanceThread_;
    thread handleCallBackThread_;

private:
    static const double ALIVE_DURATION; // sec
};


struct ProducerState
{
    ProducerCaller producerCaller;
    uint32_t curTaskCount;
    ProducerState() : curTaskCount(0) {}
    ProducerState(ProducerCaller pc, uint32_t n) : producerCaller(pc), curTaskCount(n) {}
    ProducerState& operator=(ProducerState& other)
    {
        producerCaller = other.producerCaller;
        curTaskCount = other.curTaskCount;
        return *this;
    }
};
extern unordered_map<uint64_t, ProducerState> gSvrId2ProducerState;
extern set<uint64_t> gSetIdleProducer;
extern map<double, uint64_t> gRank2SvrId;

extern void Rebalance();
extern void DecreaseProducerTask(uint64_t svrId);

struct ProduceMsgAsyncCall : public daemon_client::AsyncCall
{
    uint64_t svrId;
    ProduceMsgRequest request;
    ProduceMsgReply reply;
    unique_ptr<ClientAsyncResponseReader<ProduceMsgReply>> response_reader;
    ProduceMsgAsyncCall(uint64_t id) : svrId(id) {}
    void OnGetResponse(void*);
    void OnResponseFail(void*);
};


string parseAndGetIp(const string& peer);

}; // namespace daemon_server

#endif

