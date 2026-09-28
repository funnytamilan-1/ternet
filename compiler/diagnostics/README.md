# Compiler Diagnostics

Compiler diagnostics are part of Ternet's stable developer interface.

Every diagnostic should contain a stable code, severity, source span, primary message, optional notes, and optional fix suggestion.

Reserved codes:

- T1001 invalid syntax
- T2001 unknown name
- T3001 type mismatch
- T3002 invalid operator types
- T4001 unresolved import
- T5001 invalid capability
