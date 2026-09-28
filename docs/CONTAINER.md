# Ternet Container and Docker-Style Tooling

Ternet should provide a familiar container-oriented workflow while keeping containers outside the language core.

## Target commands

    tnc create
    tnc build --release
    tnc run
    tnc container build
    tnc container run
    tnc container inspect

## OCI integration

Ternet should generate OCI/Docker-compatible application artifacts rather than inventing a second container format. The language compiler produces the application artifact; the container tool packages that artifact with its declared runtime dependencies.

## Reproducible builds

Release metadata should record compiler version, target triple, dependency lockfile, source revision and build flags. Locked dependencies and deterministic inputs should produce reproducible artifacts where the target platform permits it.

## Runtime isolation

Container execution can additionally restrict filesystem paths, network access, CPU, memory, process count, environment variables and mounted devices. These are defense-in-depth controls and do not replace language-level safety.

## Project layout

    my_app/
    ├── src/
    ├── tests/
    ├── benches/
    ├── docs/
    ├── assets/
    ├── ternet.toml
    └── ternet.lock

## Status

The current repository does not implement a container engine. This document defines the intended tnc container interface and security boundary without pretending that Docker or OCI integration already works.