mkdir -p gen-cpp
rm consumer gen-cpp/* -rf

protoc -I grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` grpc/consumer.proto
protoc -I grpc --cpp_out=gen-cpp grpc/consumer.proto

protoc -I ../daemonServer/grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../daemonServer/grpc/daemonServer.proto
protoc -I ../daemonServer/grpc --cpp_out=gen-cpp ../daemonServer/grpc/daemonServer.proto

protoc -I ../producer/grpc --grpc_out=gen-cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../producer/grpc/producer.proto
protoc -I ../producer/grpc --cpp_out=gen-cpp ../producer/grpc/producer.proto

make
