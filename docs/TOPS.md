# TOPS

**Ternet Object Persistence Syntax** — a small, human-readable data serialization format for the Ternet ecosystem.

File extension: `.tops`

## V1 Types

- string
- int
- float
- bool
- null
- array
- object

## Example

```tops
{
    name = "Ajmal"
    age = 15
    language = "Ternet"
    active = true

    skills = [
        "Python"
        "C++"
        "Ternet"
    ]
}
```

## Nested objects

```tops
{
    user = {
        name = "Ajmal"
        role = "Developer"
    }

    projects = [
        {
            name = "Ternet"
            version = 1
        }
    ]
}
```

## Design goals

1. Simple to read and write by humans.
2. Easy to parse without JavaScript-specific semantics.
3. Native-friendly for Ternet configuration and persistence.
4. Deterministic and strict enough for tooling.

TOPS is an original Ternet ecosystem format and is not intended to be a drop-in replacement for JSON.
