#include "common.h"

const char* USER_INFO_KEY[] = {
    "OFFICIAL_FANS_",
    "MONTH_ACTIVE_USER_",
    "PUSH_SWITCH_OPENED_"
};

const uint32_t SECONDS_PER_MINUTE = 60;

extern string genReleaseKey(const char* c)
{
    return string(c) + "RELEASE";
}

extern string genTempKey(const char* c)
{
    return string(c) + "TEMP";
}


