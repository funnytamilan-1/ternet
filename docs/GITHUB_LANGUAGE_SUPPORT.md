# GitHub Language Support

Ternet source files use the `.trn` extension.

## Repository configuration

The repository maps `*.trn` to the Ternet language name through `.gitattributes`:

```gitattributes
*.trn linguist-language=Ternet
```

The syntax grammar is maintained at:

```text
syntaxes/ternet.tmLanguage.json
```

The grammar uses TextMate scopes for Ternet keywords, types, strings, numbers, comments, operators and built-in functions.

## Important status

This repository-side grammar is the syntax definition used by editor tooling and is part of Ternet's language-support foundation.

GitHub's public Linguist language registry is a separate upstream project. A repository-local `.gitattributes` entry does not by itself add Ternet to GitHub's global Linguist registry. For global GitHub recognition/highlighting as a first-class language, Ternet should eventually contribute a language definition and grammar to GitHub Linguist upstream.

Until that upstream integration exists, the repository configuration should not be described as proof that every GitHub code view globally recognizes Ternet as a built-in language.

## Example

```trn
let name = "Ajmal":
if {name == "Ajmal"};
    tnprint("Hello ${name}"):
else {};
    tnprint("Hello"):
```
