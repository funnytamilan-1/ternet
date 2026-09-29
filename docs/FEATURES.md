# Ternet Feature Roadmap

This document tracks the language and application-development feature set. A feature is only marked complete after implementation and tests land; this roadmap does not claim unsupported runtime behavior.

## Language core

- let / mut / const
- functions and closures
- classes and objects
- structs and enums
- traits and interfaces
- generics
- type inference and static checking
- Option / Result
- pattern matching
- error handling

## Collections

- List / Array
- Dict / Map
- Set
- Tuple
- Iterator

## Runtime

- bytecode VM
- garbage collection
- modules
- native FFI
- native extensions

## Concurrency

- async / await
- tasks
- threads
- channels
- synchronization primitives

## Data

- JSON
- TOPS
- file I/O
- serialization / deserialization

## Networking

- HTTP / HTTPS
- REST helpers
- WebSocket
- TCP / UDP
- DNS

## Databases

- SQLite
- PostgreSQL
- MySQL
- connection pooling

## Application development

- UI components
- event system
- state management
- navigation
- forms
- animations
- local storage
- notifications

## Tooling

- tnc compiler
- package manager
- formatter
- linter
- debugger
- test framework
- REPL
- documentation generator

## IDE / LSP

- VS Code syntax highlighting
- IntelliSense / completion
- Language Server Protocol
- go to definition
- diagnostics
- formatting
- debugging integration

## Implementation policy

Do not mark a feature as implemented merely because a declaration, roadmap entry, or API stub exists. Each completed feature needs executable implementation and regression tests appropriate to the subsystem.
