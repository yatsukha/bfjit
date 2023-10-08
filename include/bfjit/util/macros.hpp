#pragma once

#define BFJIT_WARNINGS_PUSH \
  _Pragma("clang diagnostic push") \
  _Pragma("clang diagnostic ignored \"-Wunused-parameter\"") \
  _Pragma("clang diagnostic ignored \"-Wshadow\"")

#define BFJIT_WARNINGS_POP \
  _Pragma("clang diagnostic pop")
  

