#ifndef COMMON_REDIS_HANDLER
#define COMMON_REDIS_HANDLER

#include <iostream>
#include <mutex>
#include "hiredis.h"
using namespace std;

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

#endif
