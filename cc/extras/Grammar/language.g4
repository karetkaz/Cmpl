grammar language;

options {language=Java;}

program: statement_list;

/*TODO: '!!!' // TODO
	| properties
	| unit
	|
	|
	;
*/

statement
	: ';'
	| '{' statement_list '}'
	| statement_for
	| statement_if
	| 'break' ';'
	| 'continue' ';'
	| 'return' (expression)? ';'
	// declaration statements
	| declare_alias
	| declare_type
	| declare_enum
	// expression statement
	| expression ';'
	;

//~ declaration

expression
	: (REF | CON | ('(' expression ')') | (UNOP expression))
	(('(' expression ')') | ('[' expression ']') | ('.' REF) | ('?' expression ':' expression) | (BINOP expression))*;

statement_list: (qualifier statement)*;
statement_for: 'for' '(' (declare_var | expression)? ';' expression? ';' expression? ')' statement;
statement_if: 'if' '(' expression ')' statement ('else' statement)?;

//~ declare_operator: 'operator' ...;
declare_alias: 'define' REF ('(' parameters ')')? ('=' initializer)? ';';
declare_type: 'struct' REF? (':' (PACK | typename))? '{' statement+ '}';
declare_enum: 'enum' REF? (':' typename)? '{' (REF  ('=' initializer)? ';')* '}';
declare_var: typename REF ('(' parameters ')')? ('[' expression ']')* ('=' initializer)? ';';

initializer: expression | ('{' properties* '}');

parameters: (declare_var(',' declare_var))?;
properties: (REF '=' expression);
qualifier: ('const' | 'static' | 'parallel')*;
typename: expression;	// where expression is a typename


UNOP: '+' | '-' | '~' | '!';

BINOP
	: ('+' | '-' | '*' | '/' | '%')
	| ('&' | '|' | '^' | '<<' | '>>')
	| ('<' | '<=' | '>' | '>=' | '==' | '!=')
	| ('=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<=' | '>>=')
	;

REF : LETTER (LETTER | NUMBER)*;

CON
	: NUMBER+ '.' NUMBER* (('e'|'E') ('+'|'-')? NUMBER+)?
	| '.' NUMBER+ (('e'|'E') ('+'|'-')? NUMBER+)?
	| NUMBER+
	| '0x'('a'..'f' | 'A'..'F' | '0'..'9')+
	| '0b'('0'..'1')+
	| '0o'('0'..'7')+
	//| '0'('0'..'7')+
	;

fragment NUMBER : '0' .. '9';
fragment LETTER : '_' | 'A' .. 'Z' | 'a' .. 'z';

PACK: '0' | '1' | '2' | '4' | '8' | '16';

WS: [ \t\n\r]+ -> skip ;
