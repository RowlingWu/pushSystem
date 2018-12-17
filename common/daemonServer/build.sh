mkdir -p gen-cpp
rm daemon_server gen-cpp/* -rf

protoc -I grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` grpc/daemonServer.proto
protoc -I grpc --cpp_out=gen-cpp grpc/daemonServer.proto

make
