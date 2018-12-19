#ifndef DAEMON_SERVER_HANDLERS
#define DAEMON_SERVER_HANDLERS

#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <ctime>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "gen-cpp/daemonServer.grpc.pb.h"
#include "../common/errCode.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using namespace std;

namespace daemon_server
{

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

class CallData
{
public:
    CallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq) :
        service_(service), cq_(cq), status_(CREATE)
    {}
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
    ClientRegisterCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq);
    void Proceed();

private:
    ClientRegisterRequest request_;
    ClientRegisterReply reply_;
    ServerAsyncResponseWriter<ClientRegisterReply> responder_;
};

class HeartBeatCallData : public CallData
{
public:
    HeartBeatCallData(DaemonServer::AsyncService* service, ServerCompletionQueue* cq);
    void Proceed();

private:
    HeartBeatRequest request_;
    HeartBeatReply reply_;
    ServerAsyncResponseWriter<HeartBeatReply> responder_;
};

}; // namespace daemon_server

#endif

