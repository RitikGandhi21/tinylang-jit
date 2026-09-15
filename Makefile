CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Iinclude
SRCS = src/main.cpp src/lexer.cpp src/parser.cpp src/bytecode.cpp src/value.cpp src/vm.cpp \
	src/bench_runner.cpp src/jit/profile.cpp src/jit/code_cache.cpp src/jit/template_jit.cpp \
	src/jit/native_codegen.cpp

tinylang: $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o tinylang

.PHONY: test
test: tinylang
	./tinylang bench/fib.tl --interp --bench
