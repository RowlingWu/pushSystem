#ifndef COMMON_COMMON
#define COMMON_COMMON

#include <iostream>
#include <string>
#include <stdint.h>
using namespace std;

extern const char* USER_INFO_KEY[];
extern const string TMP_USER_INFO_KEY;
extern const uint32_t SECONDS_PER_MINUTE;

extern string genReleaseKey(const char* c);
extern string genTempKey(const char* c);
extern string genDiffKey(const char* c);

extern double calLoadBalanceScore(double avgTime);

#endif
