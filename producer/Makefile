CXX = g++ -I/home/l/grpc/include -I. -I/home/l/rocketmq-client-cpp/include \
	  -I/home/l/hiredis

CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11 -ggdb
LDFLAGS += -L/usr/local/lib -L/home/l/grpc/libs/opt \
		   -L/home/l/rocketmq-client-cpp/bin \
		  `pkg-config --libs protobuf grpc++ grpc` \
		  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		  -ldl -lrocketmq -lboost_system -lboost_thread -lpthread
SRC = ../common/daemonClient/*.cpp ../common/*.cpp \
	  *.cpp gen-cpp/*.cc ../common/dataHandler/redisHandler.cpp \
	  /home/l/hiredis/libhiredis.a

all: producerd

producerd:
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm producerd gen-cpp -rf
