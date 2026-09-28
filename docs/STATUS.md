# Ternet Implementation Status

This file prevents the project from confusing specification with implementation.

## Implemented today

- C++17 + CMake build
- .trn validation
- lexer with line/column information
- escaped strings
- tnprint("text")::
- multiple print statements
- print bytecode
- TVM execution
- basic invalid-source handling
- examples and tests for the baseline

## Specified but not implemented

- V1 single-colon statement syntax
- let/mut/const/comptime
- expressions/operators
- if {condition}; / elif / else {};
- loops
- functions
- arrays/maps
- structs/enums/classes
- traits
- generics
- ownership/borrowing
- Option/Result
- async/await
- modules/imports
- package manager
- standard library modules
- formatter/linter/test runner/REPL
- bytecode verification
- native backend

## Compatibility note

The current baseline intentionally uses `::` after `tnprint`. V1 moves to `:`. This migration must be implemented with compatibility tests; documentation must not imply both are accepted.

## Completion rule

A feature becomes Implemented only when source support, positive tests, negative tests, matching documentation and a passing build all exist.
