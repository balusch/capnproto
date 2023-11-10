/*
 * Copyright(C) 2023
 */

#pragma once

#include <kj/async.h>
#include <kj/async-io.h>
#include <kj/exception.h>
#include <kj/debug.h>
#include <kj/common.h>
#include <kj/array.h>
#include <kj/vector.h>
#include <kj/string.h>
#include <kj/main.h>
#include <kj/list.h>

namespace laputa {

class CliError {
public:
    CliError(kj::String description) : description(kj::mv(description)) {}

    kj::String description;
};

template<typename Func>
auto cliMethod(Func &&func) {
  return [func = kj::fwd<Func>(func)](auto &&... params) mutable -> kj::MainBuilder::Validity {
      try {
        func(kj::fwd<decltype(params)>(params)...);
        return true;
      } catch (CliError &e) {
        return kj::mv(e.description);
      }
  };
}

#define CLI_METHOD(name) cliMethod(KJ_BIND_METHOD(*this, name))

class CliMain {
public:
    CliMain(kj::TopLevelProcessContext &context, char **argv)
        : context(context), argv(argv) {}

    kj::MainFunc getMain() {
      return kj::MainBuilder(context, kj::StringPtr("v0.0.1"), "Simple kj application.")
          .addOptionWithArg({'t', "task"}, CLI_METHOD(setTask), "<task>", "task to run")
          .callAfterParsing(CLI_METHOD(run)).build();
    }

    void setTask(kj::StringPtr taskStr) {
        task = taskStr;
    }

    void run() {
      KJ_LOG(WARNING, "ready to run task...");

      if (task == "promise") {
        testPromise();
      } else if (task == "list") {
        testList();
      } else if (task == "string") {
        testString();
      } else if (task == "array") {
        testArray();
      } else {
        KJ_LOG(ERROR, "Unknown task ", task);
      }
    }

private:
    void testPromise();
    void testList();
    void testString();
    void testArray();


    kj::ProcessContext &context;
    char **argv;
    kj::StringPtr task;

    kj::AsyncIoContext io = kj::setupAsyncIo();
};

}
