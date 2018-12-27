mkdir -p gen-cpp
rm producer gen-cpp/* -rf

protoc -I grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` grpc/producer.proto
protoc -I grpc --cpp_out=gen-cpp grpc/producer.proto

protoc -I ../daemonServer/grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../daemonServer/grpc/daemonServer.proto
protoc -I ../daemonServer/grpc --cpp_out=gen-cpp ../daemonServer/grpc/daemonServer.proto

make
