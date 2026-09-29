# Ternet V1 C++ architecture

Ternet V1 uses a CPython-inspired separation of responsibilities without copying CPython implementation code.

```text
.trn source
  -> lexer
  -> parser / AST
  -> type checker
  -> bytecode compiler
  -> Ternet VM / runtime
  -> standard library and modules
```

## Public layout

- `include/ternet/` — stable C++ headers
- `src/` — C++ translation-unit entry points
- `runtime/` — runtime object implementations (migration destination)
- `stdlib/` — Ternet standard-library modules
- `tests/` — language/compiler/VM tests
- `examples/` — runnable `.trn` examples
- `docs/` — architecture and language documentation

## Migration rule

The initial V1 migration uses thin C++17 wrappers around the existing implementation. This avoids a risky rewrite and keeps the current parser, interpreter, bytecode compiler, type checker, and VM behavior available while each subsystem is progressively moved into its new file.

CPython is used only as an architectural reference. Ternet's implementation remains original C++ code.
