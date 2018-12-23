#ifndef CONSUMER
#define CONSUMER

#include <stdlib.h>
#include <string.h>

#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "../common/rocketmq.h"
#include "../common/daemonClient/server.h"
#include "../common/handler_interface.h"
#include "gen-cpp/consumer.grpc.pb.h"
#include "gen-cpp/producer.grpc.pb.h"

using namespace std;
using namespace rocketmq;

namespace consumer
{

class MsgListener : public MessageListenerConcurrently {
 public:
  MsgListener() {}
  virtual ~MsgListener() {}

  virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt> &msgs);
};

extern MsgListener msglistener;
extern DefaultMQPushConsumer consumer;
extern TpsReportService gTps;
void InitConsumer();


class ConsumerCallData : public common::CallData
{
public:
    ConsumerCallData(Consumer::AsyncService* service, ServerCompletionQueue* cq);
    void Proceed();

private:
    Consumer::AsyncService* service_;
    ConsumeMsgReply reply_;
    ConsumeMsgRequest request_;
    ServerAsyncResponseWriter<ConsumeMsgReply> responder_;
};



}; // namespace consumer

#endif
