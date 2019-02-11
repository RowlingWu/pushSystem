CXX = g++ -I/home/l/grpc/include -I.
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11 -ggdb -fpermissive
LDFLAGS += -L/usr/local/lib -L/home/l/grpc/libs/opt \
		  `pkg-config --libs protobuf grpc++ grpc` \
		  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		  -ldl
SRC = *.cpp gen-cpp/*.cc \
	  ../common/daemonClient/daemonClient.cpp \
	  ../common/common.cpp

all: daemon_server

daemon_server:
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm daemon_server gen-cpp -rf
