#include "userInfoUpdate.h"

using namespace user_info_update;

int main()
{
    while (true)
    {
        CheckAndUpdateUserInfo(0); //OFFICIAL_FANS
        CheckAndUpdateUserInfo(1); //MONTH_ACTIVE_USER
        CheckAndUpdateUserInfo(2); //PUSH_SWITCH_OPENED

        sleep(30 * 60);
    }
    cout << "Process Exit\n";
}
