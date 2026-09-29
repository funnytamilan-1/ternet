import { Token, TokenType } from './types';

export class Lexer {
  private src: string;
  private pos: number = 0;
  private line: number = 1;
  private col: number = 1;

  constructor(src: string) {
    this.src = src;
  }

  tokenize(): Token[] {
    const tokens: Token[] = [];
    while (this.pos < this.src.length) {
      const ch = this.src[this.pos];

      // Whitespace
      if (ch === ' ' || ch === '\t' || ch === '\r') {
        this.advance();
        continue;
      }
      if (ch === '\n') {
        this.advance();
        this.line++;
        this.col = 1;
        continue;
      }

      // Line comment //
      if (ch === '/' && this.peek(1) === '/') {
        while (this.pos < this.src.length && this.src[this.pos] !== '\n') {
          this.advance();
        }
        continue;
      }

      // Block comment /* */
      if (ch === '/' && this.peek(1) === '*') {
        this.advance();
        this.advance();
        while (this.pos < this.src.length && !(this.src[this.pos] === '*' && this.peek(1) === '/')) {
          if (this.src[this.pos] === '\n') {
            this.line++;
            this.col = 1;
          }
          this.advance();
        }
        if (this.pos < this.src.length) {
          this.advance();
          this.advance();
        }
        continue;
      }

      const startLine = this.line;
      const startCol = this.col;

      // Numbers
      if (this.isDigit(ch)) {
        let numStr = '';
        let isFloat = false;
        while (this.pos < this.src.length && (this.isDigit(this.src[this.pos]) || this.src[this.pos] === '.')) {
          if (this.src[this.pos] === '.') {
            if (this.peek(1) === '.') {
              // Range syntax ..
              break;
            }
            if (isFloat) break;
            isFloat = true;
          }
          numStr += this.src[this.pos];
          this.advance();
        }
        tokens.push({ type: TokenType.Number, text: numStr, line: startLine, column: startCol });
        continue;
      }

      // Strings
      if (ch === '"' || ch === "'") {
        const quote = ch;
        this.advance();
        let str = '';
        while (this.pos < this.src.length && this.src[this.pos] !== quote) {
          if (this.src[this.pos] === '\\' && this.pos + 1 < this.src.length) {
            this.advance();
            const esc = this.src[this.pos];
            if (esc === 'n') str += '\n';
            else if (esc === 't') str += '\t';
            else if (esc === 'r') str += '\r';
            else if (esc === '\\') str += '\\';
            else if (esc === quote) str += quote;
            else str += esc;
          } else {
            if (this.src[this.pos] === '\n') {
              this.line++;
              this.col = 1;
            }
            str += this.src[this.pos];
          }
          this.advance();
        }
        if (this.pos < this.src.length) this.advance(); // consume closing quote
        tokens.push({ type: TokenType.String, text: str, line: startLine, column: startCol });
        continue;
      }

      // Identifiers and Keywords
      if (this.isAlpha(ch) || ch === '_') {
        let ident = '';
        while (this.pos < this.src.length && (this.isAlphaNum(this.src[this.pos]) || this.src[this.pos] === '_')) {
          ident += this.src[this.pos];
          this.advance();
        }
        const kwType = this.keywordType(ident);
        tokens.push({ type: kwType, text: ident, line: startLine, column: startCol });
        continue;
      }

      // Arrows & multi-char ops
      if (ch === '-' && this.peek(1) === '>') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Arrow, text: '->', line: startLine, column: startCol });
        continue;
      }
      if (ch === '=' && this.peek(1) === '=') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Op, text: '==', line: startLine, column: startCol });
        continue;
      }
      if (ch === '!' && this.peek(1) === '=') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Op, text: '!=', line: startLine, column: startCol });
        continue;
      }
      if (ch === '<' && this.peek(1) === '=') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Op, text: '<=', line: startLine, column: startCol });
        continue;
      }
      if (ch === '>' && this.peek(1) === '=') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Op, text: '>=', line: startLine, column: startCol });
        continue;
      }
      if (ch === '&' && this.peek(1) === '&') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Op, text: '&&', line: startLine, column: startCol });
        continue;
      }
      if (ch === '|' && this.peek(1) === '|') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Op, text: '||', line: startLine, column: startCol });
        continue;
      }
      if (ch === '.' && this.peek(1) === '.') {
        this.advance();
        this.advance();
        tokens.push({ type: TokenType.Op, text: '..', line: startLine, column: startCol });
        continue;
      }

      // Single character tokens
      const singleTokens: Record<string, TokenType> = {
        '(': TokenType.LParen,
        ')': TokenType.RParen,
        '{': TokenType.LBrace,
        '}': TokenType.RBrace,
        '[': TokenType.LBracket,
        ']': TokenType.RBracket,
        ',': TokenType.Comma,
        ':': TokenType.Colon,
        ';': TokenType.Semicolon,
        '.': TokenType.Dot,
      };

      if (singleTokens[ch] !== undefined) {
        this.advance();
        tokens.push({ type: singleTokens[ch], text: ch, line: startLine, column: startCol });
        continue;
      }

      // Operators +, -, *, /, %, =, <, >, !, &, |, ^
      if ('+-*/%=<>!&|^'.includes(ch)) {
        this.advance();
        tokens.push({ type: TokenType.Op, text: ch, line: startLine, column: startCol });
        continue;
      }

      // Fallback
      this.advance();
    }

    tokens.push({ type: TokenType.Eof, text: '', line: this.line, column: this.col });
    return tokens;
  }

  private advance(): string {
    const ch = this.src[this.pos];
    this.pos++;
    this.col++;
    return ch;
  }

  private peek(offset: number = 0): string {
    if (this.pos + offset >= this.src.length) return '';
    return this.src[this.pos + offset];
  }

  private isDigit(ch: string): boolean {
    return ch >= '0' && ch <= '9';
  }

  private isAlpha(ch: string): boolean {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
  }

  private isAlphaNum(ch: string): boolean {
    return this.isAlpha(ch) || this.isDigit(ch);
  }

  private keywordType(text: string): TokenType {
    switch (text) {
      case 'let': return TokenType.KeywordLet;
      case 'mut': return TokenType.KeywordMut;
      case 'const': return TokenType.KeywordConst;
      case 'fn': return TokenType.KeywordFn;
      case 'lit': return TokenType.KeywordLit;
      case 'return': return TokenType.KeywordReturn;
      case 'if': return TokenType.KeywordIf;
      case 'elif': return TokenType.KeywordElif;
      case 'else': return TokenType.KeywordElse;
      case 'while': return TokenType.KeywordWhile;
      case 'for': return TokenType.KeywordFor;
      case 'in': return TokenType.KeywordIn;
      case 'break': return TokenType.KeywordBreak;
      case 'continue': return TokenType.KeywordContinue;
      case 'match': return TokenType.KeywordMatch;
      case 'struct': return TokenType.KeywordStruct;
      case 'enum': return TokenType.KeywordEnum;
      case 'class': return TokenType.KeywordClass;
      case 'trait': return TokenType.KeywordTrait;
      case 'interface': return TokenType.KeywordInterface;
      case 'extends': return TokenType.KeywordExtends;
      case 'implements': return TokenType.KeywordImplements;
      case 'virtual': return TokenType.KeywordVirtual;
      case 'override': return TokenType.KeywordOverride;
      case 'abstract': return TokenType.KeywordAbstract;
      case 'static': return TokenType.KeywordStatic;
      case 'this': return TokenType.KeywordThis;
      case 'super': return TokenType.KeywordSuper;
      case 'try': return TokenType.KeywordTry;
      case 'catch': return TokenType.KeywordCatch;
      case 'finally': return TokenType.KeywordFinally;
      case 'throw': return TokenType.KeywordThrow;
      case 'import': return TokenType.KeywordImport;
      case 'export': return TokenType.KeywordExport;
      case 'true': return TokenType.KeywordTrue;
      case 'false': return TokenType.KeywordFalse;
      case 'null': return TokenType.KeywordNull;
      case 'type': return TokenType.KeywordType;
      default: return TokenType.Identifier;
    }
  }
}
