// { #include "type.c"
#include "main.h"
//~ type.c ---------------------------------------------------------------------
#include <string.h>

defn type_vid = 0;
defn type_bol = 0;
defn type_u32 = 0;
//~ defn type_u64 = 0;
defn type_i32 = 0;
defn type_i64 = 0;
defn type_f32 = 0;
defn type_f64 = 0;
defn type_str = 0;
//~ defn type_ptr = 0;
node oper_add = 0;

extern int rehash(register const char* str, unsigned size);

defn newdefn(state s, int kind) {
	defn def = (defn)s->buffp;
	s->buffp += sizeof(struct symn_t);
	memset(def, 0, sizeof(struct symn_t));
	def->kind = kind;
	return def;
}

int islval(node ast) {			// is an lvalue (has ref)
	if (!ast) return 0;
	if (ast->kind == TYPE_ref)
		return 1;
	if (ast->kind == OPER_idx)
		return 1;
	return 0;
}

//~ rett(defn get, defn ret)
//~ promote
//		any vid bit uns int flt ptr def

//any	 0   0   0   0   0   0   0   0 
//vid	 0  vid  0   0   0   0   0   0 
//bit	 0   0  bit uns int flt ptr def
//uns	 0   0  uns uns int flt ptr def
//int	 0   0  int int int flt ptr def
//flt	 0   0  flt flt flt flt  -  def
//ptr	 0   0  ptr ptr ptr  -  ptr def
//def	 0   0  def def def def def def


/*defn promote(defn ty) {
	if (ty) switch (ty->kind) {
		case TYPE_vid : return type_vid;
		//~ case TYPE_bit : return type_bin;
		case TYPE_uns : return type_u32;
		case TYPE_int : return (ty->size == 8) ? type_i64 : type_i32;
		case TYPE_flt : return (ty->size == 8) ? type_f64 : type_f32;
		//~ case TYPE_ptr : return ty->size ? type_arr : type_ptr;
		//~ case TYPE_enu :
		//~ case TYPE_rec :
	}
	return NULL;
}*/

//~ node iscnst(node ast) ;
//~ defn iscall(node ast) ;
//~ type iscast(node ast) ;
//~ defn isemit(node ast) ;

//~ long sizeOf(node ast);
//~ defn typeOf(node ast);

/* iscall()
defn iscast(node ast) ;		// returns type def
defn isemit(node ast) ;		// returns type ref
defn islibc(node ast) {		// returns type ref
	defn res;
	if (!ast || ast->kind != OPER_fnc) return 0;
	if (ast->lhso) switch (ast->lhso->kind) {
		case TYPE_ref : res = ast->lhso->link; break;
		case OPER_dot : res = ast->lhso->rhso ? ast->lhso->rhso->link : 0;
	}
	return res && res->libc ? res : 0;
}

defn iscall(node ast) {		// returns type ref
	if (!ast) return 0;
	if (ast->kind != TYPE_ref) return 0;
	if (ast->stmt || ast->link) return 0;
	return ast->type;
}*/

/*int typecmp(defn sym, char* name, node argv) {
	defn argt = sym->args;
	if (sym->name == 0)
		return 0;
	if (strcmp(name, sym->name) != 0)
		return 0;

	while (argv && argt) {
		if (argv->type != argt)
			return 0;
		argv = argv->next;
		argt = argt->next;
	}

	if (argv)
		return 0;

	/ *while (argt) {
		if (!argt->init)
			return 0;
		argt = argt->next;
	}* /

	if (argt)
		return 0;

	return 1;
}*/

defn instlibc(state s, const char* name) {
	int argsize = 0;
	node ftag = 0;
	defn args = 0, ftyp = 0;
	s->_ptr = (char*)name;
	s->_cnt = strlen(name);
	s->_tok = 0;
	if (!(ftag = next(s, TYPE_ref))) return 0;	// int ...
	if (!(ftyp = lookup(s, 0, ftag))) return 0;
	if (!istype(ftag)) return 0;

	if (!(ftag = next(s, TYPE_ref))) return 0;	// int bsr ...

	if (skip(s, PNCT_lp)) {				// int bsr ( ...
		if (!skip(s, PNCT_rp)) {		// int bsr ( )
			enter(s, ftyp);
			while (peek(s)) {
				node atag = 0;
				defn atyp = 0;

				if (!(atag = next(s, TYPE_ref))) break;	// int
				if (!(atyp = lookup(s, 0, atag))) break;
				if (!istype(atag)) break;

				//~ arg_ty = qual(s, 0);

				if (!(atag = next(s, TYPE_ref)))
					atag = newnode(s, TYPE_ref);

				declare(s, TYPE_ref, atag, atyp, 0);
				argsize += atyp->size;

				if (skip(s, PNCT_rp)) break;
				if (!skip(s, OPER_com)) break;
			}
			args = leave(s, 3);
		}
	}
	if (!peek(s)) {
		defn def = newdefn(s, TYPE_ref);
		unsigned hash = ftag->hash;
		def->name = ftag->name;
		def->nest = s->nest;
		def->type = ftyp;
		def->args = args;
		def->link = 0;
		def->libc = 1;
		def->call = 1;
		def->size = argsize;

		def->next = s->deft[hash];
		s->deft[hash] = def;

		// link
		def->defs = s->defs;
		s->defs = def;
		return def;
	}
	return NULL;
}// */

defn install(state s, int kind, const char* name, unsigned size) {
	defn typ = 0, def = 0;
	unsigned hash = 0;

	switch (kind) {		// check
		// declare
		case -1 : return instlibc(s, name);

		// constant
		case CNST_int : typ = type_i64; kind = TYPE_def; break;
		case CNST_flt : typ = type_f64; kind = TYPE_def; break;
		case CNST_str : typ = type_str; kind = TYPE_def; break;

		// basetype
		case TYPE_vid :
		case TYPE_bit :
		case TYPE_uns :
		case TYPE_u32 :
		//~ case TYPE_u64 :
		case TYPE_int :
		case TYPE_i32 :
		case TYPE_i64 :
		case TYPE_flt :
		case TYPE_f32 :
		case TYPE_f64 :
		case TYPE_ptr :

		// usertype
		case TYPE_def :
		case TYPE_enu :
		case TYPE_rec :

		// variable
		case TYPE_ref :
		case EMIT_opc :
			break;

		default :
			fatal(s, "install '%s'", name);	// this
			return 0;
	}

	def = newdefn(s, kind);
	def->kind = kind;
	def->nest = s->nest;
	def->type = typ;
	def->name = (char*)name;
	def->size = size;

	hash = rehash(name, strlen(name)) % TBLS;
	def->next = s->deft[hash];
	s->deft[hash] = def;

	def->defs = s->defs;
	s->defs = def;

	return def;
}

defn declare(state s, int kind, node tag, defn rtyp, defn args) {
	int hash = tag ? tag->hash : 0;
	defn def;// = s->deft[hash];

	if (!tag || tag->kind != TYPE_ref) {
		debug("declare: node('%k') is null or not an identifyer", tag);
		return 0;
	}

	// if declared on current level raise an error

	//~ debug("%k[%d]", tag, tag->hash);
	for (def = s->deft[hash]; def; def = def->next) {
		if (def->nest < s->nest) break;
		if (def->name && strcmp(def->name, tag->name) == 0) {
			error(s, s->file, tag->line, "redefinition of '%T'", def);
			if (def->link && s->file)
				info(s, s->file, def->link->line, "first defined here");
 		}
	}

	def = newdefn(s, kind);
	def->line = tag->line;
	def->file = s->file;

	def->name = tag->name;
	def->nest = s->nest;

	def->type = rtyp;
	def->args = args;
	def->link = tag;
	//~ tag->type = rtyp;

	switch (kind) {
		default :
			fatal(s, "declare:install('%T')", def);
			//~ break;

		//~ case CNST_int : def->type = type_i32; break;
		//~ case CNST_flt : def->type = type_f32; break;
		//~ case CNST_str : def->type = type_str; break;
		//~ case CNST_flt :
		//~ case CNST_str : {		// constant(define)
			//~ def->type = typ;
			//~ def->size = 4;
		//~ } break;
		case TYPE_def :
		//~ case TYPE_fnc :
		case TYPE_ref : {		// variable
			tag->kind = kind;
			tag->type = rtyp;
			tag->link = def;
			//~ def->size = typ->size;
		} break;

		case TYPE_enu :			// typedefn enum
		case TYPE_rec : {			// typedefn struct
		//~ case TYPE_def : {		// typedefn(define)
		} break;
	}
	// hash
	def->next = s->deft[hash];
	s->deft[hash] = def;

	// link
	def->defs = s->defs;
	s->defs = def;

	//~ debug("declare:install('%T')", def);
	return def;
}

defn getsym();

int samenode(node lhs, node rhs) {
	if (lhs == rhs) return 1;

	if (!lhs && rhs) return 0;
	if (lhs && !rhs) return 0;

	if (lhs->kind != rhs->kind)
		return 0;
	switch (rhs->kind) {
		case CNST_int : return lhs->cint == rhs->cint;
		case CNST_flt : return lhs->cflt == rhs->cflt;
		case CNST_str : return lhs->name == rhs->name;
		//~ case TYPE_ref : return lhs->link == rhs->link;

		case OPER_add : 
		case OPER_sub : 
		case OPER_mul : 
		case OPER_div : 
		case OPER_mod : 
		case OPER_idx : 
		case OPER_fnc : 
			return samenode(lhs->lhso, rhs->lhso)
				&& samenode(lhs->rhso, rhs->rhso);
	}
	return 0;
}

int align(int offs, int pack, int size) {
	switch (pack < size ? pack : size) {
		default : 
		case  0 : return 0;
		case  1 : return offs;
		case  2 : return (offs +  1) & (~1);
		case  4 : return (offs +  3) & (~3);
		case  8 : return (offs +  7) & (~7);
		case 16 : return (offs + 15) & (~15);
		case 32 : return (offs + 31) & (~31);
	}
}

// }
