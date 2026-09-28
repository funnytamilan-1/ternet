# Implemented Ternet Language Baseline

This document describes **only what the current compiler actually accepts**.

## Source files

Ternet source files use the `.trn` extension.

## Statement terminator

The current baseline uses `::`.

## Output

```trn
tnprint("Hello"):: 
tnprint("Line 2")::
```

## Strings

Double-quoted strings are supported. The lexer handles newline/tab escapes and escaped characters.

## Pipeline

```
.trn
  -> Lexer
  -> Parser
  -> AST
  -> Compiler
  -> Bytecode
  -> TVM
  -> process output
```

## Not implemented here

Variables, expressions, conditions, loops, functions, classes, ownership, modules, async, package management and the V1 single-colon syntax are not accepted by this baseline.

See [V1_SPEC.md](V1_SPEC.md) for the target language and [STATUS.md](STATUS.md) for the exact implementation boundary.
