CXX = g++ -I/home/l/grpc/include
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -L/home/l/grpc/libs/opt \
		  `pkg-config --libs protobuf grpc++ grpc` \
		  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		  -ldl

all: daemon_client

daemon_client:
	$(CXX) $(CXXFLAGS) *.cpp gen-cpp/*.cc $(LDFLAGS) -o $@

clean:
	rm daemon_client gen-cpp -rf
