#ifndef PRODUCER
#define PRODUCER

#include "../common/daemonClient/server.h"
#include "../common/daemonClient/daemonClient.h"
#include "../common/dataHandler/redisHandler.h"
#include "../common/handler_interface.h"
#include "../common/errCode.h"
#include "../common/rocketmq.h"
#include "../common/common.h"
#include "gen-cpp/producer.grpc.pb.h"

#include "float.h"

using namespace daemon_client;
using namespace rocketmq;

namespace producer
{

// Producer makes grpc calls through this class,
// including sending load balance info to daemonServer
class ProducerImpl
{
public:
    explicit ProducerImpl(shared_ptr<Channel> channel);
    void SendLoadBalanceInfo();
    //void CalLoadBalanceInfo(chrono::time_point<chrono::high_resolution_clock>& startTime, chrono::time_point<chrono::high_resolution_clock> endTime);

private:
    void LoadBalance(LoadBalanceRequest& req);

private:
    struct LoadBalanceAsyncCall : public AsyncCall
    {
        LoadBalanceReply reply;
        unique_ptr<ClientAsyncResponseReader<LoadBalanceReply>> response_reader;
        void OnGetResponse(void*);
    };

private:
    /*mutex loadBalanceMutex;
    uint64_t totalTasks;
    double avgTime;*/

    unique_ptr<DaemonServer::Stub> stub_;
    CompletionQueue cq_;
    thread handleCallBackThread_;
};


class ProduceMsgCallData : public common::CallData
{
public:
    ProduceMsgCallData(Producer::AsyncService* service, ServerCompletionQueue* cq, ProducerImpl* p);
    void Proceed();
    void NotifyOne();

private:
    int32_t ProduceMsg(uint32_t msgId, uint64_t startUid, uint64_t endUid);

private:
    Producer::AsyncService* service_;
    ProduceMsgRequest request_;
    ProduceMsgReply reply_;
    ServerAsyncResponseWriter<ProduceMsgReply> responder_;
    uint32_t waitForSendCount;
    mutex waitForSndCntMtx;

    //chrono::time_point<chrono::high_resolution_clock> startTime;
    //chrono::time_point<chrono::high_resolution_clock> endTime;
    ProducerImpl* pProducerImpl;
};


extern TpsReportService gTps;
extern RocketmqSendAndConsumerArgs gMQInfo;
extern DefaultMQProducer gMQProducer;
extern mutex gMQProducerMtx;

class ProducerSendCallBack : public AutoDeleteSendCallBack
{
    virtual void onSuccess(SendResult& result);
    virtual void onException(MQException& e);
public:
    ProducerSendCallBack(ProduceMsgCallData* c, const string& topic_, const string& body_) : callData(c), topic(topic_), body(body_) {}
private:
    ProduceMsgCallData* callData;
    string topic;
    string body;
};

void InitProducer();
void AsyncProducerWorker(string& topic, string& body, ProduceMsgCallData* callData);

extern const int BITS_PER_BYTE;



extern void SetDaemonClientImpl(DaemonClientImpl* p);
extern DaemonClientImpl* pDaemonClientImpl;

}; // namespace producer

#endif
