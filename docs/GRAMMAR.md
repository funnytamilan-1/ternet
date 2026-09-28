# Ternet V1 Grammar Sketch

This is a high-level parser contract. Lexer details and precedence tables belong to the compiler implementation.

```ebnf
program         = { declaration | statement } ;

statement       = binding
                | expression_statement
                | if_statement
                | loop_statement
                | return_statement
                | break_statement
                | continue_statement
                | match_statement ;

binding         = ("let" | "mut" | "const") identifier
                  [ ":" type ] "=" expression ":" ;

if_statement    = "if" "{" expression "}" ";" block
                  { "elif" "{" expression "}" ";" block }
                  [ "else" "{" "}" ";" block ] ;

block           = { declaration | statement } ;

function        = "fn" identifier "(" [ parameters ] ")"
                  [ "->" type ] block ":" ;

parameter       = identifier ":" type ;

match_statement = "match" expression "{"
                  { pattern "=>" statement }
                  "}" ":" ;

type            = primitive
                | identifier
                | identifier "<" type_list ">"
                | "&" [ "mut" ] type ;

expression      = assignment ;
assignment      = logical_or [ assignment_operator assignment ] ;
logical_or      = logical_and { "||" logical_and } ;
logical_and     = equality { "&&" equality } ;
equality        = comparison { ("==" | "!=") comparison } ;
comparison      = term { ("<" | "<=" | ">" | ">=") term } ;
term            = factor { ("+" | "-") factor } ;
factor          = unary { ("*" | "/" | "%") unary } ;
unary           = [ "!" | "-" | "&" ] postfix ;
postfix         = primary { call | index | member } ;
call            = "(" [ arguments ] ")" ;
index           = "[" expression "]" ;
member          = "." identifier ;
primary         = literal
                | identifier
                | "(" expression ")"
                | array_literal
                | struct_literal
                | lambda ;
literal         = integer | float | string | character
                | "true" | "false" | "null" ;
array_literal   = "[" [ arguments ] "]" ;
```

This grammar is a specification aid, not a claim that the current parser accepts all productions.
