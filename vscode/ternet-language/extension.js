const vscode = require("vscode");
const cp = require("child_process");
const path = require("path");

function tnc(args, cwd) {
  const configured = vscode.workspace.getConfiguration("ternet").get("tncPath", "tnc");
  return new Promise((resolve, reject) => {
    const p = cp.spawn(configured, args, { cwd });
    let out = "", err = "";
    p.stdout.on("data", d => out += d.toString());
    p.stderr.on("data", d => err += d.toString());
    p.on("error", reject);
    p.on("close", code => resolve({ code: code ?? 1, out, err }));
  });
}

function wordLocation(document, word) {
  const escaped = word.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const re = new RegExp("\\b(?:let|mut|const|fn|lit|class|struct|enum|trait|interface)\\s+" + escaped + "\\b");
  for (let i = 0; i < document.lineCount; i++) {
    const line = document.lineAt(i).text;
    const m = re.exec(line);
    if (m) return new vscode.Location(document.uri, new vscode.Position(i, m.index + m[0].lastIndexOf(word)));
  }
  return undefined;
}

function publishDiagnostics(collection, document, text) {
  const diagnostics = [];
  const re = /(?:line\s*)?(\d+)(?::|,)(\d+)?[^\n]*?(?:error|E\d{4})[^\n]*/gi;
  let m;
  while ((m = re.exec(text)) !== null) {
    const line = Math.max(0, Number(m[1]) - 1);
    const column = Math.max(0, Number(m[2] || 1) - 1);
    diagnostics.push(new vscode.Diagnostic(
      new vscode.Range(line, column, line, Math.max(column + 1, column + 1)),
      m[0].trim(),
      vscode.DiagnosticSeverity.Error
    ));
  }
  if (!diagnostics.length && text.trim()) {
    diagnostics.push(new vscode.Diagnostic(new vscode.Range(0, 0, 0, 1), text.trim(), vscode.DiagnosticSeverity.Error));
  }
  collection.set(document.uri, diagnostics);
}

async function runCommand(context, args, label) {
  const editor = vscode.window.activeTextEditor;
  if (!editor) return;
  const cwd = path.dirname(editor.document.fileName);
  try {
    const result = await tnc(args(editor.document.fileName), cwd);
    const out = vscode.window.createOutputChannel("Ternet");
    out.clear();
    out.append(result.out || result.err || `${label}: no output`);
    out.show(true);
    return result;
  } catch (error) {
    vscode.window.showErrorMessage(`Ternet: unable to start tnc: ${error.message}`);
    return { code: 1, out: "", err: error.message };
  }
}

function activate(context) {
  const diagnostics = vscode.languages.createDiagnosticCollection("ternet");
  context.subscriptions.push(diagnostics);

  const keywords = [
    "let","mut","const","fn","lit","if","elif","else","while","for","in",
    "return","break","continue","match","throw","try","catch","finally","import",
    "from","as","async","await","class","struct","enum","trait","impl","interface",
    "extends","implements","webfile","true","false","null"
  ];
  context.subscriptions.push(vscode.languages.registerCompletionItemProvider("ternet", {
    provideCompletionItems() {
      return keywords.map(x => new vscode.CompletionItem(x, vscode.CompletionItemKind.Keyword));
    }
  }));

  context.subscriptions.push(vscode.languages.registerDefinitionProvider("ternet", {
    provideDefinition(document, position) {
      const range = document.getWordRangeAtPosition(position);
      if (!range) return;
      return wordLocation(document, document.getText(range));
    }
  }));

  context.subscriptions.push(vscode.commands.registerCommand("ternet.runFile", async () => {
    await runCommand(context, file => ["run", file], "run");
  }));

  context.subscriptions.push(vscode.commands.registerCommand("ternet.checkFile", async () => {
    const editor = vscode.window.activeTextEditor;
    if (!editor) return;
    try {
      const result = await tnc(["check", editor.document.fileName], path.dirname(editor.document.fileName));
      diagnostics.clear();
      if (result.code !== 0) {
        publishDiagnostics(diagnostics, editor.document, result.err || result.out || "Ternet check failed");
      } else {
        vscode.window.showInformationMessage("Ternet: check passed.");
      }
    } catch (error) {
      diagnostics.set(editor.document.uri, [new vscode.Diagnostic(new vscode.Range(0,0,0,1), error.message, vscode.DiagnosticSeverity.Error)]);
    }
  }));

  context.subscriptions.push(vscode.commands.registerCommand("ternet.buildFile", async () => {
    const editor = vscode.window.activeTextEditor;
    if (!editor) return;
    const outFile = path.join(path.dirname(editor.document.fileName), path.basename(editor.document.fileName, ".trn") + ".tbc");
    const result = await tnc(["build", editor.document.fileName, "-o", outFile], path.dirname(editor.document.fileName));
    if (result.code === 0) vscode.window.showInformationMessage(`Ternet: built ${path.basename(outFile)}`);
    else vscode.window.showErrorMessage(result.err || "Ternet build failed");
  }));

  context.subscriptions.push(vscode.commands.registerCommand("ternet.formatDocument", async () => {
    const editor = vscode.window.activeTextEditor;
    if (!editor) return;
    const edit = new vscode.WorkspaceEdit();
    for (let i = 0; i < editor.document.lineCount; i++) {
      const line = editor.document.lineAt(i);
      const trimmed = line.text.trimEnd();
      if (trimmed !== line.text) edit.replace(editor.document.uri, line.range, trimmed);
    }
    await vscode.workspace.applyEdit(edit);
  }));
}

exports.activate = activate;
