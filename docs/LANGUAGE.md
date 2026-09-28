# Ternet Language Specification

## Source files

Ternet source files use the `.trn` extension.

## Statement terminator

Every executable statement ends with `::`.

## Output

```trn
tnprint("Hello")::
tnprint("Line 2")::
```

## Strings

Strings use double quotes. The lexer currently supports:
- `\\n`
- `\\t`
- escaped backslash
- escaped double quote

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

The syntax in this document is the contract for implemented features only. Proposed v1 features must not be treated as implemented until tests cover them.
