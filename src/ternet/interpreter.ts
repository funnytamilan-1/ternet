import {
  AstStmt,
  AstExpr,
  StmtKind,
  ExprKind,
  TernetValue,
} from './types';

export class RuntimeError extends Error {
  line: number;
  column: number;
  constructor(message: string, line: number = 1, column: number = 1) {
    super(message);
    this.line = line;
    this.column = column;
  }
}

export class Interpreter {
  private outputLogs: string[] = [];
  private scopes: Map<string, TernetValue>[] = [];
  private classes: Map<string, AstStmt> = new Map();
  private traits: Map<string, AstStmt> = new Map();
  private functions: Map<string, AstStmt> = new Map();

  getLogs(): string[] {
    return this.outputLogs;
  }

  run(ast: AstStmt[]): { output: string; logs: string[] } {
    this.outputLogs = [];
    this.scopes = [new Map()];
    this.classes.clear();
    this.traits.clear();
    this.functions.clear();

    this.registerBuiltins();

    // Pass 1: register classes, traits, functions
    for (const stmt of ast) {
      if (stmt.kind === StmtKind.Class && stmt.name) {
        this.classes.set(stmt.name, stmt);
      } else if (stmt.kind === StmtKind.Trait && stmt.name) {
        this.traits.set(stmt.name, stmt);
      } else if (stmt.kind === StmtKind.Function && stmt.name) {
        this.functions.set(stmt.name, stmt);
      }
    }

    // Pass 2: execute statements
    for (const stmt of ast) {
      if (stmt.kind === StmtKind.Class || stmt.kind === StmtKind.Trait) continue;
      this.execStmt(stmt);
    }

    // If main() function exists and wasn't explicitly called, invoke it
    if (this.functions.has('main')) {
      const mainFn = this.functions.get('main')!;
      this.callFunction(mainFn, []);
    }

    return {
      output: this.outputLogs.join('\n'),
      logs: this.outputLogs,
    };
  }

  private registerBuiltins() {
    const scope = this.currentScope();

    scope.set('print', {
      type: 'function',
      fnName: 'print',
      isNative: true,
      nativeFn: (...args: TernetValue[]) => {
        const text = args.map((a) => this.stringify(a)).join(' ');
        this.outputLogs.push(text);
        return { type: 'null' };
      },
    });

    scope.set('println', {
      type: 'function',
      fnName: 'println',
      isNative: true,
      nativeFn: (...args: TernetValue[]) => {
        const text = args.map((a) => this.stringify(a)).join(' ');
        this.outputLogs.push(text);
        return { type: 'null' };
      },
    });

    scope.set('assert', {
      type: 'function',
      fnName: 'assert',
      isNative: true,
      nativeFn: (cond?: TernetValue, msg?: TernetValue) => {
        if (!cond || !this.isTruthy(cond)) {
          const m = msg ? this.stringify(msg) : 'Assertion failed';
          throw new RuntimeError(`AssertionError: ${m}`);
        }
        return { type: 'bool', boolVal: true };
      },
    });

    scope.set('some', {
      type: 'function',
      fnName: 'some',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'option', isSome: true, innerVal: val }),
    });

    scope.set('none', {
      type: 'function',
      fnName: 'none',
      isNative: true,
      nativeFn: () => ({ type: 'option', isSome: false }),
    });

    scope.set('ok', {
      type: 'function',
      fnName: 'ok',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'result', isOk: true, innerVal: val }),
    });

    scope.set('err', {
      type: 'function',
      fnName: 'err',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'result', isOk: false, innerVal: val }),
    });

    scope.set('is_some', {
      type: 'function',
      fnName: 'is_some',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'bool', boolVal: val?.type === 'option' && !!val.isSome }),
    });

    scope.set('is_none', {
      type: 'function',
      fnName: 'is_none',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'bool', boolVal: val?.type === 'option' && !val.isSome }),
    });

    scope.set('is_ok', {
      type: 'function',
      fnName: 'is_ok',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'bool', boolVal: val?.type === 'result' && !!val.isOk }),
    });

    scope.set('is_err', {
      type: 'function',
      fnName: 'is_err',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'bool', boolVal: val?.type === 'result' && !val.isOk }),
    });

    scope.set('unwrap', {
      type: 'function',
      fnName: 'unwrap',
      isNative: true,
      nativeFn: (val?: TernetValue) => {
        if (!val) return { type: 'null' };
        if (val.type === 'option') {
          if (val.isSome && val.innerVal) return val.innerVal;
          throw new RuntimeError('Called unwrap() on none Option');
        }
        if (val.type === 'result') {
          if (val.isOk && val.innerVal) return val.innerVal;
          throw new RuntimeError(`Called unwrap() on Err Result: ${this.stringify(val.innerVal || { type: 'null' })}`);
        }
        return val;
      },
    });

    scope.set('unwrap_or', {
      type: 'function',
      fnName: 'unwrap_or',
      isNative: true,
      nativeFn: (val?: TernetValue, def?: TernetValue) => {
        const fallback = def || { type: 'null' };
        if (!val) return fallback;
        if (val.type === 'option') {
          return val.isSome && val.innerVal ? val.innerVal : fallback;
        }
        if (val.type === 'result') {
          return val.isOk && val.innerVal ? val.innerVal : fallback;
        }
        return val;
      },
    });

    scope.set('time_now', {
      type: 'function',
      fnName: 'time_now',
      isNative: true,
      nativeFn: () => ({ type: 'int', numVal: Math.floor(Date.now() / 1000) }),
    });

    scope.set('json_stringify', {
      type: 'function',
      fnName: 'json_stringify',
      isNative: true,
      nativeFn: (val?: TernetValue) => ({ type: 'string', strVal: JSON.stringify(this.toJsValue(val || { type: 'null' })) }),
    });

    scope.set('json_parse', {
      type: 'function',
      fnName: 'json_parse',
      isNative: true,
      nativeFn: (str?: TernetValue) => {
        try {
          const parsed = JSON.parse(str?.strVal || '{}');
          return this.fromJsValue(parsed);
        } catch {
          return { type: 'null' };
        }
      },
    });
  }

  private execStmt(stmt: AstStmt): TernetValue | undefined {
    switch (stmt.kind) {
      case StmtKind.Let:
      case StmtKind.Mut:
      case StmtKind.Const: {
        const val = stmt.expr ? this.evalExpr(stmt.expr) : { type: 'null' as const };
        if (stmt.name) {
          this.currentScope().set(stmt.name, val);
        }
        return undefined;
      }

      case StmtKind.Assign: {
        if (!stmt.target || !stmt.expr) return undefined;
        const value = this.evalExpr(stmt.expr);

        if (stmt.target.kind === ExprKind.Identifier) {
          const name = stmt.target.name || '';
          this.assignVar(name, value);
        } else if (stmt.target.kind === ExprKind.Member) {
          const obj = stmt.target.object ? this.evalExpr(stmt.target.object) : undefined;
          if (obj && obj.type === 'object' && obj.fields && stmt.target.property) {
            obj.fields.set(stmt.target.property, value);
          }
        } else if (stmt.target.kind === ExprKind.Index) {
          const obj = stmt.target.object ? this.evalExpr(stmt.target.object) : undefined;
          const idxVal = stmt.target.index ? this.evalExpr(stmt.target.index) : undefined;
          if (obj && obj.type === 'array' && obj.arrVal && idxVal && (idxVal.type === 'int' || idxVal.type === 'float') && idxVal.numVal !== undefined) {
            const idx = Math.floor(idxVal.numVal);
            if (idx >= 0 && idx < obj.arrVal.length) {
              obj.arrVal[idx] = value;
            }
          }
        }
        return undefined;
      }

      case StmtKind.Function: {
        if (stmt.name) {
          this.functions.set(stmt.name, stmt);
        }
        return undefined;
      }

      case StmtKind.Return: {
        return stmt.expr ? this.evalExpr(stmt.expr) : { type: 'null' as const };
      }

      case StmtKind.If: {
        if (stmt.condition && this.isTruthy(this.evalExpr(stmt.condition))) {
          if (stmt.thenBranch) {
            for (const s of stmt.thenBranch) {
              const res = this.execStmt(s);
              if (s.kind === StmtKind.Return) return res;
            }
          }
        } else if (stmt.elseBranch) {
          for (const s of stmt.elseBranch) {
            const res = this.execStmt(s);
            if (s.kind === StmtKind.Return) return res;
          }
        }
        return undefined;
      }

      case StmtKind.While: {
        while (stmt.condition && this.isTruthy(this.evalExpr(stmt.condition))) {
          if (stmt.body) {
            let shouldBreak = false;
            for (const s of stmt.body) {
              if (s.kind === StmtKind.Break) {
                shouldBreak = true;
                break;
              }
              if (s.kind === StmtKind.Continue) break;
              const res = this.execStmt(s);
              if (s.kind === StmtKind.Return) return res;
            }
            if (shouldBreak) break;
          }
        }
        return undefined;
      }

      case StmtKind.For: {
        if (stmt.iterable && stmt.iteratorName) {
          const iterVal = this.evalExpr(stmt.iterable);
          let items: TernetValue[] = [];
          if (iterVal.type === 'array' && iterVal.arrVal) {
            items = iterVal.arrVal;
          } else if ((iterVal.type === 'int' || iterVal.type === 'float') && iterVal.numVal !== undefined) {
            for (let i = 0; i < iterVal.numVal; i++) {
              items.push({ type: 'int', numVal: i });
            }
          }

          this.pushScope();
          for (const item of items) {
            this.currentScope().set(stmt.iteratorName, item);
            let shouldBreak = false;
            for (const s of stmt.body || []) {
              if (s.kind === StmtKind.Break) {
                shouldBreak = true;
                break;
              }
              if (s.kind === StmtKind.Continue) break;
              const res = this.execStmt(s);
              if (s.kind === StmtKind.Return) {
                this.popScope();
                return res;
              }
            }
            if (shouldBreak) break;
          }
          this.popScope();
        }
        return undefined;
      }

      case StmtKind.TryCatch: {
        let threw = false;
        let thrownError: any;
        try {
          if (stmt.body) {
            for (const s of stmt.body) {
              const res = this.execStmt(s);
              if (s.kind === StmtKind.Return) return res;
            }
          }
        } catch (err: any) {
          threw = true;
          thrownError = err;
        }

        if (threw && stmt.catchBody) {
          this.pushScope();
          if (stmt.catchVar) {
            this.currentScope().set(stmt.catchVar, {
              type: 'string',
              strVal: thrownError instanceof Error ? thrownError.message : String(thrownError),
            });
          }
          for (const s of stmt.catchBody) {
            const res = this.execStmt(s);
            if (s.kind === StmtKind.Return) {
              this.popScope();
              return res;
            }
          }
          this.popScope();
        }

        if (stmt.finallyBody) {
          for (const s of stmt.finallyBody) {
            const res = this.execStmt(s);
            if (s.kind === StmtKind.Return) return res;
          }
        }
        return undefined;
      }

      case StmtKind.Throw: {
        const val = stmt.expr ? this.evalExpr(stmt.expr) : { type: 'string' as const, strVal: 'Error thrown' };
        throw new RuntimeError(this.stringify(val), stmt.line, stmt.column);
      }

      case StmtKind.Expr: {
        if (stmt.expr) this.evalExpr(stmt.expr);
        return undefined;
      }

      default:
        return undefined;
    }
  }

  private evalExpr(expr: AstExpr): TernetValue {
    switch (expr.kind) {
      case ExprKind.Number: {
        const val = expr.numValue || 0;
        return { type: Number.isInteger(val) ? 'int' : 'float', numVal: val };
      }

      case ExprKind.String:
        return { type: 'string', strVal: expr.strValue || '' };

      case ExprKind.Bool:
        return { type: 'bool', boolVal: !!expr.boolValue };

      case ExprKind.Null:
        return { type: 'null' };

      case ExprKind.Identifier: {
        const name = expr.name || '';
        const found = this.lookupVar(name);
        if (found) return found;

        if (this.classes.has(name)) {
          return {
            type: 'function',
            fnName: name,
            isNative: true,
            nativeFn: (...args: TernetValue[]) => this.instantiateClass(name, args),
          };
        }

        if (this.functions.has(name)) {
          const fn = this.functions.get(name)!;
          return {
            type: 'function',
            fnName: name,
            params: fn.params || [],
            body: fn.body || [],
          };
        }

        throw new RuntimeError(`Undefined variable or identifier: '${name}'`, expr.line, expr.column);
      }

      case ExprKind.This: {
        const thisVal = this.lookupVar('this');
        if (!thisVal) throw new RuntimeError(`'this' used outside of class method context`, expr.line, expr.column);
        return thisVal;
      }

      case ExprKind.Array: {
        const elements = (expr.elements || []).map((e) => this.evalExpr(e));
        return { type: 'array', arrVal: elements };
      }

      case ExprKind.Tuple: {
        const elements = (expr.elements || []).map((e) => this.evalExpr(e));
        return { type: 'tuple', arrVal: elements };
      }

      case ExprKind.Unary: {
        const val = expr.right ? this.evalExpr(expr.right) : { type: 'null' as const };
        if (expr.op === '-') {
          if (val.type === 'int' && val.numVal !== undefined) return { type: 'int', numVal: -val.numVal };
          if (val.type === 'float' && val.numVal !== undefined) return { type: 'float', numVal: -val.numVal };
        }
        if (expr.op === '!') {
          return { type: 'bool', boolVal: !this.isTruthy(val) };
        }
        return val;
      }

      case ExprKind.Binary: {
        const left = expr.left ? this.evalExpr(expr.left) : { type: 'null' as const };

        // Short circuit logical ops
        if (expr.op === '&&') {
          if (!this.isTruthy(left)) return { type: 'bool', boolVal: false };
          const right = expr.right ? this.evalExpr(expr.right) : { type: 'null' as const };
          return { type: 'bool', boolVal: this.isTruthy(right) };
        }
        if (expr.op === '||') {
          if (this.isTruthy(left)) return { type: 'bool', boolVal: true };
          const right = expr.right ? this.evalExpr(expr.right) : { type: 'null' as const };
          return { type: 'bool', boolVal: this.isTruthy(right) };
        }

        const right = expr.right ? this.evalExpr(expr.right) : { type: 'null' as const };

        // String concatenation
        if (expr.op === '+' && (left.type === 'string' || right.type === 'string')) {
          return { type: 'string', strVal: this.stringify(left) + this.stringify(right) };
        }

        // Numeric ops
        if ((left.type === 'int' || left.type === 'float') && (right.type === 'int' || right.type === 'float')) {
          const isFloat = left.type === 'float' || right.type === 'float';
          const lv = left.numVal || 0;
          const rv = right.numVal || 0;
          switch (expr.op) {
            case '+': return { type: isFloat ? 'float' : 'int', numVal: lv + rv };
            case '-': return { type: isFloat ? 'float' : 'int', numVal: lv - rv };
            case '*': return { type: isFloat ? 'float' : 'int', numVal: lv * rv };
            case '/': return { type: 'float', numVal: lv / (rv || 1) };
            case '%': return { type: 'int', numVal: lv % (rv || 1) };
            case '<': return { type: 'bool', boolVal: lv < rv };
            case '<=': return { type: 'bool', boolVal: lv <= rv };
            case '>': return { type: 'bool', boolVal: lv > rv };
            case '>=': return { type: 'bool', boolVal: lv >= rv };
          }
        }

        if (expr.op === '==') {
          return { type: 'bool', boolVal: this.isEqual(left, right) };
        }
        if (expr.op === '!=') {
          return { type: 'bool', boolVal: !this.isEqual(left, right) };
        }

        return { type: 'null' };
      }

      case ExprKind.Call: {
        const callee = expr.left ? this.evalExpr(expr.left) : undefined;
        const args = (expr.args || []).map((a) => this.evalExpr(a));

        if (callee && callee.type === 'function') {
          if (callee.isNative && callee.nativeFn) {
            return callee.nativeFn(...args);
          }
          return this.callFunction({ kind: StmtKind.Function, name: callee.fnName, params: callee.params, body: callee.body, line: expr.line, column: expr.column }, args);
        }

        throw new RuntimeError(`Expression is not callable: ${this.stringify(callee || { type: 'null' })}`, expr.line, expr.column);
      }

      case ExprKind.Member: {
        const obj = expr.object ? this.evalExpr(expr.object) : undefined;
        const prop = expr.property || '';

        if (obj && obj.type === 'object' && obj.fields && obj.className) {
          if (obj.fields.has(prop)) {
            return obj.fields.get(prop)!;
          }
          const method = this.findClassMethod(obj.className, prop);
          if (method) {
            return {
              type: 'function',
              fnName: prop,
              params: method.params || [],
              body: method.body || [],
              isNative: true,
              nativeFn: (...args: TernetValue[]) => this.callMethod(obj, method, args),
            };
          }
        }

        // List built-in methods
        if (obj && obj.type === 'array' && obj.arrVal) {
          if (prop === 'append' || prop === 'push') {
            return {
              type: 'function',
              fnName: prop,
              isNative: true,
              nativeFn: (val?: TernetValue) => {
                if (val) obj.arrVal?.push(val);
                return { type: 'null' };
              },
            };
          }
          if (prop === 'pop') {
            return {
              type: 'function',
              fnName: 'pop',
              isNative: true,
              nativeFn: () => obj.arrVal?.pop() || { type: 'null' },
            };
          }
          if (prop === 'len') {
            return {
              type: 'function',
              fnName: 'len',
              isNative: true,
              nativeFn: () => ({ type: 'int', numVal: obj.arrVal?.length || 0 }),
            };
          }
          if (prop === 'insert') {
            return {
              type: 'function',
              fnName: 'insert',
              isNative: true,
              nativeFn: (idxVal?: TernetValue, val?: TernetValue) => {
                const idx = idxVal && (idxVal.type === 'int' || idxVal.type === 'float') && idxVal.numVal !== undefined ? Math.floor(idxVal.numVal) : 0;
                if (val && obj.arrVal) obj.arrVal.splice(idx, 0, val);
                return { type: 'null' };
              },
            };
          }
          if (prop === 'remove') {
            return {
              type: 'function',
              fnName: 'remove',
              isNative: true,
              nativeFn: (idxVal?: TernetValue) => {
                const idx = idxVal && (idxVal.type === 'int' || idxVal.type === 'float') && idxVal.numVal !== undefined ? Math.floor(idxVal.numVal) : 0;
                if (obj.arrVal && idx >= 0 && idx < obj.arrVal.length) {
                  return obj.arrVal.splice(idx, 1)[0];
                }
                return { type: 'null' };
              },
            };
          }
          if (prop === 'clear') {
            return {
              type: 'function',
              fnName: 'clear',
              isNative: true,
              nativeFn: () => {
                obj.arrVal = [];
                return { type: 'null' };
              },
            };
          }
        }

        if (obj && obj.type === 'string') {
          if (prop === 'len') {
            return {
              type: 'function',
              fnName: 'len',
              isNative: true,
              nativeFn: () => ({ type: 'int', numVal: obj.strVal?.length || 0 }),
            };
          }
        }

        throw new RuntimeError(`Member '${prop}' not found on ${this.stringify(obj || { type: 'null' })}`, expr.line, expr.column);
      }

      case ExprKind.Index: {
        const obj = expr.object ? this.evalExpr(expr.object) : undefined;
        const idx = expr.index ? this.evalExpr(expr.index) : undefined;

        if (obj && (obj.type === 'array' || obj.type === 'tuple') && obj.arrVal && idx && (idx.type === 'int' || idx.type === 'float') && idx.numVal !== undefined) {
          const indexNum = Math.floor(idx.numVal);
          if (indexNum < 0 || indexNum >= obj.arrVal.length) {
            throw new RuntimeError(`Index out of bounds: index ${indexNum}, length ${obj.arrVal.length}`, expr.line, expr.column);
          }
          return obj.arrVal[indexNum];
        }

        if (obj && obj.type === 'string' && obj.strVal !== undefined && idx && (idx.type === 'int' || idx.type === 'float') && idx.numVal !== undefined) {
          const indexNum = Math.floor(idx.numVal);
          return { type: 'string', strVal: obj.strVal[indexNum] || '' };
        }

        throw new RuntimeError(`Cannot index ${obj?.type}`, expr.line, expr.column);
      }

      case ExprKind.Ternary: {
        const cond = expr.condition ? this.evalExpr(expr.condition) : { type: 'bool' as const, boolVal: false };
        if (this.isTruthy(cond)) {
          return expr.thenBranch ? this.evalExpr(expr.thenBranch) : { type: 'null' as const };
        } else {
          return expr.elseBranch ? this.evalExpr(expr.elseBranch) : { type: 'null' as const };
        }
      }

      default:
        return { type: 'null' };
    }
  }

  private instantiateClass(className: string, args: TernetValue[]): TernetValue {
    const cls = this.classes.get(className);
    if (!cls) throw new RuntimeError(`Class '${className}' not found`);

    const fields = new Map<string, TernetValue>();

    let currentBase = cls.baseClass;
    const inheritanceChain: AstStmt[] = [cls];
    while (currentBase) {
      const parent = this.classes.get(currentBase);
      if (parent) {
        inheritanceChain.unshift(parent);
        currentBase = parent.baseClass;
      } else {
        break;
      }
    }

    for (const c of inheritanceChain) {
      for (const f of c.fields || []) {
        fields.set(f.name, { type: 'null' });
      }
    }

    const instance: TernetValue = {
      type: 'object',
      className,
      fields,
    };

    const initMethod = this.findClassMethod(className, 'init');
    if (initMethod) {
      this.callMethod(instance, initMethod, args);
    }

    return instance;
  }

  private findClassMethod(className: string, methodName: string): AstStmt | undefined {
    let curClass: string | undefined = className;
    while (curClass) {
      const cls = this.classes.get(curClass);
      if (cls) {
        const found = (cls.methods || []).find((m) => m.name === methodName);
        if (found) return found;
        curClass = cls.baseClass;
      } else {
        break;
      }
    }
    return undefined;
  }

  private callMethod(instance: TernetValue, method: AstStmt, args: TernetValue[]): TernetValue {
    this.pushScope();
    this.currentScope().set('this', instance);

    if (instance.type === 'object' && instance.fields) {
      for (const [k, v] of instance.fields) {
        this.currentScope().set(k, v);
      }
    }

    if (method.params) {
      for (let i = 0; i < method.params.length; i++) {
        const p = method.params[i];
        const val = i < args.length ? args[i] : { type: 'null' as const };
        this.currentScope().set(p.name, val);
      }
    }

    let result: TernetValue = { type: 'null' };
    for (const stmt of method.body || []) {
      const res = this.execStmt(stmt);
      if (stmt.kind === StmtKind.Return) {
        result = res || { type: 'null' };
        break;
      }
    }

    if (instance.type === 'object' && instance.fields) {
      for (const [k] of instance.fields) {
        const updated = this.currentScope().get(k);
        if (updated) instance.fields.set(k, updated);
      }
    }

    this.popScope();
    return result;
  }

  private callFunction(fn: AstStmt, args: TernetValue[]): TernetValue {
    this.pushScope();
    if (fn.params) {
      for (let i = 0; i < fn.params.length; i++) {
        const p = fn.params[i];
        const val = i < args.length ? args[i] : { type: 'null' as const };
        this.currentScope().set(p.name, val);
      }
    }

    let result: TernetValue = { type: 'null' };
    for (const stmt of fn.body || []) {
      const res = this.execStmt(stmt);
      if (stmt.kind === StmtKind.Return) {
        result = res || { type: 'null' };
        break;
      }
    }

    this.popScope();
    return result;
  }

  private isTruthy(v: TernetValue): boolean {
    if (v.type === 'null') return false;
    if (v.type === 'bool') return !!v.boolVal;
    if (v.type === 'int' || v.type === 'float') return v.numVal !== 0 && v.numVal !== undefined;
    if (v.type === 'string') return (v.strVal?.length || 0) > 0;
    if (v.type === 'array' || v.type === 'tuple') return (v.arrVal?.length || 0) > 0;
    if (v.type === 'option') return !!v.isSome;
    if (v.type === 'result') return !!v.isOk;
    return true;
  }

  private isEqual(a: TernetValue, b: TernetValue): boolean {
    if (a.type !== b.type) return false;
    if (a.type === 'null') return true;
    if (a.type === 'bool') return a.boolVal === b.boolVal;
    if (a.type === 'int' || a.type === 'float') {
      return a.numVal === b.numVal;
    }
    if (a.type === 'string') return a.strVal === b.strVal;
    return false;
  }

  stringify(v: TernetValue): string {
    if (v.type === 'null') return 'null';
    if (v.type === 'bool') return v.boolVal ? 'true' : 'false';
    if (v.type === 'int' || v.type === 'float') return String(v.numVal ?? 0);
    if (v.type === 'string') return v.strVal || '';
    if (v.type === 'array' && v.arrVal) {
      return `[${v.arrVal.map((x) => this.stringify(x)).join(', ')}]`;
    }
    if (v.type === 'tuple' && v.arrVal) {
      return `(${v.arrVal.map((x) => this.stringify(x)).join(', ')})`;
    }
    if (v.type === 'object' && v.fields) {
      const fieldPairs: string[] = [];
      for (const [k, val] of v.fields) {
        fieldPairs.push(`${k}: ${this.stringify(val)}`);
      }
      return `${v.className || 'Object'} { ${fieldPairs.join(', ')} }`;
    }
    if (v.type === 'option') {
      return v.isSome && v.innerVal ? `some(${this.stringify(v.innerVal)})` : 'none';
    }
    if (v.type === 'result') {
      return v.isOk && v.innerVal ? `ok(${this.stringify(v.innerVal)})` : `err(${this.stringify(v.innerVal || { type: 'null' })})`;
    }
    if (v.type === 'function') {
      return `<fn ${v.fnName || 'anonymous'}>`;
    }
    return String(v.type);
  }

  private toJsValue(v: TernetValue): any {
    if (v.type === 'null') return null;
    if (v.type === 'bool') return v.boolVal;
    if (v.type === 'int' || v.type === 'float') return v.numVal;
    if (v.type === 'string') return v.strVal;
    if ((v.type === 'array' || v.type === 'tuple') && v.arrVal) return v.arrVal.map((x) => this.toJsValue(x));
    if (v.type === 'object' && v.fields) {
      const obj: any = {};
      for (const [k, val] of v.fields) obj[k] = this.toJsValue(val);
      return obj;
    }
    return null;
  }

  private fromJsValue(js: any): TernetValue {
    if (js === null || js === undefined) return { type: 'null' };
    if (typeof js === 'boolean') return { type: 'bool', boolVal: js };
    if (typeof js === 'number') return { type: Number.isInteger(js) ? 'int' : 'float', numVal: js };
    if (typeof js === 'string') return { type: 'string', strVal: js };
    if (Array.isArray(js)) return { type: 'array', arrVal: js.map((x) => this.fromJsValue(x)) };
    if (typeof js === 'object') {
      const fields = new Map<string, TernetValue>();
      for (const k of Object.keys(js)) {
        fields.set(k, this.fromJsValue(js[k]));
      }
      return { type: 'object', className: 'Object', fields };
    }
    return { type: 'null' };
  }

  private pushScope() {
    this.scopes.push(new Map());
  }

  private popScope() {
    if (this.scopes.length > 1) this.scopes.pop();
  }

  private currentScope(): Map<string, TernetValue> {
    return this.scopes[this.scopes.length - 1];
  }

  private lookupVar(name: string): TernetValue | undefined {
    for (let i = this.scopes.length - 1; i >= 0; i--) {
      if (this.scopes[i].has(name)) {
        return this.scopes[i].get(name);
      }
    }
    return undefined;
  }

  private assignVar(name: string, val: TernetValue) {
    for (let i = this.scopes.length - 1; i >= 0; i--) {
      if (this.scopes[i].has(name)) {
        this.scopes[i].set(name, val);
        return;
      }
    }
    this.currentScope().set(name, val);
  }
}
