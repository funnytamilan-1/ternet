# Ternet

Ternet is a C++17 programming-language project built around readable \`.trn\` source files, the \`tnc\` command-line tool, a reference interpreter, and a bytecode/TVM execution path.

> **Truthful status:** Ternet is actively implemented, but it is **not yet a completely finished production programming language**. Some language features, standard-library modules, package downloading, resource limits, native compilation, and security enforcement remain work in progress. This README documents implemented behavior separately from planned behavior.

## Quick start

Build:

\`\`\`bash
cmake -S . -B build
cmake --build build
\`\`\`

Run:

\`\`\`bash
./build/tnc run examples/hello.trn
\`\`\`

Check:

\`\`\`bash
./build/tnc check examples/hello.trn
\`\`\`

Compile and execute bytecode:

\`\`\`bash
./build/tnc build examples/hello.trn -o hello.tbc
./build/tnc exec hello.tbc
\`\`\`

## Hello Ternet

\`\`\`trn
tnprint("Hello, Ternet"):
tnprint("Welcome to .trn"):
\`\`\`

Statements can currently use \`:\` or \`;\` terminators.

## Variables

\`\`\`trn
let name = "Ajmal":
mut score = 10:
score = 20:
const answer = 42:

tnprint(name):
tnprint(score):
tnprint(answer):
\`\`\`

\`let\` and \`const\` bindings are immutable in the current runtime; \`mut\` creates a mutable binding.

## Global/top-level variables

Top-level bindings live in the program's top-level execution scope:

\`\`\`trn
let app_name = "Ternet":
mut request_count = 0:

lit show_app() {
    tnprint(app_name):
}

show_app():
\`\`\`

A complete separate compilation/module-global system is not yet implemented, so do not confuse top-level scope with a finished linker-level global-variable ABI.

## The \`lit\` function keyword

Ternet now supports \`lit\` as a function declaration keyword:

\`\`\`trn
lit add(a, b) {
    return a + b:
}

let result = add(20, 22):
tnprint(result):
\`\`\`

\`fn\` remains supported:

\`\`\`trn
fn multiply(a, b) {
    return a * b:
}
\`\`\`

The important semantic rule is simple: **\`lit\` declares a function; it is not a variable datatype.**

## Functions

Current function support includes named functions, parameters, calls, return values and recursion.

\`\`\`trn
lit square(x) {
    return x * x:
}

tnprint(square(12)):
\`\`\`

Recursive example:

\`\`\`trn
lit fact(n) {
    if {n <= 1}; {
        return 1:
    }
    return n * fact(n - 1):
}

tnprint(fact(5)):
\`\`\`

## Current runtime value types

The current \`Value\` representation contains:

| Type | Example |
|---|---|
| null | \`null\` |
| bool | \`true\` |
| integer | \`42\` |
| float | \`3.14\` |
| string | \`"hello"\` |
| array | \`[1, 2, 3]\` |
| object | runtime representation exists, but object literals are not yet a complete surface feature |

Basic static checking exists for several of these categories.

## Expressions and operators

Implemented core operators:

\`\`\`text
+  -  *  /  %
== !=
>  <  >= <=
&& ||
!
\`\`\`

Example:

\`\`\`trn
let a = 10:
let b = 3:

tnprint(a + b):
tnprint(a - b):
tnprint(a * b):
tnprint(a / b):
tnprint(a % b):
tnprint(a > b):
tnprint(a == 10):
\`\`\`

Division by zero is rejected by the runtime.

## Strings

Escaped strings are supported:

\`\`\`trn
let text = "line1\nline2":
tnprint(text):
\`\`\`

The editor grammar also recognizes interpolation syntax:

\`\`\`trn
let name = "Ajmal":
tnprint("Hello \${name}"):
\`\`\`

## Arrays

Basic arrays and indexing are implemented:

\`\`\`trn
let numbers = [10, 20, 30, 40]:
tnprint(numbers[0]):
tnprint(numbers[3]):
\`\`\`

Out-of-range array access raises a runtime error.

A complete collections library with push/pop/map/filter/set/map/generic iterator APIs is still future work.

## Conditions

\`\`\`trn
let age = 15:

if {age >= 18}; {
    tnprint("adult"):
} else {
    tnprint("minor"):
}
\`\`\`

The parser also contains \`elif\` support.

## Loops

The implemented baseline includes \`while\`:

\`\`\`trn
mut count = 0:

while {count < 5}; {
    tnprint(count):
    count = count + 1:
}
\`\`\`

\`break\` and \`continue\` are represented in the runtime/compiler paths.

The \`for\` keyword exists in the language vocabulary, but the complete for-loop parser/runtime semantics are not yet complete.

## Errors

Reference-runtime exception syntax includes:

\`\`\`trn
try {
    throw "something went wrong":
} catch error {
    tnprint(error):
} finally {
    tnprint("finished"):
}
\`\`\`

The bytecode exception path is not yet advertised as complete.

## Web generation

The reference runtime can write generated files into \`dist/\`:

\`\`\`trn
webfile "index.html" {
    "<!doctype html>":
    "<html><body>":
    "<h1>Hello from Ternet</h1>":
    "</body></html>":
}
\`\`\`

Build:

\`\`\`bash
./build/tnc web build tests/web.trn
\`\`\`

The current implementation rejects absolute paths and parent-directory traversal for generated web files.

## Bytecode / TVM

Ternet has two execution paths: a reference interpreter and a bytecode VM.

\`\`\`text
Ternet source
    |
    v
Lexer
    |
    v
Parser / AST
    |
    +----> Reference Interpreter
    |
    v
Bytecode Compiler
    |
    v
Bytecode + Verifier
    |
    v
TVM
\`\`\`

The implemented bytecode path includes core expressions, variables, arrays/indexing, control flow, short-circuit operations and function call/return frames.

## Static checker

Run:

\`\`\`bash
./build/tnc check file.trn
\`\`\`

The checker currently covers basic literals, bindings, arithmetic/comparison, arrays, function arity, assignments, conditions, returns and several statement forms.

It is **not** yet a complete ownership/borrowing, generics, trait, lifetime or advanced inference checker.

## Package/project commands

Current project commands:

\`\`\`bash
tnc init
tnc add json 1.0.0
tnc list
tnc install
tnc remove json
tnc package
\`\`\`

A project may look like:

\`\`\`text
my_app/
├── Node.trn
├── ternet.toml
├── ternet.lock
├── src/
│   └── main.trn
└── .ternet/
\`\`\`

The current package implementation creates dependency metadata and a lockfile/package directory. It is **not yet a remote registry client** with dependency graph solving, downloads, checksums, signatures and registry mirrors.

## Standard library vision

The standard library is intentionally designed as a collection of focused modules.

### Core

\`\`\`text
core
string
collections
option
result
\`\`\`

### Math

Target modules:

\`\`\`text
math
math.constants
math.trigonometry
math.geometry
math.statistics
\`\`\`

Target API style:

\`\`\`trn
math.sqrt(25)
math.abs(-10)
math.pow(2, 8)
math.sin(angle)
math.cos(angle)
\`\`\`

These calls are **future API examples**, not claims that those modules are already implemented.

### Random

Target module:

\`\`\`text
random
\`\`\`

Target API:

\`\`\`trn
random.int(1, 100)
random.float()
random.choice(items)
random.shuffle(items)
\`\`\`

Ordinary pseudo-random generation and cryptographically secure randomness should be separate APIs.

### Files/data

\`\`\`text
fs
path
io
json
toml
csv
\`\`\`

### Network

\`\`\`text
net
http
tls
url
websocket
\`\`\`

### Async/concurrency

\`\`\`text
async
sync
channel
atomic
timer
\`\`\`

### Crypto

\`\`\`text
crypto.hash
crypto.hmac
crypto.random
crypto.encoding
\`\`\`

### OS/process

\`\`\`text
process
env
signals
os
terminal
\`\`\`

### Developer tools

\`\`\`text
log
trace
test
bench
inspect
\`\`\`

## Advanced language roadmap

These are architectural targets, not current feature claims:

- typed declarations and richer inference
- structs
- classes
- traits
- interfaces
- implementations
- enums
- pattern matching
- Option/Result
- nullable types
- generics
- closures/lambdas
- async/await
- channels
- ownership and borrowing
- attributes
- compile-time execution
- capability-gated I/O
- resource limits
- native backend
- WebAssembly
- debugger
- formatter
- linter
- language server
- REPL
- documentation generator
- benchmark framework
- remote package registry
- reproducible builds
- dependency integrity
- OCI/container integration

## Security

The intended trust boundary is:

\`\`\`text
Source -> Lexer -> Parser -> Semantic analysis
      -> IR/Bytecode -> Verifier -> TVM
\`\`\`

Sensitive capabilities should eventually be explicit:

\`\`\`text
fs.read
fs.write
net.connect
net.listen
process.spawn
environment/secret access
native FFI
device access
raw sockets
\`\`\`

Target command shape:

\`\`\`bash
tnc run app.trn --allow fs:read=./config --allow net:connect
\`\`\`

The capability model is a design target until enforcement and tests exist.

## CPU and RAM

The runtime uses memory for values, arrays, strings, scopes, bytecode and function frames. CPU is consumed during lexing, parsing, compilation and execution.

A complete configurable resource manager is not yet implemented.

Future resource controls can include:

\`\`\`text
CPU time
wall-clock timeout
heap/RAM
call depth
array/object size
output size
file size
network usage
\`\`\`

A possible future command is:

\`\`\`bash
tnc run app.trn --memory 256MB --timeout 5s
\`\`\`

## Repository structure

\`\`\`text
Include/                 Public C++ interfaces
Parser/                  Lexer + parser
Ternet/                  Reference runtime
compiler/                Bytecode + type checker
Programs/tnc/            CLI
Tools/                   Package commands
Modules/                 External integrations
tests/                   Regression tests
examples/                User examples
docs/                    Specifications and design
syntaxes/                TextMate grammar
contrib/                 Contribution aids
\`\`\`

## Editor and GitHub support

Ternet source uses \`.trn\`.

The repository includes:

- \`.gitattributes\`
- \`syntaxes/ternet.tmLanguage.json\`
- Linguist sample files
- a proposed Linguist language definition
- Linguist submission documentation

Repository-side mapping does not automatically register Ternet globally in GitHub. Global recognition requires an accepted GitHub Linguist change.

## Testing

Run:

\`\`\`bash
ctest --test-dir build --output-on-failure
\`\`\`

Every implemented feature should have positive coverage and negative coverage where meaningful.

Examples of existing test areas include:

- core execution
- loops
- errors
- block comments
- web generation
- Telegram client foundation
- bytecode
- arrays
- control flow
- short-circuiting
- functions
- type checking

## Development principles

### No fake compiler features

A feature is not called implemented because its syntax appears in a README.

### Implementation before marketing

Parser, runtime/compiler behavior and tests must exist before a feature is advertised as working.

### Documentation follows code

When implementation changes, the README and relevant docs must be updated.

### Security is enforcement

A security design document alone is not a security feature. Enforcement and regression tests are required.

### Small verified steps

Large language changes should be split into focused commits so regressions are easy to locate.

## Current implementation snapshot

### Working foundation

- \`.trn\` lexer
- comments and block comments
- strings and escapes
- numbers
- booleans and null
- arrays
- variables
- mutable/immutable bindings
- arithmetic/comparison
- logical operators
- conditions
- while loops
- functions
- recursion
- \`fn\`
- \`lit\` function declarations
- function calls
- return
- bytecode compilation
- bytecode execution
- bytecode verification
- basic static checking
- webfile output
- package/project commands
- HTTP health server
- Telegram client foundation
- GitHub Linguist preparation

### Still incomplete

- complete typed surface syntax
- complete for loops
- class/trait/interface execution
- generics
- pattern matching
- async runtime
- production math/random standard library
- remote package registry
- dependency resolution/integrity
- capability enforcement
- resource quotas
- native compiler
- production debugger/LSP
- full cross-platform toolchain
- complete security hardening

## Versioning

Ternet is still evolving. Until a stable language specification is declared, syntax and semantics may change.

## Contributing

Use \`main\` for current development and verify changes locally:

\`\`\`bash
git checkout main
git pull
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
\`\`\`

A good contribution contains the implementation, tests and documentation together.

## License

See the repository license file for the project's current licensing terms.
