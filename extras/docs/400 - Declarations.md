# Declarations

Declaration adds new variables, functions or types to the program.

**[Syntax](Cmpl.g4)**
```antlrv4
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
```
