# Ternet V1 Language Specification

**Status:** Design specification  
**Version:** 1.0 (target)  
**Source extension:** `.trn`  
**Compiler:** `tnc`  
**Runtime:** TVM (Ternet Virtual Machine)

This document defines the intended Ternet V1 language. A feature is not considered implemented until compiler/runtime code and tests exist for it.

## Design goals

Ternet is a statically typed, memory-safe, general-purpose language with distinctive syntax, inference, immutable-by-default bindings, ownership/borrowing, pattern matching, traits, generics, async/await, Result/Option, a bytecode VM, and a future native backend.

Ternet is not intended to be a clone of Rust, Python, Java or C++. Proven concepts may be adopted while keeping Ternet's own syntax and toolchain.

## Source model

Files use `.trn`. V1 uses a single colon as the statement terminator.

```trn
let name = "Ajmal":
tnprint(name):
```

Blocks use braces. Conditions use the distinctive `if {condition};` form.

## Output

```trn
tnprint("Hello, Ternet"):
tnprint("Value interpolation is supported by the string system"):
```

## Bindings

```trn
let age = 15:
mut age2 = 15:
age2 = 16:
const APP_NAME = "Ternet":
comptime VERSION = "1.0.0":
```

## Conditions

```trn
if {0 == 0};
    tnprint("hello world"):
else {};
    tnprint("hello"):
```

Multiple branches:

```trn
if {score >= 90};
    tnprint("A"):
elif {score >= 75};
    tnprint("B"):
else {};
    tnprint("C"):
```

Conditional expressions are also part of the V1 design:

```trn
let label = if {age >= 18} "adult" else "minor":
```

## Match

```trn
match status {
    "online" => tnprint("Online"):
    "offline" => tnprint("Offline"):
    _ => tnprint("Unknown"):
}:
```

Ranges and structured patterns are planned for the V1 matcher.

## Loops

```trn
for {i in 0..10};
    tnprint(i):

while {count < 10};
    count += 1:

loop {};
    work():
```

`break` and `continue` control loops.

## Functions

```trn
fn add(a: int, b: int) -> int {
    return a + b:
}:

let result = add(10, 20):
```

Lambdas:

```trn
let double = |x| x * 2:
```

## Types

Core types include `bool`, `char`, `str`, `byte`, `int`, `uint`, `float32`, `float64`, `void`, `never`, `size`, and `duration`.

Compound types include arrays, maps, tuples, structs, classes, enums, `Option<T>`, `Result<T,E>`, references and generics.

## Structs

```trn
struct User {
    id: int:
    name: str:
}:

let user = User {
    id: 1,
    name: "Ajmal"
}:
```

## Classes

```trn
class Player {
    public name: str:
    private health: int:

    init(name: str) {
        self.name = name:
        self.health = 100:
    }:

    public fn attack(self) -> void {
        tnprint("Player attacks"):
    }:
}:
```

## Traits

```trn
trait Drawable {
    fn draw(self) -> void:
}:

impl Drawable for Player {
    fn draw(self) -> void {
        tnprint("Drawing"):
    }:
}:
```

## Generics

```trn
fn first<T>(items: Array<T>) -> T {
    return items[0]:
}:
```

## Ownership and borrowing

V1 targets deterministic ownership:
- each owned value has one logical owner
- moving transfers ownership
- immutable borrows may coexist
- mutable borrowing is exclusive
- references cannot outlive owners
- invalid ownership use is a compile-time error

Reference syntax:

```trn
fn show(value: &str) -> void {
    tnprint(value):
}:

fn edit(value: &mut str) -> void {
    *value = "changed":
}:
```

## Option and Result

```trn
let name: Option<str> = Some("Ajmal"):
let missing: Option<str> = None:
```

```trn
fn read_config() -> Result<str, Error> {
    ...
}:
```

## Async

```trn
async fn fetch() -> Result<str, Error> {
    let response = await http.get(url):
    return Ok(response.body):
}:
```

## Modules

```trn
import net.http:
from json import parse:
import filesystem as fs:
```

## Compile-time features

```trn
comptime VERSION = "1.0.0":

#if DEBUG {
    tnprint("debug"):
}:
```

Compile-time execution must be sandboxed and deterministic unless a future capability system explicitly grants external access.

## Package manifest

`ternet.toml`:

```toml
[package]
name = "my_app"
version = "1.0.0"

[dependencies]
http = "1.0"
json = "1.0"
```

## Compiler pipeline

```
.trn
  -> Lexer
  -> Parser
  -> AST
  -> Name Resolver
  -> Type Checker
  -> Borrow Checker
  -> Semantic Analyzer
  -> Ternet IR
  -> Optimizer
  -> Bytecode
  -> TVM
```

A future native backend may consume the IR.

## Documentation truth rule

The repository distinguishes:
- **Implemented:** code + tests exist.
- **In progress:** code exists but is incomplete/unstable.
- **Specified:** language contract exists, implementation does not.
- **Future:** intentionally postponed.

No specified/future feature may be advertised as currently available.
