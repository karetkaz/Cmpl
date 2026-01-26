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
//  | 'static'? 'if' '(' declExpr ')' 'else' statement                                                     # IfStatement
//  | 'static' 'if' '(' declExpr ')' '{' Identifier '}'                                                    # IfStatement
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
//  | ('struct' | 'inline') '(' argumentList? ')'                                                     # TypeOfExpression
    ;

typename: 'struct' '(' expression ')' | expression;
variable: typename Identifier ('&' | '?' | '!' | '&?')? ('[' ('*' | expression)? ']')*;
function: typename Identifier '(' parameterList? ')';
property: (Literal | Identifier ':')? initializer;
initializer: typename? '{' ('...' | propertyList) '}' | expression;

parameterList: 'static'? (variable | function) (',' parameterList)? '...'?;
argumentList: ('...')? expression (',' argumentList)?;
propertyList: ('static'? declaration | property ';')* | (property (',' property)* ','?)?;    // todo: remove declaration
declExpr: expression | (variable '=' initializer);

binary: arithmetic | bitwise | relational | equality | assignment;
unary: '+' | '-' | '~' | '!' | '&';
arithmetic: '*' | '/' | '%' | '+' | '-';
bitwise: '&' | '|' | '^' | '<<' | '>>';
relational: '<' | '<=' | '>' | '>=';
equality: '==' | '!=';
logical: '&&' | '||';
assignment: '=' | '*=' | '/=' | '%=' | '+=' | '-=' | '&=' | '|=' | '^=' | '<<=' | '>>=';

Identifier: [_A-Za-z] [_A-Za-z0-9]*;

Literal
    : '0'[bB][0-1]+ [dDuU]?                                                                                    // binary
    | '0'[oO][0-7]+ [dDuU]?                                                                                     // octal
    | '0'[xX][0-9a-fA-F]+ [dDuU]?                                                                         // hexadecimal
    | [0-9]+ [dDuUfF]?                                                                                        // integer
    | '.' [0-9]+ ([eE] [+-]? [0-9]+)? [fF]?                                                                     // float
    | [0-9]+ ('.' [0-9]*)? ([eE] [+-]? [0-9]+)? [fF]?                                                           // float
    | ['] .*? [']                                                                                   // character literal
    | ["] .*? ["]                                                                                      // string literal
    ;

Whitespace: [ \t\n\r]+ -> skip;
LineComment: '//' ~[\r\n]* -> skip;
BlockComment: '/*' .*? '*/' -> skip;

//HashBang: '#!' ~[\r\n]* -> skip;
