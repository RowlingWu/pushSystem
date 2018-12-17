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
    for (int i = 0; i < 1000; ++i)
    {
        req.set_group_id(i);
        client.ClientRegister(req);
    }
}

void Test()
{
    TestRequest req;
    for (int i = 0; i < 1000; ++i)
    {
        std::stringstream ss;
        ss << "number" << i;
        req.set_content(ss.str());
        client.Test(req);
    }
}

int main()
{
    std::thread thread_ = std::thread(&DaemonClientImpl::AsyncCompleteRpc, &client);

    std::thread req1 = std::thread(&ClientRegister);
    std::thread req2 = std::thread(&Test);

    std::cout << "sending...\n";
    thread_.join();
}
