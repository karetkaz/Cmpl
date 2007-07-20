#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "node.h"
//~ #include "scan.c"

enum {
								// opc.type(i(int)/u(unsigned)/ na / )arg length
	opc_nop  = 0x00000000,		// ---							nop
	opc_pop  = 0x00000001,		// sp -= 4*i1;					pop.u1 0x01			[�, a, b, c => [�, a, b;
	opc_dup  = 0x00000002,		// sp += 4; sp(0) = sp(i1);		dup.u1 0x02			[�, a, b, c => [�, a, b, c, b;
	opc_set  = 0x00000003,		// sp(u1) = sp(0); pop 1;		set.u1 0x02			[�, a, b, c => [�, c, b;
	opc_ldc1 = 0x00000004,		// push; sp(0) = ic;			ldc.i1 0x02			[�, a, b, c => [�, a, b, c, 2;
	opc_ldc2 = 0x00000005,		// push; sp(0) = ic;			ldc.i2 0x0002		[�, a, b, c => [�, a, b, c, 2;
	opc_ldc4 = 0x00000006,		// push; sp(0) = ic;			ldc.i4 0x00000002	[�, a, b, c => [�, a, b, c, 2;
	opc_ldc  = 0x00000106,

	opc_ldi1 = 0x00000007,		// read(sp, sp(0), 1);			ldi.1;				[�, a, b, c => [�, a, b, (*(i1*)c);
	opc_ldi2 = 0x00000008,		// read(sp, sp(0), 2);			ldi.2;				[�, a, b, c => [�, a, b, (*(i2*)c);
	opc_ldi4 = 0x00000009,		// read(sp, sp(0), 4);			ldi.4;				[�, a, b, c => [�, a, b, (*(i4*)c);
	opc_ldi  = 0x00000109,

	opc_sti1 = 0x0000000a,		// m1[sp(0)] = sp(1); pop 2;	sti.1				[�, a, b, c => [�, a
	opc_sti2 = 0x0000000b,		// m2[sp(0)] = sp(1); pop 2;	sti.2				[�, a, b, c => [�, a
	opc_sti4 = 0x0000000c,		// m4[sp(0)] = sp(1); pop 2;	sti.4				[�, a, b, c => [�, a
	opc_sti  = 0x0000010c,

	//~ opc_idx1 = 0x0000000d,		// sp(1) += sp(0)*1; pop;		idx.b				[�, a, b, c => [�, a, c + b*1
	//~ opc_idx2 = 0x0000000e,		// sp(1) += sp(0)*2; pop;		idx.s				[�, a, b, c => [�, a, c + b*2
	//~ opc_idx4 = 0x0000000f,		// sp(1) += sp(0)*4; pop;		idx.l				[�, a, b, c => [�, a, c + b*4
	//~ opc_idx  = 0x0000010f,

	//~ opc_lod1 = 0x00000009,		// push; sp(0) = mem1[i4];		lod.b.i4			[�, a, b, c => [�, a, b, c, m.1[i];
	//~ opc_lod2 = 0x00000009,		// push; sp(0) = mem2[i4];		lod.s.i4			[�, a, b, c => [�, a, b, c, m.2[i];
	//~ opc_lod4 = 0x00000009,		// push; sp(0) = mem4[i4];		lod.l.i4			[�, a, b, c => [�, a, b, c, m.4[i];
	//~ opc_sto1 = 0x00000009,		// push; sp(0) = mem1[i4];		sto.b.i4			[�, a, b, c => [�, a, b;
	//~ opc_sto2 = 0x00000009,		// push; sp(0) = mem2[i4];		sto.s.i4			[�, a, b, c => [�, a, b;
	//~ opc_sto4 = 0x00000009,		// push; sp(0) = mem4[i4];		sto.l.i4			[�, a, b, c => [�, a, b;

	//~ _???	 = 0x00000010,		// ?
	//~ _???	 = 0x00000011,		// ?
	//~ _???	 = 0x00000012,		// ?
	//~ _???	 = 0x00000013,		// sp(1) += sp(0)*8; pop;
	opc_jmp  = 0x00000014,		// ip = i4;						jmp.i4 0x00000002	[�, a, b, c => [�, a, b, c
	opc_jnz  = 0x00000015,		// if sp(0) ip = i4; pop;		jnz.i4 0x00000002	[�, a, b, c => [�, a, b
	opc_jz   = 0x00000016,		// if !sp(0) ip = i4; pop;		jnz.i4 0x00000002	[�, a, b, c => [�, a, b
	//~ opc_jmpi = 0x00000000,		// ip = sp(0); pop;
	//~ opc_call = 0x00000000,		// ld4 ip; jmp i4;
	//~ opc_retn = 0x00000000,		// ip = sp(i4); pop i4;
	//~ opc_xch  = 0x00000000,		// xchg   sp(i1) sp(0);
	//~ opc_not  = 0x00000000,		// sp(0) = ~sp(0);
	//~ opc_or	 = 0x00000000,		// sp(1) |= sp(0); pop;
	//~ opc_xor  = 0x00000000,		// sp(1) ^= sp(0); pop;
	//~ opc_and  = 0x00000000,		// sp(1) &= sp(0); pop;
	//~ opc_shl  = 0x00000000,		// sp(1) <<= sp(0); pop;
	//~ opc_shr  = 0x00000000,		// sp(1) >>= sp(0); pop;
	//~ opc_sar  = 0x00000000,		// sp(1) >>= sp(0); pop;
	//~ opc_rol  = 0x00000000,		// sp(1) <|= sp(0); pop;
	//~ opc_ror  = 0x00000000,		// sp(1) >|= sp(0); pop;
	//~ opc_zxt1 = 0x00000000,		// sp(0) = sp(0) & 0xff;
	//~ opc_zxt2 = 0x00000000,		// sp(0) = sp(0) & 0xffff;
	//~ opc_sxt1 = 0x00000000,		// sp(0) = sp(0) ???;
	//~ opc_sxt2 = 0x00000000,		// sp(0) = sp(0) ???;
	//~ opc_ext1 = 0x00000000,		// zxt1; sp(0) = sp(0) << 24 | sp(0) << 16 | sp(0) << 08 | sp(0) << 00 ;
	//~ opc_ext2 = 0x00000000,		// zxt2; sp(0) = sp(0) << 16 | sp(0) << 00;
	opc_fork = 0x0000001d,		// ???
	//~ opc_join = 0x0000001e,		// ???
	opc_wait = 0x0000001e,		// ???
	opc_halt = 0x0000001f,		// jmp 0;
//~ ================================================================================
	opc_ineg = 0x00000020,			// sp(0) = -sp(0);			neg.i
	opc_iadd = 0x00000021,			// sp(1) += sp(0); pop;
	opc_isub = 0x00000022,			// sp(1) -= sp(0); pop;
	opc_imul = 0x00000023,			// sp(1) *= sp(0); pop;
	opc_idiv = 0x00000024,			// sp(1) /= sp(0); pop;
	opc_imod = 0x00000025,			// sp(1) %= sp(0); pop;
	opc_iceq = 0x00000026,			// sp(1) = sp(1) == sp(0) ? 1 : 0; pop;
	opc_icne = 0x00000027,			// sp(1) = sp(1) != sp(0) ? 1 : 0; pop;
	opc_iclt = 0x00000028,			// sp(1) = sp(1) <  sp(0) ? 1 : 0; pop;
	opc_icgt = 0x00000029,			// sp(1) = sp(1) >  sp(0) ? 1 : 0; pop;
	opc_icle = 0x0000002a,			// sp(1) = sp(1) <= sp(0) ? 1 : 0; pop;
	opc_icge = 0x0000002b,			// sp(1) = sp(1) >= sp(0) ? 1 : 0; pop;
	//~ _???	 = 0x0000002c,			// sp(0) = (bool)sp(0);
	opc_itof = 0x0000002d,			// sp(0) = (float)sp(0);
	//~ _???	 = 0x0000002e,			// sgn.ext.1
	//~ _???	 = 0x0000002f,			// sgn.ext.2
	opc_imad = 0x0000002f,				// sp(2) += sp(1)*sp(0); pop2;		mad.i  0x00000002	[�, a, b, c => [�, a + b * c
//~ ================================================================================
	opc_fneg = 0x00000030,			// sp(0) = -sp(0);
	opc_fadd = 0x00000031,			// sp(1) += sp(0); pop;
	opc_fsub = 0x00000032,			// sp(1) -= sp(0); pop;
	opc_fmul = 0x00000033,			// sp(1) *= sp(0); pop;
	opc_fdiv = 0x00000034,			// sp(1) /= sp(0); pop;
	opc_fmod = 0x00000035,			// sp(1) %= sp(0); pop;
	opc_fceq = 0x00000036,			// sp(1) = sp(1) == sp(0) ? 1 : 0; pop;
	opc_fcne = 0x00000037,			// sp(1) = sp(1) != sp(0) ? 1 : 0; pop;
	opc_fclt = 0x00000038,			// sp(1) = sp(1) <  sp(0) ? 1 : 0; pop;
	opc_fcgt = 0x00000039,			// sp(1) = sp(1) >  sp(0) ? 1 : 0; pop;
	opc_fcle = 0x0000003a,			// sp(1) = sp(1) <= sp(0) ? 1 : 0; pop;
	opc_fcge = 0x0000003b,			// sp(1) = sp(1) >= sp(0) ? 1 : 0; pop;
	opc_ftoi = 0x0000003c,			// sp(0) = (int)sp(0);
	//~ _???	 = 0x0000003d,			// test inf
	//~ _???	 = 0x0000003e,			// test nan
	//~ _???	 = 0x0000003f,			// 
	opc_iout = 0x000000f6,			// 
	opc_fout = 0x000000f7,			// 
	opc_sout = 0x000000f8,			// 
};

#pragma pack (1)
typedef struct {				// byte code decoder
	unsigned char		opc;	// opcode
	union {
		unsigned char	u1;		// imm unsigned ...
		unsigned short	u2;
		signed short	i2;
		unsigned long	u4;
		//~ float			f4;
		//~ char*			cp;
		//~ void*			vp;
		struct {
			unsigned char	dl;		// data length (copy n data from stack)
			signed short	cl;		// code length
		};
	};
} *bcdc;

static inline int vm_chks1(bcpu pu, int cnt) {		// check stack
	long *sp = (long*)(pu->sp + cnt);
	if (cnt < 0) {							// need space
		if (sp < pu->bp) {
			pu->error = "Stack overflow";
			return -1;
		}
	}
	else {									// must have cnt args
		if (sp > pu->bp + pu->ss) {
			pu->error = "Stack underflow";
			return -1;
		}
	}
	return 0;
}

static inline int vm_chks(bcpu pu, int cnt) {		// check stack
	long *sp = (long*)(pu->sp + cnt);

	if (sp > pu->bp + pu->ss) {
		pu->error = "Stack overflow";
		return -1;
	}
	else if (sp <= pu->bp) {
		pu->error = "Stack underflow";
		return -1;
	}
	return 0;
}

static inline int vm_wait(state s, int cid) {				// wait for a cpu to terminate
	if (s->vm.fcell & ~((2 << cid) - 1)) return 1;
	return 0;
}

static inline int vm_fork(state s, int cid) {		// find a free bcpu and return it;
	bcdc ip = (bcdc)s->vm.cel[cid].ip;
	int i;
	//~ if (cid) return 0;
	//~ if (ip->opc != opc_fork) return -1;
	
	for (i = cid; i < s->vm.ccnt; i += 1) {
		if (!s->vm.cel[i].ip) {
			long *sp = s->vm.cel[cid].bp + s->vm.cel[i].ss;
			s->vm.cel[i].sp = s->vm.cel[i].bp + s->vm.cel[i].ss;
			s->vm.cel[i].is = ((char*)ip) + ip->cl;
			s->vm.cel[i].ip = ((char*)ip) + 4;
			while (sp > s->vm.cel[cid].sp) {		// ip->dl
				*--s->vm.cel[i].sp = *--sp;
			}
			s->vm.fcell |= 1 << i;
			//~ printf ("fork : %d : %d\n", cid, i);
			return i;
		}
	}
	//~ printf ("fork : %d : -1\n", cid);
	return 0;
}

int vm_casm(state s, int opc, unsigned long arg) {
	struct vm *vm = &s->vm;
	bcdc ip = (bcdc)s->vm.cend;
	if (s->vm.dcnt < 0) return 0;
	switch (opc) {
		default : break;
		case opc_nop  : {
			ip->opc = opc_nop;
			vm->cend += 1;
		} break;
		case opc_pop  : {				// pop n / 2nop
			if (arg > 0xff || arg > s->vm.dcnt) {
				raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OPC_pop(%d)", __FILE__, __LINE__, arg);
				return 0;
			}
			ip->opc = opc_pop;
			ip->u1 = arg;
			vm->cend += 2;
			vm->dcnt -= arg;
		} break;
		case opc_dup  : {
			if (arg > 0xff || arg > s->vm.dcnt) {
				raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OPC_dup(%d)", __FILE__, __LINE__, arg);
				return 0;
			}
			ip->opc = opc_dup;
			ip->u1 = arg;
			vm->cend += 2;
			vm->dcnt += 1;
		} break;
		case opc_set  : {
			if (arg > 0xff || arg > s->vm.dcnt) {
				raiseerror(s, -999, 0, "%s:%d: <unhandled exception> OPC_set(%d)", __FILE__, __LINE__, arg);
				return 0;
			}
			ip->opc = opc_set;
			ip->u1 = arg;
			vm->cend += 2;
			vm->dcnt -= 1;
		} break;
		case opc_ldc1 : {
			ip->opc = opc_ldc1;
			ip->u1 = arg;
			vm->cend += 2;
			vm->dcnt += 1;
		} break;
		case opc_ldc2 : {
			ip->opc = opc_ldc2;
			ip->u2 = arg;
			vm->cend += 3;
			vm->dcnt += 1;
		} break;
		case opc_ldc4 : {
			ip->opc = opc_ldc4;
			ip->u4 = arg;
			vm->cend += 5;
			vm->dcnt += 1;
		} break;
		case opc_ldi1 : {
			ip->opc = opc_ldi1;
			vm->cend += 1;
		} break;
		case opc_ldi2 : {
			ip->opc = opc_ldi2;
			vm->cend += 1;
		} break;
		case opc_ldi4 : {
			ip->opc = opc_ldi4;
			vm->cend += 1;
		} break;
		case opc_sti1 : {
			ip->opc = opc_sti1;
			vm->cend += 1;
			vm->dcnt -= 2;
		} break;
		case opc_sti2 : {
			ip->opc = opc_sti2;
			vm->cend += 1;
			vm->dcnt -= 2;
		} break;
		case opc_sti4 : {
			ip->opc = opc_sti4;
			vm->cend += 1;
			vm->dcnt -= 2;
		} break;
		case opc_jmp  : {
			ip->opc = opc_jmp;
			//~ ip->u4 = arg;
			ip->i2 = 0;
			//~ vm->cend += 5;
			vm->cend += 3;
		} break;
		case opc_jnz  : {
			ip->opc = opc_jnz;
			//~ ip->u4 = arg;
			ip->i2 = 0;
			//~ vm->cend += 5;
			vm->cend += 3;
			vm->dcnt -= 1;
		} break;
		case opc_jz   : {
			ip->opc = opc_jz;
			//~ ip->u4 = arg;
			ip->i2 = 0;
			//~ vm->cend += 5;
			vm->cend += 3;
			vm->dcnt -= 1;
		} break;
		case opc_halt : {
			ip->opc = opc_halt;
			vm->cend += 1;
		} break;
		case opc_fork : {
			union {
				unsigned long val;
				struct {
					unsigned char	dl;		// data length (copy n data from stack)
					unsigned short	cl;		// code length
				}arg;
			} tmp = {arg};
			ip->opc = opc_fork;
			ip->dl = tmp.arg.dl;
			ip->cl = tmp.arg.cl;
			vm->cend += 4;
		} break;
		case opc_wait : {
			ip->opc = opc_wait;
			vm->cend += 1;
		} break;
		//{ Integer
		case opc_ineg : {
			ip->opc = opc_ineg;
			vm->cend += 1;
		} break;
		case opc_iadd : {
			ip->opc = opc_iadd;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_isub : {
			ip->opc = opc_isub;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_imul : {
			ip->opc = opc_imul;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_idiv : {
			ip->opc = opc_idiv;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_imod : {
			ip->opc = opc_imod;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_iceq : {
			ip->opc = opc_iceq;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_icne : {
			ip->opc = opc_icne;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_iclt : {
			ip->opc = opc_iclt;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_icgt : {
			ip->opc = opc_icgt;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_icle : {
			ip->opc = opc_icle;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_icge : {
			ip->opc = opc_icge;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		//}
		//{ Float
		case opc_fneg : {
			ip->opc = opc_fneg;
			vm->cend += 1;
		} break;
		case opc_fadd : {
			ip->opc = opc_fadd;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fsub : {
			ip->opc = opc_fsub;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fmul : {
			ip->opc = opc_fmul;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fdiv : {
			ip->opc = opc_fdiv;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fmod : {
			ip->opc = opc_fmod;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fceq : {
			ip->opc = opc_fceq;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fcne : {
			ip->opc = opc_fcne;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fclt : {
			ip->opc = opc_fclt;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fcgt : {
			ip->opc = opc_fcgt;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fcle : {
			ip->opc = opc_fcle;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		case opc_fcge : {
			ip->opc = opc_fcge;
			vm->cend += 1;
			vm->dcnt -= 1;
		} break;
		//}
		case opc_ftoi : {
			ip->opc = opc_ftoi;
			vm->cend += 1;
		} break;
		case opc_itof : {
			ip->opc = opc_itof;
			vm->cend += 1;
		} break;
		case opc_imad : {
			ip->opc = opc_imad;
			vm->cend += 1;
			vm->dcnt -= 2;
		} break;

		case opc_iout : {
			ip->opc = opc_iout;
			vm->cend += 1;
		} break;
		case opc_fout : {
			ip->opc = opc_fout;
			vm->cend += 1;
		} break;
		case opc_sout : {
			ip->opc = opc_sout;
			vm->cend += 1;
		} break;

		case opc_ldc  : {
			if (arg <= 0xff) {
				ip->opc = opc_ldc1;
				ip->u1 = arg;
				vm->cend += 2;
				vm->dcnt += 1;
				break;
			}
			else if (arg <= 0xffff) {
				ip->opc = opc_ldc2;
				ip->u2 = arg;
				vm->cend += 3;
				vm->dcnt += 1;
				break;
			}
			else {
				ip->opc = opc_ldc4;
				ip->u4 = arg;
				vm->cend += 5;
				vm->dcnt += 1;
				break;
			}
		} break;
		case opc_ldi  : switch (arg) {
			case 1 : {
				ip->opc = opc_ldi1;
				vm->cend += 1;
			} break;
			case 2 : {
				ip->opc = opc_ldi2;
				vm->cend += 1;
			} break;
			case 4 : {
				ip->opc = opc_ldi4;
				vm->cend += 1;
			} break;
		} break;
		case opc_sti  : switch (arg) {
			case 1 : {
				ip->opc = opc_sti1;
				vm->cend += 1;
				vm->dcnt -= 2;
			} break;
			case 2 : {
				ip->opc = opc_sti2;
				vm->cend += 1;
				vm->dcnt -= 2;
			} break;
			case 4 : {
				ip->opc = opc_sti4;
				vm->cend += 1;
				vm->dcnt -= 2;
			} break;
		} break;
	}
	return ip == (bcdc)vm->cend;
	//~ return -1;
}

int clen(char* src) {
	bcdc ip = (bcdc)src;
	switch (ip->opc) {
		//~ default : return -1;
		case opc_nop  : return 1;
		case opc_pop  : return 2;
		case opc_dup  : return 2;
		case opc_set  : return 2;
		case opc_ldc1 : return 2;
		case opc_ldc2 : return 3;
		case opc_ldc4 : return 5;
		case opc_ldi1 : return 1;
		case opc_ldi2 : return 1;
		case opc_ldi4 : return 1;
		case opc_sti1 : return 1;
		case opc_sti2 : return 1;
		case opc_sti4 : return 1;
		case opc_jmp  : return 3;
		case opc_jnz  : return 3;
		case opc_jz   : return 3;
		case opc_halt : return 1;
		case opc_fork : return 4;
		case opc_wait : return 1;

		case opc_ineg : return 1;
		case opc_iadd : return 1;
		case opc_isub : return 1;
		case opc_imul : return 1;
		case opc_idiv : return 1;
		case opc_imod : return 1;
		case opc_iceq : return 1;
		case opc_icne : return 1;
		case opc_iclt : return 1;
		case opc_icgt : return 1;
		case opc_icle : return 1;
		case opc_icge : return 1;

		case opc_fneg : return 1;
		case opc_fadd : return 1;
		case opc_fsub : return 1;
		case opc_fmul : return 1;
		case opc_fdiv : return 1;
		case opc_fmod : return 1;
		case opc_fceq : return 1;
		case opc_fcne : return 1;
		case opc_fclt : return 1;
		case opc_fcgt : return 1;
		case opc_fcle : return 1;
		case opc_fcge : return 1;
		case opc_itof : return 1;
		case opc_imad : return 1;

		case opc_ftoi : return 1;

		case opc_iout : return 1;
		case opc_fout : return 1;
		case opc_sout : return 1;
	}
	return -1;
}//*/

//~ /* mem rd, wt
static inline int vm_mrd4(state s, long ofs, void* stp) {		// read memory
	if (ofs < 256 || ofs > s->data) return -1;
	memcpy(stp, s->buffer + ofs, 4);
	//~ pu->error = "segmentation fault";
	return 0;
}

static inline int vm_mwt4(state s, long ofs, void* stp) {		// write memory
	if (ofs < 256 || ofs > s->data) return -1;
	//~ memcpy((void*)ofs, stp, 4);
	memcpy(s->buffer + ofs, stp, 4);
	//~ pu->error = "segmentation fault";
	return 0;
}
// * /

int cexe(state s, bcpu pu) {
	bcdc ip = (bcdc)pu->ip;
	switch (ip->opc) {
		default : return ip->opc;
		case opc_nop  : {
			pu->ip += 1;
		} break;
		case opc_pop  : {				// pop n / 2nop
			if (vm_chks(pu, ip->u1)) return -1;
			pu->sp += ip->u1;
			pu->ip += 2;
		} break;
		case opc_set  : {				// set i / 2nop
			long *sp = pu->sp;
			if (vm_chks(pu, ip->u1)) return -1;
			sp[ip->u1] = sp[0];
			pu->sp += 1;
			pu->ip += 2;
		} break;
		case opc_dup  : {
			long *sp = pu->sp - 1;
			if (vm_chks(pu, ip->u1)) return -1;
			if (vm_chks(pu, -1)) return -1;
			sp[0] = pu->sp[ip->u1];
			pu->sp -= 1;
			pu->ip += 2;
		} break;
		case opc_ldc1 : {				// load constant
			long *sp = pu->sp - 1;
			if (vm_chks(pu, -1)) return -1;
			sp[0] = ip->u1;
			pu->sp -= 1;
			pu->ip += 2;
		} break;
		case opc_ldc2 : {
			long *sp = pu->sp - 1;
			if (vm_chks(pu, -1)) return -1;
			sp[0] = ip->u2;
			pu->sp -= 1;
			pu->ip += 3;
		} break;
		case opc_ldc4 : {
			long *sp = pu->sp - 1;
			if (vm_chks(pu, -1)) return -1;
			sp[0] = ip->u4;
			pu->sp -= 1;
			pu->ip += 5;
		} break;
		case opc_ldi4 : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			if (vm_mrd4(s, sp[0], sp)) {
				pu->error = "segmentation fault";
				return -1;
			}
			//~ sp[0] = *(long*)(s->buffer + sp[0]);
			pu->ip += 1;
		} break;
		case opc_sti4 : {
			long *sp = pu->sp;
			if (vm_chks(pu, 2)) return -1;
			if (vm_mwt4(s, sp[1], sp)) {
				pu->error = "segmentation fault";
				return -1;
			}
			//~ *(long*)(s->buffer + sp[1]) = sp[0];
			pu->sp += 2;
			pu->ip += 1;
		} break;
		case opc_jmp  : {				// jump
			pu->ip += ip->i2;
		} break;
		case opc_jnz  : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			if (sp[0] != 0) pu->ip += ip->i2;
			else pu->ip += 3;
			pu->sp += 1;
		} break;
		case opc_jz   : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			if (sp[0] == 0) pu->ip += ip->i2;
			else pu->ip += 3;
			pu->sp += 1;
		} break;
		case opc_halt : {
			pu->ip = NULL;
		} break;
		case opc_fork : {
			int cid = vm_fork(s, pu - s->vm.cel);
			if (cid < 0) return -1;
			if (cid > 0)
				pu->ip += ip->cl;
			else pu->ip += 4;
		} break;
		case opc_wait : {
			int cid = vm_wait(s, pu - s->vm.cel);
			if (cid < 0) return -1;					// error
			if (cid > 0) return 0;					// wait
			pu->ip += 1;							// continue
		} break;
		//{ Integer
		case opc_ineg : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[0] = -sp[0];
			pu->ip += 1;
		} break;
		case opc_iadd : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] + sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_isub : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] - sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_imul : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] * sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_idiv : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			if (sp[0] == 0) ;
			else sp[1] = sp[1] / sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_imod : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			if (sp[0] == 0) ;
			else sp[1] = sp[1] % sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_iceq : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] == sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icne : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] != sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_iclt : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] < sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icgt : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] >  sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icle : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] <= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icge : {
			long *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] >= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		//}
		//{ Float
		case opc_fneg : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[0] = -sp[0];
			pu->ip += 1;
		} break;
		case opc_fadd : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] + sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fsub : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] - sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fmul : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] * sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fdiv : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			if (sp[0] == 0) ;
			else sp[1] = sp[1] / sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fmod : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			if (sp[0] == 0) ;
			else sp[1] = fmod(sp[1], sp[0]);
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fceq : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] == sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcne : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] != sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fclt : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] < sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcgt : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] >  sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcle : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] <= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcge : {
			float *sp = (float*)pu->sp;
			if (vm_chks(pu, 1)) return -1;
			sp[1] = sp[1] >= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		//}
		case opc_iout : {
			void *sp = pu->sp;
			printf("%d", *(int*)sp);
			pu->ip += 1;
		} break;
		case opc_fout : {
			void *sp = pu->sp;
			printf("%.2f", *(float*)sp);
			pu->ip += 1;
		} break;
		case opc_sout : {
			int *sp = (int*)pu->sp;
			printf("%s", s->buffer+sp[0]);
			pu->ip += 1;
		} break;
		case opc_itof : {
			void *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			*(float*)sp = *(int*)sp;
			pu->ip += 1;
		} break;
		case opc_ftoi : {
			void *sp = pu->sp;
			if (vm_chks(pu, 1)) return -1;
			*(int*)sp = *(float*)sp;
			pu->ip += 1;
		} break;
		case opc_imad : {
			long *sp = (long*)pu->sp;
			if (vm_chks(pu, 2)) return -1;
			sp[2] += sp[1] * sp[0];
			pu->sp += 2;
			pu->ip += 1;
		} break;
	}
	return 0;
}//*/

int cexe2(state s, bcpu pu) {
	bcdc ip = (bcdc)pu->ip;
	switch (ip->opc) {
		default : return ip->opc;
		case opc_nop  : {
			pu->ip += 1;
		} break;
		case opc_pop  : {				// pop n / 2nop
			pu->sp += ip->u1;
			pu->ip += 2;
		} break;
		case opc_set  : {				// set i / 2nop
			long *sp = pu->sp;
			sp[ip->u1] = sp[0];
			pu->sp += 1;
			pu->ip += 2;
		} break;
		case opc_dup  : {
			long *sp = pu->sp - 1;
			sp[0] = pu->sp[ip->u1];
			pu->sp -= 1;
			pu->ip += 2;
		} break;
		case opc_ldc1 : {				// load constant
			long *sp = pu->sp - 1;
			sp[0] = ip->u1;
			pu->sp -= 1;
			pu->ip += 2;
		} break;
		case opc_ldc2 : {
			long *sp = pu->sp - 1;
			sp[0] = ip->u2;
			pu->sp -= 1;
			pu->ip += 3;
		} break;
		case opc_ldc4 : {
			long *sp = pu->sp - 1;
			sp[0] = ip->u4;
			pu->sp -= 1;
			pu->ip += 5;
		} break;
		case opc_ldi4 : {
			long *sp = pu->sp;
			sp[0] = *(long*)(s->buffer + sp[0]);
			pu->ip += 1;
		} break;
		case opc_sti4 : {
			long *sp = pu->sp;
			*(long*)(s->buffer + sp[1]) = sp[0];
			pu->sp += 2;
			pu->ip += 1;
		} break;
		case opc_jmp  : {				// jump
			pu->ip += ip->i2;
		} break;
		case opc_jnz  : {
			long *sp = pu->sp;
			if (sp[0] != 0) pu->ip += ip->i2;
			else pu->ip += 3;
			pu->sp += 1;
		} break;
		case opc_jz   : {
			long *sp = pu->sp;
			if (sp[0] == 0) pu->ip += ip->i2;
			else pu->ip += 3;
			pu->sp += 1;
		} break;
		case opc_halt : {
			pu->ip = NULL;
		} break;
		case opc_fork : {
			int cid = vm_fork(s, pu - s->vm.cel);
			if (cid < 0) return -1;
			if (cid > 0)
				pu->ip += ip->cl;
			else pu->ip += 4;
		} break;
		case opc_wait : {
			int cid = vm_wait(s, pu - s->vm.cel);
			if (cid < 0) return -1;					// error
			if (cid > 0) return 0;					// wait
			pu->ip += 1;							// continue
		} break;
		//{ Integer
		case opc_ineg : {
			long *sp = pu->sp;
			sp[0] = -sp[0];
			pu->ip += 1;
		} break;
		case opc_iadd : {
			long *sp = pu->sp;
			sp[1] = sp[1] + sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_isub : {
			long *sp = pu->sp;
			sp[1] = sp[1] - sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_imul : {
			long *sp = pu->sp;
			sp[1] = sp[1] * sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_idiv : {
			long *sp = pu->sp;
			if (sp[0] == 0) ;
			else sp[1] = sp[1] / sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_imod : {
			long *sp = pu->sp;
			if (sp[0] == 0) ;
			else sp[1] = sp[1] % sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_iceq : {
			long *sp = pu->sp;
			sp[1] = sp[1] == sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icne : {
			long *sp = pu->sp;
			sp[1] = sp[1] != sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_iclt : {
			long *sp = pu->sp;
			sp[1] = sp[1] < sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icgt : {
			long *sp = pu->sp;
			sp[1] = sp[1] >  sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icle : {
			long *sp = pu->sp;
			sp[1] = sp[1] <= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_icge : {
			long *sp = pu->sp;
			sp[1] = sp[1] >= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		//}
		//{ Float
		case opc_fneg : {
			float *sp = (float*)pu->sp;
			sp[0] = -sp[0];
			pu->ip += 1;
		} break;
		case opc_fadd : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] + sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fsub : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] - sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fmul : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] * sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fdiv : {
			float *sp = (float*)pu->sp;
			if (sp[0] == 0) ;
			else sp[1] = sp[1] / sp[0];
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fmod : {
			float *sp = (float*)pu->sp;
			if (sp[0] == 0) ;
			else sp[1] = fmod(sp[1], sp[0]);
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fceq : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] == sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcne : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] != sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fclt : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] < sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcgt : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] >  sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcle : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] <= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		case opc_fcge : {
			float *sp = (float*)pu->sp;
			sp[1] = sp[1] >= sp[0] ? 1 : 0;
			pu->sp += 1;
			pu->ip += 1;
		} break;
		//}
		case opc_iout : {
			void *sp = pu->sp;
			printf("%d", *(int*)sp);
			pu->ip += 1;
		} break;
		case opc_fout : {
			void *sp = pu->sp;
			printf("%.2f", *(float*)sp);
			pu->ip += 1;
		} break;
		case opc_sout : {
			int *sp = (int*)pu->sp;
			printf("%s", s->buffer+sp[0]);
			pu->ip += 1;
		} break;
		case opc_itof : {
			void *sp = pu->sp;
			*(float*)sp = *(int*)sp;
			pu->ip += 1;
		} break;
		case opc_ftoi : {
			void *sp = pu->sp;
			*(int*)sp = *(float*)sp;
			pu->ip += 1;
		} break;
		case opc_imad : {
			long *sp = (long*)pu->sp;
			sp[2] += sp[1] * sp[0];
			pu->sp += 2;
			pu->ip += 1;
		} break;
	}
	return 0;
}

static inline unsigned long argflt(double arg) {
	union{
		unsigned long val;
		float	arg;
	} tmp = {0};
	tmp.arg = arg;
	return tmp.val;
}

static inline unsigned long argptr(void* arg) {
	union{
		unsigned long val;
		void*	arg;
	} tmp = {0};
	tmp.arg = arg;
	return tmp.val;
}

static inline unsigned long argdcl(unsigned char dl, unsigned short cl) {
	union{
		unsigned long val;
		struct {
			unsigned char	dl;		// data length (copy n data from stack)
			unsigned short	cl;		// code length
		}arg;
	} tmp = {0};
	tmp.arg.dl = dl;
	tmp.arg.cl = cl;
	return tmp.val;
}

char* dasm(char* src) {					// disassemble
	static char tmp[64];
	char *dst = tmp+12;
	bcdc ip = (bcdc)src;
	sprintf(tmp, "0x%p: ", ip); 
	switch (ip->opc) {
		default : sprintf(dst, "???????error"); break;
		case opc_nop  : sprintf(dst, "nop"); break;
		case opc_pop  : sprintf(dst, "pop.u1 %d", ip->u1); break;
		case opc_dup  : sprintf(dst, "dup.u1 %d", ip->u1); break;
		case opc_set  : sprintf(dst, "set.u1 %d", ip->u1); break;
		case opc_ldc1 : sprintf(dst, "ldc.i1 %d", (int)ip->u1); break;
		case opc_ldc2 : sprintf(dst, "ldc.i2 %d", (int)ip->u2); break;
		case opc_ldc4 : sprintf(dst, "ldc.i4 %d", (int)ip->u4); break;
		case opc_ldi1 : sprintf(dst, "ldi.1"); break;
		case opc_ldi2 : sprintf(dst, "ldi.2"); break;
		case opc_ldi4 : sprintf(dst, "ldi.4"); break;
		case opc_sti1 : sprintf(dst, "sti.1"); break;
		case opc_sti2 : sprintf(dst, "sti.2"); break;
		case opc_sti4 : sprintf(dst, "sti.4"); break;
		//~ case opc_fork : sprintf(dst, "fork %d,%d:0x%p", ip->dl, ip->cl, src + ip->cl); break;
		//~ case opc_jmp  : sprintf(dst, "jmp.4 %+d:0x%p", ip->i2, src + ip->i2); break;
		//~ case opc_jnz  : sprintf(dst, "jnz.4 %+d:0x%p", ip->i2, src + ip->i2); break;
		//~ case opc_jz   : sprintf(dst, "jz.4 %+d:0x%p", ip->i2, src + ip->i2); break;
		case opc_fork : sprintf(dst, "fork %d,%d", ip->dl, ip->cl); break;
		case opc_jmp  : sprintf(dst, "jmp.i2 %+d", ip->i2); break;
		case opc_jnz  : sprintf(dst, "jnz.i2 %+d", ip->i2); break;
		case opc_jz   : sprintf(dst, "jz.i2 %+d", ip->i2); break;
		case opc_wait : sprintf(dst, "wait"); break;
		case opc_halt : sprintf(dst, "halt"); break;
		case opc_ineg : sprintf(dst, "neg.i"); break;
		case opc_iadd : sprintf(dst, "add.i"); break;
		case opc_isub : sprintf(dst, "sub.i"); break;
		case opc_imul : sprintf(dst, "mul.i"); break;
		case opc_idiv : sprintf(dst, "div.i"); break;
		case opc_imod : sprintf(dst, "mod.i"); break;
		case opc_iceq : sprintf(dst, "ceq.i"); break;
		case opc_icne : sprintf(dst, "cne.i"); break;
		case opc_iclt : sprintf(dst, "clt.i"); break;
		case opc_icgt : sprintf(dst, "cgt.i"); break;
		case opc_icle : sprintf(dst, "cle.i"); break;
		case opc_icge : sprintf(dst, "cge.i"); break;
		case opc_fneg : sprintf(dst, "neg.f"); break;
		case opc_fadd : sprintf(dst, "add.f"); break;
		case opc_fsub : sprintf(dst, "sub.f"); break;
		case opc_fmul : sprintf(dst, "mul.f"); break;
		case opc_fdiv : sprintf(dst, "div.f"); break;
		case opc_fmod : sprintf(dst, "mod.f"); break;
		case opc_fceq : sprintf(dst, "ceq.f"); break;
		case opc_fcne : sprintf(dst, "cne.f"); break;
		case opc_fclt : sprintf(dst, "clt.f"); break;
		case opc_fcgt : sprintf(dst, "cgt.f"); break;
		case opc_fcle : sprintf(dst, "cle.f"); break;
		case opc_fcge : sprintf(dst, "cge.f"); break;
		case opc_itof : sprintf(dst, "itof"); break;
		case opc_imad : sprintf(dst, "mad.i"); break;
		case opc_ftoi : sprintf(dst, "ftoi"); break;

		case opc_iout : sprintf(dst, "out.i"); break;
		case opc_fout : sprintf(dst, "out.f"); break;
		case opc_sout : sprintf(dst, "out.s"); break;
	}
	return dst;
	return tmp;
}

int vm_jset(void* dst, char* offs) {
	bcdc  ip = (bcdc)dst;
	int   jl = (offs - (char*)dst);
	if (jl > 32760) {
		printf("%d, %s\n", jl, dasm(dst));
		//~ printf("%d\n", i2);
		return -1;
	}
	else if (jl < -32760) {
		printf("%d, %s\n", jl, dasm(dst));
		//~ printf("%d\n", i2);
		return -1;
	}
	switch (ip->opc) {
		case opc_fork : ip->cl = jl; break;
		case opc_jmp  : ip->i2 = jl; break;
		case opc_jnz  : ip->i2 = jl; break;
		case opc_jz   : ip->i2 = jl; break;
		default       : return -1;
	}
	return 0;
}

int vm_exec(state s, int cc) {
	register int i = cc;
	auto int ss = 256;
	long *ce = (long *)(s->buffer + bufmaxlen);
	while (i--) {
		ce -= ss;
		s->vm.cel[i].pi = 0;
		s->vm.cel[i].ip = 0;
		s->vm.cel[i].ss = ss;
		s->vm.cel[i].bp = ce;
		s->vm.cel[i].sp = ce + ss - 1;
	}
	if (ce < (long*)(s->buffp + 65536/2)) {
		raiseerror(s, -15, 0, "innternal error Memory overrun");
		return -3;
	}
	s->vm.cel[0].ip = s->vm.cbeg;
	s->vm.cel[0].is = s->vm.cend;
	s->vm.ccnt = cc;
	s->vm.fcell = 1;					// start first cell
	while (s->vm.fcell) {
		for (i=0; i < cc; i++) {
			//~ if (s->vm.cel[i].ip == 0) {
				//~ s->vm.fcell &= ~(1<<i);
				//~ continue;
			//~ }
			if (s->vm.cel[i].ip && s->vm.cel[i].ip < s->vm.cel[i].is) {
				//~ char *code = s->vm.cel[i].ip;
				switch (cexe2(s, &(s->vm.cel[i]))) {
					case  0 : {
						s->vm.cel[i].pi += 1;
						/*
						//~ if (i) {
						int *stp;
						printf("cell[%02x] : %-32s", i, dasm(code));
						for(stp = (int*)s->vm.cel[i].bp + s->vm.cel[i].ss - 1; stp >= (int*)s->vm.cel[i].sp; stp--)
							printf("%d, ", *stp);
						printf("\n");
						fflush(stdout);
						//~ }
						//*/
						//~ code = 0;
						break;
					}
					case -1 : {
						printf("vm_error : %s : %s\n", s->vm.cel[i].error, dasm(s->vm.cel[i].ip));
						s->vm.cel[i].ip = NULL;
						s->vm.fcell &= ~(1<<i);
					} break;
					default : {
						printf("invalid opcode %02x\n", *s->vm.cel[i].ip);
						s->vm.cel[i].ip = NULL;
						s->vm.fcell &= ~(1<<i);
					} break;
				}
			}
			else {
				s->vm.cel[i].ip = NULL;
				s->vm.fcell &= ~(1<<i);
			}
		}
	}
	return 0;
}

int vm_dasm(state s, char* fn) {
	FILE *fout;
	if (!fn) fout = stdout;
	else if(!(fout = fopen(fn, "wt"))) {
		return -1;
	}
	for (fn = s->vm.cbeg; fn < s->vm.cend;) {
		fprintf(fout, dasm(fn));
		fprintf(fout, "\n");
		fn += clen(fn);
	}
	return 0;
}

int vm_open(state s, char* file) {
	return -1;
}

int vm_dump(state s, char* file) {
	return -1;
}

void vm_info(state s) {
	int i, MKB = s->data, b='b';
	if (MKB > 1<<30) {
		b = 'G';
		MKB >>= 30;
	}
	else if (MKB > 1<<20) {
		b = 'M';
		MKB >>= 20;
	}
	else if (MKB > 1<<10) {
		b = 'K';
		MKB >>= 10;
	}
	else b = 'B';
	//~ if (!s->vm.ccnt) return;
	printf("data : %d bytes (%d%c)\n", s->data, MKB, b);
	printf("code : %d bytes\n", (int)(s->vm.cend - s->vm.cbeg));
	//~ printf("time : %d ticks\n", tme);
	for (i = 0; i < s->vm.ccnt; i += 1) {
		printf ("cell[%2d] executed %d instructions\n", i, (int)s->vm.cel[i].pi);
	}
}

void vm_done();

/*
struct state_t s;

int main() {
	//~ char *incr, *test, *done, *ji, *jt, *jd;
	//~ printf("%d\n", vm_init(&vm, 256, 1));
	s.buffp = s.buffer;
	//~ s.vm.cbeg = s.vm.cbeg = s.buffer;
	//~ vm_init(&s, 256, 1);
	s.vm.cbeg = s.vm.cend = s.buffer;
	//~ return -1;
	//~ ip = vm.cel[0].ip;
	//~ s.vm.cbeg = s.vm.cbeg = s.buffer;
	//~ s.vm.dcnt = 0;
	vm_casm(&s, opc_ldc, argflt(10));
	vm_casm(&s, opc_ldc, argflt(20));
	vm_casm(&s, opc_fadd, 0);
	vm_casm(&s, opc_fout, 0);
	/ *{ for (i=0;i<2;i = i+1) nop;
	//~ init
	ip += casm(ip, opc_ldc, 0);			// i=0
	ip += casm(jt = ip, opc_jmp, 0);	// jump to test
	incr = ip;
	ip += casm(ip, opc_ldc, 1);			// 1
	ip += casm(ip, opc_iadd, 0);		// i = i + 1
	test = ip;
	ip += casm(ip, opc_dup, 0);			// i
	ip += casm(ip, opc_ldc, 5);			// 5
	ip += casm(ip, opc_iclt, 0);		// i < 5 ?
	ip += casm(jd = ip, opc_jz, 0);		// jmp end ?
	//~ stmt
	ip += casm(ip, opc_iout, 0);		// statements

	ip += casm(ji = ip, opc_jmp, 0);	// jmp to incr
	done = ip;

	casm(jt, opc_jmp, argptr(test));	// jmp to test
	casm(ji, opc_jmp, argptr(incr));	// jmp to incr
	casm(jd,  opc_jz, argptr(done));	// end if test fail.
	//~ ip += casm(ip, opc_iout, 0);		// 
	ip += casm(ip, opc_pop, 1);			// 
	//~ get a break list ?
	//}* /
	//~ * /{
	ip += casm(ip, opc_ldc, argptr("eragon"));
	ip += casm(ip, opc_sout, 0);
	ip += casm(ip, opc_ldc, argflt(3.14));
	ip += casm(ip, opc_fout, 0);
	ip += casm(ip, opc_ldc, 255);
	ip += casm(ip, opc_iout, 0);
	ip += casm(ip, opc_pop, 3);
	//~ casm(&vm, opc_halt, 0);
	//}* /

	//~ / *
	//~ vm_dasm(&s, 0);
	vm_exec(&s);
	//~ printf("0x%06x\n", (int)argdcl(0x02,0x0003));
	//~ printf("code size %d Bytes\n", ip - vm.code);
	//~ / *
	return 0;
}

//*/
//~ 774
