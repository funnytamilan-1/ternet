import {
  AstStmt,
  AstExpr,
  StmtKind,
  ExprKind,
  OpCode,
  Instruction,
  BytecodeProgram,
  TernetValue,
} from './types';

export class BytecodeCompiler {
  private instructions: Instruction[] = [];
  private constants: TernetValue[] = [];
  private names: string[] = [];

  compile(ast: AstStmt[]): BytecodeProgram {
    this.instructions = [];
    this.constants = [];
    this.names = [];

    for (const stmt of ast) {
      this.compileStmt(stmt);
    }

    this.emit(OpCode.Halt, 0);

    return {
      instructions: this.instructions,
      constants: this.constants,
      names: this.names,
    };
  }

  private compileStmt(stmt: AstStmt) {
    switch (stmt.kind) {
      case StmtKind.Let:
      case StmtKind.Mut:
      case StmtKind.Const: {
        if (stmt.expr) {
          this.compileExpr(stmt.expr);
        } else {
          const cIdx = this.addConstant({ type: 'null' });
          this.emit(OpCode.Const, cIdx);
        }
        const nameIdx = this.addName(stmt.name || '');
        this.emit(OpCode.Store, nameIdx);
        break;
      }

      case StmtKind.Assign: {
        if (stmt.target && stmt.expr) {
          if (stmt.target.kind === ExprKind.Identifier) {
            this.compileExpr(stmt.expr);
            const nameIdx = this.addName(stmt.target.name || '');
            this.emit(OpCode.Store, nameIdx);
          } else if (stmt.target.kind === ExprKind.Index) {
            if (stmt.target.object && stmt.target.index) {
              this.compileExpr(stmt.target.object);
              this.compileExpr(stmt.target.index);
              this.compileExpr(stmt.expr);
              this.emit(OpCode.IndexSet, 0);
            }
          }
        }
        break;
      }

      case StmtKind.If: {
        if (stmt.condition) {
          this.compileExpr(stmt.condition);
          const jumpFalseIdx = this.instructions.length;
          this.emit(OpCode.JumpIfFalse, 0);

          if (stmt.thenBranch) {
            for (const s of stmt.thenBranch) this.compileStmt(s);
          }

          const jumpEndIdx = this.instructions.length;
          this.emit(OpCode.Jump, 0);

          // Patch jumpFalse
          this.instructions[jumpFalseIdx].arg = this.instructions.length;

          if (stmt.elseBranch) {
            for (const s of stmt.elseBranch) this.compileStmt(s);
          }

          // Patch jumpEnd
          this.instructions[jumpEndIdx].arg = this.instructions.length;
        }
        break;
      }

      case StmtKind.While: {
        if (stmt.condition) {
          const loopStart = this.instructions.length;
          this.compileExpr(stmt.condition);

          const jumpFalseIdx = this.instructions.length;
          this.emit(OpCode.JumpIfFalse, 0);

          if (stmt.body) {
            for (const s of stmt.body) this.compileStmt(s);
          }

          this.emit(OpCode.Jump, loopStart);
          this.instructions[jumpFalseIdx].arg = this.instructions.length;
        }
        break;
      }

      case StmtKind.Expr: {
        if (stmt.expr) {
          this.compileExpr(stmt.expr);
          this.emit(OpCode.Pop, 0);
        }
        break;
      }

      case StmtKind.Return: {
        if (stmt.expr) {
          this.compileExpr(stmt.expr);
        } else {
          const cIdx = this.addConstant({ type: 'null' });
          this.emit(OpCode.Const, cIdx);
        }
        this.emit(OpCode.Return, 0);
        break;
      }

      default:
        break;
    }
  }

  private compileExpr(expr: AstExpr) {
    switch (expr.kind) {
      case ExprKind.Number: {
        const val = expr.numValue || 0;
        const cIdx = this.addConstant({ type: Number.isInteger(val) ? 'int' : 'float', numVal: val });
        this.emit(OpCode.Const, cIdx);
        break;
      }

      case ExprKind.String: {
        const cIdx = this.addConstant({ type: 'string', strVal: expr.strValue || '' });
        this.emit(OpCode.Const, cIdx);
        break;
      }

      case ExprKind.Bool: {
        const cIdx = this.addConstant({ type: 'bool', boolVal: !!expr.boolValue });
        this.emit(OpCode.Const, cIdx);
        break;
      }

      case ExprKind.Null: {
        const cIdx = this.addConstant({ type: 'null' });
        this.emit(OpCode.Const, cIdx);
        break;
      }

      case ExprKind.Identifier: {
        const nameIdx = this.addName(expr.name || '');
        this.emit(OpCode.Load, nameIdx);
        break;
      }

      case ExprKind.Array: {
        const count = expr.elements ? expr.elements.length : 0;
        for (const elem of expr.elements || []) {
          this.compileExpr(elem);
        }
        this.emit(OpCode.MakeArray, count);
        break;
      }

      case ExprKind.Tuple: {
        const count = expr.elements ? expr.elements.length : 0;
        for (const elem of expr.elements || []) {
          this.compileExpr(elem);
        }
        this.emit(OpCode.MakeTuple, count);
        break;
      }

      case ExprKind.Binary: {
        if (expr.left) this.compileExpr(expr.left);
        if (expr.right) this.compileExpr(expr.right);

        switch (expr.op) {
          case '+': this.emit(OpCode.Add, 0); break;
          case '-': this.emit(OpCode.Sub, 0); break;
          case '*': this.emit(OpCode.Mul, 0); break;
          case '/': this.emit(OpCode.Div, 0); break;
          case '%': this.emit(OpCode.Mod, 0); break;
          case '==': this.emit(OpCode.Eq, 0); break;
          case '!=': this.emit(OpCode.Neq, 0); break;
          case '<': this.emit(OpCode.Lt, 0); break;
          case '<=': this.emit(OpCode.Lte, 0); break;
          case '>': this.emit(OpCode.Gt, 0); break;
          case '>=': this.emit(OpCode.Gte, 0); break;
          case '&&': this.emit(OpCode.And, 0); break;
          case '||': this.emit(OpCode.Or, 0); break;
        }
        break;
      }

      case ExprKind.Unary: {
        if (expr.right) this.compileExpr(expr.right);
        if (expr.op === '-') this.emit(OpCode.Neg, 0);
        if (expr.op === '!') this.emit(OpCode.Not, 0);
        break;
      }

      case ExprKind.Call: {
        if (expr.left?.kind === ExprKind.Identifier && (expr.left.name === 'print' || expr.left.name === 'println')) {
          for (const arg of expr.args || []) {
            this.compileExpr(arg);
          }
          this.emit(OpCode.Print, expr.args ? expr.args.length : 0);
          const cIdx = this.addConstant({ type: 'null' });
          this.emit(OpCode.Const, cIdx);
          break;
        }

        if (expr.left) this.compileExpr(expr.left);
        for (const arg of expr.args || []) {
          this.compileExpr(arg);
        }
        this.emit(OpCode.Call, expr.args ? expr.args.length : 0);
        break;
      }

      case ExprKind.Index: {
        if (expr.object) this.compileExpr(expr.object);
        if (expr.index) this.compileExpr(expr.index);
        this.emit(OpCode.IndexGet, 0);
        break;
      }

      default:
        break;
    }
  }

  private emit(op: OpCode, arg: number) {
    this.instructions.push({ op, arg });
  }

  private addConstant(val: TernetValue): number {
    this.constants.push(val);
    return this.constants.length - 1;
  }

  private addName(name: string): number {
    const idx = this.names.indexOf(name);
    if (idx !== -1) return idx;
    this.names.push(name);
    return this.names.length - 1;
  }
}

export class BytecodeVM {
  private stack: TernetValue[] = [];
  private globals: Map<string, TernetValue> = new Map();
  private outputLogs: string[] = [];

  run(prog: BytecodeProgram): { output: string; logs: string[] } {
    this.stack = [];
    this.globals = new Map();
    this.outputLogs = [];

    let ip = 0;
    while (ip < prog.instructions.length) {
      const instr = prog.instructions[ip++];
      switch (instr.op) {
        case OpCode.Halt:
          return { output: this.outputLogs.join('\n'), logs: this.outputLogs };

        case OpCode.Const:
          this.stack.push(prog.constants[instr.arg]);
          break;

        case OpCode.Load: {
          const name = prog.names[instr.arg];
          const val = this.globals.get(name) || { type: 'null' };
          this.stack.push(val);
          break;
        }

        case OpCode.Store: {
          const name = prog.names[instr.arg];
          const val = this.stack.pop() || { type: 'null' };
          this.globals.set(name, val);
          break;
        }

        case OpCode.Pop:
          this.stack.pop();
          break;

        case OpCode.Add: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          if (a.type === 'string' || b.type === 'string') {
            this.stack.push({ type: 'string', strVal: this.stringifyVal(a) + this.stringifyVal(b) });
          } else {
            const isFloat = a.type === 'float' || b.type === 'float';
            const av = a.numVal || 0;
            const bv = b.numVal || 0;
            this.stack.push({ type: isFloat ? 'float' : 'int', numVal: av + bv });
          }
          break;
        }

        case OpCode.Sub: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          const isFloat = a.type === 'float' || b.type === 'float';
          const av = a.numVal || 0;
          const bv = b.numVal || 0;
          this.stack.push({ type: isFloat ? 'float' : 'int', numVal: av - bv });
          break;
        }

        case OpCode.Mul: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          const isFloat = a.type === 'float' || b.type === 'float';
          const av = a.numVal || 0;
          const bv = b.numVal || 0;
          this.stack.push({ type: isFloat ? 'float' : 'int', numVal: av * bv });
          break;
        }

        case OpCode.Div: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          const av = a.numVal || 0;
          const bv = b.numVal || 1;
          this.stack.push({ type: 'float', numVal: av / bv });
          break;
        }

        case OpCode.Eq: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          this.stack.push({ type: 'bool', boolVal: this.isEqual(a, b) });
          break;
        }

        case OpCode.Lt: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          const av = a.numVal || 0;
          const bv = b.numVal || 0;
          this.stack.push({ type: 'bool', boolVal: av < bv });
          break;
        }

        case OpCode.Lte: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          const av = a.numVal || 0;
          const bv = b.numVal || 0;
          this.stack.push({ type: 'bool', boolVal: av <= bv });
          break;
        }

        case OpCode.Gt: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          const av = a.numVal || 0;
          const bv = b.numVal || 0;
          this.stack.push({ type: 'bool', boolVal: av > bv });
          break;
        }

        case OpCode.Gte: {
          const b = this.stack.pop() || { type: 'null' };
          const a = this.stack.pop() || { type: 'null' };
          const av = a.numVal || 0;
          const bv = b.numVal || 0;
          this.stack.push({ type: 'bool', boolVal: av >= bv });
          break;
        }

        case OpCode.Jump:
          ip = instr.arg;
          break;

        case OpCode.JumpIfFalse: {
          const cond = this.stack.pop();
          const truthy = cond?.type === 'bool' ? !!cond.boolVal : Boolean(cond?.numVal || cond?.strVal);
          if (!truthy) {
            ip = instr.arg;
          }
          break;
        }

        case OpCode.MakeArray: {
          const elements: TernetValue[] = [];
          for (let i = 0; i < instr.arg; i++) {
            elements.unshift(this.stack.pop() || { type: 'null' });
          }
          this.stack.push({ type: 'array', arrVal: elements });
          break;
        }

        case OpCode.MakeTuple: {
          const elements: TernetValue[] = [];
          for (let i = 0; i < instr.arg; i++) {
            elements.unshift(this.stack.pop() || { type: 'null' });
          }
          this.stack.push({ type: 'tuple', arrVal: elements });
          break;
        }

        case OpCode.IndexGet: {
          const idxVal = this.stack.pop() || { type: 'null' };
          const target = this.stack.pop() || { type: 'null' };
          if ((target.type === 'array' || target.type === 'tuple') && target.arrVal && (idxVal.type === 'int' || idxVal.type === 'float')) {
            const idx = Math.floor(idxVal.numVal || 0);
            this.stack.push(target.arrVal[idx] || { type: 'null' });
          } else {
            this.stack.push({ type: 'null' });
          }
          break;
        }

        case OpCode.Print: {
          const args: TernetValue[] = [];
          for (let i = 0; i < instr.arg; i++) {
            args.unshift(this.stack.pop() || { type: 'null' });
          }
          const text = args.map((a) => this.stringifyVal(a)).join(' ');
          this.outputLogs.push(text);
          break;
        }

        default:
          break;
      }
    }

    return { output: this.outputLogs.join('\n'), logs: this.outputLogs };
  }

  private isEqual(a: TernetValue, b: TernetValue): boolean {
    if (a.type !== b.type) return false;
    if (a.type === 'null') return true;
    if (a.type === 'bool') return a.boolVal === b.boolVal;
    if (a.type === 'int' || a.type === 'float') return a.numVal === b.numVal;
    if (a.type === 'string') return a.strVal === b.strVal;
    return false;
  }

  private stringifyVal(v: TernetValue): string {
    if (v.type === 'null') return 'null';
    if (v.type === 'bool') return v.boolVal ? 'true' : 'false';
    if (v.type === 'int' || v.type === 'float') return String(v.numVal ?? 0);
    if (v.type === 'string') return v.strVal || '';
    if (v.type === 'array' && v.arrVal) return `[${v.arrVal.map((x) => this.stringifyVal(x)).join(', ')}]`;
    if (v.type === 'tuple' && v.arrVal) return `(${v.arrVal.map((x) => this.stringifyVal(x)).join(', ')})`;
    return String(v.type);
  }

  disassemble(prog: BytecodeProgram): string {
    const lines: string[] = [];
    lines.push('=== DISASSEMBLY (TVM Bytecode) ===');
    lines.push(`Constants: [${prog.constants.map((c, i) => `${i}:${this.stringifyVal(c)}`).join(', ')}]`);
    lines.push(`Names: [${prog.names.map((n, i) => `${i}:${n}`).join(', ')}]`);
    lines.push('----------------------------------');

    for (let i = 0; i < prog.instructions.length; i++) {
      const instr = prog.instructions[i];
      const opName = OpCode[instr.op].padEnd(14, ' ');
      let detail = '';
      if (instr.op === OpCode.Const) {
        detail = `(val=${this.stringifyVal(prog.constants[instr.arg] || { type: 'null' })})`;
      } else if (instr.op === OpCode.Load || instr.op === OpCode.Store) {
        detail = `(name='${prog.names[instr.arg]}')`;
      } else if (instr.op === OpCode.Jump || instr.op === OpCode.JumpIfFalse || instr.op === OpCode.JumpIfTrue) {
        detail = `(target=${instr.arg})`;
      }
      lines.push(`${String(i).padStart(4, '0')}:  ${opName}  ${String(instr.arg).padEnd(4, ' ')} ${detail}`);
    }
    return lines.join('\n');
  }
}
