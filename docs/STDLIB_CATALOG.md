# Ternet Standard Library Catalog

The standard library is split into capability-oriented modules. A module is considered implemented only when its code and tests exist.

## Core
- core — primitives, conversions and assertions
- collections — Array, Map, Set and Queue
- option — Option<T>
- result — Result<T,E>
- string — Unicode-aware string utilities
- math — numeric operations
- random — deterministic and OS-backed random APIs

## Files and data
- fs — files and directories with capability checks
- path — platform-safe paths
- io — streams and buffered I/O
- json — JSON parsing and serialization
- toml — TOML parsing
- csv — CSV parsing and serialization

## Time
- time — timestamps and durations
- timezone — timezone conversion
- timer — timers and scheduling

## Networking
- net — sockets and DNS
- http — HTTP client and server
- tls — TLS configuration
- url — URL parsing
- websocket — WebSocket client and server

Network operations require explicit capabilities in sandboxed execution.

## Security and cryptography
- crypto.hash — cryptographic hashes
- crypto.hmac — HMAC
- crypto.random — OS-backed secure random
- crypto.aead — authenticated encryption
- crypto.keys — key-material handling
- crypto.x509 — certificate parsing and validation

High-level safe APIs are preferred over exposing low-level primitives.

## OS and processes
- process — process execution
- env — environment access
- signals — signal handling
- os — platform information
- terminal — terminal I/O

Process and environment access are capability-gated.

## Concurrency
- async — futures and tasks
- sync — locks and synchronization
- channel — typed channels
- atomic — atomic primitives

## Developer tooling
- log — structured logging
- trace — tracing
- test — assertions and test support
- bench — benchmarks
- inspect — diagnostics and runtime inspection

## Databases

Database drivers should remain packages instead of bloating the language core. Planned ecosystem targets include SQLite, PostgreSQL, MySQL-compatible servers and Redis. Drivers must expose parameterized queries rather than encouraging string-concatenated SQL.

## Web, AI, media and games

Optional packages may provide routing, middleware, sessions, authentication, model clients, streaming, FFmpeg integration, graphics, audio, physics and other specialized APIs. These remain outside the compiler core.

## Status

This catalog is a design target. It must not be advertised as currently implemented unless the repository contains the implementation and tests.