# Ternet Security Standard Library

Security-focused APIs for defensive development, CTF/lab tooling, secure automation, and security research.

The standard library should expose safe, auditable primitives rather than exploit automation.

Planned public modules:

- `crypto` — cryptographic hashing and authenticated primitives backed by vetted system/library implementations.
- `net` — TCP/UDP and address parsing with explicit timeouts.
- `http` — HTTP/HTTPS client APIs with certificate verification enabled by default.
- `bytes` — byte buffers, hexadecimal encoding and decoding.
- `process` — explicitly controlled subprocess APIs.
- `audit` — structured security/audit logging.

Security rules:

1. TLS certificate verification is enabled by default.
2. APIs must validate lengths, paths and resource limits.
3. Package extraction must reject traversal paths.
4. Network and process APIs must not silently bypass operating-system security controls.
5. Offensive automation is not part of the standard library contract.
