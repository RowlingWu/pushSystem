#include "consumer.h"

namespace daemon_client
{

template<>
void ServerImpl<consumer::Consumer>::HandleRpcs()
{
    thread t = thread(&consumer::InitConsumer);
    new consumer::ConsumerCallData(&service_, cq_.get());

    void* tag;
    bool ok;
    while (true)
    {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        static_cast<common::CallData*>(tag)->Proceed();
    }
}

}; // namespace daemon_client


namespace consumer
{

TpsReportService gTps;
DefaultMQPushConsumer consumer("please_rename_unique_group_name");
MsgListener msglistener;

void InitConsumer()
{
    consumer.setNamesrvAddr("192.168.99.100:9876");
    consumer.setGroupName("consumer");
    consumer.setNamesrvDomain("push");
    consumer.setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);

    consumer.setInstanceName("consumer");

    consumer.subscribe("push", "*");
    consumer.setConsumeThreadCount(15);
    consumer.setTcpTransportTryLockTimeout(1000);
    consumer.setTcpTransportConnectTimeout(400);

    consumer.registerMessageListener(&msglistener);

    try {
        cout << "Starting consumer...\n";
        consumer.start();
    } catch (MQClientException &e) {
        cout << "StartConsumerException:" << e << endl;
    }
    gTps.start();
}

ConsumeStatus MsgListener::consumeMessage(const std::vector<MQMessageExt> &msgs)
{
    for (size_t i = 0; i < msgs.size(); ++i)
    {
        gTps.Increment();
        producer::ProducerMsg msg;
        msg.ParseFromString(msgs[i].getBody());
        cout << "[" << __func__ << "]" << "toUid:" << msg.to_uid() << " msgId:" << msg.msg_id() << endl;
    }

    return CONSUME_SUCCESS;
}

ConsumerCallData::ConsumerCallData(Consumer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void ConsumerCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestConsumeMsg(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new ConsumerCallData(service_, cq_);

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

}; // namespace consumer
