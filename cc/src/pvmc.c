#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "node.h"
#include "stmt.c"
#include "code.c"

enum {
	get_nil = 0x00000000,		// nothing
	get_val = 0x00000002,		// value
	get_ref = 0x00000003,		// reference

	got_ref = 0x00000003,		// in memory
	got_stn = 0x00000004,		// on stack
	got_val = 0x00000009,		// on stack
	//~ 5982
	//~ got_val = 0x00000004,		// value is on stack
	//~ got_ref = 0x00000004,		// reference addres is lhs
	//~ got_rsv = 0x00000004,		// reference is stack value
	//~ got_rsa = 0x00000004,		// reference addres on stack
};

int cgen(state s, tree ptr, int flags) {
	int lhsf = 0;
	symbol lhst,rhst;
	if (!ptr) return -1;
	switch (ptr->kind & tok_msk) {
		case tok_val : {
			//~ printf("load val(i) d(%d), f(%f), p(%p)\n", ptr->cint, ptr->cflt, ptr->cstr);
			if (flags == get_ref) {
				raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
				return 0;
			}
			else if (ptr->type == type_int) vm_casm(s, opc_ldc, ptr->cint);
			else if (ptr->type == type_flt) vm_casm(s, opc_ldc, argflt(ptr->cflt));
			else if (ptr->type && ptr->type->type == type_str) vm_casm(s, opc_ldc, argptr((void*)ptr->type->offs));
			else raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
		} return tok_val;
		case tok_idf : {
			if (!ptr->type || flags == get_nil) {
				raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
				return 0;
			}
			else if (flags == get_val) {
				if (ptr->type->kind & idr_mem) {
					//~ printf("load val(m) %s\n", ptr->type->name);
					vm_casm(s, opc_ldc4, ptr->type->offs);
					vm_casm(s, opc_ldi, ptr->type->type->size);
					return got_val;
				}
				else {
					//~ printf("load val(s) %s\n", ptr->type->name);
					vm_casm(s, opc_dup, s->vm.dcnt - ptr->type->offs);
					return got_val;
				}
			}
			else if (flags == get_ref) {
				if ((ptr->type->kind & idr_mem) == idr_mem) {
					//~ printf("load ref(m) %s\n", ptr->type->name);
					vm_casm(s, opc_ldc4, ptr->type->offs);
					return got_ref;
				}
				else {
					//~ printf("load ref(s) %s\n", ptr->type->name);
					return got_stn;
				}
			}
			else return 0;
		} break;
		case tok_opr : {										// break;
			switch (ptr->kind) {
				case opr_nop : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception - generating \"nop\">", __FILE__, __LINE__);
				} return 0;
				case opr_jmp : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception - generating \"jmp\">", __FILE__, __LINE__);
				} return 0;
				case opr_fnc : {
					symbol argv[256];//, argt[256];
					int argc = 0;
					node args;
					if ((args = ptr->orhs)) {
						while (args->kind == opr_com) {
							symbol type = nodetype(args->orhs);
							cgen(s, args->orhs, get_val);
							//~ if (argt[argc] && argt[argc] != type)...;
							argv[argc] = type;
							args = args->olhs;
							argc += 1;
						}
						argv[argc] = nodetype(args);
						cgen(s, args, get_val);
						argc += 1;
					}
					if (ptr->type == oper_out) {
						while (argc--) {
							symbol type = argv[argc];
							if (type == type_int)
								vm_casm(s, opc_iout, 0);
							else if (type == type_flt)
								vm_casm(s, opc_fout, 0);
							else if (type == type_str)
								vm_casm(s, opc_sout, 0);
							vm_casm(s, opc_pop, 1);
						}
					}
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> invalid funtion call(%s)", __FILE__, __LINE__, ptr->name);
				} return 0;
				case opr_idx : {
					int basesize=0;
					node basenode;
					symbol basetype;
					if (!ptr->type) {
						raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
						return -1;
					}
					for (basenode = ptr; basenode->kind == opr_idx; basenode = basenode->olhs);
					basetype = basenode->type;
					basenode = basetype->arrs;
					basesize = basetype->type->size;
					vm_casm(s, opc_ldc4, basetype->offs);
					while (ptr->kind == opr_idx) {
						symbol type = nodetype(ptr->orhs);
						lhsf = cgen(s, ptr->orhs, get_val);
						vm_casm(s, opc_ldc, basesize);
						if (type == type_int);
						else if (type == type_flt) vm_casm(s, opc_ftoi, 0);
						else raiseerror(s, 99, 0, "%s:%d: <unhandled exception> type: %s", __FILE__, __LINE__, basenode->name);
						vm_casm(s, opc_imad, 0);
						//~ vm_casm(s, opc_imul, 0);
						//~ vm_casm(s, opc_iadd, 0);
						basesize *= basenode->orhs->cint;
						basenode = basenode->olhs;
						ptr = ptr->olhs;
					}
					if (flags == get_ref) return got_ref;
					vm_casm(s, opc_ldi, basetype->type->size);
				} return got_val;
			}
			if (ptr->olhs) {									// only binary operators have lhs
				if ((ptr->kind & 0xf0) == 0x20) {				// assignment : =, +=, ... /?
					lhsf = cgen(s, ptr->olhs, get_ref);
					if (ptr->kind != ass_equ) {
						switch (lhsf) {
							case got_ref : {
								vm_casm(s, opc_dup, 0);
								if (flags) vm_casm(s, opc_dup, 0);
								vm_casm(s, opc_ldi4, ptr->olhs->type->offs);
							} break;
							case got_stn : vm_casm(s, opc_dup, s->vm.dcnt - ptr->olhs->type->offs); break;
							default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
						}
					}
					else switch (lhsf) {
						case got_stn : break;
						case got_ref : if (flags) vm_casm(s, opc_dup, 0); break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
					}
				}
				else {
					lhsf = cgen(s, ptr->olhs, get_val);
					lhst = nodetype(ptr->olhs);
					if (ptr->type != lhst) {
						if (ptr->type == type_int && lhst == type_flt) {
							vm_casm(s, opc_ftoi, 0);
						}
						else if (ptr->type == type_flt && lhst == type_int) {
							vm_casm(s, opc_itof, 0);
						}
						else raiseerror(s, 999, 0, "%s:%d: <unhandled exception> OP %x %s", __FILE__, __LINE__, ptr->kind, lhst->name);
					}
				}
			}
			else {
				//~ raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			}
			cgen(s, ptr->orhs, get_val);
			rhst = nodetype(ptr->orhs);
			if (ptr->type != rhst) {
				if (ptr->type == type_int && rhst == type_flt) {
					vm_casm(s, opc_ftoi, 0);
				}
				else if (ptr->type == type_flt && rhst == type_int) {
					vm_casm(s, opc_itof, 0);
				}
			}
			switch (ptr->kind) {
				default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind); break;
				case opr_idx : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_fnc : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_tpc : break;
				case opr_dot : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_unm : {
					if (ptr->type == type_int)
						vm_casm(s, opc_ineg, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fneg, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_unp : {
					if (ptr->type == type_int);
					else if (ptr->type == type_flt);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_not : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_neg : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_mul : {
					if (ptr->type == type_int)
						vm_casm(s, opc_imul, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fmul, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_div : {
					if (ptr->type == type_int)
						vm_casm(s, opc_idiv, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fdiv, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_mod : {
					if (ptr->type == type_int)
						vm_casm(s, opc_imod, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fmod, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_add : {
					if (ptr->type == type_int)
						vm_casm(s, opc_iadd, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fadd, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_sub : {
					if (ptr->type == type_int)
						vm_casm(s, opc_isub, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fsub, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_shr : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_shl : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_gte : {
					if (ptr->type == type_int)
						vm_casm(s, opc_icgt, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fcgt, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_geq : {
					if (ptr->type == type_int)
						vm_casm(s, opc_icge, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fcge, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_lte : {
					if (ptr->type == type_int)
						vm_casm(s, opc_iclt, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fclt, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_leq : {
					if (ptr->type == type_int)
						vm_casm(s, opc_icle, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fcle, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_neq : {
					if (ptr->type == type_int)
						vm_casm(s, opc_icne, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fcne, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_equ : {
					if (ptr->type == type_int)
						vm_casm(s, opc_iceq, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fceq, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_and : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_xor : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_or  : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_lnd : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_lor : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case opr_ite : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case opr_els : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;

				case ass_equ : switch (lhsf) {
					case got_ref : {
						vm_casm(s, opc_sti, ptr->type->size);
						if (flags) vm_casm(s, opc_ldi, ptr->type->size);
					} break;
					case got_stn : {
						if (flags) vm_casm(s, opc_dup, 0);
						vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
					} break;
					default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
				} break;
				case ass_add : {
					if (ptr->type == type_int)
						vm_casm(s, opc_iadd, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fadd, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_sub : {
					if (ptr->type == type_int)
						vm_casm(s, opc_isub, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fsub, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_mul : {
					if (ptr->type == type_int)
						vm_casm(s, opc_imul, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fmul, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_div : {
					if (ptr->type == type_int)
						vm_casm(s, opc_idiv, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fdiv, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_mod : {
					if (ptr->type == type_int)
						vm_casm(s, opc_imod, 0);
					else if (ptr->type == type_flt)
						vm_casm(s, opc_fmod, 0);
					else raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_shl : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					//~ vm_casm(s, opc_shl, 0);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_shr : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					//~ vm_casm(s, opc_shr, 0);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_or  : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					//~ vm_casm(s, opc_or, 0);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_and : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					//~ vm_casm(s, opc_and, 0);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
				case ass_xor : {raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					//~ vm_casm(s, opc_xor, 0);
					switch (lhsf) {
						case got_ref : {
							vm_casm(s, opc_sti, ptr->type->size);
							if (flags) vm_casm(s, opc_ldi, ptr->type->size);
						} break;
						case got_stn : {
							if (flags) vm_casm(s, opc_dup, 0);
							vm_casm(s, opc_set, s->vm.dcnt - ptr->olhs->type->offs);
						} break;
						default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OP %x", __FILE__, __LINE__, ptr->kind);
					}
				} break;
			}
		} break;
	}
	return flags;
}

int cc_comp(state s, char* file) {
	node tmp;
	llist list;
	if (cc_open(s, file)) {
		raiseerror(s, -9, 0, "file open error '%s'", file);
		return -1;
	}
	while ((tmp = next_token(s))) {
		back_token(s, tmp);
		stmt(s);
		if (s->errc) return -1;
	}
	if (cc_done(s)) return -2;
	s->data = 256;
	for (list = s->tbeg; list; list = list->next) {		// data : initialized??
		symbol defn = (symbol)list->data;
		if (!defn || (defn->kind & tok_msk) != tok_val) {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			return -3;
		}
		else if (defn->type == type_str) {
			defn->offs = s->data;
			s->data += (strlen(defn->name)+1+3) & ~3;
			//~ printf("%d, %d: %s\n", defn->offs, defn->size, defn->name);
		}
		else {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			return -2;
		}
	}
	for (list = s->dbeg; list; list = list->next) {		// data : uninitialized
		symbol defn = (symbol)list->data;
		if (!defn || (defn->kind & tok_msk) != tok_idf) {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
		}
		else switch (defn->kind & idf_msk) {
			default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__); break;
			case idf_ref : {
				//~ *(int*)(s->buffer + s->data) = 0;
				defn->offs = s->data;
				s->data += ((typesize(defn)+3) & ~3);
			} break;
		}
	}
	if (s->buffp < s->buffer+s->data) {
		s->buffp = s->buffer + s->data;
	}
	if ((s->buffp + 65536) < (s->buffer + bufmaxlen)) {	// init : machine
		char* jmps[65536];
		if (s->jmps > 65536) {
			raiseerror(s, -15, 0, "too many jumps");
			return -66;
		}
		memset(jmps, 0, sizeof(jmps));
		s->vm.cbeg = s->vm.cend = s->buffp;
		for (list = s->cbeg; list; list = list->next) {		// code : generate it
			node expr = (node)list->data;
			int dcnt = list->flgs;
			if (s->vm.dcnt > dcnt) {						// data : popn
				vm_casm(s, opc_pop, s->vm.dcnt - dcnt);
			}
			else {
				while (s->vm.dcnt < dcnt) {				// data : push
					vm_casm(s, opc_ldc, 0);
				}
				if (s->vm.dcnt >= 255) {
					raiseerror(s, 99, 0, "%s:%d: <unhandled exception> (%d-%d)", __FILE__, __LINE__,list->flgs, s->vm.dcnt);
				}
			}
			switch (expr->kind) {
				default : list->data = 0; break;
				case opr_nop : {
					jmps[expr->line] = s->vm.cend;
					list->data = 0;
					expr = 0;
				} break;
				case opr_jmp : {
					if (expr->type == stmt_fork) {
						if (expr->orhs) {
							list->flgs = expr->orhs->line;
							list->data = s->vm.cend;
							vm_casm(s, opc_fork, 0);
						}
						else {
							list->data = 0;
							vm_casm(s, opc_wait, 0);
						}
					}
					else if (expr->olhs) {
						cgen(s, expr->olhs, get_val);
						list->flgs = expr->orhs->line;
						list->data = s->vm.cend;
						vm_casm(s, opc_jz, 0);
					}
					else {
						list->flgs = expr->orhs->line;
						list->data = s->vm.cend;
						vm_casm(s, opc_jmp, 0);
					}
					expr = 0;
				} break;
			}
			cgen(s, expr, 0);
			if (s->errc) return -1;
			if (dcnt != s->vm.dcnt) {
				printf("sp+%d : ", list->flgs); walk(expr, 0, 0); printf("\n");
				raiseerror(s, 5, expr->line, "statement with no effect");
				//~ raiseerror(s, 999, 0, "%s:%d: <unhandled exception> (%d-%d)", __FILE__, __LINE__,list->flgs, s->vm.dcnt);
				//~ (s=0)+1;
			}
		}
		for (list = s->cbeg; list; list = list->next) {		// code : fix jumps
			if (list->data) {
				if(vm_jset(list->data, jmps[list->flgs])){
					raiseerror(s, -15, 0, "innternal error vm_jset failed");
					return -8;
				}
			}
		}
		//~ vm_casm(s, opc_nop, 0);
	}
	else {
		raiseerror(s, -15, 0, "innternal error Memory overrun");
		return -2;
	}
	for (list = s->tbeg; list; list = list->next) {			// data : initialized??
		symbol defn = (symbol)list->data;
		if (!defn || (defn->kind & tok_msk) != tok_val) {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			return -3;
		}
		else if (defn->type == type_str) {
			strcpy(s->buffer + defn->offs, defn->name);
			//~ printf("%d : %s\n", defn->offs, s->buffer + defn->offs);
		}
		else {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			return -2;
		}
	}
	return 0;
}

struct state_t s;

int main(int argc, char *argv[]) {
	#define C (1<<1)
	#define D (1<<2)
	#define I (1<<3)
	#define O (1<<4)
	#define H (1<<5)
	int i, pcs = 0, opt = H;
	char *src = 0, *out = 0;
	if (argc <= 2) {								// help
		printf("usage : %s [options] file...\n", argv[0]);
		printf("options: -c <file> compile file\n");
		printf("\t -e[pcs] [<code>] execute the compiled or code file\n");
		printf("\t  where [pcs] is the number of processors used in execution\n");
		printf("\t  and [<code>] is the exec file if no compilation happens\n");
		printf("\t -o <file> set output file for (dis)assembled code\n");
		printf("\t -d disassamble\n");
		printf("\t -i information\n");
		return 0;
	}
	for (i = 1; i < argc; i++) {
		if (*argv[i] == '-') {
			switch(argv[i][1]) {
				case 'e' : {
					if(!(pcs = atoi(argv[i]+2))) pcs = 1;
					src = src ? src : argv[i+=1];
				} break;
				case 'd' : opt |= D; break;
				case 'i' : opt |= I; break;
				case 'c' : opt |= C; src = argv[i+=1]; break;
				case 'o' : opt |= O; out = argv[i+=1]; break;
				default  : raiseerror(&s, -1, 0, "invalid option '%s'", argv[i]);
			}
		}
		else raiseerror(&s, -1, 0, "invalid argument '%s'", argv[i]);
	}
	if ((opt & C)/* && src*/) {						// compile
		if (cc_comp(&s, src)) return -1;
		if (opt & D) vm_dasm(&s, out);
		else if (out && vm_dump(&s, out) != 0) {
			raiseerror(&s, -1, 0, "output faild");
		}
		if (pcs) vm_exec(&s, pcs);
	}
	else if (pcs) {
		if (vm_open(&s, src)) return -2;
		if (opt & D) vm_dasm(&s, out);
		else if (out && vm_dump(&s, out) != 0) {
			raiseerror(&s, -1, 0, "output faild");
		}
		vm_exec(&s, pcs);
	}
	else return 0;
	if (opt & I) vm_info(&s);
	return 0;
	return 0;
}
