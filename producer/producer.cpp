#include "producer.h"

namespace daemon_client
{

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
RedisHandler redisHandler;
mutex redisMtx;
const string TMP_USER_INFO_KEY = "TMP_USER_INFO_KEY";

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
    redisMtx.lock();
    // Check KEY exists
    string cmd = "EXISTS " + TMP_USER_INFO_KEY;
    const redisReply* redisReply = redisHandler.command(cmd.c_str());
    if (NULL == redisReply)
    {
        redisMtx.unlock();
        return common::ERR;
    }
    if (!redisReply->integer) // KEY not exists
    {
        // TODO: create KEY
        cmd = "BITOP AND " + TMP_USER_INFO_KEY
            + " " + genReleaseKey(USER_INFO_KEY[0])
            + " " + genReleaseKey(USER_INFO_KEY[1])
            + " " + genReleaseKey(USER_INFO_KEY[2]);
        redisReply = redisHandler.command(cmd.c_str());
        if (NULL == redisReply ||
                redisReply->integer <= 0)
        {
            redisMtx.unlock();
            cout << "RedisErr:" << (NULL == redisReply
                    ? "redisReplyNULL." : "")
                << TMP_USER_INFO_KEY
                << " length is 0!\n";
            return common::ERR;
        }
        ostringstream sscmd;
        sscmd << "EXPIRE " << TMP_USER_INFO_KEY << " "
            << 60 * SECONDS_PER_MINUTE;
        redisReply = redisHandler.command(sscmd.str().c_str());
        if (NULL == redisReply || !redisReply->integer)
        {
            redisMtx.unlock();
            cout << "RedisErr:" << (NULL == redisReply
                    ? "redisReplyNULL." : "")
                << "fail to set key expiration\n";
            return common::ERR;
        }
    }
    // Get start-end range of the KEY
    // Generate vector<to_uids>
    // Producer sends
    redisHandler.freeReply();
    redisMtx.unlock();

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


RedisHandler::RedisHandler() :
    context(NULL), reply(NULL)
{
    connect();
}

RedisHandler::~RedisHandler()
{
    freeConnection();
}

bool RedisHandler::connect()
{
    if (NULL != context)
    {
        return true;
    }

    for (uint32_t i = 0; i < 3; ++i)
    {
        context = redisConnect("127.0.0.1", 6379);
        if (c == NULL || c->err)
        {
            if (c)
            {
                cout << "RedisConnectionErr:"
                    << c->errstr << endl;
                redisFree(context);
            }
            else
            {
                cout << "RedisConnectionErr:"
                    << "can't allocate redis context\n";
            }
            cout << "Retry connecting for "
                << i + 1 << " time\n";
        }
        else
        {
            cout << "RedisConnectionSuccess.\n";
            return true;
        }
    }
    cout << "Give up redis connection\n";
    return false;
}

const redisReply* RedisHandler::command(const char* const cmd)
{
    if (NULL == context && !connect())
    {
        return NULL;
    }
    for (uint32_t i = 0; i < 3; ++i)
    {
        reply = (redisReply*)redisCommand(context, cmd);
        if (NULL == reply)
        {
            cout << "RedisCommandErr:" << context->err
                << " desc:" << context->errstr
                << ". Retry for the " << i + 1 << "time\n";
        }
        else
        {
            return reply;
        }
    }
    cout << "RedisCommandErr:give up the command!\n";
    return NULL;
}

void RedisHandler::freeConnection()
{
    if (context != NULL)
    {
        redisFree(context);
    }
}

void RedisHandler::freeReply()
{
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }
}

RedisHandler redisHandler;
mutex redisMtx;

}; // namespace producer
