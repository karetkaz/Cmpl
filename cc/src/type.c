#include <string.h>
#include <stdio.h>
#include <math.h>

#include "node.h"

symbol stmt_if;
symbol stmt_for;
symbol stmt_fork;
//~ symbol stmt_ret;

symbol type_int;
symbol type_str;
symbol type_flt;
symbol type_void;
symbol oper_sof;
symbol oper_out;

const unsigned crc_tab[] = {
	0x00000000,
	0x77073096,
	0xEE0E612C,
	0x990951BA,
	0x076DC419,
	0x706AF48F,
	0xE963A535,
	0x9E6495A3,
	0x0EDB8832,
	0x79DCB8A4,
	0xE0D5E91E,
	0x97D2D988,
	0x09B64C2B,
	0x7EB17CBD,
	0xE7B82D07,
	0x90BF1D91,
	0x1DB71064,
	0x6AB020F2,
	0xF3B97148,
	0x84BE41DE,
	0x1ADAD47D,
	0x6DDDE4EB,
	0xF4D4B551,
	0x83D385C7,
	0x136C9856,
	0x646BA8C0,
	0xFD62F97A,
	0x8A65C9EC,
	0x14015C4F,
	0x63066CD9,
	0xFA0F3D63,
	0x8D080DF5,
	0x3B6E20C8,
	0x4C69105E,
	0xD56041E4,
	0xA2677172,
	0x3C03E4D1,
	0x4B04D447,
	0xD20D85FD,
	0xA50AB56B,
	0x35B5A8FA,
	0x42B2986C,
	0xDBBBC9D6,
	0xACBCF940,
	0x32D86CE3,
	0x45DF5C75,
	0xDCD60DCF,
	0xABD13D59,
	0x26D930AC,
	0x51DE003A,
	0xC8D75180,
	0xBFD06116,
	0x21B4F4B5,
	0x56B3C423,
	0xCFBA9599,
	0xB8BDA50F,
	0x2802B89E,
	0x5F058808,
	0xC60CD9B2,
	0xB10BE924,
	0x2F6F7C87,
	0x58684C11,
	0xC1611DAB,
	0xB6662D3D,
	0x76DC4190,
	0x01DB7106,
	0x98D220BC,
	0xEFD5102A,
	0x71B18589,
	0x06B6B51F,
	0x9FBFE4A5,
	0xE8B8D433,
	0x7807C9A2,
	0x0F00F934,
	0x9609A88E,
	0xE10E9818,
	0x7F6A0DBB,
	0x086D3D2D,
	0x91646C97,
	0xE6635C01,
	0x6B6B51F4,
	0x1C6C6162,
	0x856530D8,
	0xF262004E,
	0x6C0695ED,
	0x1B01A57B,
	0x8208F4C1,
	0xF50FC457,
	0x65B0D9C6,
	0x12B7E950,
	0x8BBEB8EA,
	0xFCB9887C,
	0x62DD1DDF,
	0x15DA2D49,
	0x8CD37CF3,
	0xFBD44C65,
	0x4DB26158,
	0x3AB551CE,
	0xA3BC0074,
	0xD4BB30E2,
	0x4ADFA541,
	0x3DD895D7,
	0xA4D1C46D,
	0xD3D6F4FB,
	0x4369E96A,
	0x346ED9FC,
	0xAD678846,
	0xDA60B8D0,
	0x44042D73,
	0x33031DE5,
	0xAA0A4C5F,
	0xDD0D7CC9,
	0x5005713C,
	0x270241AA,
	0xBE0B1010,
	0xC90C2086,
	0x5768B525,
	0x206F85B3,
	0xB966D409,
	0xCE61E49F,
	0x5EDEF90E,
	0x29D9C998,
	0xB0D09822,
	0xC7D7A8B4,
	0x59B33D17,
	0x2EB40D81,
	0xB7BD5C3B,
	0xC0BA6CAD,
	0xEDB88320,
	0x9ABFB3B6,
	0x03B6E20C,
	0x74B1D29A,
	0xEAD54739,
	0x9DD277AF,
	0x04DB2615,
	0x73DC1683,
	0xE3630B12,
	0x94643B84,
	0x0D6D6A3E,
	0x7A6A5AA8,
	0xE40ECF0B,
	0x9309FF9D,
	0x0A00AE27,
	0x7D079EB1,
	0xF00F9344,
	0x8708A3D2,
	0x1E01F268,
	0x6906C2FE,
	0xF762575D,
	0x806567CB,
	0x196C3671,
	0x6E6B06E7,
	0xFED41B76,
	0x89D32BE0,
	0x10DA7A5A,
	0x67DD4ACC,
	0xF9B9DF6F,
	0x8EBEEFF9,
	0x17B7BE43,
	0x60B08ED5,
	0xD6D6A3E8,
	0xA1D1937E,
	0x38D8C2C4,
	0x4FDFF252,
	0xD1BB67F1,
	0xA6BC5767,
	0x3FB506DD,
	0x48B2364B,
	0xD80D2BDA,
	0xAF0A1B4C,
	0x36034AF6,
	0x41047A60,
	0xDF60EFC3,
	0xA867DF55,
	0x316E8EEF,
	0x4669BE79,
	0xCB61B38C,
	0xBC66831A,
	0x256FD2A0,
	0x5268E236,
	0xCC0C7795,
	0xBB0B4703,
	0x220216B9,
	0x5505262F,
	0xC5BA3BBE,
	0xB2BD0B28,
	0x2BB45A92,
	0x5CB36A04,
	0xC2D7FFA7,
	0xB5D0CF31,
	0x2CD99E8B,
	0x5BDEAE1D,
	0x9B64C2B0,
	0xEC63F226,
	0x756AA39C,
	0x026D930A,
	0x9C0906A9,
	0xEB0E363F,
	0x72076785,
	0x05005713,
	0x95BF4A82,
	0xE2B87A14,
	0x7BB12BAE,
	0x0CB61B38,
	0x92D28E9B,
	0xE5D5BE0D,
	0x7CDCEFB7,
	0x0BDBDF21,
	0x86D3D2D4,
	0xF1D4E242,
	0x68DDB3F8,
	0x1FDA836E,
	0x81BE16CD,
	0xF6B9265B,
	0x6FB077E1,
	0x18B74777,
	0x88085AE6,
	0xFF0F6A70,
	0x66063BCA,
	0x11010B5C,
	0x8F659EFF,
	0xF862AE69,
	0x616BFFD3,
	0x166CCF45,
	0xA00AE278,
	0xD70DD2EE,
	0x4E048354,
	0x3903B3C2,
	0xA7672661,
	0xD06016F7,
	0x4969474D,
	0x3E6E77DB,
	0xAED16A4A,
	0xD9D65ADC,
	0x40DF0B66,
	0x37D83BF0,
	0xA9BCAE53,
	0xDEBB9EC5,
	0x47B2CF7F,
	0x30B5FFE9,
	0xBDBDF21C,
	0xCABAC28A,
	0x53B39330,
	0x24B4A3A6,
	0xBAD03605,
	0xCDD70693,
	0x54DE5729,
	0x23D967BF,
	0xB3667A2E,
	0xC4614AB8,
	0x5D681B02,
	0x2A6F2B94,
	0xB40BBE37,
	0xC30C8EA1,
	0x5A05DF1B,
	0x2D02EF8D
};

unsigned rehash(char *name) {
	register unsigned hashval = 0xffffffff;
	register const char *buff = name;
	while (*buff)
		hashval = (hashval >> 8) ^ crc_tab[(hashval ^ *buff++) & 0xff];
	return (hashval ^ 0xffffffff);
}

char* lookupstr(state s, char *name, int slen, unsigned hash) {
	struct llist_t *list = NULL, *next;
	hash %= MAX_TBLS;
	for (next = s->strt[hash]; next; next = next->next) {
		register int c = strcmp((char*)next->data, name);
		if (c == 0) return (char*)next->data;
		if (c > 0) break;
		list = next;
	}
	if (slen) {
		if (name != s->buffp)
			strncpy(s->buffp, name, slen);
		s->buffp += slen + 1;
	}
	if (!list) {
		list = s->strt[hash] = (struct llist_t *)s->buffp;
		s->buffp += sizeof(struct llist_t);
	}
	else {
		list = list->next = (struct llist_t *)s->buffp;
		s->buffp += sizeof(struct llist_t);
	}
	list->data = name;
	list->next = next;
	return name;
}

symbol lookupdef(state s, char *name, int level, unsigned hash) {
	symbol type = NULL;
	llist list;
	hash %= MAX_TBLS;
	for(list = s->deft[hash]; list; list = list->next) {
		type = (symbol)list->data;
		if (type->dlev > level) continue;
		if (strcmp (type->name, name) == 0) break;
	}
	return list ? type : NULL;
}

symbol install(state s, symbol type, char* name, int size, int kind) {
	llist list;
	symbol defn = type;
	unsigned hash = rehash(name);
	name = lookupstr(s, name, 0, hash);
	defn = (struct symbol_t*)(s->buffp);
	s->buffp += sizeof(struct token_t);
	defn->kind = kind;
	defn->dlev = s->level;
	defn->type = type;
	defn->name = name;
	defn->size = size;
	if ((kind & tok_msk) == tok_idf) {				// insert into definition table
		hash %= MAX_TBLS;
		list = (struct llist_t*)(s->buffp);
		s->buffp += sizeof(struct llist_t);
		list->data = defn;
		list->next = s->deft[hash];
		s->deft[hash] = list;
		switch (kind & idf_msk) {
			default : return defn;
			case idf_ref : {
				//~ if ((kind & idm_con) == idm_con) {		// constant, do nothing
					//~ return defn;
				//~ }
				if (s->level == 0) {				// global
					defn->kind |= idr_mem;
				}
				else if ((kind & idm_sta) == idm_sta) {	// variable, global, array, place it memeory
					defn->kind |= idr_mem;
					//~ kind = 2;
				}
				else if ((kind & idm_vol) == idm_vol) {	// variable, global, array, place it memeory
					defn->kind |= idr_mem;
					//~ kind = 2;
				}
				else {
					s->data += 1;
					defn->offs = s->data;
					return defn;						// variable, local, place it on the stack
				}
			} break;
		}
		list = (struct llist_t*)(s->buffp);
		s->buffp += sizeof(struct llist_t);
		list->data = defn;
		list->next = 0;
		if (s->dend) {
			s->dend->next = list;
		}
		else {
			s->dbeg = list;
		}
		s->dend = list;
	}
	else if ((kind & tok_msk) == tok_val) {
		if (type != type_str) {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>(insert value !cstr)", __FILE__, __LINE__);
			return defn;
		}
		list = (struct llist_t*)(s->buffp);
		s->buffp += sizeof(struct llist_t);
		list->data = defn;
		list->next = 0;
		if (s->tend) {
			s->tend->next = list;
		}
		else {
			s->tbeg = list;
		}
		s->tend = list;

	}
	if (kind == 2) {								// insert into globaldata list
		llist list = (struct llist_t*)(s->buffp);
		s->buffp += sizeof(struct llist_t);
		list->data = defn;
		list->next = 0;
		if (s->dend) {
			s->dend->next = list;
		}
		else {
			s->dbeg = list;
		}
		s->dend = list;
	}
	return defn;
}

void uninst(state s, int level) {
	int i;
	for (i = 0; i < MAX_TBLS; i+=1) {
		struct llist_t *list = s->deft[i];
		while (list) {
			symbol defn = (symbol)list->data;
			if (defn->dlev < level) break;
			if ((defn->kind & (tok_msk|idf_msk)) == idf_ref) {
				if ((defn->kind & 0xff) == 0) {
					//~ raiseerror(s, 999, 0, "%s:%d: uninst local", __FILE__, __LINE__);
					s->data -= 1;
				}
			}
			list = list->next;
		}
		s->deft[i] = list;
	}
}

symbol nodetype(node n) {
	if (!n || !n->type) return 0;
	switch (n->kind & tok_msk) {
		default : return 0;
		case tok_val : {						// constant
			if (n->type->type == type_str) return type_str;
		} return n->type;
		case tok_opr : return n->type;			// operator, function ???
		case tok_idf : {
			if (!n->type) return 0;
			//~ if ((n->type->kind & tok_msk) != tok_idf) return 0;
			switch (n->type->kind & idf_msk) {	// typename, function ???
				default : return 0;
				//~ case tok_def : return 0;
				case idf_kwd : return 0;
				case idf_dcl : return 0;// n->type;
				case idf_ref : {
					if (n->type->arrs) return 0;	// type_arr;
					return n->type->type;
				} break;
			}
		} break;
	}
	return 0;
}

/*char* nodename(node n) {
	if (!n) return 0;
	switch (n->kind & tok_msk) {
		default : return 0;
		case tok_val : return "constant";						// constant
		case tok_opr : return "operator";						// operator, function ???
		case tok_idf : switch (n->kind & idf_msk) {				// typename, function ???
			case tok_idf : return n->name;
			case idf_kwd : return 0;
			case idf_dcl : return 0;
			case idf_ref : return 0;
		} break;
	}
	return 0;
}//*/

unsigned typesize(symbol n) {
	if (!n) return 0;
	if((n->kind & tok_msk) != tok_idf) return 0;
	switch (n->kind & idf_msk) {					// typename, function ???
		default : return 0;
		//~ case tok_def : return 0;
		//~ case idf_kwd : return 0;
		case idf_dcl : return n->size;
		case idf_ref : {
			unsigned size;
			node arr = n->arrs;
			if (!n->type) return 0;					// typeless reference ?
			size = n->type->size;
			while (arr && arr->kind == opr_idx) {
				size = (arr->orhs ? (size * arr->orhs->cint) : n->type->size);
				arr = arr->olhs;
			}
			return size;
		} break;
	}
	return 0;
}

unsigned nodesize(node n) {
	if (!n) return 0;
	switch (n->kind & tok_msk) {
		default : return 0;
		case tok_val : return n->type ? n->type->size : 0;		// constant(4)
		case tok_opr : return typesize(n->type);			// operator, function ???
		case tok_idf : return typesize(n->type);
	}
	return 0;
}

symbol cast(state s, node opr, node lhs, node rhs) {
	int cast = 0;
	symbol type = NULL;
	if (!opr) return NULL;
	if (opr->kind & otf_unf) {
		if (!rhs) return NULL;
		if (nodetype(rhs) == type_int) cast = 0x0001;
		else if (nodetype(rhs) == type_flt) cast = 0x0002;
	}
	else {
		if (!lhs) return NULL;
		if (nodetype(lhs) == type_int) {
			if (nodetype(rhs) == type_int) {
				cast = 0x0101;
			}
			else if (nodetype(rhs) == type_flt) {
				cast = 0x0102;
			}
		}
		else if (nodetype(lhs) == type_flt) {
			if (nodetype(rhs) == type_int) {
				cast = 0x0201;
			}
			else if (nodetype(rhs) == type_flt) {
				cast = 0x0202;
			}
		}
	}
	switch (opr->kind) {
		case opr_idx : {
			int nas = 1, tas = 0;
			if (nodetype(rhs) != type_int) {
				raiseerror(s, -1, opr->line, "integer type expected in index operator");
				return 0;
			}
			while (lhs && lhs->kind == opr_idx) {
				lhs = lhs->olhs; nas += 1;
			}
			if ((type = lhs->type)) {
				lhs = lhs->type->arrs;
				while (lhs && lhs->kind == opr_idx) {
					lhs = lhs->olhs; tas += 1;
				}
				if (nas == tas) return type->type;
				if (nas > tas) {
					raiseerror(s, -1, opr->line, "Subscript on non-array");
					return 0;
				}
			}
			else return 0;
		} break;
		case opr_unm : switch (cast) {
			case 0x0001 : type = type_int; break;
			case 0x0002 : type = type_flt; break;
		} break;
		case opr_unp : switch (cast) {
			case 0x0001 : type = type_int; break;
			case 0x0002 : type = type_flt; break;
		} break;
		case opr_not : switch (cast) {
			case 0x0001 : type = type_int; break;
			case 0x0002 : {
				raiseerror(s, -1, opr->line, "wrong type argument to bit-complement");
			} return 0;
		} break;
		case opr_neg : switch (cast) {
			case 0x0001 : type = type_int; break;
			case 0x0002 : type = type_int; break;
		} break;

		case opr_mul : 
		case opr_div : 
		case opr_mod : 

		case opr_add : 
		case opr_sub : switch (cast) {
			case 0x0101 : type = type_int; break;
			case 0x0102 : type = type_flt; break;
			case 0x0201 : type = type_flt; break;
			case 0x0202 : type = type_flt; break;
		} break;

		case opr_shr : 
		case opr_shl : switch (cast) {
			case 0x0101 : type = type_int; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : {
				raiseerror(s, -1, opr->line, "invalid operands to binary <<");
			} return 0;
		} break;

		case opr_gte : 
		case opr_geq : 
		case opr_lte : 
		case opr_leq : 

		case opr_equ : 
		case opr_neq : switch (cast) {
			case 0x0101 : type = type_int; break;
			case 0x0102 : type = type_flt; break;
			case 0x0201 : type = type_flt; break;
			case 0x0202 : type = type_flt; break;
		} break;

		case opr_and : switch (cast) {
			case 0x0101 : type = type_int; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : {
				raiseerror(s, -1, opr->line, "invalid operands to binary &");
			} return 0;
		} break;
		case opr_xor : switch (cast) {
			case 0x0101 : type = type_int; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : {
				raiseerror(s, -1, opr->line, "invalid operands to binary ^");
			} return 0;
		} break;
		case  opr_or : switch (cast) {
			case 0x0101 : type = type_int; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : {
				raiseerror(s, -1, opr->line, "invalid operands to binary |"); break;
			} return 0;
		} break;
		case ass_equ : 
		case ass_mul : 
		case ass_div :
		case ass_mod :
		case ass_add :
		case ass_sub : {
			if (lhs->kind != tok_idf && lhs->kind != opr_idx) {
				raiseerror(s, -1, opr->line, "invalid left value in assignment");
				return 0;
			}
			if (lhs->kind == tok_idf && lhs->type && (lhs->type->kind & idm_con)==idm_con) {
				raiseerror(s, -1, opr->line, "invalid constant left value in assignment");
				return 0;
			}
			switch (cast) {
				case 0x0101 :
				case 0x0102 : type = type_int; break;
				case 0x0201 :
				case 0x0202 : type = type_flt; break;
			} break;
		} break;
		case ass_shr :
		case ass_shl :
		case ass_and :
		case ass_xor :
		case  ass_or : {
			if (lhs->kind != tok_idf && lhs->kind != opr_idx) {
				raiseerror(s, -1, opr->line, "invalid left value in assignment");
				return 0;
			}
			if (lhs->kind == tok_idf && lhs->type && (lhs->type->kind & idm_con)==idm_con) {
				raiseerror(s, -1, opr->line, "invalid constant left value in assignment");
				return 0;
			}
			switch (cast) {
				case 0x0101 : type = type_int; break;
				case 0x0102 : 
				case 0x0201 : 
				case 0x0202 : {
					raiseerror(s, -1, opr->line, "invalid operands to binary |");
				} return 0;
			} break;
		} break;
		case opr_com : return 0;
	}
	if (!type) {
		raiseerror(s, -1, opr->line, "invalid operands");
	}
	/*else {							// constant fold
		if (opr->kind & otf_unf) {
			if (rhs && rhs->kind == tok_val) cast = 0x0001;
			//~ if (rhs && rhs->kind == type_int) cast = 0x0001;
			else if (nodetype(rhs) == type_flt) cast = 0x0002;
		}
		else {
			if (!lhs) return NULL;
			if (nodetype(lhs) == type_int) {
				if (nodetype(rhs) == type_int) {
					cast = 0x0101;
				}
				else if (nodetype(rhs) == type_flt) {
					cast = 0x0102;
				}
			}
			else if (nodetype(lhs) == type_flt) {
				if (nodetype(rhs) == type_int) {
					cast = 0x0201;
				}
				else if (nodetype(rhs) == type_flt) {
					cast = 0x0202;
				}
			}
		}
		//~ if ()
		//~ fold???
	}*/
	return type;
}

void fold(state s, node opr, node lhs, node rhs) {		// constant folding
	int cast = 0;
	symbol type = NULL;
	if (!opr || (opr->kind & tok_msk) != tok_opr) return;
	if (!rhs || (rhs->kind & tok_msk) != tok_val) return;
	if (opr->kind & otf_unf) {
		if (rhs->type == type_int) cast = 0x0001;
		else if (rhs->type == type_flt) cast = 0x0002;
	}
	else {
		if (!lhs || (lhs->kind & tok_msk) != tok_val) return;
		if (opr->kind & opf_rla) {
			if (lhs->type == type_int) cast = 0x0100;
			else if (lhs->type == type_flt) cast = 0x0200;
		}
		if (lhs->type == type_int) {
			if (rhs->type == type_int) {
				cast = 0x0101;
			}
			else if (rhs->type == type_flt) {
				cast = 0x0102;
			}
		}
		else if (lhs->type == type_flt) {
			if (rhs->type == type_int) {
				cast = 0x0201;
			}
			else if (rhs->type == type_flt) {
				cast = 0x0202;
			}
		}
	}
	switch (opr->kind) {
		default		 : return ;
		case opr_unm : switch (cast) {
			case 0x0001 : type = type_int; opr->cint = -rhs->cint; break;
			case 0x0002 : type = type_flt; opr->cflt = -rhs->cflt; break;
		} break;
		case opr_unp : switch (cast) {
			case 0x0001 : type = type_int; opr->cint = rhs->cint; break;
			case 0x0002 : type = type_flt; opr->cflt = rhs->cflt; break;
		} break;
		case opr_not : switch (cast) {
			case 0x0001 : type = type_int; opr->cint = ~rhs->cint; break;
			case 0x0002 : raiseerror(s, -1, opr->line, "wrong type argument to bit-complement"); break;
		} break;
		case opr_neg : switch (cast) {
			case 0x0001 : type = type_int; opr->cint = !rhs->cint; break;
			case 0x0002 : type = type_int; opr->cint = !rhs->cflt; break;
		} break;

		case opr_mul : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint * rhs->cint; break;
			case 0x0102 : type = type_flt; opr->cflt = lhs->cint * rhs->cflt; break;
			case 0x0201 : type = type_flt; opr->cflt = lhs->cflt * rhs->cint; break;
			case 0x0202 : type = type_flt; opr->cflt = lhs->cflt * rhs->cflt; break;
		} break;
		case opr_div : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint / rhs->cint; break;
			case 0x0102 : type = type_flt; opr->cflt = lhs->cint / rhs->cflt; break;
			case 0x0201 : type = type_flt; opr->cflt = lhs->cflt / rhs->cint; break;
			case 0x0202 : type = type_flt; opr->cflt = lhs->cflt / rhs->cflt; break;
		} break;
		case opr_mod : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint % rhs->cint; break;
			case 0x0102 : type = type_flt; opr->cflt = fmod(lhs->cint, rhs->cflt); break;
			case 0x0201 : type = type_flt; opr->cflt = fmod(lhs->cflt, rhs->cint); break;
			case 0x0202 : type = type_flt; opr->cflt = fmod(lhs->cflt, rhs->cflt); break;
		} break;

		case opr_add : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint + rhs->cint; break;
			case 0x0102 : type = type_flt; opr->cflt = lhs->cint + rhs->cflt; break;
			case 0x0201 : type = type_flt; opr->cflt = lhs->cflt + rhs->cint; break;
			case 0x0202 : type = type_flt; opr->cflt = lhs->cflt + rhs->cflt; break;
		} break;
		case opr_sub : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint - rhs->cint; break;
			case 0x0102 : type = type_flt; opr->cflt = lhs->cint - rhs->cflt; break;
			case 0x0201 : type = type_flt; opr->cflt = lhs->cflt - rhs->cint; break;
			case 0x0202 : type = type_flt; opr->cflt = lhs->cflt - rhs->cflt; break;
		} break;

		case opr_shr : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint >> rhs->cint; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : raiseerror(s, -1, opr->line, "invalid operands to binary >>"); break;
		} break;
		case opr_shl : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint << rhs->cint; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : raiseerror(s, -1, opr->line, "invalid operands to binary <<"); break;
		} break;

		case opr_gte : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint > rhs->cint; break;
			case 0x0102 : type = type_int; opr->cint = lhs->cint > rhs->cflt; break;
			case 0x0201 : type = type_int; opr->cint = lhs->cflt > rhs->cint; break;
			case 0x0202 : type = type_int; opr->cint = lhs->cflt > rhs->cflt; break;
		} break;
		case opr_geq : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint >= rhs->cint; break;
			case 0x0102 : type = type_int; opr->cint = lhs->cint >= rhs->cflt; break;
			case 0x0201 : type = type_int; opr->cint = lhs->cflt >= rhs->cint; break;
			case 0x0202 : type = type_int; opr->cint = lhs->cflt >= rhs->cflt; break;
		} break;
		case opr_lte : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint < rhs->cint; break;
			case 0x0102 : type = type_int; opr->cint = lhs->cint < rhs->cflt; break;
			case 0x0201 : type = type_int; opr->cint = lhs->cflt < rhs->cint; break;
			case 0x0202 : type = type_int; opr->cint = lhs->cflt < rhs->cflt; break;
		} break;
		case opr_leq : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint <= rhs->cint; break;
			case 0x0102 : type = type_int; opr->cint = lhs->cint <= rhs->cflt; break;
			case 0x0201 : type = type_int; opr->cint = lhs->cflt <= rhs->cint; break;
			case 0x0202 : type = type_int; opr->cint = lhs->cflt <= rhs->cflt; break;
		} break;

		case opr_equ : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint == rhs->cint; break;
			case 0x0102 : type = type_int; opr->cint = lhs->cint == rhs->cflt; break;
			case 0x0201 : type = type_int; opr->cint = lhs->cflt == rhs->cint; break;
			case 0x0202 : type = type_int; opr->cint = lhs->cflt == rhs->cflt; break;
		} break;
		case opr_neq : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint != rhs->cint; break;
			case 0x0102 : type = type_int; opr->cint = lhs->cint != rhs->cflt; break;
			case 0x0201 : type = type_int; opr->cint = lhs->cflt != rhs->cint; break;
			case 0x0202 : type = type_int; opr->cint = lhs->cflt != rhs->cflt; break;
		} break;

		case opr_and : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint & rhs->cint; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : raiseerror(s, -1, opr->line, "invalid operands to binary &"); break;
		} break;
		case opr_xor : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint ^ rhs->cint; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : raiseerror(s, -1, opr->line, "invalid operands to binary ^"); break;
		} break;
		case  opr_or : switch (cast) {
			case 0x0101 : type = type_int; opr->cint = lhs->cint | rhs->cint; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : raiseerror(s, -1, opr->line, "invalid operands to binary |"); break;
		} break;
		case ass_equ : 
		case ass_mul : 
		case ass_div :
		case ass_mod :
		case ass_add :
		case ass_sub : switch (cast) {
			case 0x0101 :
			case 0x0102 : type = type_int; break;
			case 0x0201 :
			case 0x0202 : type = type_flt; break;
		} break;
		case ass_shr :
		case ass_shl :
		case ass_and :
		case ass_xor :
		case  ass_or : switch (cast) {
			case 0x0101 : type = type_int; break;
			case 0x0102 : 
			case 0x0201 : 
			case 0x0202 : raiseerror(s, -1, opr->line, "invalid operands to binary |"); break;
		} break;
	}
	if (type) {
		opr->type = type;
		opr->kind = tok_val;
	}
	else {
	}
}
