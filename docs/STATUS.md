# Ternet Implementation Status

This file prevents the project from confusing specification with implementation.

## Implemented today

- C++17 + CMake build
- `.trn` lexer with line/column information
- line comments and block comments (`/* ... */`)
- escaped strings
- integers, floats, booleans and null literals
- `let`, `mut`, `const` bindings
- arithmetic, comparison and logical expressions
- arrays and indexing
- `tnprint(...)` with multiple arguments
- `if / elif / else`
- `while`, `break`, `continue`
- named functions, arguments and `return`
- `throw` / `try` / `catch` / `finally`
- `webfile` output to the protected `dist/` directory
- `tnc run`, `check`, `version`, `init`, `add`, `install`, `list`, `remove`, `package`
- reference interpreter/TVM-style runtime
- CTest regression coverage for core, loops, errors, web output and block comments

## Specified but not implemented

- static type checker and type inference
- structs/enums/classes/traits/generics
- ownership/borrowing
- Option/Result
- async/await scheduler
- module/import resolution
- production standard library modules
- real bytecode compiler and bytecode verifier
- native backend
- remote package registry, signatures and integrity verification
- formatter/linter/LSP/debugger/profiler
- OCI/container runtime enforcement
- capability enforcement and sandboxed OS access

## Compatibility note

The current baseline intentionally uses `::` after `tnprint`. V1 moves to `:`. This migration must be implemented with compatibility tests; documentation must not imply both are accepted.

## Completion rule

A feature becomes Implemented only when source support, positive tests, negative tests, matching documentation and a passing build all exist.
