// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: daemonServer.proto
#ifndef GRPC_daemonServer_2eproto__INCLUDED
#define GRPC_daemonServer_2eproto__INCLUDED

#include "daemonServer.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace daemon_server {

class DaemonServer final {
 public:
  static constexpr char const* service_full_name() {
    return "daemon_server.DaemonServer";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status ClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::daemon_server::ClientRegisterReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::daemon_server::ClientRegisterReply>> AsyncClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::daemon_server::ClientRegisterReply>>(AsyncClientRegisterRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::daemon_server::ClientRegisterReply>> PrepareAsyncClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::daemon_server::ClientRegisterReply>>(PrepareAsyncClientRegisterRaw(context, request, cq));
    }
    class experimental_async_interface {
     public:
      virtual ~experimental_async_interface() {}
      virtual void ClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response, std::function<void(::grpc::Status)>) = 0;
    };
    virtual class experimental_async_interface* experimental_async() { return nullptr; }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::daemon_server::ClientRegisterReply>* AsyncClientRegisterRaw(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::daemon_server::ClientRegisterReply>* PrepareAsyncClientRegisterRaw(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status ClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::daemon_server::ClientRegisterReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::daemon_server::ClientRegisterReply>> AsyncClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::daemon_server::ClientRegisterReply>>(AsyncClientRegisterRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::daemon_server::ClientRegisterReply>> PrepareAsyncClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::daemon_server::ClientRegisterReply>>(PrepareAsyncClientRegisterRaw(context, request, cq));
    }
    class experimental_async final :
      public StubInterface::experimental_async_interface {
     public:
      void ClientRegister(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response, std::function<void(::grpc::Status)>) override;
     private:
      friend class Stub;
      explicit experimental_async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class experimental_async_interface* experimental_async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class experimental_async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::daemon_server::ClientRegisterReply>* AsyncClientRegisterRaw(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::daemon_server::ClientRegisterReply>* PrepareAsyncClientRegisterRaw(::grpc::ClientContext* context, const ::daemon_server::ClientRegisterRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_ClientRegister_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_ClientRegister : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_ClientRegister() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_ClientRegister() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestClientRegister(::grpc::ServerContext* context, ::daemon_server::ClientRegisterRequest* request, ::grpc::ServerAsyncResponseWriter< ::daemon_server::ClientRegisterReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_ClientRegister<Service > AsyncService;
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_ClientRegister : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    ExperimentalWithCallbackMethod_ClientRegister() {
      ::grpc::Service::experimental().MarkMethodCallback(0,
        new ::grpc::internal::CallbackUnaryHandler< ExperimentalWithCallbackMethod_ClientRegister<BaseClass>, ::daemon_server::ClientRegisterRequest, ::daemon_server::ClientRegisterReply>(
          [this](::grpc::ServerContext* context,
                 const ::daemon_server::ClientRegisterRequest* request,
                 ::daemon_server::ClientRegisterReply* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   this->ClientRegister(context, request, response, controller);
                 }, this));
    }
    ~ExperimentalWithCallbackMethod_ClientRegister() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  typedef ExperimentalWithCallbackMethod_ClientRegister<Service > ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_ClientRegister : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_ClientRegister() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_ClientRegister() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_ClientRegister : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithRawMethod_ClientRegister() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_ClientRegister() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestClientRegister(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_ClientRegister : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    ExperimentalWithRawCallbackMethod_ClientRegister() {
      ::grpc::Service::experimental().MarkMethodRawCallback(0,
        new ::grpc::internal::CallbackUnaryHandler< ExperimentalWithRawCallbackMethod_ClientRegister<BaseClass>, ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
          [this](::grpc::ServerContext* context,
                 const ::grpc::ByteBuffer* request,
                 ::grpc::ByteBuffer* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   this->ClientRegister(context, request, response, controller);
                 }, this));
    }
    ~ExperimentalWithRawCallbackMethod_ClientRegister() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void ClientRegister(::grpc::ServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_ClientRegister : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_ClientRegister() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler< ::daemon_server::ClientRegisterRequest, ::daemon_server::ClientRegisterReply>(std::bind(&WithStreamedUnaryMethod_ClientRegister<BaseClass>::StreamedClientRegister, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_ClientRegister() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status ClientRegister(::grpc::ServerContext* context, const ::daemon_server::ClientRegisterRequest* request, ::daemon_server::ClientRegisterReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedClientRegister(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::daemon_server::ClientRegisterRequest,::daemon_server::ClientRegisterReply>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_ClientRegister<Service > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_ClientRegister<Service > StreamedService;
};

}  // namespace daemon_server


#endif  // GRPC_daemonServer_2eproto__INCLUDED
