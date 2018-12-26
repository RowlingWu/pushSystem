/*
* Licensed to the Apache Software Foundation (ASF) under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The ASF licenses this file to You under the Apache License, Version 2.0
* (the "License"); you may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef ROCKETMQ_CLIENT4CPP_EXAMPLE_COMMON_H_
#define ROCKETMQ_CLIENT4CPP_EXAMPLE_COMMON_H_

#include <boost/atomic.hpp>
#include <boost/chrono.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "Arg_helper.h"
#include "DefaultMQProducer.h"
#include "DefaultMQPullConsumer.h"
#include "DefaultMQPushConsumer.h"
using namespace std;

extern boost::atomic<int> g_msgCount;

class RocketmqSendAndConsumerArgs {
 public:
  RocketmqSendAndConsumerArgs()
      : body("msgbody for test"),
        thread_count(boost::thread::hardware_concurrency()),
        broadcasting(false),
        syncpush(false),
        SelectUnactiveBroker(false),
        IsAutoDeleteSendCallback(false),
        retrytimes(5),
        PrintMoreInfo(false) {}

 public:
  std::string namesrv;
  std::string namesrv_domain;
  std::string groupname;
  std::string topic;
  std::string body;
  int thread_count;
  bool broadcasting;
  bool syncpush;
  bool SelectUnactiveBroker;  // default select active broker
  bool IsAutoDeleteSendCallback;
  int retrytimes;  // default retry 5 times;
  bool PrintMoreInfo;
};

class TpsReportService {
 public:
  TpsReportService() : tps_interval_(10), quit_flag_(false), tps_count_(0) {}
  void start() {
    tps_thread_.reset(
        new boost::thread(boost::bind(&TpsReportService::TpsReport, this)));
  }

  ~TpsReportService() {
    quit_flag_.store(true);
    if (tps_thread_->joinable()) tps_thread_->join();
  }

  void Increment() { ++tps_count_; }

  void TpsReport() {
    while (!quit_flag_.load()) {
      boost::this_thread::sleep_for(tps_interval_);
      std::cout << "tps: " << tps_count_.load() << " per " << tps_interval_ << " sec\n";
      tps_count_.store(0);
    }
  }

 private:
  boost::chrono::seconds tps_interval_;
  boost::shared_ptr<boost::thread> tps_thread_;
  boost::atomic<bool> quit_flag_;
  boost::atomic<long> tps_count_;
};

extern void PrintResult(rocketmq::SendResult* result);

void PrintPullResult(rocketmq::PullResult* result);

extern void PrintRocketmqSendAndConsumerArgs(
    const RocketmqSendAndConsumerArgs& info);

extern void help();

extern bool ParseArgs(int argc, char* argv[],                   RocketmqSendAndConsumerArgs* info);

#endif  // ROCKETMQ_CLIENT4CPP_EXAMPLE_COMMON_H_
