#include "type.h"
#include "tree.h"
#include "code.h"
#include "debuger.h"
#include "printer.h"
#include "compiler.h"

#include <stdarg.h>

const char *const type_doc_builtin = "@builtin";
const char *const type_fmt_signed32 = "%d";
const char *const type_fmt_signed64 = "%D";
const char *const type_fmt_unsigned32 = "%u";
const char *const type_fmt_unsigned64 = "%U";
const char *const type_fmt_float32 = "%f";
const char *const type_fmt_float64 = "%F";
const char *const type_fmt_character = "%c";
const char *const type_fmt_string = "%s";
const char *const type_fmt_typename = "<%T>";
const char type_fmt_string_chr = '\"';
const char type_fmt_character_chr = '\'';

const char *const pluginLibImport = "cmplUnit";
const char *const pluginLibInstall = "cmplInit";
const char *const pluginLibDestroy = "cmplClose";

/// Private native function for reflection.
// return type for file and name will be converted to string
static const char *const type_get_file = "pointer file(typename type)";
static const char *const type_get_line = "int32 line(typename type)";
static const char *const type_get_name = "pointer name(typename type)";
static const char *const type_get_base = "typename base(typename type)";
static const char *const type_ref_size = "int refSize(typename type)";

static const char *const type_get_is_ind = "bool isIndirect(typename type)";
static const char *const type_get_is_opt = "bool isOptional(typename type)";
static const char *const type_get_is_mut = "bool isMutable(typename type)";
static const char *const type_get_lookup = "typename lookup(typename type)";
static const char *const type_get_fields = "typename fields(typename type)";
static const char *const type_get_field = "typename field(typename type, char name![*])";
static const char *const type_get_next = "typename next(typename type)";
static vmError typenameReflect(nfcContext ctx) {
	if (ctx->proto == type_get_field) {
		symn sym = rtLookup(ctx->rt, argRef(ctx, vm_ref_size), 0);
		char *name = vmPointer(ctx->rt, argRef(ctx, 0));
		if (sym == NULL || name == NULL) {
			return nativeCallError;
		}

		for (symn fld = sym->fields; fld != NULL; fld = fld->next) {
			if (strcmp(name, fld->name) == 0) {
				retRef(ctx, vmOffset(ctx->rt, fld));
				return noError;
			}
		}

		retRef(ctx, 0);
		return noError;
	}

	size_t symOffs = argRef(ctx, 0);
	if (ctx->proto == type_get_fields) {
		if (symOffs == 0) {
			symn sym = ctx->rt->main;
			retRef(ctx, vmOffset(ctx->rt, sym->fields));
		} else {
			symn sym = rtLookup(ctx->rt, symOffs, 0);
			retRef(ctx, vmOffset(ctx->rt, sym->fields));
		}
		return noError;
	}

	if (ctx->proto == type_get_lookup) {
		symn sym = rtLookup(ctx->rt, symOffs, 0);
		retRef(ctx, vmOffset(ctx->rt, sym));
		return noError;
	}

	if (symOffs == 0) {
		// null has no next
		return nativeCallError;
	}
	symn sym = vmPointer(ctx->rt, symOffs);

	if (ctx->proto == type_get_is_ind) {
		retI32(ctx, isIndirect(sym) != 0);
		return noError;
	}

	if (ctx->proto == type_get_is_opt) {
		retI32(ctx, isOptional(sym) != 0);
		return noError;
	}

	if (ctx->proto == type_get_is_mut) {
		retI32(ctx, isMutable(sym) != 0);
		return noError;
	}

	if (ctx->proto == type_get_next) {
		retRef(ctx, vmOffset(ctx->rt, sym->next));
		return noError;
	}

	if (ctx->proto == type_get_file) {
		retRef(ctx, vmOffset(ctx->rt, sym->file));
		return noError;
	}
	if (ctx->proto == type_get_line) {
		retI32(ctx, sym->line);
		return noError;
	}
	if (ctx->proto == type_get_name) {
		retRef(ctx, vmOffset(ctx->rt, sym->name));
		return noError;
	}
	if (ctx->proto == type_get_base) {
		retRef(ctx, vmOffset(ctx->rt, sym->type));
		return noError;
	}
	if (ctx->proto == type_ref_size) {
		retRef(ctx, refSize(sym));
		return noError;
	}
	return nativeCallError;
}

static const char *const variant_as = "pointer as(variant var, typename type)";
static const char *const variant_get_type = "typename type(variant value)";
static const char *const variant_get_data = "pointer data(variant value)";
static vmError variantHelpers(nfcContext ctx) {
	if (ctx->proto == variant_get_type) {
		vmValue *variant = getArg(ctx, 0);
		retRef(ctx, variant->type);
		return noError;
	}
	if (ctx->proto == variant_get_data) {
		vmValue *variant = getArg(ctx, 0);
		retRef(ctx, variant->ref);
		return noError;
	}

	size_t symOffs = argRef(ctx, 0);
	vmValue *varOffs = getArg(ctx, vm_ref_size);

	if (ctx->proto == variant_as) {
		if (varOffs->type == symOffs) {
			retRef(ctx, varOffs->ref);
		} else {
			retRef(ctx, 0);
		}
		return noError;
	}
	return nativeCallError;
}

static const char *const object_create = "pointer create(typename type)";
static const char *const object_destroy = "void destroy(object this)";
static const char *const object_cast = "pointer as(object this, typename type)";
static const char *const object_type = "typename type(object this)";
static vmError objectHelpers(nfcContext ctx) {
	rtContext rt = ctx->rt;
	if (ctx->proto == object_create) {
		size_t offs = argRef(ctx, 0);
		symn sym = rtLookup(rt, offs, KIND_typ);
		if (sym == NULL || sym->offs != offs || !isObjectType(sym)) {
			// invalid type to allocate
			return nativeCallError;
		}
		void *obj = rt->api.alloc(ctx, NULL, sym->size);
		if (obj == NULL) {
			// allocation failed
			return nativeCallError;
		}
		// persist type of object
		*(vmOffs*)obj = vmOffset(rt, sym);
		retRef(ctx, vmOffset(rt, obj));
		return noError;
	}

	if (ctx->proto == object_destroy) {
		void *obj = vmPointer(rt, argRef(ctx, 0));
		rt->api.alloc(ctx, obj, 0);
		return noError;
	}

	if (ctx->proto == object_cast) {
		size_t offs = argRef(ctx, 0 * vm_ref_size);
		symn sym = rtLookup(rt, offs, KIND_typ);
		void *obj = vmPointer(rt, argRef(ctx, 1 * vm_ref_size));
		if (sym == NULL || sym->offs != offs || !isObjectType(sym)) {
			// invalid type to cast as
			return nativeCallError;
		}
		if (obj == NULL) {
			// null is null type does not matter
			retRef(ctx, vmOffset(rt, obj));
			return noError;
		}
		symn type = vmPointer(rt, *(vmOffs*)obj);
		while (type != NULL) {
			if (type == type->type) {
				obj = NULL;
				break;
			}
			if (sym == type) {
				break;
			}
			type = type->type;
		}
		retRef(ctx, vmOffset(rt, obj));
		return noError;
	}

	if (ctx->proto == object_type) {
		void *obj = vmPointer(rt, argRef(ctx, 0));
		if (obj == NULL) {
			retRef(ctx, 0);	// TODO: this should return object
			// invalid type to cast as
			return noError;
		}
		symn type = vmPointer(rt, *(vmOffs*)obj);
		retRef(ctx, vmOffset(rt, type));
		return noError;
	}

	return nativeCallError;
}

static void raiseStd(rtContext ctx, raiseLevel level, const char *file, int line, struct nfcArgArr details, const char *msg, ...) {
	va_list vaList;
	va_start(vaList, msg);
	print_log(ctx, level, file, line, details, msg, vaList);
	va_end(vaList);
}

static const char *const sys_raise = "void raise(char file[*], int32 line, int32 level, int32 trace, char message[*], variant details...)";
static vmError sysRaise(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	const char *file = rt->api.nextArg(&al)->ref;
	const int line = rt->api.nextArg(&al)->i32;
	const int logLevel = rt->api.nextArg(&al)->i32;
	const int maxTrace = rt->api.nextArg(&al)->i32;
	const char *message = rt->api.nextArg(&al)->ref;
	struct nfcArgArr details = rt->api.nextArg(&al)->arr;

	// logging is disabled or log level not reached.
	if (rt->logFile == NULL || logLevel > (int)rt->logLevel) {
		return noError;
	}

	if (logLevel == raiseFatal && isChecked(rt)) {
		// do not terminate execution, just show the error
		raiseStd(rt, raiseError, file, line, details, "%?s", message);
	} else {
		raiseStd(rt, logLevel, file, line, details, "%?s", message);
	}

	// print stack trace excluding this function
	if (rt->dbg != NULL && maxTrace > 0) {
		traceCalls(rt->dbg, rt->logFile, 1, maxTrace - 1, 1);
	}

	// abort the execution
	if (logLevel == raiseFatal) {
		return nativeCallError;
	}

	return noError;
}

// int tryExec(variant outError, pointer args, void action(pointer args));
const char *const sys_try_exec = "int32 tryExec(pointer args, void action(pointer args))";
static vmError sysTryExec(nfcContext ctx) {
	vmError result;
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	size_t args = argRef(ctx, rt->api.nextArg(&al)->offset);
	size_t actionOffs = argRef(ctx, rt->api.nextArg(&al)->offset);
	symn action = rt->api.lookup(rt, actionOffs, KIND_fun);

	if (action != NULL && action->offs == actionOffs) {
		#pragma pack(push, 4)
		struct { vmOffs ptr; } cbArg;
		cbArg.ptr = (vmOffs) args;
		#pragma pack(pop)
		result = rt->api.invoke(ctx, action, NULL, &cbArg, ctx->extra);
	}
	else {
		result = illegalState;
	}
	retI32(ctx, result);
	return noError;
}

/// Install instruction for the emit intrinsic
static inline symn ccDefOpCode(ccContext ctx, const char *name, symn type, vmOpcode code, size_t args) {
	astn opc = newNode(ctx, TOKEN_opc);
	if (opc != NULL) {
		opc->type = type;
		opc->opc.code = code;
		opc->opc.args = args;
		symn result = install(ctx, name, ATTR_stat | KIND_def, 0, type, opc);
		if (result == NULL) {
			return NULL;
		}
		result->doc = type_doc_builtin;
		return result;
	}
	return NULL;
}

/// Install type system.
static void install_type(ccContext ctx, ccConfig config) {
	symn type_ptr = NULL, type_var = NULL, type_obj = NULL;
	astn init_val = intNode(ctx, 0);

	symn type_rec = install(ctx, "typename", ATTR_stat | KIND_typ | CAST_ptr, vm_ref_size, NULL, NULL);
	// update required variables to be able to install other types.
	ctx->type_rec = type_rec->type = type_rec;   // TODO: !cycle: typename is instance of typename
	type_rec->size = sizeof(struct symNode);    // expose the real size of the internal representation.
	init_val->type = type_rec;	// HACK: the only type we have right now ...

	symn type_vid = install(ctx, "void", ATTR_stat | KIND_typ | CAST_vid, 0, type_rec, NULL);
	symn type_bol = install(ctx, "bool", ATTR_stat | KIND_typ | CAST_bit, 1, type_rec, init_val);
	symn type_chr = install(ctx, "char", ATTR_stat | KIND_typ | CAST_i32, 1, type_rec, init_val);
	symn type_i08 = install(ctx, "int8", ATTR_stat | KIND_typ | CAST_i32, 1, type_rec, init_val);
	symn type_i16 = install(ctx, "int16", ATTR_stat | KIND_typ | CAST_i32, 2, type_rec, init_val);
	symn type_i32 = install(ctx, "int32", ATTR_stat | KIND_typ | CAST_i32, 4, type_rec, init_val);
	symn type_i64 = install(ctx, "int64", ATTR_stat | KIND_typ | CAST_i64, 8, type_rec, init_val);
	symn type_u08 = install(ctx, "uint8", ATTR_stat | KIND_typ | CAST_u32, 1, type_rec, init_val);
	symn type_u16 = install(ctx, "uint16", ATTR_stat | KIND_typ | CAST_u32, 2, type_rec, init_val);
	symn type_u32 = install(ctx, "uint32", ATTR_stat | KIND_typ | CAST_u32, 4, type_rec, init_val);
	symn type_u64 = install(ctx, "uint64", ATTR_stat | KIND_typ | CAST_u64, 8, type_rec, init_val);
	symn type_f32 = install(ctx, "float32", ATTR_stat | KIND_typ | CAST_f32, 4, type_rec, init_val);
	symn type_f64 = install(ctx, "float64", ATTR_stat | KIND_typ | CAST_f64, 8, type_rec, init_val);
	init_val->type = type_i64;

	if (config & install_ptr) {
		type_ptr = install(ctx, "pointer", ATTR_stat | ATTR_opt | KIND_typ | CAST_ptr, 1 * vm_ref_size, type_rec, NULL);
		type_ptr->doc = type_doc_builtin;
	}
	if (config & install_var) {
		type_var = install(ctx, "variant", ATTR_stat | KIND_typ | CAST_var, 2 * vm_ref_size, type_rec, NULL);
		type_var->doc = type_doc_builtin;
	}
	if (config & install_obj) {
		type_obj = install(ctx, "object", ATTR_stat | KIND_typ | CAST_obj, 1 * vm_ref_size, type_rec, NULL);
		type_obj->doc = type_doc_builtin;
	}
	symn type_fun = install(ctx, "function", ATTR_stat | KIND_typ | CAST_ptr, 1 * vm_ref_size, type_rec, NULL);

	type_vid->fmt = NULL;
	type_vid->doc = type_doc_builtin;
	type_bol->fmt = type_fmt_signed32;
	type_bol->doc = type_doc_builtin;
	type_chr->fmt = type_fmt_character;
	type_chr->doc = type_doc_builtin;
	type_i08->fmt = type_fmt_signed32;
	type_i08->doc = type_doc_builtin;
	type_i16->fmt = type_fmt_signed32;
	type_i16->doc = type_doc_builtin;
	type_i32->fmt = type_fmt_signed32;
	type_i32->doc = type_doc_builtin;
	type_i64->fmt = type_fmt_signed64;
	type_i64->doc = type_doc_builtin;
	type_u08->fmt = type_fmt_unsigned32;
	type_u08->doc = type_doc_builtin;
	type_u16->fmt = type_fmt_unsigned32;
	type_u16->doc = type_doc_builtin;
	type_u32->fmt = type_fmt_unsigned32;
	type_u32->doc = type_doc_builtin;
	type_u64->fmt = type_fmt_unsigned64;
	type_u64->doc = type_doc_builtin;
	type_f32->fmt = type_fmt_float32;
	type_f32->doc = type_doc_builtin;
	type_f64->fmt = type_fmt_float64;
	type_f64->doc = type_doc_builtin;
	type_rec->fmt = type_fmt_typename;
	type_rec->doc = type_doc_builtin;
	type_fun->doc = type_doc_builtin;

	ctx->type_vid = type_vid;
	ctx->type_bol = type_bol;
	ctx->type_chr = type_chr;
	ctx->type_i08 = type_i08;
	ctx->type_i16 = type_i16;
	ctx->type_i32 = type_i32;
	ctx->type_i64 = type_i64;
	ctx->type_u08 = type_u08;
	ctx->type_u16 = type_u16;
	ctx->type_u32 = type_u32;
	ctx->type_u64 = type_u64;
	ctx->type_f32 = type_f32;
	ctx->type_f64 = type_f64;
	ctx->type_ptr = type_ptr;
	ctx->type_fun = type_fun;
	ctx->type_var = type_var;
	ctx->type_obj = type_obj;
	if (vm_ref_size == 8) {
		ctx->type_int = type_i64;
	} else {
		ctx->type_int = type_i32;
	}

	if (type_ptr != NULL) {
		ctx->null_ref = install(ctx, "null", ATTR_stat | ATTR_opt | KIND_def | CAST_ptr, 0, type_ptr, intNode(ctx, 0));
		ctx->null_ref->init->type = type_ptr;
		type_ptr->doc = type_doc_builtin;
		ctx->null_ref->doc = type_doc_builtin;
	}

	// cache the `length` of slices
	enter(ctx, NULL, NULL);
	ctx->length_ref = install(ctx, "length", KIND_var, ctx->type_int->size, ctx->type_int, NULL);
	leave(ctx, KIND_typ, 0, 0, NULL, NULL);
	ctx->length_ref->offs = offsetOf(vmValue, length);
	ctx->length_ref->doc = type_doc_builtin;
	ctx->length_ref->next = NULL;

	// aliases
	install(ctx, "int", ATTR_stat | KIND_def, 0, type_rec, lnkNode(ctx, ctx->type_int));

	ctx->type_str = install(ctx, ".cstr", ATTR_stat | KIND_typ | CAST_arr, vm_ref_size, type_chr, NULL);
	if (ctx->type_str != NULL) {
		// arrays without length property are c-like pointers
		ctx->type_str->fmt = type_fmt_string;
	}
	if (ctx->type_vid != NULL) {
		ctx->void_tag = lnkNode(ctx, ctx->type_vid);
		if (ctx->void_tag == NULL) {
			error(ctx->rt, NULL, 0, ERR_INTERNAL_ERROR);
		}
	}

	// undefined initializer: {...}
	ctx->init_und = newNode(ctx, STMT_beg);
	dieif(ctx->init_und == NULL, ERR_INTERNAL_ERROR);
	ctx->init_und->type = type_vid;
}

/// Install emit intrinsic.
static void install_emit(ccContext ctx, ccConfig config) {
	symn emit = install(ctx, "inline", ATTR_stat | KIND_typ | CAST_vid, 0, ctx->type_fun, NULL);
	if (emit == NULL) {
		// error already reported
		return;
	}

	emit->doc = type_doc_builtin;
	if ((config & installEopc) == installEopc && ccExtend(ctx, emit)) {
		symn type_vid = ctx->type_vid;
		symn type_bol = ctx->type_bol;

		symn type_i32 = install(ctx, "i32", ATTR_stat | KIND_typ | CAST_i32, 4, ctx->type_rec, NULL);
		symn type_i64 = install(ctx, "i64", ATTR_stat | KIND_typ | CAST_i64, 8, ctx->type_rec, NULL);
		symn type_u32 = install(ctx, "u32", ATTR_stat | KIND_typ | CAST_u32, 4, ctx->type_rec, NULL);
		symn type_u64 = install(ctx, "u64", ATTR_stat | KIND_typ | CAST_u64, 8, ctx->type_rec, NULL);
		symn type_f32 = install(ctx, "f32", ATTR_stat | KIND_typ | CAST_f32, 4, ctx->type_rec, NULL);
		symn type_f64 = install(ctx, "f64", ATTR_stat | KIND_typ | CAST_f64, 8, ctx->type_rec, NULL);
		symn type_p128 = install(ctx, "p128", ATTR_stat | KIND_typ | CAST_val, 16, ctx->type_rec, NULL);

		dieif(type_i32 == NULL, ERR_INTERNAL_ERROR);
		dieif(type_i64 == NULL, ERR_INTERNAL_ERROR);
		dieif(type_u32 == NULL, ERR_INTERNAL_ERROR);
		dieif(type_u64 == NULL, ERR_INTERNAL_ERROR);
		dieif(type_f32 == NULL, ERR_INTERNAL_ERROR);
		dieif(type_f64 == NULL, ERR_INTERNAL_ERROR);
		dieif(type_p128 == NULL, ERR_INTERNAL_ERROR);

		ccDefOpCode(ctx, "not", type_bol, opc_not, 0);
		ccDefOpCode(ctx, "ret", type_vid, opc_jmpi, 0);
		ccDefOpCode(ctx, "call", type_vid, opc_call, 0);

		ccDefOpCode(ctx, "dup1", type_i32, opc_dup1, 0);
		ccDefOpCode(ctx, "dup2", type_i64, opc_dup2, 0);
		ccDefOpCode(ctx, "dup4", type_p128, opc_dup4, 0);
		ccDefOpCode(ctx, "set1", type_vid, opc_set1, 1);
		ccDefOpCode(ctx, "set2", type_vid, opc_set2, 2);
		ccDefOpCode(ctx, "set4", type_vid, opc_set4, 4);
		ccDefOpCode(ctx, "set1_2", type_vid, opc_set1, 2);
		ccDefOpCode(ctx, "set1_3", type_vid, opc_set1, 3);
		ccDefOpCode(ctx, "set1_4", type_vid, opc_set1, 4);
		ccDefOpCode(ctx, "set1_5", type_vid, opc_set1, 5);

		// load memory indirect
		ccDefOpCode(ctx, "ldi_s8", type_i32, opc_ldis1, 0);
		ccDefOpCode(ctx, "ldi_u8", type_i32, opc_ldiu1, 0);
		ccDefOpCode(ctx, "ldi_s16", type_i32, opc_ldis2, 0);
		ccDefOpCode(ctx, "ldi_u16", type_i32, opc_ldiu2, 0);
		ccDefOpCode(ctx, "ldi32", type_i32, opc_ldi4, 0);
		ccDefOpCode(ctx, "ldi64", type_i64, opc_ldi8, 0);
		ccDefOpCode(ctx, "ldi128", type_p128, opc_ldiq, 0);

		// store memory indirect
		ccDefOpCode(ctx, "sti8", type_vid, opc_sti1, 0);
		ccDefOpCode(ctx, "sti16", type_vid, opc_sti2, 0);
		ccDefOpCode(ctx, "sti32", type_vid, opc_sti4, 0);
		ccDefOpCode(ctx, "sti64", type_vid, opc_sti8, 0);
		ccDefOpCode(ctx, "sti128", type_vid, opc_stiq, 0);

		if (ccExtend(ctx, type_u32) != NULL) {
			type_u32->doc = type_doc_builtin;
			ccDefOpCode(ctx, "cmt", type_u32, u32_cmt, 0);
			ccDefOpCode(ctx, "and", type_u32, u32_and, 0);
			ccDefOpCode(ctx, "xor", type_u32, u32_xor, 0);
			ccDefOpCode(ctx, "or", type_u32, u32_ior, 0);
			ccDefOpCode(ctx, "mul", type_u32, u32_mul, 0);
			ccDefOpCode(ctx, "div", type_u32, u32_div, 0);
			ccDefOpCode(ctx, "rem", type_u32, u32_rem, 0);

			ccDefOpCode(ctx, "clt", type_bol, u32_clt, 0);
			ccDefOpCode(ctx, "cgt", type_bol, u32_cgt, 0);

			ccDefOpCode(ctx, "shl", type_u32, u32_shl, 0);
			ccDefOpCode(ctx, "shr", type_u32, u32_shr, 0);
			ccDefOpCode(ctx, "sar", type_u32, u32_sar, 0);

			ccDefOpCode(ctx, "i64", type_i64, u32_i64, 0);
			ccDefOpCode(ctx, "swap", type_u32, p4x_swz, 1 | 0 << 2 | 2 << 4 | 3 << 6); // todo: requires 4 elements on stack
			ccEnd(ctx, type_u32);
		}

		if (ccExtend(ctx, type_u64) != NULL) {
			type_u64->doc = type_doc_builtin;
			ccDefOpCode(ctx, "cmt", type_u64, u64_cmt, 0);
			ccDefOpCode(ctx, "and", type_u64, u64_and, 0);
			ccDefOpCode(ctx, "xor", type_u64, u64_xor, 0);
			ccDefOpCode(ctx, "or", type_u64, u64_ior, 0);
			ccDefOpCode(ctx, "mul", type_u64, u64_mul, 0);
			ccDefOpCode(ctx, "div", type_u64, u64_div, 0);
			ccDefOpCode(ctx, "rem", type_u64, u64_rem, 0);

			ccDefOpCode(ctx, "clt", type_bol, u64_clt, 0);
			ccDefOpCode(ctx, "cgt", type_bol, u64_cgt, 0);

			ccDefOpCode(ctx, "shl", type_u64, u64_shl, 0);
			ccDefOpCode(ctx, "shr", type_u64, u64_shr, 0);
			ccDefOpCode(ctx, "sar", type_u64, u64_sar, 0);

			ccDefOpCode(ctx, "i32", type_i32, i64_i32, 0); // keep low part, not sign related
			ccDefOpCode(ctx, "u32", type_u32, i64_i32, 0); // keep low part, not sign related
			ccDefOpCode(ctx, "swap", type_u64, p4x_swz, 2 | 3 << 2 | 0 << 4 | 1 << 6);
			ccEnd(ctx, type_u64);
		}

		if (ccExtend(ctx, type_i32) != NULL) {
			type_i32->doc = type_doc_builtin;
			ccDefOpCode(ctx, "neg", type_i32, i32_neg, 0);
			ccDefOpCode(ctx, "add", type_i32, i32_add, 0);
			ccDefOpCode(ctx, "sub", type_i32, i32_sub, 0);
			ccDefOpCode(ctx, "mul", type_i32, i32_mul, 0);
			ccDefOpCode(ctx, "div", type_i32, i32_div, 0);
			ccDefOpCode(ctx, "rem", type_i32, i32_rem, 0);

			ccDefOpCode(ctx, "ceq", type_bol, i32_ceq, 0);
			ccDefOpCode(ctx, "clt", type_bol, i32_clt, 0);
			ccDefOpCode(ctx, "cgt", type_bol, i32_cgt, 0);

			ccDefOpCode(ctx, "bool", type_i32, i32_bol, 0);
			ccDefOpCode(ctx, "i64", type_i64, i32_i64, 0);
			ccDefOpCode(ctx, "f32", type_f32, i32_f32, 0);
			ccDefOpCode(ctx, "f64", type_f64, i32_f64, 0);
			ccEnd(ctx, type_i32);
		}

		if (ccExtend(ctx, type_i64) != NULL) {
			type_i64->doc = type_doc_builtin;
			ccDefOpCode(ctx, "neg", type_i64, i64_neg, 0);
			ccDefOpCode(ctx, "add", type_i64, i64_add, 0);
			ccDefOpCode(ctx, "sub", type_i64, i64_sub, 0);
			ccDefOpCode(ctx, "mul", type_i64, i64_mul, 0);
			ccDefOpCode(ctx, "div", type_i64, i64_div, 0);
			ccDefOpCode(ctx, "rem", type_i64, i64_rem, 0);

			ccDefOpCode(ctx, "ceq", type_bol, i64_ceq, 0);
			ccDefOpCode(ctx, "clt", type_bol, i64_clt, 0);
			ccDefOpCode(ctx, "cgt", type_bol, i64_cgt, 0);

			ccDefOpCode(ctx, "i32", type_i32, i64_i32, 0);
			ccDefOpCode(ctx, "bool", type_i64, i64_bol, 0);
			ccDefOpCode(ctx, "f32", type_f32, i64_f32, 0);
			ccDefOpCode(ctx, "f64", type_f64, i64_f64, 0);
			ccDefOpCode(ctx, "u32", type_vid, i64_i32, 0); // keep low part, not sign related
			ccEnd(ctx, type_i64);
		}

		if (ccExtend(ctx, type_f32) != NULL) {
			type_f32->doc = type_doc_builtin;
			ccDefOpCode(ctx, "neg", type_f32, f32_neg, 0);
			ccDefOpCode(ctx, "add", type_f32, f32_add, 0);
			ccDefOpCode(ctx, "sub", type_f32, f32_sub, 0);
			ccDefOpCode(ctx, "mul", type_f32, f32_mul, 0);
			ccDefOpCode(ctx, "div", type_f32, f32_div, 0);
			ccDefOpCode(ctx, "rem", type_f32, f32_rem, 0);

			ccDefOpCode(ctx, "ceq", type_bol, f32_ceq, 0);
			ccDefOpCode(ctx, "clt", type_bol, f32_clt, 0);
			ccDefOpCode(ctx, "cgt", type_bol, f32_cgt, 0);

			ccDefOpCode(ctx, "i32", type_i32, f32_i32, 0);
			ccDefOpCode(ctx, "i64", type_i64, f32_i64, 0);
			ccDefOpCode(ctx, "bool", type_bol, f32_bol, 0);
			ccDefOpCode(ctx, "f64", type_f64, f32_f64, 0);
			ccEnd(ctx, type_f32);
		}

		if (ccExtend(ctx, type_f64) != NULL) {
			type_f64->doc = type_doc_builtin;
			ccDefOpCode(ctx, "neg", type_f64, f64_neg, 0);
			ccDefOpCode(ctx, "add", type_f64, f64_add, 0);
			ccDefOpCode(ctx, "sub", type_f64, f64_sub, 0);
			ccDefOpCode(ctx, "mul", type_f64, f64_mul, 0);
			ccDefOpCode(ctx, "div", type_f64, f64_div, 0);
			ccDefOpCode(ctx, "rem", type_f64, f64_rem, 0);

			ccDefOpCode(ctx, "ceq", type_bol, f64_ceq, 0);
			ccDefOpCode(ctx, "clt", type_bol, f64_clt, 0);
			ccDefOpCode(ctx, "cgt", type_bol, f64_cgt, 0);

			ccDefOpCode(ctx, "i32", type_i32, f64_i32, 0);
			ccDefOpCode(ctx, "i64", type_i64, f64_i64, 0);
			ccDefOpCode(ctx, "f32", type_bol, f64_f32, 0);
			ccDefOpCode(ctx, "bool", type_f64, f64_bol, 0);
			ccEnd(ctx, type_f64);
		}

		if (ccExtend(ctx, type_p128) != NULL) {
			type_p128->doc = type_doc_builtin;
			ccDefOpCode(ctx, "neg4f", type_p128, v4f_neg, 0);
			ccDefOpCode(ctx, "add4f", type_p128, v4f_add, 0);
			ccDefOpCode(ctx, "sub4f", type_p128, v4f_sub, 0);
			ccDefOpCode(ctx, "mul4f", type_p128, v4f_mul, 0);
			ccDefOpCode(ctx, "div4f", type_p128, v4f_div, 0);

			ccDefOpCode(ctx, "min4f", type_p128, v4f_min, 0);
			ccDefOpCode(ctx, "max4f", type_p128, v4f_max, 0);

			ccDefOpCode(ctx, "neg2d", type_p128, v2d_neg, 0);
			ccDefOpCode(ctx, "add2d", type_p128, v2d_add, 0);
			ccDefOpCode(ctx, "sub2d", type_p128, v2d_sub, 0);
			ccDefOpCode(ctx, "mul2d", type_p128, v2d_mul, 0);
			ccDefOpCode(ctx, "div2d", type_p128, v2d_div, 0);

			ccDefOpCode(ctx, "min2d", type_p128, v2d_min, 0);
			ccDefOpCode(ctx, "max2d", type_p128, v2d_max, 0);

			ccDefOpCode(ctx, "ceq", type_bol, v4f_ceq, 0);
			ccDefOpCode(ctx, "dp3", type_f32, v4f_dp3, 0);
			ccDefOpCode(ctx, "dp4", type_f32, v4f_dp4, 0);
			ccDefOpCode(ctx, "dph", type_f32, v4f_dph, 0);

			if ((config & installEswz) == installEswz) {
				rtContext rt = ctx->rt;
				for (size_t i = 0; i < 256; i += 1) {
					if (rt->_end - rt->_beg < 5) {
						fatal(ERR_MEMORY_OVERRUN);
						return;
					}
					rt->_beg[0] = (unsigned char) "xyzw"[(i >> 0) & 3];
					rt->_beg[1] = (unsigned char) "xyzw"[(i >> 2) & 3];
					rt->_beg[2] = (unsigned char) "xyzw"[(i >> 4) & 3];
					rt->_beg[3] = (unsigned char) "xyzw"[(i >> 6) & 3];
					rt->_beg[4] = 0;
					const char *name = ccUniqueStr(ctx, (char *) rt->_beg, -1, -1);
					ccDefOpCode(ctx, name, type_p128, p4x_swz, i);
				}
			}
			ccEnd(ctx, type_p128);
		}

		symn opc;
		if ((opc = ccBegin(ctx, "cvt")) != NULL) {
			// todo: remove unsafe duplicates
			ccDefOpCode(ctx, "f32_u32", type_vid, f32_i32, 0);
			ccDefOpCode(ctx, "f32_u64", type_vid, f32_i64, 0);
			ccDefOpCode(ctx, "f64_u32", type_vid, f64_i32, 0);
			ccDefOpCode(ctx, "f64_u64", type_vid, f64_i64, 0);
			ccDefOpCode(ctx, "u32_f32", type_vid, i32_f32, 0);
			ccDefOpCode(ctx, "u32_f64", type_vid, i32_f64, 0);
			ccDefOpCode(ctx, "u64_f32", type_vid, i64_f32, 0);
			ccDefOpCode(ctx, "u64_f64", type_vid, i64_f64, 0);
			ccEnd(ctx, opc);
		}
		ccEnd(ctx, emit);
		ctx->emit_opc = emit;
	}
}

/**
 * Install base functions.
 *
 * @param ctx Runtime context.
 * @param onHalt the function to be invoked when execution stops.
 * @returns 0 on success
 */
static int install_base(ccContext ctx, ccConfig config, vmError onHalt(nfcContext)) {
	//! halt must be the first native call.
	int error = !ccAddCall(ctx, onHalt, "void halt()");

	// tryExecute
	if (!error && ctx->type_ptr != NULL) {
		ctx->libc_try = ccAddCall(ctx, sysTryExec, sys_try_exec);
		if (ctx->libc_try == NULL) {
			error = 2;
		}
	}
	// raise(fatal, error, warn, info, debug, trace)
	if (ctx->type_var != NULL) {
		ctx->libc_dbg = ccAddCall(ctx, sysRaise, sys_raise);
		if (ctx->libc_dbg == NULL) {
			error = 2;
		}
		if (!error && ccExtend(ctx, ctx->libc_dbg)) {
			ctx->libc_dbg->doc = "Report messages or raise errors.";
			error = error || !ccDefInt(ctx, "abort", raiseFatal);
			error = error || !ccDefInt(ctx, "error", raiseError);
			error = error || !ccDefInt(ctx, "warn", raiseWarn);
			error = error || !ccDefInt(ctx, "info", raiseInfo);
			error = error || !ccDefInt(ctx, "debug", raiseDebug);
			error = error || !ccDefInt(ctx, "verbose", raiseVerbose);
			// trace levels
			error = error || !ccDefInt(ctx, "noTrace", 0);
			error = error || !ccDefInt(ctx, "defTrace", 128);
			ccEnd(ctx, ctx->libc_dbg);
		}
	}

	// 4 reflection
	if (ctx->type_rec != NULL && ctx->type_var != NULL) {
		enter(ctx, NULL, ctx->type_var);

		if ((config & installLibs) != 0) {
			error = error || !ccAddCall(ctx, variantHelpers, variant_as);
			error = error || !ccAddCall(ctx, variantHelpers, variant_get_type);
			error = error || !ccAddCall(ctx, variantHelpers, variant_get_data);
		}

		dieif(ctx->type_var->fields != NULL, ERR_INTERNAL_ERROR);
		ctx->type_var->fields = leave(ctx, KIND_def, 0, 0, NULL, ctx->type_var->fields);


		enter(ctx, NULL, ctx->type_rec);

		if ((config & installLibs) != 0) {
			symn field;
			if ((field = install(ctx, "size", KIND_var, vm_stk_align, ctx->type_int, NULL))) {
				field->offs = offsetOf(struct symNode, size);
				field->doc = type_doc_builtin;
			} else {
				error = 1;
			}
			if ((field = install(ctx, "offset", KIND_var, vm_stk_align, ctx->type_int, NULL))) {
				field->offs = offsetOf(struct symNode, offs);
				field->doc = type_doc_builtin;
				field->fmt = "@%06x";
			} else {
				error = 1;
			}

			error = error || !ccAddCall(ctx, typenameReflect, type_get_base);
			error = error || !(field = ccAddCall(ctx, typenameReflect, type_get_file));
			if (field != NULL && field->params != NULL) {
				// hack: change return type from pointer to string
				field->params->type = ctx->type_str;
			}
			error = error || !ccAddCall(ctx, typenameReflect, type_get_line);
			error = error || !(field = ccAddCall(ctx, typenameReflect, type_get_name));
			if (field != NULL && field->params != NULL) {
				// hack: change return type from pointer to string
				field->params->type = ctx->type_str;
			}
			error = error || !ccAddCall(ctx, typenameReflect, type_ref_size);
			error = error || !ccAddCall(ctx, typenameReflect, type_get_field);
			error = error || !ccAddCall(ctx, typenameReflect, type_get_fields);
			error = error || !ccAddCall(ctx, typenameReflect, type_get_lookup);
			error = error || !ccAddCall(ctx, typenameReflect, type_get_is_ind);
			error = error || !ccAddCall(ctx, typenameReflect, type_get_is_opt);
			error = error || !ccAddCall(ctx, typenameReflect, type_get_is_mut);
			error = error || !ccAddCall(ctx, typenameReflect, type_get_next);

			/* TODO: more 4 reflection
			error = error || !ccAddCall(rt, typenameReflect, "variant lookup(variant &obj, int options, string name, variant args...)");
			error = error || !ccAddCall(rt, typenameReflect, "variant invoke(variant &obj, int options, string name, variant args...)");
			// setValue and getValue can be done with lookup and invoke
			error = error || !ccAddCall(rt, typenameReflect, "variant setValue(typename field, variant value)");
			error = error || !ccAddCall(rt, typenameReflect, "variant getValue(typename field)");

			error = error || !ccAddCall(rt, typenameReflect, "bool canAssign(typename type, variant value, bool strict)");
			error = error || !ccAddCall(rt, typenameReflect, "bool instanceOf(typename type, variant obj)");
			//~ */
		}

		dieif(ctx->type_rec->fields != NULL, ERR_INTERNAL_ERROR);
		ctx->type_rec->fields = leave(ctx, KIND_def, 0, 0, NULL, ctx->type_rec->fields);
	}

	if (ctx->type_obj != NULL && ctx->type_rec != NULL) {
		enter(ctx, NULL, ctx->type_obj);

		symn field;
		if ((field = install(ctx, ".type", KIND_var | CAST_ptr, vm_ref_size, ctx->type_rec, NULL))) {
			field->offs = 0;
		} else {
			error = 1;
		}

		if ((config & installLibs) != 0) {
			error = error || !(ctx->libc_new = ccAddCall(ctx, objectHelpers, object_create));
			error = error || !ccAddCall(ctx, objectHelpers, object_destroy);
			error = error || !ccAddCall(ctx, objectHelpers, object_cast);
			error = error || !ccAddCall(ctx, objectHelpers, object_type);
		}

		dieif(ctx->type_obj->fields != NULL, ERR_INTERNAL_ERROR);
		ctx->type_obj->fields = leave(ctx, KIND_def, 0, 0, NULL, ctx->type_obj->fields);
	}

	return error;
}

ccContext ccInit(rtContext ctx, ccConfig config, vmError onHalt(nfcContext)) {
	dieif(ctx->cc != NULL, ERR_INTERNAL_ERROR);
	dieif(ctx->_beg != ctx->_mem + 1, ERR_INTERNAL_ERROR);
	dieif(ctx->_end != ctx->_mem + ctx->_size, ERR_INTERNAL_ERROR);

	ctx->_end -= sizeof(struct ccContextRec);
	if (ctx->_end < ctx->_beg) {
		fatal(ERR_MEMORY_OVERRUN);
		return NULL;
	}

	ccContext cc = (ccContext) ctx->_end;
	memset(cc, 0, sizeof(struct ccContextRec));
	initBuff(&cc->jumps, 1, sizeof(struct jumpToFix));

	cc->rt = ctx;
	ctx->cc = cc;

	cc->genDocumentation = 0;
	cc->genStaticGlobals = 1;
	cc->errPrivateAccess = 1;
	cc->errUninitialized = 1;

	install_type(cc, config);
	install_emit(cc, config);
	install_base(cc, config, onHalt);

	cc->root = newNode(cc, STMT_beg);
	cc->root->type = cc->type_vid;

	return cc;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Todo

symn rtLookup(rtContext ctx, size_t offset, ccKind filter) {
	dieif(offset > ctx->_size, ERR_INVALID_OFFSET, offset);
	if (offset >= ctx->vm.px + px_size) {
		// local variable on stack ?
		return NULL;
	}
	symn sym = ctx->main;
	if (offset >= sym->offs && offset < sym->offs + sym->size) {
		// is the main function ?
		return sym;
	}
	ccKind filterStat = filter & ATTR_stat;
	ccKind filterKind = filter & MASK_kind;
	ccKind filterCast = filter & MASK_cast;
	for (sym = sym->fields; sym; sym = sym->global) {
		if (filterStat && (sym->kind & ATTR_stat) != filterStat) {
			continue;
		}
		ccKind symKind = sym->kind & MASK_kind;
		if (filterKind && symKind != filterKind) {
			continue;
		}
		if (filterCast && (sym->kind & MASK_cast) != filterCast) {
			continue;
		}
		if (offset == sym->offs) {
			return sym;
		}
		if (symKind == KIND_def) {
			// skip inline symbols
			continue;
		}

		size_t size;
		if (symKind == KIND_typ) {
			// size of a type is typename->size
			size = sym->type->size;
		} else {
			size = refSize(sym);
		}
		if (offset > sym->offs && offset < sym->offs + size) {
			return sym;
		}
	}
	return NULL;
}

const char *ccUniqueStr(ccContext ctx, const char *str, size_t len, unsigned hash) {
	if (str == NULL) {
		return NULL;
	}

	if (len == (size_t)-1) {
		len = strlen(str) + 1;
	}
	else if (str[len - 1] != 0) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	if (hash == (unsigned)-1) {
		hash = rehash(str, len);
	}

	list prev = NULL;
	hash %= lengthOf(ctx->stringTable);
	list next = ctx->stringTable[hash];
	while (next != NULL) {
		int cmp = strncmp(next->data, str, len);
		if (cmp == 0) {
			return next->data;
		}
		if (cmp > 0) {
			break;
		}
		prev = next;
		next = next->next;
	}

	rtContext rt = ctx->rt;
	if (rt->_beg >= rt->_end - (sizeof(struct list) + len)) {
		fatal(ERR_MEMORY_OVERRUN);
		return NULL;
	}
	if (str != (char *)rt->_beg) {
		// copy data from constants ?
		memcpy(rt->_beg, str, len + 1);
		str = (char*)rt->_beg;
	}

	// allocate list node as temp data (end of memory)
	rt->_end -= sizeof(struct list);
	list node = (list)rt->_end;

	rt->_beg += len;

	if (!prev) {
		ctx->stringTable[hash] = node;
	}
	else {
		prev->next = node;
	}

	node->next = next;
	node->data = str;
	return str;
}
