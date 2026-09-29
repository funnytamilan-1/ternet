# Ternet Implementation Status

This tracker is the source of truth for the requested platform scope. A feature is **planned** until executable implementation and regression tests are present.

## Existing implementation baseline

The current development build already includes `.trn` lexing, primitive values, `let`/`mut`/`const`, operators, arrays, control flow, named functions, the reference VM/interpreter, CLI/package commands, local module imports, enums/basic match, struct member access, and webfile lowering. See the existing regression suite before extending these systems.

## Language Core

- [ ] let / mut / const — extend existing implementation with full compile-time semantics
- [ ] Functions / closures
- [ ] Classes / objects
- [ ] Struct / enum — extend existing struct/enum support to full semantics
- [ ] Traits / interfaces
- [ ] Generics
- [ ] Pattern matching — extend basic match to exhaustive/destructuring matching
- [ ] Type inference
- [ ] Option / Result
- [ ] Error handling

## Collections

- [ ] List
- [ ] Dict / Map
- [ ] Set
- [ ] Tuple
- [ ] Iterator

## Runtime

- [ ] VM — harden current reference/bytecode paths
- [ ] Bytecode — verifier/compiler completeness
- [ ] GC
- [ ] Modules — production namespaces and separate compilation
- [ ] FFI
- [ ] Native extensions

## Concurrency

- [ ] async / await
- [ ] Tasks
- [ ] Threads
- [ ] Channels
- [ ] Synchronization

## Data

- [ ] JSON
- [ ] TOPS
- [ ] File I/O
- [ ] Serialization

## Networking

- [ ] HTTP / HTTPS
- [ ] REST
- [ ] WebSocket
- [ ] TCP / UDP
- [ ] DNS

## Database

- [ ] SQLite
- [ ] PostgreSQL
- [ ] MySQL
- [ ] Connection pooling

## App Development

- [ ] UI components
- [ ] Events
- [ ] State
- [ ] Navigation
- [ ] Forms
- [ ] Animations
- [ ] Local storage
- [ ] Notifications

## Tooling

- [ ] tnc — extend existing CLI with production compiler workflows
- [ ] Package manager — extend existing local package commands with a remote registry
- [ ] Formatter
- [ ] Linter
- [ ] Debugger
- [ ] Test framework
- [ ] REPL
- [ ] Documentation generator

## IDE

- [ ] VS Code
- [ ] Syntax highlighting
- [ ] IntelliSense
- [ ] LSP
- [ ] Go to definition
- [ ] Diagnostics
- [ ] Formatting
- [ ] Debugging

## Implementation rule

Do not mark an item complete from documentation or a stub alone. Each item requires executable code, integration, and regression tests. Unsupported features must remain explicitly unimplemented rather than being represented by placeholders as if they were production-ready.
