#include "daemonClient.h"
#include <sstream>
#include <unistd.h>

using namespace daemon_client;

string serverListeningPort = "50052";
string procName = "daemon_client";
uint32_t groupId = 602;

int main()
{
    DaemonClientImpl client(grpc::CreateChannel("192.168.99.100:50051", grpc::InsecureChannelCredentials()), serverListeningPort, procName, groupId);
    std::thread thread_ = std::thread(&DaemonClientImpl::AsyncCompleteRpc, &client);

    std::cout << "sending...\n";
    thread_.join();
}
