# Ternet Language Platform Feature Contracts

This document defines the implementation contracts for the requested Ternet platform features. It is intentionally explicit about which parts require compiler/runtime/native integration.

## Language core

### Variables

- `let`: immutable binding by default.
- `mut`: mutable binding.
- `const`: compile-time constant where the type permits constant evaluation.

### Functions and closures

Functions have typed or inferred parameters and return values. Closures capture referenced bindings according to the runtime's ownership/lifetime rules.

### Classes, structs, enums

Classes provide methods and object identity. Structs provide value-oriented records. Enums provide tagged variants and are the basis for exhaustive pattern matching.

### Traits and interfaces

Traits/interfaces define required methods and associated behavior. Implementations must satisfy the declared method signatures before code generation.

### Generics

Generic functions, types, and methods use compile-time type parameters. Constraints are checked before bytecode/native code generation.

### Pattern matching

`match` must support literals, enum variants, tuple/list destructuring, and a wildcard arm. Exhaustiveness diagnostics are required for closed enums.

### Type inference

Infer local bindings and expression types where possible while preserving explicit annotations. Type errors must identify the expression and expected/actual types.

### Option / Result

`Option<T>` represents a value-or-none state. `Result<T, E>` represents success-or-error state. Both integrate with pattern matching and error propagation.

## Collections

- List: ordered dynamic sequence.
- Dict/Map: key/value collection.
- Set: unique hashable values.
- Tuple: fixed-size heterogeneous value collection.
- Iterator: lazy traversal abstraction with composable operations.

## Runtime

The runtime contract includes bytecode execution, garbage collection, module loading, FFI, and native extension loading. Native boundaries must validate value ownership and lifetime.

## Concurrency

Async tasks, `async`/`await`, threads, channels, and synchronization primitives must provide deterministic error propagation and safe shutdown semantics.

## Data and networking

JSON and TOPS provide serialization/deserialization. File I/O, HTTP/HTTPS, REST, WebSocket, TCP/UDP, and DNS belong in the standard library or native-backed modules rather than the parser itself.

## Database

SQLite, PostgreSQL, and MySQL drivers should expose parameterized queries and explicit connection lifecycle management. Connection pooling is a runtime/library concern.

## Application development

UI components, events, state, navigation, forms, animations, local storage, and notifications require platform-specific backends. The language API should remain platform-neutral where practical.

## Tooling and IDE

The compiler executable (`tnc`) should expose stable diagnostics and machine-readable output. Formatter, linter, debugger, test runner, REPL, documentation generator, and LSP should consume the same parser/type information to avoid duplicated language semantics.

## Implementation rule

A feature is considered implemented only when executable code and regression tests exist. Documentation, declarations, or API stubs alone do not count as implementation.
