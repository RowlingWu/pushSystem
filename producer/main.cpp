#include "producer.h"

using namespace daemon_client;
using namespace producer;

string serverListeningPort = "50053";
string procName = "producer";
uint32_t groupId = 601;

int main()
{
    DaemonClientImpl client(grpc::CreateChannel("192.168.99.100:50051", grpc::InsecureChannelCredentials()), serverListeningPort, procName, groupId);
    std::thread thread_ = std::thread(&DaemonClientImpl::AsyncCompleteRpc, &client);

    cout << "1\n";
    daemon_client::ServerImpl<Producer> server;
    cout << "2\n";
    server.Run("localhost:" + serverListeningPort);
}
