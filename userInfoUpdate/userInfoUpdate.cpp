#include "userInfoUpdate.h"

namespace user_info_update
{
    
const uint32_t COUNTS_PER_LOOP = 10240;
const uint64_t END_UID = 100000000;
const uint32_t DIFF_PERCENTAGE = 100;
vector<string> tableNames = {
    "official_fans",
    "month_active_users",
    "push_switch"
};

void CheckAndUpdateUserInfo(const int32_t tag)
{
    ostringstream cmd;
    cmd << "SELECT last_update_time "
        << "FROM user_info_update_record "
        << "WHERE type = " << tag;
    MYSQL_RES* res;
    if (!mysqlHandler.command(cmd.str().c_str(), res))
    {
        return;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row > 0)
    {
        stringstream ss;
        ss << row[0];
        uint64_t timestamp;
        ss >> timestamp;
        time_t now = time(NULL);
        tm tstm = *localtime((time_t*)(&timestamp));
        tm nowtm = *localtime(&now);
        if (tstm.tm_mday == nowtm.tm_mday &&
                tstm.tm_mon == nowtm.tm_mon)
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
    cout << "Begin updating " << USER_INFO_KEY[tag]
        << endl;

    string cmd = "DEL " + genTempKey(USER_INFO_KEY[tag]);
    if (!redisHandler.command(cmd.c_str()))
    {
        cout << "Stop update " << USER_INFO_KEY[tag]
            << endl;
        return;
    }
    redisHandler.freeReply();

    for (uint64_t uid = 0; uid <= END_UID;
            uid += COUNTS_PER_LOOP,
            mysqlHandler.freeResult())
    {
        // Read user infos (uids) from MySQL,
        // the number of uids is no more than
        // COUNTS_PER_LOOP in every for-loop
        MYSQL_RES* res;
        if (!mysqlHandler.command(GenSQL(tag, uid).c_str(), res))
        {
            cout << "Stop updating "
                << USER_INFO_KEY[tag] << endl;
            return;
        }

        // Convert uids into bitmap (the data structure
        // in Redis).
        // The offset of a bit represents the uid.
        // If the bit in the bitmap is set to 1,
        // the corresponding uid record exists in MySQL.
        // For example, bits[offset] = 1, means that
        // there exists a record uid = offset in MySQL.
        // Here we use std::string to store the
        // generated (sub)bitmap
        string bitmap(COUNTS_PER_LOOP / 8, 0);
        MYSQL_ROW row;
        while (true)
        {
            row = mysql_fetch_row(res);
            if (row <= 0)
            {
                break;
            }
            stringstream ss;
            uint64_t row2Uid;
            ss << row[0];
            ss >> row2Uid;
            const uint64_t offset = row2Uid - uid;
            bitmap[offset / 8] |= (1 << (7 - offset % 8));
        }

        // Writes this (sub)bitmap to Redis, using a 
        // corresponding TEMP_KEY
        ostringstream sscmd;
        sscmd <<  "SETRANGE "
            << genTempKey(USER_INFO_KEY[tag]) << " "
            << uid / 8 << " %b";  // offset
        if (!redisHandler.command(sscmd.str().c_str(),
                bitmap.c_str(), COUNTS_PER_LOOP / 8))
        {
            cout << "Stop updating "
                << USER_INFO_KEY[tag] << endl;
            return;
        }
        redisHandler.freeReply();
    }

    // Check whether RELEASE_KEY exists
    bool ok = false;
    cmd = "EXISTS " + genReleaseKey(USER_INFO_KEY[tag]);
    const redisReply* redisRes = redisHandler.command(cmd.c_str());
    if (NULL == redisRes)
    {
        cout << "Stop update " << USER_INFO_KEY[tag]
            << endl;
        return;
    }
    int isReleaseKeyExists = redisRes->integer;
    redisHandler.freeReply();
    if (!isReleaseKeyExists) // RELEASE_KEY not exists
    {
        cout << genReleaseKey(USER_INFO_KEY[tag])
            << " not exists, directly generate it\n";
        ok = true;
    }
    else
    {
        // Generate DIFF_KEY
        cout << "Checking diff between "
            << genReleaseKey(USER_INFO_KEY[tag])
            << " and " << genTempKey(USER_INFO_KEY[tag])
            << endl;
        cmd = "BITOP XOR "
            + genDiffKey(USER_INFO_KEY[tag]) + " "
            + genReleaseKey(USER_INFO_KEY[tag]) + " "
            + genTempKey(USER_INFO_KEY[tag]);
        if (redisHandler.command(cmd.c_str()) == NULL)
        {
            cout << "Stop update " << USER_INFO_KEY[tag]
                << endl;
            return;
        }
        redisHandler.freeReply();

        // check diff between RELEASE_KEY and TEMP_KEY
        cmd = "BITCOUNT "
            + genDiffKey(USER_INFO_KEY[tag]);
        redisRes = redisHandler.command(cmd.c_str());
        if (NULL == redisRes)
        {
            cout << "Stop update " << USER_INFO_KEY[tag]
                << endl;
            return;
        }
        uint32_t bitcounts = redisRes->integer;
        redisHandler.freeReply();
        double diff = (double)bitcounts * 100
            / (double)END_UID;
        cout << "Diff count:" << bitcounts
            << ". Diff percentage:" << diff << endl;
        if (diff < DIFF_PERCENTAGE)
        {
            ok = true;
        }

        // Del DIFF_KEY
        cmd = "DEL " + genDiffKey(USER_INFO_KEY[tag]);
        redisHandler.command(cmd.c_str());
        redisHandler.freeReply();
    }
    cout << (ok ? "Accept" : "Reject")
        << " to rename TEMP_KEY to RELEASE_KEY\n";
    // If ok, rename KEY
    if (ok)
    {
        cmd = "RENAME " + genTempKey(USER_INFO_KEY[tag])
            + " " + genReleaseKey(USER_INFO_KEY[tag]);
        if (redisHandler.command(cmd.c_str()) == NULL)
        {
            cout << "Stop update " << USER_INFO_KEY[tag]
                << endl;
            return;
        }
        redisHandler.freeReply();

        // Write last_update_time to mysql 
        ostringstream sscmd;
        time_t now = time(NULL);
        sscmd << "UPDATE user_info_update_record SET "
            << "last_update_time = " << (uint64_t)now
            << " WHERE type = " << tag;
        MYSQL_RES* res;
        if (!mysqlHandler.command(sscmd.str().c_str(), res))
        {
            cout << "Stop update " << USER_INFO_KEY[tag]
                << endl;
            return;
        }
        int32_t affectedRows = mysql_affected_rows(mysqlHandler.get());
        mysqlHandler.freeResult();
        if (affectedRows <= 0)
        {
            ostringstream sscmd;
            sscmd << "INSERT INTO user_info_update_record"
                << "(type, last_update_time) VALUES("
                << tag << "," << (uint64_t)now << ")";
            if (!mysqlHandler.command(sscmd.str().c_str(), res))
            {
                cout << "Stop update "
                    << USER_INFO_KEY[tag] << endl;
                return;
            }
            mysqlHandler.freeResult();
        }

        // Delete TMP_USER_INFO_KEY from Redis, because
        // the RELEASE_KEY has been updated, and the
        // TMP_USER_INFO_KEY should be rebuild by the
        // producer process
        cmd = "DEL " + TMP_USER_INFO_KEY;
        redisHandler.command(cmd.c_str());
        redisHandler.freeReply();
    }
    cout << "Finish updating " << USER_INFO_KEY[tag]
        << endl;
}

string GenSQL(const int32_t tag, const int64_t beginUid)
{
    ostringstream cmd;
    cmd << "SELECT uid FROM " << tableNames[tag]
        << " WHERE uid >= " << beginUid
        << " AND uid < " << beginUid + COUNTS_PER_LOOP;
    if (2 == tag)
    {
        cmd << " AND opened = 1";
    }
    return cmd.str();
}

}; // namespace user_info_update
