# Publishing the Ternet VS Code Extension

The extension in this directory provides `.trn` language support and TextMate syntax highlighting for Ternet.

## Local package

```bash
cd vscode-ternet
npm install
npm install --save-dev @vscode/vsce
npx vsce package
```

This creates a `.vsix` package that can be installed locally in VS Code.

## Marketplace publishing

Publishing requires the Ternet publisher account and a Microsoft/Azure DevOps Personal Access Token (PAT) with Marketplace publishing permission. Do not commit the PAT to this repository.

After authenticating `vsce` with the publisher account:

```bash
npx vsce publish
```

After publication, users can install **Ternet Language** from the Visual Studio Code Marketplace and `.trn` files will use the Ternet language grammar.
