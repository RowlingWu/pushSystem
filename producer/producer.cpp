#include "producer.h"

namespace daemon_client
{

template<>
void ServerImpl<producer::Producer>::HandleRpcs()
{
    producer::InitProducer();
    new producer::ProduceMsgCallData(&service_, cq_.get());

    producer::ProducerImpl producerImpl(
            grpc::CreateChannel(
                "192.168.99.100:50051",
                grpc::InsecureChannelCredentials())
    );
    thread t = thread(&producer::ProducerImpl::SendLoadBalanceInfo, &producerImpl);
    t.detach();

    void* tag;
    bool ok;
    while (true)
    {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        if (ok)
        {
            thread t = thread(&common::CallData::Proceed, static_cast<common::CallData*>(tag));
            t.detach();
        }
        else
        {
            cout << "GPR_ASSERT fail!\n";
            delete static_cast<common::CallData*>(tag);
        }
    }
}

}; // namespace daemon_client

namespace producer
{

TpsReportService gTps;
RocketmqSendAndConsumerArgs gMQInfo;
DefaultMQProducer gMQProducer("rename_group_name");
mutex gMQProducerMtx;
atomic<int32_t> taskCount(0);
const int BITS_PER_BYTE = 8;

ProduceMsgCallData::ProduceMsgCallData(Producer::AsyncService* service, ServerCompletionQueue* cq) :
    CallData(cq), service_(service),
    responder_(&ctx_)
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

        ++taskCount;
        ProduceMsg(request_.msg_id(), request_.start_uid(), request_.end_uid());
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
        --taskCount;
cout << "Finish producing msg. Notify daemon. startUid:"
    << request_.start_uid()
    << " endUid:" << request_.end_uid() << endl;
        status_ = FINISH;
        reply_.set_err(common::SUCCESS);
        responder_.Finish(reply_, Status::OK, this);
    }
    waitForSndCntMtx.unlock();
}

int32_t ProduceMsgCallData::ProduceMsg(uint32_t msgId, uint64_t startUid, uint64_t endUid)
{
    redisMtx.lock();
    // Check KEY exists
    string cmd = "EXISTS " + TMP_USER_INFO_KEY;
    const redisReply* redisReply = redisHandler.command(cmd.c_str());
    if (NULL == redisReply)
    {
        redisMtx.unlock();
        cout << "RedisErr:EXISTS cmd fail\n";
        return common::ERR;
    }
    if (!redisReply->integer) // KEY not exists
    {
        redisHandler.freeReply();
        // Create TMP_USER_INFO_KEY
        cmd = "BITOP AND " + TMP_USER_INFO_KEY
            + " " + genReleaseKey(USER_INFO_KEY[0])
            + " " + genReleaseKey(USER_INFO_KEY[1])
            + " " + genReleaseKey(USER_INFO_KEY[2]);
        redisReply = redisHandler.command(cmd.c_str());
        if (NULL == redisReply ||
                redisReply->integer <= 0)
        {
            redisMtx.unlock();
            cout << "RedisErr:" << TMP_USER_INFO_KEY
                << " length is 0!\n";
            return common::ERR;
        }
        redisHandler.freeReply();

        // Set expiration of TMP_USER_INFO_KEY
        ostringstream sscmd;
        sscmd << "EXPIRE " << TMP_USER_INFO_KEY << " "
            << 30 * SECONDS_PER_MINUTE;
        redisReply = redisHandler.command(sscmd.str().c_str());
        if (NULL == redisReply || !redisReply->integer)
        {
            redisMtx.unlock();
            cout << "RedisErr:"
                << "fail to set key expiration\n";
            return common::ERR;
        }
        redisHandler.freeReply();
    }
    else
    {
        redisHandler.freeReply();
    }

    // Get start-end range of the KEY
    ostringstream sscmd;
    sscmd << "GETRANGE " << TMP_USER_INFO_KEY << " "
        << startUid / 8 << " " << endUid / 8;
    redisReply = redisHandler.command(sscmd.str().c_str());
    if (NULL == redisReply)
    {
        redisMtx.unlock();
        cout << "RedisErr:GETRANGE cmd fail\n";
        return common::ERR;
    }
    const size_t len = endUid / 8 - startUid / 8 + 1;
    const string userInfoStr(redisReply->str, len);
    redisHandler.freeReply();
    redisMtx.unlock();

    // Generate vector<to_uids>
    vector<uint64_t> uidsToSend;
    for (size_t i = 0; i < len; ++i)
    {
        for (int j = 0; j < BITS_PER_BYTE; ++j)
        {
            if (userInfoStr[i] & (1 << (7 - j)))
            {
                uidsToSend.push_back(startUid + BITS_PER_BYTE * i + j);
            }
        }
    }

    // Producer sends
    waitForSendCount = uidsToSend.size();
    const size_t uidCount = uidsToSend.size();
    if (uidCount)
    {
        for (size_t i = 0; i < uidCount; ++i)
        {
            ProducerMsg msg;
            msg.set_to_uid(uidsToSend[i]);
            msg.set_msg_id(msgId);
            string body;
            msg.SerializeToString(&body);
            AsyncProducerWorker(gMQInfo.topic, body, this);
        }
    }
    else
    {
        status_ = FINISH;
        reply_.set_err(common::SUCCESS);
        responder_.Finish(reply_, Status::OK, this);
    }
    return common::SUCCESS;
}


void ProducerSendCallBack::onSuccess(SendResult& result)
{
    gTps.Increment();

    callData->NotifyOne();
}

void ProducerSendCallBack::onException(MQException& e)
{
    cout << "SendToBrokerException: " << e.what()
       << "\n";
    thread t = thread(&RetrySending, topic, body, callData);
    t.detach();
}

void RetrySending(string topic, string body, ProduceMsgCallData* callData)
{
    sleep(1);
    AsyncProducerWorker(topic, body, callData);
}

void AsyncProducerWorker(string& topic, string& body, ProduceMsgCallData* callData)
{
    MQMessage msg(topic, // topic
            "*",   // tag
            body); // body
    try
    {
        gMQProducer.send(msg, new ProducerSendCallBack(callData, topic, body));
    }
    catch (MQException& e)
    {
        cout << __func__ << " exception:" << e.what() << ".Retry sending..." << endl;
        sleep(1);
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


ProducerImpl::ProducerImpl(shared_ptr<Channel> channel) :
    stub_(DaemonServer::NewStub(channel))
{
    handleCallBackThread_ = thread(&common::AsyncCompleteRpc, this, &cq_);
}

void ProducerImpl::LoadBalanceAsyncCall::OnGetResponse(void*)
{
}

void ProducerImpl::LoadBalance(LoadBalanceRequest& req)
{
    LoadBalanceAsyncCall* call = new LoadBalanceAsyncCall;
    call->response_reader = stub_->PrepareAsyncLoadBalance(&call->context, req, &cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
}

void ProducerImpl::SendLoadBalanceInfo()
{
    LoadBalanceRequest req;
    while (true)
    {
        sleep(10);

        uint64_t serverId;  // get server id
        if (NULL != pDaemonClientImpl &&
            pDaemonClientImpl->GetServerId(serverId))
        {
            req.set_server_id(serverId);
        }
        else
        {
            continue;
        }

        double avgTime = gTps.GetAvgSpeed();
        stringstream ssScore;   // get score
        double score;
        int32_t curTaskCount = taskCount.load();
        if (avgTime < 0.0001 && curTaskCount)
        { // producer may not be able to produce msg
            score = 0.0001;
        }
        else
        {
            score = calLoadBalanceScore(avgTime);
        }
        ssScore << score;
        req.set_score(ssScore.str());
cout << "[" << __func__ << "]avgTime:" << avgTime
    << " score:" << score
    << " curTaskCount:" << curTaskCount << endl;

        LoadBalance(req);
    }
}

DaemonClientImpl* pDaemonClientImpl = NULL;

void SetDaemonClientImpl(DaemonClientImpl* p)
{
    pDaemonClientImpl = p;
}

}; // namespace producer
