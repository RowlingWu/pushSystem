#ifndef COMMON_REDIS_HANDLER
#define COMMON_REDIS_HANDLER

#include <iostream>
#include <mutex>
#include <unistd.h>
#include "hiredis.h"
using namespace std;

class RedisHandler
{
public:
    RedisHandler();
    ~RedisHandler();
    bool connect();
    const redisReply* command(const char* const cmd);
    const redisReply* command(const char* const cmd, const char* const value, size_t size);
    void freeConnection();
    void freeReply();

private:
    redisContext* context;
    redisReply* reply;
};

extern RedisHandler redisHandler;
extern mutex redisMtx;

#endif
