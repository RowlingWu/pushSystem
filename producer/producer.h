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
    ProducerSendCallBack(ProduceMsgCallData* c, const string& topic_, const string& body_) : callData(c), topic(topic_), body(body_) {}
private:
    ProduceMsgCallData* callData;
    string topic;
    string body;
};

void InitProducer();
void AsyncProducerWorker(string& topic, string& body, ProduceMsgCallData* callData);

extern const int BITS_PER_BYTE;

}; // namespace producer

#endif
