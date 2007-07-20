#ifndef __xcc_h
#define __xcc_h

#define HS 0x23847fff
#define MAX_TOKS 512
#define MAX_TBLS 256
#define bufmaxlen (32<<20)

enum {								// tokens
#include "tokens.h"
};

enum {								// state automata
	/** expr : ( expr )
	 *		 | cval
	 *		 | name
	 *		 | name \. name				\dot	member ref
	 *		 | name \( [expr] \)		\fnc	function call | cast | ctor | size | ...
	 *		 | name (\[ expr \])+		\arr	array reference
	X*		 | (++/--) expr				\uop	pre increment
	X*		 | expr (++/--)				\uop	post increment
	 *		 | uopr expr				\uop	unary operators
	 *		 | expr bopr expr			\bop	binary operators
	?*		 | expr '?' expr ':' expr	\top	ternary operators
	**/
	/** decl : (static)? type name = expr [, name = expr]*								// var decl
	 *		 | (static)? type name'['expr']' (= '('(expr[,expr]*)')'|expr)?				// arr decl
	X*		 | (static)? type name'('type name (= expr)? [,type name (= expr)?]*')'		// fnc decl
	**/
	/** stmt : { stmt; }
	 *		 | decl;
	 *		 | expr;
	 *		 | "return" (expr)? | break;
	 *		 | "for" ( expr?; expr?; expr? ) {stmt}
	 *		 | "if" (expr) {stmt} [else {stmt}]
	 *		 | ;
	**/

	get_cval = 0x00000001,
	get_name = 0x00000002,
	get_type = 0x00000004,
	get_expr = 0x00000003,
	get_decl = 0x00000006,
	get_stmt = 0x00000008,

	ack_null = 0x00000010,				// accept (), []
	ack_stop = 0x00000020,				// operator | ';'
	get_dots = 0x00000080,				// .
	get_arrs = 0x000000C0,				// [

	got_stop = 0x10000000,				// ,;)]
	got_lexe = 0x20000000,				// lexical ERROR
	got_stxe = 0x40000000,				// syntax error
	got_eror = 0x60000000				// 4+2
	#define get_next(__get_next) {get_next = ((get_next & (~0xff))|(__get_next));}
};

typedef struct llist_t {			// linked list
	struct llist_t* next;
	unsigned		flgs;
	void*			data;
} *llist, *table[MAX_TBLS];

typedef struct token_t {
	unsigned int			kind;				// decl, cnst, oper, type, name, keyw, stmt (low 16 bits) bit 24 : read, bit 26 : write
	signed int				line;				// token on line
	struct symbol_t*		type;				// type / link / proto type / oper result type
	char *					name;				// identifier, function name ?
	union {
		int					cint;				// cnst : integer
		double				cflt;				// cnst : float
		struct symbol_t*	cstr;				// cnst : string
		struct {
			struct token_t*	olhs;				// oper : left hand side
			struct token_t*	orhs;				// oper : right hand side
		};
	};
} *node, *tree;//, *symbol;

typedef struct symbol_t {
	unsigned int		kind;				// decl, cnst, oper, type, name, keyw, stmt
	union {
		//~ signed int		line;				// token on line
		signed int		dlev;				// declaration level
	};
	struct symbol_t*	type;				// type / link / proto type / oper result type
	char *				name;				// identifier, function name
	union {
		//~ int						cint;		// cnst : integer
		//~ double					cflt;		// cnst : float
		//~ const char*				cstr;		// cnst : string
		struct {
			union {
				//~ struct token_t*	olhs;		// oper : left hand side
				unsigned int	size;		// type : size
				unsigned int	offs;		// name : address
			};
			union {
				//~ struct token_t*	orhs;		// oper : right hand side
				//~ struct token_t*	args;		// func : function arguments
				//~ struct llist_t*	flds;		// stuc : struct-union fields
				struct token_t*	arrs;		// aray : array sizes
			};
		};
	};
} *symbol;

typedef struct cellpu_t {		// cell process unit
	long	pi;					// processed instuctions
	char*	ip;					// instruction pointer
	char*	is;					// instruction stop
	long*	sp;					// stack top
	long*	bp;					// stack base
	long	ss;					// stack size
	char*	error;
} *bcpu, cell;

typedef struct state_t {
	unsigned		errc;			// error count
	unsigned		data;			// global data usage
	struct {						// cc
		struct {					// input
			char*		buff;
			unsigned	line;
			const char*	name;
			FILE*		file;
			int			curchr, nextc;
			// nextchr, backchr;
		};
		struct {					// lexer
			table		strt;		// string table
			node		nextt;
			// nexttok, backtok, eattok;
		};
		struct {					// code
			table		deft;		// definitions
			llist		cbeg, cend;	// code
			llist		dbeg, dend;	// data (global only)
			llist		tbeg, tend;	// initialized data, string constants, ...
			int			level, jmps;
			struct {
				int			wait;
				//~ unsigned	flag;	// can break, continue, is parralel
				symbol		link;	// function, statment(for|if|null)
				node		test;	// if, while, for test
				node		incr;	// continue
				node		fork;	// continue
			} levelb[32];
		};
	};
	struct vm {						// vm
		//~ long	data;
		long	dcnt;
		long	ccnt;				// cell count
		//~ char	*tbeg, *tend;
		//~ char	*dbeg, *dend;
		char	*cbeg, *cend;
		//~ long	flags;				// bit 0-32 : cpu status ???
		long	fcell;				// cell avability
		cell	cel[32];
	} vm;
	char	*buffp, buffer[bufmaxlen+2];
} *state;

//~ /*
extern symbol stmt_if;
extern symbol stmt_for;
extern symbol stmt_ret;
extern symbol type_int;
extern symbol type_str;
extern symbol type_flt;
//~ extern symbol type_dbl;

extern symbol oper_sof;			// sizeof() operator
//~ extern symbol type_void;

// cgen
void walk(tree root, int fmt, int fix);

//~ node flow(state s, tree root);						// data flow

// syna		syntax
tree decl(state s);
tree stmt(state s);
tree expr(state s);

// sema		semantic
symbol cast(state s, node opr, node lhs, node rhs);		// type check
void fold(state s, node opr, node lhs, node rhs);		// type check

// lexa		lexic
node next_token(state);
node back_token(state, node);
node eat_token(state, node);

symbol nodetype(node);
unsigned nodesize(node);

// init
//~ void cc_init(state, const char * str);

char* lookupstr(state, char* name, int size, unsigned hash);
symbol lookupdef(state, char* name, int level, unsigned hash);
symbol install(state, symbol, char* name, int size, int kind);
void uninstall(state, int level);

int raiseerror(state, int level, int line, char *msg,...);
int enterblock(state s, symbol t, node fork, node test, node cont);
int leaveblock(state);					// remove names, types, on level;

int cc_open(state s, const char * str);
void cc_init(state s, const char * str);
int cc_done(state s);

void print_token(node ref, int);

extern const unsigned crc_tab[];
//~ */
#endif
