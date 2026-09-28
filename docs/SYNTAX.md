# Ternet Syntax Reference

This is the concise V1 design reference.

## Terminators

```trn
let x = 10:
tnprint(x):
```

## Blocks

```trn
{
    statement:
}
```

## Conditions

```trn
if {condition};
    statement:
else {};
    statement:
```

## Branch chains

```trn
if {a};
    one():
elif {b};
    two():
else {};
    three():
```

## Variables

```trn
let immutable = 10:
mut mutable = 20:
const LIMIT = 100:
```

## Types

```trn
let age: int = 15:
let name: str = "Ajmal":
```

## Functions

```trn
fn add(a: int, b: int) -> int {
    return a + b:
}:
```

## Arrays

```trn
let values = [1, 2, 3]:
values[0]:
```

## Match

```trn
match value {
    0 => zero():
    1 => one():
    _ => other():
}:
```

## Classes

```trn
class User {
    name: str:

    init(name: str) {
        self.name = name:
    }:
}:
```

## Traits

```trn
trait Printable {
    fn print(self) -> void:
}:
```

## Generics

```trn
fn first<T>(items: Array<T>) -> T {
    return items[0]:
}:
```

## References

```trn
&value
&mut value
*reference
```

## Async

```trn
async fn load() -> Result<str, Error> {
    return await fetch():
}:
```

## Imports

```trn
import net.http:
from json import parse:
import filesystem as fs:
```

## Comments

```trn
// comment
/// documentation
```
