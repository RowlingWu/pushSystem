CXX = g++ -I/home/l/grpc/include -I. -I/usr/local/include/rocketmq
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -L/home/l/grpc/libs/opt -L/usr/local/lib\
		  `pkg-config --libs protobuf grpc++ grpc` \
		  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		  -ldl -lrocketmq -lboost_system -lboost_thread -lpthread
SRC = ../common/*.cpp \
	  *.cpp gen-cpp/*.cc

all: consumer

consumer:
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm consumer gen-cpp -rf
