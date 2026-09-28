# Ternet Type System

Ternet uses static typing with inference. The compiler determines the type of every expression after semantic analysis.

## Primitive types

`bool`, `char`, `byte`, `int`, `uint`, `int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`, `float32`, `float64`, `str`, `void`, `never`, `size`, `duration`.

## Inference

```trn
let count = 10:
let active = true:
let name = "Ajmal":
```

## Explicit annotations

```trn
let count: int = 10:
```

## Collections

```trn
Array<int>
Map<str, int>
Tuple<int, str>
```

## Option

```trn
Some(value)
None
```

## Result

```trn
Ok(value)
Err(error)
```

## Conversions

Implicit conversions are deliberately limited. Explicit conversion APIs should be required for potentially lossy numeric conversions.

## Type aliases

```trn
type UserID = int:
type Username = str:
```

## Generics

Generic code is checked against declared constraints and instantiated by the compiler/runtime backend.

## Safety invariant

A well-typed program must not perform an operation outside the semantic contract of its types. Dynamic boundaries such as parsing external data still require runtime validation.
