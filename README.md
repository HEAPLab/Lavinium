# Lavinium

Lavinium is an LLVM 17.0.6 fork for WCET-driven compiler autotuning. It embeds LLVMTA in LLVM's backend and evaluates LLVM IR pass sequences against a configurable static timing model. The current flow explores each function independently, keeps the sequence with the lowest estimated WCET, and emits the resulting IR and measurements.

## Reference paper

G. Magnani, D. Baroffio, F. Reghenzani, G. Agosta, and W. Fornaciari, “Modern LLVM-based Compiler Autotuning for WCET Optimization,” *46th IEEE Real-Time Systems Symposium (RTSS)*, 2025. DOI: `10.1109/RTSS66672.2025.00043`.

- [Paper PDF](https://re.public.polimi.it/retrieve/6e6fa958-3b9b-49dd-83a2-382958e002ca/2025___RTSS___Lavinium.pdf)
- [Politecnico di Milano record](https://re.public.polimi.it/handle/11311/1298226)

The paper reports up to 69% lower WCET than `-O3` and a 2.41× speedup over its black-box exploration baseline. Those are results from the paper's experimental setup, not guarantees for every target or checkout.

## How it works

1. Clang produces LLVM IR for the target program.
2. LLVMTA runs in the LLVM backend with the selected abstract microarchitecture and memory model.
3. Lavinium evaluates candidate pass sequences per function, caches the WCET reported by LLVMTA, and retains the best sequence.
4. The selected sequence is applied and the optimized IR and WCET result are written to the benchmark output directory.

WCET values are static estimates in clock cycles for LLVMTA's configured abstract model; they are not measurements or hardware WCET guarantees.

## Repository layout

| Path | Purpose |
| --- | --- |
| `llvm/lib/CodeGen/Lavinium*` and `llvm/include/llvm/IR/Lavinium*` | Lavinium tracker, rescheduler, and LLVM integration |
| `llvm/lib/CodeGen/LLVMTA` | Embedded LLVMTA timing analysis |
| `llvm/lib/Transforms/Utils/LoopAnnotation.*` | The `loop-annota` pass |
| `llvm/lib/CodeGen/LaviniumStrategies` | Search strategies |
| `test/passes` | Candidate LLVM pass pipeline fragments |
| `test/C`, `test/C_new`, `test/C_riscv_renesas`, `test/PolyBench` | Benchmark suites |
| `test/run_mt.py` | Current multithreaded benchmark runner |

## Build

The LLVM build needs CMake, Ninja, a C++ compiler, and lp_solve development files (the `lpsolve55` library and headers). The benchmark runner additionally needs Python 3, the `pandas` Python package, and a RISC-V 32-bit sysroot.

```bash
cmake -S llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_TARGETS_TO_BUILD="ARM;RISCV"
cmake --build build --target clang opt
```

The patched tools are `build/bin/clang` and `build/bin/opt`.

## Quick start

From the repository root, set the toolchain and target sysroot, then run a small benchmark:

```bash
export LAVINIUM_PATH="$PWD/build/bin"
export SYSROOT=/path/to/riscv32-sysroot

cd test
python3 run_mt.py -cdir C -compile -only add \
  -custom_args="-mllvm -lavinium-strategy=greedy -mllvm -lavinium-depth=2"
```


## Candidate passes and strategies

`test/passes` contains one candidate pass pipeline fragment per line. Lines beginning with `#` are comments, and entries use LLVM pass-pipeline syntax.

Select a strategy with `-mllvm -lavinium-strategy=<name>`. The available names and main controls are:

| Strategy | Description | Main controls |
| --- | --- | --- |
| `none` | Baseline; do not explore alternatives | — |
| `greedy` | Add the best pass at each depth | `-lavinium-depth=N` |
| `cartesian` | Exhaustively explore sequences up to a depth | `-lavinium-depth=N` |
| `cartesian-pruned` | Prune dependent/idempotent sequences before search | `-lavinium-depth=N` |
| `random` | Sample random pass sequences | `-rand-sample=N`, `-sequence-length=N` |
| `genetic` | Evolve a population using crossover and mutation | `-genetic-pool=N`, `-genetic-sample=N`, `-sequence-length=N` |
| `association` | Explore sequences using weighted associations | `-assocrule-pool=N`, `-assoc-sample=N`, `-sequence-length=N`, `-assoc-clean=true` |
| `apply-csv` | Replay the best sequences recorded in a CSV file | `-csv-file=<file>`, `-benchmark-name=<name>` |

Pass these options to Clang with `-mllvm`, for example:

```bash
-custom_args="-mllvm -lavinium-strategy=random -mllvm -rand-sample=100 -mllvm -sequence-length=3"
```

Search cost can grow quickly for exhaustive strategies. The current genetic implementation requires `-genetic-pool` to be divisible by four. Specify the strategy and search controls explicitly when comparing runs.

## License

See [`LICENSE.TXT`](LICENSE.TXT) and the license headers in the embedded LLVMTA sources.
