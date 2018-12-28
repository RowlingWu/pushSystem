mkdir -p gen-cpp
rm user_info_update gen-cpp/* -rf

protoc -I grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` grpc/userInfoUpdate.proto
protoc -I grpc --cpp_out=gen-cpp grpc/userInfoUpdate.proto

protoc -I ../daemonServer/grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../daemonServer/grpc/daemonServer.proto
protoc -I ../daemonServer/grpc --cpp_out=gen-cpp ../daemonServer/grpc/daemonServer.proto

make
