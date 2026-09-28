# Contributing to Ternet

## Ground rules

- Keep the compiler honest: do not document unimplemented syntax as working.
- Prefer small compiler stages with focused tests.
- Every syntax feature needs lexer/parser coverage.
- Every semantic feature needs positive and negative tests.
- Runtime changes need VM regression tests.
- Keep C++17 compatibility.

## Development flow

```bash
cmake -S . -B build
cmake --build build
./build/tnc examples/hello.trn
```

## Feature workflow

1. Update the language specification.
2. Add or update AST/token definitions.
3. Implement lexer/parser behavior.
4. Add semantic checks.
5. Add compiler/bytecode support.
6. Add VM support.
7. Add positive and negative tests.
8. Update implementation status.
9. Update user-facing documentation.

## Compatibility

The current baseline uses `::`. V1 is designed around `:`. Any migration must be deliberate and tested.
