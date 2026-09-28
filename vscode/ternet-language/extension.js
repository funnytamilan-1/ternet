const vscode = require("vscode");
const cp = require("child_process");
const path = require("path");

function tnc(args, cwd) {
  return new Promise((resolve, reject) => {
    const p = cp.spawn("tnc", args, { cwd });
    let out = "", err = "";
    p.stdout.on("data", d => out += d);
    p.stderr.on("data", d => err += d);
    p.on("error", reject);
    p.on("close", code => resolve({ code, out, err }));
  });
}

function activate(context) {
  const diagnostics = vscode.languages.createDiagnosticCollection("ternet");
  context.subscriptions.push(diagnostics);

  context.subscriptions.push(vscode.languages.registerCompletionItemProvider("ternet", {
    provideCompletionItems() {
      return ["let","mut","const","fn","if","elif","else","while","for","return","break","continue","tnprint","true","false","null","import","from","async","await"].map(x => {
        return new vscode.CompletionItem(x, vscode.CompletionItemKind.Keyword);
      });
    }
  }));

  context.subscriptions.push(vscode.languages.registerDefinitionProvider("ternet", {
    provideDefinition(document, position) {
      const range = document.getWordRangeAtPosition(position);
      if (!range) return;
      const word = document.getText(range);
      const re = new RegExp("\\b(?:let|mut|const|fn)\\s+" + word + "\\b");
      for (let i = 0; i < document.lineCount; i++) {
        const line = document.lineAt(i).text;
        const m = re.exec(line);
        if (m) return new vscode.Location(document.uri, new vscode.Position(i, m.index + m[0].lastIndexOf(word)));
      }
    }
  }));

  context.subscriptions.push(vscode.commands.registerCommand("ternet.runFile", async () => {
    const e = vscode.window.activeTextEditor;
    if (!e) return;
    const r = await tnc(["run", e.document.fileName], path.dirname(e.document.fileName));
    const out = vscode.window.createOutputChannel("Ternet");
    out.clear(); out.append(r.out || r.err); out.show(true);
  }));

  context.subscriptions.push(vscode.commands.registerCommand("ternet.checkFile", async () => {
    const e = vscode.window.activeTextEditor;
    if (!e) return;
    const r = await tnc(["check", e.document.fileName], path.dirname(e.document.fileName));
    diagnostics.clear();
    if (r.code !== 0) {
      diagnostics.set(e.document.uri, [new vscode.Diagnostic(new vscode.Range(0,0,0,1), r.err || "Ternet check failed", vscode.DiagnosticSeverity.Error)]);
    } else {
      vscode.window.showInformationMessage("Ternet: check passed.");
    }
  }));

  context.subscriptions.push(vscode.commands.registerCommand("ternet.formatDocument", async () => {
    const e = vscode.window.activeTextEditor;
    if (!e) return;
    const edit = new vscode.WorkspaceEdit();
    for (let i = 0; i < e.document.lineCount; i++) {
      const line = e.document.lineAt(i);
      const trimmed = line.text.trim();
      if (trimmed) edit.replace(e.document.uri, line.range, trimmed);
    }
    await vscode.workspace.applyEdit(edit);
  }));
}

exports.activate = activate;
