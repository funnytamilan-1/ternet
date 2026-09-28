# Ternet Architecture

## Compiler

The compiler is divided into:
1. Lexer: converts source text into tokens.
2. Parser: validates token order and creates an AST.
3. Compiler: converts AST nodes into bytecode.
4. TVM: executes bytecode.

## Runtime

The TVM currently has a small instruction set. The design intentionally keeps the instruction representation independent from the parser so future language features can be added without making the runtime parse source text.

## CLI

`tnc <file.trn>` compiles and executes a Ternet source file.

Errors are reported with source line information where available.
