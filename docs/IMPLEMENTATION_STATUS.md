# Ternet Implementation Status

## Implemented in 0.2 development build
- `.trn` lexer with locations, comments and escaped strings
- integers, floats, booleans and null
- immutable `let`, mutable `mut`, immutable `const`
- arithmetic, comparison and logical operators
- arrays and indexing
- `tnprint(...)` with multiple arguments
- `if / elif / else`, `while`, `break`, `continue`
- named functions, arguments and `return`
- reference VM/interpreter
- `tnc run`, `check`, `version`, `init`, `add`, `install`, `list`, `remove`, `package`
- local dependency metadata and `ternet.lock`

## Not yet implemented
- static type checker/inference
- structs/classes/traits/generics
- ownership/borrowing
- Option/Result
- module/import resolution
- async/await scheduler
- bytecode compiler/verifier and native backend
- production filesystem/network/crypto standard libraries
- remote registry, signatures and integrity verification
- formatter/linter/LSP/debugger/profiler
- OCI/Docker integration
- capability enforcement and sandboxed OS access

A feature is implemented only when code and regression tests exist.