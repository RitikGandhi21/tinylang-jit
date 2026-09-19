# Design notes

## Open questions (answers for this repo)

### SSA IR vs bytecode → machine code?

**Bytecode → template machine code** for v1. Add a thin LIR when safepoints/deopt/OSR require uniform frame maps.

### Linear-scan vs spill-all?

**Spill-all / stack slots** until deopt works. Linear-scan comes after safepoints.

### x86-64 vs AArch64?

**Host ISA**: AArch64 on Apple Silicon (current dev machine), x86-64 on Intel/Linux. Same pipeline; `native_codegen.cpp` is `#if defined(__aarch64__)`.

## JIT status

- Profiling + `mmap` W^X code cache: implemented
- Template JIT emitter: in progress (stack lowering for `CALL`/`ADD`/`JMP` must be finished before `fib` compiles)
- Call-site patching: indirect via `tinylang_jit_call1` → `call_function` (direct patch is next)
- Inline caches / OSR: not started

## Run

```bash
make
./tinylang bench/fib.tl --interp --bench
clang -O2 bench/c/fib.c -o /tmp/fib-c && /tmp/fib-c
```
