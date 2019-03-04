CXX = g++ -I/home/l/grpc/include -I. -I/home/l/rocketmq-client-cpp/include
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -L/home/l/grpc/libs/opt -L/usr/local/lib\
		   -L/home/l/rocketmq-client-cpp/bin \
		  `pkg-config --libs protobuf grpc++ grpc` \
		  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		  -ldl -lrocketmq -lboost_system -lboost_thread -lpthread
SRC = ../common/*.cpp ../common/daemonClient/daemonClient.cpp \
	  *.cpp gen-cpp/*.cc

all: consumerd

consumerd:
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm consumerd gen-cpp -rf
