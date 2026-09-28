# Ternet Full Language Specification

Ternet is designed as a general-purpose language, not a Node.js clone.

## Project entry

A project may contain Node.trn. `tnc run` uses Node.trn first, then src/main.trn.

## Toolchain

Source -> Lexer -> Parser -> Semantic Analyzer -> IR -> Bytecode -> Verifier -> TVM -> OS capability layer.

## Core language

- immutable let
- mutable mut
- const and comptime
- static types and future inference
- arrays, maps, tuples
- functions and closures
- classes, interfaces, traits and generics
- enums
- Option/Result
- ownership and borrowing
- pattern matching
- async/await
- modules and imports
- attributes
- compile-time configuration

## Standard library

Target modules include core, collections, string, math, time, fs, path, io, json, toml, net, http, tls, websocket, crypto, process, env, async, sync, log, test and bench.

Optional ecosystem packages can provide databases, web frameworks, AI, media and game tooling.

## Package management

Project metadata lives in ternet.toml; exact dependency resolution belongs in ternet.lock.

Current tnc add/install/list/remove/package implementation is local metadata management. A remote registry, cryptographic integrity verification, signatures and publishing protocol are not yet implemented.

## Diagnostics

Stable diagnostic codes are reserved for toolchain errors. The next diagnostic contract should include code, severity, file, start/end line and column, message, explanation, safe suggestion and related source spans.

## IDE

VS Code support includes .trn registration, TextMate highlighting, completion, basic definition navigation, run/check commands and formatter integration.

Precise compiler diagnostics, semantic IntelliSense, LSP, refactoring and DAP debugging require corresponding compiler/runtime protocols and are not claimed as complete.

## Container/host tooling

Ternet targets Docker/OCI-compatible application packaging rather than implementing a container engine in the language. Capability policy should control filesystem, network, process, environment, device and native-FFI access.

## Security

Sandboxed execution must not receive ambient OS privileges. Sensitive capabilities must be explicit and auditable. Bytecode verification and resource limits are required before production sandbox claims.

## Truthfulness rule

A feature is implemented only when source code and regression tests demonstrate it. Design documents do not count as implementation.
