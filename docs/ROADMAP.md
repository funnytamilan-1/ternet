# Ternet Development Roadmap

## Phase 0 — Current baseline

Implemented:
- C++17 project
- lexer
- parser
- AST for print statements
- bytecode compiler
- TVM
- CLI
- .trn extension
- basic tests/examples

## Phase 1 — Core language

- identifiers
- numeric/bool/null literals
- variable bindings
- expression parser
- arithmetic/comparison/logical operators
- distinctive if/elif/else syntax
- loops
- arrays
- diagnostics
- lexer/parser/compiler/VM unit tests

## Phase 2 — Functions and types

- functions
- parameters and returns
- lexical scopes
- type checking
- inference
- structs
- enums
- generics

## Phase 3 — Safety

- references
- ownership
- borrow checking
- Option
- Result
- deterministic resource handling

## Phase 4 — OOP and modules

- classes
- constructors
- methods
- visibility
- traits
- impl blocks
- module resolution

## Phase 5 — Async and standard library

- async/await
- scheduler
- filesystem
- JSON
- HTTP
- processes
- time
- collections

## Phase 6 — Toolchain

- formatter
- linter
- test runner
- REPL
- package manager
- documentation generator
- lockfile
- reproducible builds

## Phase 7 — Runtime hardening

- bytecode verifier
- robust runtime errors
- resource limits
- profiling hooks
- fuzzing

## Phase 8 — Native backend

- stable Ternet IR
- native code generation
- platform targets
- FFI
- release packaging

A version is complete only when implementation, tests, documentation and diagnostics agree.
