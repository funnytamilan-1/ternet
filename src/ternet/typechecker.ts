import {
  AstStmt,
  AstExpr,
  AstType,
  StmtKind,
  ExprKind,
  Diagnostic,
} from './types';

export class TypeChecker {
  private diagnostics: Diagnostic[] = [];
  private scopes: Map<string, AstType>[] = [];
  private classes: Map<string, AstStmt> = new Map();
  private traits: Map<string, AstStmt> = new Map();
  private functions: Map<string, AstStmt> = new Map();

  check(ast: AstStmt[]): Diagnostic[] {
    this.diagnostics = [];
    this.scopes = [new Map()];
    this.classes.clear();
    this.traits.clear();
    this.functions.clear();

    // Built-in functions
    this.registerBuiltins();

    // First pass: collect declarations
    for (const stmt of ast) {
      if (stmt.kind === StmtKind.Class && stmt.name) {
        this.classes.set(stmt.name, stmt);
      } else if (stmt.kind === StmtKind.Trait && stmt.name) {
        this.traits.set(stmt.name, stmt);
      } else if (stmt.kind === StmtKind.Function && stmt.name) {
        this.functions.set(stmt.name, stmt);
      }
    }

    // Second pass: validate trait implementations on classes
    for (const [_, cls] of this.classes) {
      if (cls.implementsTraits) {
        for (const traitName of cls.implementsTraits) {
          const trait = this.traits.get(traitName);
          if (!trait) {
            this.error('T3001', `Trait '${traitName}' implemented by class '${cls.name}' is not defined`, cls.line, cls.column);
            continue;
          }
          for (const traitMethod of trait.methods || []) {
            const implemented = (cls.methods || []).find((m) => m.name === traitMethod.name);
            if (!implemented) {
              this.error('T3002', `Class '${cls.name}' does not implement required trait method '${traitMethod.name}' from trait '${traitName}'`, cls.line, cls.column, `Add 'fn ${traitMethod.name}()' to class '${cls.name}'`);
            }
          }
        }
      }
    }

    // Third pass: typecheck statements
    for (const stmt of ast) {
      this.checkStmt(stmt);
    }

    return this.diagnostics;
  }

  private registerBuiltins() {
    const scope = this.currentScope();
    const primitives = ['print', 'println', 'assert', 'time_now', 'fs_read', 'fs_write', 'env_get', 'json_stringify', 'json_parse', 'len', 'push', 'pop', 'some', 'none', 'ok', 'err', 'is_some', 'is_none', 'is_ok', 'is_err', 'unwrap', 'unwrap_or'];
    for (const p of primitives) {
      scope.set(p, { name: 'Function', generics: [], tupleTypes: [], isTuple: false });
    }
  }

  private checkStmt(stmt: AstStmt) {
    switch (stmt.kind) {
      case StmtKind.Let:
      case StmtKind.Mut:
      case StmtKind.Const: {
        let inferredType: AstType | undefined;
        if (stmt.expr) {
          inferredType = this.checkExpr(stmt.expr);
        }

        if (stmt.typeAnnotation && inferredType) {
          if (!this.isAssignable(stmt.typeAnnotation, inferredType)) {
            this.error(
              'T1001',
              `Type mismatch in variable '${stmt.name}': cannot assign '${this.formatType(inferredType)}' to '${this.formatType(stmt.typeAnnotation)}'`,
              stmt.line,
              stmt.column,
              `Change '${stmt.name}' type to '${this.formatType(inferredType)}' or adjust the assigned expression.`
            );
          }
        }

        const varType = stmt.typeAnnotation || inferredType || { name: 'any', generics: [], tupleTypes: [], isTuple: false };
        if (stmt.name) {
          this.currentScope().set(stmt.name, varType);
        }
        break;
      }

      case StmtKind.Assign: {
        if (stmt.target && stmt.expr) {
          const targetType = this.checkExpr(stmt.target);
          const valueType = this.checkExpr(stmt.expr);
          if (!this.isAssignable(targetType, valueType)) {
            this.error(
              'T1002',
              `Cannot assign type '${this.formatType(valueType)}' to target of type '${this.formatType(targetType)}'`,
              stmt.line,
              stmt.column
            );
          }
        }
        break;
      }

      case StmtKind.Function: {
        this.pushScope();
        if (stmt.params) {
          for (const p of stmt.params) {
            const pType = p.type || { name: 'any', generics: [], tupleTypes: [], isTuple: false };
            this.currentScope().set(p.name, pType);
          }
        }
        if (stmt.body) {
          for (const s of stmt.body) {
            this.checkStmt(s);
          }
        }
        this.popScope();
        break;
      }

      case StmtKind.Class: {
        this.pushScope();
        this.currentScope().set('this', { name: stmt.name || 'Object', generics: [], tupleTypes: [], isTuple: false });
        for (const f of stmt.fields || []) {
          this.currentScope().set(f.name, f.type || { name: 'any', generics: [], tupleTypes: [], isTuple: false });
        }
        for (const m of stmt.methods || []) {
          this.checkStmt(m);
        }
        this.popScope();
        break;
      }

      case StmtKind.If: {
        if (stmt.condition) {
          this.checkExpr(stmt.condition);
        }
        if (stmt.thenBranch) {
          this.pushScope();
          for (const s of stmt.thenBranch) this.checkStmt(s);
          this.popScope();
        }
        if (stmt.elseBranch) {
          this.pushScope();
          for (const s of stmt.elseBranch) this.checkStmt(s);
          this.popScope();
        }
        break;
      }

      case StmtKind.While: {
        if (stmt.condition) this.checkExpr(stmt.condition);
        if (stmt.body) {
          this.pushScope();
          for (const s of stmt.body) this.checkStmt(s);
          this.popScope();
        }
        break;
      }

      case StmtKind.For: {
        this.pushScope();
        if (stmt.iteratorName) {
          this.currentScope().set(stmt.iteratorName, { name: 'int', generics: [], tupleTypes: [], isTuple: false });
        }
        if (stmt.iterable) {
          this.checkExpr(stmt.iterable);
        }
        if (stmt.body) {
          for (const s of stmt.body) this.checkStmt(s);
        }
        this.popScope();
        break;
      }

      case StmtKind.TryCatch: {
        if (stmt.body) {
          this.pushScope();
          for (const s of stmt.body) this.checkStmt(s);
          this.popScope();
        }
        if (stmt.catchBody) {
          this.pushScope();
          if (stmt.catchVar) {
            this.currentScope().set(stmt.catchVar, { name: 'String', generics: [], tupleTypes: [], isTuple: false });
          }
          for (const s of stmt.catchBody) this.checkStmt(s);
          this.popScope();
        }
        if (stmt.finallyBody) {
          this.pushScope();
          for (const s of stmt.finallyBody) this.checkStmt(s);
          this.popScope();
        }
        break;
      }

      case StmtKind.Expr: {
        if (stmt.expr) {
          this.checkExpr(stmt.expr);
        }
        break;
      }

      case StmtKind.Return: {
        if (stmt.expr) {
          this.checkExpr(stmt.expr);
        }
        break;
      }

      default:
        break;
    }
  }

  private checkExpr(expr: AstExpr): AstType {
    switch (expr.kind) {
      case ExprKind.Number:
        return { name: Number.isInteger(expr.numValue || 0) ? 'int' : 'float', generics: [], tupleTypes: [], isTuple: false };

      case ExprKind.String:
        return { name: 'String', generics: [], tupleTypes: [], isTuple: false };

      case ExprKind.Bool:
        return { name: 'bool', generics: [], tupleTypes: [], isTuple: false };

      case ExprKind.Null:
        return { name: 'null', generics: [], tupleTypes: [], isTuple: false };

      case ExprKind.Identifier: {
        const found = this.lookup(expr.name || '');
        if (!found) {
          if (this.classes.has(expr.name || '')) {
            return { name: 'ClassConstructor', generics: [], tupleTypes: [], isTuple: false };
          }
          this.error('T2001', `Undefined variable '${expr.name}'`, expr.line, expr.column);
          return { name: 'any', generics: [], tupleTypes: [], isTuple: false };
        }
        return found;
      }

      case ExprKind.Array: {
        if (!expr.elements || expr.elements.length === 0) {
          return { name: 'List', generics: [{ name: 'any', generics: [], tupleTypes: [], isTuple: false }], tupleTypes: [], isTuple: false };
        }
        const elemType = this.checkExpr(expr.elements[0]);
        return { name: 'List', generics: [elemType], tupleTypes: [], isTuple: false };
      }

      case ExprKind.Tuple: {
        const tupleTypes = (expr.elements || []).map((e) => this.checkExpr(e));
        return { name: 'Tuple', generics: [], tupleTypes, isTuple: true };
      }

      case ExprKind.This: {
        const thisType = this.lookup('this');
        if (!thisType) {
          this.error('T4001', `'this' is only valid inside class methods`, expr.line, expr.column);
          return { name: 'any', generics: [], tupleTypes: [], isTuple: false };
        }
        return thisType;
      }

      case ExprKind.Binary: {
        const leftType = expr.left ? this.checkExpr(expr.left) : { name: 'any', generics: [], tupleTypes: [], isTuple: false };
        const rightType = expr.right ? this.checkExpr(expr.right) : { name: 'any', generics: [], tupleTypes: [], isTuple: false };

        const isComparison = ['==', '!=', '<', '<=', '>', '>=', '&&', '||'].includes(expr.op || '');
        if (isComparison) {
          return { name: 'bool', generics: [], tupleTypes: [], isTuple: false };
        }

        if (leftType.name === 'String' || rightType.name === 'String') {
          return { name: 'String', generics: [], tupleTypes: [], isTuple: false };
        }
        if (leftType.name === 'float' || rightType.name === 'float') {
          return { name: 'float', generics: [], tupleTypes: [], isTuple: false };
        }
        return leftType;
      }

      case ExprKind.Call: {
        if (expr.left?.kind === ExprKind.Identifier) {
          const callee = expr.left.name || '';
          if (this.classes.has(callee)) {
            return { name: callee, generics: [], tupleTypes: [], isTuple: false };
          }
        }
        if (expr.left) {
          this.checkExpr(expr.left);
        }
        if (expr.args) {
          for (const a of expr.args) this.checkExpr(a);
        }
        return { name: 'any', generics: [], tupleTypes: [], isTuple: false };
      }

      case ExprKind.Member: {
        const objType = expr.object ? this.checkExpr(expr.object) : { name: 'any', generics: [], tupleTypes: [], isTuple: false };
        if (expr.property === 'append' || expr.property === 'pop' || expr.property === 'len' || expr.property === 'insert' || expr.property === 'remove' || expr.property === 'clear') {
          return { name: 'Function', generics: [], tupleTypes: [], isTuple: false };
        }
        return { name: 'any', generics: [], tupleTypes: [], isTuple: false };
      }

      case ExprKind.Index: {
        const objType = expr.object ? this.checkExpr(expr.object) : { name: 'any', generics: [], tupleTypes: [], isTuple: false };
        if (expr.index) this.checkExpr(expr.index);
        if (objType.isTuple && objType.tupleTypes.length > 0 && expr.index?.kind === ExprKind.Number) {
          const idx = expr.index.numValue || 0;
          if (idx >= 0 && idx < objType.tupleTypes.length) {
            return objType.tupleTypes[idx];
          }
        }
        if (objType.name === 'List' && objType.generics.length > 0) {
          return objType.generics[0];
        }
        return { name: 'any', generics: [], tupleTypes: [], isTuple: false };
      }

      case ExprKind.Ternary: {
        if (expr.condition) this.checkExpr(expr.condition);
        const thenType = expr.thenBranch ? this.checkExpr(expr.thenBranch) : { name: 'any', generics: [], tupleTypes: [], isTuple: false };
        if (expr.elseBranch) this.checkExpr(expr.elseBranch);
        return thenType;
      }

      default:
        return { name: 'any', generics: [], tupleTypes: [], isTuple: false };
    }
  }

  private isAssignable(target: AstType, source: AstType): boolean {
    if (target.name === 'any' || source.name === 'any') return true;
    if (target.name === source.name) {
      if (target.generics.length > 0 && source.generics.length > 0) {
        return this.isAssignable(target.generics[0], source.generics[0]);
      }
      return true;
    }
    // Integer subtyping (int, int32, int64, uint)
    const intTypes = ['int', 'int8', 'int16', 'int32', 'int64', 'uint', 'uint8', 'uint16', 'uint32', 'uint64', 'byte', 'char'];
    if (intTypes.includes(target.name) && intTypes.includes(source.name)) return true;

    // Float subtyping
    const floatTypes = ['float', 'float32', 'float64'];
    if (floatTypes.includes(target.name) && (floatTypes.includes(source.name) || intTypes.includes(source.name))) return true;

    // String / str alias
    if ((target.name === 'String' || target.name === 'str') && (source.name === 'String' || source.name === 'str')) return true;

    // Class inheritance
    if (this.classes.has(source.name)) {
      const cls = this.classes.get(source.name);
      if (cls?.baseClass === target.name) return true;
      if (cls?.implementsTraits?.includes(target.name)) return true;
    }

    return false;
  }

  private formatType(t: AstType): string {
    if (t.isTuple) {
      return `(${t.tupleTypes.map((sub) => this.formatType(sub)).join(', ')})`;
    }
    if (t.generics && t.generics.length > 0) {
      return `${t.name}<${t.generics.map((g) => this.formatType(g)).join(', ')}>`;
    }
    return t.name;
  }

  private pushScope() {
    this.scopes.push(new Map());
  }

  private popScope() {
    if (this.scopes.length > 1) this.scopes.pop();
  }

  private currentScope(): Map<string, AstType> {
    return this.scopes[this.scopes.length - 1];
  }

  private lookup(name: string): AstType | undefined {
    for (let i = this.scopes.length - 1; i >= 0; i--) {
      if (this.scopes[i].has(name)) {
        return this.scopes[i].get(name);
      }
    }
    return undefined;
  }

  private error(code: string, message: string, line: number, column: number, suggestion?: string) {
    this.diagnostics.push({
      code,
      message,
      severity: 'error',
      line,
      column,
      suggestion,
    });
  }
}
