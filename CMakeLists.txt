cmake_minimum_required(VERSION 3.16)
project(tinylang-jit CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(tinylang
  src/main.cpp
  src/lexer.cpp
  src/parser.cpp
  src/bytecode.cpp
  src/vm.cpp
  src/value.cpp
  src/jit/profile.cpp
  src/jit/code_cache.cpp
  src/jit/template_jit.cpp
  src/jit/native_codegen.cpp
  src/bench_runner.cpp
)

target_include_directories(tinylang PRIVATE include)

if(APPLE)
  target_link_libraries(tinylang PRIVATE pthread)
endif()

enable_testing()
add_test(NAME fib_interp COMMAND tinylang ${CMAKE_SOURCE_DIR}/bench/fib.tl --interp)
add_test(NAME fib_jit COMMAND tinylang ${CMAKE_SOURCE_DIR}/bench/fib.tl --jit)
