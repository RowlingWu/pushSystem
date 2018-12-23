#include "consumer.h"

using namespace consumer;

string serverListeningPort = "50000";

int main()
{
    daemon_client::ServerImpl<Consumer> server;
    server.Run("localhost:" + serverListeningPort);
}
