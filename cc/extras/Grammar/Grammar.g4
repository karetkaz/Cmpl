grammar Grammar;

// options {language=Java;}

program: statementList EOF;

statement
	: ';'                                                                     # EmptyStatement
	| '{' statementList '}'                                                   # CompoundStatement
	// control flow
	| 'for' '(' for_init? ';' expression? ';' expression? ')' statement       # ForStatement
	| 'for' '(' variable ':' expression ')' statement                         # ForEachStatement
	| 'if' '(' expression ')' statement ('else' statement)?                   # IfStatement
	| 'if' '(' variable '=' initializer ')' statement ('else' statement)?     # IfStatement
	| 'return' (expression)? ';'                                              # ReturnStatement
	| 'break' ';'                                                             # BreakStatement
	| 'continue' ';'                                                          # ContinueStatement
	| expression ';'                                                          # ExpressionStatement
	| declaration                                                             # DeclarationStatement
	| qualifier statement                                                     # QualifiedStatement   // static if, parallel for, and so on
	;
statementList: statement*;

declaration
	: qualifier declaration                                                   # QualifiedDeclaration
	| 'struct' Identifier? (':' (Literal | typename))? '{' declarationList '}' # TypeDeclaration
	| 'enum' Identifier? (':' typename)? '{' propertyList '}'                 # EnumDeclaration
	| 'define' Identifier parameters? '=' initializer ';'                     # InlineDeclaration
	| variable ( '=' initializer)? ';'                                        # VariableDeclaration
	| typename Identifier parameters '{' statementList '}'                    # FunctionDeclaration
	;
declarationList: declaration*;

expression
	: (Identifier | Literal)                                                  # ValueExpression
	| '(' expressionList? ')'                                                 # Parenthesized
	| expression '(' expressionList? ')'                                      # FunctionCall
	| expression '.' Identifier                                               # MemberAccess
	| expression '[' expressionList? ']'                                      # ElementAccess
	| ('+' | '-' | '~' | '!' | '&') expression                                # UnaryExpression
	| expression ('*' | '/' | '%' | '+' | '-') expression                     # ArithmeticExpression
	| expression ('&' | '|' | '^' | '<<' | '>>') expression                   # BitwiseExpression
	| expression ('<' | '<=' | '>' | '>=') expression                         # RelationalExpression
	| expression ('==' | '!=' | '===' | '!==') expression                     # EqualityExpression
	| expression ('=' | '+=' | '-=' | '*=' | '/=' | '%=') expression          # AssignmentExpression
	| expression ('&=' | '|=' | '^=' | '<<=' | '>>=') expression              # AssignmentExpression
	| expression ('&&' | '||') expression                                     # LogicalExpression
	| expression '?' expression ':' expression                                # TernaryExpression
	;
expressionList: expression (',' expression)*;

qualifier: ('const' | 'static' | 'parallel')+;
parameters: '(' (qualifier? variable (',' qualifier? variable)*)? ')';

typename: Identifier ('.' Identifier)*;
variable: typename ('&' | '&&')? Identifier parameters? ('[' ('*' | expression)? ']')*;
// TODO: allow only `var: value;` mode?
property: (Identifier | Literal) ((':' | '=') initializer)?;
propertyList: property (',' property)* | (property ';')+;

initializer
	: expression                  # Value
	| '{' expressionList '}'      # Array
	| '{' propertyList '}'        # Object
	// TODO: remove
	| '[' expressionList ']'      # Array
	| '[' expressionList ',' ']'  # TrailingArray
	| '{' expressionList ',' '}'  # TrailingArray
	;

for_init: expressionList | variable ('=' initializer)?;

Identifier : Letter (Letter | Number)*;

Literal
	: '0' | '1'..'9' Number* Suffix?
	// floating point literals
	| '.' Number+ (('e'|'E') ('+'|'-')? Number+)? Suffix?
	| (Number+ ('.' Number*)?) (('e'|'E') ('+'|'-')? Number+)? Suffix?
	// custom base integers
	| '0x'('a'..'f' | 'A'..'F' | '0'..'9')+
	| '0b'('0'..'1')+
	| '0o'('0'..'7')+
	//| '0'('0'..'7')+
	// character and string literals
	| '\'' .*? '\''
	| '"' .*? '"'
	;

fragment Suffix : 'f' | 'F' | 'd' | 'D';
fragment Number : '0' .. '9';
fragment Letter : '_' | 'A' .. 'Z' | 'a' .. 'z';

Whitespace: [ \t\n\r]+ -> skip;
LineComment: '//' ~[\r\n]* -> skip;
BlockComment: '/*' .*? '*/' -> skip;
NestedComment: '/+' .*? '+/' -> skip;

