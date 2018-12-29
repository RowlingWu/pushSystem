#ifndef USER_INFO_UPDATE
#define USER_INFO_UPDATE

#include <stdlib.h>
#include <string.h>

#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <sstream>

#include "../common/dataHandler/redisHandler.h"
#include "../common/dataHandler/mysqlHandler.h"
#include "../common/common.h"

using namespace std;

namespace user_info_update
{

extern vector<string> tableNames;
extern const uint32_t COUNTS_PER_LOOP;
extern const uint64_t END_UID;
extern const uint32_t DIFF_PERCENTAGE;

extern void CheckAndUpdateUserInfo(const int32_t tag);
extern void UpdateUserInfo(const int32_t tag);
extern string GenSQL(const int32_t tag, const int64_t lastUid);

};  // namespace user_info_update

#endif
