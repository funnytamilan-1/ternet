#!/usr/bin/env python3
"""Validate repository-level Ternet tooling metadata.

This intentionally checks metadata/tooling only; it never treats the presence of
keywords or documentation as proof that a language feature is implemented.
"""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
errors: list[str] = []


def fail(message: str) -> None:
    errors.append(message)


# .gitattributes must explicitly classify Ternet source files.
attrs = ROOT / ".gitattributes"
if not attrs.is_file():
    fail("missing .gitattributes")
else:
    text = attrs.read_text(encoding="utf-8")
    if "*.trn linguist-language=Ternet" not in text:
        fail(".gitattributes does not map *.trn to Ternet")

# Both maintained VS Code copies must contain valid JSON and the same grammar.
grammars = [
    ROOT / "syntaxes/ternet.tmLanguage.json",
    ROOT / "vscode-ternet/syntaxes/ternet.tmLanguage.json",
    ROOT / "vscode/ternet-language/syntaxes/ternet.tmLanguage.json",
]
parsed = []
for path in grammars:
    if not path.is_file():
        fail(f"missing grammar: {path.relative_to(ROOT)}")
        continue
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON in {path.relative_to(ROOT)}: {exc}")
        continue
    if data.get("scopeName") != "source.ternet":
        fail(f"wrong scopeName in {path.relative_to(ROOT)}")
    if not isinstance(data.get("patterns"), list):
        fail(f"grammar patterns must be an array in {path.relative_to(ROOT)}")
    parsed.append(data)

if parsed and any(data != parsed[0] for data in parsed[1:]):
    fail("Ternet TextMate grammar copies are out of sync")

# Linguist submission aid must remain a proposal, not a fabricated generated ID.
linguist = ROOT / "contrib/github-linguist/Ternet.language.yml"
if not linguist.is_file():
    fail("missing Linguist submission definition")
else:
    text = linguist.read_text(encoding="utf-8")
    if "Ternet:" not in text or '".trn"' not in text:
        fail("invalid/incomplete Linguist submission definition")
    if "language_id:" in text:
        fail("language_id must not be manually supplied in the submission aid")

# Keep representative samples available for an eventual upstream Linguist PR.
samples = list((ROOT / "contrib/github-linguist/samples/Ternet").glob("*.trn"))
if not samples:
    fail("no Ternet Linguist samples found")

if errors:
    print("Ternet repository validation failed:")
    for error in errors:
        print(f"- {error}")
    raise SystemExit(1)

print("Ternet repository metadata validation passed.")
