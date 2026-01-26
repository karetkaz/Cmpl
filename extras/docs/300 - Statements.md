# Statements
Statements are the basic blocks of a program.

**[Syntax](Cmpl.g4)**
```antlrv4
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
```
