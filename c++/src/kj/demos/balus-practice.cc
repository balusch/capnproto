/*
 * Copyright(C) 2023
 */

#include "balus-practice.h"
#include <kj/array.h>
#include <kj/compat/http.h>

namespace laputa {

void CliMain::testString() {

}

void CliMain::testArray() {
  kj::Array<int> arr;
}

void CliMain::testList() {
  struct Widget {
    Widget(kj::StringPtr name) : name(kj::heapString(name)) {}

    kj::String name;
    kj::ListLink<Widget> link;
  };

  kj::List<Widget, &Widget::link> widgetList;

  Widget w1("tom");
  widgetList.add(w1);
  KJ_DEFER(widgetList.remove(w1));

  Widget w2("jerry");
  widgetList.addFront(w2);
  KJ_DEFER(widgetList.remove(w2));

  KJ_LOG(WARNING, "front: ", widgetList.front().name);
}

class EchoHttpService : public kj::HttpService {
public:
    EchoHttpService() = default;

    kj::Promise<void> request(kj::HttpMethod method,
                              kj::StringPtr url,
                              const kj::HttpHeaders &requestHeaders,
                              kj::AsyncInputStream &requestBody,
                              Response &response) override
    {
      KJ_LOG(WARNING, "Process request", method, url);

      if (method != kj::HttpMethod::GET) {
        kj::HttpHeaderTable::Builder builder;
        auto headerTable = builder.build();
        kj::HttpHeaders responseHeaders(kj::mv(*headerTable));
        co_return co_await response.sendError(400, "Bad Request", responseHeaders);
      }

      kj::HttpHeaderTable::Builder builder;
      auto headerTable = builder.build();
      kj::HttpHeaders responseHeaders(kj::mv(*headerTable));
      auto reqStr = co_await requestBody.readAllText();
      auto output = response.send(200, "OK", responseHeaders);
      co_return co_await output->write(reqStr.cStr(), reqStr.size()).attach(kj::mv(reqStr));
    }

    kj::Promise<void> connect(kj::StringPtr host,
                              const kj::HttpHeaders &headers,
                              kj::AsyncIoStream &connection,
                              ConnectResponse &response,
                              kj::HttpConnectSettings settings) override
    {
      KJ_LOG(WARNING, "Process CONNECT request", host);

      kj::HttpHeaderTable::Builder builder;
      auto headerTable = builder.build();
      kj::HttpHeaders responseHeaders(kj::mv(*headerTable));
      co_return co_await response.reject(400, "Bad Request", responseHeaders)
          ->write("Nice to meet you", sizeof("Nice to meet you") - 1);
    }
};

void CliMain::testPromise() {
  io.provider->getNetwork()
      .parseAddress("127.0.0.1:9877")
      .then([this](kj::Own<kj::NetworkAddress> addr) {
      auto receiver = addr->listen();
      EchoHttpService echoService;
      kj::HttpHeaderTable headerTable;
      kj::HttpServer server(io.provider->getTimer(), headerTable, echoService);
      return server.listenHttp(*receiver).wait(io.waitScope);
  }).wait(io.waitScope);
}

} // namespace laputa
