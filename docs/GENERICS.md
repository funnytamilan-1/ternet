# Ternet Generics

## Status

Generic syntax and generic parameter representations are present in the parser/compiler. Full generic substitution, inference, and constraint checking remain implementation work.

## Required semantics

- Generic type parameters must be substituted with concrete types at use sites.
- Generic function calls should infer type arguments from argument and expected return types where possible.
- Explicit type arguments must be validated against the declared parameters.
- Generic constraints/bounds must be checked before instantiation.
- Nested and multiple generic arguments must preserve their complete type structure.
- Invalid generic applications must produce source-positioned diagnostics.

## Verification requirements

A generic feature is complete only after positive and negative compiler tests pass for functions, classes, nested types, inference, explicit arguments, and constraints.
