mkdir -p gen-cpp
rm start_push gen-cpp/* -rf

protoc -I ../daemonServer/grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../daemonServer/grpc/daemonServer.proto
protoc -I ../daemonServer/grpc --cpp_out=gen-cpp ../daemonServer/grpc/daemonServer.proto

make
