
#include <kj/compat/http.h>

class EchoHttpService : public kj::HttpService, public kj::TaskSet::ErrorHandler {
public:
    EchoHttpService() : tasks(*this) { }

    kj::Promise<void> request(kj::HttpMethod method,
                              kj::StringPtr url,
                              const kj::HttpHeaders &requestHeaders,
                              kj::AsyncInputStream &requestBody,
                              Response &response) override
    {
      KJ_LOG(WARNING, "Process request", method, url);

      if (method != kj::HttpMethod::POST) {
        co_return co_await response.sendError(400, "Bad Request", kj::HttpHeaders(headerTable));
      }

      auto output = response.send(200, "OK", kj::HttpHeaders(headerTable));
      auto task = requestBody.pumpTo(*output)
          .then([method, url, _= kj::mv(output)](uint64_t) {
            KJ_LOG(WARNING, "Process request done", method, url);
          });
#if 0
      tasks.add(kj::mv(task));
      co_return co_await kj::Promise<void>(kj::READY_NOW);
#endif
      co_return co_await task;
    }

    kj::Promise<void> connect(kj::StringPtr host,
                              const kj::HttpHeaders &headers,
                              kj::AsyncIoStream &connection,
                              ConnectResponse &response,
                              kj::HttpConnectSettings settings) override
    {
      KJ_UNIMPLEMENTED("Handling for CONNECT requests is not supported yet!");
    }

    void taskFailed(kj::Exception&& exception) override {
      KJ_LOG(ERROR, "log exception", exception);
    }

private:
    kj::HttpHeaderTable headerTable;
    kj::TaskSet tasks;
};

int main(int argc, char **argv) {
  static constexpr kj::StringPtr serverAddress = "127.0.0.1:9877"_kj;

  kj::AsyncIoContext io = kj::setupAsyncIo();
  io.provider->getNetwork()
      .parseAddress("127.0.0.1:9877")
       .then([&io](kj::Own<kj::NetworkAddress> addr) -> kj::Promise<void> {
          auto receiver = addr->listen();
          EchoHttpService echoService;
          kj::HttpHeaderTable headerTable;
          kj::HttpServer server(io.provider->getTimer(), headerTable, echoService);
          KJ_LOG(WARNING, "starting to listen on ", serverAddress);
          co_return co_await server.listenHttp(*receiver);
       }).wait(io.waitScope);
}