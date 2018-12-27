CXX = g++ -I/home/l/grpc/include -I. -I/usr/local/include/rocketmq \
	  -I/home/l/hiredis
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -L/home/l/grpc/libs/opt -L/usr/local/lib\
		   -L/usr/lib64/mysql \
		  `pkg-config --libs protobuf grpc++ grpc` \
		  -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
		  -ldl -lrocketmq -lboost_system -lboost_thread -lpthread
SRC = ../common/daemonClient/*.cpp ../common/*.cpp \
	  *.cpp gen-cpp/*.cc \
	  /home/l/hiredis/libhiredis.a

all: producer

producer:
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm producer gen-cpp -rf



MYSQL_CONCPP_DIR= /usr
CPPFLAGS += -I $(MYSQL_CONCPP_DIR)/include -I $(MYSQL_CONCPP_DIR)/include/mysql \
CXX = g++ 
CXXFLAGS += -std=c++11 -Wall
LDFLAGS += -L/usr/local/lib -L/usr/local/lib\
		  -Wl,--no-as-needed -Wl,--as-needed\
		  -ldl -lboost_system -lboost_thread -lpthread \
		  -lmysqlclient -lm -lrt -lssl -lcrypto
#SRC = main.cpp /usr/lib64/libmysqlcppconn8.so.1


