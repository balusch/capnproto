/*
 * Copyright(C) 2023
 */

#include <kj/compat/http.h>
#include <kj/string.h>

int main(int argc, char **argv) {
  kj::AsyncIoContext io = kj::setupAsyncIo();
  kj::HttpHeaderTable responseHeaderTable;

  static auto run_client = [](kj::HttpClient &client, kj::StringPtr url) -> kj::Promise<void> {
    kj::HttpHeaderTable requestHeaderTable;
    kj::HttpHeaders headers(requestHeaderTable);
    kj::StringPtr requestBody = "Shall we?"_kj;
    auto request = client.request(kj::HttpMethod::POST, url, headers);
    co_return co_await request.body->write(requestBody.cStr(), requestBody.size())
        .then([&request]() -> kj::Promise<kj::HttpClient::Response> {
          request.body = nullptr;
          co_return co_await kj::mv(request.response);
        }).then([](kj::HttpClient::Response response) -> kj::Promise<void> {
          auto text = co_await response.body->readAllText();
          KJ_LOG(WARNING, "response body:", text);
          co_return;
        });
  };

  {
    static auto test_coroutine = []() -> kj::Promise<void> {
      co_return co_await kj::Promise<void>(kj::READY_NOW);
    };
    test_coroutine().wait(io.waitScope);
  }

  {
    auto client = kj::newHttpClient(io.provider->getTimer(),
        responseHeaderTable, io.provider->getNetwork(), kj::none);
    run_client(*client, "http://127.0.0.1:9877/hello"_kj).wait(io.waitScope);
  }

  {
    auto address = io.provider->getNetwork().parseAddress("127.0.0.1", 9877).wait(io.waitScope);
    auto client = kj::newHttpClient(io.provider->getTimer(), responseHeaderTable, *address);
    run_client(*client, "https://google.com/world"_kj).wait(io.waitScope);
  }

  {
  }
}