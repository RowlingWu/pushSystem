#include "startPush.h"

using namespace start_push;
using namespace daemon_server;

int main()
{
    CompletionQueue cq;
    StartPush client(grpc::CreateChannel("192.168.99.100:50051", grpc::InsecureChannelCredentials()), &cq);

    BeginPushRequest req;
    req.set_msg_id(123456);
    req.set_start_uid(0);
    req.set_end_uid(200000000);
    client.BeginPush(req);

    thread thread_ = thread(&common::AsyncCompleteRpc, &client, &cq);
    thread_.join();
}
