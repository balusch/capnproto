/*
 * Copyright(C) 2023
 */

#include "balus-practice.h"

int main(int argc, char **argv) {
  kj::TopLevelProcessContext context(argv[0]);
  laputa::CliMain mainObject(context, argv);
  return kj::runMainAndExit(context, mainObject.getMain(), argc, argv);
}
