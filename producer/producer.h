#ifndef PRODUCER
#define PRODUCER

#include "../common/daemonClient/server.h"
#include "../common/daemonClient/daemonClient.h"
#include "../common/handler_interface.h"
#include "../common/errCode.h"
#include "../common/rocketmq.h"
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
    void FinishProceed(ProduceMsgReply& reply);

private:
    int32_t ProduceMsg(uint32_t msgId, uint64_t startUid, uint64_t endUid);

private:
    Producer::AsyncService* service_;
    ProduceMsgRequest request_;
    ProduceMsgReply reply_;
    ServerAsyncResponseWriter<ProduceMsgReply> responder_;
};


extern TpsReportService gTps;
extern RocketmqSendAndConsumerArgs gMQInfo;
extern DefaultMQProducer gMQProducer;

class ProducerSendCallBack : public SendCallback
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

}; // namespace producer

#endif
