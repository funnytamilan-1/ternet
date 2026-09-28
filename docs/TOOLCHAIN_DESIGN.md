# Ternet Toolchain Design

## Unified CLI

    tnc init
    tnc check
    tnc build
    tnc run
    tnc test
    tnc bench
    tnc fmt
    tnc lint
    tnc repl
    tnc doc
    tnc package
    tnc install
    tnc update
    tnc audit
    tnc publish
    tnc inspect
    tnc container

## Compiler modes

check performs lexical, syntactic and semantic validation without running the program.
build produces verified bytecode or a future native artifact.
run builds when necessary, verifies the artifact and executes it with configured capabilities and resource limits.
audit reports dependencies, integrity metadata, requested capabilities and unsafe boundaries.
inspect displays bytecode metadata, module dependencies and compiler information.

## Package manifest

    [package]
    name = "my_app"
    version = "0.1.0"
    edition = "2026"

    [dependencies]
    json = "1.0"
    http = "1.0"

    [capabilities]
    network = ["api.example.com:443"]
    filesystem_read = ["./config"]

Capability declarations require launcher enforcement before they are considered functional.

## Lockfile

ternet.lock records exact dependency versions, source locations and integrity hashes.

## Diagnostics

Diagnostics should include a source span, stable error code, concise message, explanation, safe fix suggestion and related spans when useful.

Example:

    E1007: cannot assign to immutable binding
     --> src/main.trn:4:1
      |
    4 | count = count + 1:
      | ^^^^^
      help: use mut count when mutation is required

## Security invariant

The CLI must never silently widen capabilities because a program requested them. Sensitive capabilities must be explicitly granted by the user or deployment policy.