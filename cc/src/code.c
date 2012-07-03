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
		struct {		// when starting a task
			uint8_t  dl;	// data to to be copied to stack
			uint16_t cl;	// code to to be executed paralel: 0 means fork
		};
		/*struct {				// extended: 4 bytes `res := lhs OP rhs`
			uint32_t opc:3;		// 0 ... 7
			uint32_t pad:1;		//
			uint32_t mem:2;		// mem access
			uint32_t res:6;		// res
			uint32_t lhs:6;		// lhs
			uint32_t rhs:6;		// rhs
			/+ --8<-------------------------------------------
			void* res = sp + ip->ext.res;
			void* lhs = sp + ip->ext.lhs;
			void* rhs = sp + ip->ext.rhs;

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
	//~ dbgf dbg;
	unsigned char	*ip;		// Instruction pointer
	unsigned char	*sp;		// Stack pointer(bp + ss)
	unsigned char	*bp;		// Stack base

	// multiproc
	unsigned int	ss;			// stack size
	unsigned int	cp;			// child start(==0 join)
	unsigned int	pp;			// parent proc(==0 main)

	// flags ?
	//~ unsigned	zf:1;			// zero flag
	//~ unsigned	sf:1;			// sign flag
	//~ unsigned	cf:1;			// carry flag
	//~ unsigned	of:1;			// overflow flag
} *cell;

typedef uint8_t* memptr;
typedef uint32_t* stkptr;

static inline int isValidOffset(state rt, void* ptr) {
	if ((unsigned char*)ptr > rt->_mem + rt->_size) {
		return 0;
	}
	if (ptr && (unsigned char*)ptr < rt->_mem) {
		return 0;
	}
	return 1;
}

int vmOffset(state rt, void* ptr) {
	dieif(!isValidOffset(rt, ptr), "invalid reference");
	return ptr ? (unsigned char*)ptr - rt->_mem : 0;
}

static inline void* getip(state rt, int pos) {
	return (void*)(rt->_mem + pos);
}

//#{ libc.c ---------------------------------------------------------------------
//~ TODO: installref should go to type.c
static symn installref(state rt, const char* prot, astn* argv) {
	astn root, args;
	symn result = NULL;
	int level;
	int errc = rt->errc;

	if (!ccOpen(rt, NULL, 0, (char*)prot)) {
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
//~ TODO: libcall: parts of it should go to tree.c
symn ccAddCall(state rt, int libc(state, void* data), void* data, const char* proto) {
	symn arg, sym = NULL;
	int stdiff = 0;
	astn args = NULL;

	dieif(libc == NULL || !proto, "FixMe");

	//~ from: int64 zxt(int64 val, int offs, int bits)
	//~ make: define zxt(int64 val, int offs, int bits) = emit(int64, libc(25), i64(val), i32(offs), i32(bits));

	if ((sym = installref(rt, proto, &args))) {
		struct libc* lc = NULL;
		symn link = newdefn(rt->cc, EMIT_opc);
		astn libcinit;
		int libcpos = rt->cc->libc ? rt->cc->libc->pos + 1 : 0;

		dieif(rt->_end - rt->_beg < sizeof(struct libc), "FixMe");

		rt->_end -= sizeof(struct libc);
		lc = (struct libc*)rt->_end;
		lc->next = rt->cc->libc;
		rt->cc->libc = lc;

		link->name = "libc";
		link->type = sym->type;
		link->offs = opc_libc;
		link->init = intnode(rt->cc, libcpos);

		libcinit = lnknode(rt->cc, link);
		stdiff = fixargs(sym, 4, 0);

		// glue the new libcinit argument
		if (args && args != rt->cc->void_tag) {
			astn narg = newnode(rt->cc, OPER_com);
			astn arg = args;
			narg->op.lhso = libcinit;

			if (1) {
				symn s = NULL;
				astn arg = args;
				while (arg->kind == OPER_com) {
					astn n = arg->op.rhso;
					s = linkOf(n);
					arg = arg->op.lhso;
					if (s && n) {
						//~ logif(1, "%-T: %-T(%k) casts to %t", sym, s, n, s->cast);
						n->cst2 = s->cast;
					}
				}
				s = linkOf(arg);
				if (s && arg) {
					//~ logif(1, "%-T: %-T(%k) casts to %t", sym, s, arg, s->cast);
					arg->cst2 = s->cast;
				}
			}

			if (arg->kind == OPER_com) {
				while (arg->op.lhso->kind == OPER_com) {
					arg = arg->op.lhso;
				}
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
		lc->data = data;
		lc->pos = libcpos;
		lc->sym = sym;

		lc->chk = stdiff / 4;

		stdiff -= sizeOf(sym->type);
		lc->pop = stdiff / 4;

		// make arguments symbolic by default
		for (arg = sym->args; arg; arg = arg->next) {
			//~ arg->cast = arg->type->cast;
			if (arg->cast != TYPE_ref)
				arg->cast = TYPE_def;
		}
	}
	else {
		error(rt, NULL, 0, "install(`%s`)", proto);
	}

	return sym;
}
//#}

static inline int32_t bitlo(int32_t a) {return a & -a;}
static inline int32_t bitsf(uint32_t x) {
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
	libc libcvec = rt->vm.libv;
	bcde ip = getip(rt, rt->vm.pos);

	dieif((unsigned char*)ip + 16 >= rt->_end, "memory overrun");

	/*/ set max stack use.
	switch (opc) {
		case opc_dup1:
		case opc_set1:
			logif(1, "dupset(%d, %d)", rt->vm.su, arg.i4);
			if (rt->vm.su < arg.i4 + 1)
				rt->vm.su = arg.i4 + 1;
			break;
		case opc_dup2:
		case opc_set2:
			logif(1, "dupset(%d, %d)", rt->vm.su, arg.i4);
			if (rt->vm.su < arg.i4 + 2)
				rt->vm.su = arg.i4 + 2;
			break;
		case opc_dup4:
		case opc_set4:
			logif(1, "dupset(%d, %d)", rt->vm.su, arg.i4);
			if (rt->vm.su < arg.i4 + 4)
				rt->vm.su = arg.i4 + 4;
			break;
		case opc_ldi:
		case opc_ldi1:
		case opc_ldi2:
		case opc_ldi4:
		case opc_ldi8:
		case opc_ldiq:
		case opc_sti:
		case opc_sti1:
		case opc_sti2:
		case opc_sti4:
		case opc_sti8:
		case opc_stiq: {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp) {
				logif(1, "ldsp(%d, %d)", rt->vm.su, arg.i4);
				if (rt->vm.su < arg.i4 / 4)
					rt->vm.su = arg.i4 / 4;
			}
		} break;
	}// */

	if (opc == markIP) {
		return rt->vm.pos;
	}

	// arithm
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
	else if (opc == opc_ldi) {
		ip = getip(rt, rt->vm.pc);
		if (ip->opc == opc_ldsp) {
			int vardist = (ip->rel + arg.i4) / 4;
			if (rt->vm.su < vardist)
				rt->vm.su = vardist;
		}

		switch (arg.i4) {
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
	}
	else if (opc == opc_sti) {
		ip = getip(rt, rt->vm.pc);
		if (ip->opc == opc_ldsp) {
			int vardist = (ip->rel + arg.i4) / 4;
			if (rt->vm.su < vardist)
				rt->vm.su = vardist;
		}
		/*ip = getip(rt, rt->vm.pc);
		//~ dieif(ip->opc == opc_ldsp, "FixMe");
		if (ip->opc == opc_ldsp) {
			int vardist = (ip->rel + arg.i4) / 4;
			vardist -= rt->vm.ss - rt->vm.pp.sm;
			//~ logif(1, "ldsp(%d, %d)", rt->vm.pp.ss, arg.i4);
			if (rt->vm.pp.ss < vardist)
				rt->vm.pp.ss = vardist;
		}*/

		switch (arg.i4) {
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
		else if (opc == opc_jmpi) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_jmpi) {
				//~ rt->vm.pos = rt->vm.pc;
				return rt->vm.pc;
			}
		}
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
			//~ TODO: check other opcodes too
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

	logif(DEBUGGING > 10, ">cgen:[sp%02d]@%9.*A", rt->vm.ss, rt->vm.pc, ip);
	//~ fputfmt(stdout, ">cgen:[sp%02d:%08x]@%9.*A\n", rt->vm.ss, 0, rt->vm.pc, ip);

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
			dieif(1, "FixMe");
			break;

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			tmp.i8 /= vm_size;
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
	dieif(stc > 0 && stc & 3, "FixMe");
	if (src >= 0) {
		bcde ip = getip(rt, src);
		if (src) switch (ip->opc) {
			default:
				fatal("FixMe");
				break;
			case opc_task:
				ip->dl = stc / 4;
				ip->cl = dst - src;
				//~ dieif(ip->dl > vm_regs, "FixMe");
				dieif(ip->dl != stc / 4, "FixMe");
				dieif(ip->cl != dst - src, "FixMe");
				return 1;
			//~ case opc_ldsp:
			//~ case opc_call:
			case opc_jmp:
			case opc_jnz:
			case opc_jz:
				ip->rel = dst - src;
				break;
		}
		if (stc >= 0) {
			rt->vm.ss = stc / vm_size;
		}
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

#define traceProc(__PU, __CP, __MSG) logif(1, "%?s: {pu:%d, ip:%x, bp:%x, sp:%x, ss:%d, parent:%d, childs:%d}", __MSG, __CP, __PU[__CP].ip, __PU[__CP].bp, __PU[__CP].sp, __PU[__CP].ss, __PU[__CP].pp, __PU[__CP].cp);
/*static void traceProc(cell pu, int cp, char* msg) {
	//~ unsigned char	*ip;		// Instruction pointer
	//~ unsigned char	*sp;		// Stack pointer(bp + ss)
	//~ unsigned char	*bp;		// Stack base

	//~ unsigned int	ss;			// stack size
	//~ unsigned int	cp;			// child start(==0 join)
	//~ unsigned int	pp;			// parent proc(==0 main)
	logif(1, "%s: {pu:%d, ip:%x, bp:%x, sp:%x, stacksize:%d, parent:%d, childs:%d}", msg, cp, pu[cp].ip, pu[cp].bp, pu[cp].sp, pu[cp].ss, pu[cp].pp, pu[cp].cp);
}*/

//~ parallel processing
static int task(cell pu, int n, int master, int cl) {
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
		pu[slave].cp = 0;
		pu[slave].pp = master;

		pu[slave].ip = pu[master].ip;
		pu[slave].sp = pu[slave].bp + pu[slave].ss - cl;
		memcpy(pu[slave].sp, pu[master].sp, cl);
		//~ logif(1, "master(%d): starts: %d.", master, slave);
		//~ traceProc(pu, master, "master");
		//~ traceProc(pu, slave, "slave");

		return slave;
	}
	//~ else logif(1, "master(%d): continue.", master);
	return 0;
}

static int sync(cell pu, int cp, int wait) {
	int pp = pu[cp].pp;
	//~ traceProc(pu, cp, "join");

	// there are workers to wait for

	// slave proc
	//~ logif(1, "slave(%d) of %d finish.", cp, pp);
	if (pp != cp) {
		if (pu[cp].cp == 0) {
			//~ logif(1, "stop slave(%d).", cp);
			pu[cp].ip = NULL;
			pu[pp].cp -= 1;
			return 1;
		}
		return 0;
	}
	else {
		//~ logif(1, "master(%d): continue.", cp);
		if (pu[cp].cp > 0) {
			return !wait;
		}
	}

	/*if (pu[cp].wj > 0) {
		//~ logif(1, "master(%d): continue.", cp);
		pu[cp].wj -= 1;
		return 1;
	}// */

	// cell can continue to work
	return 1;
}

static inline int ovf(cell pu) {
	return (pu->sp - pu->bp) < 0;
}

static void dbugerr(state rt, cell cpu, char* file, int line, int pu, void* ip, stkptr bp, int ss, char* text, int xxx) {
	int IP = ((unsigned char*)ip) - rt->_mem;
	//~ error(rt, file, line, "exec:%s(%?d):[pu%02d][sp%02d]@%9.*A rw@%06x", text, xxx, pu, ss, IP, ip);
	error(rt, file, line, "exec:%s(%?d):[sp%02d]@%.*A rw@%06x", text, xxx, ss, IP, ip);
}

#if MAXPROCSEXEC > 1
static int dbgpu(state rt, cell cpu, const int cc) {
	char* err_file = 0;
	int err_line = 0;
	int err_code = 0;

	int cp = -1;
	const dbgf dbg = rt->dbug;			// debuger function
	const libc libcvec = rt->libv;		// libcalls

	const int ms = rt->_size;
	const int ro = rt->vm.ro;			// read only region
	const memptr mp = (void*)rt->_mem;	// memory begins here

	for ( ; ; ) {

		cell pu;
		register bcde ip;
		register stkptr sp;
		stkptr st;

		if ((cp += 1) >= cc)
			cp = 0;

		pu = &cpu[cp];
		ip = (void*)pu->ip;
		sp = (void*)pu->sp;

		if (ip == NULL)
			continue;

		st = (void*)(pu->bp + pu->ss);
		if (dbg && dbg(rt, cp, ip, (long*)sp, st - sp))
			return -9;

		switch (ip->opc) {
			stop_vm:	// halt virtual machine
				return 0;

			error_opc:
				dbugerr(rt, cpu, err_file, err_line, cp, ip, sp, st - sp, "invalid opcode", err_code);
				return -1;

			error_ovf:
				dbugerr(rt, cpu, err_file, err_line, cp, ip, sp, st - sp, "stack overflow", err_code);
				return -2;

			error_mem:
				dbugerr(rt, cpu, err_file, err_line, cp, ip, sp, st - sp, "segmentation fault", err_code);
				return -3;

			error_div_flt:
				dbugerr(rt, cpu, err_file, err_line, cp, ip, sp, st - sp, "division by zero", err_code);
				// continue execution on floating point division by zero.
				break;

			error_div:
				dbugerr(rt, cpu, err_file, err_line, 0, ip, sp, st - sp, "division by zero", err_code);
				return -4;

			error_libc:
				error(rt, err_file, err_line, "libc: %-T returned: %d", libcvec[ip->rel].sym);
				//~ dbugerr(rt, cpu, err_file, err_line, 0, ip, sp, st - sp, "libcall error", err_code);
				return -5;

			#define NEXT(__IP, __CHK, __SP) {pu->sp -= vm_size * (__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK, __ERC) if (__CHK) {err_file = __FILE__; err_line = __LINE__; err_code = __ERC; goto __ERR;}
			#define EXEC
			#include "code.inl"
		}
	}
	return 0;
}
#else
static int dbgpu(state rt, cell pu) {
	char* err_file = 0;
	int err_line = 0;
	int err_code = 0;

	const cell cpu = pu;
	const int cp = 0, cc = 1;
	const dbgf dbg = rt->vm.dbug;
	const libc libcvec = rt->vm.libv;

	const int ms = rt->_size;			// memory size
	const int ro = rt->vm.ro;			// read only region
	const memptr mp = (void*)rt->_mem;
	const stkptr st = (void*)(pu->bp + pu->ss);

	for ( ; ; ) {

		register bcde ip = (void*)pu->ip;
		register stkptr sp = (void*)pu->sp;

		if (dbg) {
			/* handle stacktrace
			switch (ip->opc) {
				case opc_libc:?
				case opc_call:
					*--rt->trace.sp = findsym(rt, mp + *sp);
					break;
				case opc_jmpi:
					rt->trace.sp++;
					break;
			}*/
			if (dbg(rt, cp, ip, (long*)sp, st - sp))
				return -9;
		}

		switch (ip->opc) {
			stop_vm:	// halt virtual machine
				return 0;

			error_opc:
				dbugerr(rt, pu, err_file, err_line, cp, ip, sp, st - sp, "invalid opcode", err_code);
				return -1;

			error_ovf:
				dbugerr(rt, cpu, err_file, err_line, cp, ip, sp, st - sp, "stack overflow", err_code);
				return -2;

			error_mem:
				dbugerr(rt, cpu, err_file, err_line, cp, ip, sp, st - sp, "segmentation fault", err_code);
				return -3;

			error_div_flt:
				dbugerr(rt, cpu, err_file, err_line, cp, ip, sp, st - sp, "division by zero", err_code);
				// continue execution on floating point division by zero.
				break;

			error_div:
				dbugerr(rt, cpu, err_file, err_line, 0, ip, sp, st - sp, "division by zero", err_code);
				return -4;

			error_libc:
				error(rt, err_file, err_line, "libc: %-T returned: %d", libcvec[ip->rel].sym, err_code);
				//~ dbugerr(rt, cpu, err_file, err_line, 0, ip, sp, st - sp, "libcall error", err_code);
				return -5;

			#define NEXT(__IP, __CHK, __SP) {pu->sp -= vm_size * (__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK, __ERC) if (__CHK) {err_file = __FILE__; err_line = __LINE__; err_code = __ERC; goto __ERR;}
			#define EXEC
			#include "code.inl"
		}
	}
	return 0;
}
#endif

/** vmExec
 * executes the script.
 * @arg rt: enviroment
 * @arg ss: the stack size (TODO)
 * @arg dbg: debugger function
 * @return: error code
**/
int vmExec(state rt, dbgf dbg, int ss) {
	//~ int i = 0;
	cell pu;
	rt->_end = rt->_mem + rt->_size;

	// allocate cells
	rt->_end -= sizeof(struct cell);
	rt->vm.cell = pu = (void*)rt->_end;

	/*ss /= MAXPROCSEXEC;
	for (i = 0; i < MAXPROCSEXEC; ++i) {
		rt->_end -= ss;

		pu[i].ip = NULL;

		pu[i].cp = 0;
		pu[i].pp = 0;

		pu[i].ss = ss;
		pu[i].bp = rt->_end;
		pu[i].sp = rt->_end + ss;
	}// */

	rt->_end -= ss;

	pu->cp = 0;
	pu->pp = 0;
	pu->ss = ss;
	pu->bp = rt->_end;
	pu->sp = rt->_end + ss;
	pu->ip = rt->_mem + rt->vm.pc;

	/*if ((((int)pu->sp) & (vm_size-1))) {
		error(vm->s, 0, "invalid statck size");
		return -99;
	}// */

	if (pu->bp > pu->sp) {
		error(rt, NULL, 0, "invalid statck size");
		return -99;
	}

	rt->cc = NULL;			// invalidate compiler
	rt->vm.cell = pu;
	rt->vm.dbug = dbg;

	// reinitialize memory manager
	rt->vm._free = rt->vm._used = NULL;

	// TODO argc, argv
	if (rt->vm.dbug) {
		fputfmt(stdout, "\n>> >> >> Initialize\n");
	}

	return dbgpu(rt, pu);
	//~ return dbgNpu(rt, pu, lengthof(pu));
}

/** vmCall
 * executes a function from the script.
 * vmExec should be called before this function
 * to initialize global variables.
 * @param state
 * @param fun the function to be executed.
 * @param args arguments for the function.
 * @return: error code
**/
int vmCall(state rt, symn fun, void* ret, void* args) {

	int result = 0;
	cell pu = rt->vm.cell;
	void* ip = pu->ip;
	void* sp = pu->sp;
	struct libcstate old = rt->libc;

	void* resp = NULL;
	int retsize = sizeOf(fun->type);

	//dieif(!rt || !fun, "FixMe");
	dieif(!(fun->kind == TYPE_ref && fun->call), "FixMe");

	// result is the last argument.
	resp = pu->sp - retsize;

	// make space for paramerters & result
	pu->sp -= fun->args->offs;

	if (args) {
		memcpy(pu->sp, args, fun->args->offs - retsize);
		//_watch: *(long (*)[4])(pu->sp)
	}

	// return here: vm->px: program exit
	pu->sp -= vm_size;
	*(int*)(pu->sp) = rt->vm.px;
	pu->ip = rt->_mem - fun->offs;

	if (rt->vm.dbug) {
		fputfmt(stdout, "\n>> >> >> Invoke: %-T\n", fun);
	}

	result = dbgpu(rt, pu);

	if (ret) {
		memcpy(ret, resp, retsize);
	}

	rt->libc = old;
	pu->ip = ip;
	pu->sp = sp;

	return result;
}

/*/ todo: 32 bits only
static int vmCall2(state rt, symn fun, ...) {
	int result = 0;
	if (fun != NULL) {
		va_list args;
		va_start(args, fun);
		result = vmCall(rt, fun, args);
		va_end(args);
	}
	return result;
	// return vmCall(rt, fun, ((char*)&fun) + sizeof(fun));
}*/

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

void fputopc(FILE* fout, unsigned char* ptr, int len, int offs, state rt) {
	bcde ip = (bcde)ptr;
	int i;

	if (offs >= 0)
		fputfmt(fout, ".%06x: ", offs);

	//~ TODO: writing data shold be a function
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
				fprintf(fout, " .%06x", offs + ip->rel);
			break;

		case opc_task:
			//~ fputfmt(fout, " %d, %d", ip->dl, ip->cl); break;
			if (offs < 0)
				fputfmt(fout, " %d, %d", ip->dl, ip->cl);
			else
				fprintf(fout, " %d, .%06x", ip->dl, offs + ip->cl);
			break;
		case opc_sync: fputfmt(fout, " %d", ip->idx); break;

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
				else if (rt->vm.libv) {
					lc = &((libc)rt->vm.libv)[ip->idx];
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
void fputasm(FILE* fout, state rt, int beg, int end, int mode) {
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

void vm_fputval(state rt, FILE* fout, symn var, stkval* ref, int level) {
	symn typ = var->kind == TYPE_ref ? var->type : var;
	char* fmt = var->pfmt ? var->pfmt : typ->pfmt;

	fputfmt(fout, "%I", level);

	if (var != typ) {
		fputfmt(fout, "%T: ", var);
	}

	if (var->cast == TYPE_ref) {		// by reference
		if (ref->u4 == 0) {				// null reference.
			fputfmt(fout, "%T(null)", typ);
			return;
		}
		ref = (stkval*)(rt->_mem + ref->u4);
	}

	if (!isValidOffset(rt, ref)) {
		fputfmt(fout, "%T(invalid reference)", typ);
		return;
	}

	switch (typ->kind) {
		case TYPE_rec: {
			symn tmp;
			int n = 0;

			if (var && var->call) {
				fputfmt(fout, "function @%d", -var->offs);
				break;
			}
			if (fmt != NULL) {
				switch (typ->size) {
					case 1: fputfmt(fout, fmt, ref->u1); break;
					case 2: fputfmt(fout, fmt, ref->u2); break;
					case 4: switch (typ->cast) {
						case TYPE_f32: fputfmt(fout, fmt, ref->f4); break;
						default: fputfmt(fout, fmt, ref->u4); break;
					} break;
					case 8: if (typ->cast == TYPE_f64) {
								fputfmt(fout, fmt, ref->f8);
							}
							else {
								fputfmt(fout, fmt, ref->i8);
							}
							break;
					default:
						fputfmt(fout, "%T(size: %d)", typ, typ->size);
						break;
				}
			}
			else {
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
			}
		} break;
		case TYPE_arr: {
			// TODO: typ->size / typ->type->size;
			int i, n = typ->size;
			symn base = typ->type;

			if (fmt != NULL) {
				// TODO: string uses this
				fputfmt(fout, fmt, ref);
			}
			else {
				int elementsOnNewLine = 0;
				int arrayHasMoreElements = 0;
				fputfmt(fout, "%T {", typ);

				if (var->cast == TYPE_arr) {
					n = ref->arr.length;
					ref = (stkval*)(rt->_mem + ref->u4);
				}

				#ifdef MAX_ARR_PRINT
				if (n > MAX_ARR_PRINT) {
					n = MAX_ARR_PRINT;
					arrayHasMoreElements = 1;
				}
				#endif

				if (base->kind == TYPE_arr)
					elementsOnNewLine = 1;

				if (base->kind == TYPE_rec && base->args)
					elementsOnNewLine = 1;

				for (i = 0; i < n; ++i) {
					if (i > 0)
						fputfmt(fout, ",");
					if (elementsOnNewLine)
						fputfmt(fout, "\n");
					else
						fputfmt(fout, " ");

					vm_fputval(rt, fout, base, (stkval*)((char*)ref + i * sizeOf(base)), elementsOnNewLine ? level + 1 : 0);
				}

				if (arrayHasMoreElements) {
					if (elementsOnNewLine)
						fputfmt(fout, ",\n%I...", level + 1);
					else
						fputfmt(fout, ", ...");
				}

				if (elementsOnNewLine) {
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

#if DEBUGGING
const int vmTest() {
	int i, e = 0;
	struct bcde ip[1];
	struct libc libcvec[1];
	ip->arg.i8 = 0;
	for (i = 0; i < opc_last; i++) {
		int err = 0;
		//~ ip->opc = i;
		if (opc_tbl[i].size == 0) continue;
		if (opc_tbl[i].code != i) {
			fprintf(stderr, "invalid '%s'[%02x]\n", opc_tbl[i].name, i);
			e += err = 1;
		}
		else switch (i) {
			error_len: e += 1; fputfmt(stderr, "opcode size 0x%02x: '%A'\n", i, ip); break;
			error_chk: e += 1; fputfmt(stderr, "stack check 0x%02x: '%A'\n", i, ip); break;
			error_dif: e += 1; fputfmt(stderr, "stack difference 0x%02x: '%A'\n", i, ip); break;
			error_opc: e += 1; fputfmt(stderr, "unimplemented opcode 0x%02x: '%A'\n", i, ip); break;
			#define NEXT(__IP, __CHK, __DIFF) {\
				if (opc_tbl[i].size && (__IP) && opc_tbl[i].size != (__IP)) goto error_len;\
				if (opc_tbl[i].chck != 9 && opc_tbl[i].chck != (__CHK)) goto error_chk;\
				if (opc_tbl[i].diff != 9 && opc_tbl[i].diff != (__DIFF)) goto error_dif;\
			}
			#define STOP(__ERR, __CHK, __ERR1) if (__CHK) goto __ERR
			#include "code.inl"
		}
	}
	return e;
}
const int vmHelp() {
	int i, e = 0;
	FILE* out = stdout;
	struct bcde ip[1];
	ip->arg.i8 = 0;

	for (i = 0; i < opc_last; i++) {
		int err = 0;

		if (!opc_tbl[i].name)
			continue;

		if (!opc_tbl[i].size)
			continue;

		fprintf(out, "\nInstruction: %s: #\n", opc_tbl[i].name);
		//~ fputfmt(out, "\topcode size: %d\n", opc_tbl[i].size);
		//~ fputfmt(out, "\tstack check: %d\n", opc_tbl[i].chck);
		//~ fputfmt(out, "\tstack difference: %d\n", opc_tbl[i].diff);

		fprintf(out, "Opcode		size		Stack trasition\n");
		fprintf(out, "0x%02x		%d		[..., a, b, c, d => [..., a, b, c, d#\n", opc_tbl[i].code, opc_tbl[i].size);

		fprintf(out, "\nDescription\n");
		if (opc_tbl[i].code != i) {
			e += err = 1;
		}
		else switch (i) {
			error_opc: e += err = 1; break;
			#define NEXT(__IP, __CHK, __SP) if (opc_tbl[i].size != (__IP)) goto error_opc;
			#define STOP(__ERR, __CHK, __ERR1) if (__CHK) goto error_opc
			#include "code.inl"
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
#endif
