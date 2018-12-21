#include "rocketmq.h"

boost::atomic<int> g_msgCount(1);

void PrintResult(rocketmq::SendResult* result)
{
  std::cout << "sendresult = " << result->getSendStatus()
            << ", msgid = " << result->getMsgId()
            << ", queueOffset = " << result->getQueueOffset() << ","
            << result->getMessageQueue().toString() << endl;
}

void PrintPullResult(rocketmq::PullResult* result)
{
  std::cout << result->toString() << std::endl;
  if (result->pullStatus == rocketmq::FOUND) {
    std::cout << result->toString() << endl;
    std::vector<rocketmq::MQMessageExt>::iterator it =
        result->msgFoundList.begin();
    for (; it != result->msgFoundList.end(); ++it) {
      cout << "=======================================================" << endl
           << (*it).toString() << endl;
    }
  }
}

void PrintRocketmqSendAndConsumerArgs(
    const RocketmqSendAndConsumerArgs& info)
{
  std::cout << "nameserver: " << info.namesrv << endl
            << "topic: " << info.topic << endl
            << "groupname: " << info.groupname << endl
            << "produce content: " << info.body << endl
            << "msg  count: " << g_msgCount.load() << endl
            << "thread count: " << info.thread_count << endl;
}

void help()
{
  std::cout
      << "need option,like follow: \n"
      << "-n nameserver addr, if not set -n and -i ,no nameSrv will be got \n"
         "-i nameserver domain name,  if not set -n and -i ,no nameSrv will be "
         "got \n"
         "-g groupname \n"
         "-t  msg topic \n"
         "-m messagecout(default value: 1) \n"
         "-c content(default value: only test ) \n"
         "-b (BROADCASTING model, default value: CLUSTER) \n"
         "-s sync push(default is async push)\n"
         "-r setup retry times(default value: 5 times)\n"
         "-u select active broker to send msg(default value: false)\n"
         "-d use AutoDeleteSendcallback by cpp client(defalut value: false) \n"
         "-T thread count of send msg or consume msg(defalut value: system cpu "
         "core number) \n"
         "-v print more details information \n";
}

bool ParseArgs(int argc, char* argv[],
                      RocketmqSendAndConsumerArgs* info)
{
#ifndef WIN32
  int ch;
  while ((ch = getopt(argc, argv, "n:i:g:t:m:c:b:s:h:r:T:bu")) != -1) {
    switch (ch) {
      case 'n':
        info->namesrv.insert(0, optarg);
        break;
      case 'i':
        info->namesrv_domain.insert(0, optarg);
        break;
      case 'g':
        info->groupname.insert(0, optarg);
        break;
      case 't':
        info->topic.insert(0, optarg);
        break;
      case 'm':
        g_msgCount.store(atoi(optarg));
        break;
      case 'c':
        info->body.insert(0, optarg);
        break;
      case 'b':
        info->broadcasting = true;
        break;
      case 's':
        info->syncpush = true;
        break;
      case 'r':
        info->retrytimes = atoi(optarg);
        break;
      case 'u':
        info->SelectUnactiveBroker = true;
        break;
      case 'T':
        info->thread_count = atoi(optarg);
        break;
      case 'v':
        info->PrintMoreInfo = true;
        break;
      case 'h':
        help();
        return false;
      default:
        help();
        return false;
    }
  }
#else
  rocketmq::Arg_helper arg_help(argc, argv);
  info->namesrv = arg_help.get_option_value("-n");
  info->namesrv_domain = arg_help.get_option_value("-i");
  info->groupname = arg_help.get_option_value("-g");
  info->topic = arg_help.get_option_value("-t");
  info->broadcasting = atoi(arg_help.get_option_value("-b").c_str());
  string msgContent(arg_help.get_option_value("-c"));
  if (!msgContent.empty()) info->body = msgContent;
  info->syncpush = atoi(arg_help.get_option_value("-s").c_str());
  int retrytimes = atoi(arg_help.get_option_value("-r").c_str());
  if (retrytimes > 0) info->retrytimes = retrytimes;
  info->SelectUnactiveBroker = atoi(arg_help.get_option_value("-u").c_str());
  int thread_count = atoi(arg_help.get_option_value("-T").c_str());
  if (thread_count > 0) info->thread_count = thread_count;
  info->PrintMoreInfo = atoi(arg_help.get_option_value("-v").c_str());
  g_msgCount = atoi(arg_help.get_option_value("-m").c_str());
#endif
  if (info->groupname.empty() || info->topic.empty() ||
      (info->namesrv_domain.empty() && info->namesrv.empty())) {
    std::cout << "please use -g to setup groupname and -t setup topic \n";
    help();
    return false;
  }
  return true;
}

