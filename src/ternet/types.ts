export enum TokenType {
  Eof,
  Identifier,
  Number,
  String,
  Op,
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Colon,
  Semicolon,
  Dot,
  Arrow,
  KeywordLet,
  KeywordMut,
  KeywordConst,
  KeywordFn,
  KeywordLit,
  KeywordReturn,
  KeywordIf,
  KeywordElif,
  KeywordElse,
  KeywordWhile,
  KeywordFor,
  KeywordIn,
  KeywordBreak,
  KeywordContinue,
  KeywordMatch,
  KeywordStruct,
  KeywordEnum,
  KeywordClass,
  KeywordTrait,
  KeywordInterface,
  KeywordExtends,
  KeywordImplements,
  KeywordVirtual,
  KeywordOverride,
  KeywordAbstract,
  KeywordStatic,
  KeywordThis,
  KeywordSuper,
  KeywordTry,
  KeywordCatch,
  KeywordFinally,
  KeywordThrow,
  KeywordImport,
  KeywordExport,
  KeywordTrue,
  KeywordFalse,
  KeywordNull,
  KeywordType,
}

export interface Token {
  type: TokenType;
  text: string;
  line: number;
  column: number;
}

export interface AstType {
  name: string;
  generics: AstType[];
  tupleTypes: AstType[];
  isTuple: boolean;
}

export enum ExprKind {
  Number,
  String,
  Bool,
  Null,
  Identifier,
  Binary,
  Unary,
  Call,
  Member,
  Index,
  Array,
  Tuple,
  Object,
  This,
  Super,
  Match,
  Ternary,
  TryCatch,
}

export interface AstExpr {
  kind: ExprKind;
  name?: string;
  numValue?: number;
  strValue?: string;
  boolValue?: boolean;
  op?: string;
  left?: AstExpr;
  right?: AstExpr;
  condition?: AstExpr;
  thenBranch?: AstExpr;
  elseBranch?: AstExpr;
  elements?: AstExpr[];
  object?: AstExpr;
  property?: string;
  index?: AstExpr;
  args?: AstExpr[];
  line: number;
  column: number;
}

export enum StmtKind {
  Let,
  Mut,
  Const,
  Assign,
  Expr,
  Return,
  If,
  While,
  For,
  Break,
  Continue,
  Function,
  Struct,
  Enum,
  Class,
  Trait,
  TryCatch,
  Throw,
  Import,
  Block,
}

export interface Parameter {
  name: string;
  type?: AstType;
}

export interface AstStmt {
  kind: StmtKind;
  name?: string;
  typeAnnotation?: AstType;
  expr?: AstExpr;
  target?: AstExpr;
  body?: AstStmt[];
  thenBranch?: AstStmt[];
  elseBranch?: AstStmt[];
  condition?: AstExpr;
  init?: AstStmt;
  step?: AstStmt;
  iteratorName?: string;
  iterable?: AstExpr;
  params?: Parameter[];
  returnType?: AstType;
  typeParams?: string[];
  fields?: { name: string; type?: AstType; isStatic?: boolean }[];
  methods?: AstStmt[];
  variants?: { name: string; value?: number }[];
  baseClass?: string;
  implementsTraits?: string[];
  isVirtual?: boolean;
  isOverride?: boolean;
  isStatic?: boolean;
  isAbstract?: boolean;
  catchVar?: string;
  catchBody?: AstStmt[];
  finallyBody?: AstStmt[];
  importPath?: string;
  line: number;
  column: number;
}

export interface TernetValue {
  type: 'null' | 'bool' | 'int' | 'float' | 'string' | 'array' | 'tuple' | 'object' | 'function' | 'option' | 'result';
  boolVal?: boolean;
  numVal?: number;
  strVal?: string;
  arrVal?: TernetValue[];
  className?: string;
  fields?: Map<string, TernetValue>;
  fnName?: string;
  params?: Parameter[];
  body?: AstStmt[];
  isNative?: boolean;
  nativeFn?: (...args: TernetValue[]) => TernetValue;
  isSome?: boolean;
  isOk?: boolean;
  innerVal?: TernetValue;
}

export interface Diagnostic {
  code: string;
  message: string;
  severity: 'error' | 'warning' | 'info';
  line: number;
  column: number;
  context?: string;
  suggestion?: string;
}

export enum OpCode {
  Halt,
  Const,
  Load,
  Store,
  Pop,
  Dup,
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Neg,
  Not,
  Eq,
  Neq,
  Lt,
  Lte,
  Gt,
  Gte,
  And,
  Or,
  Jump,
  JumpIfFalse,
  JumpIfTrue,
  Call,
  CallMethod,
  Return,
  MakeArray,
  MakeTuple,
  MakeObject,
  IndexGet,
  IndexSet,
  MemberGet,
  MemberSet,
  Print,
  PushCatch,
  PopCatch,
  Throw,
}

export interface Instruction {
  op: OpCode;
  arg: number;
  line?: number;
}

export interface BytecodeProgram {
  instructions: Instruction[];
  constants: TernetValue[];
  names: string[];
}
