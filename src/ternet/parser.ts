import {
  Token,
  TokenType,
  AstStmt,
  AstExpr,
  AstType,
  StmtKind,
  ExprKind,
  Parameter,
} from './types';

export class Parser {
  private tokens: Token[];
  private pos: number = 0;

  constructor(tokens: Token[]) {
    this.tokens = tokens;
  }

  parse(): AstStmt[] {
    const stmts: AstStmt[] = [];
    while (!this.isAtEnd()) {
      const stmt = this.statement();
      if (stmt) stmts.push(stmt);
    }
    return stmts;
  }

  private statement(): AstStmt | null {
    if (this.isAtEnd()) return null;
    const tok = this.peek();

    if (this.match(TokenType.KeywordLet) || this.match(TokenType.KeywordMut) || this.match(TokenType.KeywordConst)) {
      const kind = tok.type === TokenType.KeywordLet ? StmtKind.Let : tok.type === TokenType.KeywordMut ? StmtKind.Mut : StmtKind.Const;
      const nameTok = this.consume(TokenType.Identifier, 'Expected variable name');
      let typeAnnotation: AstType | undefined;
      if (this.match(TokenType.Colon)) {
        typeAnnotation = this.parseType();
      }
      let expr: AstExpr | undefined;
      if (this.match(TokenType.Op) && this.previous().text === '=') {
        expr = this.expression();
      }
      this.eatDelimiters();
      return {
        kind,
        name: nameTok.text,
        typeAnnotation,
        expr,
        line: tok.line,
        column: tok.column,
      };
    }

    if (this.match(TokenType.KeywordFn) || this.match(TokenType.KeywordLit)) {
      return this.functionDeclaration(tok);
    }

    if (this.match(TokenType.KeywordClass)) {
      return this.classDeclaration(tok);
    }

    if (this.match(TokenType.KeywordTrait) || this.match(TokenType.KeywordInterface)) {
      return this.traitDeclaration(tok);
    }

    if (this.match(TokenType.KeywordStruct)) {
      return this.structDeclaration(tok);
    }

    if (this.match(TokenType.KeywordEnum)) {
      return this.enumDeclaration(tok);
    }

    if (this.match(TokenType.KeywordIf)) {
      return this.ifStatement(tok);
    }

    if (this.match(TokenType.KeywordWhile)) {
      return this.whileStatement(tok);
    }

    if (this.match(TokenType.KeywordFor)) {
      return this.forStatement(tok);
    }

    if (this.match(TokenType.KeywordReturn)) {
      let expr: AstExpr | undefined;
      if (!this.check(TokenType.Semicolon) && !this.check(TokenType.Colon) && !this.check(TokenType.RBrace) && !this.isAtEnd()) {
        expr = this.expression();
      }
      this.eatDelimiters();
      return { kind: StmtKind.Return, expr, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordBreak)) {
      this.eatDelimiters();
      return { kind: StmtKind.Break, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordContinue)) {
      this.eatDelimiters();
      return { kind: StmtKind.Continue, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordTry)) {
      return this.tryCatchStatement(tok);
    }

    if (this.match(TokenType.KeywordThrow)) {
      const expr = this.expression();
      this.eatDelimiters();
      return { kind: StmtKind.Throw, expr, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordImport)) {
      const pathTok = this.consume(TokenType.String, 'Expected import path');
      this.eatDelimiters();
      return { kind: StmtKind.Import, importPath: pathTok.text, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.LBrace)) {
      const body = this.block();
      return { kind: StmtKind.Block, body, line: tok.line, column: tok.column };
    }

    // Assignment or Expression Statement
    const expr = this.expression();
    if (this.match(TokenType.Op) && this.previous().text === '=') {
      const value = this.expression();
      this.eatDelimiters();
      return { kind: StmtKind.Assign, target: expr, expr: value, line: tok.line, column: tok.column };
    }

    this.eatDelimiters();
    return { kind: StmtKind.Expr, expr, line: tok.line, column: tok.column };
  }

  private functionDeclaration(tok: Token, isVirtual = false, isOverride = false, isStatic = false): AstStmt {
    let name = '';
    if (this.check(TokenType.Identifier)) {
      name = this.advance().text;
    }

    const typeParams: string[] = [];
    if (this.match(TokenType.Op) && this.previous().text === '<') {
      while (!this.isAtEnd() && !(this.check(TokenType.Op) && this.peek().text === '>')) {
        const p = this.consume(TokenType.Identifier, 'Expected generic parameter');
        typeParams.push(p.text);
        if (this.match(TokenType.Comma)) continue;
        else break;
      }
      this.consumeOp('>', "Expected '>'");
    }

    this.consume(TokenType.LParen, "Expected '(' after function name");
    const params: Parameter[] = [];
    if (!this.check(TokenType.RParen)) {
      do {
        const pName = this.consume(TokenType.Identifier, 'Expected parameter name').text;
        let pType: AstType | undefined;
        if (this.match(TokenType.Colon)) {
          pType = this.parseType();
        }
        params.push({ name: pName, type: pType });
      } while (this.match(TokenType.Comma));
    }
    this.consume(TokenType.RParen, "Expected ')' after parameters");

    let returnType: AstType | undefined;
    if (this.match(TokenType.Arrow) || this.match(TokenType.Colon)) {
      returnType = this.parseType();
    }

    if (this.match(TokenType.Semicolon)) {
      return {
        kind: StmtKind.Function,
        name,
        params,
        returnType,
        typeParams,
        isVirtual,
        isOverride,
        isStatic,
        isAbstract: true,
        body: [],
        line: tok.line,
        column: tok.column,
      };
    }

    this.consume(TokenType.LBrace, "Expected '{' before function body");
    const body = this.block();
    this.eatDelimiters();

    return {
      kind: StmtKind.Function,
      name,
      params,
      returnType,
      typeParams,
      isVirtual,
      isOverride,
      isStatic,
      body,
      line: tok.line,
      column: tok.column,
    };
  }

  private classDeclaration(tok: Token): AstStmt {
    const name = this.consume(TokenType.Identifier, 'Expected class name').text;
    let baseClass: string | undefined;
    const implementsTraits: string[] = [];

    if (this.match(TokenType.KeywordExtends)) {
      baseClass = this.consume(TokenType.Identifier, 'Expected base class name').text;
    }

    if (this.match(TokenType.KeywordImplements)) {
      do {
        implementsTraits.push(this.consume(TokenType.Identifier, 'Expected trait name').text);
      } while (this.match(TokenType.Comma));
    }

    this.consume(TokenType.LBrace, "Expected '{' before class body");
    const fields: { name: string; type?: AstType; isStatic?: boolean }[] = [];
    const methods: AstStmt[] = [];

    while (!this.check(TokenType.RBrace) && !this.isAtEnd()) {
      let isVirtual = false;
      let isOverride = false;
      let isStatic = false;

      if (this.match(TokenType.KeywordVirtual)) isVirtual = true;
      if (this.match(TokenType.KeywordOverride)) isOverride = true;
      if (this.match(TokenType.KeywordStatic)) isStatic = true;

      if (this.match(TokenType.KeywordFn) || this.match(TokenType.KeywordLit)) {
        methods.push(this.functionDeclaration(this.previous(), isVirtual, isOverride, isStatic));
      } else if (this.match(TokenType.KeywordLet) || this.match(TokenType.KeywordMut) || this.match(TokenType.KeywordConst)) {
        const fName = this.consume(TokenType.Identifier, 'Expected field name').text;
        let fType: AstType | undefined;
        if (this.match(TokenType.Colon)) {
          fType = this.parseType();
        }
        this.eatDelimiters();
        fields.push({ name: fName, type: fType, isStatic });
      } else if (this.check(TokenType.Identifier) && this.peek().text === 'init') {
        const initTok = this.advance();
        methods.push(this.functionDeclaration(initTok, false, false, false));
      } else {
        this.advance();
      }
    }

    this.consume(TokenType.RBrace, "Expected '}' after class body");
    this.eatDelimiters();

    return {
      kind: StmtKind.Class,
      name,
      baseClass,
      implementsTraits,
      fields,
      methods,
      line: tok.line,
      column: tok.column,
    };
  }

  private traitDeclaration(tok: Token): AstStmt {
    const name = this.consume(TokenType.Identifier, 'Expected trait/interface name').text;
    this.consume(TokenType.LBrace, "Expected '{' before trait body");
    const methods: AstStmt[] = [];
    while (!this.check(TokenType.RBrace) && !this.isAtEnd()) {
      if (this.match(TokenType.KeywordFn)) {
        methods.push(this.functionDeclaration(this.previous(), true, false, false));
      } else {
        this.advance();
      }
    }
    this.consume(TokenType.RBrace, "Expected '}' after trait body");
    this.eatDelimiters();

    return {
      kind: StmtKind.Trait,
      name,
      methods,
      line: tok.line,
      column: tok.column,
    };
  }

  private structDeclaration(tok: Token): AstStmt {
    const name = this.consume(TokenType.Identifier, 'Expected struct name').text;
    this.consume(TokenType.LBrace, "Expected '{' before struct body");
    const fields: { name: string; type?: AstType }[] = [];
    while (!this.check(TokenType.RBrace) && !this.isAtEnd()) {
      const fName = this.consume(TokenType.Identifier, 'Expected field name').text;
      let fType: AstType | undefined;
      if (this.match(TokenType.Colon)) {
        fType = this.parseType();
      }
      this.eatDelimiters();
      fields.push({ name: fName, type: fType });
    }
    this.consume(TokenType.RBrace, "Expected '}' after struct body");
    this.eatDelimiters();

    return {
      kind: StmtKind.Struct,
      name,
      fields,
      line: tok.line,
      column: tok.column,
    };
  }

  private enumDeclaration(tok: Token): AstStmt {
    const name = this.consume(TokenType.Identifier, 'Expected enum name').text;
    this.consume(TokenType.LBrace, "Expected '{' before enum body");
    const variants: { name: string; value?: number }[] = [];
    let curVal = 0;
    while (!this.check(TokenType.RBrace) && !this.isAtEnd()) {
      const vName = this.consume(TokenType.Identifier, 'Expected variant name').text;
      let val = curVal;
      if (this.match(TokenType.Op) && this.previous().text === '=') {
        const nTok = this.consume(TokenType.Number, 'Expected integer value');
        val = parseInt(nTok.text, 10);
      }
      variants.push({ name: vName, value: val });
      curVal = val + 1;
      this.match(TokenType.Comma);
    }
    this.consume(TokenType.RBrace, "Expected '}' after enum body");
    this.eatDelimiters();

    return {
      kind: StmtKind.Enum,
      name,
      variants,
      line: tok.line,
      column: tok.column,
    };
  }

  private ifStatement(tok: Token): AstStmt {
    const condition = this.expression();
    if (this.match(TokenType.Semicolon)) {
      // syntax: if cond; { ... }
    }
    this.consume(TokenType.LBrace, "Expected '{' after if condition");
    const thenBranch = this.block();

    let elseBranch: AstStmt[] | undefined;
    if (this.match(TokenType.KeywordElif)) {
      const elifStmt = this.ifStatement(this.previous());
      elseBranch = [elifStmt];
    } else if (this.match(TokenType.KeywordElse)) {
      if (this.match(TokenType.KeywordIf)) {
        const elifStmt = this.ifStatement(this.previous());
        elseBranch = [elifStmt];
      } else {
        this.consume(TokenType.LBrace, "Expected '{' after else");
        elseBranch = this.block();
      }
    }

    this.eatDelimiters();
    return {
      kind: StmtKind.If,
      condition,
      thenBranch,
      elseBranch,
      line: tok.line,
      column: tok.column,
    };
  }

  private whileStatement(tok: Token): AstStmt {
    const condition = this.expression();
    if (this.match(TokenType.Semicolon)) {
      // optional
    }
    this.consume(TokenType.LBrace, "Expected '{' after while condition");
    const body = this.block();
    this.eatDelimiters();

    return {
      kind: StmtKind.While,
      condition,
      body,
      line: tok.line,
      column: tok.column,
    };
  }

  private forStatement(tok: Token): AstStmt {
    const iterName = this.consume(TokenType.Identifier, 'Expected iterator variable name').text;
    this.consume(TokenType.KeywordIn, "Expected 'in' after loop variable");
    const iterable = this.expression();
    this.consume(TokenType.LBrace, "Expected '{' after for header");
    const body = this.block();
    this.eatDelimiters();

    return {
      kind: StmtKind.For,
      iteratorName: iterName,
      iterable,
      body,
      line: tok.line,
      column: tok.column,
    };
  }

  private tryCatchStatement(tok: Token): AstStmt {
    this.consume(TokenType.LBrace, "Expected '{' after try");
    const body = this.block();
    let catchVar: string | undefined;
    let catchBody: AstStmt[] | undefined;

    if (this.match(TokenType.KeywordCatch)) {
      if (this.match(TokenType.LParen)) {
        catchVar = this.consume(TokenType.Identifier, 'Expected catch error variable').text;
        this.consume(TokenType.RParen, "Expected ')' after catch variable");
      }
      this.consume(TokenType.LBrace, "Expected '{' after catch");
      catchBody = this.block();
    }

    let finallyBody: AstStmt[] | undefined;
    if (this.match(TokenType.KeywordFinally)) {
      this.consume(TokenType.LBrace, "Expected '{' after finally");
      finallyBody = this.block();
    }

    this.eatDelimiters();
    return {
      kind: StmtKind.TryCatch,
      body,
      catchVar,
      catchBody,
      finallyBody,
      line: tok.line,
      column: tok.column,
    };
  }

  private block(): AstStmt[] {
    const stmts: AstStmt[] = [];
    while (!this.check(TokenType.RBrace) && !this.isAtEnd()) {
      const s = this.statement();
      if (s) stmts.push(s);
    }
    this.consume(TokenType.RBrace, "Expected '}' to close block");
    return stmts;
  }

  private expression(): AstExpr {
    return this.ternary();
  }

  private ternary(): AstExpr {
    let expr = this.logicOr();
    if (this.match(TokenType.Op) && this.previous().text === '?') {
      const thenBranch = this.expression();
      this.consume(TokenType.Colon, "Expected ':' in ternary expression");
      const elseBranch = this.expression();
      expr = {
        kind: ExprKind.Ternary,
        condition: expr,
        thenBranch,
        elseBranch,
        line: expr.line,
        column: expr.column,
      };
    }
    return expr;
  }

  private logicOr(): AstExpr {
    let expr = this.logicAnd();
    while (this.matchOp('||')) {
      const right = this.logicAnd();
      expr = { kind: ExprKind.Binary, op: '||', left: expr, right, line: expr.line, column: expr.column };
    }
    return expr;
  }

  private logicAnd(): AstExpr {
    let expr = this.equality();
    while (this.matchOp('&&')) {
      const right = this.equality();
      expr = { kind: ExprKind.Binary, op: '&&', left: expr, right, line: expr.line, column: expr.column };
    }
    return expr;
  }

  private equality(): AstExpr {
    let expr = this.comparison();
    while (this.matchOp('==') || this.matchOp('!=')) {
      const op = this.previous().text;
      const right = this.comparison();
      expr = { kind: ExprKind.Binary, op, left: expr, right, line: expr.line, column: expr.column };
    }
    return expr;
  }

  private comparison(): AstExpr {
    let expr = this.term();
    while (this.matchOp('<') || this.matchOp('<=') || this.matchOp('>') || this.matchOp('>=')) {
      const op = this.previous().text;
      const right = this.term();
      expr = { kind: ExprKind.Binary, op, left: expr, right, line: expr.line, column: expr.column };
    }
    return expr;
  }

  private term(): AstExpr {
    let expr = this.factor();
    while (this.matchOp('+') || this.matchOp('-')) {
      const op = this.previous().text;
      const right = this.factor();
      expr = { kind: ExprKind.Binary, op, left: expr, right, line: expr.line, column: expr.column };
    }
    return expr;
  }

  private factor(): AstExpr {
    let expr = this.unary();
    while (this.matchOp('*') || this.matchOp('/') || this.matchOp('%')) {
      const op = this.previous().text;
      const right = this.unary();
      expr = { kind: ExprKind.Binary, op, left: expr, right, line: expr.line, column: expr.column };
    }
    return expr;
  }

  private unary(): AstExpr {
    if (this.matchOp('!') || this.matchOp('-')) {
      const op = this.previous().text;
      const right = this.unary();
      return { kind: ExprKind.Unary, op, right, line: this.previous().line, column: this.previous().column };
    }
    return this.postfix();
  }

  private postfix(): AstExpr {
    let expr = this.primary();

    while (true) {
      if (this.match(TokenType.LParen)) {
        const args: AstExpr[] = [];
        if (!this.check(TokenType.RParen)) {
          do {
            args.push(this.expression());
          } while (this.match(TokenType.Comma));
        }
        this.consume(TokenType.RParen, "Expected ')' after arguments");
        expr = { kind: ExprKind.Call, left: expr, args, line: expr.line, column: expr.column };
      } else if (this.match(TokenType.Dot)) {
        if (this.match(TokenType.Number)) {
          // Tuple index e.g. .0 or .1
          const idx = parseInt(this.previous().text, 10);
          expr = {
            kind: ExprKind.Index,
            object: expr,
            index: { kind: ExprKind.Number, numValue: idx, line: expr.line, column: expr.column },
            line: expr.line,
            column: expr.column,
          };
        } else {
          const prop = this.consume(TokenType.Identifier, 'Expected property name after .').text;
          expr = { kind: ExprKind.Member, object: expr, property: prop, line: expr.line, column: expr.column };
        }
      } else if (this.match(TokenType.LBracket)) {
        const idx = this.expression();
        this.consume(TokenType.RBracket, "Expected ']' after index");
        expr = { kind: ExprKind.Index, object: expr, index: idx, line: expr.line, column: expr.column };
      } else {
        break;
      }
    }

    return expr;
  }

  private primary(): AstExpr {
    const tok = this.peek();

    if (this.match(TokenType.Number)) {
      const val = parseFloat(tok.text);
      return { kind: ExprKind.Number, numValue: val, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.String)) {
      return { kind: ExprKind.String, strValue: tok.text, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordTrue)) {
      return { kind: ExprKind.Bool, boolValue: true, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordFalse)) {
      return { kind: ExprKind.Bool, boolValue: false, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordNull)) {
      return { kind: ExprKind.Null, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordThis)) {
      return { kind: ExprKind.This, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.KeywordSuper)) {
      return { kind: ExprKind.Super, line: tok.line, column: tok.column };
    }

    if (this.match(TokenType.Identifier)) {
      return { kind: ExprKind.Identifier, name: tok.text, line: tok.line, column: tok.column };
    }

    // List literal [1, 2, 3]
    if (this.match(TokenType.LBracket)) {
      const elements: AstExpr[] = [];
      if (!this.check(TokenType.RBracket)) {
        do {
          elements.push(this.expression());
        } while (this.match(TokenType.Comma));
      }
      this.consume(TokenType.RBracket, "Expected ']' after array elements");
      return { kind: ExprKind.Array, elements, line: tok.line, column: tok.column };
    }

    // Parenthesized or Tuple literal (a, b)
    if (this.match(TokenType.LParen)) {
      if (this.match(TokenType.RParen)) {
        return { kind: ExprKind.Tuple, elements: [], line: tok.line, column: tok.column };
      }
      const first = this.expression();
      if (this.match(TokenType.Comma)) {
        const elements = [first];
        if (!this.check(TokenType.RParen)) {
          do {
            elements.push(this.expression());
          } while (this.match(TokenType.Comma));
        }
        this.consume(TokenType.RParen, "Expected ')' after tuple elements");
        return { kind: ExprKind.Tuple, elements, line: tok.line, column: tok.column };
      }
      this.consume(TokenType.RParen, "Expected ')' after expression");
      return first;
    }

    // Advance to avoid infinite loop
    this.advance();
    return { kind: ExprKind.Null, line: tok.line, column: tok.column };
  }

  private parseType(): AstType {
    if (this.match(TokenType.LParen)) {
      const tupleTypes: AstType[] = [];
      if (!this.check(TokenType.RParen)) {
        do {
          tupleTypes.push(this.parseType());
        } while (this.match(TokenType.Comma));
      }
      this.consume(TokenType.RParen, "Expected ')' after tuple type");
      return { name: 'Tuple', generics: [], tupleTypes, isTuple: true };
    }

    const name = this.consume(TokenType.Identifier, 'Expected type name').text;
    const generics: AstType[] = [];

    if (this.matchOp('<')) {
      do {
        generics.push(this.parseType());
      } while (this.match(TokenType.Comma));
      this.consumeOp('>', "Expected '>' after generic type arguments");
    }

    return { name, generics, tupleTypes: [], isTuple: false };
  }

  private match(...types: TokenType[]): boolean {
    for (const t of types) {
      if (this.check(t)) {
        this.advance();
        return true;
      }
    }
    return false;
  }

  private matchOp(opStr: string): boolean {
    if (this.check(TokenType.Op) && this.peek().text === opStr) {
      this.advance();
      return true;
    }
    return false;
  }

  private consumeOp(opStr: string, msg: string): Token {
    if (this.check(TokenType.Op) && this.peek().text === opStr) {
      return this.advance();
    }
    throw new Error(`${msg} at line ${this.peek().line}, col ${this.peek().column}`);
  }

  private check(type: TokenType): boolean {
    if (this.isAtEnd()) return type === TokenType.Eof;
    return this.peek().type === type;
  }

  private advance(): Token {
    if (!this.isAtEnd()) this.pos++;
    return this.previous();
  }

  private isAtEnd(): boolean {
    return this.pos >= this.tokens.length || this.tokens[this.pos].type === TokenType.Eof;
  }

  private peek(): Token {
    return this.tokens[this.pos] || { type: TokenType.Eof, text: '', line: 1, column: 1 };
  }

  private previous(): Token {
    return this.tokens[this.pos - 1];
  }

  private consume(type: TokenType, message: string): Token {
    if (this.check(type)) return this.advance();
    throw new Error(`${message} at line ${this.peek().line}, col ${this.peek().column}`);
  }

  private eatDelimiters() {
    while (this.match(TokenType.Semicolon) || this.match(TokenType.Colon)) {
      // consume
    }
  }
}
