CXX = g++ -I/home/l/grpc/include -I. \
	  -I/home/l/hiredis
CXXFLAGS += -std=c++11 -ggdb
LDFLAGS += -L/usr/local/lib \
		   -L/usr/lib64/mysql \
		  -Wl,--no-as-needed -Wl,--as-needed\
		  -ldl -lpthread \
		  -lmysqlclient -lm -lrt -lssl -lcrypto
SRC =  ../common/common.cpp ../common/dataHandler/*.cpp \
	  *.cpp \
	  /home/l/hiredis/libhiredis.a

all: user_info_update

user_info_update:
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm user_info_update -rf

