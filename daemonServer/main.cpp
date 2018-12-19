#include "daemonServer.h"

using namespace daemon_server;

int main()
{
    ServerImpl server;
    server.Run();
}
