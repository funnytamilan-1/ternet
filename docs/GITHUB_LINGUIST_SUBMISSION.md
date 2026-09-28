# GitHub Linguist support for Ternet

Ternet uses the `.trn` source-file extension.

## Repository-side support

The repository already provides:

- `.trn` source files
- `.gitattributes` mapping `*.trn` to the language name `Ternet`
- a TextMate grammar at `syntaxes/ternet.tmLanguage.json`
- real Ternet examples and tests
- documentation describing the language syntax

## What this does

The repository configuration is enough to prepare Ternet files for a future GitHub Linguist submission, but it does **not** register a new language globally in GitHub.

Global language-bar recognition is controlled by GitHub Linguist. The official submission must be made to the `github-linguist/linguist` project and accepted there.

## Upstream submission plan

When Ternet has sufficient public usage, the upstream Linguist change should include:

1. Add a `Ternet` language entry to `lib/linguist/languages.yml`.
2. Register `.trn` as the source extension.
3. Set the appropriate language type and color.
4. Add the Ternet TextMate grammar to Linguist's grammar collection.
5. Add representative Ternet sample files under Linguist's samples directory.
6. Generate/update Linguist's language ID data as required by the current contribution workflow.
7. Run Linguist's tests and sample validation.
8. Open an upstream pull request with evidence that Ternet meets the current usage requirements.

## Important

Do not spoof another language (for example C++) just to change the repository language bar. Ternet should be recognized as Ternet only after GitHub Linguist has an actual Ternet language definition.

The current repository mapping is intentionally truthful:

```gitattributes
*.trn linguist-language=Ternet
*.trn text
```

This prepares the repository without falsely claiming that GitHub globally recognizes Ternet yet.
