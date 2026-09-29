<!-- V1 C++ architecture is developed on branch `v1-cpp-architecture`; see docs/v1-cpp-architecture.md. -->

<div align="center">
  <h1>Ternet</h1>
  <p><strong>A modern, readable C++17 programming language built around .trn source, tnc, a type checker, bytecode compiler, and TVM.</strong></p>
</div>

## V1 architecture

The V1 migration separates the implementation into `include/ternet`, `src`, `runtime`, `stdlib`, `tests`, `examples`, and `docs` while preserving the existing tested compiler/runtime behavior during the incremental split.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
./build/tnc-v1 run examples/hello.trn
```

See `docs/v1-cpp-architecture.md` for the migration design.
