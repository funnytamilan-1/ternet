# Ternet Package Manager

Project creation:

    tnc init my_app
    cd my_app

Dependency workflow:

    tnc add json 1.0
    tnc install
    tnc list
    tnc remove json

Manifest:

    [package]
    name = "my_app"
    version = "0.1.0"
    edition = "2026"

    [dependencies]
    json = "1.0"

`tnc install` currently records requested package specifications in `ternet.lock` and creates local metadata under `.ternet/packages/`. It intentionally does not download arbitrary remote code.

Remote registry resolution, integrity hashes, signatures, dependency graph verification and sandboxed build scripts remain security milestones.