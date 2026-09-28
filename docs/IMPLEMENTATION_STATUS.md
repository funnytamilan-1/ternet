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
- local source-module import resolution for `import` declarations
- enum declarations and basic `match` statements
- struct field/member access on the reference VM and bytecode VM
- webfile lowering and execution in the bytecode VM

## Not yet implemented
- production-grade separate compilation/linking and exported module namespaces
- complete static type inference and user-defined type checking
- full classes/traits/interfaces/generics
- ownership/borrowing
- Option/Result
- async/await scheduler
- bytecode compiler/verifier and native backend
- production filesystem/network/crypto standard libraries
- remote registry, signatures and integrity verification
- formatter/linter/LSP/debugger/profiler
- OCI/Docker integration
- capability enforcement and sandboxed OS access

A feature is implemented only when code and regression tests exist.