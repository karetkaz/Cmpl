grammar Cmpl;

// starting point for parsing a file
unit: statement* EOF;

statement
    : ';'                                                                                               # EmptyStatement
    | 'inline' Literal '?'? ';'                                                                       # IncludeStatement
    | 'static'? '{' statement* '}'                                                                   # CompoundStatement
    | 'static'? 'if' '(' declExpr ')' statement ('else' statement)?                                        # IfStatement
    | 'static'? 'for' '(' declExpr? ';' expression? ';' expression? ')' statement                         # ForStatement
    | 'return' initializer? ';'                                                                        # ReturnStatement
    | 'break' ';'                                                                                       # BreakStatement
    | 'continue' ';'                                                                                 # ContinueStatement
    | 'static'? declaration                                                                       # DeclarationStatement
    | 'static'? expression ';'                                                                     # ExpressionStatement
    ;

declaration
    : 'enum' Identifier? (':' typename)? '{' propertyList '}'                                          # EnumDeclaration
    | 'struct' Identifier? ('(' parameterList? ')')? (':' expression)? '{' statement* '}'              # TypeDeclaration
    | 'inline' Identifier? ('(' parameterList? ')')? '=' initializer ';'                              # AliasDeclaration
    | (variable | function) ( '=' initializer)? ';'                                                # VariableDeclaration
    | function '{' statement* '}'                                                                  # FunctionDeclaration
    | 'inline' unary ('(' parameterList? ')')? '=' initializer ';'                               # UnaryOperatorOverload
    | 'inline' binary ('(' parameterList? ')')? '=' initializer ';'                             # BinaryOperatorOverload
    | 'inline' '('')' ('(' parameterList? ')')? '=' initializer ';'                            # InvokerOperatorOverload
    | 'inline' '['']' ('(' parameterList? ')')? '=' initializer ';'                            # IndexerOperatorOverload
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
    ;

typename: 'struct' '(' expression ')' | expression;
variable: typename Identifier ('&' | '?' | '!' | '&?')? ('[' ('*' | expression)? ']')*;
function: typename Identifier '(' parameterList? ')';
property: (Literal | Identifier ':')? initializer;
initializer: typename? '{' propertyList '}'| expression;

parameterList: 'static'? (variable | function) (',' parameterList)? '...'?;
argumentList: ('&' | '...')? expression (',' argumentList)?;
propertyList: (declaration | property ';')* | (property (',' property)* ','?)?;              // todo: remove declaration
declExpr: expression | (variable '=' initializer);

binary: arithmetic | bitwise | relational | equality | assignment;
unary: '+' | '-' | '~' | '!';
arithmetic: '*' | '/' | '%' | '+' | '-';
bitwise: '&' | '|' | '^' | '<<' | '>>';
relational: '<' | '<=' | '>' | '>=';
equality: '==' | '!=';
logical: '&&' | '||';
assignment: '=' | '*=' | '/=' | '%=' | '+=' | '-=' | '&=' | '|=' | '^=' | '<<=' | '>>=';

Identifier: Letter (Letter | [0-9])*;

Literal
    : '0'[bB][0-1]+                                                                                            // binary
    | '0'[oO][0-7]+                                                                                             // octal
    | '0'[xX][0-9a-fA-F]+                                                                                 // hexadecimal
    | '.' [0-9]+ Exponent?                                                                     // floating point literal
    | Number ('.' [0-9]*)? Exponent?                                                           // floating point literal
    | '\'' .*? '\''                                                                                 // character literal
    | '"' .*? '"'                                                                                      // string literal
    ;

fragment Letter: [_A-Za-z];
fragment Number: [0] | ([1-9] [0-9]*);
fragment Exponent: [eE] [+-]? [0-9]+;

Whitespace: [ \t\n\r]+ -> skip;
LineComment: '//' ~[\r\n]* -> skip;
BlockComment: '/*' .*? '*/' -> skip;
