# Ternet V1 C++ Architecture

The V1 migration introduces a lowercase C++17 layout around the existing Ternet compiler/runtime.

- `include/ternet/` — public C++ headers
- `src/` — compiler/runtime translation units
- `runtime/` — runtime layer
- `stdlib/` — standard library layer
- `examples/` — runnable `.trn` examples
- `docs/` — architecture notes

Build with CMake and use `tnc-v1` for the migration target.
