#include <math.h>
#include <string.h>
#include "ccvm.h"

#ifdef __WATCOMC__
//~ Warning! W124: Comparison result always 0
//~ Warning! W201: Unreachable code

#pragma disable_message(124);
#pragma disable_message(201);
#endif

#pragma pack(push, 1)
typedef struct bcde {			// byte code decoder
	uint8_t opc;
	union {
		stkval arg;		// argument (load const)
		uint8_t idx;		// usualy index for stack
		int32_t rel:24;	// relative offset (ip, sp) +- 7MB
		/*struct {		// when starting a task
			uint8_t  dl;	// data to to be copied to stack
			uint16_t cl;	// code to to be executed paralel: 0 means fork
		};// */
		/*struct {				// extended3: 4 bytes `res := lhs OP rhs`
			uint32_t opc:4;		// 0 ... 15
			uint32_t mem:2;		// mem
			uint32_t res:6;		// res
			uint32_t lhs:6;		// lhs
			uint32_t rhs:6;		// rhs
			/+ --8<-------------------------------------------
			void *res = sp + ip->ext.res;
			void *lhs = sp + ip->ext.lhs;
			void *rhs = sp + ip->ext.rhs;

			chkstk(ip->ext.res);
			chkstk(ip->ext.lhs);
			chkstk(ip->ext.rhs);

			switch (ip->ext.mem) {
				case 0: break;
				case 1: res = *(void**)res; break;
				case 2: lhs = *(void**)lhs; break;
				case 3: rhs = *(void**)rhs; break;
			}
			switch (ip->ext.opc) {
				case ext_add: add(res, lhs, rhs); break;
				case ext_...: ...(res, lhs, rhs); break;
			}
			// +/ --8<----------------------------------------
		}ext;// */
	};
} *bcde;
#pragma pack(pop)

typedef struct cell {			// processor
	unsigned char	*ip;			// Instruction pointer
	unsigned char	*sp;			// Stack pointer(bp + ss)
	unsigned char	*bp;			// Stack base

	// multiproc
	//~ unsigned int	cs;			// child start(==0 join) / code size (pc)
	//~ unsigned int	pp;			// parent proc(==0 main) / ???

	// flags ?
	//~ unsigned	zf:1;			// zero flag
	//~ unsigned	sf:1;			// sign flag
	//~ unsigned	cf:1;			// carry flag
	//~ unsigned	of:1;			// overflow flag

} *cell;

//{ libc.c ---------------------------------------------------------------------

//~ #pragma intrinsic(log,cos,sin,tan,sqrt,fabs,pow,atan2,fmod)
//~ #pragma intrinsic(acos,asin,atan,cosh,exp,log10,sinh,tanh)

#define LIBCALLS 256

void* vmOffset(state s, symn arg, int ss) {
	if (arg->offs <= 0)
		return s->_mem - arg->offs;
	return ((char*)s->argv) + ss - arg->offs;
}
int libCallExitQuiet(state s) {
	return 0;
}

void vm_fputval(state, FILE *fout, symn var, stkval* ref, int flgs);
int libCallExitDebug(state s) {
	symn arg = s->libc->args;
	int argc = (char*)s->retv - (char*)s->argv;
	//~ debug("calling %-T", s->libc);
	//~ debug("CallDebugArgs(%-T): argc:(%d / %d)", s->libc, s->argc, argc);
	for ( ;arg; arg = arg->next) {
		char *ofs;

		if (arg->offs <= 0) {
			// global variable.
			ofs = (void*)(s->_mem - arg->offs);
		}
		else {
			// argument or local variable.
			ofs = ((char*)s->argv) + argc - arg->offs;
		}

		//~ debug("argv: %08x", s->argv);
		//~ if (arg->kind != TYPE_ref && s->args == s->defs) continue;
		if (arg->kind != TYPE_ref) continue;

		if (arg->file && arg->line)
			fputfmt(stdout, "%s:%d:",arg->file, arg->line);
		else
			fputfmt(stdout, "var: ",arg->file, arg->line);

		fputfmt(stdout, "@%d[0x%08x]\t: ", arg->offs < 0 ? -1 : arg->offs, ofs);

		//~ fputfmt(stdout, "@%d[0x%08x]\t: ", arg->offs, ofs);
		//~ fputfmt(stdout, "@%d[0x%08x]\t: ", s->argc - arg->offs, ofs);
		vm_fputval(s, stdout, arg, (stkval*)ofs, 0);
		fputc('\n', stdout);
	}
	return 0;
}
static struct lfun {
	int (*call)(state);
	const char* proto;
	symn sym;
	int8_t chk, pop, pad[2];
	//~ int32_t _pad;
}
libcvec[LIBCALLS];

/*void printargcast(astn arg) {
	if (arg) {
		if (arg->kind == OPER_com) {
			while (arg->kind == OPER_com) {
				//~ debug("arg:%t: %+k", arg->op.rhso->cst2, arg->op.rhso);
				arg = arg->op.lhso;
			}
		}
		//~ debug("arg:%t: %+k", arg->cst2, arg);
		if (arg->kind == TYPE_ref) {
			debug("arg:%t: %+T(arg:%t: %+k)", arg->id.link->cast, arg->id.link, arg->cst2, arg);
		}
	}
}// */

static symn installref(state s, const char *prot, astn *argv) {
	astn root;
	symn result = NULL;
	int level;
	int errc = s->errc;

	//~ prot = "int alma;";

	// TODO: this should look something like: `astn root = parse(s, TYPE_ref);`
	if (!ccOpen(s, srcText, (char *)prot)) {
		//~ TODO: error()
		return NULL;
	}

	s->cc->warn = 9;
	level = s->cc->nest;
	if ((root = decl(s->cc, decl_Ref))) {

		dieif(level != s->cc->nest, "FixMe");
		dieif(root->kind != TYPE_def, "FixMe");

		//~ result = root->id.link;
		if ((result = root->id.link)) {
			dieif(result->kind != TYPE_ref, "FixMe");
			*argv = root->id.args;
			result->cast = 0;//TYPE_def;
		}
	}

	//~ printargcast(root->id.args);

	return errc == s->errc ? result : NULL;
}//*/

/*
 * install a library call
 * @arg s: the runtime state.
 * @arg libc: the function to call.
 * @arg proto: prototype of function.
 */
int (*libcSwapExit(state s, int libc(state)))(state) {
	int (*result)(state) = libcvec[0].call;
	libcvec[0].call = libc ? libc : libCallExitQuiet;
	(void)s;
	return result;
}

symn libcall(state s, int libc(state), int pos, const char* proto) {
	static int libccnt = 0;
	symn arg, sym = NULL;
	int stdiff = 0;
	astn args = NULL;

	//~ if (!s->cc && !ccInit(s)) {return NULL;}
	if (!libc && strcmp(proto, "reset") == 0) {
		//~ debug("resetting lib calls");

		memset(libcvec, 0, sizeof(libcvec));
		libcvec[0].proto = "void Halt();";
		libcvec[0].call = libCallExitDebug;
		libcvec[0].sym = NULL;
		libccnt = 0;
	}

	if (!libccnt && !libc) {
		libccnt = 0;
		if (!libc) while (libcvec[libccnt].proto) {
			int i = libccnt;
			if (!libcall(s, libcvec[i].call, 0, libcvec[i].proto))
				return NULL;
		}
		return NULL;
	}// */

	if (libccnt >= (sizeof(libcvec) / sizeof(*libcvec))) {
		error(s, NULL, 0, "to many functions on install('%s')", proto);
		return NULL;
	}

	if (!libc || !proto)
		return 0;

	libcvec[libccnt].call = libc;
	libcvec[libccnt].proto = proto;
	sym = installref(s, libcvec[libccnt].proto, &args);

	//~ from: int64 zxt(int64 val, int offs, int bits)
	//~ make: define zxt(int64 val, int offs, int bits) = int64(emit(libc(25), int64 val, int offs, int bits));

	if (sym) {
		symn link = newdefn(s->cc, EMIT_opc);
		astn libc = newnode(s->cc, TYPE_ref);

		link->name = "libc";
		link->type = sym->type;
		link->offs = opc_libc;
		link->init = intnode(s->cc, libccnt);

		libc->id.name = link->name;
		libc->id.hash = -1;
		libc->id.link = link;
		libc->type = link->type;
		// TODO: end

		// glue the new libc argument
		if (args && args != s->cc->argz) {
			astn narg = newnode(s->cc, OPER_com);
			astn arg = args;
			narg->op.lhso = libc;

			if (arg->kind == OPER_com) {
				while (arg->op.lhso->kind == OPER_com)
					arg = arg->op.lhso;
				narg->op.rhso = arg->op.lhso;
				arg->op.lhso = narg;
			}
			else {
				narg->op.rhso = args;
				args = narg;
			}
		}
		else {
			args = libc;
		}

		libc = newnode(s->cc, OPER_fnc);
		//~ libc->op.lhso = emit_opc->tag;
		libc->op.lhso = newnode(s->cc, TYPE_ref);
		libc->op.lhso->id.link = emit_opc;
		libc->op.lhso->id.name = "emit";
		libc->op.lhso->id.hash = -1;
		libc->type = sym->type;
		libc->op.rhso = args;

		sym->kind = TYPE_def;
		sym->init = libc;
		sym->offs = pos;

		stdiff = fixargs(sym, 4, 0);
		libcvec[libccnt].chk = stdiff / 4;

		stdiff -= sizeOf(sym->type);
		libcvec[libccnt].pop = stdiff / 4;

		libcvec[libccnt].sym = sym;

		//~ debug("FixMe: %-T(chk: %d, pop: %d)", sym, libcfnc[libccnt].chk, libcfnc[libccnt].pop);
		// make arguments symbolic by default
		for (arg = sym->args; arg; arg = arg->next) {
			//~ stdiff += sizeOf(arg->type);
			if (arg->cast != TYPE_ref)
				arg->cast = TYPE_def;
			//~ arg->load = 0;
		}// */

		/*for (arg = sym->args; arg; arg = arg->next) {
			//~ dieif(arg->offs != stdiff, "FixMe: %+T(%d, %d)", arg, arg->offs, stdiff);
			//~ debug("FixMe: %+T(%d, %d)", arg, arg->offs, stdiff);
			stdiff += sizeOf(arg->type);
		}// */

	}
	else {
		error(s, NULL, 0, "install('%s')", proto);
		//~ return NULL;
	}

	libccnt += 1;

	return sym;
}
//}

static int lobit(int a) {return a & -a;}
static inline void* getip(state s, int pos) {
	return (void *)(s->_mem + pos);
}

int emitarg(state s, int opc, stkval arg) {
	bcde ip = getip(s, s->vm.pos);
	//~ stkval arg = *(stkval*)(&opc + 1);

	if (opc == markIP) {
		return s->vm.pos;
	}
	if (opc == opc_line) {
		static int line = 0;
		if (line != arg.i4) {
			//~ debug("line = %d", arg.i4);
			line = arg.i4;
		}
		return s->vm.pc;
	}

	if (s->vm._end - (unsigned char *)ip < 16) {
		debug("memory overrun");
		return 0;
	}

	else if (opc == opc_neg) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_neg; break;
		case TYPE_i64: opc = i64_neg; break;
		case TYPE_f32: opc = f32_neg; break;
		case TYPE_f64: opc = f64_neg; break;
		//~ case TYPE_pf2: opc = p4d_neg; break;
		//~ case TYPE_pf4: opc = p4f_neg; break;
	}
	else if (opc == opc_add) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_add; break;
		case TYPE_i64: opc = i64_add; break;
		case TYPE_f32: opc = f32_add; break;
		case TYPE_f64: opc = f64_add; break;
		//~ case TYPE_pf2: opc = p4d_add; break;
		//~ case TYPE_pf4: opc = p4f_add; break;
	}
	else if (opc == opc_sub) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_sub; break;
		case TYPE_i64: opc = i64_sub; break;
		case TYPE_f32: opc = f32_sub; break;
		case TYPE_f64: opc = f64_sub; break;
		//~ case TYPE_pf2: opc = p4d_sub; break;
		//~ case TYPE_pf4: opc = p4f_sub; break;
	}
	else if (opc == opc_mul) switch (arg.i4) {
		case TYPE_u32: opc = u32_mul; break;
		case TYPE_i32: opc = i32_mul; break;
		case TYPE_i64: opc = i64_mul; break;
		case TYPE_f32: opc = f32_mul; break;
		case TYPE_f64: opc = f64_mul; break;
		//~ case TYPE_pf2: opc = p4d_mul; break;
		//~ case TYPE_pf4: opc = p4f_mul; break;
	}
	else if (opc == opc_div) switch (arg.i4) {
		case TYPE_u32: opc = u32_div; break;
		case TYPE_i32: opc = i32_div; break;
		case TYPE_i64: opc = i64_div; break;
		case TYPE_f32: opc = f32_div; break;
		case TYPE_f64: opc = f64_div; break;
		//~ case TYPE_pf2: opc = p4d_div; break;
		//~ case TYPE_pf4: opc = p4f_div; break;
	}
	else if (opc == opc_mod) switch (arg.i4) {
		case TYPE_u32: opc = u32_mod; break;
		case TYPE_i32: opc = i32_mod; break;
		case TYPE_i64: opc = i64_mod; break;
		case TYPE_f32: opc = f32_mod; break;
		case TYPE_f64: opc = f64_mod; break;
		//~ case TYPE_pf2: opc = p4d_mod; break;
		//~ case TYPE_pf4: opc = p4f_mod; break; p4f_crs ???
	}

	// cmp
	else if (opc == opc_ceq) switch (arg.i4) {
		case TYPE_bit: //opc = u32_ceq; break;
		case TYPE_ref: //opc = u32_ceq; break;
		case TYPE_u32: //opc = u32_ceq; break;
		case TYPE_i32: opc = i32_ceq; break;
		case TYPE_f32: opc = f32_ceq; break;
		case TYPE_i64: opc = i64_ceq; break;
		case TYPE_f64: opc = f64_ceq; break;
	}
	else if (opc == opc_clt) switch (arg.i4) {
		case TYPE_u32: opc = u32_clt; break;
		case TYPE_i32: opc = i32_clt; break;
		case TYPE_f32: opc = f32_clt; break;
		case TYPE_i64: opc = i64_clt; break;
		case TYPE_f64: opc = f64_clt; break;
	}
	else if (opc == opc_cgt) switch (arg.i4) {
		case TYPE_u32: opc = u32_cgt; break;
		case TYPE_i32: opc = i32_cgt; break;
		case TYPE_f32: opc = f32_cgt; break;
		case TYPE_i64: opc = i64_cgt; break;
		case TYPE_f64: opc = f64_cgt; break;
	}
	else if (opc == opc_cne) {
		if (emitarg(s, opc_ceq, arg))
			opc = opc_not;
	}
	else if (opc == opc_cle) {
		if (emitarg(s, opc_cgt, arg))
			opc = opc_not;
	}
	else if (opc == opc_cge) {
		if (emitarg(s, opc_clt, arg))
			opc = opc_not;
	}

	// bit
	else if (opc == opc_cmt) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_cmt; break;
	}
	else if (opc == opc_shl) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_shl; break;
		//~ case TYPE_i64:
		//~ case TYPE_f32:
		//~ case TYPE_f64: return 0;
	}
	else if (opc == opc_shr) switch (arg.i4) {
		case TYPE_i32: opc = b32_sar; break;
		case TYPE_u32: opc = b32_shr; break;
		//~ case TYPE_i64:
		//~ case TYPE_f32:
		//~ case TYPE_f64: return 0;
	}
	else if (opc == opc_and) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_and; break;
		//~ case TYPE_i64:
		//~ case TYPE_f32:
		//~ case TYPE_f64: return 0;
	}
	else if (opc == opc_ior) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_ior; break;
		//~ case TYPE_i64:
		//~ case TYPE_f32:
		//~ case TYPE_f64: return 0;
	}
	else if (opc == opc_xor) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_xor; break;
		//~ case TYPE_i64:
		//~ case TYPE_f32:
		//~ case TYPE_f64: return 0;
	}

	// load/store
	/*else if (opc == opc_ldc) {
		if (arg.u8 == 0) opc = opc_ldz;
		else if (arg.u8 <= 0xff) opc = opc_ldc1;
		else if (arg.u8 <= 0xffff) opc = opc_ldc2;
		else if (arg.u8 <= 0xffffffff) opc = opc_ldc4;
		else opc = opc_ldc8;
	}// */
	/*else if (opc == opc_ldi) {

		const int bms = 4;		// the minimum moveable size in bytes

		while (opc == opc_ldi) {
			switch (arg.i4) {
				case  1: opc = opc_ldi1; break;
				case  2: opc = opc_ldi2; break;
				case  4: opc = opc_ldi4; break;
				case  8: opc = opc_ldi8; break;
				case 16: opc = opc_ldiq; break;
				default: {
					if (arg.i4 > bms) {
						int moveLen = arg.i4 > 16 ? 16 : arg.i4 > 8 ? 8 : bms;
						ip = getip(s, s->vm.pc);

						switch (ip->opc) {
							case opc_ldsp: {
								int rel;
								arg.i4 -= moveLen;
								rel = ip->rel += arg.i4;
								if (!emitint(s, opc_ldi, moveLen)) {
									debug("FixMe");
									return 0;
								}
								if (!emitint(s, opc_ldsp, rel)) {
									debug("FixMe");
									return 0;
								}
							} break;

							default:
								debug("FixMe");
								return 0;
						}
						continue;
					}
					return 0;
				}
			}
		}
	}*/
	else if (opc == opc_ldi) switch (arg.i4) {
		case  1: opc = opc_ldi1; break;
		case  2: opc = opc_ldi2; break;
		case  4: opc = opc_ldi4; break;
		case  8: opc = opc_ldi8; break;
		case 16: opc = opc_ldiq; break;
		default: 
			//~ arg.i4 = +arg.i4;
			//~ opc = opc_cpy;
			fatal("FixMe");
			break;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case  1: opc = opc_sti1; break;
		case  2: opc = opc_sti2; break;
		case  4: opc = opc_sti4; break;
		case  8: opc = opc_sti8; break;
		case 16: opc = opc_stiq; break;
		default: 
			//~ arg.i4 = -arg.i4;
			//~ opc = opc_cpy;
			fatal("FixMe");
			break;
	}

	/*else if (opc == opc_spc) {
		dieif (arg.i8 & 3, "FixMe");
		if (arg.i8 < 0) {
			if (-arg.i8 < max_reg * 4) {
				opc = opc_drop;
				arg.i8 /= -4;
			}
		}
		else {
			if (+arg.i8 < max_reg * 4) {
				opc = opc_loc;
				arg.i8 /= 4;
			}
		}
			//~ return 0;
	}// */

	else if (opc == opc_inc) {		// TODO: DelMe!
		ip = getip(s, s->vm.pc);
		switch (ip->opc) {
			case opc_ldsp:
				if (s->vm.opti) {
						//~ return 0;
					if (arg.i4 < 0)
						return 0;
					ip->rel += arg.i4;
					return s->vm.pc;
				}// fall
			default:
				if (emitint(s, opc_ldc4, arg.i4))
					return emitopc(s, i32_add);
		}
		return 0;
	}

	if (opc > opc_last) {
		fatal("invalid opc(0x%x, 0x%X)", opc, arg.i8);
		//~ debug("invalid opc(0x%x, 0x%X)", opc, arg.i8);
		//~ debug("invalid arg(0x%X)", arg.i8);
		return 0;
	}
	// Optimize
	if (s->vm.opti > 1) {
		if (0) ;
		else if (opc == opc_ldi1) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}
		else if (opc == opc_ldi2) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}

		else if (opc == opc_ldi4) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_ld32;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}
		else if (opc == opc_sti4) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_st32;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set1;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}

		else if (opc == opc_ldi8) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_ld64;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup2;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}
		else if (opc == opc_sti8) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_st64;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set2;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}

		else if (opc == opc_ldiq) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) <= max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup4;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}
		else if (opc == opc_stiq) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) <= max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set4;
				s->vm.pos = s->vm.pc;
				s->vm.ss -= 1;
			}
		}

		// conditional jumps
		else if (opc == opc_not) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_not) {
				s->vm.pos = s->vm.pc;
				return s->vm.pc;
			}
		}

		else if (opc == opc_jnz) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_not) {
				s->vm.pos = s->vm.pc;
				opc = opc_jz;
			}
		}
		else if (opc == opc_jz) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_not) {
				s->vm.pos = s->vm.pc;
				opc = opc_jnz;
			}
		}

		// mul, div, mod
		/*else if (opc == u32_mod) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldc4) {
				int x = ip->arg.i4;
				// if constant contains only one bit
				if (x > 0 && lobit(x) == x) {
					ip->arg.i4 -= 1;
					opc = b32_and;
				}
			}
		}
		else if (opc == u32_div) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_ldc4) {
				int x = ip->arg.i4;
				// if constant contains only one bit
				if (x > 0 && lobit(x) == x) {
					ip->arg.i4 ?= 1;
					opc = b32_shr;
				}
			}
		}*/

		/*~:)) ?? others
		if (opc == opc_ldz1) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldz1) {
				opc = opc_ldz2;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_ldz2) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldz2) {
				opc = opc_ldz4;
				s->cs = s->pc;
				s->ss -= 2;
			}
		}
		else if (opc == opc_shl || opc == opc_shr || opc == opc_sar) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldc4) {
				if (opc == opc_shl) arg.i1 = 1 << 5;
				if (opc == opc_shr) arg.i1 = 2 << 5;
				if (opc == opc_sar) arg.i1 = 3 << 5;
				s->rets -= 1;
				opc = opc_bit;
				arg.i1 |= ip->arg.u1 & 0x1F;
				s->cs = s->pc;
			}
		}// */

	}

	ip = getip(s, s->vm.pos);

	ip->opc = opc;
	ip->arg = arg;
	s->vm.pc = s->vm.pos;	// previous program pointer is here

	if (opc == opc_jmp || opc == opc_jnz || opc == opc_jz) {
		ip->rel -= s->vm.pc;
		dieif((ip->rel + s->vm.pc) != arg.i8, "FixMe");
	}
	else if (opc == opc_spc) {
		/*TODO: int max = 0x7fffff;
		while (arg.i8 > max) {
			if (!emit(s, opc, max))
				return 0;
			arg.i8 -= max;
		}// */
		//~ ip->rel = arg.i8 / 4;
		dieif(ip->rel != arg.i8, "FixMe");
	}
	else switch (opc) {
		case opc_ldsp:
			dieif(ip->rel != arg.i8, "FixMe");
			break;
		case opc_drop:
			dieif(ip->idx != arg.i8, "FixMe");
			break;
		case opc_loc:
			dieif(ip->idx != arg.i8, "FixMe");
			break;
		// TODO: check other opcodes too
	}

	//~ debug("opc_x%0x(0x%0X)%09.*A", opc, arg.i8, s->pc, ip);
	//~ debug("@%04x[ss:%03d]: %09.*A", s->pc, s->ss, s->pc, ip);
	//~ debug("ss(%d): %09.*A", s->ss, s->pc, ip);

	switch (opc) {
		error_opc:
			error(s, NULL, 0, "invalid opcode: op[%.*A]", s->vm.pc, ip);
			//~ fputasm(stderr, s, s->seg, -1, 0x10);
			return 0;

		error_stc:
			error(s, NULL, 0, "stack underflow: op[%.*A](%d)", s->vm.pc, ip, s->vm.ss);
			//~ fputasm(stderr, s, s->seg, -1, 0x10);
			return 0;

		#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
		#define NEXT(__IP, __CHK, __SP)\
			STOP(error_stc, s->vm.ss < (__CHK));\
			s->vm.ss += (__SP);\
			s->vm.pos += (__IP);
		#include "code.i"
	}

	if (opc == opc_call) {
		//~ after a call ends with a return,
		//~ wich drops ip from the stack.
		s->vm.ss -= 1;
	}

	//~ debug("ss(%d): %09.*A", s->ss * 4, s->pc, ip);

	if (s->vm.sm < s->vm.ss)
		s->vm.sm = s->vm.ss;

	return s->vm.pc;
}
int emiti32(state s, int32_t arg) {
	stkval tmp;
	tmp.i4 = arg;
	return emitarg(s, opc_ldc4, tmp);
}
int emiti64(state s, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(s, opc_ldc8, tmp);
}
int emitf32(state s, float32_t arg) {
	stkval tmp;
	tmp.f4 = arg;
	return emitarg(s, opc_ldcf, tmp);
}
int emitf64(state s, float64_t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return emitarg(s, opc_ldcF, tmp);
}
int emitptr(state s, void* arg) {
	stkval tmp;
	unsigned char* ptr = arg;

	dieif(ptr < s->_mem, "invalid reference");
	dieif(ptr > s->_mem + s->vm.ro, "invalid reference");

	tmp.i8 = (ptr - s->_mem);
	return emitarg(s, opc_ldcr, tmp);
}

int emitopc(state s, int opc) {
	stkval arg;
	arg.i8 = 0;
	return emitarg(s, opc, arg);
}
int emitidx(state s, int opc, int arg) {
	stkval tmp;
	tmp.i8 = (int32_t)s->vm.ss * 4 - arg;

	switch (opc) {
		case opc_drop:
			opc = opc_spc;
			tmp.i8 = -tmp.i8;
		case opc_ldsp:
			return emitarg(s, opc, tmp);
	}

	if (tmp.i8 > 255) {
		debug("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	if (tmp.i8 > s->vm.ss * 4) {
		debug("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	return emitarg(s, opc, tmp);
}
int emitint(state s, int opc, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(s, opc, tmp);
}
int fixjump(state s, int src, int dst, int stc) {
	dieif(stc & 3, "FixMe");
	if (src >= 0) {
		bcde ip = getip(s, src);
		if (src) switch (ip->opc) {
			default: fatal("FixMe");
			//~ case opc_ldsp:
			//~ case opc_call:
			case opc_jmp:
			case opc_jnz:
			case opc_jz:
				ip->rel = dst - src;
				break;
		}
		if (!src && !dst)
			s->vm.ss = stc / 4;
		else if (stc > 0)
			s->vm.ss = stc / 4;
		return 1;
	}

	fatal("FixMe");
	return 0;
}

int stkoffs(state s, int size) {
	return size + s->vm.ss * 4;
}

/*static cell task(state ee, cell pu, int cc, int n, int cl, int dl) {
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
	return pu + n;
}// */

/*static int done(state ee, int cp) {
	pu[pu[cp].pp].cp -= 1;
	sigsnd(pp->pp, SIG_quit);
}// */

typedef enum {
	doInit,		// initialize, master is runing
	doTask,		// start a new processor
	doWait,		// wait for childs
	//~ doBrk,		// 
	//~ getNext,	// get next working cell
	//~ doSync = 1;		// syncronize
} doWhat;
static int mtt(state ee, doWhat cmd, cell pu, int n) {
	static int workers = 0;
	switch (cmd) {
		case doInit: {			// master thread is running
			int i;
			workers = 1;
			for (i = 0; i < n; ++i) {
				pu[i].ip = 0;
			}
		} break;
		case doTask: if (workers) {		// uninitialized or no more processors left
			// the lowest free processor
			int fpu = lobit(~workers);
			workers |= fpu;
			return fpu;
		} break;
		case doWait: {			// master thread is running
			workers = 1;
		} break;
		/*case doBrk: if (ee->dbg) {
			ee->dbg(ee, ....);
		} break;*/
	}
	(void)ee;
	return 0;
}
static inline int ovf(cell pu) {
	return (pu->sp - pu->bp) < 0;
}

/** vmExec
 * executes on a single thread
 * @arg env: enviroment
 * @arg cc: cell count
 * @arg ss: stack size
 * @return: error code
**/

static void dbugerr(state s, int pu, void *ip, long* bp, int ss, char *text) {
	//~ int SP = ((char*)s->vm._end) - ((char*)bp);
	int IP = ((unsigned char*)ip) - s->_mem - s->vm.pc;
	error(s, NULL, 0, ">exec:%s:[pu%02d][sp%02d]@%9.*A", text, pu, ss, IP, ip);
	//~ fputfmt(stdout, ">vmExec:[pu%02d][sp%02d]@%9.*A\n", pu, ss, IP, ip);
}

static int dbugpu(state s, cell pu, dbgf dbg) {
	typedef uint32_t *stkptr;
	typedef uint8_t *memptr;

	long ms = s->vm._end - s->_mem;
	memptr mp = (void*)s->_mem;
	stkptr st = (void*)pu->sp;

	for ( ; ; ) {

		register bcde ip = (void*)pu->ip;
		register stkptr sp = (void*)pu->sp;

		if (dbg && dbg(s, 0, ip, (long*)sp, st - sp))
			return -9;

		switch (ip->opc) {		// vmExec

			stop_vm:
				//~ if (dbg) dbg(vm->s, 0, NULL, (long*)sp, st - sp);
				return 0;

			error_opc:
				dbugerr(s, 0, ip, (long*)sp, st - sp, "invalid opcode");
				//~ error(vm->s, 0, "invalid opcode: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip);
				return -1;

			error_ovf:
				dbugerr(s, 0, ip, (long*)sp, st - sp, "stack overflow");
				//~ error(vm->s, 0, "stack overflow: op[%.*A] / %d", (char*)ip - (char*)(vm->_mem), ip, (vm->_end - pu->sp) / 4);
				return -2;

			error_mem:
				dbugerr(s, 0, ip, (long*)sp, st - sp, "segmentation fault");
				//~ error(vm->s, 0, "segmentation fault: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip);
				return -3;

			error_div:
				dbugerr(s, 0, ip, (long*)sp, st - sp, "division by zero");
				//~ error(s, 0, "division by zero: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip);
				return -4;

			#define NEXT(__IP, __CHK, __SP) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#define EXEC
			#include "code.i"
		}
	}

	return 0;
}

int vmExec(state s, dbgf dbg) {
	struct cell pu[1];

	pu->ip = (unsigned char *)s->_mem + s->vm.pc;
	pu->bp = pu->ip + s->vm.pos;		// the last emited instruction
	pu->sp = s->vm._end;

	/*if ((((int)pu->sp) & 3)) {
		error(vm->s, 0, "invalid statck size");
		return -99;
	}// */

	if (pu->bp < pu->ip) {
		error(s, NULL, 0, "invalid statck size");
		return -99;
	}

	return dbugpu(s, pu, dbg);
}

int vmCall(state s, symn fun, ...) {

	symn arg;
	int argsize = 0;
	int result = 0;
	struct cell pu[1];

	char *resp = NULL;
	char *argp = NULL;
	char *ap = (char*)&fun + sizeof(fun);

	/*if (!s) {
		trace("null");
		return -1;
	}
	if (!fun) {
		trace("null");
		return -1;
	}
	if (!fun->call) {
		trace("error");
		return -1;
	}
	if (fun->offs >= 0) {
		trace("relative ?");
		return -1;
	}
	if (fun->kind != TYPE_ref) {
		trace("error");
		return -1;
	}
	// */

	pu->ip = s->_mem - fun->offs;
	pu->bp = pu->ip + s->vm.pos;
	pu->sp = s->vm._end;

	// push Arguments;
	//~ va_start(ap, fun);
	for(arg = fun->args; arg; arg = arg->next) {

		int size = sizeOf(arg->type);
		unsigned char *offs = pu->sp - arg->offs;

		if (arg->cast == TYPE_ref || arg->type->cast == TYPE_ref) {
			trace("by reference");
			return -2;
		}
		if (offs < pu->bp) {
			trace("stack overflow");
			return -3;
		}
		//~ TODO: setVar(s, arg, ap + argsize);
		memcpy(offs, ap + argsize, size);
		argsize += size;
	}
	//~ va_end(ap);

	pu->sp -= sizeOf(fun->type);
	resp = (char*)pu->sp;

	pu->sp -= argsize;
	argp = (void*)pu->sp;

	// return here: vm->px: program exit
	pu->sp -= 4;
	*(int*)(pu->sp) = s->vm.px;

	if (pu->bp < pu->ip) {
		error(s, NULL, 0, "invalid statck size");
		return -99;
	}

	result = dbugpu(s, pu, NULL);
	s->retv = resp;
	s->argv = argp;
	return result;
}

/*int setVar(state s, symn arg, void* argRef) {
	int size = sizeOf(arg->type);
	void* ref = vmOffset(arg);

	switch (size) {

		case 1:
		case 2:
		case 4:
			*(int32_t*)ref = *(int32_t*)argRef;
			break;

		case 8:
			*(int64_t*)ref = *(int64_t*)argRef;
			break;

		default:
			memcpy(ref, argRef, size);
			break;
	}
}// */

void fputopc(FILE *fout, unsigned char* ptr, int len, int offs) {
	int i;
	bcde ip = (bcde)ptr;

	if (offs >= 0)
		fputfmt(fout, ".%04x: ", offs);

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
	else
		fputfmt(fout, "opc%02x", ip->opc);

	switch (ip->opc) {
		case opc_spc:
		case opc_ldsp:
			fprintf(fout, " %+d", ip->rel);
			break;

		case opc_ld32:
		case opc_st32:
		case opc_ld64:
		case opc_st64:
			fprintf(fout, " %x", ip->rel);
			break;

		case opc_loc:
		case opc_drop:
			fprintf(fout, " %d", ip->idx);
			break;

		case opc_jmp:
		case opc_jnz:
		case opc_jz:
			if (offs < 0)
				fprintf(fout, " %+d", ip->rel);
			else
				fprintf(fout, " .%04x", offs + ip->rel);
			break;
		/*case opc_bit: switch (ip->arg.u1 >> 5) {
			default: fprintf(fout, "bit.???"); break;
			case  1: fprintf(fout, "shl %d", ip->arg.u1 & 0x1F); break;
			case  2: fprintf(fout, "shr %d", ip->arg.u1 & 0x1F); break;
			case  3: fprintf(fout, "sar %d", ip->arg.u1 & 0x1F); break;
			case  4: fprintf(fout, "rot %d", ip->arg.u1 & 0x1F); break;
			case  5: fprintf(fout, "get %d", ip->arg.u1 & 0x1F); break;
			case  6: fprintf(fout, "set %d", ip->arg.u1 & 0x1F); break;
			case  7: fprintf(fout, "cmt %d", ip->arg.u1 & 0x1F); break;
			case  0: switch (ip->arg.u1) {
				default: fprintf(fout, "bit.???"); break;
				case  1: fprintf(fout, "bit.any"); break;
				case  2: fprintf(fout, "bit.all"); break;
				case  3: fprintf(fout, "bit.sgn"); break;
				case  4: fprintf(fout, "bit.par"); break;
				case  5: fprintf(fout, "bit.cnt"); break;
				case  6: fprintf(fout, "bit.bsf"); break;		// most significant bit index
				case  7: fprintf(fout, "bit.bsr"); break;
				case  8: fprintf(fout, "bit.msb"); break;		// use most significant bit only
				case  9: fprintf(fout, "bit.lsb"); break;
				case 10: fprintf(fout, "bit.rev"); break;		// reverse bits
				// swp, neg, cmt, 
			} break;
		} break;*/

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			fputfmt(fout, " sp(%d)", ip->idx);
			break;

		//~ case opc_ldc1: fputfmt(fout, " %d", ip->arg.i1); break;
		//~ case opc_ldc2: fputfmt(fout, " %d", ip->arg.i2); break;
		case opc_ldc4: fputfmt(fout, " %d", ip->arg.i4); break;
		case opc_ldc8: fputfmt(fout, " %D", ip->arg.i8); break;
		case opc_ldcf: fputfmt(fout, " %f", ip->arg.f4); break;
		case opc_ldcF: fputfmt(fout, " %F", ip->arg.f8); break;
		case opc_ldcr: fputfmt(fout, " %x", ip->arg.u4); break;

		case opc_libc:
			if (libcvec[ip->idx].sym)
				fputfmt(fout, "(pop %d, check %d) %-T", libcvec[ip->idx].pop, libcvec[ip->idx].chk, libcvec[ip->idx].sym);
			else
				fputfmt(fout, " %s", libcvec[ip->idx].proto);
			break;

		case p4x_swz: {
			char c1 = "xyzw"[ip->idx >> 0 & 3];
			char c2 = "xyzw"[ip->idx >> 2 & 3];
			char c3 = "xyzw"[ip->idx >> 4 & 3];
			char c4 = "xyzw"[ip->idx >> 6 & 3];
			fprintf(fout, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->idx);
		} break;
	}
}
void fputasm(FILE *fout, state s, int beg, int end, int mode) {
	int rel = 0;//mode & 0x10;// ? 0 : -1;
	int i, is = 12345;
	//~ int ss = 0;
	if (end == -1)
		end = s->vm.pos;// - mark;

	switch (mode & 0x30) {
		case 0x00: rel = -1; break;
		case 0x10: rel = 0; break;
		case 0x20: rel = beg; break;
		//~ case 0x30: rel = (unsigned char*)getip(s, mark) - s->_mem; break;
	}

	for (i = beg; i < end; i += is) {
		bcde ip = getip(s, i);

		switch (ip->opc) {
			error_opc: error(s, NULL, 0, "invalid opcode: %02x '%A'", ip->opc, ip); return;
			//~ #define NEXT(__IP, __CHK, __SP) {if (__IP) is = (__IP);/*  ss += (__SP); */}
			#define NEXT(__IP, __CHK, __SP) {if (__IP) is = (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "code.i"
		}

		if (mode & 0xf00)
			fputfmt(fout, "%I", (mode & 0xf00) >> 8);

		//~ if (mode & 0x20)		// TODO: remove stack size
			//~ fputfmt(fout, "ss(%03d): ", ss);

		fputopc(fout, (void*)ip, mode & 0xf, rel >= 0 ? (rel + i) : -1);
		//~ fputopc(fout, (void*)ip, mode & 0xf, rel ? (beg - s->_mem) : -1);

		fputc('\n', fout);
	}
}

void vm_fputval(state s, FILE *fout, symn var, stkval* ref, int flgs) {
	symn typ = var->kind == TYPE_ref ? var->type : var;

	if (typ) switch (typ->kind) {
		/*
		case TYPE_def:
			vm_fputval(s, fout, typ, ref, flgs);
			break;
		// */

		case TYPE_rec: {
			symn tmp;
			int n = 0;

			if (var != typ && !var->pfmt)
				fputfmt(fout, "%T = ", var);

			if (var->cast == TYPE_ref || typ->cast == TYPE_ref) {		// arrays, strings, functions
				ref = (stkval*)(s->_mem + ref->u4);
			}

			if (var && var->call) {
				fputfmt(fout, "function @%d", -var->offs);
				break;
			}

			if (var->pfmt || typ->pfmt) {
				char *fmt = var->pfmt ? var->pfmt : typ->pfmt;
				switch (typ->size) {
					case 1: fputfmt(fout, fmt, ref->u1); break;
					case 2: fputfmt(fout, fmt, ref->u2); break;
					case 4: fputfmt(fout, fmt, ref->u4); break;
					case 8: fputfmt(fout, fmt, ref->i8); break;
				}
			}
			else {
				fputfmt(fout, "%T(", typ);
				switch (typ->cast) {
					case TYPE_bit:
					case TYPE_u32: switch (typ->size) {
						case 1: fputfmt(fout, "%u", ref->i1); break;
						case 2: fputfmt(fout, "%u", ref->i2); break;
						case 4: fputfmt(fout, "%u", ref->i4); break;
						case 8: fputfmt(fout, "%U", ref->i8); break;
					} break;

					case TYPE_i32:
					case TYPE_i64: switch (typ->size) {
						case 1: fputfmt(fout, "%d", ref->i1); break;
						case 2: fputfmt(fout, "%d", ref->i2); break;
						case 4: fputfmt(fout, "%d", ref->i4); break;
						case 8: fputfmt(fout, "%D", ref->i8); break;
					} break;

					case TYPE_f32:
					case TYPE_f64: switch (typ->size) {
						case 4: fputfmt(fout, "%f", ref->f4); break;
						case 8: fputfmt(fout, "%F", ref->f8); break;
					} break;

					/*case TYPE_ref: {
						fputfmt(fout, ".ERROR.", );
						break;
					}// */
					default: {
						//~ fputfmt(fout, "%T{", typ);
						for (tmp = typ->args; tmp; tmp = tmp->next) {
							if (tmp->kind != TYPE_ref)
								continue;

							if (tmp->pfmt && !*tmp->pfmt)
								continue;

							if (n > 0)
								fputfmt(fout, ", ");

							vm_fputval(s, fout, tmp, (void*)((char*)ref + tmp->offs), 0);
							n += 1;
						}
						//~ fputfmt(fout, "}");
					} break;
				}
				fputfmt(fout, ")", typ);
			}
		} break;
		case TYPE_arr: {
			int i = 0, n = 0;
			//~ struct astn tmp;
			symn base = typ->type;

			fputfmt(fout, "%I", flgs & 0xff);
			if (var != typ)
				fputfmt(fout, "%T: ", var);

			if (typ == type_str) {
				ref = (stkval*)(s->_mem + ref->u4);
				fputfmt(fout, "(\"%s\")", ref);
			}
			else {
				fputfmt(fout, "%T{", typ);
				if ((n = typ->size) < 0) {
					fputfmt(fout, "Unknown Dimension}");
					break;
				}
				else if (n > 20) {
					//~ n = 0;
				}

				while (i < n) {
					if (base->kind == TYPE_arr)
						fputfmt(fout, "\n");

					vm_fputval(s, fout, base, (stkval*)((char*)ref + i * sizeOf(base)), ((flgs + 1) & 0xff));
					if (++i < n)
						fputfmt(fout, ", ");
				}
				if (n > 0) {
					if (base->kind == TYPE_arr)
						fputfmt(fout, "\n%I", flgs & 0xff);
				}
				else {
					fputfmt(fout, "...");
				}
				fputfmt(fout, "}");
			}
		} break;
		default: {
			if (var != typ)
				fputfmt(fout, "%T:", var);
			fputfmt(fout, "%T[ERROR]", typ);
		} break;
	}
}

#if 0
int vmTest() {
	int e = 0;
	struct bcde opc, *ip = &opc;
	opc.arg.i8 = 0;
	for (opc.opc = 0; opc.opc < opc_last; opc.opc++) {
		int err = 0;
		if (opc_tbl[opc.opc].size == 0) continue;
		if (opc_tbl[opc.opc].code != opc.opc) {
			fprintf(stderr, "invalid '%s'[%02x]\n", opc_tbl[opc.opc].name, opc.opc);
			e += err = 1;
		}
		else switch (opc.opc) {
			error_len: e += 1; fputfmt(stderr, "opcode size 0x%02x: '%A'\n", opc.opc, ip); break;
			error_chk: e += 1; fputfmt(stderr, "stack check 0x%02x: '%A'\n", opc.opc, ip); break;
			error_dif: e += 1; fputfmt(stderr, "stack difference 0x%02x: '%A'\n", opc.opc, ip); break;
			error_opc: e += 1; fputfmt(stderr, "unimplemented opcode 0x%02x: '%A'\n", opc.opc, ip); break;
			#define NEXT(__IP, __CHK, __DIFF) {\
				if (opc_tbl[opc.opc].size && (__IP) && opc_tbl[opc.opc].size != (__IP)) goto error_len;\
				if (opc_tbl[opc.opc].chck != 9 && opc_tbl[opc.opc].chck != (__CHK)) goto error_chk;\
				if (opc_tbl[opc.opc].diff != 9 && opc_tbl[opc.opc].diff != (__DIFF)) goto error_dif;\
			}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "incl/vm.h"
		}
	}
	return e;
}
#endif
