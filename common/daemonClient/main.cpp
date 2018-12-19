#include "daemonClient.h"
#include <sstream>
#include <unistd.h>

using namespace daemon_client;

DaemonClientImpl client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

void ClientRegister()
{
    ClientRegisterRequest req;
    req.set_address("0.0.0.0:5007");
    req.set_proc_name("cl");
    for (int i = 0; i < 1; ++i)
    {
        req.set_group_id(i);
        client.ClientRegister(req);
        if (i % 1000 == 0)
        {
            usleep(1000);
        }
    }
}

int main()
{
    std::thread thread_ = std::thread(&DaemonClientImpl::AsyncCompleteRpc, &client);

    std::thread req1 = std::thread(&ClientRegister);

    std::cout << "sending...\n";
    thread_.join();
}
