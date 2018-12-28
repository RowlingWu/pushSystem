#include "mysqlHandler.h"

MysqlHandler mysqlHandler;
mutex mysqlMtx;

MysqlHandler::MysqlHandler() :
    connection(NULL), result(NULL)
{
    connect();
}

bool MysqlHandler::connect()
{
    if (NULL != connection)
    {
        return true;
    }

    connection = mysql_init(NULL);
    if (NULL == connection)
    {
        cout << "mysql_init err:"
            << mysql_error(connection) << endl;
        return false;
    }
    connection = mysql_real_connection(connection, "localhost", "root", "Wo1bbbbzhul@", "user_info", 0, NULL, 0);
    if (NULL == connection)
    {
        cout << "mysql_real_connection err:"
            << mysql_error(connection) << endl;
        return false;
    }

    cout << "Connect to MySQL success\n";
    return true;
}

MysqlHandler::~MysqlHandler()
{
    freeConnection();
}

void MysqlHandler::freeConnection()
{
    if (NULL != connection)
    {
        mysql_close(connection);
        connection = NULL;
    }
}

const MYSQL_RES* MysqlHandler::command(const char* const cmd)
{
    for (int i = 0; i < 3; ++i)
    {
        if (mysql_query(connection, cmd))
        {
            cout << "mysql_query err:"
                << mysql_error(connection) << endl
                << "Retry cmd `" << cmd
                << "` and reconnect for "
                << i + 1 << " time\n";
            freeConnection();
            connect();
        }
        else
        {
            result = mysql_use_result(connection);
            return result;
        }
    }
    cout << "Error:Give up mysql_query\n";
    return NULL;
}

void MysqlHandler::freeResult()
{
    if (NULL != result)
    {
        mysql_free_result(result);
        result = NULL;
    }
}

