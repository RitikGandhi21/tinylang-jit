# tinylang-jit

A small dynamic-ish language with a bytecode interpreter and a tiered JIT.

## Pipeline

```
source (.tl) → parser → bytecode → interpreter (tier 0)
                              ↘ profiling counters
                                ↘ template JIT → mmap'd executable code (tier 1)
```

## Design choices (v1)

| Question | Decision |
|----------|----------|
| IR + SSA? | **No** — bytecode → template machine code. Add thin LIR later if OSR/deopt need it. |
| Register allocation? | **Spill to stack** in template JIT until safepoints/deopt exist. |
| Target ISA | **Native host**: AArch64 on Apple Silicon, x86-64 on Intel/Linux (`native_codegen.cpp`). |

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tinylang bench/fib.tl --interp
./build/tinylang bench/fib.tl --jit
./build/tinylang bench/fib.tl --jit --bench
```

## Flags

- `--interp` — interpreter only (no JIT)
- `--jit` — enable profiling + JIT (default)
- `--jit-threshold N` — compile after N invocations (default 32)
- `--bench` — print timing for the entry call

## Benchmarks vs gcc

C equivalents live in `bench/c/` for manual comparison:

```bash
clang -O2 bench/c/fib.c -o /tmp/fib && /tmp/fib
```

Numeric kernels in a dynamic language are not expected to match `gcc -O2`; compare **interp vs JIT** first.

## Roadmap

- [x] Bytecode VM
- [x] Invocation profiling + template JIT + W^X code cache
- [x] Patch direct calls to JIT entry (when callee is already compiled)
- [ ] Inline caches for dynamic dispatch
- [ ] Linear-scan regalloc on thin LIR
- [ ] Safepoints + deopt
- [ ] OSR (stretch)
