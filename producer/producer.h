#ifndef PRODUCER
#define PRODUCER

#include "../common/daemonClient/server.h"
#include "../common/daemonClient/daemonClient.h"
#include "../common/handler_interface.h"
#include "../common/errCode.h"
#include "../common/rocketmq.h"
#include "../common/common.h"
#include "gen-cpp/producer.grpc.pb.h"

#include "hiredis.h"

using namespace daemon_client;
using namespace rocketmq;

namespace producer
{

class ProduceMsgCallData : public common::CallData
{
public:
    ProduceMsgCallData(Producer::AsyncService* service, ServerCompletionQueue* cq);
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
};


extern TpsReportService gTps;
extern RocketmqSendAndConsumerArgs gMQInfo;
extern DefaultMQProducer gMQProducer;

class ProducerSendCallBack : public AutoDeleteSendCallBack
{
    virtual void onSuccess(SendResult& result);
    virtual void onException(MQException& e);
public:
    ProducerSendCallBack(ProduceMsgCallData* c) : callData(c) {}
private:
    ProduceMsgCallData* callData;
};

void InitProducer();
void AsyncProducerWorker(string& topic, string& body, ProduceMsgCallData* callData);


class RedisHandler
{
public:
    RedisHandler();
    ~RedisHandler();
    bool connect();
    const redisReply* command(const char* const cmd);
    void freeConnection();
    void freeReply();

private:
    redisContext* context;
    redisReply* reply;
};

extern RedisHandler redisHandler;
extern mutex redisMtx;
const string TMP_USER_INFO_KEY;

}; // namespace producer

#endif
