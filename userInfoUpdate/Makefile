CXX = g++ -I/home/l/grpc/include -I. \
	  -I/home/l/hiredis
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -L/home/l/grpc/libs/opt \
		   -L/usr/lib64/mysql \
		  `pkg-config --libs protobuf grpc++ grpc` \
		  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		  -ldl -lpthread \
		  -lmysqlclient -lm -lrt -lssl -lcrypto
SRC = ../common/daemonClient/*.cpp ../common/common.cpp \
	  *.cpp gen-cpp/*.cc \
	  /home/l/hiredis/libhiredis.a

all: user_info_update

user_info_update:
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm user_info_update gen-cpp -rf

