# Ternet Compiler

Production compiler pipeline target:

`.trn -> Lexer -> Parser -> Semantic Analyzer -> Type Checker -> HIR -> MIR -> Optimizer -> Bytecode -> TVM`

The reference interpreter remains the compatibility baseline until each compiler stage is executable and covered by tests.

## Rules

- Do not document a compiler stage as implemented until it is executable and tested.
- Compiler diagnostics use stable error codes and source spans.
- Bytecode verification is a mandatory trust boundary before TVM execution.
- CMake remains the authoritative build system.

## Current milestone

The repository is migrating incrementally from the reference interpreter toward a real compiler. Parser/AST changes required for declared types and typed function signatures must land before static type checking is enabled.
