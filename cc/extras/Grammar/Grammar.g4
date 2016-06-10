grammar Grammar;

// options {language=Java;}

program: statement_list;

statement
	: ';'  // empty statement
	| '{' statement_list '}'
	| statement_for
	| statement_if
	| 'break' ';'
	| 'continue' ';'
	| 'return' (expression)? ';'
	// declaration statements
	| declare_alias ';'
	| declare_type
	| declare_enum
	| declare_fun
	| declare_var initializer? ';'
	// expression statement
	| expression ';'
	;

expression
	: NAME
	| VALUE
	| '(' expression ')'
	| unop expression
	| expression binop expression
	| expression '.' NAME
	| expression '(' expression? ')'
	| expression '[' expression? ']'
	| expression '?' expression ':' expression
	;

statement_list: (qualifier? statement)*;
statement_for: 'for' '(' ((declare_var initializer?) | expression)? ';' expression? ';' expression? ')' statement;
statement_if: 'if' '(' expression ')' statement ('else' statement)?;

//~ declare_operator: 'operator' ...;
declare_alias: 'define' NAME ('(' parameters? ')')? initializer?;
declare_type: 'struct' NAME? (':' (VALUE | typename))? '{' statement_list '}';
declare_enum: 'enum' NAME? (':' typename)? '{' (NAME initializer? ';')* '}';
declare_var : typename ('&' | '&&')? NAME ('(' parameters? ')')? ('[' expression? ']')*;
declare_fun: typename NAME ('(' parameters? ')') '{' statement_list '}';

initializer: ('=' | ':=') (expression | ('{' (expression | (properties ';')+) '}'));

parameters: qualifier? declare_var (',' parameters)*;
properties: NAME ((':' ((typename? '{' (properties ';')+ '}') | expression)) | ('{' (properties ';')+ '}'));
qualifier: ('const' | 'static' | 'parallel')+;
typename: NAME ('.' NAME)*;

binop
	: '+' | '-' | '*' | '/' | '%'
	| '&' | '|' | '^' | '<<' | '>>'
	| '<' | '<=' | '>' | '>=' | '==' | '!='
	| '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<=' | '>>='
	| '&&' | '||' | ','
	// TODO: remove ':='
	| ':='
	;

unop: '+' | '-' | '~' | '!' | '&';

NAME : LETTER (LETTER | NUMBER)*;

VALUE
	: '0' | '1'..'9' NUMBER*
	// floating point literals
	| '.' NUMBER+ (('e'|'E') ('+'|'-')? NUMBER+)?
	| (NUMBER+ ('.' NUMBER*)?) (('e'|'E') ('+'|'-')? NUMBER+)?
	// custom base integers
	| '0x'('a'..'f' | 'A'..'F' | '0'..'9')+
	| '0b'('0'..'1')+
	| '0o'('0'..'7')+
	//| '0'('0'..'7')+
	// character and string literals
	| '\'' .*? '\''
	| '"' .*? '"'
	//| '0'('0'..'7')+
	;

fragment NUMBER : '0' .. '9';
fragment LETTER : '_' | 'A' .. 'Z' | 'a' .. 'z';

Whitespace
    :   [ \t]+
        -> skip
    ;

Newline
    :   (   '\r' '\n'?
        |   '\n'
        )
        -> skip
    ;

BlockComment
    :   '/*' .*? '*/'
        -> skip
    ;

BlockComment2
    :   '/+' .*? '+/'
        -> skip
    ;

LineComment
    :   '//' ~[\r\n]*
        -> skip
    ;