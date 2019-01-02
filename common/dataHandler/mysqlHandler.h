#ifndef COMMON_MYSQL_HANDLER
#define COMMON_MYSQL_HANDLER

#include <iostream>
#include <mutex>
#include <mysql/mysql.h>
#include <unistd.h>
using namespace std;

class MysqlHandler
{
public:
    MysqlHandler();
    ~MysqlHandler();
    MYSQL* get();
    bool connect();
    void freeConnection();
    bool command(const char* const cmd, MYSQL_RES*& res);
    void freeResult();

private:
    MYSQL* connection;
    MYSQL_RES* result;
};

extern MysqlHandler mysqlHandler;
extern mutex mysqlMtx;

#endif
