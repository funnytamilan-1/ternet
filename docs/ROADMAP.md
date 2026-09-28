# Ternet Development Roadmap

## Phase 0 — Current baseline
- C++17 project
- lexer
- parser
- AST for print statements
- bytecode compiler
- TVM
- CLI
- `.trn` extension
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
- explicit unsafe boundaries
- capability model

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
- TLS
- processes
- time
- collections
- cryptography
- logging/tracing

## Phase 6 — Secure package and build system
- `ternet.toml` manifest
- `ternet.lock` lockfile
- dependency resolution
- integrity hashes
- reproducible builds
- package audit
- package registry client

## Phase 7 — Developer and container tooling
- formatter
- linter
- test runner
- REPL
- documentation generator
- profiler
- `tnc inspect`
- `tnc container` OCI/Docker integration
- container resource/capability policy

## Phase 8 — Runtime hardening
- bytecode verifier
- robust runtime errors
- resource limits
- fuzzing
- malformed-bytecode tests
- sandbox/capability enforcement

## Phase 9 — Native backend
- stable Ternet IR
- native code generation
- platform targets
- FFI behind explicit unsafe boundaries
- release packaging

A version is complete only when implementation, tests, documentation, diagnostics and security enforcement agree. Specified features are never described as implemented.