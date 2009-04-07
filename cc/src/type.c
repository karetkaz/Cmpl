//{ #include "type.c"
#include "ccvm.h"
//~ type.c ---------------------------------------------------------------------
#include <string.h>

defn type_vid = 0;
defn type_u32 = 0;
//~ defn type_u64 = 0;
defn type_i32 = 0;
defn type_i64 = 0;
defn type_f32 = 0;
defn type_f64 = 0;
defn type_str = 0;
defn emit_opc = 0;
//~ defn type_ptr = 0;

defn newdefn(state s, int kind) {
	defn def = (defn)s->buffp;
	s->buffp += sizeof(struct symn_t);
	memset(def, 0, sizeof(struct symn_t));
	def->kind = kind;
	return def;
}

int typeId(node ast) {
	if (!ast || !ast->type) return 0;
	return ast->type->kind;
}

/*defn typeof(node ast) {
	return NULL;
}*/

int castid(node lhs, node rhs) {
	if (!lhs && !rhs) return 0;
	if (!rhs) return typeId(lhs);
	if (!lhs) return typeId(rhs);
	switch (typeId(lhs)) {
		case TYPE_i32 : switch (typeId(rhs)) {
			case TYPE_u32 : return TYPE_i32;
			case TYPE_i32 : return TYPE_i32;
			case TYPE_f32 : return TYPE_f32;
			case TYPE_f64 : return TYPE_f64;
		} break;
		case TYPE_u32 : switch (typeId(rhs)) {
			case TYPE_u32 : return TYPE_u32;
			case TYPE_i32 : return TYPE_u32;
			case TYPE_f32 : return TYPE_f32;
			case TYPE_f64 : return TYPE_f64;
		} break;
		case TYPE_f32 : switch (typeId(rhs)) {
			case TYPE_u32 : return TYPE_f32;
			case TYPE_i32 : return TYPE_f32;
			case TYPE_f32 : return TYPE_f32;
			case TYPE_f64 : return TYPE_f64;
		} break;
		case TYPE_f64 : switch (typeId(rhs)) {
			case TYPE_u32 : return TYPE_f64;
			case TYPE_i32 : return TYPE_f64;
			case TYPE_f32 : return TYPE_f64;
			case TYPE_f64 : return TYPE_f64;
		} break;
	}
	return 0;
}

int istype(node ast) {
	if (!ast) return 0;
	if (ast->kind != TYPE_ref) return 0;
	if (ast->link) return 0;
	if (ast->stmt) return istype(ast->stmt);
	return ast->type != 0;
}

int islval(node ast) {
	if (!ast) return 0;
	if (ast->kind == TYPE_ref)
		return 1;
	if (ast->kind == OPER_idx)
		return 1;
	return 0;
}

/*defn iscast(node ast) ;
defn iscall(node ast) ;
defn islibc(node ast) {
	defn res;
	if (!ast || ast->kind != OPER_fnc) return 0;
	debug("%+k", ast);
	if (ast->lhso) switch (ast->lhso->kind) {
		case TYPE_ref : res = ast->lhso->link; break;
		case OPER_dot : res = ast->lhso->rhso ? ast->lhso->rhso->link : 0;
	}
	return res ? res->libc ? res : 0 : 0;
}
*/
/*defn iscall(node ast) {
	if (!ast) return 0;
	if (ast->kind != TYPE_ref) return 0;
	if (ast->stmt || ast->link) return 0;
	return ast->type != 0;
}*/

/*defn typofid(int id) {
	switch (id) {
		case TYPE_u08 :
		case TYPE_u16 :
		case TYPE_u32 : return type_u32;

		case TYPE_i08 :
		case TYPE_i16 :
		case TYPE_i32 : return type_i32;

		case TYPE_i64 : return type_i64;

		case TYPE_f32 : return type_f32;
		case TYPE_f64 : return type_f64;

		default : return 0;
	}
}*/

defn install(state s, char* name, unsigned size, int kind) {
	defn typ = 0, def = newdefn(s, kind);
	unsigned hash = rehash(name, name ? strlen(name) : 0) % TBLS;

	switch (kind) {		// check
		// declare
		case 0: break;

		// constant
		case CNST_int : typ = type_i64; break;
		case CNST_flt : typ = type_f64; break;
		case CNST_str : typ = type_str; break;

		// typename
		case TYPE_vid :
		//~ case TYPE_bit :
		//~ case TYPE_uns :
		//~ case TYPE_int :
		//~ case TYPE_flt :
		case TYPE_u08 :
		case TYPE_u16 :
		case TYPE_u32 :

		case TYPE_i08 :
		case TYPE_i16 :
		case TYPE_i32 :
		case TYPE_i64 :

		case TYPE_f32 :
		case TYPE_f64 :

		case TYPE_enu :
		//~ case TYPE_rec :
			break;
		// 
		//~ case TYPE_ref : scan;
		case EMIT_opc : 
		case TYPE_def : 
		case TYPE_fnc : 
			break;

		default :
			fatal(0, "install '%s'", name);	// this 
			return 0;
	}

	def->kind = kind;
	def->nest = s->nest;
	def->type = typ;
	def->name = name;
	def->size = size;

	def->deft = s->deft[hash];
	s->deft[hash] = def;

	def->next = s->defs;
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
	for (def = s->deft[hash]; def; def = def->deft) {
		if (def->nest < s->nest) break;
		// typecmp(def, rtyp, args)
		if (def->name && (strcmp(def->name, tag->name) == 0)) {
			if (kind != TYPE_fnc) {
			error(s, s->file, tag->line, "redefinition of '%T'", def);
			if (def->link && s->file)
				info(s, s->file, def->link->line, "first defined here");
			}
		}
	}

	def = newdefn(s, kind);
	def->name = tag->name;
	def->nest = s->nest;
	def->type = rtyp;
	def->args = args;
	def->link = tag;
	//~ tag->type = rtyp;

	switch (kind) {
		//~ case CNST_int : def->type = type_i32; break;
		//~ case CNST_flt : def->type = type_f32; break;
		//~ case CNST_str : def->type = type_str; break;
		//~ case CNST_flt :
		//~ case CNST_str : {		// constant(define)
			//~ def->type = typ;
			//~ def->size = 4;
		//~ } break;
		//~ case TYPE_fnc :
		case TYPE_ref : {		// variable
			tag->kind = kind;
			tag->type = rtyp;
			tag->link = def;
			//~ def->size = typ->size;
		} break;

		case TYPE_enu :			// typedefn enum
		case TYPE_rec :			// typedefn struct
		case TYPE_def : {		// typedefn(define)
		} break;
	}
	// hash
	def->deft = s->deft[hash];
	s->deft[hash] = def;

	// link
	def->next = s->defs;
	s->defs = def;

	//~ debug("declare:install('%T')", def);
	return def;
}

defn getsym();

defn lookup(state s, node ptr, node arg) {
	//~ int lht = 0, rht = 0;
	node ast = ptr;
	defn loc = 0, sym, lht = 0, rht = 0;

	//~ debug("lookup: '%k' in {%T}", ast, typ);

	if (!ast) {
		debug("lookup: node is null");
		return 0;
	}
	if (ast->type) {
		return ast->type;
	}
	if (ast->kind == OPER_dot) {
		if (!(loc = lookup(s, ast->lhso, 0))) return 0;
		if (!ast->rhso || ast->rhso->kind != TYPE_ref) {
			//~ error(...)
			debug("OPER_dot");
			return 0;
		}
		ast = ast->rhso;
	}
	switch (ast->kind) {
		case OPER_fnc : {		// '()'
			if (ast->lhso) {
				node argv = ast->rhso;
				while (argv) {
					node arg = argv->kind == OPER_com ? argv->rhso : argv;
					argv = argv->kind == OPER_com ? argv->lhso : 0;
					lookup(s, arg, 0);
				}

				//~ while ((argt[argc++] = *argp++));		// memcpy
				//~ */
				ast->type = lookup(s, ast->lhso, ast->rhso);
			}
			else ast->type = lookup(s, ast->rhso, 0);
		} break;
		case OPER_idx : {		// '[]'
			if (!(lht = lookup(s, ast->lhso, 0))) return 0;
			if (!(rht = lookup(s, ast->rhso, 0))) return 0;
			if (!islval(ast->lhso)) {
				error(s, s->file, ast->line, "invalid lvalue on operator: '%k'", ast);
				break;
			}
			switch (typeId(ast->rhso)) {
				case TYPE_i32 :
				case TYPE_u32 : ast->type = lht->type; break;
				//~ case TYPE_f32 :
				//~ case TYPE_f64 : ast->type = ast->lhso->type; break;
				default : {
				} return 0;
			}
			//~ ast->type = lht->type;
		} break;

		/*case OPER_pri : (dst, "++"); break;
		//case OPER_prd : (dst, "--"); break;
		//case OPER_ptr : (dst, "*"); break;
		//case OPER_adr : (dst, "&"); break;
		*/
		/*case OPER_dot : {		// '.'
			sym = lookup(s, ast->lhso, 0);
			//~ ast->type = lookup(s, / *sym, * /ast->rhso);
			if (ast->rhso && ast->rhso->kind == TYPE_ref) {
				ast = ast->rhso;
				for (sym = sym->args; sym; sym = sym->next) {
					if (!sym->name) continue;
					//~ if (sym->name == ast->name) break;
					if (strcmp(sym->name, ast->name) == 0) break;
				}
				switch (sym ? sym->kind : 0) {
					// define: typedef|consdef

					case TYPE_vid :
					case TYPE_u08 :
					case TYPE_u16 :
					case TYPE_u32 :
					case TYPE_i08 :
					case TYPE_i16 :
					case TYPE_i32 :
					case TYPE_i64 :
					case TYPE_f32 :
					case TYPE_f64 :
					case TYPE_enu :
					case TYPE_rec : {		// typename
						ast->kind = TYPE_ref;
						ast->type = sym;
						ast->stmt = 0;
						ast->link = 0;
					} break;

					case CNST_int :
					case CNST_flt :
					case CNST_str :
					case TYPE_def : {
						ast->kind = TYPE_ref;
						ast->type = sym->type;
						ast->rhso = sym->init;
						ast->link = NULL;
						//~ ast->link = sym;
					} break;

					//~ case CNST_int :
					//~ case CNST_flt :
					//~ case CNST_str :			// constant
					//~ case TYPE_def :			// typename
					case TYPE_fnc : {		// function
						//~ int argc = 0;
						node argv = arg;
						defn argT = sym->args;
						node argt[TOKS], *argp = argt + TOKS;
						for (*--argp = 0; argv; ) {
							node arg = argv->kind == OPER_com ? argv->rhso : argv;
							argv = argv->kind == OPER_com ? argv->lhso : 0;
							*--argp = arg;//lookup(s, arg, 0);
						}
						//~ while ((argt[argc++] = *argp++));		// memcpy
						while (argT && *argp) {
							node arg = *argp;
							arg->cast = argT->type->kind;
							argT = argT->next;
							argp = argp + 1;
						}
					} // fall
					case TYPE_ref : {		// variable
						ast->kind = TYPE_ref;
						ast->type = sym->type;
						ast->cast = sym->type->kind;
						//~ ast->rhso = sym->init;	// init|body
						ast->link = sym;
						sym->used = 1;
					} break;
					//~ case 0 : return 0;
					default : {
						error(s, s->file, ast->line, "undeclared `%k` in `%?T`", ast, 0);
						sym = declare(s, TOKN_err, ast, NULL, NULL);
						//~ sym->type = type_i32;
						//~ ast->kind = TYPE_ref;
						//~ ast->type = type_i32;
						//~ ast->link = sym;
						//~ ast->type = 0;
						//~ sym = 0;
					} return 0;
				}
			}
		} break;*/

		case OPER_pls : 		// '+'
		case OPER_mns : {		// '-'
			if (!lookup(s, ast->rhso, 0)) return 0;
			switch (typeId(ast->rhso)) {
				case TYPE_u32 : ast->type = type_u32; break;
				case TYPE_i32 : ast->type = type_i32; break;
				case TYPE_f32 : ast->type = type_f32; break;
				case TYPE_f64 : ast->type = type_f64; break;
				default : {
				} return 0;
			}
		} break;
		case OPER_not : {		// '!'
			if (!lookup(s, ast->rhso, 0)) return 0;
			switch (typeId(ast->rhso)) {
				case TYPE_u32 : ast->type = type_u32; break;
				case TYPE_i32 : ast->type = type_i32; break;
				//~ case TYPE_u64 : ast->type = type_u64; break;
				case TYPE_i64 : ast->type = type_i64; break;
				case TYPE_f32 : ast->type = type_f32; break;
				case TYPE_f64 : ast->type = type_f64; break;
				default : {
				} return 0;
			}
		} break;
		case OPER_cmt : {		// '~'
			if (!lookup(s, ast->rhso, 0)) return 0;
			switch (typeId(ast->rhso)) {
				case TYPE_u32 : ast->type = type_i32; break;
				case TYPE_i32 : ast->type = type_i32; break;
				default : {
				} return 0;
			}
		} break;

		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : {		// '%'
			if (!(lht = lookup(s, ast->lhso, 0))) return 0;
			if (!(rht = lookup(s, ast->rhso, 0))) return 0;
			switch (ast->cast = castid(ast->lhso, ast->rhso)) {
				case TYPE_u32 : ast->type = type_u32; break;
				case TYPE_i32 : ast->type = type_i32; break;
				case TYPE_i64 : ast->type = type_i64; break;
				case TYPE_f32 : ast->type = type_f32; break;
				case TYPE_f64 : ast->type = type_f64; break;
				//~ case TYPE_cls : find OPER in (lhso OR rhso) then make it a call
			}
		} break;

		case OPER_neq : 		// '!='
		case OPER_equ : 		// '=='
		case OPER_gte : 		// '>'
		case OPER_geq : 		// '>='
		case OPER_lte : 		// '<'
		case OPER_leq : {		// '<='
			if (!lookup(s, ast->lhso, 0)) return 0;
			if (!lookup(s, ast->rhso, 0)) return 0;
			switch (ast->cast = castid(ast->lhso, ast->rhso)) {
				case TYPE_u32 :
				case TYPE_i32 :
				case TYPE_f32 :
				case TYPE_f64 : ast->type = type_i32; break;
			}
		} break;

		case OPER_shr : 		// '>>'
		case OPER_shl : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_xor : 		// '^'
		case OPER_ior : {		// '|'
			if (!lookup(s, ast->lhso, 0)) return 0;
			if (!lookup(s, ast->rhso, 0)) return 0;
			switch (castid(ast->lhso, ast->rhso)) {
				case TYPE_u32 : ast->type = type_u32; break;
				case TYPE_i32 : ast->type = type_i32; break;
				//~ default : {
					//~ error(s, s->file, ast->line, "cannot apply binary operator to (%T, %T):", ast->lhso->type, ast->rhso->type);
					//~ debug("cannot apply binary operator to (%T, %T):", ast->lhso->type, ast->rhso->type);
					//~ ast->type = 0;
				//~ } break;
			}
		}break;

		//~ case OPER_lnd : (dst, "&&"); break;
		//~ case OPER_lor : (dst, "||"); break;
		case OPER_sel : {		// '?:'
			if (!lookup(s, ast->test, 0)) return 0;
			if (!lookup(s, ast->lhso, 0)) return 0;
			if (!lookup(s, ast->rhso, 0)) return 0;
			switch (castid(ast->lhso, ast->rhso)) {
				case TYPE_u32 : ast->type = type_u32; break;
				case TYPE_i32 : ast->type = type_i32; break;
				case TYPE_i64 : ast->type = type_i64; break;
				case TYPE_f32 : ast->type = type_f32; break;
				case TYPE_f64 : ast->type = type_f64; break;
			}
		} break;

		case ASGN_set : {		// ':='
			if (!lookup(s, ast->lhso, 0)) return 0;
			if (!lookup(s, ast->rhso, 0)) return 0;
			if (!islval(ast->lhso)) {
				error(s, s->file, ast->line, "invalid lvalue on operator: '%k' `%+k`", ast, ast->lhso);
				break;
			}
			switch (typeId(ast->lhso)) {
				case TYPE_i32 :
				case TYPE_u32 :
				case TYPE_i64 :
				case TYPE_f32 :
				case TYPE_f64 : ast->type = ast->lhso->type; break;
			}
			if (typeId(ast->lhso) < castid(ast->lhso, ast->rhso))
				warn(s, 8, s->file, ast->line, "possible loss of precision: '%+k'", ast);
		} break;

		case TYPE_ref : {
			sym = loc ? loc->args : s->deft[ast->hash];
			for (; sym; sym = loc ? sym->next : sym->deft) {
				if (!sym->name) continue;
				//~ debug("lookup('%k' is '%T' in '%?T')", ast, sym, loc);
				if (strcmp(sym->name, ast->name) != 0) continue;
				if (arg && sym->kind == TYPE_fnc) {
					int argi = 0;
					node argv = arg;
					defn argT = sym->args;
					defn argt[TOKS], *argp = argt + TOKS;
					for (*--argp = 0; argv; ) {
						node arg = argv->kind == OPER_com ? argv->rhso : argv;
						argv = argv->kind == OPER_com ? argv->lhso : 0;
						*--argp = arg->type;
					}
					while ((argt[argi++] = *argp++));	// memcpy
					argp = argt;
					while (argT && *argp) {
						defn arg = *argp;
						defn typ = argT->type;
						if (arg && typ) {
							if (typ->kind != arg->kind) break;
						}
						argT = argT->next;
						argp = argp + 1;
					}
					if (argT || *argp) continue;
				}//*/
				break;
			}
			//~ debug("found('%k(%?+k)' is '%T' in '%?T')", ast, arg, sym, loc);
			switch (sym ? sym->kind : 0) {
				//~ case TYPE_vid :

				case TYPE_i08 :
				case TYPE_u08 :

				case TYPE_i16 :
				case TYPE_u16 :

				case TYPE_i32 :
				case TYPE_u32 :

				case TYPE_i64 :
				case TYPE_u64 :

				case TYPE_f32 :
				case TYPE_f64 : 

				case TYPE_enu :
				case TYPE_rec : 
				case TYPE_cls : {		// typename
					ast->kind = TYPE_ref;
					ast->type = sym;
					if (arg)
						arg->cast = sym->kind;
					ast->stmt = 0;
					ast->link = 0;
				} break;

				case CNST_int :
				case CNST_flt :
				case CNST_str :
				case TYPE_def : {
					ast->kind = TYPE_ref;
					ast->type = sym->type;
					ast->rhso = sym->init;
					ast->link = NULL;
					//~ ast->link = sym;
				} break;

				//~ case CNST_int :
				//~ case CNST_flt :
				//~ case CNST_str :			// constant
				//~ case TYPE_def :			// typename
				case TYPE_fnc : {		// function
					//~ int argc = 0;
					node argv = arg;
					defn argT = sym->args;
					node argt[TOKS], *argp = argt + TOKS;
					for (*--argp = 0; argv; ) {
						node arg = argv->kind == OPER_com ? argv->rhso : argv;
						argv = argv->kind == OPER_com ? argv->lhso : 0;
						*--argp = arg;//lookup(s, arg, 0);
					}
					while (argT && *argp) {
						node arg = *argp;
						arg->cast = argT->type->kind;
						argT = argT->next;
						argp = argp + 1;
					}

				} // fall
				case TYPE_ref : {		// variable
					ast->kind = TYPE_ref;
					ast->type = sym->type;
					ast->cast = sym->type->kind;
					//~ ast->rhso = sym->init;	// init|body
					ast->link = sym;
					sym->used = 1;
				} break;
				case EMIT_opc : {		// variable
					ast->kind = TYPE_ref;
					ast->type = sym->type;
					ast->cast = sym->type->kind;
					//~ ast->rhso = sym->init;	// init|body
					ast->link = sym;
					sym->used = 1;
				} break;
				default : {
					error(s, s->file, ast->line, "undeclared `%k`", ast);
					sym = declare(s, TOKN_err, ast, NULL, NULL);
					//~ sym->type = type_i32;
					//~ ast->kind = TYPE_ref;
					//~ ast->type = type_i32;
					//~ ast->link = sym;
					//~ ast->type = 0;
					//~ sym = 0;
				} return 0;
			}
		} break;
		case CNST_int : return ast->type = type_i32;	// 'int'
		case CNST_flt : return ast->type = type_f32;	// 'float'
		case CNST_str : return ast->type = type_str;	// "str"
		case EMIT_opc : return ast->type = type_vid;	// "str"

		default :
			fatal(s, "lookup of '%k':%04x:%s unimpl", ast, ast->kind, tok_tbl[ast->kind].name);	// this 
			return ast->type = 0;
	}

	if (!ast->type) {
		error(s, s->file, ast->line, "cast error (%T, %T):'%k'", ast->lhso ? ast->lhso->type : 0, ast->rhso ? ast->rhso->type : 0, ast);
		//~ error(s, s->file, ast->line, "cast error %+k", ast);
	}
	else if (!ast->cast) ast->cast = ast->type->kind;
	//~ trace(1, "lookup(%k is %T:[%d])", ast, ast->type, ast->type ? ast->type->size : -1);
	trace(1, "lookup(%k is %T:%s)", ast, ast->type, istype(ast) ? "TYPE" : "NOTY");
	return ptr->type = ast->type;
}

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

//}
