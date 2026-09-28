# Ternet Security Model

Ternet is designed for systems, backend, automation, networking and security-sensitive software. Security is a language/toolchain property, not a claim that every application written in Ternet is automatically secure.

## Security principles

1. Least privilege by default
2. Explicit capabilities for privileged operations
3. Memory safety as a compiler/runtime goal
4. No ambient filesystem, process, network or secret access for sandboxed code
5. Dependency integrity and reproducible builds
6. Verified bytecode before TVM execution
7. Deterministic diagnostics and bounded resource use
8. Unsafe operations must be explicit and auditable

## Capability model

Sensitive APIs are capability-gated:

- fs.read / fs.write
- net.connect / net.listen
- process.spawn
- environment and secret access
- native FFI
- device access
- raw sockets

Target launcher model:

    tnc run app.trn --allow fs:read=./config --allow net:connect

The exact flag grammar is a V1 toolchain contract and is not considered implemented until launcher enforcement and tests exist.

## Trust boundaries

    Source -> Lexer -> Parser -> Semantic checks -> IR -> Bytecode
                                                       |
                                                       v
                                               Bytecode verifier
                                                       |
                                                       v
                                                       TVM
                                                /       |       \
                                           memory    syscalls   limits

Untrusted bytecode must never bypass verification.

## Memory safety

The target V1 model combines static ownership/borrowing rules with runtime checks where required. The compiler must reject use-after-move, invalid borrows, incompatible mutable aliases and references that outlive their owners.

## Supply-chain security

The package manager is planned to support lockfiles, cryptographic package checksums, immutable package versions, dependency graph inspection, reproducible builds, optional signature verification and audit output.

## Unsafe boundary

Native FFI, raw memory, platform-specific syscalls and other operations that can violate memory guarantees belong behind an explicit unsafe boundary and should be restricted by policy.

## Status

This document specifies the target security model. It does not claim that all controls are implemented in the current repository. See docs/STATUS.md for actual implementation state.