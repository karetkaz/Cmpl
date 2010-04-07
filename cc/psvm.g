grammar psvm;

options {
	language=CSharp2;
	backtrack=true;
	memoize=true;
	k=1;
}

program
	: stmt*
	;

stmt
	: decl
	| expr ';'
	| /* ('parallel')? */ '{' stmt* '}'

	| ('parallel'|'static')? stmt_for
	| ('static')? stmt_if

	| 'return' expr? ';'

	//~ | 'sync' ('('ID')')? '{' stmt '}'		// synchronized
	//~ | 'task' ('('ID')')? '{' stmt '}'		// parallel task
	//~ | 'fork' ('('ID')')? '{' stmt '}'		// parallel task
	;

stmt_for : 'for' '(' decl?';' expr?';' expr? ')' stmt;
stmt_if : 'if' '(' expr ')' stmt ('else' stmt)?;

decl
	: decl_typ
	| decl_var
	| decl_fnc
	;

decl_typ : 'TODO';
decl_var : 'TODO';
decl_fnc : 'TODO';

expr
	: CON
	| ID
	//~ | expr '[' expr ']'			//arr	array
	//! | '[' expr ']'				//ptr	pointer
	//~ | expr '(' expr? ')'		//fnc	function
	| '(' expr ')'				//pri	precedence
	| OP expr					//uop	unary operator
	//~ | expr OP expr				//bop	binary operator
	//~ | expr '?' expr ':' expr	//top	ternary operator
	;

ID : LETTER (LETTER | NUMBER)*;
OP : '+' | '-';

CON //: CHR | DEC | OCT | HEX | FLT | STR;
	: NUMBER+ '.' NUMBER* (('e'|'E') ('+'|'-')? NUMBER+)?
	| '.' NUMBER+ (('e'|'E') ('+'|'-')? NUMBER+)?
	| NUMBER*
	;

fragment NUMBER : '0'..'9';
fragment LETTER : '_' | 'A'..'Z' | 'a'..'z';

WS : (' '|'\r'|'\t'|'\n') {$channel=HIDDEN;};
