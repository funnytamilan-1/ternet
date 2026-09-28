# Ternet Toolchain

## Current CLI

```bash
tnc <file.trn>
```

## Planned CLI

```bash
tnc run app.trn
tnc build app.trn
tnc check app.trn
tnc test
tnc fmt
tnc lint
tnc repl
tnc docs
tnc package
tnc install
tnc publish
tnc version
tnc help
```

## Formatter

`tnc fmt` will produce deterministic source formatting.

## Linter

`tnc lint` will detect unused bindings, suspicious constructs and unreachable code without changing semantics.

## Test runner

```trn
test "addition" {
    assert(add(2, 3) == 5):
}:
```

## REPL

```text
$ tnc repl
Ternet REPL v1.0
>>> 2 + 3
5
```

## Diagnostics

Diagnostics should contain severity, stable error code, message, source location, source span and actionable help.

Example:

```text
error[T1002]: expected ':' after binding
 --> src/main.trn:4:15
  |
4 | let name = "Ajmal"
  |                   ^ expected ':'
```
