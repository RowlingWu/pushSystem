#include "common.h"

const char* USER_INFO_KEY[] = {
    "OFFICIAL_FANS_",
    "MONTH_ACTIVE_USER_",
    "PUSH_SWITCH_OPENED_"
};

const string TMP_USER_INFO_KEY = "TMP_USER_INFO_KEY";

const uint32_t SECONDS_PER_MINUTE = 60;

string genReleaseKey(const char* c)
{
    return string(c) + "RELEASE";
}

string genTempKey(const char* c)
{
    return string(c) + "TEMP";
}

string genDiffKey(const char* c)
{
    return string(c) + "DIFF";
}

double calLoadBalanceScore(double avgTime)
{
    return 10.0 * 500.0 / (avgTime + 500.0);
}
