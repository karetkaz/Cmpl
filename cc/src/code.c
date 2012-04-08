/*******************************************************************************
 *   File: code.c
 *   Date: 2011/06/23
 *   Desc: bytecode related stuff
 *******************************************************************************
code emmiting, executing and formatting
*******************************************************************************/
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "core.h"

#ifdef __WATCOMC__
#pragma disable_message(124);
//~ #pragma disable_message(201);
static inline double fmodf(float x, float y) {
	return fmod(x, y);
}
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
		/*struct {				// extended: 4 bytes `res := lhs OP rhs`
			uint32_t opc:4;		// 0 ... 15
			uint32_t mem:2;		// mem access
			uint32_t res:6;		// res
			uint32_t lhs:6;		// lhs
			uint32_t rhs:6;		// rhs
			/+ --8<-------------------------------------------
			void *res = sp + ip->ext.res;
			void *lhs = sp + ip->ext.lhs;
			void *rhs = sp + ip->ext.rhs;

			CHKSTK(ip->ext.res);
			CHKSTK(ip->ext.lhs);
			CHKSTK(ip->ext.rhs);

			// if there is an indirection
			switch (ip->ext.mem) {
				case 0:
					break;
				case 1:
					CHKMEM_W(*(int**)res);
					res = *(void**)res;
					break;
				case 2:
					CHKMEM_R(*(int**)lhs);
					lhs = *(void**)lhs;
					break;
				case 3:
					CHKMEM_R(*(int**)rhs);
					rhs = *(void**)rhs;
					break;
			}

			switch (ip->ext.opc) {
				case ext_neg: *(type*)res = -(*(type*)rhs); break;
				case ext_add: *(type*)res = (*(type*)lhs) + (*(type*)rhs); break;
				case ext_sub: *(type*)res = (*(type*)lhs) - (*(type*)rhs); break;
				case ext_mul: *(type*)res = (*(type*)lhs) * (*(type*)rhs); break;
				case ext_div: *(type*)res = (*(type*)lhs) / (*(type*)rhs); break;
				case ext_mod: *(type*)res = (*(type*)lhs) % (*(type*)rhs); break;
				case ext_...: res = lhs ... rhs; break;
			}
			// +/ --8<----------------------------------------
		} ext;// */
	};
} *bcde;
#pragma pack(pop)

typedef struct cell {			// processor
	unsigned char	*ip;		// Instruction pointer
	unsigned char	*sp;		// Stack pointer(bp + ss)
	unsigned char	*bp;		// Stack base

	// multiproc
	//~ unsigned int	cs;			// child start(==0 join)
	//~ unsigned int	pp;			// parent proc(==0 main)

	// flags ?
	//~ unsigned	zf:1;			// zero flag
	//~ unsigned	sf:1;			// sign flag
	//~ unsigned	cf:1;			// carry flag
	//~ unsigned	of:1;			// overflow flag
} *cell;

int vmOffset(state rt, void *ptr) {
	dieif((unsigned char*)ptr > rt->_mem + rt->_size, "invalid reference");
	dieif(ptr && (unsigned char*)ptr < rt->_mem, "invalid reference");
	return ptr ? (unsigned char*)ptr - rt->_mem : 0;
}

static inline void* getip(state rt, int pos) {
	return (void *)(rt->_mem + pos);
}

//{ libc.c ---------------------------------------------------------------------
TODO("installref should go to type.c")
static symn installref(state rt, const char *prot, astn *argv) {
	astn root, args;
	symn result = NULL;
	int level;
	int errc = rt->errc;

	if (!ccOpen(rt, NULL, 0, (char *)prot)) {
		trace("FixMe");
		return NULL;
	}

	rt->cc->warn = 9;
	level = rt->cc->nest;
	if ((root = decl_var(rt->cc, &args, decl_NoDefs | decl_NoInit))) {
		logif(!skip(rt->cc, STMT_do), "`;` expected declaring: %s", prot);
		dieif(peek(rt->cc), "FixMe");
		dieif(level != rt->cc->nest, "FixMe");
		dieif(root->kind != TYPE_ref, "FixMe %+k", root);

		if ((result = root->ref.link)) {
			dieif(result->kind != TYPE_ref, "FixMe");
			*argv = args;
			result->cast = 0;
		}
	}
	return errc == rt->errc ? result : NULL;
}

/* install a library call
 * @arg rt: the runtime state.
 * @arg libc: the function to call.
 * @arg proto: prototype of function.
 */
TODO("libcall: parts of it should go to tree.c")
symn libcall(state rt, int libc(state), const char* proto) {
	symn arg, sym = NULL;
	int stdiff = 0;
	astn args = NULL;

	dieif(libc == NULL || !proto, "FixMe");

	//~ from: int64 zxt(int64 val, int offs, int bits)
	//~ make: define zxt(int64 val, int offs, int bits) = emit(int64, libc(25), i64(val), i32(offs), i32(bits));

	if ((sym = installref(rt, proto, &args))) {
		struct libc *lc = NULL;
		symn link = newdefn(rt->cc, EMIT_opc);
		astn libcinit;
		int libcpos = rt->cc->libc ? rt->cc->libc->pos + 1 : 0;

		dieif(rt->_end - rt->_beg < sizeof(struct libc), "FixMe");

		rt->_end -= sizeof(struct libc);
		lc = (struct libc *)rt->_end;
		lc->next = rt->cc->libc;
		rt->cc->libc = lc;

		link->name = "libc";
		link->type = sym->type;
		link->offs = opc_libc;
		link->init = intnode(rt->cc, libcpos);

		libcinit = lnknode(rt->cc, link);

		// glue the new libcinit argument
		if (args && args != rt->cc->void_tag) {
			astn narg = newnode(rt->cc, OPER_com);
			astn arg = args;
			narg->op.lhso = libcinit;

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
			args = libcinit;
		}

		libcinit = newnode(rt->cc, OPER_fnc);
		libcinit->op.lhso = rt->cc->emit_tag;
		libcinit->type = sym->type;
		libcinit->op.rhso = args;

		sym->kind = TYPE_def;
		sym->init = libcinit;
		sym->offs = libcpos;
		//~ sym->size = libcpos;

		lc->call = libc;
		lc->pos = libcpos;
		lc->sym = sym;

		stdiff = fixargs(sym, 4, 0);
		lc->chk = stdiff / 4;

		stdiff -= sizeOf(sym->type);
		lc->pop = stdiff / 4;

		// make arguments symbolic by default
		for (arg = sym->args; arg; arg = arg->next) {
			if (arg->cast != TYPE_ref)
				arg->cast = TYPE_def;
		}
	}
	else {
		error(rt, NULL, 0, "install(`%s`)", proto);
	}

	return sym;
}
//}

static inline int lobit(int a) {return a & -a;}
static inline int bitsf(unsigned int x) {
	int ans = 0;
	if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
	if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
	if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
	if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000001) == 0) { ans +=  1; }
	return x ? ans : -1;
}

// emiting opcodes
int emitarg(state rt, vmOpcode opc, stkval arg) {
	libc libcvec = rt->libv;
	bcde ip = getip(rt, rt->vm.pos);

	dieif((unsigned char*)ip + 16 >= rt->_end, "memory overrun");

	if (opc == markIP) {
		return rt->vm.pos;
	}
	else if (opc == opc_neg) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_neg; break;
		case TYPE_i64: opc = i64_neg; break;
		case TYPE_f32: opc = f32_neg; break;
		case TYPE_f64: opc = f64_neg; break;
	}
	else if (opc == opc_add) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_add; break;
		case TYPE_i64: opc = i64_add; break;
		case TYPE_f32: opc = f32_add; break;
		case TYPE_f64: opc = f64_add; break;
	}
	else if (opc == opc_sub) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_sub; break;
		case TYPE_i64: opc = i64_sub; break;
		case TYPE_f32: opc = f32_sub; break;
		case TYPE_f64: opc = f64_sub; break;
	}
	else if (opc == opc_mul) switch (arg.i4) {
		case TYPE_u32: opc = u32_mul; break;
		case TYPE_i32: opc = i32_mul; break;
		case TYPE_i64: opc = i64_mul; break;
		case TYPE_f32: opc = f32_mul; break;
		case TYPE_f64: opc = f64_mul; break;
	}
	else if (opc == opc_div) switch (arg.i4) {
		case TYPE_u32: opc = u32_div; break;
		case TYPE_i32: opc = i32_div; break;
		case TYPE_i64: opc = i64_div; break;
		case TYPE_f32: opc = f32_div; break;
		case TYPE_f64: opc = f64_div; break;
	}
	else if (opc == opc_mod) switch (arg.i4) {
		case TYPE_u32: opc = u32_mod; break;
		case TYPE_i32: opc = i32_mod; break;
		case TYPE_i64: opc = i64_mod; break;
		case TYPE_f32: opc = f32_mod; break;
		case TYPE_f64: opc = f64_mod; break;
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
		if (emitarg(rt, opc_ceq, arg))
			opc = opc_not;
	}
	else if (opc == opc_cle) {
		if (emitarg(rt, opc_cgt, arg))
			opc = opc_not;
	}
	else if (opc == opc_cge) {
		if (emitarg(rt, opc_clt, arg))
			opc = opc_not;
	}

	// bit
	else if (opc == opc_cmt) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_cmt; break;
		//~ case TYPE_i64: opc = b64_cmt; break;
	}
	else if (opc == opc_shl) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_shl; break;
		//~ case TYPE_i64: opc = b64_shl; break;
	}
	else if (opc == opc_shr) switch (arg.i4) {
		case TYPE_u32: opc = b32_shr; break;
		case TYPE_i32: opc = b32_sar; break;
		//~ case TYPE_i64: opc = b64_sar; break;
	}
	else if (opc == opc_and) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_and; break;
		//~ case TYPE_i64: opc = b64_and; break;
	}
	else if (opc == opc_ior) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_ior; break;
		//~ case TYPE_i64: opc = b64_ior; break;
	}
	else if (opc == opc_xor) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_xor; break;
		//~ case TYPE_i64: opc = b64_xor; break;
	}

	// load/store
	else if (opc == opc_ldi) switch (arg.i4) {
		case  1: opc = opc_ldi1; break;
		case  2: opc = opc_ldi2; break;
		case  4: opc = opc_ldi4; break;
		case  8: opc = opc_ldi8; break;
		case 16: opc = opc_ldiq; break;
		default: // we have src on the stack

			// push dst
			if (!emitint(rt, opc_ldsp, 4 - arg.i4)) {
				trace("FixMe");
				return 0;
			}

			// copy
			if (!emitint(rt, opc_move, -arg.i4)) {
				trace("FixMe");
				return 0;
			}

			// create n bytes on stack
			arg.i8 = +arg.i8;
			opc = opc_spc;

			break;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case  1: opc = opc_sti1; break;
		case  2: opc = opc_sti2; break;
		case  4: opc = opc_sti4; break;
		case  8: opc = opc_sti8; break;
		case 16: opc = opc_stiq; break;
		default: // we have dst on the stack

			ip = getip(rt, rt->vm.pc);

			// push dst
			if (!emitint(rt, opc_ldsp, vm_size)) {
				trace("FixMe");
				return 0;
			}

			// copy
			if (!emitint(rt, opc_move, arg.i4)) {
				trace("FixMe");
				return 0;
			}

			if (ip->opc == opc_ldsp) {
				//~ if we copy from the stack to the stack
				//~ do not remove more elements than
				//~ we copy to
				if (arg.i8 > ip->rel) {
					arg.i8 = ip->rel;
				}
			}
			// remove n bytes from stack
			arg.i8 = -arg.i8;
			opc = opc_spc;
			break;
	}

	if (opc > opc_last) {
		fatal("invalid opc(0x%x, 0x%X)", opc, arg.i8);
		return 0;
	}

	// Optimize
	if (rt->vm.opti > 1) {
		if (0) {}

		else if (opc == opc_ldc8) {
			if (arg.i8 == 0) opc = opc_ldz2;
		}
		else if (opc == opc_ldc4) {
			// vm sign extends ints
			if (arg.i8 == 0) opc = opc_ldz1;
			//else if (arg.i8 <= 0xff) opc = opc_ldc1;
			//else if (arg.u8 <= 0xffff) opc = opc_ldc2;
		}

		/* memory access
		else if (opc == opc_ldi1) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup1;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}
		else if (opc == opc_ldi2) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup1;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}
		// */
		else if (opc == opc_ldi4) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_ld32;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup1;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}
		else if (opc == opc_sti4) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_st32;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set1;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}

		else if (opc == opc_ldi8) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_ld64;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup2;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}
		else if (opc == opc_sti8) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldcr && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_st64;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set2;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}

		else if (opc == opc_ldiq) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) <= vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup4;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}
		else if (opc == opc_stiq) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) <= vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set4;
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
			}
		}

		// */ grouping
		else if (opc == opc_not) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_not) {
				rt->vm.pos = rt->vm.pc;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
		}
		else if (opc == i32_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == i32_neg) {
				rt->vm.pos = rt->vm.pc;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_ldc4) {
				ip->arg.i4 = -ip->arg.i4;
				return rt->vm.pc;
			}
		}
		else if (opc == i64_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == i64_neg) {
				rt->vm.pos = rt->vm.pc;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_ldc8) {
				ip->arg.i8 = -ip->arg.i8;
				return rt->vm.pc;
			}
		}
		else if (opc == f32_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == f32_neg) {
				rt->vm.pos = rt->vm.pc;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_ldc4) {
				ip->arg.f4 = -ip->arg.f4;
				return rt->vm.pc;
			}
		}
		else if (opc == f64_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == f64_neg) {
				rt->vm.pos = rt->vm.pc;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_ldc8) {
				ip->arg.f8 = -ip->arg.f8;
				return rt->vm.pc;
			}
		}
		else if (opc == opc_inc) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_inc) {
				stkval tmp;
				tmp.i8 = arg.i8 + ip->rel;
				if (tmp.rel == tmp.i8) {
					ip->rel = tmp.rel;
					return rt->vm.pc;
				}
			}
		}
		else if (opc == opc_spc) {	/* still not good
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_spc) {
				stkval tmp;
				tmp.i8 = arg.i8 + ip->rel;
				if (tmp.rel == tmp.i8) {
					rt->vm.pos = rt->vm.pc;
					rt->vm.ss -= ip->rel / vm_size;
					arg.i8 += ip->rel;
				}
			}// */
		}

		// */ conditional jumps
		else if (opc == opc_jnz) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_not) {
				rt->vm.pos = rt->vm.pc;
				opc = opc_jz;
			}
		}
		else if (opc == opc_jz) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_not) {
				rt->vm.pos = rt->vm.pc;
				opc = opc_jnz;
			}
		}

		// */ shl, shr, and
		else if (opc == b32_shl) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldc4) {
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
				opc = b32_bit;
				arg.i4 = b32_bit_shl | (ip->arg.i4 & 0x3f);
			}
		}
		else if (opc == b32_shr) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldc4) {
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
				opc = b32_bit;
				arg.i4 = b32_bit_shr | (ip->arg.i4 & 0x3f);
			}
		}
		else if (opc == b32_sar) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldc4) {
				rt->vm.pos = rt->vm.pc;
				rt->vm.ss -= 1;
				opc = b32_bit;
				arg.i4 = b32_bit_sar | (ip->arg.i4 & 0x3f);
			}
		}
		else if (opc == b32_and) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldc4) {
				if ((ip->arg.i4 & (ip->arg.i4 + 1)) == 0) {
					rt->vm.pos = rt->vm.pc;
					rt->vm.ss -= 1;
					opc = b32_bit;
					arg.i4 = b32_bit_and | (bitsf(ip->arg.i4 + 1) & 0x3f);
				}
			}
		}

		else if (opc == i32_add) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldc4) {
				arg.i8 = ip->arg.i4;
				if (arg.rel == arg.i8) {
					rt->vm.pos = rt->vm.pc;
					rt->vm.ss -= 1;
					opc = opc_inc;
				}
			}
		}
		else if (opc == i32_sub) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldc4) {
				arg.i8 = -ip->arg.i4;
				if (arg.rel == arg.i8) {
					rt->vm.pos = rt->vm.pc;
					rt->vm.ss -= 1;
					opc = opc_inc;
				}
			}
		}

		/* mul, div, mod
		else if (opc == u32_mod) {
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

		/*else if (opc == opc_ldz1) {
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
		}*/
		/*else if (opc == opc_spc) {
			dieif(arg.i8 & (vm_size-1), "FixMe");
			if (arg.i8 < 0) {
				if (-arg.i8 < vm_regs * vm_size) {
					arg.i8 /= -vm_size;
					opc = opc_drop;
				}
			}
			else {
				if (+arg.i8 < vm_regs * vm_size) {
					arg.i8 /= vm_size;
					opc = opc_loc;
				}
			}
				//~ return 0;
		}*/

		/*if (opc == opc_set) {
			int ipp, rm = 0;
			int op = 0, lhs, rhs;
			ip = getip(rt, rt->vm.pc);
			switch (ip->opc) {
				case i32_add:
				case i32_sub:
				case i32_mul:
				case i32_div:
				case i32_mod:
					op = i32_ext;
					break;
				case f32_add:
				case f32_sub:
				case f32_mul:
				case f32_div:
				case f32_mod:
					op = f32_ext;
					break;
				case i64_add:
				case i64_sub:
				case i64_mul:
				case i64_div:
				case i64_mod:
					op = i64_ext;
					break;
				case f64_add:
				case f64_sub:
				case f64_mul:
				case f64_div:
				case f64_mod:
					op = f64_ext;
					break;
			}
			if (op == i32_ext || op == f32_ext) {
				ip = getip(rt, rt->vm.p[ipp = 1]);
				if (ip->opc == opc_ldi4 && rm == 0) {
					ip = getip(rt, rt->vm.p[ipp += 1]);
					rm = 1;
				}
				if (ip->opc == opc_dup1) {
					rhs = ip->idx;
					ip = getip(rt, rt->vm.p[ipp += 1]);
					if (ip->opc == opc_ldi4 && rm == 0) {
						ip = getip(rt, rt->vm.p[ipp += 1]);
						rm = 2;
					}
					if (ip->opc == opc_dup1) {
						lhs = ip->idx;
						opc = op;
					}
				}
			}
			if (op == i64_ext || op == f64_ext) {
				...
			}

			if (opc == op) {
				arg = extarg(arg, lhs, rhs, rm);
				ip = pc = rt->vm.p[ipp];
			}
		}*/
	}

	ip = getip(rt, rt->vm.pos);

	ip->opc = opc;
	ip->arg = arg;
	rt->vm.pc = rt->vm.pos;	// previous program pointer is here

	if ((opc == opc_jmp || opc == opc_jnz || opc == opc_jz) && arg.i8 != 0) {
		ip->rel -= rt->vm.pc;
		logif((ip->rel + rt->vm.pc) != arg.i8, "FixMe");
	}
	else switch (opc) {
		case opc_inc:
			dieif(ip->rel != arg.i8, "FixMe");
			break;

		case opc_spc:
			/*TODO: int max = 0x7fffff;
			while (arg.i8 > max) {
				if (!emit(s, opc, max))
					return 0;
				arg.i8 -= max;
			}
			//~ ip->rel = arg.i8 / vm_size;
			// */
			if (arg.i8 > 0x7fffff) {
				trace("max 2 megs can be allocated on stack, not(%D)", arg.i8);
				return 0;
			}
			dieif(ip->rel != arg.i8, "FixMe");
			break;

		case opc_ldsp:
		case opc_move:
			dieif(ip->rel != arg.i8, "FixMe");
			break;

		case opc_drop:
		case opc_loc:
			dieif(ip->idx != arg.i8, "FixMe");
			break;

		default:
			TODO("check other opcodes too");
			break;
	}

	switch (opc) {
		error_opc:
			error(rt, NULL, 0, "invalid opcode: op[%.*A]", rt->vm.pc, ip);
			return 0;

		error_stc:
			error(rt, NULL, 0, "stack underflow: op[%.*A](%d)", rt->vm.pc, ip, rt->vm.ss);
			return 0;

		#define STOP(__ERR, __CHK, __ERC) if (__CHK) goto __ERR
		#define NEXT(__IP, __CHK, __SP)\
			STOP(error_stc, rt->vm.ss < (__CHK), -1);\
			rt->vm.ss += (__SP);\
			rt->vm.pos += (__IP);
		#include "code.inl"
	}

	if (opc == opc_call) {
		//~ a call ends with a return,
		//~ wich drops ip from the stack.
		rt->vm.ss -= 1;
	}

	//~ debug("ss(%d): %09.*A", rt->ss * vm_size, rt->pc, ip);

	if (rt->vm.sm < rt->vm.ss)
		rt->vm.sm = rt->vm.ss;

	return rt->vm.pc;
}
int emitopc(state rt, vmOpcode opc) {
	stkval arg;
	arg.i8 = 0;
	return emitarg(rt, opc, arg);
}
int emitinc(state rt, int arg) {
	if (rt->vm.opti > 0) {
		bcde ip = getip(rt, rt->vm.pc);

		if (arg == 0) {
			return rt->vm.pc;
		}

		switch (ip->opc) {
			default:
				break;

			case opc_ldsp:
				if (arg < 0)
					return 0;
				ip->rel += arg;
				return rt->vm.pc;

			case opc_ldcr:
				if (arg < 0)
					return 0;
				ip->rel += arg;
				return rt->vm.pc;
		}
	}
	return emitint(rt, opc_inc, arg);
}
int emitidx(state rt, vmOpcode opc, int arg) {
	stkval tmp;
	tmp.i8 = (int32_t)rt->vm.ss * vm_size - arg;

	switch (opc) {
		default:
			break;
		case opc_drop:
			opc = opc_spc;
			tmp.i8 = -tmp.i8;
			// no break
		case opc_ldsp:
			return emitarg(rt, opc, tmp);
	}

	if (tmp.i8 > 255) {
		debug("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	if (tmp.i8 > rt->vm.ss * vm_size) {
		debug("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	return emitarg(rt, opc, tmp);
}
int emitint(state rt, vmOpcode opc, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(rt, opc, tmp);
}
int fixjump(state rt, int src, int dst, int stc) {
	dieif(stc & 3, "FixMe");
	if (src >= 0) {
		bcde ip = getip(rt, src);
		if (src) switch (ip->opc) {
			default:
				fatal("FixMe");
				break;
			//~ case opc_ldsp:
			//~ case opc_call:
			case opc_jmp:
			case opc_jnz:
			case opc_jz:
				ip->rel = dst - src;
				break;
		}
		if (!src && !dst)
			rt->vm.ss = stc / vm_size;
		else if (stc > 0)
			rt->vm.ss = stc / vm_size;
		return 1;
	}

	fatal("FixMe");
	return 0;
}

// emiting constant values.
int emiti32(state rt, int32_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(rt, opc_ldc4, tmp);
}
int emiti64(state rt, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(rt, opc_ldc8, tmp);
}
int emitf32(state rt, float32_t arg) {
	stkval tmp;
	tmp.f4 = arg;
	return emitarg(rt, opc_ldcf, tmp);
}
int emitf64(state rt, float64_t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return emitarg(rt, opc_ldcF, tmp);
}
int emitref(state rt, void* arg) {
	stkval tmp;
	tmp.i8 = vmOffset(rt, arg);
	return emitarg(rt, opc_ldcr, tmp);
}

int stkoffs(state rt, int size) {
	logif(size < 0, "FixMe");
	return padded(size, vm_size) + rt->vm.ss * vm_size;
}

/* parallel processing
static cell task(state rt, cell pu, int n, int master, int cl, int dl) {
	// find an empty cpu
	int slave = master + 1;
	while (slave < n) {
		if (pu[slave].ip == NULL) {
			break;
		}
		slave += 1;
	}

	if (slave < n) {
		// set workers, parent, ip, sp and copy stack
		pu[master].cp++;
		pu[slave].pp = master;
		pu[slave].ip = pu[master].ip;
		pu[slave].sp = pu[master].bp + ss - cl;
		memcpy(pu[slave].sp, pu[master].sp, cl * vm_size);
		return pu + slave;
	}
	return NULL;
}

static int join(state rt, cell pu, int cp) {

	// there are workers to wait for
	if (pu[cp].cp > 0) {
		return 0;
	}
	// a slave finishes his work
	if (pu[cp].pp != cp) {
		pu[pu[cp].pp].cp -= 1;
		pu[cp].ip = NULL;
	}
	// cell can continue work
	return 1;
}
// */

static inline int ovf(cell pu) {
	return (pu->sp - pu->bp) < 0;
}
static void dbugerr(state rt, cell cpu, char *file, int line, int pu, void *ip, long* bp, int ss, char *text, int xxx) {
	int IP = ((unsigned char*)ip) - rt->_mem;
	//~ fputfmt(stderr, "pu: {ip: %d, sp: %d, bp: %d}\n", cpu->ip - rt->_mem, cpu->sp - rt->_mem, cpu->bp - rt->_mem);
	//~ error(rt, file, line, "exec:%s(%?d):[pu%02d][sp%02d]@%9.*A rw@%06x", text, xxx, pu, ss, IP, ip);
	error(rt, file, line, "exec:%s(%?d):[sp%02d]@%9.*A rw@%06x", text, xxx, ss, IP, ip);
}
static int dbugpu(state rt, cell pu, dbgf dbg) {
	typedef uint32_t *stkptr;
	typedef uint8_t *memptr;
	libc libcvec = rt->libv;

	long ms = rt->_size;
	memptr mp = (void*)rt->_mem;
	stkptr st = (void*)pu->sp;

	const int ro = rt->vm.ro;		// read only region
	char* err_file = 0;
	int err_line = 0;
	int err_code = 0;

	for ( ; ; ) {

		register bcde ip = (void*)pu->ip;
		register stkptr sp = (void*)pu->sp;

		if (dbg && dbg(rt, 0, ip, (long*)sp, st - sp))
			return -9;

		switch (ip->opc) {
			stop_vm:	// halt virtual mashine
				return 0;

			error_opc:
				dbugerr(rt, pu, err_file, err_line, 0, ip, (long*)sp, st - sp, "invalid opcode", err_code);
				return -1;

			error_ovf:
				dbugerr(rt, pu, err_file, err_line, 0, ip, (long*)sp, st - sp, "stack overflow", err_code);
				return -2;

			error_mem:
				dbugerr(rt, pu, err_file, err_line, 0, ip, (long*)sp, st - sp, "segmentation fault", err_code);
				return -3;

			error_div_flt:
				dbugerr(rt, pu, err_file, err_line, 0, ip, (long*)sp, st - sp, "division by zero", err_code);
				// continue execution on floating point division by zero.
				break;

			error_div:
				dbugerr(rt, pu, err_file, err_line, 0, ip, (long*)sp, st - sp, "division by zero", err_code);
				return -4;

			error_libc:
				dbugerr(rt, pu, err_file, err_line, 0, ip, (long*)sp, st - sp, "libcall error", err_code);
				return -5;

			#define NEXT(__IP, __CHK, __SP) {pu->sp -= vm_size * (__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK, __ERC) if (__CHK) {err_file = __FILE__; err_line = __LINE__; err_code = __ERC; goto __ERR;}
			#define EXEC
			#include "code.inl"
		}
	}
	return 0;
}

/** vmExec
 * executes the script.
 * @arg rt: enviroment
 * @arg ss: the stack size (TODO)
 * @arg dbg: debugger function
 * @return: error code
**/
int vmExec(state rt, dbgf dbg) {
	struct cell pu[1];

	pu->ip = rt->_mem + rt->vm.pc;
	pu->bp = rt->_mem + rt->vm.pos;		// the last emited instruction
	pu->sp = rt->_mem + rt->_size;

	/*if ((((int)pu->sp) & (vm_size-1))) {
		error(vm->s, 0, "invalid statck size");
		return -99;
	}// */

	if (pu->bp < pu->ip) {
		error(rt, NULL, 0, "invalid statck size");
		return -99;
	}

	rt->cc = NULL;		// invalidate compiler
	rt->_end = pu->sp;	// initiate stack position

	// reinitialize memory manager
	rt->_free = rt->_used = NULL;

	// TODO argc, argv
	return dbugpu(rt, pu, dbg);
}

/** vmCall
 * executes a function from the script.
 * vmExec should be called before this function
 * to initialize global variables.
 * @arg rt: enviroment
 * @arg fun: the function to be executed.
 * @arg...: arguments for the function.
 * @return: error code
**/
int vmCall(state rt, symn fun, ...) {

	symn arg;
	int argsize = 0;
	int result = 0;
	struct cell pu[1];

	char *resp = NULL;
	char *argp = NULL;
	char *ap = (char*)&fun + sizeof(fun);

	/*if (!s || !s->_ptr) {
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

	dieif(!(fun->kind == TYPE_ref && fun->call), "FixMe");

	pu->ip = rt->_mem - fun->offs;
	pu->bp = rt->_mem + rt->vm.pos;
	pu->sp = rt->_end;

	// push Arguments;
	//~ va_start(ap, fun);
	for (arg = fun->args; arg; arg = arg->next) {
		int size = sizeOf(arg->type);
		unsigned char *offs = pu->sp - arg->offs;
		if (arg->cast == TYPE_ref) {
			trace("by reference");
			return -2;
		}
		if (offs < pu->bp) {
			trace("stack overflow");
			return -3;
		}
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
	*(int*)(pu->sp) = rt->vm.px;

	if (pu->bp < pu->ip) {
		error(rt, NULL, 0, "invalid statck size");
		return -99;
	}

	result = dbugpu(rt, pu, NULL);
	rt->retv = resp;
	rt->argv = argp;
	return result;
}

// returns a statement for an ip (very slow)
static astn infoAt(state rt, int pos) {
	if (rt->cc) {
		int i;
		for (i = 0; i < rt->cc->dbg.cnt; i += 1) {
			list l = getBuff(&rt->cc->dbg, i);
			if (l && l->size == pos) {
				return (astn)l->data;
			}
		}
	}
	return NULL;
}

void fputopc(FILE *fout, unsigned char* ptr, int len, int offs, state rt) {
	bcde ip = (bcde)ptr;
	int i;

	if (offs >= 0)
		fputfmt(fout, ".%06x: ", offs);

	TODO("writing data shold be a function")
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

	if (opc_tbl[ip->opc].name)
		fputs(opc_tbl[ip->opc].name, fout);
	else
		fputfmt(fout, "opc%02x", ip->opc);

	switch (ip->opc) {
		case opc_inc:
		case opc_spc:
		case opc_ldsp:
		case opc_move:
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

		case b32_bit: switch (ip->idx & 0xc0) {
			case b32_bit_and: fprintf(fout, "and 0x%03x", (1 << (ip->idx & 0x3f)) - 1); break;
			case b32_bit_shl: fprintf(fout, "shl 0x%03x", ip->idx & 0x3f); break;
			case b32_bit_shr: fprintf(fout, "shr 0x%03x", ip->idx & 0x3f); break;
			case b32_bit_sar: fprintf(fout, "sar 0x%03x", ip->idx & 0x3f); break;
		} break;

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
			if (rt != NULL) {
				libc lc = NULL;
				if (rt->cc && rt->cc->libc) {
					lc = rt->cc->libc;
					while (lc) {
						if (lc->pos == ip->idx) {
							break;
						}
						lc = lc->next;
					}
				}
				else if (rt->libv) {
					lc = &((libc)rt->libv)[ip->idx];
				}
				if (lc && lc->sym) {
					fputfmt(fout, ": %-T", lc->sym);
				}
				else {
					fputfmt(fout, "(%d)", ip->idx);
				}
			}
			else {
				fputfmt(fout, "(%d)", ip->idx);
			}
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
void fputasm(FILE *fout, state rt, int beg, int end, int mode) {
	int i, is = 12345;
	int rel = 0;
	int stmtEnd = 0;
	astn ast = NULL;

	if (end == -1)
		end = rt->vm.pos;

	switch (mode & 0x30) {
		case 0x00: rel = -1; break;
		case 0x10: rel = 0; break;
		case 0x20: rel = beg; break;
		//~ case 0x30: rel = (unsigned char*)getip(s, mark) - s->_mem; break;
	}

	for (i = beg; i < end; i += is) {
		bcde ip = getip(rt, i);

		switch (ip->opc) {
			error_opc: error(rt, NULL, 0, "invalid opcode: %02x '%A'", ip->opc, ip); return;
			#define NEXT(__IP, __CHK, __SP) {if (__IP) is = (__IP);}
			#define STOP(__ERR, __CHK, __ERC) if (__CHK) goto __ERR
			#include "code.inl"
		}

		if (1 && (ast = infoAt(rt, i))) {
			if (stmtEnd) {
				fputc('}', fout);
				fputc('\n', fout);
			}
			fputfmt(fout, "%s:%d:%+k\n", ast->file, ast->line, ast);
			//~ stmtEnd = 1;
		}

		if (mode & 0xf00)
			fputfmt(fout, "%I", (mode & 0xf00) >> 8);

		fputopc(fout, (void*)ip, mode & 0xf, rel >= 0 ? (rel + i) : -1, rt);

		fputc('\n', fout);
	}
	if (stmtEnd) {
		fputc('}', fout);
		fputc('\n', fout);
	}
}

void vm_fputval(state rt, FILE *fout, symn var, stkval* ref, int level) {
	symn typ = var->kind == TYPE_ref ? var->type : var;
	if (typ) {
		fputfmt(fout, "%I", level);
		if (var != typ)
			fputfmt(fout, "%T: ", var);

		switch (typ->kind) {
			case TYPE_rec: {
				symn tmp;
				int n = 0;

				if (var->cast == TYPE_ref) {		// arrays, strings, functions, references
					if (ref->u4 == 0) {
						fputfmt(fout, "%T(null)", typ);
						break;
					}
					ref = (stkval*)(rt->_mem + ref->u4);
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
				else switch (typ->cast) {
					case TYPE_bit:
					case TYPE_u32:
						switch (typ->size) {
							case 1:
								fputfmt(fout, "%T(%u)", typ, ref->u1);
								break;
							case 2:
								fputfmt(fout, "%T(%u)", typ, ref->u2);
								break;
							case 4:
								fputfmt(fout, "%T(%u)", typ, ref->u4);
								break;
							case 8:
								fputfmt(fout, "%T(%U)", typ, ref->i8);
								break;
						}
						break;

					case TYPE_i32:
					case TYPE_i64:
						switch (typ->size) {
							case 1:
								fputfmt(fout, "%T(%d)", typ, ref->i1);
								break;
							case 2:
								fputfmt(fout, "%T(%d)", typ, ref->i2);
								break;
							case 4:
								fputfmt(fout, "%T(%d)", typ, ref->i4);
								break;
							case 8:
								fputfmt(fout, "%T(%D)", typ, ref->i8);
								break;
						}
						break;

					case TYPE_f32:
					case TYPE_f64:
						switch (typ->size) {
							case 4:
								fputfmt(fout, "%T(%f)", typ, ref->f4);
								break;
							case 8:
								fputfmt(fout, "%T(%F)", typ, ref->f8);
								break;
						}
						break;

					default: {
						fputfmt(fout, "%T {", typ);
						for (tmp = typ->args; tmp; tmp = tmp->next) {
							if (tmp->stat || tmp->kind != TYPE_ref)
								continue;

							if (tmp->pfmt && !*tmp->pfmt)
								continue;

							if (n > 0)
								fputfmt(fout, ",");

							fputfmt(fout, "\n");
							vm_fputval(rt, fout, tmp, (void*)((char*)ref + tmp->offs), level + 1);
							n += 1;
						}
						if (typ->args)
							fputfmt(fout, "\n");
						fputfmt(fout, "%I}", level, typ);
					} break;
				}
			} break;
			case TYPE_arr: {
				int i = 0, n = 0;
				symn base = typ->type;
				if (var->pfmt || typ->pfmt) {
					char *fmt = var->pfmt ? var->pfmt : typ->pfmt;
					if (ref->u4 == 0) {
						fputfmt(fout, "%T(null)", typ);
						break;
					}
					fputfmt(fout, fmt, rt->_mem + ref->u4);
				}
				else {
					int arronnln = 0;
					int arrMoreElements = 0;
					fputfmt(fout, "%T {", typ);
					if ((n = typ->size) < 0) {	// if (var->cst2 == TYPE_arr)
						n = ref->arr.length;
						ref = (stkval*)(rt->_mem + ref->u4);
					}
					#ifdef MAX_ARR_PRINT
					if (n > MAX_ARR_PRINT) {
						n = MAX_ARR_PRINT;
						arrMoreElements = 1;
					}
					#endif

					if (base->kind == TYPE_arr)
						arronnln = 1;
					if (base->kind == TYPE_rec && base->args)
						arronnln = 1;

					while (i < n) {
						if (arronnln)
							fputfmt(fout, "\n");

						vm_fputval(rt, fout, base, (stkval*)((char*)ref + i * sizeOf(base)), arronnln ? level + 1 : 0);
						if (++i < n) {
							fputfmt(fout, ",");
							if (!arronnln)
								fputfmt(fout, " ");
						}
					}
					if (arrMoreElements) {
						fputfmt(fout, ", ...");
					}
					if (arronnln) {
						fputfmt(fout, "\n%I", level);
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
}

#ifdef DEBUGGING
const int vmTest() {
	int e = 0;
	struct libc libcvec[1];
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
			#define STOP(__ERR, __CHK, __ERR1) if (__CHK) goto __ERR
			#include "code.inl"
		}
	}
	return e;
}
#endif
