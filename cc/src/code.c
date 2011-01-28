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
	//~ return ((char*)s->argv) + (ss ? ss - arg->offs : arg->offs);
	return ((char*)s->argv) + ss - arg->offs;
}
static int libHalt(state s) {
	symn arg = s->args;
	//~ debug("calling %-T", s->libc);
	//~ debug("argv: %08x", s->argv);
	for ( ;arg; arg = arg->next) {
		char *ofs = vmOffset(s, arg, s->argc);
		void vm_fputval(state, FILE *fout, symn var, stkval* ref, int flgs);

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
libcfnc[LIBCALLS] = {
	{libHalt, "void Halt();", NULL, 0, 0},
	//~ {libHalt, "void Exit(int Code);", NULL, 0, 0},
	//~ {libHalt, "void Test(int x, int y, int z);", NULL, 0, 0},
	{NULL},
};

symn installref(state s, const char *prot, astn *argv){
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
	if ((root = decl(s->cc, 0))) {

		dieif(level != s->cc->nest, "FixMe");
		dieif(root->kind != TYPE_def, "FixMe");

		result = root->id.link;
		if ((result = root->id.link)) {
			dieif(result->kind != TYPE_ref, "FixMe");
			*argv = root->id.args;
		}
	}

	return errc == s->errc ? result : NULL;
}//*/

symn libcall(state s, int libc(state), int pos, const char* proto) {
	static int libccnt = 0;
	symn arg, sym = NULL;
	int stdiff = 0;
	astn args;

	//~ if (!s->cc && !ccInit(s)) {return NULL;}

	if (!libccnt && !libc) {
		libccnt = 0;
		if (!libc) while (libcfnc[libccnt].proto) {
			int i = libccnt;
			if (!libcall(s, libcfnc[i].call, 0, libcfnc[i].proto))
				return NULL;
		}
		return NULL;
	}

	if (libccnt >= (sizeof(libcfnc) / sizeof(*libcfnc))) {
		error(s, 0, "to many functions on install('%s')", proto);
		return NULL;
	}

	if (!libc || !proto)
		return 0;

	libcfnc[libccnt].call = libc;
	libcfnc[libccnt].proto = proto;
	sym = installref(s, libcfnc[libccnt].proto, &args);

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

		// make arguments symbolic by default
		for (arg = sym->args; arg; arg = arg->next) {
			//~ stdiff += sizeOf(arg->type);
			arg->cast = TYPE_def;
			//~ arg->load = 0;
		}// */

		libc = newnode(s->cc, OPER_fnc);
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
		libcfnc[libccnt].chk = stdiff / 4;

		stdiff -= sizeOf(sym->type);
		libcfnc[libccnt].pop = stdiff / 4;

		libcfnc[libccnt].sym = sym;

		//~ debug("FixMe: %-T(chk: %d, pop: %d)", sym, libcfnc[libccnt].chk, libcfnc[libccnt].pop);
		for (arg = sym->args; arg; arg = arg->next) {
			//~ dieif(arg->offs != stdiff, "FixMe: %+T(%d, %d)", arg, arg->offs, stdiff);
			//~ debug("FixMe: %+T(%d, %d)", arg, arg->offs, stdiff);
			stdiff += sizeOf(arg->type);
		}// */

	}
	else {
		error(s, 0, "install('%s')", proto);
		//~ return NULL;
	}

	libccnt += 1;

	return sym;
}
//}

static inline void* getip(vmState s, int pos) {
	return (void *)(s->_mem + pos);
}

// TODO: exchange this with emitarg(vmState s, int opc, stkval arg);
//~ int emit(vmState s, int opc, ...) {

int emitarg(vmState s, int opc, stkval arg) {
	bcde ip = getip(s, s->pos);
	//~ stkval arg = *(stkval*)(&opc + 1);

	/*if (opc == get_ip) {
		return s->cs;
	}
	/ *if (opc == get_sp) {
		return -s->ss;
	}
	if (opc == loc_data) {
		s->_ptr += arg.i4;
		return 0;
	}*/
	/*if (opc == seg_code) {
		s->pc = s->_ptr - s->_mem;
		s->_ptr += s->cs;
		return s->ss;
	}*/
	if (opc == markIP) {
		return s->pos;
	}
	if (opc == opc_line) {
		static int line = 0;
		if (line != arg.i4) {
			//~ debug("line = %d", arg.i4);
			line = arg.i4;
		}
		return s->pc;
	}

	if (s->_end - (unsigned char *)ip < 16) {
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
		//~ case TYPE_u32: opc = u32_mod; break;
		case TYPE_i32: opc = i32_mod; break;
		case TYPE_i64: opc = i64_mod; break;
		case TYPE_f32: opc = f32_mod; break;
		case TYPE_f64: opc = f64_mod; break;
		//~ case TYPE_pf2: opc = p4d_mod; break;
		//~ case TYPE_pf4: opc = p4f_mod; break; p4f_crs ???
	}

	// cmp
	else if (opc == opc_ceq) switch (arg.i4) {
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
	else if (opc == opc_ldi) {

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
						ip = getip(s, s->pc);

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
		/*switch (arg.i4) {
			case  1: opc = opc_ldi1; break;
			case  2: opc = opc_ldi2; break;
			case  4: opc = opc_ldi4; break;
			case  8: opc = opc_ldi8; break;
			case 16: opc = opc_ldiq; break;
			default: while (arg.i4 > 4) {
				if (arg.i4 > bms) {
					ip = getip(s, s->pc);

					switch (ip->opc) {
						case opc_ldsp: {
							int rel2 = ip->rel;

							ip->rel += size;

							if (!emitint(s, opc_ldi, bms)) {
								debug("FixMe(%d/%d)", arg.i4, size);
								return 0;
							}
							debug("%A", ip);

							if (!emitint(s, opc_ldsp, rel2)) {
								debug("FixMe");
								return 0;
							}
						} break;

						default:
							debug("FixMe");
							return 0;
					}
					arg.i4 -= bms;
					continue;
				}
			}
		}// */
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case  1: opc = opc_sti1; break;
		case  2: opc = opc_sti2; break;
		case  4: opc = opc_sti4; break;
		case  8: opc = opc_sti8; break;
		case 16: opc = opc_stiq; break;
	}

	else if (opc == opc_drop) {		// pop nothing
		if (arg.i4 == 0)
			return s->pc;

		dieif(arg.i4 & 3, "FixMe");
		arg.i4 /= 4;
	}

	else if (opc == opc_inc) {		// TODO: DelMe!
		ip = getip(s, s->pc);
		switch (ip->opc) {
			case opc_ldsp:
				if (s->opti) {
						//~ return 0;
					if (arg.i4 < 0)
						return 0;
					ip->rel += arg.i4;
					return s->pc;
				}// fall
			default:
				if (emitint(s, opc_ldc4, arg.i4))
					return emitopc(s, i32_add);
		}
		return 0;
	}

	if (opc > opc_last) {
		//~ fatal("invalid opc_0x%x", opc);
		debug("invalid opc(0x%x, 0x%X)", opc, arg.i8);
		debug("invalid arg(0x%X)", arg.i8);
		return 0;
	}
	if (s->opti > 1) {
		if (0) ;
		else if (opc == opc_ldi1) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_ldi2) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}

		else if (opc == opc_ldi4) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_sti4) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set1;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}

		else if (opc == opc_ldi8) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup2;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_sti8) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set2;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}

		else if (opc == opc_ldiq) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) <= max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup4;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_stiq) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) <= max_reg)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set4;
				s->pos = s->pc;
				s->ss -= 1;
			}
		}

		// conditional jumps
		else if (opc == opc_not) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_not) {
				s->pos = s->pc;
				return s->pc;
			}
		}
		else if (opc == opc_jnz) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_not) {
				s->pos = s->pc;
				opc = opc_jz;
			}
		}
		else if (opc == opc_jz) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_not) {
				s->pos = s->pc;
				opc = opc_jnz;
			}
		}

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

	ip = getip(s, s->pos);

	//~ s->ic += s->pc != s->pos;

	s->pc = s->pos;

	ip->opc = opc;
	ip->arg = arg;

	if (opc == opc_ldsp) {
		if (ip->rel != arg.i8)
			return 0;
	}
	else if (opc == opc_spc) {
		/*TODO: int max = 0x7fffff;
		while (arg.i8 > max) {
			if (!emit(s, opc, max))
				return 0;
			arg.i8 -= max;
		}// */
		ip->rel = arg.i8;// / 4;
		if (ip->rel != arg.i8)
			return 0;
	}
	else if (opc == opc_jmp || opc == opc_jnz || opc == opc_jz) {
		ip->rel -= s->pc;
	}
	//~ debug("opc_x%0x(0x%0X)%09.*A", opc, arg.i8, s->pc, ip);
	//~ debug("@%04x[ss:%03d]: %09.*A", s->pc, s->ss, s->pc, ip);
	//~ debug("ss(%d): %09.*A", s->ss, s->pc, ip);

	switch (opc) {
		error_opc:
			error(s->s, 0, "invalid opcode: op[%.*A]", s->pc, ip);
			//~ fputasm(stderr, s, s->seg, -1, 0x10);
			return 0;

		error_stc:
			error(s->s, 0, "stack underflow: op[%.*A]", s->pc, ip);
			//~ fputasm(stderr, s, s->seg, -1, 0x10);
			return 0;

		#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
		#define NEXT(__IP, __CHK, __SP)\
			STOP(error_stc, s->ss < (__CHK));\
			s->ss += (__SP);\
			s->pos += (__IP);
		#include "incl/vm.h"
	}
	
	if (opc == opc_call) {
		//~ after a call ends with a return,
		//~ wich drops 1 item from the stack.
		s->ss -= 1;
	}

	//~ debug("ss(%d): %09.*A", s->ss * 4, s->pc, ip);

	if (s->sm < s->ss)
		s->sm = s->ss;

	return s->pc;
}
int emiti32(vmState s, int32_t arg) {
	stkval tmp;
	tmp.i4 = arg;
	return emitarg(s, opc_ldc4, tmp);
}
int emiti64(vmState s, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(s, opc_ldc8, tmp);
}
int emitf32(vmState s, float32_t arg) {
	stkval tmp;
	tmp.f4 = arg;
	return emitarg(s, opc_ldcf, tmp);
}
int emitf64(vmState s, float64_t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return emitarg(s, opc_ldcF, tmp);
}
int emitptr(vmState s, void* arg) {
	stkval tmp;
	unsigned char* ptr = (unsigned char*)arg;
	//~ dieif(ptr < s->_mem, "invalid reference");
	dieif(ptr < s->_mem || ptr > (s->_mem + s->ds), "FixMe");
	tmp.i8 = (ptr - s->_mem);
	return emitarg(s, opc_ldcr, tmp);
}

int emitopc(vmState s, int opc) {
	stkval arg;
	arg.i8 = 0;
	return emitarg(s, opc, arg);
}
int emitidx(vmState s, int opc, int arg) {
	stkval tmp;
	tmp.i8 = (int32_t)s->ss * 4 - arg;

	if (opc == opc_ldsp || opc == opc_drop)
		return emitarg(s, opc, tmp);

	if (tmp.u4 > 255) {
		debug("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	if (tmp.u4 > s->ss * 4) {
		fatal("%d", tmp.u4);
		return 0;
	}
	return emitarg(s, opc, tmp);
}
int emitint(vmState s, int opc, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(s, opc, tmp);
}
int fixjump(vmState s, int src, int dst, int stc) {
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
			s->ss = stc / 4;
		else if (stc > 0)
			s->ss = stc / 4;
		return 1;
	}

	fatal("FixMe");
	return 0;
}

int stkoffs(vmState s, int size) {
	return +(s->ss * 4 + size);
}

/*static cell task(vmState ee, cell pu, int cc, int n, int cl, int dl) {
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

/*static int done(vmState ee, int cp) {
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
static int lobit(int a) {return a & -a;}
static int mtt(vmState ee, doWhat cmd, cell pu, int n) {
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
	return 0;
}
static inline int ovf(cell pu) {
	return (pu->sp - pu->bp) < 0;
}

/** exec
 * executes on a single thread
 * @arg env: enviroment
 * @arg cc: cell count
 * @arg ss: stack size
 * @return: error code
**/

static void dbugerr(state s, int pu, void *ip, long* bp, int ss, char *text) {
	vmState vm = s->vm;
	int IP, SP;


	IP = ((char*)ip) - ((char*)vm->_mem) - vm->pc;
	SP = ((char*)vm->_end) - ((char*)bp);

	error(vm->s, 0, ">exec:%s:[pu%02d][sp%02d]@%9.*A", text, pu, ss, IP, ip);
	//~ fputfmt(stdout, ">exec:[pu%02d][sp%02d]@%9.*A\n", pu, ss, IP, ip);
}

static int dbugpu(vmState vm, cell pu, dbgf dbg) {
	typedef uint32_t *stkptr;
	typedef uint8_t *memptr;

	long ms = vm->_end - vm->_mem;
	memptr mp = (void*)vm->_mem;
	stkptr st = (void*)pu->sp;

	for ( ; ; ) {

		register bcde ip = (void*)pu->ip;
		register stkptr sp = (void*)pu->sp;

		if (dbg && dbg(vm->s, 0, ip, (long*)sp, st - sp))
			return -9;

		switch (ip->opc) {		// exec

			stop_vm:
				//~ if (dbg) dbg(vm->s, 0, NULL, (long*)sp, st - sp);
				return 0;

			error_opc:
				dbugerr(vm->s, 0, ip, (long*)sp, st - sp, "invalid opcode");
				//~ error(vm->s, 0, "invalid opcode: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip);
				return -1;

			error_ovf:
				dbugerr(vm->s, 0, ip, (long*)sp, st - sp, "stack overflow");
				//~ error(vm->s, 0, "stack overflow: op[%.*A] / %d", (char*)ip - (char*)(vm->_mem), ip, (vm->_end - pu->sp) / 4);
				return -2;

			error_mem:
				dbugerr(vm->s, 0, ip, (long*)sp, st - sp, "segmentation fault");
				//~ error(vm->s, 0, "segmentation fault: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip);
				return -3;

			error_div:
				dbugerr(vm->s, 0, ip, (long*)sp, st - sp, "division by zero");
				//~ error(vm->s, 0, "division by zero: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip);
				return -4;

			#define NEXT(__IP, __CHK, __SP) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#define EXEC
			#include "incl/vm.h"
		}
	}

	return 0;
}

int exec(vmState vm, dbgf dbg) {
	struct cell pu[1];

	pu->ip = vm->_mem + vm->pc;
	pu->bp = pu->ip + vm->cs;
	//~ pu->bp = vm->_end - ss;
	pu->sp = vm->_end;

	/*if ((((int)pu->sp) & 3)) {
		error(vm->s, 0, "invalid statck size");
		return -99;
	}// */

	if (pu->bp < pu->ip) {
		error(vm->s, 0, "invalid statck size");
		return -99;
	}

	return dbugpu(vm, pu, dbg);
}

void fputopc(FILE *fout, unsigned char* ptr, int len, int offs) {
	int i;
	//~ unsigned char* ptr = (unsigned char*)ip;
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

		case opc_ldc1: fputfmt(fout, " %d", ip->arg.i1); break;
		case opc_ldc2: fputfmt(fout, " %d", ip->arg.i2); break;
		case opc_ldc4: fputfmt(fout, " %x", ip->arg.i4); break;
		case opc_ldc8: fputfmt(fout, " %D", ip->arg.i8); break;
		case opc_ldcf: fputfmt(fout, " %f", ip->arg.f4); break;
		case opc_ldcF: fputfmt(fout, " %F", ip->arg.f8); break;
		case opc_ldcr: fputfmt(fout, " %x", ip->arg.u4); break;

		case opc_libc:
			if (libcfnc[ip->idx].sym)
				fputfmt(fout, " %-T", libcfnc[ip->idx].sym);
			else
				fputfmt(fout, " %s", libcfnc[ip->idx].proto);
			break;

		case p4d_swz: {
			char c1 = "xyzw"[ip->idx >> 0 & 3];
			char c2 = "xyzw"[ip->idx >> 2 & 3];
			char c3 = "xyzw"[ip->idx >> 4 & 3];
			char c4 = "xyzw"[ip->idx >> 6 & 3];
			fprintf(fout, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->idx);
		} break;
	}
}
void fputasm(FILE *fout, vmState s, int mark, int end, int mode) {
	unsigned char* beg = getip(s, mark);
	int rel = mode & 0x10;// ? 0 : -1;
	int i, is = 12345;
	//~ int ss = 0;
	if (end == -1)
		end = s->pos - mark;

	switch (mode & 0x30) {
		case 0x00: rel = -1; break;
		case 0x10: rel = 0; break;
		case 0x20: rel = mark; break;
		case 0x30: rel = beg - s->_mem; break;
	}

	for (i = 0; i < end; i += is) {
		bcde ip = (bcde)(beg + i);

		switch (ip->opc) {
			error_opc: error(NULL, 0, "invalid opcode: %02x '%A'", ip->opc, ip); return;
			//~ #define NEXT(__IP, __CHK, __SP) {if (__IP) is = (__IP);/*  ss += (__SP); */}
			#define NEXT(__IP, __CHK, __SP) {if (__IP) is = (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "incl/vm.h"
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

	//~ if (typ && typ->kind == TYPE_enu)
		//~ typ = typ->type;

	if (typ) switch (typ->kind) {
		/*case TYPE_enu:
			typ = typ->type;
			// fall
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

					case TYPE_ref: {
						if (typ == type_str) {
							//~ ref = (stkval*)(s->_mem + ref->u4);
							fputfmt(fout, "\"%s\"", ref);
						}
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
			fputfmt(fout, "%T{", typ);

			if ((n = typ->size) < 0) {
				fputfmt(fout, "Unknown Dimension}");
				break;
			}
			else if (n > 100) {
				n = 10;
			}

			while (i < n) {
				if (base->kind == TYPE_arr)
					fputfmt(fout, "\n");

				vm_fputval(s, fout, base, (stkval*)((char*)ref + i * sizeOf(base)), ((flgs + 1) & 0xff));
				if (++i < n)
					fputfmt(fout, ", ");
			}
			if (base->kind == TYPE_arr)
				fputfmt(fout, "\n%I", flgs & 0xff);
			fputfmt(fout, "}");
		} break;
		default: {
			if (var != typ)
				fputfmt(fout, "%T:", var);
			fputfmt(fout, "%T[ERROR]", typ);
		} break;
	}
}

void vmInfo(FILE* out, vmState vm) {
	int i;
	fprintf(out, "stack minimum size: %d\n", vm->sm);
	i = vm->ds; fprintf(out, "data(@.%04x) size: %dM, %dK, %dB\n", 0, i >> 20, i >> 10, i);
	i = vm->cs; fprintf(out, "code(@.%04x) size: %dM, %dK, %dB\n", vm->pc, i >> 20, i >> 10, i);
	//~ i = vm->cs; fprintf(out, "code(@.%04x) size: %dM, %dK, %dB, %d instructions\n", vm->pc, i >> 20, i >> 10, i, vm->ic + 1);
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
