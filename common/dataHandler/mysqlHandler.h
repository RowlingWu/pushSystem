#ifndef COMMON_MYSQL_HANDLER
#define COMMON_MYSQL_HANDLER

#include <iostream>
#include <mutex>
#include <mysql/mysql.h>
using namespace std;

class MysqlHandler
{
public:
    MysqlHandler();
    ~MysqlHandler();
    bool connect();
    void freeConnection();
    const MYSQL_RES* command(const char* const cmd);
    void freeResult();

private:
    MYSQL* connection;
    MYSQL_RES* result;
};

extern MysqlHandler mysqlHandler;
extern mutex mysqlMtx;

#endif
