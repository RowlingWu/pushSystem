#include "producer.h"

namespace daemon_client
{
using namespace producer;

template<>
void ServerImpl<producer::Producer>::HandleRpcs()
{
    InitProducer();
    new producer::ProduceMsgCallData(&service_, cq_.get());

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

namespace producer
{

TpsReportService gTps;
RocketmqSendAndConsumerArgs gMQInfo;
DefaultMQProducer gMQProducer("rename_group_name");

ProduceMsgCallData::ProduceMsgCallData(Producer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service), responder_(&ctx_)
{
    Proceed();
}

void ProduceMsgCallData::Proceed()
{
    if (status_ == CREATE)
    {
        status_ = PROCESS;
        service_->RequestProduceMsg(&ctx_, &request_, &responder_, cq_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
        new ProduceMsgCallData(service_, cq_);

        reply_.set_err(common::ERR);
        cout << "Start producing msg. msgId:"
            << request_.msg_id()
            << " startUid:" << request_.start_uid()
            << " endUid:" << request_.end_uid() << endl;

        ProduceMsg(request_.msg_id(), request_.start_uid(), request_.end_uid());

        //status_ = FINISH;
        //responder_.Finish(reply_, Status::OK, this);
    }
    else
    {
        GPR_ASSERT(status_ == FINISH);
        delete this;
    }
}

void ProduceMsgCallData::NotifyOne()
{
    waitForSndCntMtx.lock();
    --waitForSendCount;
    if (0 == waitForSendCount)
    {
        status_ = FINISH;
        reply_.set_err(common::SUCCESS);
        responder_.Finish(reply_, Status::OK, this);
    }
    waitForSndCntMtx.unlock();
}

int32_t ProduceMsgCallData::ProduceMsg(uint32_t msgId, uint64_t startUid, uint64_t endUid)
{
    waitForSendCount = endUid - startUid + 1;
    for (uint64_t uid = startUid; uid <= endUid; ++uid)
    {
        // TODO: read SSDB for user info
        // produce msg to broker
        ProducerMsg msg;
        msg.set_to_uid(uid);
        msg.set_msg_id(msgId);
        string body;
        msg.SerializeToString(&body);
        AsyncProducerWorker(gMQInfo.topic, body, this);
    }
}


void ProducerSendCallBack::onSuccess(SendResult& result)
{
    gTps.Increment();
    cout << "SendToBrokerSuccess.";
    PrintResult(&result);

    callData->NotifyOne();
}

void ProducerSendCallBack::onException(MQException& e)
{
    cout << "SendToBrokerException: " << e.what() << endl;
}

void AsyncProducerWorker(string& topic, string& body, ProduceMsgCallData* callData)
{
    MQMessage msg(topic, // topic
            "*",   // tag
            body); // body
    try
    {
        gMQProducer.send(msg, new ProducerSendCallBack(callData));
    }
    catch (MQException& e)
    {
        cout << __func__ << " exception:" << e.what() << ".Retry sending..." << endl;
        AsyncProducerWorker(topic, body, callData);
    }
}

void InitProducer()
{
    gMQInfo.namesrv = "192.168.99.100:9876";
    gMQInfo.groupname = "producer";
    gMQInfo.namesrv_domain = "push";
    gMQInfo.topic = "push";

    gMQProducer.setNamesrvAddr(gMQInfo.namesrv);
    gMQProducer.setGroupName(gMQInfo.groupname);
    gMQProducer.setNamesrvDomain(gMQInfo.namesrv_domain);
    gMQProducer.start();
    gTps.start();
}

}; // namespace producer
