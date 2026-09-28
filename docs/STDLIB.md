# Ternet Standard Library

The standard library is separate from the language core.

## Core

Assertions, basic utilities and runtime primitives.

## Collections

Array, Map, Set, Queue, Stack and iterators.

## Text

Unicode-aware strings, parsing, formatting and search.

## Math

Numeric operations, constants and conversion helpers.

## Time

Durations, timestamps, clocks and timers.

## Filesystem

```trn
import fs:

let data = fs.read("data.txt"):
fs.write("out.txt", data):
```

Filesystem failures return typed errors.

## Networking

```trn
import net.http:

let response = await http.get("https://example.com"):
```

Secure TLS defaults should be used where applicable.

## JSON

External JSON parsing returns a typed error.

## Crypto

Cryptographic APIs should wrap reviewed primitives rather than implement cryptography from scratch in Ternet source.

## Database

Parameterized queries are the default API.

## Git

Git integration is a library/tooling layer, not a compiler keyword.

## AI

An optional AI client library may expose provider APIs. Credentials must remain outside source code.

## Game

Game development belongs in external libraries/ecosystem packages rather than the language core.
