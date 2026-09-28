# Ternet Architecture

## Current implementation

The current compiler is intentionally small:

1. **Lexer** converts source text into tokens.
2. **Parser** validates the current print grammar and creates an AST.
3. **Compiler** converts AST nodes into bytecode instructions.
4. **TVM** executes bytecode.

Current flow:

```
.trn
  |
  v
Lexer
  |
  v
Parser
  |
  v
AST
  |
  v
Compiler
  |
  v
Bytecode
  |
  v
TVM
```

## Target V1 architecture

```
Source
  |
Lexer
  |
Parser
  |
AST
  |
Name Resolver
  |
Type Checker
  |
Borrow Checker
  |
Semantic Analyzer
  |
Ternet IR
  |
Optimizer
  |
Bytecode Compiler ----> TVM
  |
  +--------------------> future native backend
```

## Repository layers

```
include/   public compiler/runtime interfaces
src/       compiler and VM implementation
std/       standard-library design boundary
examples/  runnable examples for implemented features
tests/     regression and negative tests
docs/      language and architecture specifications
```

## Design rules

- The VM never parses source.
- Front-end semantic checks remain separate from runtime execution.
- Bytecode instructions must not depend on parser token objects.
- New syntax requires lexer/parser tests.
- New runtime behavior requires VM/compiler tests.
- Documentation must clearly distinguish implemented from specified behavior.

## CLI

The current CLI is:

```bash
tnc <file.trn>
```

Future subcommands are documented in [TOOLING.md](TOOLING.md).
