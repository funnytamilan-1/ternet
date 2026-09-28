# Ternet Platform

Ternet is being developed as a programming language and toolchain.

## Runtime architecture

.trn -> Lexer -> Parser/AST -> Semantic checker -> IR -> Bytecode -> Verifier -> TVM -> Runtime/stdlib

## Product targets

- tnc compiler and project tool
- TVM bytecode runtime
- standard library
- package manager and registry
- VS Code language server and debug adapter
- documentation website
- sandboxed browser playground
- desktop/mobile tooling

## Completion rule

A feature is complete only when syntax, AST, semantic rules, runtime behavior, compiler/VM behavior, tests, documentation, and tooling are implemented. A roadmap or stub is not counted as implementation.