#include "userInfoUpdate.h"

void CheckAndUpdateUserInfo(const int32_t tag)
{
    ostringstream cmd;
    cmd << "SELECT last_update_time "
        << "FROM user_info_update_record "
        << "WHERE type = " << tag;
    MYSQL_RES* res = mysqlHandler.command(cmd.str().c_str());
    if (!res)
    {
        return;
    }
    if (mysql_num_rows(res) > 0)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        time_t timestamp = (time_t)row[0];
        if (/* day is today*/)
        {
            mysqlHandler.freeResult();
            return;
        }
    }
    mysqlHandler.freeResult();
    UpdateUserInfo(tag);
}

void UpdateUserInfo(const int32_t tag)
{
    cout << "Begin update " << USER_INFO_KEY[i] << endl;

    // read mysql for 1024 users per time
    // write to Redis using TEMP_KEY
    // finish while loop
    // check diff between RELEASE_KEY and TEMP_KEY
    // if ok, rename KEY
    // write last_update_time to mysql 
}


}; // namespace user_info_update
