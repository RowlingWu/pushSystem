#include "redisHandler.h"

RedisHandler redisHandler;
mutex redisMtx;

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
        if (context == NULL || context->err)
        {
            if (context)
            {
                cout << "RedisConnectionErr:"
                    << context->errstr << endl;
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
    context = NULL;
    return false;
}

const redisReply* RedisHandler::command(const char* const cmd)
{
    if (NULL == context && !connect())
    {
        cout << "While executing cmd`" << cmd << "`"
            << ", connection fail!\n";
        return NULL;
    }
    for (uint32_t i = 0; i < 3; ++i)
    {
        reply = (redisReply*)redisCommand(context, cmd);
        if (NULL == reply)
        {
            cout << "RedisCommandErr: cmd`" << cmd
                << "` err:" << context->err
                << " desc:" << context->errstr
                << ". Retry for the " << i + 1 << "time\n";
            freeConnection();
            connect();
        }
        else if (reply->type == REDIS_REPLY_ERROR)
        {
            cout << "CMD:" << cmd 
                << ". REDIS_REPLY_ERROR:" << reply->str
                << endl;
        }
        else if (reply->type == REDIS_REPLY_NIL)
        {
            cout << "CMD:" << cmd <<
                ". No data to access\n";
            return reply;
        }
        else
        {
            return reply;
        }
        sleep(1);
    }
    cout << "RedisCommandErr:give up the command!\n";
    return NULL;
}

const redisReply* RedisHandler::command(const char* const cmd, const char* const value, size_t size)
{
    if (NULL == context && !connect())
    {
        cout << "While executing cmd`" << cmd << "`"
            << ", connection fail!\n";
        return NULL;
    }
    for (uint32_t i = 0; i < 3; ++i)
    {
        reply = (redisReply*)redisCommand(context, cmd, value, size);
        if (NULL == reply)
        {
            cout << "RedisCommandErr: cmd`" << cmd
                << "` err:" << context->err
                << " desc:" << context->errstr
                << ". Retry for the " << i + 1 << "time\n";
            freeConnection();
            connect();
        }
        else if (reply->type == REDIS_REPLY_ERROR)
        {
            cout << "CMD:" << cmd 
                << ". REDIS_REPLY_ERROR:" << reply->str
                << endl;
        }
        else if (reply->type == REDIS_REPLY_NIL)
        {
            cout << "CMD:" << cmd <<
                ". No data to access\n";
            return reply;
        }
        else
        {
            return reply;
        }
        sleep(1);
    }
    cout << "RedisCommandErr:give up the command!\n";
    return NULL;
}

void RedisHandler::freeConnection()
{
    if (context != NULL)
    {
        redisFree(context);
        context = NULL;
    }
}

void RedisHandler::freeReply()
{
    if (reply != NULL)
    {
        freeReplyObject(reply);
        reply = NULL;
    }
}

