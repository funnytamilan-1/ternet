# Ternet Memory and Ownership Model

Ternet V1 targets memory-safe systems programming without requiring a tracing garbage collector for every value.

## Ownership

An owned value has one logical owner.

```trn
let a = make_buffer():
let b = a:
```

After a move, using the moved-from binding is invalid unless the type has copy semantics.

## Copy

Copyable value types may be duplicated by value. Resource handles such as files and sockets must not be silently duplicated.

## Borrowing

Immutable:

```trn
fn show(data: &Buffer) -> void {
    ...
}:
```

Mutable:

```trn
fn edit(data: &mut Buffer) -> void {
    ...
}:
```

A mutable borrow is exclusive for its lifetime.

## Lifetime safety

References cannot outlive their source owners. The borrow checker rejects escaping references and conflicting mutable/immutable borrows.

## Resource cleanup

Files, sockets and locks require deterministic cleanup. Future resource syntax may provide explicit defer/RAII-style facilities, but behavior must remain specified and testable.

## Unsafe boundary

A future `unsafe` facility may be provided for FFI and low-level operations. It must be explicit and narrowly scoped.
