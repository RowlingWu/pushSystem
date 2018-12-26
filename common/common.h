#ifndef COMMON_COMMON
#define COMMON_COMMON

const char* USER_INFO_KEY = {
    "OFFICIAL_FANS_",
    "MONTH_ACTIVE_USER_",
    "PUSH_SWITCH_OPENED_"
};

const uint32_t SECONDS_PER_MINUTE = 60;

string genReleaseKey(const char* c)
{
    return string(c) + "RELEASE";
}

string genTempKey(const char* c)
{
    return string(c) + "TEMP";
}

#endif
