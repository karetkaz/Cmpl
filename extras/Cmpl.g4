grammar Cmpl;

// starting point for parsing a file
unit: statement* EOF;

statement
    : ';'                                                                                               # EmptyStatement
    | 'inline' Literal '?'? ';'                                                                       # IncludeStatement
    | qualifiers? '{' statement* '}'                                                                 # CompoundStatement
    | qualifiers? 'if' '(' declExpr ')' statement ('else' statement)?                                      # IfStatement
    | qualifiers? 'for' '(' declExpr? ';' expression? ';' expression? ')' statement                       # ForStatement
    | 'for' '(' variable ':' expression ')' statement                                                    # EachStatement
    | 'return' initializer? ';'                                                                        # ReturnStatement
    | 'break' ';'                                                                                       # BreakStatement
    | 'continue' ';'                                                                                 # ContinueStatement
    | expression ';'                                                                               # ExpressionStatement
    | declaration                                                                                 # DeclarationStatement
    ;

declaration
    : qualifiers? 'enum' Identifier? (':' typename)? '{' propertyList '}'                              # EnumDeclaration
    | qualifiers? 'struct' Identifier? ('(' templateList? ')')? (':' expression)? '{' statement* '}'   # TypeDeclaration
    | qualifiers? 'inline' Identifier? ('(' parameterList? ')')? '=' initializer ';'                  # AliasDeclaration
    | qualifiers? (variable | function) ( '=' initializer)? ';'                                    # VariableDeclaration
    | qualifiers? function '{' statement* '}'                                                      # FunctionDeclaration
    | qualifiers? 'inline' unary ('(' parameterList? ')')? '=' initializer ';'                   # UnaryOperatorOverload
    | qualifiers? 'inline' binary ('(' parameterList? ')')? '=' initializer ';'                 # BinaryOperatorOverload
    | qualifiers? 'inline' '('')' ('(' parameterList? ')')? '=' initializer ';'                # InvokerOperatorOverload
    | qualifiers? 'inline' '['']' ('(' parameterList? ')')? '=' initializer ';'                # IndexerOperatorOverload
    ;

expression
    : Literal                                                                                        # LiteralExpression
    | Identifier                                                                                  # IdentifierExpression
    | '(' argumentList? ')'                                                                    # ParenthesizedExpression
    | expression '(' argumentList? ')'                                                          # FunctionCallExpression
    | expression '[' argumentList? ']'                                                           # ArrayAccessExpression
    | expression '[' expression? '...' expression ']'                                             # ArraySliceExpression
    | expression '.' Identifier                                                                 # MemberAccessExpression
    | unary expression                                                                                 # UnaryExpression
    | expression (binary | logical) expression                                                        # BinaryExpression
    | expression '?' expression ':' expression                                                   # ConditionalExpression
    | 'struct' '(' argumentList? ')'                                                                      # TEMP// FIXME
    ;

typename: expression;
variable: typename Identifier ('&&' | '&' | '?' | ('[' ('*' | expression)? ']')*);
function: typename Identifier '(' parameterList? ')';
property: (Literal | Identifier ':')? initializer;
initializer: typename? '{' propertyList '}'| expression;

parameterList: qualifiers? (variable | function) (',' parameterList)? '...'?;
templateList: ('struct' | typename) Identifier (',' templateList)?;
argumentList: ('&' | '...')? expression (',' argumentList)?;
propertyList: (declaration | property ';')* | (property (',' property)* ','?)?;              // todo: remove declaration
declExpr: expression | (variable '=' initializer);

qualifiers: ('const' | 'static')+;
binary: arithmetic | bitwise | relational | equality | assignment;
unary: ('+' | '-' | '~' | '!');
arithmetic: ('*' | '/' | '%' | '+' | '-');
bitwise: ('&' | '|' | '^' | '<<' | '>>');
relational: ('<' | '<=' | '>' | '>=');
equality: ('==' | '!=');
assignment: '=' | '*=' | '/=' | '%=' | '+=' | '-=' | '&=' | '|=' | '^=' | '<<=' | '>>=';
logical: '&&' | '||';

Identifier: Letter (Letter | Number)*;

Literal
    : '0'[bB][0-1]+                                                                                            // binary
    | '0'[oO]?[0-7]+                                                                                            // octal
    | '0'[xX][0-9a-fA-F]+                                                                                 // hexadecimal
    | Decimal Suffix?                                                                                         // decimal
    | Decimal Exponent Suffix?                                                                 // floating point literal
    | '.' Number+ Exponent? Suffix?                                                            // floating point literal
    | Decimal '.' Number* Exponent? Suffix?                                                    // floating point literal
    | '\'' .*? '\''                                                                                 // character literal
    | '"' .*? '"'                                                                                      // string literal
    ;

fragment Number: [0-9];
fragment Letter: [_A-Za-z];
fragment Suffix: [dDuUfF];
fragment Decimal: [0] | ([1-9] Number*);
fragment Exponent: [eE] [+-]? Number+;

Whitespace: [ \t\n\r]+ -> skip;
LineComment: '//' ~[\r\n]* -> skip;
BlockComment: '/*' .*? '*/' -> skip;
