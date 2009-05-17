#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "main.h"

// default values
static const int wl = 9;		// warninig level
static const int ol = 1;		// optimize level

static const int dl = 2;		// execution level
static const int cc = 1;		// execution cores
static const int ss = 2 << 20;		// execution stack(256K)
//~ static const int hs = 128 << 20;		// execution heap(128M)

#ifdef __WATCOMC__
#pragma disable_message(124, 201);
#endif
#if defined(WINDOWS) || defined(_WIN32)
const char* os = "Windows";
#elif defined(linux) || defined(_linux)
const char* os = "Linux";
#else
const char* os = "Unknown";
#endif

const tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, KIND, STR) {KIND, TYPE, SIZE, STR},
	#include "incl/defs.inl" 
};
const opc_inf opc_tbl[255] = {
	#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) {Code, Size, Mnem},
	#include "incl/defs.inl"
};

//~ #include "clog.c"
//~ #include "clex.c"
//~ #include "scan.c"
//~ #include "tree.c"
//~ #include "type.c"

//{ #include "code.c"
//~ code.c - Builder & Executor ------------------------------------------------
#include <math.h>
#include "incl/libc.inl"

#pragma pack(push, 1)
typedef union {			// stack value type
	int08t	i1;
	uns08t	u1;
	int16t	i2;
	uns16t	u2;
	int32t	i4;
	uns32t	u4;
	int64t	i8;
	uns64t	u8;
	flt32t	f4;
	flt64t	f8;
	f32x4t	pf;
	f64x2t	pd;
} stkval;
struct bcdc_t {			// byte code decoder
	uns08t opc;
	union {
		stkval arg;		// argument (load const)
		uns08t idx;		// usualy index for stack
		int32t jmp:24;		// jump relative
		struct {		// when starting a task
			uns08t	dl;	// data to copy to stack
			uns16t	cl;	// code to exec paralel
		};
		/*struct {				// extended
			uint8_t opc:4;		// 0-15
			uint8_t opt:2;		// idx(3) / rev(3) / imm(6) / mem(6)
			uint8_t mem:2;		// mem size
			union {
				uint8_t si[4];
				val32t	arg;
			};
		}ext;*/
	};
};
#pragma pack(pop)

void fputasm(FILE *fout, bcde ip, int len, int offs) {
	int i;
	unsigned char* ptr = (unsigned char*)ip;
	if (offs >= 0) fprintf(fout, ".%04x ", offs);
	if (len > 1 && len < opc_tbl[ip->opc].size) {
		for (i = 0; i < len - 2; i++) {
			if (i < opc_tbl[ip->opc].size) fprintf(fout, "%02x ", ptr[i]);
			else fprintf(fout, "   ");
		}
		if (i < opc_tbl[ip->opc].size)
			fprintf(fout, "%02x... ", ptr[i]);
	}
	else for (i = 0; i < len; i++) {
		if (i < opc_tbl[ip->opc].size) fprintf(fout, "%02x ", ptr[i]);
		else fprintf(fout, "   ");
	}
	//~ if (!opc) return;
	if (opc_tbl[ip->opc].name)
		fputs(opc_tbl[ip->opc].name, fout);
	else fputmsg(fout, "opc%02x", ip->opc);
	switch (ip->opc) {
		case opc_jmp : 
		case opc_jnz : 
		case opc_jz : {
			if (offs < 0) fprintf(fout, " %+d", ip->jmp);
			else fprintf(fout, " .%04x", offs + ip->jmp);
		} break;
		/*case opc_bit : switch (ip->arg.u1 >> 5) {
			default : fprintf(fout, "bit.???"); break;
			case  1 : fprintf(fout, "shl %d", ip->arg.u1 & 0x1F); break;
			case  2 : fprintf(fout, "shr %d", ip->arg.u1 & 0x1F); break;
			case  3 : fprintf(fout, "sar %d", ip->arg.u1 & 0x1F); break;
			case  4 : fprintf(fout, "rot %d", ip->arg.u1 & 0x1F); break;
			//~ case  5 : fprintf(fout, "get %d", ip->arg.u1 & 0x1F); break;
			//~ case  6 : fprintf(fout, "set %d", ip->arg.u1 & 0x1F); break;
			//~ case  7 : fprintf(fout, "cmt %d", ip->arg.u1 & 0x1F); break;
			case  0 : switch (ip->arg.u1) {
				default : fprintf(fout, "bit.???"); break;
				case  1 : fprintf(fout, "bit.any"); break;
				case  2 : fprintf(fout, "bit.all"); break;
				case  3 : fprintf(fout, "bit.sgn"); break;
				case  4 : fprintf(fout, "bit.par"); break;
				case  5 : fprintf(fout, "bit.cnt"); break;
				case  6 : fprintf(fout, "bit.bsf"); break;		// most significant bit index
				case  7 : fprintf(fout, "bit.bsr"); break;
				case  8 : fprintf(fout, "bit.msb"); break;		// use most significant bit only
				case  9 : fprintf(fout, "bit.lsb"); break;
				case 10 : fprintf(fout, "bit.rev"); break;		// reverse bits
				// swp, neg, cmt, 
			} break;
		} break;*/
		case opc_pop : fprintf(fout, " %d", ip->idx); break;
		case opc_dup1 : case opc_dup2 :
		case opc_dup4 : fprintf(fout, " sp(%d)", ip->idx); break;
		case opc_set1 : case opc_set2 :
		case opc_set4 : fprintf(fout, " sp(%d)", ip->idx); break;
		case opc_ldc1 : fprintf(fout, " %d", ip->arg.i1); break;
		case opc_ldc2 : fprintf(fout, " %d", ip->arg.i2); break;
		case opc_ldc4 : fprintf(fout, " %d", (int)ip->arg.i4); break;
		case opc_ldc8 : fprintf(fout, " %Ld", (long long int)ip->arg.i8); break;
		case opc_ldcf : fprintf(fout, " %f", ip->arg.f4); break;
		case opc_ldcF : fprintf(fout, " %lf", ip->arg.f8); break;
		case opc_ldcr : fprintf(fout, " %x", (int)ip->arg.u4); break;
		//~ case opc_libc : fputmsg(fout, " %s %T", libcfnc[ip->idx].name, libcfnc[ip->idx].sym->type); break;
		//~ case opc_libc : fputmsg(fout, " %+T", libcfnc[ip->idx].sym); break;
		//~ case opc_libc : fprintf(fout, " %s", libcfnc[ip->idx].name); break;
		case opc_sysc : switch (ip->arg.u1) {
			case  0 : fprintf(fout, ".halt"); break;
			default : fprintf(fout, ".unknown"); break;
		} break;
		case p4d_swz  : {
			char c1 = "wxyz"[ip->arg.u1 >> 0 & 3];
			char c2 = "wxyz"[ip->arg.u1 >> 2 & 3];
			char c3 = "wxyz"[ip->arg.u1 >> 4 & 3];
			char c4 = "wxyz"[ip->arg.u1 >> 6 & 3];
			fprintf(fout, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->arg.u1);
		} break;
		case opc_cmp  : {
			//~ switch (ip->arg.u1 & 7) 
			fprintf(fout, "(%s)", "??\0eq\0lt\0le\0nz\0ne\0ge\0gt\0" + (3 * (ip->arg.u1 & 7)));
		} break;
	}
}

defn emit_opc = 0;

int emit(state s, int opc, ...) {
	bcde ip = 0;
	stkval arg = *(stkval*)(&opc + 1);

	if (s->code.cs >= s->code.mems - 16) {
		error (s, 0, 0, "memory overrun");
		return -1;
	}
	if (s->code.mins < s->code.rets)
		s->code.mins = s->code.rets;

	if (opc == opc_line) {
		static int line = 0;
		if (line != arg.i4) {
			//~ info(s, s->file, arg.i4, "emit(line)");
			line = arg.i4;
		}
		return s->code.cs;
	}

	//~ if (s->code.cs & 1) emit(s, opc_nop, arg);

	/*else if (opc == opc_ldci) {
		if (arg.u8 == 0) opc = opc_ldz;
		else if (arg.u8 <= 0xff) opc = opc_ldc1;
		else if (arg.u8 <= 0xffff) opc = opc_ldc2;
		else if (arg.u8 <= 0xffffffff) opc = opc_ldc4;
		else opc = opc_ldc8;
	}*/
	/*else if (opc == opc_ldi) switch (arg.i4) {
		case 1 : opc = opc_ldi1; break;
		case 2 : opc = opc_ldi2; break;
		case 4 : opc = opc_ldi4; break;
		default: return -1;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case 1 : opc = opc_sti1; break;
		case 2 : opc = opc_sti2; break;
		case 4 : opc = opc_sti4; break;
		default: return -1;
	}*/

	else if (opc == opc_add) switch (arg.i4) {
		case TYPE_u32 :
		case TYPE_i32 : opc = i32_add; break;
		case TYPE_i64 : opc = i64_add; break;
		case TYPE_f32 : opc = f32_add; break;
		case TYPE_f64 : opc = f64_add; break;
		//~ case TYPE_pf2 : opc = p4d_add; break;
		//~ case TYPE_pf4 : opc = p4f_add; break;
		default: return -1;
	}
	else if (opc == opc_sub) switch (arg.i4) {
		case TYPE_u32 :
		case TYPE_i32 : opc = i32_sub; break;
		case TYPE_i64 : opc = i64_sub; break;
		case TYPE_f32 : opc = f32_sub; break;
		case TYPE_f64 : opc = f64_sub; break;
		//~ case TYPE_pf2 : opc = p4d_sub; break;
		//~ case TYPE_pf4 : opc = p4f_sub; break;
		default: return -1;
	}
	else if (opc == opc_mul) switch (arg.i4) {
		case TYPE_u32 : opc = u32_mul; break;
		case TYPE_i32 : opc = i32_mul; break;
		case TYPE_i64 : opc = i64_mul; break;
		case TYPE_f32 : opc = f32_mul; break;
		case TYPE_f64 : opc = f64_mul; break;
		//~ case TYPE_pf2 : opc = p4d_mul; break;
		//~ case TYPE_pf4 : opc = p4f_mul; break;
		default: return -1;
	}
	else if (opc == opc_div) switch (arg.i4) {
		case TYPE_u32 : opc = u32_div; break;
		case TYPE_i32 : opc = i32_div; break;
		case TYPE_i64 : opc = i64_div; break;
		case TYPE_f32 : opc = f32_div; break;
		case TYPE_f64 : opc = f64_div; break;
		//~ case TYPE_pf2 : opc = p4d_div; break;
		//~ case TYPE_pf4 : opc = p4f_div; break;
		default: return -1;
	}
	else if (opc == opc_mod) switch (arg.i4) {
		case TYPE_u32 : opc = u32_mod; break;
		case TYPE_i32 : opc = i32_mod; break;
		case TYPE_i64 : opc = i64_mod; break;
		case TYPE_f32 : opc = f32_mod; break;
		case TYPE_f64 : opc = f64_mod; break;
		//~ case TYPE_pf2 : opc = p4d_mod; break;
		//~ case TYPE_pf4 : opc = p4f_mod; break; p4f_crs ???
		default: return -1;
	}

	else if (opc == opc_not) switch (arg.i4) {
		//~ case TYPE_u32 :
		//~ case TYPE_i32 : opc = i32_add; break;
		//~ case TYPE_i64 : opc = i64_add; break;
		//~ case TYPE_f32 : opc = f32_add; break;
		//~ case TYPE_f64 : opc = f64_add; break;
		//~ case TYPE_pf2 : opc = p4d_add; break;
		//~ case TYPE_pf4 : opc = p4f_add; break;
		default: return -1;
	}
	else if (opc == opc_ceq) switch (arg.i4) {
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i32 | cmp_tun; break;
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i32; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_f32; break;
		//~ case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i64 | cmp_tun; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i64; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_clt) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_clt | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i64; break;
		//~ case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_clt | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_cle) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_cle | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i64; break;
		//~ case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_cle | cmp_f64; break;
		default: return -1;
	}

	else if (opc == opc_cne) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i64; break;
		//~ case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_cge) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i64; break;
		//~ case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_cgt) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i64; break;
		//~ case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_f64; break;
		default: return -1;
	}

	// bits
	else if (opc == opc_shl) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_shl; break;
		default: return -1;
	}
	else if (opc == opc_shr) switch (arg.i4) {
		case TYPE_u32 : opc = b32_sar; break;
		case TYPE_i32 : opc = b32_shr; break;
		default: return -1;
	}
	else if (opc == opc_and) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_and; break;
		default: return -1;
	}
	else if (opc == opc_ior) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_ior; break;
		default: return -1;
	}
	else if (opc == opc_xor) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_xor; break;
		default: return -1;
	}

	//~ :)) ?? rollback (optimize)
	else if (opc == opc_ldz1) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_ldz1) {
			opc = opc_ldz2;
			s->code.cs = s->code.pc;
			s->code.rets -= 1;
		}
	}
	else if (opc == opc_ldz2) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_ldz2) {
			opc = opc_ldz4;
			s->code.cs = s->code.pc;
			s->code.rets -= 2;
		}
	}
	else if (opc == opc_dup1) {			// TODO 
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_dup1 && ip->idx == arg.u4) {
			ip->opc = opc_dup2;
			ip->idx -= 1;
			s->code.rets += 1;
			return s->code.pc;
		}
	}
	else if (opc == opc_dup2) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_dup2 && ip->idx == arg.u4) {
			ip->opc = opc_dup4;
			ip->idx -= 2;
			s->code.rets += 2;
			return s->code.pc;
		}
	}// */
	/*else if (opc == opc_jnz || opc == opc_jz) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_bit && ip->arg.u1 == 1) {
			opc = (opc == opc_jnz) ? opc_jz : opc_jnz;
			s->code.cs = s->code.pc;
		}
	}// */
	/*else if (opc == opc_shl || opc == opc_shr || opc == opc_sar) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_ldc4) {
			if (opc == opc_shl) arg.i1 = 1 << 5;
			if (opc == opc_shr) arg.i1 = 2 << 5;
			if (opc == opc_sar) arg.i1 = 3 << 5;
			s->code.rets -= 1;
			opc = opc_bit;
			arg.i1 |= ip->arg.u1 & 0x1F;
			s->code.cs = s->code.pc;
		}
	}// */

	if (opc > opc_last) {
		//~ fatal(s, __FILE__, __LINE__, "xxx_opc%x", opc);
		fatal(s, "xxx_opc%x", opc);
		return 0;
	}

	ip = (bcde)&s->code.memp[s->code.cs];
	//~ TODO s->code.ic += s->code.pc != s->code.cs;
	s->code.pc = s->code.cs;

	ip->opc = opc;
	ip->arg = arg;

	switch (opc) {
		error_opc : fatal(s, "emit:invalid opcode:[%02x]:%A", ip->opc, ip); return -1;
		error_stc : fatal(s, "emit:stack underflow: near opcode %A", ip); return -1;
		#define NEXT(__IP, __SP, __CHK)\
			if(s->code.rets < (__CHK)) goto error_stc;\
			s->code.rets += (__SP);\
			s->code.cs += (__IP);
		#define STOP(__ERR) goto __ERR
		//~ #define EXEC(__STMT)
		//~ #define MEMP(__MPTR)
		//~ #define CHECK()
		#include "incl/exec.inl"
	}
	return s->code.pc;
}

int emitidx(state s, int opc, int arg) {
	stkval tmp;
	tmp.i4 = s->code.rets - arg;
	return emit(s, opc, tmp);
}

int emiti32(state s, int opc, int32t arg) {
	stkval tmp;
	tmp.i4 = arg;
	return emit(s, opc, tmp);
}

int emiti64(state s, int opc, int64t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emit(s, opc, tmp);
}

int emitf32(state s, int opc, flt32t arg) {
	stkval tmp;
	tmp.f4 = arg;
	return emit(s, opc, tmp);
}

int emitf64(state s, int opc, flt64t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return emit(s, opc, tmp);
}

int fixjump(state s, int src, int dst, int stc) {
	bcde ip = (bcde)(s->code.memp + src);
	if (src >= 0) switch (ip->opc) {
		case opc_jmp : 
		case opc_jnz : 
		case opc_jz  : ip->jmp = dst - src; break;
		default : 
			debug("expecting jump, but found : '%+A'", ip);
	}
	return 0;
}

int istype(node ast) {		// type[.type]
	if (!ast) return 0;

	dieif(ast->kind == TYPE_def2);

	if (ast->kind != TYPE_ref) return 0;
	dieif(ast->type == 0);

	if (ast->link && ast->link->kind == TYPE_ref) return 0;
	return ast->type ? ast->type->kind : 0;
}
int iscast(node ast) {		// type (...)
	if (!ast) return 0;
	if (ast->kind != OPER_fnc) return 0;
	return istype(ast->lhso);
	//~ if (!(ast = ast->lhso)) return 0;
	//~ if (ast->kind != TYPE_ref) return 0;
	//~ if (ast->link) return 0;
	//~ return ast->type;
}
int isemit(node ast) {		// emit (...)
	if (!ast) return 0;
	if (ast->kind != OPER_fnc) return 0;

	if (!ast->lhso) return 0;
	if (ast->lhso->kind != EMIT_opc) return 0;

	return EMIT_opc;
}

//~ islval
defn linkOf(node ast) {
	if (ast && ast->kind == OPER_dot)
		ast = ast->rhso;
	if (ast && ast->kind == TYPE_ref)
		return ast->link;
	//~ if (ast && ast->type && ast->kind == EMIT_opc)
		//~ return ast->link;
	debug("linkof:%s",tok_tbl[ast->kind].name);
	//~ if (ast && ast->kind == TYPE_lnk)
		//~ return ast->link;
	return 0;
}// */

/*defn typeOf(node ast) {
	if (ast && ast->kind == OPER_dot)
		ast = ast->rhso;
	if (ast && ast->kind == TYPE_ref)
		return ast->link;
	debug("linkof:%s",tok_tbl[ast->kind].name);
 	//~ if (ast && ast->kind == TYPE_lnk)
		//~ return ast->link;
	return 0;
}*/

/** Cast 
 * returns one of (u32, i32, i64, f32, f64)
**/
int castId(defn typ) {
	if (typ) switch (typ->kind) {
		default : return 0;
		//~ case TYPE_vid : return 0;
		case TYPE_bit : return TYPE_i32;
		case TYPE_uns : return TYPE_u32;
		case TYPE_int : switch (typ->size) {
			case 1 : return TYPE_i32;
			case 2 : return TYPE_i32;
			case 4 : return TYPE_i32;
			case 8 : return TYPE_i64;
		} break;
		case TYPE_flt : switch (typ->size) {
			case 4 : return TYPE_f32;
			case 8 : return TYPE_f64;
		} break;
		//~ case TYPE_ptr : return TYPE_i32;
		//~ case TYPE_enu : return TYPE_u32;
		//~ case TYPE_rec : return 0;
		case TYPE_u32 :
		case TYPE_i32 :
		case TYPE_i64 :
		case TYPE_f32 :
		case TYPE_f64 :
			return typ->kind;
	}// */
	return 0;
}
defn castTy(defn lhs, defn rhs) {
	defn ret = NULL;
	if (lhs && rhs) {
		switch (lhs->kind) {
			case TYPE_bit : switch (rhs->kind) {
				case TYPE_bit : ret = rhs; break;
				case TYPE_uns : ret = rhs; break;
				case TYPE_int : ret = rhs; break;
				case TYPE_flt : ret = rhs; break;
			} break;
			case TYPE_uns : switch (rhs->kind) {
				case TYPE_bit : ret = lhs; break;
				case TYPE_uns : ret = rhs; break;
				case TYPE_int : ret = rhs; break;
				case TYPE_flt : ret = rhs; break;
			} break;
			case TYPE_int : switch (rhs->kind) {
				case TYPE_bit : ret = lhs; break;
				case TYPE_uns : ret = lhs; break;
				case TYPE_int : ret = lhs->size > rhs->size ? lhs : rhs; break;
				case TYPE_flt : ret = rhs; break;
			} break;
			case TYPE_flt : switch (rhs->kind) {
				case TYPE_bit : ret = lhs; break;
				case TYPE_uns : ret = lhs; break;
				case TYPE_int : ret = lhs; break;
				case TYPE_flt : ret = lhs->size > rhs->size ? lhs : rhs; break;
			} break;
		}
	}
	if (ret) switch (rhs->kind) {
		case TYPE_bit : if (ret->size <= 4) return type_u32;
		case TYPE_uns : if (ret->size <= 4) return type_u32;
		case TYPE_int : if (ret->size <= 4) return type_i32;
		case TYPE_flt : return ret;
	}
	return ret;
}// */

//~ args assumed to be in list(args->next...)
//~ defn findfn(state s, defn loc, node ast, node args) {}

defn lookup(state s, defn loc, node ast) {
	//~ struct astn_t tmp;
	node args = 0, ref = 0;
	defn sym = 0;
	if (!ast) {
		return 0;
	}
	/*if (ast->type) {
		return ast->type;
		/ *case OPER_add : {		// 'lhs + rhs' => add(lhs, rhs)
			if (promote(ast->lhso) && promote(ast->rhso)) {
				return alma;
			}
			find = oper_add;
			args = ast->lhso;
			ast->rhso->next = 0;
			args->next = ast->rhso;
		} break;// * /
	}*/
	switch (ast->kind) {
		case OPER_dot : {
			//~ defn l1 = lookup(s, loc, ast->lhso);
			//~ defn l2 = l1 ? lookup(s, l1, ast->rhso) : 0;
			//~ debug("dot:%T::%T::%T", loc, l1, l2);
			if ((loc = lookup(s, loc, ast->lhso)))
				return ast->type = lookup(s, loc, ast->rhso);
			return NULL;
		} break;
		case OPER_fnc : {
			node next = 0;
			defn lin = isemit(ast) ? emit_opc : 0;
			args = ast->rhso;
			while (args && args->kind == OPER_com) {
				if (!lookup(s, lin, args->rhso)) {
					if (lin && !lookup(s, 0, args->rhso)) {
						//~ error(s, s->file, args->rhso->line, "cast arg %+k", args->rhso);
						debug("cast arg %+k", args->rhso);
						return 0;
					}
					//~ else if !iscast warn
				}
				args->rhso->cast = castId(args->rhso->type);
				args->rhso->next = next;
				next = args->rhso;
				args = args->lhso;
			}
			if (args) {
				if (!lookup(s, lin, args)) {
					if (lin && !lookup(s, 0, args)) {
						debug("???");
						return 0;
					}
					//~ else if !iscast warn
				}
				args->next = next;
				args->cast = castId(args->type);
			}
			if (ast->lhso == 0) {
				if (ast->rhso != args) return NULL;			// a + (2, 3)
				return ast->type = lookup(s, 0, ast->rhso);
			}
			if (isemit(ast)) {
				ast->type = lookup(s, emit_opc, args);
				ast->cast = castId(ast->type);
				return ast->type;
			}
			switch (ast->lhso->kind) {
				case EMIT_opc :
					return ast->type = lookup(s, emit_opc, args);

				case OPER_dot : {
					ref = ast->lhso->rhso;
					loc = lookup(s, loc, ast->lhso->lhso);
					if (!loc) return 0;
				}

				default : ref = ast->lhso; break;
			}
				//~ "a.b.c(a,b,c)" := ((a.b).c)(((a, b), c))
		} break;
		//~ case OPER_idx : ;

		case OPER_pls : 		// '+'
		case OPER_mns : 		// '-'
		case OPER_cmt : {		// '~'
			// 'lhs op rhs' => op(lhs, rhs)
			defn ret = lookup(s, 0, ast->rhso);
			if ((ast->cast = castId(ret))) {
				// todo float.cmt : invalid
				return ast->type = ret;
			}

			ast->cast = 0;
			return ast->type = NULL;
		} break;
		case OPER_not : {		// '!'
			defn ret = lookup(s, 0, ast->rhso);
			if (castId(ret)) {
				ast->cast = TYPE_bit;
				return ast->type = type_bol;
			}
			ast->cast = 0;
			return ast->type = NULL;
		} break;

		case OPER_add : 
		case OPER_sub : 
		case OPER_mul : 
		case OPER_div : 
		case OPER_mod : {		// 'lhs op rhs' => op(lhs, rhs)
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			ast->type = castTy(lht, rht);
			ast->cast = castId(ast->type);

			if (!lht || !rht)
				return NULL;

			if (ast->cast)
				return ast->type;

			// 'lhs + rhs' => tok_name[OPER_add] '(lhs, rhs)'

			//~ fun = tok_tbl[ast->kind].name;			// __add
			//~ args = ast->lhso;
			//~ ast->rhso->next = 0;
			//~ args->next = ast->rhso;
			return 0;
		} break;

		case OPER_shl : 		// '>>'
		case OPER_shr : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_ior : 		// '|'
		case OPER_xor : {		// '^'
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);
			defn ret = castTy(lht, rht);

			switch ((ast->cast = castId(ret))) {
				case TYPE_u32 : return ast->type = ret;
				//~ case TYPE_u64 : return ast->type = ret;
				case TYPE_i32 : return ast->type = ret;
				case TYPE_i64 : return ast->type = ret;
				case TYPE_f32 : 
				case TYPE_f64 : ast->cast = 0; return ast->type = 0;
			}

			// TODO find operator __op (lhs, rhs)

			return 0;
		} break;

		//~ case OPER_equ : 		// '=='
		//~ case OPER_neq : 		// '!='
		//~ case OPER_lte : 		// '<'
		//~ case OPER_leq : 		// '<='
		//~ case OPER_gte : 		// '>'
		//~ case OPER_geq : 		// '>='

		case CNST_int : return ast->type = type_i32;
		case CNST_flt : return ast->type = type_f64;
		case TYPE_ref : ref = ast; break;
		case EMIT_opc : return 0;

		case OPER_lor : 
		case OPER_lnd : {
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);
			if (castId(lht) && castId(rht)) {
				ast->cast = TYPE_bit;
				return ast->type = type_bol;
			}
			return NULL;
		} break;

		case OPER_sel : {
			defn cmp = lookup(s, 0, ast->test);
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);
			defn ret = castTy(lht, rht);
			if (castId(cmp) && (ast->cast = castId(ret)))
				return ast->type = ret;
			return NULL;
		} break;

		//~ case TYPE_def : {}

		default : {
			debug("invalid lookup `%s` in %?T", tok_tbl[ast->kind].name, loc);
		} return NULL;

		//~ case OPER_not : return lookup(s, 0, ast->rhso);
	}

	sym = loc ? loc->args : s->deft[ref->hash];

	debug("%k(%k) in %T[%d]", ref, ast, sym, ast->hash);

	for (; sym; sym = sym->next) {
		//~ node arg = args;			// callee arguments
		//~ defn typ = sym->args;		// caller arguments
		if (!sym->name) continue;
		if (strcmp(sym->name, ref->name)) continue;

		//~ debug("%k ?= %T", ref, sym);
		switch (sym->kind) {
			// constant
			case CNST_int :
			case CNST_flt :
			case CNST_str : {
				debug("%k ?= %T(%k)", ref, sym->type, sym->init);
				if (args) continue;
				//~ debug("%k ?= %T(%k)\n", ref, sym->type, sym->init);
				ref->kind = TYPE_ref;
				ref->type = ast->type = sym->type;
				ref->rhso = sym->init;
				ref->link = sym;
			} break;

			case TYPE_def2 : {
				debug("%k ?= %T(%k)", ref, sym->type, sym->init);
				if (args && !args->cast) continue;
				ref->kind = TYPE_ref;
				ref->type = ast->type = sym->type;
				ref->rhso = sym->init;
				ref->link = sym;
			} break;

			// typename
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

			case TYPE_enu :
			case TYPE_rec : {
				//~ if (args) continue;	// cast probably
				ref->kind = TYPE_ref;
				ref->type = ast->type = sym;
				ast->cast = castId(sym);
				ref->stmt = 0;
				ref->link = 0;
			} break;// */

			// variable/operator/property/function ?
			case EMIT_opc :
			case TYPE_ref : {
				// TODO lot to do here
				debug("%k ?= %T(%k)", ref, sym->type, sym->init);
				ref->kind = TYPE_ref;
				ref->type = sym->type;
				//~ ref->cast = sym->type->kind;
				//~ ast->rhso = sym->init;	// init|body
				ref->link = sym;
				sym->used = 1;
			} break;
			default : {
				error(s, s->file, ast->line, "undeclared `%k`", ast);
				sym = declare(s, TOKN_err, ast, NULL, NULL);
				sym->type = type_i32;
				ast->kind = TYPE_ref;
				ast->type = type_i32;
				ast->link = sym;
			} break;
		}

		// if we are here then sym is found.
		break;
	}

	//~ debug("lookup(%k in %?T) is %T[%T]", ast, loc, ast->type, sym);
	return ast->type;
}

int cgen(state s, node ast, int get) {
	struct astn_t tmp;
	int ret = 0;

	if (!ast) return 0;

	//~ debug("cgen:(%+k)%T", ast, ast->type);

	switch (ast->kind) {
		//{ STMT
		case TYPE_def2 : break;	// decl
		case OPER_nop : {		// expr statement

			//~ dieif(!ast->stmt);					// is cleaned

			if (lookup(s, 0, ast->stmt)) {
				//~ if (eval(&tmp, ast->stmt, get))
				emit(s, opc_line, ast->line);
				cgen(s, ast->stmt, TYPE_vid);
			}
			else {
				error(s, s->file, ast->line, "in expression (%+k)", ast->stmt);
			}
		} return 0;
		case OPER_beg : {		// {}
			defn typ;
			node ptr;
			int stc = 0;
			emiti32(s, opc_line, ast->line);
			for (ptr = ast->stmt; ptr; ptr = ptr->next) {
				cgen(s, ptr, TYPE_vid);
				//~ if (stc) // this can be a type decl
			}
			for (typ = ast->type; typ; typ = typ->next)
				if (typ->onst) stc += 1;
			if (stc) emiti32(s, opc_pop, stc);
		} return 0;
		case OPER_jmp : {		// if ( ) then {} else {}
			int tt = eval(&tmp, ast->test, TYPE_bit);
			emiti32(s, opc_line, ast->line);
			//~ if (ast->cast & QUAL_sta)			// TODO: compile time if
				//~ if ( !tt ) { error(); return 0; }

			if (ast->test) {					// if then else
				int jmpt, jmpf;
				if (s->copt > 0 && tt) {		// if true
					if (constbol(&tmp))
						cgen(s, ast->stmt, TYPE_vid);
					else
						cgen(s, ast->step, TYPE_vid);
				}
				else if (ast->stmt && ast->step) {		// if then else
					cgen(s, ast->test, TYPE_bit);
					jmpt = emiti32(s, opc_jz, 0);
					cgen(s, ast->stmt, TYPE_vid);
					jmpf = emiti32(s, opc_jmp, 0);
					fixjump(s, jmpt, s->code.cs, 0);
					cgen(s, ast->step, TYPE_vid);
					fixjump(s, jmpf, s->code.cs, 0);
				}
				else if (ast->stmt) {					// if then
					cgen(s, ast->test, TYPE_bit);
					//~ if false skip THEN block
					jmpt = emiti32(s, opc_jz, 0);
					cgen(s, ast->stmt, TYPE_vid);
					fixjump(s, jmpt, s->code.cs, 0);
				}
				else if (ast->step) {					// if else
					cgen(s, ast->test, TYPE_bit);
					//~ if true skip ELSE block
					jmpt = emiti32(s, opc_jnz, 0);
					cgen(s, ast->step, TYPE_vid);
					fixjump(s, jmpt, s->code.cs, 0);
				}
				else {
					//~ cgen(s, ast->test, TYPE_vid);
					//~ print(s, 8, s->file, ast->line, "");
				}
			}
			else debug("unimplemented: goto / break/ continue;");
		} return 0;
		case OPER_for : {		// for ( ; ; ) {}
			//!TODO: replace 'xxx = s->code.cs' with 'xxx = emit(...)'
			defn typ;
			int stc = 0;
			int beg, end, cmp = -1;

			emiti32(s, opc_line, ast->line);
			cgen(s, ast->init, TYPE_any);

			beg = s->code.cs;
			if (ast->step) {
				int tmp = beg;
				if (ast->init) emiti32(s, opc_jmp, 0);

				beg = s->code.cs;
				cgen(s, ast->step, TYPE_vid);
				if (ast->init) fixjump(s, tmp, s->code.cs, 0);
			}
			if (ast->test) {
				cgen(s, ast->test, TYPE_bit);
				cmp = emiti32(s, opc_jz, 0);		// if (!test) break;
			}

			// push(list_jmp, 0);
			cgen(s, ast->stmt, TYPE_vid);
			end = emiti32(s, opc_jmp, beg);			// continue;
			fixjump(s, end, beg, 0);
			fixjump(s, cmp, s->code.cs, 0);				// break;
			//~ fixjump(s, cmp, end, 0);				// break;

			//~ while (list_jmp) {
			//~ if (list_jmp->kind == break)
			//~ 	fixj(s, list_jmp->offs, end, 0);
			//~ if (list_jmp->kind == continue)
			//~ 	fixj(s, list_jmp->offs, beg, 0);
			//~ list_jmp = list_jmp->next;
			//~ }
			// pop(list_jmp);


			for (typ = ast->type; typ; typ = typ->next)
				if (typ->onst) stc += 1;
			if (stc) emiti32(s, opc_pop, stc);
		} return 0;
		//}
		//{ OPER
		case OPER_fnc : {		// '()' emit/cast/call/libc
			dieif(!ast->type);

			//~ debug("call (%+k) : %T", ast->rhso, ast->type);
			if (!ast->lhso || iscast(ast)) {	// (a + b)
				//~ debug("cast(%+k) : %T", ast->rhso, ast->type);
				if (!cgen(s, ast->rhso, ast->cast)) {
					fatal(s, "cgen(%k)", ast);
					return 0;
				}
				//~ debug("cast(%?+k)", ast->rhso);
				ret = ast->cast;
			}
			else if (isemit(ast)) {			// emit(a)
				//~ int stu = s->code.cs;
				defn opc;
				node argv = ast->rhso;

				//~ debug("emit(%+k) : %T", ast->rhso, ast->type);
				while (argv && argv->kind == OPER_com) {
					node arg = argv->rhso;
					switch (iscast(arg)) {
						case 0 :
							warn(s, 6, s->file, arg->line, "suggest cast for arguments");
							break;

						default :
							warn(s, 9, s->file, arg->line, "suggest other cast for arguments for %+k", arg);

						case TYPE_u32 :
						//~ case TYPE_u64 :
						case TYPE_i32 :
						case TYPE_i64 :
						case TYPE_f32 :
						case TYPE_f64 :
							//~ arg = arg->rhso;
							break;

						//~ case TYPE_vid :
						//~ case TYPE_bit :
						//~ case TYPE_uns :
						//~ case TYPE_int :
						//~ case TYPE_flt :
					}
					if (!cgen(s, arg, argv->rhso->cast)) {
						fatal(s, "cgen:arg(%+k)", argv->rhso);
						return 0;
					}
					argv = argv->lhso;
				}
				//~ if (!argv || argv->kind != TY)
				if ((opc = linkOf(argv))) {
					ret = emiti64(s, opc->offs, opc->init ? opc->init->cint : 0);
				}
				else if (argv) {
					if (argv->type != type_vid)
						error(s, s->file, argv->line, "opcode expected, not %k", argv);
				}
				else {
					error(s, s->file, ast->line, "opcode expected, and or arguments");
				}
				//~ debug("emit(%+k)", argv);
			}// */
			else {					// call(0x32)
				node argv = ast->rhso;
				debug("call(%+k)", ast->lhso);
				while (argv && argv->kind == OPER_com) {
					if (!cgen(s, argv->rhso, argv->rhso->cast)) {
						fatal(s, "cgen:arg(%+k)", argv->rhso);
						return 0;
					}
					argv = argv->lhso;
				}
				if (argv && !cgen(s, argv, argv->cast)) {
					fatal(s, "cgen:arg(%+k)", argv->rhso);
					return 0;
				}
			}
		} break;
		case OPER_dot : {
			//~ ret = cgen(s, ast->rhso, ast->cast);
			/*
			if (islval(ast->rhso)) {
				defn ref;
				int ofs = 0;
				node in = ast->lhso;
				if (!cgen(s, ast->rhso, TYPE_ref))
					return 0;
				while (ast->kind == OPER_dot) {
					ast = ast->lhso;
					dieif(!ast->rhso);
					ref = islval(ast->rhso);
					if (ref) ofs += ref->offs;
				}
			}// */
			if (!cgen(s, ast->rhso, ast->cast)) return 0;
		} break; //*/
		/*case OPER_idx : {		// '[]'
			if (!cgen(s, ast->lhso, TYPE_ref)) return 0;
			if (!cgen(s, ast->rhso, TYPE_i32)) return 0;
			emit(s, opc_ldc4, argi32(ast->type->size));
			emit(s, opc_madu, noarg);
			if (!ast->type->init) {		// []
				emit(s, opc_ldi4, noarg);
			}
			if ((ret = get) != TYPE_ref) {
				ret = typeId(ast);
				emit(s, opc_ldi, argi32(ast->type->size));
			}
		} goto castret;*/

		case OPER_pls : 		// '+'
		case OPER_mns : 		// '-'
		case OPER_cmt : 		// '~'
		case OPER_not : 		// '!'
			dieif(!ast->type);
			if (!cgen(s, ast->lhso, ast->cast)) return 0;
			switch (ast->kind) {
				case OPER_pls : break;
				//~ case OPER_mns : ret = emit(s, opc_neg, ast->cast); break;
				//~ case OPER_cmt : ret = emit(s, opc_clt, ast->cast); break;
				case OPER_not : {
					dieif(ast->cast != TYPE_bit);
					ret = emit(s, opc_not, TYPE_bit);
				} break;
			}

		case OPER_shl : 		// '>>'
		case OPER_shr : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_ior : 		// '|'
		case OPER_xor : 		// '^'

		case OPER_equ : 		// '=='
		case OPER_neq : 		// '!='
		case OPER_lte : 		// '<'
		case OPER_leq : 		// '<='
		case OPER_gte : 		// '>'
		case OPER_geq : 		// '>='
			dieif(!ast->type);
			if (!cgen(s, ast->rhso, ast->cast)) return 0;
			if (!cgen(s, ast->lhso, ast->cast)) return 0;
			if (ast->lhso) {}
			else switch (ast->kind) {
			}
			break ;

		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : 		// '%'
			dieif(!ast->type);
			if (!cgen(s, ast->rhso, ast->cast)) return 0;
			if (!cgen(s, ast->lhso, ast->cast)) return 0;
			if (0) {	// if we have a function to execute(ast->rhso)
			}
			else switch (ast->kind) {

				case OPER_add : ret = emit(s, opc_add, ast->cast); break;
				case OPER_sub : ret = emit(s, opc_sub, ast->cast); break;
				case OPER_mul : ret = emit(s, opc_mul, ast->cast); break;
				case OPER_div : ret = emit(s, opc_div, ast->cast); break;
				case OPER_mod : ret = emit(s, opc_mod, ast->cast); break;

				case OPER_equ : ret = emit(s, opc_ceq, ast->cast); break;
				case OPER_neq : ret = emit(s, opc_cne, ast->cast); break;
				case OPER_lte : ret = emit(s, opc_clt, ast->cast); break;
				case OPER_leq : ret = emit(s, opc_cle, ast->cast); break;
				case OPER_gte : ret = emit(s, opc_cgt, ast->cast); break;
				case OPER_geq : ret = emit(s, opc_cge, ast->cast); break;

				case OPER_shl : 		// '>>'
				case OPER_shr : 		// '<<'
				case OPER_and : 		// '&'
				case OPER_ior : 		// '|'
				case OPER_xor : 		// '^'

				default: dieif(1); return 0;
			}
			break ;

		//~ case OPER_sel : 		// ?:
		case ASGN_set : {		// '='
			defn var = 0;//islval(ast->lhso);

			dieif(!ast->type);

			if (var && var->onst) {
				debug("unimpl '%+k'(%T)", ast, var);
				return 0;
			}
			else if (var) {
				debug("unimpl '%+k'(%T)", ast, var);
				return 0;
			}
			else {
				fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
				return 0;
			}
		} break;
		//}
		//{ TVAL
		case CNST_int : switch (get) {
			default : debug("cgen: invalid cast :[%d->%d](%?s to %?s) '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast);
			case TYPE_vid : return TYPE_vid;
			//~ case TYPE_any : emiti32(s, opc_ldc4, ast->cint); return TYPE_int;

			//~ case TYPE_bit :
			//~ case TYPE_uns :
			//~ case TYPE_int : emiti32(s, opc_ldc4, ast->cint); return get;
			//~ case TYPE_flt : emitf64(s, opc_ldcF, ast->cint); return get;

			//~ case TYPE_u32 : emiti32(s, opc_ldc4, ast->cint); return get;
			//~ case TYPE_u64 : emiti32(s, opc_ldc4, ast->cint); return get;

			case TYPE_i32 : emiti32(s, opc_ldc4, ast->cint); return get;
			case TYPE_i64 : emiti64(s, opc_ldc8, ast->cint); return get;

			case TYPE_f32 : emitf32(s, opc_ldcf, ast->cint); return get;
			case TYPE_f64 : emitf64(s, opc_ldcF, ast->cint); return get;

			//~ case TYPE_ref : error("invalid lvalue");
		} return 0;
		case CNST_flt : switch (get) {
			default : debug("cgen: invalid cast :[%d->%d](%?s to %?s) '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast);
			//~ default : debug("error %04x:%s %+k", get, tok_tbl[get].name, ast); break;
			//~ case TYPE_vid : return TYPE_vid;
			//~ case TYPE_any : emitf32(s, opc_ldcf, ast->cflt); return TYPE_f32;

			case TYPE_u32 :
			case TYPE_i32 : emiti32(s, opc_ldc4, ast->cflt); return get;
			//~ case TYPE_u64 :
			case TYPE_i64 : emiti64(s, opc_ldc8, ast->cflt); return get;

			case TYPE_f32 : emitf32(s, opc_ldcf, ast->cflt); return get;
			case TYPE_f64 : emitf64(s, opc_ldcF, ast->cflt); return get;

			//~ case TYPE_ref : error("invalid lvalue");
		} return 0;
		case CNST_str : switch (get) {
			default : debug("error %04x:%s %+k", get, tok_tbl[get].name, ast); break;
			case TYPE_vid : return TYPE_vid;
			//~ case TYPE_ptr : 
			//~ case TYPE_ref : emiti32(s, opc_ldcr, ast->link->offs); return get;
		} return 0;

		/*case TYPE_def : {
			debug("cgen(TYPE_def) : %+k", ast);
			struct astn_t tmp, *ref = ast->stmt;
			if (eval(&tmp, ref, 0)) ref = &tmp;
			return cgen(s, ref, get);
		}*/

		case TYPE_ref : {
			defn typ = ast->type;		// type
			defn var = ast->link;		// link

			fatal(s, "Node(%k)", ast->stmt);
			dieif(!typ || !var);
			ret = typ->kind;

			debug("cgen(TYPE_ref:'%T':%b@%x):%T", var, var->onst, var->offs, typ);
			if (ast->rhso) return cgen(s, ast->stmt, ast->cast);
			if (var->offs == 0) {	// new
				node val = var->init;
				switch (ret = typ->kind) {
					case TYPE_u32 :
					case TYPE_int :
					case TYPE_i32 :
					case TYPE_f32 : {
						var->onst = 1;// s->level > 1;
						if (val) cgen(s, val, ret);
						else emit(s, opc_ldz1);
						var->offs = s->code.rets;
					} return ret;
					case TYPE_i64 :
					case TYPE_f64 : {
						var->onst = 1;
						if (val) cgen(s, val, ret);
						else emit(s, opc_ldz2);
						var->offs = s->code.rets;
					} break;
					/*
					//~ case TYPE_ptr :
					case TYPE_arr : {	// malloc();
						//~ debug("unimpl ptr or arr %+k %+T", ast, typ);
						emit(s, opc_ldcr, argi32(0xff00ff));
						//~ var->onst = 1;
						//~ if (val) cgen(s, val, typ->kind);
						//~ else emit(s, opc_ldz1, noarg);
						//~ var->offs = s->code.rets;
					} break;
					//~ case TYPE_rec :
					//~ case TYPE_enu :	// malloc(); || salloc();
					//~ */
					default : 
						debug("unimpl %08x[%k]:%T", typ->kind, ast, typ);
						//~ debug("unimpl%T", typ->kind, ast, typ);
						break;
				}
				fatal(s, "unimpl '%+k'(%T)", ast, var);
				return 0;
			}
			else if (var->onst) {	// on stack
				fatal(s, "unimpl '%+k'(%T)", ast, var);
				return 0;
			}
			else /* if (var) */ {	// in memory
				fatal(s, "unimpl '%+k'(%T)", ast, var);
				return 0;
			}
		} break;
		//}
		default:
			fatal(s, "Node(%k)", ast);
			return 0;
	}
	if (get != ret) switch (get) {
		case TYPE_vid : return TYPE_vid;
		// bool
		//~ case TYPE_bit : switch (ret) {}

		// cast
		case TYPE_u32 : switch (ret) {
			case TYPE_u32 : break;	// cannot happen : get == ret
			case TYPE_i32 : break;
			case TYPE_f32 : emit(s, f32_i32); break;
			case TYPE_f64 : emit(s, f64_i32); break;
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_i32 : switch (ret) {
			case TYPE_bit : break;
			case TYPE_u32 : break;
			//~ case TYPE_i32 : break;	// cannot happen : get == ret
			case TYPE_i64 : emit(s, i64_i32); break;
			case TYPE_f32 : emit(s, f32_i32); break;
			case TYPE_f64 : emit(s, f64_i32); break;
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_i64 : switch (ret) {
			case TYPE_bit : emit(s, i32_i64); break;
			case TYPE_u32 : emit(s, i32_i64); break;
			case TYPE_i32 : emit(s, i32_i64); break;
			case TYPE_f32 : emit(s, f32_i64); break;
			case TYPE_f64 : emit(s, f64_i64); break;
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_f32 : switch (ret) {
			case TYPE_bit :
			case TYPE_u32 :
			case TYPE_i32 : emit(s, i32_f32); break;
			case TYPE_f64 : emit(s, f64_f32); break;
			//~ case TYPE_flt : break;	// cannot happen : get == ret
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_f64 : switch (ret) {
			case TYPE_bit :
			case TYPE_u32 :
			case TYPE_i32 : emit(s, i32_f64); break;
			case TYPE_f32 : emit(s, f32_f64); break;
			//~ case TYPE_f64 : break;	// cannot happen : get == ret
			default : goto errorcast;
		} return get;	// conversion success

		// other casts ar function calls

		//~ case TYPE_ref : 	// ... if we need a reference: array, pointer, ... is ok

		default : fatal(s, "cgen(`%+k`, %s):%s", ast, tok_tbl[get].name, tok_tbl[ret].name);
		errorcast: debug("invalid cast :[%d->%d](%?s to %?s) '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast);
			return 0;
	}

	return ret;
}

//~ int ngen(state s, ???);

/*static cell task(cell pp, int cl, int dl, int ss) {
	// find an empty cpu
	cell pu = 0;//pp;
	while (pu && pu->ip)
		pu = pu->next;
	if (pu != NULL) {
		// set ip and copy stack
		pp->cp++;		// workers
		pu->pp = pp;		// parent
		pu->ip = pp->ip;
		pu->sp = pu->bp + ss - cl;
		memcpy(pu->sp, pp->sp, cl * 4);
	}
	return pu;
}// */

/*static int quit(cell pu) {
	pp->pp->cp--;			// workers
	sigsnd(pp->pp, SIG_quit);
}// */

/**
 * @arg env : enviroment
 * @arg cc  : cell count
 * @arg ss  : stack size
 * @return  : error code
**/

typedef int (*dbgf)(state env, cell pu, int n, bcde ip, unsigned ss);

int dbgCon(state env, cell pu, int pn, bcde ip, unsigned ss) {
	int stki, stkn = (pu->bp + ss - pu->sp);
	//~ if (stkn < dbg*4) stkn = dbg*4;
	//~ fputmsg(stdout, "exec: ss:[%03d] pu:(%02d) : %A\n", ssiz, i, ip);
	//~ fputmsg(stdout, ">exec:pu%02d:[ss:%03d zf:%d sf:%d cf:%d of:%d]: %A\n", i, ssiz, pu->zf, pu->sf, pu->cf, pu->of, ip);
	//~ fputmsg(stdout, ">exec:pu%02d:[ss:%03d, zf:%d sf:%d cf:%d of:%d]: %A\n", i, ssiz, pu->zf, pu->sf, pu->cf, pu->of, ip);
	fputmsg(stdout, ">exec:pu%02d:[ss:%03d]: %A\n", pn, stkn/4, ip);
	for (stki = 0; stki < stkn; stki += 4) {
		stkval* stkv = (stkval*)(pu->sp + stki);
		fputmsg(stdout, "\tsp(%03d): {int32t(%d), flt32(%g), int64t(%D), flt64(%G)}\n", stki/4, stkv->i4, stkv->f4, stkv->i8, stkv->f8);
		//~ fputmsg(stdout, "\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G), p4f32(%g, %g, %g, %g), p4f64(%G, %G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
		//~ fputmsg(stdout, "!\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8);
		//~ sp = (stkval*)(((char*)sp) + 4);
	}
	//~ */
	fflush(stdout);
	return 0;
}

/*int exec_le(state env, unsigned cc, unsigned ss, int dbg) {
	//~ struct cell_t proc[32], *pu = proc;	// execution units
	unsigned long size = env->code.mems;
	unsigned char *mem = env->code.memp;
	unsigned char *end = mem + size;

	register bcde ip, li = 0, bi = 0;
	cell pu;
	//~ register unsigned char *sptr;//, *mp;
	unsigned i = 0;

	if (env->errc) {
		//~ fputmsg(stderr, "errors\n");
		return -1;
	}

	if (cc > 32) {
		error(env, "VM_ERR", 0, "cell overrun\n");
		return -1;
	}

	if (env->code.mins * 4 >= ss) {
		error(env, "VM_ERR", 0, "stack overrun\n");
		return -1;
	}

	if ((env->code.cs + env->code.ds + ss * cc) >= size) {
		error(env, "VM_ERR", 0, "memory overrun\n");
		return -1;
	}

	//~ env->code.pu = 0;
	for (i = 0; i < cc; i++) {
		end -= sizeof(struct cell_t) + ss;
		pu = (cell)end;
		pu->next = env->code.pu;
		env->code.pu = pu;
		//~ pu->zf = 0;
		//~ pu->sf = 0;
		//~ pu->cf = 0;
		//~ pu->of = 0;
		pu->sp = pu->bp + ss;
		pu->ip = 0;
	}

	pu->sp = pu->bp + ss;
	pu->ip = mem;//env->code.ip;		// start firt processor

	bi = li = (bcde)(mem + env->code.cs);

	for (env->code.ic = i = 0; ; env->code.ic += 1) {		// run forever
		//for (i = 0 ; i < cc; i++) {						// run each proc
		//~ pu = env->code.cell;		// get proc (should be in a queue)
		register unsigned char *sptr = pu->sp;			// stack
		//~ register unsigned char *sptop = pu->sp;			// stack
		//~ register unsigned char *spres = pu->sp;			// stack
		ip = (bcde)pu->ip;		// get instr

		if (ip && ip == bi) {			// break
			if (ip == li) break;		// stop
			//~ next brp
			//~ debug
		}
		switch (ip->opc) {				// exec
			error_opc : error(env, "VM_ERR", 0, "invalid opcode:[%02x]", ip->opc); return -1;
			error_ovf : error(env, "VM_ERR", 0, "stack overflow :op[%A]", ip); return -2;
			error_div : error(env, "VM_DBG", 0, "division by zero :op[%A]", ip); break;
			//~ error_mem : error("VM_ERR : %s\n", "segmentation fault"); return -5;
			//~ error_stc : error("VM_ERR : %s\n", "stack underflow"); return -3;
			//~ debug_ovf : error("\nVM_ERR : %s", "stack overflow"); break;
			#define CDBG
			#define FLGS	// 
			#define NEXT(__IP, __SP, __CHK) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			//~ #define MEMP(__MPTR) (mp = (__MPTR))
			#define STOP(__ERR) goto __ERR
			#define EXEC
			#include "incl/exec.inl"
		}
		if (dbg >= 0) {					// debug
			int stki, stkn = (pu->bp + ss - pu->sp);
			//~ if (stkn < dbg*4) stkn = dbg*4;
			//~ fputmsg(stdout, "exec: ss:[%03d] pu:(%02d) : %A\n", ssiz, i, ip);
			//~ fputmsg(stdout, ">exec:pu%02d:[ss:%03d zf:%d sf:%d cf:%d of:%d]: %A\n", i, ssiz, pu->zf, pu->sf, pu->cf, pu->of, ip);
			//~ fputmsg(stdout, ">exec:pu%02d:[ss:%03d, zf:%d sf:%d cf:%d of:%d]: %A\n", i, ssiz, pu->zf, pu->sf, pu->cf, pu->of, ip);
			fputmsg(stdout, ">exec:pu%02d:[ss:%03d]: %A\n", i, stkn/4, ip);
			if (dbg > 1) for (stki = 0; stki < stkn; stki += 4) {
				stkval* stkv = (stkval*)(pu->sp + stki);
				fputmsg(stdout, "\tsp(%03d): {int32t(%d), flt32(%g), int64t(%D), flt64(%G)}\n", stki/4, stkv->i4, stkv->f4, stkv->i8, stkv->f8);
				//~ fputmsg(stdout, "\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G), p4f32(%g, %g, %g, %g), p4f64(%G, %G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
				//~ fputmsg(stdout, "!\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8);
				//~ sp = (stkval*)(((char*)sp) + 4);
			}
			fflush(stdout);
		}
	}

	return 0;
}*/

int exec(state env, unsigned cc, unsigned ss, int dbg) {
	struct cell_t proc[256], *pu = proc;	// execution units
	unsigned long size = env->code.mems;
	unsigned char *mem = env->code.memp;
	unsigned char *end = mem + size;

	register bcde ip;
	bcde li = 0, bi = 0;

	if (env->errc) {
		//~ fputmsg(stderr, "errors\n");
		return -1;
	}

	if (ss < env->code.mins * 4) {
		error(env, "VM_ERR", 0, "stack overrun\n");
		return -1;
	}
	if (cc > sizeof(proc) / sizeof(*proc)) {
		error(env, "VM_ERR", 0, "cell overrun\n");
		return -1;
	}
	//~ TODO if ((env->code.cs + env->code.ds + ss * cc) >= size) {
	if ((env->code.cs + ss * cc) >= size) {
		error(env, "VM_ERR", 0, "`memory overrun\n");
		return -1;
	}

	for (; (signed)cc >= 0; cc -= 1) {
		pu = &proc[cc];
		//~ debug("%d", cc);
		pu->bp = end -= ss;

		pu->sp = 0;
		pu->ip = 0;
		//~ pu->zf = 0;
		//~ pu->sf = 0;
		//~ pu->cf = 0;
		//~ pu->of = 0;
	}

	pu = proc;
	pu->sp = pu->bp + ss;
	pu->ip = mem;//env->code.ip;		// start firt processor

	bi = li = (bcde)(mem + env->code.cs);

	for (/* env->code.ic = 0 */; ; /* env->code.ic += 1 */) {		// run forever
		pu = proc + 0;						// proc
		register unsigned char *sptr = pu->sp;			// stack
		//~ register unsigned char *mptr = NULL;			// memory

		//~ ticks += pu == proc;

		ip = (bcde)pu->ip;		// get instr

		if (ip && ip == bi) {			// break
			if (ip == li) break;		// stop
			//~ next brp
			//~ bi = dbg(s, proc, n)
		}
		switch (ip->opc) {			// exec
			error_opc : error(env, "VM_ERR", 0, "invalid opcode:[%02x]", ip->opc); return -1;
			error_ovf : error(env, "VM_ERR", 0, "stack overflow :op[%A]", ip); return -2;
			error_div : error(env, "VM_DBG", 0, "division by zero :op[%A]", ip); break;
			//~ error_mem : error("VM_ERR : %s\n", "segmentation fault"); return -5;
			//~ #define CDBG
			//~ #define FLGS
			#define NEXT(__IP, __SP, __CHK) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			//~ #define MEMP(__MPTR) (mp = (__MPTR))
			#define STOP(__ERR) goto __ERR
			#define EXEC
			#include "incl/exec.inl"
		}
		if (dbg >= 0) {				// dbg
			dbgCon(env, pu, pu - proc, ip, ss);
		}
	}
	if (dbg >= 0) {
		extern void vm_tags(state s, unsigned* sb);
		vm_tags(env, (unsigned*) pu->sp);
	}

	return 0;
}
//}
//{ core.c ---------------------------------------------------------------------
int srctext(state s, char *file, int line, char *text) {
	if (source(s, 1, text))
		return -1;
	s->file = file;
	s->line = line;
	return 0;
}
int srcfile(state s, char *file) {
	if (source(s, 0, file))
		return -1;
	return 0;
}
int logfile(state s, char* file) {
	if (s->logf)
		fclose(s->logf);
	s->logf = 0;
	if (file) {
		s->logf = fopen(file, "wb");
		if (!s->logf) return -1;
	}
	return 0;
}

int compile(state s, int mode) {
	if (s->errc) return -1;
	s->root = scan(s, mode);
	return s->errc;
}

int gencode(state s) {
	if (s->errc) return -1;

	s->code.memp = (unsigned char*)s->buffp;
	s->code.mems = bufmaxlen - (s->buffp - s->buffer);

	//~ s->code.ds = 
	s->code.cs = s->code.pc = 0;
	s->code.rets = s->code.mins = 0;
	//~ s->code.ic = 0;

	/* emit data seg
	for (sym = s->glob; sym; sym = sym->next) {
		if (sym->kind != TYPE_ref) continue;
		if (sym->init) {
			mem[s->code.ds] = sym->init;
		}
		s->code.ds += data->size;
		// TODO initialize
	}// */

	/* emit code seg
	for (ast = s->node; ast; ast = ast->next) {
		cgen(ast);
	}// */

	/*/ emit type inf
	for (data = s->glob; data; data = data->next) {
		if (data->kind != TYPE_def) continue;
		emit_typ(data);
		// TODO initialize
	}// */
	// emit dbug inf
	cgen(s, s->root, TYPE_any);

	return s->errc;
}

void* cc_malloc(state s, unsigned size) {//unsigned allign
	void* ret = s->buffp;
	s->buffp += size;
	return ret;
}
void* cc_calloc(state s, unsigned size) {//unsigned allign
	void* ret = s->buffp;
	memset(ret, 0, size);
	s->buffp += size;
	return ret;
}

void instint(state s, defn it, int64t sgn) {
	int size = it->size;
	int bits = size * 8;
	int64t mask = (-1LLU >> -bits);
	int64t minv = sgn << (bits - 1);
	int64t maxv = (minv - 1) & mask;
	enter(s, it);
	install(s, CNST_int, "min",  0)->init = intnode(s, minv);
	install(s, CNST_int, "max",  0)->init = intnode(s, maxv);
	install(s, CNST_int, "mask", 0)->init = intnode(s, mask);
	install(s, CNST_int, "bits", 0)->init = intnode(s, bits);
	install(s, CNST_int, "size", 0)->init = intnode(s, size);
	it->args = leave(s, 3);
}//~ */

//~ defn math_cls;

int cc_init(state s) {
	int TYPE_opc = TYPE_ref;
	defn typ, def;
	defn type_i08 = 0, type_i16 = 0;
	defn type_u08 = 0, type_u16 = 0;
	//~ defn type_bol = 0;//, type_bit = 0;
	//~ defn type_u64 = 0;//, type_f16 = 0;
	defn type_v4f = 0, type_v2d = 0;

	s->logf = 0; s->errc = 0;
	s->warn = wl; s->copt = ol;

	s->file = 0; s->line = s->nest = 0;

	s->_fin = s->_chr = -1;
	s->_ptr = 0; s->_cnt = 0;

	s->root = 0;
	s->defs = 0;

	s->buffp = s->buffer;
	s->deft = (defT)cc_calloc(s, sizeof(defn*) * TBLS);
	s->strt = (strT)cc_calloc(s, sizeof(list*) * TBLS);

	//{ install type
	//+ type_bit = install(s, "bit",  -1, TYPE_bit);
	//+ -> bitfields <=> array of bits ???

	type_vid = install(s, TYPE_vid, "void",  0);

	type_bol = install(s, TYPE_bit, "bool",  1);

	type_u08 = install(s, TYPE_uns, "uns08", 1);
	type_u16 = install(s, TYPE_uns, "uns16", 2);
	type_u32 = install(s, TYPE_uns, "uns32", 4);
	//~ type_u64 = install(s, TYPE_u64, "uns64", 8);

	type_i08 = install(s, TYPE_int, "int08", 1);
	type_i16 = install(s, TYPE_int, "int16", 2);
	type_i32 = install(s, TYPE_int, "int32", 4);
	type_i64 = install(s, TYPE_int, "int64", 8);

	//~ type_f16 = install(s, TYPE_flt, "flt16", 2);
	type_f32 = install(s, TYPE_flt, "flt32", 4);
	type_f64 = install(s, TYPE_flt, "flt64", 8);

	type_v4f = install(s, TYPE_flt, "v4f32", 16);
	type_v2d = install(s, TYPE_flt, "v2f64", 16);
	//~ type_v4f = install(s, TYPE_flt, "itn4f", 8);
	//~ type_v4f = install(s, TYPE_flt, "vec4f", 8);
	//~ type_v4f = install(s, TYPE_flt, "vec4f", 8);

	//~ type_ptr = install(s, TYPE_ptr, "pointer", 0);
	type_str = install(s, TYPE_ptr, "string", 0);

	//~ instint(s, type_i08, -0); instint(s, type_u08, -1);
	//~ instint(s, type_i16, -0); instint(s, type_u16, -1);
	//~ instint(s, type_i32, -0); instint(s, type_u32, -1);
	//~ instint(s, type_i64, -0);// instint(s, type_u64, -0);

	install(s, TYPE_def2, "int", 0)->type = type_i32;
	install(s, TYPE_def2, "long", 0)->type = type_i64;
	install(s, TYPE_def2, "float", 0)->type = type_f32;
	install(s, TYPE_def2, "double", 0)->type = type_f64;

	//} */// types
	//{ install Emit
	emit_opc = install(s, TYPE_ref, "emit", -1);
	//~ emit_opc->call = 1;
	//~ emit_opc->type = 0;
	enter(s, emit_opc);

	install(s, TYPE_opc, "nop", 0)->type = type_vid;

	enter(s, typ = install(s, TYPE_i32, "i32", 4));
	install(s, TYPE_opc, "neg", i32_neg)->type = type_i32;
	install(s, TYPE_opc, "add", i32_add)->type = type_i32;
	install(s, TYPE_opc, "sub", i32_sub)->type = type_i32;
	install(s, TYPE_opc, "mul", i32_mul)->type = type_i32;
	install(s, TYPE_opc, "div", i32_div)->type = type_i32;
	install(s, TYPE_opc, "mod", i32_mod)->type = type_i32;
	typ->args = leave(s, 3);

	enter(s, typ = install(s, TYPE_i64, "i64", 8));
	install(s, TYPE_opc, "neg", i64_neg)->type = type_i64;
	install(s, TYPE_opc, "add", i64_add)->type = type_i64;
	install(s, TYPE_opc, "sub", i64_sub)->type = type_i64;
	install(s, TYPE_opc, "mul", i64_mul)->type = type_i64;
	install(s, TYPE_opc, "div", i64_div)->type = type_i64;
	install(s, TYPE_opc, "mod", i64_mod)->type = type_i64;
	typ->args = leave(s, 3);

	enter(s, typ = install(s, TYPE_f32, "f32", 4));
	install(s, TYPE_opc, "neg", f32_neg)->type = type_f32;
	install(s, TYPE_opc, "add", f32_add)->type = type_f32;
	install(s, TYPE_opc, "sub", f32_sub)->type = type_f32;
	install(s, TYPE_opc, "mul", f32_mul)->type = type_f32;
	install(s, TYPE_opc, "div", f32_div)->type = type_f32;
	install(s, TYPE_opc, "mod", f32_mod)->type = type_f32;
	typ->args = leave(s, 3);

	enter(s, typ = install(s, TYPE_f64, "f64", 8));
	install(s, TYPE_opc, "neg", f64_neg)->type = type_f64;
	install(s, TYPE_opc, "add", f64_add)->type = type_f64;
	install(s, TYPE_opc, "sub", f64_sub)->type = type_f64;
	install(s, TYPE_opc, "mul", f64_mul)->type = type_f64;
	install(s, TYPE_opc, "div", f64_div)->type = type_f64;
	install(s, TYPE_opc, "mod", f64_mod)->type = type_f64;
	typ->args = leave(s, 3);

	enter(s, typ = install(s, TYPE_enu, "v4f", 16));
	install(s, TYPE_opc, "neg", v4f_neg)->type = type_v4f;
	install(s, TYPE_opc, "add", v4f_add)->type = type_v4f;
	install(s, TYPE_opc, "sub", v4f_sub)->type = type_v4f;
	install(s, TYPE_opc, "mul", v4f_mul)->type = type_v4f;
	install(s, TYPE_opc, "div", v4f_div)->type = type_v4f;
	install(s, TYPE_opc, "dp3", v4f_dp3)->type = type_f32;
	install(s, TYPE_opc, "dp4", v4f_dp4)->type = type_f32;
	install(s, TYPE_opc, "dph", v4f_dph)->type = type_f32;
	typ->args = leave(s, 3);

	enter(s, typ = install(s, TYPE_enu, "v2d", 16));
	install(s, TYPE_opc, "neg", v2d_neg)->type = type_v2d;
	install(s, TYPE_opc, "add", v2d_add)->type = type_v2d;
	install(s, TYPE_opc, "sub", v2d_sub)->type = type_v2d;
	install(s, TYPE_opc, "mul", v2d_mul)->type = type_v2d;
	install(s, TYPE_opc, "div", v2d_div)->type = type_v2d;
	typ->args = leave(s, 3);

	enter(s, typ = install(s, TYPE_enu, "swz", 2));
	{
		int i;
		defn swz;
		for (i = 0; i < 256; i += 1) {
			s->buffp[0] = "wxyz"[i>>0&3];
			s->buffp[1] = "wxyz"[i>>2&3];
			s->buffp[2] = "wxyz"[i>>4&3];
			s->buffp[3] = "wxyz"[i>>6&3];
			s->buffp[4] = 0;

			//~ printf("\t%c%c%c%c = 0x%02x;\n", c4, c3, c2, c1, i);
			swz = install(s, TYPE_opc, mapstr(s, s->buffp, 4, -1), p4d_swz);
			swz->init = intnode(s, i);
			swz->init->cast = 1;
			swz->type = typ;
		}
	}
	typ->args = leave(s, 3);

	emit_opc->args = leave(s, 3);

	//} */
	//{ install Math
		enter(s, def = install(s, TYPE_enu, "math", 0));

		install(s, CNST_flt, "Qnan", 0)->init = fh8node(s, 0x7FFFFFFFFFFFFFFFLL);
		install(s, CNST_flt, "Snan", 0)->init = fh8node(s, 0xfff8000000000000LL);
		install(s, CNST_flt, "inf", 0)->init = fh8node(s, 0x7ff0000000000000LL);
		install(s, CNST_flt, "l2e", 0)->init = fh8node(s, 0x3FF71547652B82FELL);	// log_2(e)
		install(s, CNST_flt, "l2t", 0)->init = fh8node(s, 0x400A934F0979A371LL);	// log_2(10)
		install(s, CNST_flt, "lg2", 0)->init = fh8node(s, 0x3FD34413509F79FFLL);	// log_10(2)
		install(s, CNST_flt, "ln2", 0)->init = fh8node(s, 0x3FE62E42FEFA39EFLL);	// log_e(2)
		install(s, CNST_flt, "pi", 0)->init = fh8node(s, 0x400921fb54442d18LL);		// 3.1415...
		install(s, CNST_flt, "e", 0)->init = fh8node(s, 0x4005bf0a8b145769LL);		// 2.7182...
		if ((typ = install(s, -1, "flt32 sin(flt32 x)", 0))) {
			typ->offs = 1; typ->call = 1; typ->libc = 1;
			//~ libcfnc[i].sym = fun;
			//~ libcfnc[i].sym->offs = i;
			//~ libcfnc[i].sym->libc = 1;
			//libcfnc[i].arg = argsize;
			//~ libcfnc[i].ret = fun->type->size;
			//~ libcfnc[i].pop = fun->size;
			//~ cc_libc(s, &libcfnc[i], i);
		}// * /
		else dieif(1);
		/* if ((typ = install(s, -1, "flt64 sin(flt64 x)", 0))) {
			typ->offs = 1; typ->call = 1; typ->libc = 1;
			//~ libcfnc[i].sym = fun;
			//~ libcfnc[i].sym->offs = i;
			//~ libcfnc[i].sym->libc = 1;
			//libcfnc[i].arg = argsize;
			//~ libcfnc[i].ret = fun->type->size;
			//~ libcfnc[i].pop = fun->size;
			//~ cc_libc(s, &libcfnc[i], i);
		}// */
		//~ else dieif(1);
		def->args = leave(s, 3);

		//~ enter(s, def = install(s, TYPE_enu, "libc", 0));
		//~ for (i = 0; i < sizeof(libcfnc) / sizeof(*libcfnc); i += 1) {
			//~ defn fun = instlibc(s, -1, libcfnc[i].proto,0);
			//~ libcfnc[i].sym = fun;
			//~ libcfnc[i].sym->offs = i;
			//~ libcfnc[i].sym->libc = 1;
			//~ //libcfnc[i].arg = argsize;
			//~ libcfnc[i].ret = fun->type->size;
			//~ libcfnc[i].pop = fun->size;
			//- cc_libc(s, &libcfnc[i], i);
		//~ }
		//~ leave(s, 3, &def->args);
	//} */
	/*{ install ccon
	enter(s, typ = install(s, TYPE_enu, "ccon", 0));
	install(s, CNST_int, "version", 0)->init = intnode(s, 0x20090400);
	install(s, CNST_str, "host", 0)->init = strnode(s, (char*)os);
	//? install(s, TYPE_def, "emit", 0);// warn(int, void ...);
	//? install(s, TYPE_def, "warn", 0);// warn(int, string);

	//- install(s, CNST_str, "type", 0);// current type;
	//~ install(s, CNST_str, "date", 0)->init = &s->ccd;
	//~ install(s, CNST_str, "file", 0)->init = &s->ccfn;
	//~ install(s, CNST_str, "line", 0)->init = &s->ccfl;
	//~ install(s, CNST_str, "time", 0)->init = &s->ccft;
	//~ install(s, CNST_int, "bits", 32)->init = intnode(s, 32);
	typ->args = leave(s, 3);

	//} */
	/*{ install operators
	install(s, -1, "int32 add(int32 a, uns32 b) {emit(add.i32, i32(a), i32(b));}", 0);
	install(s, -1, "int64 add(int32 a, uns64 b) {emit(add.i64, i64(a), i64(b));}", 0);
	install(s, -1, "int32 add(int32 a, int32 b) {emit(add.i32, i32(a), i32(b));}", 0);
	install(s, -1, "int64 add(int32 a, int64 b) {emit(add.i64, i64(a), i64(b));}", 0);
	install(s, -1, "flt32 add(int32 a, flt32 b) {emit(add.f32, f32(a), f32(b));}", 0);
	install(s, -1, "flt64 add(int32 a, flt64 b) {emit(add.f64, f64(a), f64(b));}", 0);

	install(s, -1, "int32 add(out int32 a, uns32 b) {return a = add(a, b)}", 0);
	install(s, -1, "int64 add(out int32 a, uns64 b) {return a = add(a, b)}", 0);
	install(s, -1, "int32 add(out int32 a, int32 b) {return a = add(a, b)}", 0);
	install(s, -1, "int64 add(out int32 a, int64 b) {return a = add(a, b)}", 0);
	install(s, -1, "flt32 add(out int32 a, flt32 b) {return a = add(a, b)}", 0);
	install(s, -1, "flt64 add(out int32 a, flt64 b) {return a = add(a, b)}", 0);

	install(s, -1, "int32 add(int32 a, uns32 b) {emit(i32.add, i32(a), i32(b));}", 0);	// +
	install(s, -1, "int32 sub(int32 a, uns32 b) {emit(i32.sub, i32(a), i32(b));}", 0);	// -
	install(s, -1, "int32 mul(int32 a, uns32 b) {emit(i32.mul, i32(a), i32(b));}", 0);	// *
	install(s, -1, "int32 div(int32 a, uns32 b) {emit(i32.div, i32(a), i32(b));}", 0);	// /
	install(s, -1, "int32 mod(int32 a, uns32 b) {emit(i32.mod, i32(a), i32(b));}", 0);	// %

	install(s, -1, "int32 ceq(int32 a, uns32 b) {emit(b32.ceq, i32(a), i32(b));}", 0);	// ==
	install(s, -1, "int32 cne(int32 a, uns32 b) {emit(b32.cne, i32(a), i32(b));}", 0);	// !=
	install(s, -1, "int32 clt(int32 a, uns32 b) {emit(i32.clt, i32(a), i32(b));}", 0);	// <
	install(s, -1, "int32 cle(int32 a, uns32 b) {emit(i32.cle, i32(a), i32(b));}", 0);	// <=
	install(s, -1, "int32 cgt(int32 a, uns32 b) {emit(i32.cgt, i32(a), i32(b));}", 0);	// >
	install(s, -1, "int32 cge(int32 a, uns32 b) {emit(i32.cge, i32(a), i32(b));}", 0);	// >=

	install(s, -1, "int32 shr(int32 a, uns32 b) {emit(b32.shl, i32(a), i32(b));}", 0);	// >>
	install(s, -1, "int32 shl(int32 a, uns32 b) {emit(b32.sar, i32(a), i32(b));}", 0);	// <<
	install(s, -1, "int32 shl(int32 a, uns32 b) {emit(b32.and, i32(a), i32(b));}", 0);	// &
	install(s, -1, "int32 shl(int32 a, uns32 b) {emit(b32. or, i32(a), i32(b));}", 0);	// |
	install(s, -1, "int32 shl(int32 a, uns32 b) {emit(b32.xor, i32(a), i32(b));}", 0);	// ^

	install(s, -1, "int32 pls(int32 a) asm {emit(nop, int32(a));}", 0);	// +
	install(s, -1, "int32 neg(int32 a) asm {emit(neg.i32, i32(a));}", 0);	// -
	install(s, -1, "int32 cmt(int32 a) asm {emit(cmt.b32, i32(a));}", 0);	// ~
	install(s, -1, "int32 not(int32 a) asm {emit(i32.bit.any, i32(a), i32(0));}", 0);	// !

	//} */
	return 0;
}

int cc_done(state s) {
	if (s->nest)
		error(s, s->file, s->line, "premature end of file");
	s->nest = 0;
	source(s, 0, NULL);
	return s->errc;
}

/** print symbols
 * output looks like:
 * file:line:kind:?prot:name:type
 * file: is the file name of decl
 * line: is the line number of decl
 * kind:
 * 	'^' : typename(dcl)
 * 	'#' : constant(def)
 * 	'$' : variable(ref)
 *+	'$' : function?
 *+	'*' : operator?
 *+prot: protection level
 * 	'+' : public
 * 	'-' : private
 * 	'=' : protected?
 * 
**/
void dumpsym(FILE *fout, defn sym) {
	int tch;
	defn ptr, bs[TOKS], *ss = bs;
	defn *tp, bp[TOKS], *sp = bp;
	for (*sp = sym; sp >= bp;) {
		if (!(ptr = *sp)) {
			--sp, --ss;
			continue;
		}
		*sp = ptr->next;

		//~ if (!line) continue;

		switch (ptr->kind) {

			// constant
			case CNST_int :
			case CNST_flt :
			case CNST_str : tch = '#'; break;

			// variable/function
			case TYPE_ref : tch = '$'; break;

			// typename
			case TYPE_vid :
			case TYPE_bit :
			case TYPE_uns :
			case TYPE_u32 :
			case TYPE_int :
			case TYPE_i32 :
			case TYPE_i64 :
			case TYPE_flt :
			case TYPE_f32 :
			case TYPE_f64 :
			case TYPE_ptr :
			case TYPE_enu :
			case TYPE_rec :
				*++sp = ptr->args;
				*ss++ = ptr;

			case TYPE_def2 :
				tch = '^';
				break;

			case EMIT_opc :
				*++sp = ptr->args;
				*ss++ = ptr;
				tch = '@';
				break;

			default :
				debug("psym:%d:%T", ptr->kind, ptr);
				tch = '?';
				break;
		}
		if (ptr->file && ptr->line)
			fputmsg(fout, "%s:%d:", ptr->file, ptr->line);

		fputc(tch, fout);

		for (tp = bs; tp < ss; tp++) {
			defn typ = *tp;
			if (typ != ptr && typ && typ->name)
				fputmsg(fout, "%s.", typ->name);
		}// */

		fputmsg(fout, "%?s", ptr->name);

		/*if (ptr->kind == EMIT_opc) {
			fputmsg(fout, ":%T", ptr->type);
			fputmsg(fout, " %02x", ptr->offs);
			if (ptr->init) switch (ptr->init->cast) {
				case 1 : fputmsg(fout, " %002x", ptr->init->cint & 0xff); break;
				case 2 : fputmsg(fout, " %004x", ptr->init->cint & 0xffff); break;
				case 3 : fputmsg(fout, " %008x", ptr->init->cint & 0xffffff); break;
				default: break;
			}
			fputc('\n', fout);
			continue;
		}// */

		if (ptr->kind == TYPE_ref && ptr->call) {
			defn arg = ptr->args;
			fputc('(', fout);
			while (arg) {
				fputmsg(fout, "%?s", arg->type->name);
				if (arg->name) fputmsg(fout, " %s", arg->name);
				if (arg->init) fputmsg(fout, "= %+k", arg->init);
				if ((arg = arg->next)) fputs(", ", fout);
			}
			fputc(')', fout);
		}

		if (ptr->kind == TYPE_ref) {
			fputmsg(fout, "@%xh", ptr->size);
		}
		else if (ptr->kind == EMIT_opc) {
			fputmsg(fout, "@%xh", ptr->size);
		}
		else {
			fputmsg(fout, ":%d", ptr->size);
		}


		if (ptr->type) {
			fputmsg(fout, ":%T", ptr->type);
			if (ptr->init)
				fputmsg(fout, " = %T(%+k)", ptr->type, ptr->init);
		}
		fputc('\n', fout);
		fflush(fout);
	}
}
void dumpasm(state s, FILE *fout, int mode) {
	unsigned st = 0, i, is, n = 1, offs = 0;
	unsigned char* beg = s->code.memp + offs;
	unsigned len = s->code.cs - offs;

	for (i = 0; i < len; n++) {
		int ofs = i;
		bcde ip = (bcde)(beg + ofs);
		switch (ip->opc) {
			error_opc : error(s, "VM_ERR", 0, "invalid opcode:[%02x]", ip->opc); return;
			#define NEXT(__IP, __SP, __CHK) i += is = (__IP); st += (__SP);
			#define STOP(__ERR) goto __ERR
			#include "incl/exec.inl"
		}
		//~ fputmsg(fout, "%3d .0x%04x[%04d](%d) %09A\n", n, ofs, st, is, ip);
		fputmsg(fout, "SP[%04d]", st);
		fputasm(fout, ip, 9, ofs); fputc('\n', fout);
	}
}
void dumpxml(FILE *fout, node ast, int lev, const char* text, int level) {
	if (ast) switch (ast->kind) {
		//{ STMT
		case OPER_nop : {
			fputmsg(fout, "%I<%s id='%d' line='%d' stmt='%?+k'>\n", lev, text, ast->kind, ast->line, ast->stmt);
			dumpxml(fout, ast->stmt, lev+1, "expr", level);
			fputmsg(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_beg : {
			node l = ast->stmt;
			fputmsg(fout, "%I<%s id='%d' line='%d' stmt='{}'>\n", lev, text, ast->kind, ast->line);
			for (l = ast->stmt; l; l = l->next)
				dumpxml(fout, l, lev + 1, "stmt", level);
			fputmsg(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_jmp : {
			fputmsg(fout, "%I<%s id='%d' line='%d' stmt='if (%+k)'>\n", lev, text, ast->kind, ast->line, ast->test);
			dumpxml(fout, ast->test, lev + 1, "test", level);
			dumpxml(fout, ast->stmt, lev + 1, "then", level);
			dumpxml(fout, ast->step, lev + 1, "else", level);
			fputmsg(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_for : {
			//~ fputmsg(fout, "%I<stmt id='%d' stmt='for (%?+k; %?+k; %?+k)'>\n", lev, ast->kind, ast->init, ast->test, ast->step);
			fputmsg(fout, "%I<%s id='%d' line='%d' stmt='for (%?+k; %?+k; %?+k)'>\n", lev, text, ast->kind, ast->line, ast->init, ast->test, ast->step);

			dumpxml(fout, ast->init, lev + 1, "init", level);
			dumpxml(fout, ast->test, lev + 1, "test", level);
			dumpxml(fout, ast->step, lev + 1, "step", level);
			dumpxml(fout, ast->stmt, lev + 1, "stmt", level);
			fputmsg(fout, "%I</stmt>\n", lev);
		} break;
		//}

		//{ OPER
		case OPER_fnc : {		// '()'
			node arg = ast->rhso;
			switch (level) {
				case 3 : fputmsg(fout, "%I<%s id='%d' cast='%s' type='%?T' oper='%?k' expr='%?+k'>\n", lev, text, ast->kind, tok_tbl[ast->cast].name, ast->type, ast, ast); break;
				case 2 : fputmsg(fout, "%I<%s id='%d' type='%?T' oper='%?k'>\n", lev, text, ast->kind, ast->type, ast); break;
				case 1 : fputmsg(fout, "%I<%s id='%d' oper='%?k'>\n", lev, text, ast->kind, ast); break;
			}
			while (arg && arg->kind == OPER_com) {
				dumpxml(fout, arg->rhso, lev + 1, "push", level);
				arg = arg->lhso;
			}
			dumpxml(fout, arg, lev + 1, "push", level);
			dumpxml(fout, ast->lhso, lev + 1, "call", level);
			fputmsg(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_dot : 		// '.'
		case OPER_idx : 		// '[]'

		case OPER_pls : 		// '+'
		case OPER_mns : 		// '-'
		case OPER_cmt : 		// '~'
		case OPER_not : 		// '!'

		//~ case OPER_pow : 		// '**'
		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : 		// '%'

		case OPER_shl : 		// '>>'
		case OPER_shr : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_ior : 		// '|'
		case OPER_xor : 		// '^'

		case OPER_equ : 		// '=='
		case OPER_neq : 		// '!='
		case OPER_lte : 		// '<'
		case OPER_leq : 		// '<='
		case OPER_gte : 		// '>'
		case OPER_geq : 		// '>='

		case OPER_lnd : 		// '&&'
		case OPER_lor : 		// '||'
		//~ case OPER_sel : 		// '?:'
		//~ case OPER_min : 		// '?<'
		//~ case OPER_max : 		// '?>'

		//~ case ASGN_pow : 		// '**='
		//~ case ASGN_add : 		// '+='
		//~ case ASGN_sub : 		// '-='
		//~ case ASGN_mul : 		// '*='
		//~ case ASGN_div : 		// '/='
		//~ case ASGN_mod : 		// '%='

		//~ case ASGN_shl : 		// '>>='
		//~ case ASGN_shr : 		// '<<='
		//~ case ASGN_and : 		// '&='
		//~ case ASGN_ior : 		// '|='
		//~ case ASGN_xor : 		// '^='

		case ASGN_set : {		// '='
			switch (level) {
				case 4 : fputmsg(fout, "%I<%s id='%d' oper='%?k' stmt='%?+k' type='%?T' cast='%s'>\n", lev, text, ast->kind, ast, ast, ast->type, tok_tbl[ast->cast].name); break;
				case 3 : fputmsg(fout, "%I<%s id='%d' oper='%?k' stmt='%?+k' type='%?T'>\n", lev, text, ast->kind, ast, ast, ast->type); break;
				case 2 : fputmsg(fout, "%I<%s id='%d' oper='%?k' stmt='%?+k'>\n", lev, text, ast->kind, ast, ast); break;
				case 1 : fputmsg(fout, "%I<%s id='%d' oper='%?k'>\n", lev, text, ast->kind, ast); break;
			}

			dumpxml(fout, ast->lhso, lev + 1, "lval", level);
			dumpxml(fout, ast->rhso, lev + 1, "rval", level);
			fputmsg(fout, "%I</%s>\n", lev, text, ast->kind, ast->type, ast);
		} break;
		//}*/

		//{ TVAL
		case EMIT_opc : 
		case CNST_int : 
		case CNST_flt : 
		case TYPE_ref :
		case TYPE_def2 : 
		case CNST_str : {
			//~ case 3 : fputmsg(fout, "%I<%s id='%d' cast='%s' type='%?T' oper='%?k' expr='%?+k'>\n", lev, text, ast->kind, tok_tbl[ast->cast].name, ast->type, ast, ast); break;
			//~ case 2 : fputmsg(fout, "%I<%s id='%d' type='%?T' oper='%?k'>\n", lev, text, ast->kind, ast->type, ast); break;
			//~ case 1 : fputmsg(fout, "%I<%s id='%d' oper='%?k'>\n", lev, text, ast->kind, ast); break;
			switch (level) {
				case 4 : 
				case 3 : 
				case 2 : fputmsg(fout, "%I<%s id='%d' type='%?T'>%k</%s>\n", lev, text, ast->kind, ast->type, ast, text); break;
				case 1 : fputmsg(fout, "%I<%s id='%d'>%k</%s>\n", lev, text, ast->kind, ast, text); break;
			}
			//~ TYPE_def : dumpxml(fout, ast->rhso, lev + 1, text, level);
		} break;
		//}

		default: fatal(s, "%s / %k", tok_tbl[ast->kind].name, ast); break;
	}
	else ;
}
void dumpast(FILE *fout, node ast, int mode) {
	//~ TODO: dumpast(state s, FILE *fout, int mode)
	if (mode > 0)
		dumpxml(fout, ast, 0, "root", mode);
	else
		fputmsg(fout, "%-k", ast);
}
//}
//{ misc.c ---------------------------------------------------------------------
#if 0
int vm_hgen() {
	int i, e = 0;
	FILE *out = stdout;
	for (i = 0; i < opc_last; i++) {
		int err = 0;
		//~ if (opc_tbl[i].size <= 0 || !opc_tbl[i].name) continue;

		fprintf(out, "\nInstruction: %s: #\n", opc_tbl[i].name);
		fprintf(out, "Opcode		argsize		Stack trasition\n");
		fprintf(out, "0x%02x		%d		[..., a, b, c, d => [..., a, b, c, d#\n", opc_tbl[i].code, opc_tbl[i].size-1);

		fprintf(out, "\nDescription\n");
		if (opc_tbl[i].code != i) {
			e += err = 1;
		}
		else switch (i) {
			error_opc : e += err = 1; break;
			#define NEXT(__IP, __SP, __CHK) if (opc_tbl[i].size != (__IP)) goto error_opc;
			#include "exec.inl"
		}
		if (err)
		fprintf(out, "The '%s' instruction %s\n", opc_tbl[i].name, err ? "is invalid" : "#");

		fprintf(out, "\nOperation\n");
		fprintf(out, "#\n");

		fprintf(out, "\nExceptions\n");
		fprintf(out, "None#\n");

		fprintf(out, "\n");		// end of text
	}
	return e;
}

unsigned i32CmpF(int32t lhs, int32t rhs) {
	int32t res = lhs - rhs;
	int sf = res < 0;
	int zf = res == 0;
	int cf = (unsigned)res < (unsigned)lhs;
	int of = ((rhs ^ lhs) >> 31) != (res >> 31);
	return (((((of << 1) | sf) << 1) | zf) << 1) | cf;
}
unsigned f32CmpF(flt32t lhs, flt32t rhs) {
	flt32t res = lhs - rhs;
	int sf = res < 0;
	int zf = res == 0;
	int cf = 0;
	//~ int of = ((rhs ^ lhs) >> 31) != (res >> 31);
	//~ OF = NAN || INF || -INF;
	int of = res != res || res == res + 1 || res == res - 1;
	return (((((of << 1) | sf) << 1) | zf) << 1) | cf;
}

int mk_test(char *file, int size) {
	//~ char test[] = "signed _000000000000000001 = 8;\n";
	char test[] = "int _0000001=6;\n";
	FILE* f = fopen(file, "wb");
	int n, sp = sizeof(test) - 6;

	if (!f) {
		debug("cann not open file\n");
		return -1;
	}

	if (size <= (128 << 20)) {
		for (n = 0; n < size; n += sizeof(test)-1) {
			fputs(test, f);
			while (++test[sp] > '9') {
				test[sp--] = '0';
				if (test[sp] == '_') {
					fclose(f);
					return -2;
				}
			}
			sp = sizeof(test) - 6;
		}
	}
	else debug("file is too large (128M max)\n");

	fclose(f);
	return 0;
}
#endif

void vm_tags(state s, unsigned* sb) {
	defn ptr;
	FILE *fout = stdout;
	//~ unsigned* sb = (unsigned*)s->code.pu->sp;
	for (ptr = s->glob; ptr; ptr = ptr->next) {
		if (ptr->kind == TYPE_ref && ptr->onst) {
			int spos = s->code.rets - ptr->offs;
			stkval* sp = (stkval*)(sb + spos);
			int line = ptr->link->line;
			defn typ = ptr->type;
			if (s->file && line)
				fputmsg(fout, "%s:%d:", s->file, line);
			fputmsg(fout, "sp(0x%02x):%s", spos, ptr->name);
			switch(typ ? typ->kind : 0) {
				default :
				TYPE_XXX:
					fputmsg(fout, " = ????\n");
					break;
				case TYPE_bit :
				case TYPE_uns : switch (typ->size) {
					case 1 : fputmsg(fout, " = u08[%008x](%u)\n", sp->i4, sp->i4); break;
					case 2 : fputmsg(fout, " = u16[%008x](%u)\n", sp->i4, sp->i4); break;
					case 4 : fputmsg(fout, " = u32[%008x](%u)\n", sp->i4, sp->i4); break;
					case 8 : fputmsg(fout, " = u64[%016X](%U)\n", sp->i8, sp->i8); break;
					default : goto TYPE_XXX;
				} break;
				case TYPE_int : switch (typ->size) {
					case 1 : fputmsg(fout, " = i08[%008x](%d)\n", sp->i4, sp->i4); break;
					case 2 : fputmsg(fout, " = i16[%008x](%d)\n", sp->i4, sp->i4); break;
					case 4 : fputmsg(fout, " = i32[%008x](%d)\n", sp->i4, sp->i4); break;
					case 8 : fputmsg(fout, " = i64[%016X](%D)\n", sp->i8, sp->i8); break;
					default : goto TYPE_XXX;
				} break;
				case TYPE_flt : switch (typ->size) {
					//~ case 1 : fputmsg(fout, " = uns32[%08d](%u)\n", sp->i4, sp->i4); break;
					//~ case 2 : fputmsg(fout, " = uns32[%08d](%u)\n", sp->i4, sp->i4); break;
					case 4 : fputmsg(fout, " = f32[%008x](%g)\n", sp->i4, sp->i4); break;
					case 8 : fputmsg(fout, " = f64[%016X](%G)\n", sp->i8, sp->i8); break;
					default : goto TYPE_XXX;
				} break;
			}
		}
	}
}

//} */

int dbg_eval(state s, char* text) {
	struct astn_t res;
	node ast;
	defn typ;
	int tid;
	cc_init(s);

	if (source(s, 1, text)) {
		return -1;				// this never happens ???
	}

	ast = expr(s, 0);
	debug("expr := `%+k`", ast);
	typ = lookup(s, 0, ast);
	//~ fputmsg("type := `%-k`", ast);
	//~ fputast(stdout, ast, -1, 0xf); fputc('\n', stdout);
	dumpast(stdout, ast, 1);
	tid = eval(&res, ast, 0);
	if (!ast || !typ || !tid) {
		fputmsg(stdout, "ERROR\n", exp, &res);
		//~ return -2;
	}

	debug("eval(`%+k`) = %T(%k)", ast, typ, &res, tok_tbl[tid].name);

	return 0;
}

void dbg_info(state s) {
	int i;
	i = s->buffp - s->buffer;
	printf("parse mem: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
	//~ i = s->code.ds; printf("data size: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
	i = s->code.cs;
	printf("code size: %dM, %dK, %dB, %d instructions\n", i >> 20, i >> 10, i, -1/*s->code.ic*/);
	i = s->code.mins * 4;
	printf("max stack: %dM, %dK, %dB, %d slots\n", i >> 20, i >> 10, i, s->code.mins);
	//~ printf("exec inst: %d\n", s->code.ic);
}

int dbg_file(state s, char* src) {

	int ret;
	time_t time;

	cc_init(s);

	ret = srcfile(s, src);

	time = clock();
	ret = compile(s, 0);
	time = clock() - time;
	printf(">scan: Exit code: %d\tTime: %lg\n", ret, (double)time / CLOCKS_PER_SEC);

	//~ printf(">ast:ccc\n"); dumpast(stdout, s->root, 0);
	//~ printf(">ast:xml\n"); dumpast(stdout, s->root, 1);

	time = clock();
	ret = gencode(s);
	time = clock() - time;
	printf(">cgen: Exit code: %d\tTime: %lg\n", ret, (double)time / CLOCKS_PER_SEC);

	time = clock();
	ret = exec(s, cc, ss, 0);
	time = clock() - time;
	printf(">exec: Exit code: %d\tTime: %lg\n", ret, (double)time / CLOCKS_PER_SEC);

	//~ dbg_info(s);
	//~ printf(">tags:unit\n"); dumpsym(stdout, s->glob);
	//~ printf(">tags:lstd\n"); dumpsym(stdout, leave(s, -1));
	//~ printf(">tags:emit\n"); dumpsym(stdout, emit_opc->args);
	//~ printf(">dasm:unit\n"); dumpasm(stdout, s);
	//~ printf(">astc:unit\n"); dumpast(stdout, s->root, 0);
	//~ printf(">xmlc:unit\n"); dumpast(stdout, s->root, 2);

	return ret;
}// */

int dbg_test(state s) {
	//~ int c0, c1, c2;
	cc_init(s);
	srctext(s, __FILE__, __LINE__ + 2,
		//~ "!flt32(2. + (3 + 1));\n"			//>re = a.re * b.re - a.im * b.im
		"emit(f32.neg, f32(emit(f32.neg, f32(2))));\n"					//>re = a.re * b.re - a.im * b.im
		//~ "emit(f64.div, f64(2), flt64(3.14));\n"			//>re = a.re * b.re - a.im * b.im

		//~ "flt64 re = 3, im = 4;\n"		// -14, 23
		//~ "flt64 r2 = 2, i2 = 5;\n"
		//~ "emit(p4d.mul, flt64(i2), flt64(r2), flt64(im), flt64(re));\n"	// tmp1,2 = a.re * b.re, a.im * b.im
		//~ "emit(p4d.mul, flt64(r2), flt64(i2), flt64(im), flt64(re));\n"	// tmp3,4 = a.re * b.im, a.im * b.re
		//~ "im = emit(f64.add);\n"			//>im = a.re * b.im + a.im * b.re
		//~ "re = emit(f64.sub);\n"			//>re = a.re * b.re - a.im * b.im
	);// */

	//~ printf("Stack max = %d / %d slots\n", s->code.mins * 4, s->code.mins);
	//~ printf("Code size = %d / %d instructions\n", s->code.cs, s->code.ic);
	compile(s, 0);
	gencode(s);

	//~ emitf64(s, opc_ldcF, 3.14);
	//~ emitf64(s, opc_ldcF, 2.);
	//~ emit(s, f64_div);

	//~ emitf32(s, opc_ldcf, 3.14);
	//~ emitf32(s, opc_ldcf, 2);
	//~ emit(s, f32_div);

	//~ emitidx(s, opc_dup4, c2);
	//~ emitopc(s, p4d_mul);
	//~ emitopc(s, f64_sub);
	//~ dumpasm(stdout, s);

	//~ dbg_exec(s, cc, ss, 2);

	return cc_done(s);
}

//~ #define USE_COMPILED_CODE
//~ #include "incl/test.c"

void usage(state s, char* name, char* cmd) {
	int n = 0;
	//~ char *pn = strrchr(name, '/');
	//~ if (pn) name = pn+1;
	if (strcmp(cmd, "-api") == 0) {
		defn api;
		cc_init(s);
		api = leave(s, 3);
		dumpsym(stdout, api);
		return ;
	}
	if (cmd == NULL) {
		puts("Usage : program command [options] ...");
		puts("where command can be one of:");
		puts("\t-c: compile");
		puts("\t-e: execute");
		puts("\t-m: make");
		puts("\t-h: help");
		//~ puts("\t-d: dump");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-c") == 0) {
		//~ puts("compile : %s -c [options] files...", name);
		puts("compile : psvm -c [options] files...");
		puts("Options :");

		puts("\t[Output to]");
		puts("\t-o <file> set file for output.");
		puts("\t-l <file> set file for errors.");

		puts("\t[Output is]");
		puts("\t-t tags");
		puts("\t-s code");
		puts("\t-(ast|xml) -- debug mode only");

		puts("\t[Errors & Warnings]");
		//~ puts("\t-w<num> set warning level to <num> [default=%d]", wl);
		puts("\t-w<num> set warning level to <num>");
		puts("\t-wa all warnings");
		puts("\t-wx treat warnings as errors");

		//~ puts("\t-x<num> run");
		//~ puts("\t-xd<num> run");

		//~ puts("\t[Debug & Optimization]");
		//~ puts("\t-d0 generate no debug information");
		//~ puts("\t-d1 generate debug {line numbers}");
		//~ puts("\t-d2 generate debug {-d1 + Symbols}");
		//~ puts("\t-d3 generate debug {-d2 + Symbols (all)}");
		//~ puts("\t-O<num> set optimization level. [default=%d]", dol);
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-e") == 0) {
		puts("command : '-e' : execute\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-d") == 0) {
		puts("command : '-d' : disassemble\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-m") == 0) {
		puts("command : '-m' : make\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-h") == 0) {
		puts("command : '-h' : help\n");
		n += 1;
	}
	if (!n)
		puts("invalid command\n");
}

int prog(state s, int argc, char *argv[]) {
	if (argc < 3) usage(s, argv[0], argv[1]);
	else if (strcmp(argv[1], "=") == 0) {	// eval
		return dbg_eval(s, argv[2]);
	}
	else if (strcmp(argv[1], "-c") == 0) {	// compile
		FILE *fout = stdout;
		int level = -1, arg;
		int warn = wl;
		//~ int optl = ol;
		//~ int genc = 0;
		int outc = 0;			// output type
		char *logf = 0;			// logger
		char *outf = 0;			// output
		char *srcf = 0;			// source
		enum {
			//~ gen_bin = 0x0010,	// cgen

			out_tag = 0x0001,	// tags
			out_asm = 0x0011,	// dasm

			//~ out_bin = 0x0012,	// dump
			run_bin = 0x0012,	// exec
			dbg_bin = 0x0013,	// exec

			out_ast = 0x0003,	// walk
		};

		// options
		for (arg = 2; arg < argc; arg += 1) {
			if (*argv[arg] != '-') {
				if (srcf) {
					fputmsg(stderr, "multiple sources not suported\n");
					return -1;
				}
				srcf = argv[arg];
			}

			// output file
			else if (strcmp(argv[arg], "-l") == 0) {		// log
				if (++arg >= argc || logf) {
					fputmsg(stderr, "logger error\n");
					return -1;
				}
				logf = argv[arg];
			}
			else if (strcmp(argv[arg], "-o") == 0) {		// out
				if (++arg >= argc || outf) {
					fputmsg(stderr, "output error\n");
					return -1;
				}
				outf = argv[arg];
			}

			// output as
			else if (strcmp(argv[arg], "-t") == 0) {		// tags
				outc = out_tag;
			}
			else if (strcmp(argv[arg], "-s") == 0) {		// asm
				outc = out_asm;
			}
			else if (strncmp(argv[arg], "-C", 2) == 0) {		// ast
				switch (argv[arg][2]) {
					case  0  : level = 1; break;
					case '0' : level = 0; break;
					case '1' : level = 1; break;
					case '2' : level = 2; break;
					case '3' : level = 3; break;
					default  : 
						fputmsg(stderr, "invalid level '%c'\n", argv[arg][2]);
						level = 0;
						break;
				}
				outc = out_ast;
			}

			// Override settings
			else if (strncmp(argv[arg], "-w", 2) == 0) {		// warning level
				if (!argv[arg][3]) switch (argv[arg][2]) {
					case '0' : warn = 0; break;
					case '1' : warn = 1; break;
					case '2' : warn = 2; break;
					case '3' : warn = 3; break;
					case '4' : warn = 4; break;
					case '5' : warn = 5; break;
					case '6' : warn = 6; break;
					case '7' : warn = 7; break;
					case '8' : warn = 8; break;
					case '9' : warn = 9; break;
					case 'a' : warn = 9; break;		// all
					case 'x' : warn = -1; break;	// threat as errors
					default  :
						fputmsg(stderr, "invalid warning level '%c'\n", argv[arg][2]);
						break;
				}
				else fputmsg(stderr, "invalid warning level '%s'\n", argv[arg]);
			}
			/*else if (strncmp(argv[arg], "-d", 2) == 0) {		// optimize/debug level
				return -1;
			}*/

			else if (strncmp(argv[arg], "-x", 2) == 0) {		// execute
				char *str = argv[arg] + 2;
				outc = run_bin;
				if (*str == 'd') {
					str ++;
					outc = dbg_bin;
				}
				while (*str) {
					char c = *str;
					if (c >= '0' && c <= '9')
						c -= '0';
					level = level * 10 + c;
				}
				//~ dbgl = atoi()
			}

			else {
				fputmsg(stderr, "invalid option '%s' for -compile\n", argv[arg]);
				return -1;
			}
		}

		if (!srcf) {
			fputmsg(stderr, "no input file\n");
			return -1;
		}


		cc_init(s);
		s->warn = warn;
		/*if (source(s, 0, "lstd.cxx") != 0) {
			fputmsg(stderr, "can not open standard library `%s`\n", "lstd.ccc");
			exit(-1);
		}
		scan(s, 0);
		*/
		if (logfile(s, logf) != 0) {
			fputmsg(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (srcfile(s, srcf) != 0) {
			fputmsg(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (compile(s, 0) != 0) {
			return s->errc;
		}

		// output
		if (outf) {
			fout = fopen(outf, "wb");
			if (fout == NULL) {
				fputmsg(stderr, "can not open file `%s`\n", outf);
			}
		}

		switch (outc) {
			//~ case out_pch : dumppch(s, outf); break;
			//~ case out_bin : dumpbin(s, outf); break;

			case out_tag :
				dumpsym(fout, s->glob);
				break;

			case out_ast :
				if (level > 1)
					gencode(s);
				dumpast(fout, s->root, level);
				break;

			case out_asm :
				gencode(s);
				dumpasm(s, fout, 0);
				break;

			case run_bin :		// exec
				gencode(s);
				exec(s, cc, ss, 0);
				break;

			case dbg_bin :		// dbug
				gencode(s);
				exec(s, cc, ss, 1);
				break;
		}
		if (outf) {
			fclose(fout);
		}
		return cc_done(s);
	}
	else if (strcmp(argv[1], "-e") == 0) {	// execute
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
		//~ objfile(s, ...);
		//~ return exec(s, cc, ss, dbgl);
	}
	else if (strcmp(argv[1], "-d") == 0) {	// disassemble
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
		//~ objfile(s, ...);
		//~ return dumpasm(s, cc, ss, dbgl);
	}
	else if (strcmp(argv[1], "-m") == 0) {	// make
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
	}
	else if (strcmp(argv[1], "-h") == 0) {	// help
		usage(s, argv[0], argc < 4 ? argv[3] : NULL);
	}
	else error(0, __FILE__, __LINE__, "invalid option '%s' \n", argv[1]);
	return 0;
}

struct state_t env;
int main(int argc, char *argv[]) {
	int i;
	//~ defn sym = 0;
	char *src = "test0.cxx";
	//~ if (vm_test()) return -1;
	//~ if (cc_test()) return -1;
	if (1 && argc == 1) {
		char *args[] = {
			"psvm",
			"-c",
			"-x",
			"test0.cxx",
		};
		argc = sizeof(args) / sizeof(*args); argv = args;
	}
	//~ argv[argc = 2] = "-x";
	//~ argv[argc = 3] = "test0.cxx";

	if (argc != 1)
		return prog(&env, argc, argv);

	//~ cc_init(&env);				//for tests

	/*for (i = 0; i < TBLS; i += 1) {
		for (sym = env.deft[i]; sym; sym = sym->next) {
			//~ dumpsym(stdout, "%?+T", env.deft[i]);
			dumpsym(stdout, env.deft[i]);
			fputs(sym->next ? ", " : "\n", stdout);
		}
	}// */

	//~ return cplfltmul(&env);
	//~ return cpldblmul(&env);
	//~ return swz_test(&env);
	//~ return dbg_emit(&env);
	//~ return vm_hgen();
	//~ mk_test(src = "txx.cxx", 1<<20);
	//~ return dbg_test(&env);
	//~ return dbg_eval(&env, "1 < 2");
	//~ return dbg_eval(&env, "math.sin(flt32(math.pi))");
	return dbg_file(&env, src);

	//~ return dbg_emit(&env);

	/* swizzle generator
	for(i = 0; i < 256; i++) {
		char c1 = "xyzw"[i>>0&3];
		char c2 = "xyzw"[i>>2&3];
		char c3 = "xyzw"[i>>4&3];
		char c4 = "xyzw"[i>>6&3];
		//~ printf("\t%c%c%c%c = 0x%02x;\n", c4, c3, c2, c1, i);
		printf("install(s, \"%c%c%c%c\", -1, CNST_int, %s)->init = intnode(s, 0x%02x);\n", c4, c3, c2, c1, "mod", i);
	} // */
	//~ /*
	//~ cc_init(&env);
	//~ dumpall(stdout, &env);
	/*for(i = 0; i < sizeof(libcfnc)/sizeof(*libcfnc); i++) {
		fputmsg(stdout, "%T;ret:%d;pop:%d;arg:%d\n", libcfnc[i].sym, libcfnc[i].ret, libcfnc[i].pop, libcfnc[i].arg);
	}//~ */
	return i = 0;
}

