import { AstStmt, Diagnostic, StmtKind } from './types';

export class Linter {
  lint(ast: AstStmt[], source: string): Diagnostic[] {
    const diagnostics: Diagnostic[] = [];

    // Check 1: Trailing whitespace / long lines
    const lines = source.split(/\r?\n/);
    lines.forEach((line, idx) => {
      if (line.endsWith(' ') || line.endsWith('\t')) {
        diagnostics.push({
          code: 'L1001',
          message: 'Trailing whitespace detected',
          severity: 'info',
          line: idx + 1,
          column: line.length,
          suggestion: 'Run formatter or trim trailing whitespace',
        });
      }
      if (line.length > 120) {
        diagnostics.push({
          code: 'L1002',
          message: 'Line exceeds 120 characters',
          severity: 'info',
          line: idx + 1,
          column: 120,
          suggestion: 'Split line across multiple lines',
        });
      }
    });

    // Check 2: Naming conventions
    const checkNames = (stmts: AstStmt[]) => {
      for (const stmt of stmts) {
        if (stmt.kind === StmtKind.Class || stmt.kind === StmtKind.Trait || stmt.kind === StmtKind.Struct || stmt.kind === StmtKind.Enum) {
          if (stmt.name && !/^[A-Z][A-Za-z0-9]*$/.test(stmt.name)) {
            diagnostics.push({
              code: 'L2001',
              message: `Type name '${stmt.name}' should follow PascalCase convention`,
              severity: 'warning',
              line: stmt.line,
              column: stmt.column,
            });
          }
        }

        if (stmt.kind === StmtKind.Function) {
          if (stmt.name && !/^[a-z_][a-z0-9_]*$/.test(stmt.name)) {
            diagnostics.push({
              code: 'L2002',
              message: `Function name '${stmt.name}' should follow snake_case convention`,
              severity: 'warning',
              line: stmt.line,
              column: stmt.column,
            });
          }
        }

        if (stmt.body) checkNames(stmt.body);
        if (stmt.thenBranch) checkNames(stmt.thenBranch);
        if (stmt.elseBranch) checkNames(stmt.elseBranch);
        if (stmt.methods) checkNames(stmt.methods);
      }
    };

    checkNames(ast);

    return diagnostics;
  }
}
