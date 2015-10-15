grammar psvm;

options {
	language=Java;
	backtrack=true;
	memoize=true;
	//k=1;
}

program: stmt*;

stmt

	// expression statement
	: expr ';'

	// declaration statements
	| decl_enum
	| ('static')? decl_type
	| ('static' | 'const')+ decl_alias
	| ('static' | 'const')+ decl_var
	| ('static' | 'const')+ decl_fun
	| ('static' | 'const')+ decl_op

	// 
	: ';'
	| ('parallel')? stmt_block
	| ('static')? stmt_if
	| ('static' | 'parallel')? stmt_for
	| 'continue' ';'
	| 'break' ';'
	| 'return' (expr)? ';'
	;

stmt_block: '{' stmt* '}';
stmt_if: 'if' '(' expr ')' stmt ('else' stmt)?;
stmt_for: 'for' '(' (decl_var | expr)? ';' expr? ';' expr? ')' stmt;

decl_enum: 'enum' REF? (':' type)? '{' (REF decl_init? ';')* '}'
decl_type: 'struct' REF? (':' (PACK | type))? '{' stmt+ '}'
decl_alias: 'define' REF ('(' args ')')? decl_init ';';
decl_var: type REF ('[' expr? ']')* (decl_init)? ';';
decl_fun: type REF '(' args ')' ((decl_init)? ';' | stmt_block)
decl_op
	: 'operator' REF '(' type REF ')' decl_init ';'
	| 'operator' UNOP '(' type REF ')' decl_init ';'
	| 'operator' '(' type REF ')' BINOP '(' type REF ')' decl_init ';'
	;

decl_init: init_var | init_lit

init_var
	: (':' | ':=') expr

init_lit
	: (':' | ':=') '{' (expr (',' expr)*)? '}'
	| (':' | ':=') '{' (REF ':' decl_init (',' REF ':' decl_init)*)? '}'

expr: (REF | CON | ('(' expr ')') | (UNOP expr)) (('(' expr ')') | ('[' expr ']') | ('.' REF) | ('?' expr ':' expr) | (BINOP expr))*;

type: REF ('.' REF)*;

args: (type REF (',' type REF)*)?;

fragment UNOP
	: '+' | '-' | '~' | '!'
	//| 'new';
	;

fragment BINOP
	: ('+' | '-' | '*' | '/' | '%')
	| ('&' | '|' | '^' | '<<' | '>>')
	| ('<' | '<=' | '>' | '>=' | '==' | '!=')
	| ('=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<=' | '>>=')
	;

fragment REF : LETTER (LETTER | NUMBER)*;

fragment CON
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

fragment WS : (' '|'\r'|'\t'|'\n') {$channel=HIDDEN;};

fragment PACK: '0' | '1' | '2' | '4' | '8' | '16';
