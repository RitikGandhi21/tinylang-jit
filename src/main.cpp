#include "lexer.h"
#include "parser.h"
#include "vm.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string read_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open: " + path);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: tinylang <file.tl> [--interp] [--jit] [--jit-threshold N] [--bench]\n";
    return 1;
  }

  std::string path = argv[1];
  bool interp_only = false;
  bool bench = false;
  uint32_t threshold = 32;

  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--interp") interp_only = true;
    else if (arg == "--jit") interp_only = false;
    else if (arg == "--bench") bench = true;
    else if (arg == "--jit-threshold" && i + 1 < argc) threshold = static_cast<uint32_t>(std::stoi(argv[++i]));
    else {
      std::cerr << "unknown arg: " << arg << "\n";
      return 1;
    }
  }

  std::string source = read_file(path);
  tinylang::Lexer lexer(source);
  auto tokens = lexer.tokenize();
  tinylang::Parser parser(tokens);
  tinylang::Program program = parser.parse();

  std::vector<tinylang::FunctionObj> functions;
  functions.reserve(program.functions.size());
  for (auto& chunk : program.functions) {
    tinylang::FunctionObj fn;
    fn.chunk = std::move(chunk);
    functions.push_back(std::move(fn));
  }

  tinylang::VM vm(std::move(functions), program.entry_index);
  vm.set_jit_enabled(!interp_only);
  vm.set_jit_threshold(threshold);

  auto start = std::chrono::steady_clock::now();
  tinylang::Value result = vm.run();
  auto end = std::chrono::steady_clock::now();

  if (bench) {
    auto ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "result=" << result << " time_ms=" << ms << "\n";
  } else {
    std::cout << result << "\n";
  }
  return 0;
}
