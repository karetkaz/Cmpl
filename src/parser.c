/**
 * convert token stream to an abstract syntax tree.
 */
#include <limits.h>
#include "tree.h"
#include "type.h"
#include "lexer.h"
#include "printer.h"
#include "compiler.h"
#include "util.h"
#include "code.h"

static const char *unknown_tag = "<?>";

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Utils
static inline int isMember(symn sym) {
	if (sym->owner == NULL) {
		// global variable
		return 0;
	}
	return isTypename(sym->owner);
}
static inline void addHead(astn list, astn node) {
	dieif(list == NULL, ERR_INTERNAL_ERROR);
	dieif(list->kind != STMT_beg, ERR_INTERNAL_ERROR);

	node->next = list->lst.head;
	list->lst.head = node;
	if (list->lst.tail == NULL) {
		list->lst.tail = node;
	}
}
static inline void addTail(astn list, astn node) {
	dieif(list == NULL, ERR_INTERNAL_ERROR);
	dieif(list->kind != STMT_beg, ERR_INTERNAL_ERROR);

	if (list->lst.tail != NULL) {
		list->lst.tail->next = node;
	}
	else {
		list->lst.head = node;
	}
	list->lst.tail = node;
}
/// Add length property to arrays.
static inline void addLength(ccContext ctx, symn sym, astn init) {
	dieif(sym->fields != NULL, ERR_INTERNAL_ERROR);

	if (init == NULL) {
		// add dynamic length field to slices
		sym->fields = ctx->length_ref;
		return;
	}

	// add static length to array
	enter(ctx, NULL, NULL);
	ccKind kind = ATTR_stat | KIND_def | refCast(ctx->type_int);
	symn length = install(ctx, "length", kind, ctx->type_int->size, ctx->type_int, init);
	if (length != NULL) {
		length->doc = type_doc_builtin;
	}
	sym->fields = leave(ctx, KIND_def, 0, 0, NULL, NULL);
}

static inline symn tagType(ccContext ctx, astn tag) {
	symn tagType = ctx->symbolStack[tag->id.hash % lengthOf(ctx->symbolStack)];
	tagType = lookup(ctx, tagType, tag, NULL, 0, 0);
	if (tagType != NULL && isTypename(tagType)) {
		return tagType;
	}
	return NULL;
}

astn tagNode(ccContext ctx, const char *name) {
	astn ast = NULL;
	if (ctx != NULL && name != NULL) {
		ast = newNode(ctx, TOKEN_var);
		if (ast != NULL) {
			size_t len = strlen(name);
			ast->file = ctx->file;
			ast->line = ctx->line;
			ast->type = NULL;
			ast->id.link = NULL;
			ast->id.hash = rehash(name, len + 1);
			ast->id.name = ccUniqueStr(ctx, name, len + 1, ast->id.hash);
		}
	}
	return ast;
}

/**
 * Inline the content of a file relative to the path of the file inlines it.
 * @param ctx compiler context.
 * @param tag ast node containing which file to inline
 */
static int ccInline(ccContext ctx, astn tag) {
	if (tag->type != ctx->type_str) {
		return 0;
	}

	char buff[PATH_MAX] = {0};
	if (strncmp(tag->id.name, "./", 2) == 0 || strncmp(tag->id.name, "../", 3) == 0) {
		// relative path, use the current folder as search folder
		if (absolutePath(tag->file, buff, sizeof(buff)) == NULL) {
			fatal(ERR_INTERNAL_ERROR);
		}
		// replace the file name
		char *path = strrchr(buff, '/');
		if (path == NULL) {
			// TODO: replace '\' with '/' on windows before this
			path = strrchr(buff, '\\');
		}
		if (path != NULL) {
			*(path += 1) = 0;
		}
	}
	else if (strncmp(tag->id.name, "/", 1) != 0) {
		// not absolute path, use the $CMPL_HOME folder as search folder
		if (absolutePath(ctx->home, buff, sizeof(buff)) == NULL) {
			return -1;
		}

		size_t len = strlen(buff) - 1;
		if (buff[len] != '/') {
			strncat(buff, "/", sizeof(buff) - len);
		}
	}

	// convert windows path names to linux path names
	for (char *ptr = buff; *ptr; ++ptr) {
		if (*ptr == '\\') {
			*ptr = '/';
		}
	}

	strncat(buff, tag->id.name, sizeof(buff) - strlen(buff) - 1);
	if (absolutePath(buff, buff, sizeof(buff)) == NULL) {
		return -1;
	}
	char *path = relativeToCWD(buff);
	printLog(ctx->rt, raiseDebug, tag->file, tag->line, WARN_INLINE_FILE, path);
	return ccParse(ctx, path, 1, NULL);
}

/**
 * Install a new symbol: alias, type, variable or function.
 *
 * @param kind Kind of symbol: (KIND_def, KIND_var, KIND_typ, CAST_arr)
 * @param tag Parsed tree node representing the symbol.
 * @param type Type of symbol.
 * @return The symbol.
 */
static symn declare(ccContext ctx, ccKind kind, astn tag, symn type, symn params, const char *doc) {
	if (tag == NULL || tag->kind != TOKEN_var) {
		fatal(ERR_INTERNAL_ERROR": identifier expected, not: %t", tag);
		return NULL;
	}

	size_t size = type->size;
	switch (kind & MASK_kind) {
		default: break;
		case KIND_def:
		case KIND_typ:
		case KIND_fun:
			// declaring a new type or alias, start with size 0
			// struct Complex { ... }
			size = 0;
			break;

		case KIND_var:
			switch (kind & MASK_cast) {
				default: break;
				case CAST_ptr:
				case CAST_obj:
					size = vm_ref_size;
			}
			break;
	}
	symn def = install(ctx, tag->id.name, kind, size, type, NULL);

	if (def != NULL) {
		tag->id.link = def;
		tag->type = type;

		def->unit = ctx->unit;
		def->file = tag->file;
		def->line = tag->line;
		def->params = params;
		def->use = tag;
		def->tag = tag;
		def->doc = doc;

		addUsage(def, tag);

		for (symn ptr = def->next; ptr; ptr = ptr->next) {
			if (ptr->name == NULL) {
				continue;
			}

			// generated variable
			if (ptr->name[0] == '.') {
				continue;
			}

			if (def->tag == ptr->tag && def->params == ptr->params) {
				// same definition, static virtual method
				continue;
			}

			if (strcmp(def->name, ptr->name) != 0) {
				continue;
			}

			if ((params == NULL || ptr->params == NULL) && params != ptr->params) {
				// allow function and variable name to be the same
				continue;
			}

			if (params != NULL && ptr->params != NULL && !canAssign(ctx, ptr, tag, 0)) {
				// allow function overriding
				continue;
			}

			// test if override is possible
			if (ptr->owner != NULL && def->owner != NULL && isObjectType(def->owner)) {
				if (isFunction(def) && !isStatic(def) && isVariable(ptr) && isInvokable(ptr)) {
					debug(ctx->rt, def->file, def->line, WARN_FUNCTION_OVERRIDE, ptr);
					if (doc == NULL && ptr->doc != NULL) {
						def->doc = ptr->doc;
					}
					def->override = ptr;
					break;
				}
			}

			if (kind == (ATTR_stat|KIND_typ|CAST_vid)) {
				debug(ctx->rt, def->file, def->line, WARN_EXTENDING_NAMESPACE, ptr);
				def->fields = ptr->fields;
				break;
			}

			// test if overwrite is possible (forward function implementation)
			if (ptr->init == NULL && ptr->owner == def->owner && def->nest == ptr->nest) {
				if (isFunction(def) && isVariable(ptr) && isInvokable(ptr)) {
					debug(ctx->rt, def->file, def->line, WARN_FUNCTION_OVERLOAD, ptr);
					ptr->init = lnkNode(ctx, def);
					break;
				}
			}

			if (ptr->owner == def->owner && ptr->nest >= def->nest && strcmp(def->name, unknown_tag) != 0) {
				error(ctx->rt, def->file, def->line, ERR_DECLARATION_REDEFINED, def);
				if (ptr->file && ptr->line) {
					error(ctx->rt, ptr->file, ptr->line, INFO_PREVIOUS_DEFINITION, ptr);
				}
				break;
			}
			else {
				debug(ctx->rt, def->file, def->line, WARN_DECLARATION_REDEFINED, def);
				if (ptr->file && ptr->line) {
					debug(ctx->rt, ptr->file, ptr->line, INFO_PREVIOUS_DEFINITION, ptr);
				}
			}
		}

		// if the tag is a typename it should return an instance of this type
		if (params != NULL) {
			symn tagTyp = tagType(ctx, tag);
			if (tagTyp != NULL && params->type != tagTyp) {
				error(ctx->rt, tag->file, tag->line, WARN_FUNCTION_TYPENAME, tag, params->type);
			}
		}
	}

	return def;
}

/// expand compound assignment expressions like `a += 1` into `a = a + 1`
static astn expandAssignment(ccContext ctx, astn root) {
	astn tmp = NULL;
	switch (root->kind) {
		default:
			break;

		case ASGN_add:
			tmp = newNode(ctx, OPER_add);
			break;

		case ASGN_sub:
			tmp = newNode(ctx, OPER_sub);
			break;

		case ASGN_mul:
			tmp = newNode(ctx, OPER_mul);
			break;

		case ASGN_div:
			tmp = newNode(ctx, OPER_div);
			break;

		case ASGN_mod:
			tmp = newNode(ctx, OPER_mod);
			break;

		case ASGN_shl:
			tmp = newNode(ctx, OPER_shl);
			break;

		case ASGN_shr:
			tmp = newNode(ctx, OPER_shr);
			break;

		case ASGN_and:
			tmp = newNode(ctx, OPER_and);
			break;

		case ASGN_ior:
			tmp = newNode(ctx, OPER_ior);
			break;

		case ASGN_xor:
			tmp = newNode(ctx, OPER_xor);
			break;
	}
	if (tmp != NULL) {
		// convert `a += 1` into `a = a + 1`
		tmp->op.rhso = root->op.rhso;
		tmp->op.lhso = root->op.lhso;
		tmp->file = root->file;
		tmp->line = root->line;
		root->kind = ASGN_set;
		root->op.rhso = tmp;
	}
	return root;
}

/// expand expression or declaration to statement
static astn expand2Statement(ccContext ctx, astn node, int block) {
	astn result = newNode(ctx, block ? STMT_beg : STMT_end);
	if (result != NULL) {
		result->type = ctx->type_vid;
		result->file = node->file;
		result->line = node->line;
		result->stmt.stmt = node;
	}
	return result;
}

static astn expandInitializerObj(ccContext ctx, astn varNode, astn initObj, astn list) {
	astn keys = NULL;
	ssize_t length = 0;

	symn type = varNode->type;
	if (initObj->type != NULL) {
		type = initObj->type;
	}

	// translate initialization to assignment
	if (initObj->stmt.stmt != NULL && initObj->stmt.stmt->kind == INIT_set) {
		length = -1;
	}

	for (astn init = initObj->stmt.stmt; init != NULL; init = init->next) {
		astn key = NULL;
		astn value = init;
		if (length < 0 && init->kind == INIT_set) {
			key = init->op.lhso;
			value = init->op.rhso;
			// key: value => variable.key = value;
			init->op.lhso = opNode(ctx, NULL, OPER_dot, varNode, key);
			if (!typeCheck(ctx, NULL, init->op.lhso, 0)) {
				// HACK: type of variable may differ from the initializer type:
				// object view = Button{ ... };
				typeCheck(ctx, type, key, 0);
			}
			init->op.lhso->file = key->file;
			init->op.lhso->line = key->line;
		} else {
			key = intNode(ctx, length);
			key->file = value->file;
			key->line = value->line;

			key = opNode(ctx, NULL, OPER_idx, varNode, key);
			key->file = value->file;
			key->line = value->line;

			init = opNode(ctx, NULL, INIT_set, key, value);
			init->file = value->file;
			init->line = value->line;
			init->next = value->next;
			length += 1;
		}

		if (value->kind == STMT_beg) {
			// lookup key
			if (typeCheck(ctx, NULL, init->op.lhso, 1) != NULL) {
				expandInitializerObj(ctx, init->op.lhso, value, list);
			}
			init->op.rhso = init->op.lhso;
		} else {
			addTail(list, expand2Statement(ctx, init, 0));

			// type check assignment
			symn field = NULL;
			if (typeCheck(ctx, NULL, init, 1) != NULL) {
				field = linkOf(key, 1);
			}
			if (field != NULL && !canAssign(ctx, field, value, 0)) {
				error(ctx->rt, value->file, value->line, ERR_INVALID_INITIALIZER, init);
			}
			if (field != NULL && isStatic(field)) {
				error(ctx->rt, init->file, init->line, ERR_INVALID_STATIC_FIELD_INIT, init);
			}
			init->type = ctx->type_vid;
		}

		key->next = keys;
		keys = key;
	}

	if (isArrayType(type)) {
		symn len = type->fields;
		if (len == NULL || len == ctx->length_ref) {
			symn ref = linkOf(varNode, 0);
			if (varNode->kind != TOKEN_var || (isMember(ref) && !isStatic(ref))) {
				// non-static members can not expand their size, raise error, that this is not possible yet
				error(ctx->rt, varNode->file, varNode->line, ERR_INVALID_INITIALIZER, varNode);
				return NULL;
			}

			// if slice or pointer is initialized with a literal,
			// create a fixed size array from the literal,
			// then assign it to the slice or pointer
			size_t initSize = length * refSize(type->type);
			symn arr = newSymn(ctx, ATTR_stat | KIND_typ | CAST_arr);
			symn var = newSymn(ctx, KIND_var | CAST_val);

			addLength(ctx, arr, intNode(ctx, length));
			var->file = arr->file = varNode->file;
			var->line = arr->line = varNode->line;
			var->name = "init";

			arr->type = type->type;
			var->owner = ref;
			var->type = arr;

			var->size = arr->size = initSize;
			var->offs = ref->size;
			ref->size += initSize;

			// slice = slice.array
			astn arrArr = opNode(ctx, var->type, OPER_dot, lnkNode(ctx, ref), lnkNode(ctx, var));
			astn setLen = opNode(ctx, ctx->type_vid, INIT_set, lnkNode(ctx, ref), arrArr);
			arrArr->op.lhso->type = ctx->emit_opc;	// FIXME: force ignore indirection of slice
			setLen->file = arrArr->file = ref->file;
			setLen->line = arrArr->line = ref->line;
			addHead(list, setLen);

			// for better performance initialize the fixed size array
			// slice[0] = 42 => slice.array[0] = 42
			*varNode = *arrArr;
		}
		else if (length > intValue(len->init)) {
			error(ctx->rt, varNode->file, varNode->line, ERR_INVALID_ARRAY_VALUES, len->init);
		}
	}

	// if variable is instance of object then allocate it
	if (isObjectType(type)) {
		if (ctx->libc_new == NULL) {
			error(ctx->rt, varNode->file, varNode->line, "Failed to allocate variable `%t` allocator not found or disabled", varNode);
			return initObj;
		}

		astn new = lnkNode(ctx, ctx->libc_new);
		astn arg = lnkNode(ctx, type);
		astn call = opNode(ctx, type, OPER_fnc, new, arg);
		call->file = arg->file = new->file = initObj->file;
		call->line = arg->line = new->line = initObj->line;

		astn init = opNode(ctx, ctx->type_vid, INIT_set, varNode, call);
		init->file = varNode->file;
		init->line = varNode->line;

		// make allocation the first statement
		addHead(list, expand2Statement(ctx, init, 0));
	}

	// add default initializer of uninitialized fields
	size_t unionInitSize = type->size;
	for (symn field = type->fields; field; field = field->next) {
		if (isStatic(field)) {
			continue;
		}
		if (field->offs != 0) {
			unionInitSize = 0;
			break;
		}
		dieif(unionInitSize != 0 && unionInitSize < field->size, ERR_INTERNAL_ERROR);
	}
	for (symn field = type->fields; field; field = field->next) {
		if (isStatic(field)) {
			continue;
		}
		int initialized = 0;
		// check if last usage is from the current initialization
		for (astn key = keys; key; key = key->next) {
			if (field->use == key) {
				initialized = 1;
				break;
			}
		}

		if (initialized || unionInitSize > 0) {
			if (initialized && field->size < unionInitSize) {
				warn(ctx->rt, varNode->file, varNode->line, ERR_PARTIAL_INITIALIZE_UNION, linkOf(varNode, 0), field);
			}
			continue;
		}

		if (field->name != NULL && *field->name == '.') {
			// no need to initialize hidden internal fields (.result, .type)
			continue;
		}
		if (field == ctx->length_ref) {
			// no need to initialize length field of a slice
			continue;
		}

		astn defInit = field->init;
		for (symn ovr = type->fields; ovr != NULL; ovr = ovr->next) {
			if (field->tag == ovr->tag) {
				continue;
			}
			if (field != ovr->override) {
				continue;
			}
			if (ovr->init != NULL) {
				defInit = lnkNode(ctx, ovr);
				break;
			}
		}

		if (defInit == NULL && isMutable(field) && isMutableVar(varNode)) {
			// only assignable members can be initialized with default type initializer.
			defInit = field->type->init;
		}

		if (defInit == NULL) {
			if (isFixedSizeArray(field->type)) {
				warn(ctx->rt, varNode->file, varNode->line, ERR_UNINITIALIZED_MEMBER, linkOf(varNode, 0), field);
				continue;
			}
			error(ctx->rt, varNode->file, varNode->line, ERR_UNINITIALIZED_MEMBER, linkOf(varNode, 0), field);
			continue;
		}

		if (defInit == field->init) {
			debug(ctx->rt, varNode->file, varNode->line, WARN_USING_DEF_FIELD_INITIALIZER, field, defInit);
		} else {
			debug(ctx->rt, varNode->file, varNode->line, WARN_USING_DEF_TYPE_INITIALIZER, field, defInit);
		}

		astn fld = opNode(ctx, NULL, OPER_dot, varNode, lnkNode(ctx, field));
		astn init = opNode(ctx, NULL, INIT_set, fld, defInit);
		init->file = fld->file = varNode->file;
		init->line = fld->line = varNode->line;

		addTail(list, expand2Statement(ctx, init, 0));

		typeCheck(ctx, NULL, init, 1);
		init->type = ctx->type_vid;
	}
	return list;
}

/** Expand literal initializer to initializer statement
 * complex c = {re: 5, im: 6}; => { c.re = 5; c.im = 6; };
 * int ar[3] = {1, 2, 3}; => { ar[0] = 1; ar[1] = 2; ar[2] = 3; };
 * TODO: int ar[string] = {a: 3, "b": 1}; => { ar["a"] = 3; ar["b"] = 1; };
 */
static astn expandInitializer(ccContext ctx, symn variable, astn initializer) {
	dieif(initializer->kind != STMT_beg, ERR_INTERNAL_ERROR);
	dieif(initializer->lst.tail != NULL, ERR_INTERNAL_ERROR);
	if (initializer->type == NULL) {
		// initializer type not specified, use from declaration
		initializer->type = variable->type;
	}
	if (!canAssign(ctx, variable, initializer, 0)) {
		error(ctx->rt, variable->file, variable->line, ERR_INVALID_VALUE_ASSIGN, variable, initializer);
		return initializer;
	}
	astn varNode = lnkNode(ctx, variable);
	varNode->file = variable->file;
	varNode->line = variable->line;
	return expandInitializerObj(ctx, varNode, initializer, initializer);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Parser
static astn statement_list(ccContext ctx);
static astn statement(ccContext ctx, const char *doc);
static astn declaration(ccContext ctx, ccKind attr, astn *args, const char *doc);

/** Parse qualifiers.
 * @brief scan for qualifiers: const static
 * @param ctx compiler context.
 * @return qualifiers.
 */
static ccKind qualifier(ccContext ctx) {
	ccKind result = (ccKind) 0;
	astn ast;

	while ((ast = peekToken(ctx, TOKEN_any))) {
		switch (ast->kind) {
			default:
				return result;

			// static
			case STATIC_kwd:
				skipToken(ctx, STATIC_kwd, 1);
				if (result & ATTR_stat) {
					error(ctx->rt, ast->file, ast->line, ERR_UNEXPECTED_QUAL, ast);
				}
				result |= ATTR_stat;
				break;
		}
	}
	return result;
}

/**
 * @brief Parse expression.
 * @param ctx compiler context.
 * @return root of expression tree
 */
static astn expression(ccContext ctx, int comma) {
	astn buff[maxTokenCount], *const base = buff + maxTokenCount;
	astn *ptr, ast, prev = NULL;
	astn *postfix = buff;			// postfix form of expression
	astn *stack = base;				// precedence stack
	char sym[maxTokenCount];		// symbol stack {, [, (, ?
	int unary = 1;					// start in unary mode
	int level = 0;					// precedence and top of symbol stack

	sym[level] = 0;

	// parse expression using Shunting-yard algorithm
	while ((ast = nextToken(ctx, TOKEN_any, 0))) {
		int l2r, precedence = level << 4;

		// statement tokens are not allowed in expressions
		if (ast->kind >= STMT_beg && ast->kind <= STMT_end) {
			backToken(ctx, ast);
			break;
		}

		switch (ast->kind) {
			default:
				// operator
				if (token_tbl[ast->kind].args > 0) {
					if (unary) {
						error(ctx->rt, ast->file, ast->line, ERR_SYNTAX_ERR_BEFORE, ast);
						*postfix++ = NULL;
					}
					unary = 1;
					goto tok_operator;
				}

				// literal, variable or keyword
				if (!unary) {
					backToken(ctx, ast);
					ast = NULL;
					break;
				}
				*postfix++ = ast;
				unary = 0;
				break;

			tok_operator:
				precedence |= token_tbl[ast->kind].type & 0x0f;
				l2r = token_tbl[ast->kind].type & 0x10;
				ast->op.prec = precedence;
				while (stack < base) {
					// associates left to right
					if (l2r && (*stack)->op.prec <= ast->op.prec) {
						break;
					}
					if ((*stack)->op.prec < ast->op.prec) {
						break;
					}
					*postfix++ = *stack++;
				}
				if (ast->kind != STMT_end) {
					*--stack = ast;
				}
				break;

			case LEFT_par:			// '('
				if (unary) {		// a * (3 - b)
					*postfix++ = NULL;
				}
				ast->kind = OPER_fnc;
				sym[++level] = '(';
				unary = 1;
				goto tok_operator;

			case LEFT_sqr:			// '['
				if (unary) {		// x + [ptr]
					*postfix++ = NULL;
				}
				ast->kind = OPER_idx;
				sym[++level] = '[';
				unary = 1;
				goto tok_operator;

			case OPER_sel:		// '?'
				if (unary) {
					error(ctx->rt, ast->file, ast->line, ERR_SYNTAX_ERR_BEFORE, ast);
					*postfix++ = 0;
				}
				sym[++level] = '?';
				unary = 1;
				goto tok_operator;

			case RIGHT_par:			// ')'
				if (unary && prev && prev->kind == OPER_fnc) {
					*postfix++ = NULL;
					unary = 0;
				}
				if (unary || (level > 0 && sym[level] != '(')) {
					error(ctx->rt, ast->file, ast->line, ERR_SYNTAX_ERR_BEFORE, ast);
					break;
				}
				if (level <= 0) {
					backToken(ctx, ast);
					ast = NULL;
					break;
				}
				ast->kind = STMT_end;
				level -= 1;
				unary = 0;
				goto tok_operator;

			case RIGHT_sqr:			// ']'
				if (unary && prev && prev->kind == OPER_idx) {
					*postfix++ = NULL;
					unary = 0;
				}
				if (unary || (level > 0 && sym[level] != '[')) {
					error(ctx->rt, ast->file, ast->line, ERR_SYNTAX_ERR_BEFORE, ast);
					break;
				}
				if (level <= 0) {
					backToken(ctx, ast);
					ast = NULL;
					break;
				}
				ast->kind = STMT_end;
				level -= 1;
				unary = 0;
				goto tok_operator;

			case OPER_cln:		// ':'
				if (unary || (level > 0 && sym[level] != '?')) {
					error(ctx->rt, ast->file, ast->line, ERR_SYNTAX_ERR_BEFORE, ast);
					break;
				}
				if (level <= 0) {
					backToken(ctx, ast);
					ast = NULL;
					break;
				}
				level -= 1;
				unary = 1;
				precedence = level << 4;
				goto tok_operator;

			case OPER_add:		// '+'
				if (unary) {
					ast->kind = OPER_pls;
				}
				unary = 1;
				goto tok_operator;

			case OPER_sub:		// '-'
				if (unary) {
					ast->kind = OPER_mns;
				}
				unary = 1;
				goto tok_operator;

			case OPER_and:		// '&'
				if (unary) {
					ast->kind = OPER_adr;
				}
				unary = 1;
				goto tok_operator;

			case OPER_not:		// '!'
			case OPER_cmt:		// '~'
			case PNCT_dot3:		// '...'
				if (!unary) {
					error(ctx->rt, ast->file, ast->line, ERR_SYNTAX_ERR_BEFORE, ast);
				}
				unary = 1;
				goto tok_operator;

			case OPER_com:		// ','
				if (comma && level <= 0) {
					backToken(ctx, ast);
					ast = NULL;
					break;
				}
				if (unary) {
					error(ctx->rt, ast->file, ast->line, ERR_SYNTAX_ERR_BEFORE, ast);
					*postfix++ = NULL;
				}
				unary = 1;
				goto tok_operator;
		}

		if (postfix >= stack) {
			error(ctx->rt, ctx->file, ctx->line, ERR_EXPR_TOO_COMPLEX);
			return NULL;
		}

		if (ast == NULL) {
			break;
		}

		prev = ast;
	}

	// expression is not finished: `3 + ` or `(2 + 3`
	if (unary || level > 0) {
		char *expected = "expression";
		if (level > 0) {
			switch (sym[level]) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return NULL;
				case '(':
					expected = "')'";
					break;
				case '[':
					expected = "']'";
					break;
				case '?':
					expected = "':'";
					break;
			}
		}
		const char *file = ctx->file;
		int line = ctx->line;
		if (ast == NULL) {
			ast = peekToken(ctx, TOKEN_any);
		}
		if (ast != NULL) {
			file = ast->file;
			line = ast->line;
		}
		error(ctx->rt, file, line, ERR_EXPECTED_BEFORE_TOK, expected, peekToken(ctx, TOKEN_any));
		return NULL;
	}

	// flush operators left on precedence stack
	while (stack < base) {
		*postfix++ = *stack++;
	}

	// build the abstract syntax tree from the postfix form
	for (ptr = buff; ptr < postfix; ++ptr) {
		ast = *ptr;

		if (ast == NULL) {
			*--stack = ast;
			continue;
		}

		int args = token_tbl[ast->kind].args;
		switch (args) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return NULL;

			case 2:
				ast->op.lhso = stack[1];
				// fall through
			case 1:
				ast->op.rhso = stack[0];
				// fall through
			case 0:
				break;
		}
		// check for stack underflow
		if ((stack += args) > base) {
			fatal(ERR_INTERNAL_ERROR);
			return NULL;
		}
		*--stack = expandAssignment(ctx, ast);
	}

	// stack must contain a single root element
	if (stack != base - 1) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	return *stack;
}

/** Parse initializer value, object and array literals.
 * @brief parse the initializer of a declaration.
 * @param ctx compiler context.
 * @return the initializer expression.
 */
static astn initializer(ccContext ctx) {
	astn ast = peekToken(ctx, STMT_beg) ? NULL : expression(ctx, 1);
	symn type = NULL;
	if (ast != NULL) {
		if (typeCheck(ctx, NULL, ast, 0) && isTypeExpr(ast)) {
			type = linkOf(ast, 1);
		}
	}

	if (!skipToken(ctx, STMT_beg, 0)) {	// '{'
		// initializer is an expression
		return ast;
	}

	if (skipToken(ctx, PNCT_dot3, 0)) {
		skipToken(ctx, RIGHT_crl, 1);
		// undefined initializer: {...}
		return ctx->init_und;
	}

	if (ast != NULL && !isTypeExpr(ast)) {
		// avoid value initializers: Rect x = roi {};
		error(ctx->rt, ast->file, ast->line, ERR_INVALID_BASE_TYPE, ast);
	}

	ccToken sep = STMT_end;
	astn head = NULL, tail = NULL;
	while ((ast = peekToken(ctx, TOKEN_any)) != NULL) {
		switch (ast->kind) {
			default:
				break;

			case RIGHT_crl:	// '}'
				ast = NULL;
				break;

			case STATIC_kwd:
				// allow static declarations in initializer
				declaration(ctx, qualifier(ctx), NULL, NULL);
				skipToken(ctx, STMT_end, 1);
				continue;
		}
		if (ast == NULL) {
			break;
		}

		// try to read: `<property> ':' <initializer>`
		ast = nextToken(ctx, TOKEN_var, 0);
		astn op = nextToken(ctx, OPER_cln, 0);
		if (ast != NULL && op != NULL) {
			astn init = initializer(ctx);
			op->kind = INIT_set;
			op->op.lhso = ast;
			op->op.rhso = init;
			ast = op;
		}
		else {
			if (ast != NULL) {
				backToken(ctx, ast);
			}
			ast = initializer(ctx);
		}

		if (ast == NULL) {
			// TODO: error
			break;
		}

		if (tail != NULL) {
			tail->next = ast;
		}
		else {
			head = ast;
		}
		tail = ast;

		// allow ';' or ',' as delimiter
		if (!skipToken(ctx, sep, 0)) {
			if (sep == STMT_end) {
				if (head == tail && skipToken(ctx, OPER_com, 0)) {
					sep = OPER_com;
				}
				else {
					astn next = peekToken(ctx, TOKEN_any);
					if (head != tail && next != NULL) {
						error(ctx->rt, next->file, next->line, ERR_UNMATCHED_SEPARATOR, next, STMT_end);
					}
					break;
				}
			}
			else {
				break;
			}
		}
	}

	skipToken(ctx, RIGHT_crl, 1);
	ast = newNode(ctx, STMT_beg);
	ast->stmt.stmt = head;
	ast->type = type;
	return ast;
}

/** Parse function parameter list.
 * @brief Parse the parameters of a function declaration.
 * @param ctx compiler context.
 * @param returns the return type of the function
 * @param function tag node of the function for position information
 * @return expression tree.
 */
static astn parameters(ccContext ctx, symn returns, astn function) {
	astn tok = NULL;

	if (returns != NULL) {
		symn res = install(ctx, ".result", KIND_var | ATTR_mut | refCast(returns), 0, returns, returns->init);
		tok = lnkNode(ctx, res);
		if (function != NULL) {
			res->unit = ctx->unit;
			res->file = function->file;
			res->line = function->line;
		}
	}

	if (peekToken(ctx, RIGHT_par) != NULL) {
		return tok;
	}

	// while there are tokens to read
	while (peekToken(ctx, TOKEN_any) != NULL) {
		ccKind attr = qualifier(ctx);
		astn arg = declaration(ctx, attr, NULL, NULL);

		if (arg == NULL) {
			// probably parse error
			break;
		}

		symn parameter = arg->id.link;

		if (attr & ATTR_stat) {
			fatal(ERR_UNIMPLEMENTED_FEATURE": compile time parameter");
		}

		// fixed size arrays are passed by reference
		if (castOf(parameter->type) == CAST_arr) {
			if (refCast(parameter) == CAST_val) {
				parameter->size = vm_ref_size;
				parameter->kind &= ~MASK_cast;
				parameter->kind |= CAST_ptr;
			}
		}

		tok = argNode(ctx, tok, arg);
		if (attr != 0) {
			error(ctx->rt, arg->file, arg->line, ERR_UNEXPECTED_ATTR, attr);
		}
		if (!skipToken(ctx, OPER_com, 0)) {
			if (skipToken(ctx, PNCT_dot3, 0)) {
				symn arr = newSymn(ctx, KIND_typ);

				// dynamic-size array: int a[]
				addLength(ctx, arr, NULL);
				arr->size = 2 * vm_ref_size;

				// int fun(int a, int b, int refs&...)
				if (isIndirect(parameter) && !isIndirect(parameter->type)) {
					error(ctx->rt, arg->file, arg->line, ERR_INVALID_TYPE, arg);
				}

				arr->kind = ATTR_stat | KIND_typ | CAST_arr;
				arr->offs = vmOffset(ctx->rt, arr);
				arr->type = parameter->type;
				parameter->size = arr->size;
				parameter->type = arr;
				arg->type = arr;

				parameter->kind &= ~MASK_cast;
				parameter->kind |= CAST_arr;
				parameter->kind |= ARGS_varg;
			}
			break;
		}
	}
	return tok;
}

/** Parse variable or function declaration.
 * @brief parse the declaration of a variable or function.
 * @param ctx compiler context.
 * @param attr extra attributes, like `static`
 * @param args out parameter for function arguments.
 * @return parsed syntax tree.
 */
static astn declaration(ccContext ctx, ccKind attr, astn *args, const char* doc) {
	// read typename
	astn tok = expression(ctx, 0);
	if (tok == NULL) {
		error(ctx->rt, ctx->file, ctx->line, ERR_INVALID_TYPE, tok);
		return NULL;
	}

	// check if the expression is a typename
	if (!typeCheck(ctx, NULL, tok, 1)) {
		error(ctx->rt, tok->file, tok->line, ERR_INVALID_TYPE, tok);
		return NULL;
	}
	symn type = linkOf(tok, 1);
	if (type == NULL) {
		error(ctx->rt, tok->file, tok->line, ERR_INVALID_TYPE, tok);
		return NULL;
	}

	astn tag = nextToken(ctx, TOKEN_var, 1);
	if (tag == NULL) {
		tag = tagNode(ctx, ".variable");
	}

	if (isOptional(type)) {
		attr |= ATTR_opt;
	}

	ccKind cast = CAST_any;
	if (skipToken(ctx, OPER_and, 0)) {
		// mutable reference: int a&;
		if (!isIndirect(type) && !peekToken(ctx, LEFT_sqr) && !peekToken(ctx, OPER_dot)) {
			// copyable arguments become references
			cast = CAST_ptr;
		}
		attr |= ATTR_mut;
	}
	else if (skipToken(ctx, OPER_not, 0)) {
		// immutable reference: int a!;
		// if variable is static, it will be just a constant: static int a! = 6;
		if ((attr & ATTR_stat) == 0 && !isIndirect(type) && !peekToken(ctx, LEFT_sqr) && !peekToken(ctx, OPER_dot)) {
			// copyable arguments become references, except static variables, they will be constants
			cast = CAST_ptr;
		}
		attr &= ~ATTR_mut;
	}
	else if (skipToken(ctx, OPER_sel, 0)) {
		// optional reference: int a?;
		if (!isIndirect(type) && !peekToken(ctx, LEFT_sqr) && !peekToken(ctx, OPER_dot)) {
			// copyable arguments become references
			cast = CAST_ptr;
		}
		attr |= ATTR_opt;
	}
	else {
		if (!isIndirect(type) && !peekToken(ctx, LEFT_sqr) && !peekToken(ctx, OPER_dot)) {
			// copyable arguments are mutable
			attr |= ATTR_mut;
		}
	}

	// parse function parameters
	symn def = NULL, params = NULL;
	if (skipToken(ctx, LEFT_par, 0)) {			// int a(...)
		enter(ctx, tag, def);
		astn argRoot = parameters(ctx, type, tag);
		skipToken(ctx, RIGHT_par, 1);

		params = leave(ctx, KIND_fun, vm_stk_align, 0, NULL, NULL);
		type = ctx->type_fun;

		if (args != NULL) {
			*args = argRoot;
		}

		// parse function body
		if (peekToken(ctx, STMT_beg)) {
			int isMember = (attr & ATTR_stat) == 0;
			def = declare(ctx, KIND_fun | attr | cast, tag, type, params, doc);
			if (isMember && def->override == NULL && ctx->owner && isTypename(ctx->owner) && !isStatic(ctx->owner)) {
				debug(ctx->rt, def->file, def->line, "Creating virtual method for: %T", def);
				symn override = declare(ctx, KIND_var | CAST_ptr, tag, type, params, doc);
				override->init = lnkNode(ctx, def);
			}

			// enable parameter lookup
			def->fields = params;

			enter(ctx, tag, def);
			def->init = statement(ctx, NULL);
			if (def->init == NULL) {
				def->init = newNode(ctx, STMT_beg);
				def->init->type = ctx->type_vid;
			}
			backToken(ctx, newNode(ctx, STMT_end));
			leave(ctx, KIND_def, -1, 0, NULL, NULL);

			// mark as static immutable and disable parameter lookup
			def->kind |= ATTR_stat;
			def->kind &= ~ATTR_mut;
			def->fields = NULL;
			return tag;
		}

		if (cast != CAST_any) {	// error: int a&(int x) {...}
			error(ctx->rt, tag->file, tag->line, ERR_INVALID_TYPE, tag);
		}

		// unimplemented functions are immutable function references
		attr &= ~ATTR_mut;
		cast = CAST_ptr;
	}

	// parse array dimensions
	int arrDepth = 0;
	while (skipToken(ctx, LEFT_sqr, 0)) {		// int a[...][][]...

		arrDepth += 1;
		if (cast != CAST_any && arrDepth == 1) {	// error: int a&[100]
			error(ctx->rt, tag->file, tag->line, ERR_INVALID_TYPE, tag);
		}

		symn arr = newSymn(ctx, KIND_typ);
		if (peekToken(ctx, RIGHT_sqr) != NULL) {
			// dynamic-size array: `int a[]`
			addLength(ctx, arr, NULL);
			arr->size = 2 * vm_ref_size;
			cast = CAST_arr;
		}
		else if (skipToken(ctx, OPER_mul, 0)) {
			// unknown-size array: `int a[*]`
			arr->size = vm_ref_size;
			cast = CAST_ptr;
		}
		else {
			// TODO: dynamic allocated array: `int a[n]`
			// TODO: associative array: `int a[string]`
			// TODO: sparse array: `int a[int]`
			// fixed-size array: `int a[42]`
			int64_t length = -1;
			const char *file = ctx->file;
			int line = ctx->line;
			astn len = expression(ctx, 0);
			if (len != NULL) {
				len->type = typeCheck(ctx, NULL, len, 1);
				line = len->line;
				file = len->file;
				if (eval(ctx, len, len)) {
					length = intValue(len);
				}
			}
			if (length <= 0) {
				error(ctx->rt, file, line, ERR_INVALID_ARRAY_LENGTH, len);
			}
			addLength(ctx, arr, len);
			arr->size = length * type->size;
			cast = CAST_val;
		}
		skipToken(ctx, RIGHT_sqr, 1);

		arr->kind = ATTR_stat | KIND_typ | CAST_arr;
		arr->offs = vmOffset(ctx->rt, arr);
		arr->type = type;
		type = arr;
	}

	if (cast == CAST_any) {
		cast = refCast(type);
		if (cast == CAST_any) {
			cast = CAST_val;
		}
	}

	def = declare(ctx, (attr & MASK_attr) | KIND_var | cast, tag, type, params, doc);
	if (skipToken(ctx, ASGN_set, 0)) {
		astn init = initializer(ctx);
		if (init == NULL || init == ctx->init_und) {
			// undefined initializer: {...}
			def->init = init;
			return tag;
		}

		if (init->kind == STMT_beg) {
			init = expandInitializer(ctx, def, init);
			if (init != NULL) {
				init->type = type;
			}
		}
		else if (init->type != ctx->emit_opc) {
			if (!canAssign(ctx, def, init, 0)) {
				error(ctx->rt, tag->file, tag->line, ERR_INVALID_INITIALIZER, init);
			}
		}
		def->init = init;
	}

	return tag;
}

/** Parse alias declaration.
 * @brief parse the declaration of an alias/inline expression.
 * @param ctx compiler context.
 * @return root of declaration.
 */
static astn declare_alias(ccContext ctx, ccKind attr, const char* doc) {
	symn type = NULL;
	astn tag;

	if (!skipToken(ctx, INLINE_kwd, 1)) {
		dbgTraceAst(peekToken(ctx, TOKEN_any));
		return NULL;
	}

	// inline "file.cmpl"
	if ((tag = nextToken(ctx, TOKEN_val, 0))) {
		ccToken optional = skipToken(ctx, OPER_sel, 0);
		skipToken(ctx, STMT_end, 1);

		if (!ctx->inStaticIfFalse && ccInline(ctx, tag) != 0) {
			if (optional) {
				warn(ctx->rt, tag->file, tag->line, ERR_OPENING_FILE, tag->id.name);
			} else {
				error(ctx->rt, tag->file, tag->line, ERR_OPENING_FILE, tag->id.name);
			}
			return NULL;
		}
		return NULL;
	}

	int isOptional = 0;
	tag = nextToken(ctx, TOKEN_any, 0);
	if (tag == NULL) {
		skipToken(ctx, STMT_end, 1);
		return NULL;
	}

	ccToken operator = tag->kind;
	if (tag->kind == TOKEN_var) {
		isOptional = skipToken(ctx, OPER_sel, 0);
		operator = TOKEN_any;
	} else {
		const char *op = token_tbl[tag->kind].op;
		if (op == NULL) {
			skipToken(ctx, STMT_end, 1);
			return NULL;
		}
		size_t len = strlen(op);
		tag->kind = TOKEN_var;
		tag->id.link = NULL;
		tag->id.hash = rehash(op, len + 1);
		tag->id.name = ccUniqueStr(ctx, op, len + 1, tag->id.hash);
		if (doc == NULL) {
			doc = op;
		}
	}

	enter(ctx, tag, NULL);

	if (skipToken(ctx, LEFT_par, 0)) {
		parameters(ctx, ctx->type_vid, tag);
		skipToken(ctx, RIGHT_par, 1);
	}

	skipToken(ctx, ASGN_set, 1);
	astn init = initializer(ctx);
	symn params = leave(ctx, KIND_fun, vm_stk_align, 0, NULL, NULL);
	if (init == NULL) {
		dbgTraceAst(init);
		return NULL;
	}
	if (operator != TOKEN_any) {
		int args = -1; // exclude result
		for (symn p = params; p != NULL; p = p->next) {
			args += 1;
		}
		switch (operator) {
			// both binary and unary operators
			case OPER_pls:
			case OPER_add:
			case OPER_mns:
			case OPER_sub:
				if (args != 1 && args != 2) {
					error(ctx->rt, tag->file, tag->line, ERR_INVALID_OPERATOR_ARG, operator, args);
				}
				break;

			default:
				if (args != token_tbl[operator].args) {
					error(ctx->rt, tag->file, tag->line, ERR_INVALID_OPERATOR_ARGC, operator, token_tbl[operator].args, args);
				}
				break;
		}
	}
	symn tagTyp = tagType(ctx, tag);
	if (init->kind == STMT_beg) {
		type = tagTyp;
		if (type != NULL && params != NULL) {
			// force result to be a variable
			params->type = type;
			params->size = type->size;
			params->init = type->init;
			params->kind = ARGS_inln | KIND_var;
		}
		init = expandInitializer(ctx, params, init);
		for (astn n = init->stmt.stmt; n != NULL; n = n->next) {
			if (!typeCheck(ctx, NULL, n, 0)) {
				error(ctx->rt, n->file, n->line, ERR_INVALID_TYPE, n);
			}
			n->type = ctx->type_vid;
		}
		fatal("%?s:%?u: "ERR_UNIMPLEMENTED_FEATURE": %-t", tag->file, tag->line, init);
	} else {
		type = typeCheck(ctx, NULL, init, 1);
		if (tagTyp != NULL && isTypename(tagTyp) && type == ctx->emit_opc) {
			// inline int32(uint32 x) = emit(x);
			if (isOptional) {
				attr |= ATTR_opt;
				isOptional = 0;
			}
			type = tagTyp;
		}
		/* TODO: evaluate inline expressions
		struct astNode value;
		if (eval(cc, &value, init)) {
			init = dupNode(cc, &value);
		}*/
	}
	init->type = type;
	if (type == NULL || isOptional) {
		// raise the error if lookup failed
		error(ctx->rt, init->file, init->line, ERR_INVALID_TYPE, init);
	}
	if (params != NULL) {
		size_t offs = 0;
		if (type != NULL) {
			// update the result variable, make it inline
			params->type = type;
			params->size = type->size;
			params->init = type->init;
			params->kind = (params->kind & ~MASK_cast) | refCast(type);
		}
		for (symn param = params; param != NULL; param = param->next) {
			int usages = 0;
			for (astn use = param->use; use != NULL; use = use->id.used) {
				if (use != param->tag) {
					usages += 1;
				}
			}

			if (!(param->kind & ARGS_inln) && (isInline(param) || usages < 2)) {
				// mark parameter as inline if it was not used more than once
				param->kind = (param->kind & ~MASK_kind) | KIND_def;
			}
			else {
				// mark params used more than once to be cached
				offs += padOffset(param->size, vm_stk_align);
				param->offs = offs;
			}
		}
		type = ctx->type_fun;
	}
	else {
		type = ctx->type_rec;
		// allow: `inline sin = double.sin;`
		symn initLnk = linkOf(init, 0);
		if (initLnk && initLnk->params) {
			params = initLnk->params;
		}
	}
	skipToken(ctx, STMT_end, 1);

	symn def = declare(ctx, (attr & MASK_attr) | KIND_def | ATTR_stat, tag, type, params, doc);
	if (def != NULL) {
		def->params = params;
		def->init = init;
		def->size = 0;
	}

	return tag;
}

/** Parse record declaration.
 * @brief parse the declaration of a structure.
 * @param ctx compiler context.
 * @param attr attributes: static or const.
 * @return root of declaration.
 */
static astn declare_record(ccContext ctx, ccKind attr, const char* doc) {
	if (!skipToken(ctx, RECORD_kwd, 1)) {
		dbgTraceAst(peekToken(ctx, 0));
		return NULL;
	}

	astn tag = nextToken(ctx, TOKEN_var, 0);
	int expose = 0;
	if (tag == NULL) {
		tag = tagNode(ctx, unknown_tag);
		expose = 1;
	}

	size_t baseSize = 0;
	size_t align = vm_mem_align;
	ccKind cast = CAST_val;
	symn extends = NULL;
	symn partial = NULL;

	if (attr & ATTR_stat) {
		// make not instantiable
		if (typeCheck(ctx, NULL, tag, 0) != NULL) {
			partial = tag->id.link;
		}
		cast = CAST_vid;
	}

	if (skipToken(ctx, OPER_cln, 0)) {			// ':' base type or packing
		astn tok = expression(ctx, 0);
		if (tok == NULL) {
			error(ctx->rt, tag->file, tag->line, ERR_INVALID_BASE_OR_PACK, peekToken(ctx, TOKEN_any));
		}
		else if (!typeCheck(ctx, NULL, tok, 1)) {
			error(ctx->rt, tok->file, tok->line, ERR_INVALID_BASE_OR_PACK, tok);
		}
		else {
			// type-check the base type or packing
			if (isTypeExpr(tok)) {
				// ':' extended type
				extends = linkOf(tok, 1);
				if (extends == NULL || !isObjectType(extends)) {
					// : nonsense
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_INHERITANCE, tok);
				}
				else if (partial != NULL && extends != partial) {
					// partial type extending a different type: `static struct Float64: Array { ... }`
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_INHERITANCE, tok);
				}
				else {
					baseSize = extends->size;
					align = vm_stk_align;
					cast = CAST_obj;
				}
			} else {
				// ':' packed type
				struct astNode tmp = {0};
				if (eval(ctx, &tmp, tok) != CAST_i64) {
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_BASE_OR_PACK, tok, "must be an integer constant");
				}
				else if (tmp.cInt < 0 || tmp.cInt > 32) {
					// alignment must be a small number: 0 ... 32
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_BASE_OR_PACK, tok, "must be a small number");
				}
				else if ((tmp.cInt & tmp.cInt - 1) != 0) {
					// alignment must be a power of 2: 0, 1, 2, 4, 8, ...
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_BASE_OR_PACK, tok, "must be a power of 2");
				}
				else if ((attr & ATTR_stat) != 0) {
					// static struct can not be re-aligned
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_BASE_TYPE, tok, "can not realign");
				}
				else {
					align = tmp.cInt;
				}
			}
		}
	}

	skipToken(ctx, STMT_beg, 1);	// '{'

	symn type = extends == NULL ? ctx->type_rec : extends;
	type = declare(ctx, KIND_typ | attr | cast, tag, type, NULL, doc);

	symn fields = extends ? extends->fields : type->fields;
	type->fields = fields; // allow lookup of fields from base type

	enter(ctx, tag, type);
	statement_list(ctx);
	type->fields = leave(ctx, KIND_typ | attr, align, baseSize, &type->size, fields);
	type->kind |= ATTR_stat;
	if (expose) {
		// HACK: convert the type into a variable of its own type ...?
		type->kind = KIND_var | CAST_val;
		type->type = type;

		for (symn field = type->fields; field != fields; field = field->next) {
			symn link = install(ctx, field->name, KIND_def, 0, field->type, field->tag);
			link->unit = field->unit;
			link->file = field->file;
			link->line = field->line;
			link->doc = field->doc;
		}
	}

	skipToken(ctx, RIGHT_crl, 1);	// '}'
	return tag;
}

/** Parse enum declaration.
 * @brief parse the declaration of an enumeration.
 * @param ctx compiler context.
 * @return root of declaration.
 */
static astn declare_enum(ccContext ctx, const char* doc) {

	if (!skipToken(ctx, ENUM_kwd, 1)) {
		dbgTraceAst(peekToken(ctx, 0));
		return NULL;
	}

	astn tag = nextToken(ctx, TOKEN_var, 0);
	symn base = ctx->type_int;

	if (skipToken(ctx, OPER_cln, 0)) {			// ':' base type
		astn tok = expression(ctx, 0);
		if (tok != NULL) {
			base = NULL;
			// type-check the base type
			if (typeCheck(ctx, NULL, tok, 0) && isTypeExpr(tok)) {
				base = linkOf(tok, 1);
			}
			else {
				error(ctx->rt, tok->file, tok->line, ERR_INVALID_BASE_TYPE, tok);
			}
		}
		else {
			error(ctx->rt, ctx->file, ctx->line, ERR_INVALID_BASE_TYPE, peekToken(ctx, TOKEN_any));
		}
	}

	symn type = NULL;
	if (tag != NULL) {
		type = declare(ctx, ATTR_stat | KIND_typ | CAST_enm, tag, base, NULL, doc);
		enter(ctx, tag, type);
	}

	int64_t nextValue = 0;
	astn ast = initializer(ctx);
	for (astn prop = ast->stmt.stmt; prop != NULL; prop = prop->next) {
		astn id = prop->kind == INIT_set ? prop->op.lhso : prop;
		if (id == NULL || id->kind != TOKEN_var) {
			error(ctx->rt, prop->file, prop->line, ERR_INITIALIZER_EXPECTED, prop);
			continue;
		}

		// enumeration values are documented public symbols
		symn member = declare(ctx, ATTR_stat | KIND_def | CAST_val, id, base, NULL, id->id.name);
		astn value = NULL;
		if (id == prop) {
			value = intNode(ctx, nextValue);
		}
		else {
			struct astNode temp;
			// TODO: type-check value
			value = prop->op.rhso;

			if (!typeCheck(ctx, NULL, value, 1)) {
				continue;
			}
			switch (eval(ctx, &temp, value)) {
				default:
					error(ctx->rt, id->file, id->line, ERR_INVALID_VALUE_ASSIGN, member, value);
					break;

				case CAST_bit:
				case CAST_f32:
				case CAST_f64:
					value = dupNode(ctx, &temp);
					break;

				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
					nextValue = intValue(&temp);
					value = dupNode(ctx, &temp);
					break;
			}
		}
		nextValue += 1;
		if (!canAssign(ctx, member, value, 0)) {
			error(ctx->rt, id->file, id->line, ERR_INVALID_VALUE_ASSIGN, member, value);
		}
		member->init = castTo(ctx, value, type == NULL ? base : type);
		dbgInfo("%s:%u: enum.member: `%t` => %+T", prop->file, prop->line, prop, member);
	}

	if (type != NULL) {
		type->fields = leave(ctx, KIND_typ, vm_stk_align, 0, &type->size, NULL);
		type->size = base->size;
	}

	return tag;
}

/** Parse if statement.
 * @brief parse: `if (condition) statement [else statement]`.
 * @param ctx compiler context.
 * @param attr the qualifier of the statement (ie. static if)
 * @return the parsed syntax tree.
 */
static astn statement_if(ccContext ctx, ccKind attr) {
	astn ast = nextToken(ctx, STMT_if, 1);
	if (ast == NULL) {
		dbgTraceAst(ast);
		return NULL;
	}

	skipToken(ctx, LEFT_par, 1);
	astn test = expression(ctx, 0);
	// static if statement true branch does not enter a new scope to expose declarations.
	int staticIf = (attr & ATTR_stat) != 0;
	int enterThen = 1;
	int enterElse = 1;

	if (test != NULL) {
		test->type = typeCheck(ctx, NULL, test, 1);
		if (staticIf) {
			struct astNode value;
			if (eval(ctx, &value, test)) {
				if (bolValue(&value)) {
					enterThen = 0;
				} else {
					enterElse = 0;
				}
			} else {
				error(ctx->rt, test->file, test->line, ERR_INVALID_CONST_EXPR, test);
			}
			if (enterThen) {
				enter(ctx, ast, NULL);
			}
		} else {
			if (enterThen) {
				enter(ctx, ast, NULL);
			}
			if (isTypeExpr(test)) {
				backToken(ctx, test);
				astn def = declaration(ctx, ATTR_mut, NULL, NULL);
				if (def != NULL && def->id.link->init != NULL) {
					def->type = typeCheck(ctx, NULL, def, 1);
					ast->stmt.init = def;
					if (isIndirect(def->id.link)) {
						test = opNode(ctx, NULL, OPER_cne, lnkNode(ctx, def->id.link), tagNode(ctx, "null"));
					} else {
						test = lnkNode(ctx, def->id.link);
					}
					test->type = typeCheck(ctx, NULL, test, 1);
				}
			}
		}
		ast->stmt.test = test;
	}
	skipToken(ctx, RIGHT_par, 1);

	int insideStaticIf = ctx->inStaticIfFalse;
	ctx->inStaticIfFalse = insideStaticIf || (staticIf && enterThen);

	if (peekToken(ctx, ELSE_kwd)) {
		// hack: allow else without statement: if (condition) else {statements}
		backToken(ctx, newNode(ctx, STMT_end));
	}

	ast->stmt.stmt = statement(ctx, NULL);

	// parse else part if next token is 'else'
	if (skipToken(ctx, ELSE_kwd, 0)) {
		if (enterThen) {
			leave(ctx, KIND_def, 0, 0, NULL, NULL);
			enterThen = 0;
		}
		if (enterElse) {
			enter(ctx, ast, NULL);
			enterThen = 1;
		}
		ctx->inStaticIfFalse = insideStaticIf || (staticIf && enterElse);
		ast->stmt.step = statement(ctx, NULL);
	}

	if (enterThen) {
		leave(ctx, KIND_def, 0, 0, NULL, NULL);
	}
	ctx->inStaticIfFalse = insideStaticIf;

	return ast;
}

/** Parse for statement.
 * @brief parse: `for (init; test; step) statement`.
 * @param ctx compiler context.
 * @return the parsed syntax tree.
 */
static astn statement_for(ccContext ctx) {
	astn ast = nextToken(ctx, STMT_for, 1);
	if (ast == NULL) {
		dbgTraceAst(ast);
		return NULL;
	}

	enter(ctx, ast, NULL);
	skipToken(ctx, LEFT_par, 1);

	astn init = peekToken(ctx, STMT_end) ? NULL : expression(ctx, 0);
	if (init != NULL) {
		init->type = typeCheck(ctx, NULL, init, 1);
		if (isTypeExpr(init)) {
			backToken(ctx, init);
			init = declaration(ctx, ATTR_mut, NULL, NULL);
			if (init != NULL && init->id.link->init != NULL) {
				astn astInit = init->id.link->init;
				astInit->type = typeCheck(ctx, NULL, astInit, 1);
			}
		}
		ast->stmt.init = init;
	}

	if (peekToken(ctx, OPER_cln)) {
		// transform `for (type value : values) {...}` to
		// `for (type value, iterator .it = iterator(values); .it.next(&value); ) {...}`
		// `for (iterator value = iterator(values); value.next(); ) {...}`
		// depending on the result type of: `iterator(values)`
		fatal(ERR_UNIMPLEMENTED_FEATURE);
	}

	skipToken(ctx, STMT_end, 1);
	astn test = peekToken(ctx, STMT_end) ? NULL : expression(ctx, 0);
	if (test != NULL) {
		test->type = typeCheck(ctx, NULL, test, 1);
		ast->stmt.test = test;
	}

	skipToken(ctx, STMT_end, 1);
	astn step = peekToken(ctx, RIGHT_par) ? NULL : expression(ctx, 0);
	if (step != NULL) {
		step->type = typeCheck(ctx, NULL, step, 1);
		ast->stmt.step = step;
	}

	skipToken(ctx, RIGHT_par, 1);

	enter(ctx, ast, NULL);
	ast->stmt.stmt = statement(ctx, NULL);
	leave(ctx, KIND_def, 0, 0, NULL, NULL);
	leave(ctx, KIND_def, 0, 0, NULL, NULL);

	ast->type = ctx->type_vid;
	return ast;
}

/** Parse all the statements in the current scope.
 * @brief parse statements.
 * @param ctx compiler context.
 * @return the parsed statements chained by ->next, NULL if no statements parsed.
 */
static astn statement_list(ccContext ctx) {
	astn ast, head = NULL, tail = NULL;

	const char *doc = NULL;
	while ((ast = ctx->tokNext) != NULL) {
		if (ast->kind == TOKEN_doc) {
			ctx->tokNext = ctx->tokNext->next;
			doc = ast->id.name;
			continue;
		}

		if (ast->kind == RIGHT_crl) {
			// end of statement list
			break;
		}

		if ((ast = statement(ctx, doc))) {
			if (tail != NULL) {
				tail->next = ast;
			} else {
				head = ast;
			}
			tail = ast;
		}
		doc = NULL;
	}

	return head;
}

/**
 * Parse statement.
 *
 * @param ctx compiler context.
 * @return parsed syntax tree.
 */
static astn statement(ccContext ctx, const char *doc) {
	const char *file = ctx->file;
	int line = ctx->line;
	int validStart = 1;
	astn check = NULL;
	astn ast = NULL;

	//~ skip to the first valid statement start
	while ((ast = peekToken(ctx, TOKEN_any)) != NULL) {
		switch (ast->kind) {
			default:
				// valid statement start
				file = ast->file;
				line = ast->line;
				ast = NULL;
				break;

			// invalid statement start tokens
			case RIGHT_par:
			case RIGHT_sqr:
			case RIGHT_crl:
			case TOKEN_any:
				if (validStart == 1) {
					error(ctx->rt, ast->file, ast->line, ERR_UNEXPECTED_TOKEN, ast);
					validStart = 0;
				}
				skipToken(ctx, TOKEN_any, 0);
				break;
		}
		if (ast == NULL) {
			break;
		}
	}

	// parse the statement
	ccKind attr = qualifier(ctx);
	ast = nextToken(ctx, TOKEN_any, 0);
	if (ast == NULL) {
		warn(ctx->rt, file, line, WARN_NO_NEW_LINE_AT_END, ast);
		return NULL;
	}

	switch (ast->kind) {
		case STMT_end:             // ;
			warn(ctx->rt, ast->file, ast->line, WARN_EMPTY_STATEMENT, ast);
			recycle(ctx, ast);
			ast = NULL;
			break;

		case STMT_beg:   // { ... }
			ast->stmt.stmt = statement_list(ctx);
			skipToken(ctx, RIGHT_crl, 1);
			ast->type = ctx->type_vid;

			if (ast->stmt.stmt == NULL) {
				// remove nodes such: {{;{;};{}{}}}
				recycle(ctx, ast);
				ast = NULL;
			}
			break;

		case STMT_if:   // if (...)
			backToken(ctx, ast);
			ast = statement_if(ctx, attr);
			if (ast != NULL) {
				ast->type = ctx->type_vid;
				astn ifThen = ast->stmt.stmt;
				// force static if to use compound statements
				if (ifThen != NULL && ifThen->kind != STMT_beg) {
					ast->stmt.stmt = expand2Statement(ctx, ifThen, 1);
					if (attr & ATTR_stat) {
						error(ctx->rt, ifThen->file, ifThen->line, WARN_USE_BLOCK_STATEMENT, ifThen);
					} else {
						warn(ctx->rt, ifThen->file, ifThen->line, WARN_USE_BLOCK_STATEMENT, ifThen);
					}
				}
				astn ifElse = ast->stmt.step;
				if (ifElse != NULL && ifElse->kind != STMT_beg) {
					ast->stmt.step = expand2Statement(ctx, ifElse, 1);
					if (attr & ATTR_stat) {
						error(ctx->rt, ifElse->file, ifElse->line, WARN_USE_BLOCK_STATEMENT, ifElse);
					}
					else if (ifElse->kind != STMT_if) {
						warn(ctx->rt, ifElse->file, ifElse->line, WARN_USE_BLOCK_STATEMENT, ifElse);
					}
				}
				if (attr & ATTR_stat) {
					ast->kind = STMT_sif;
					attr &= ~ATTR_stat;
				}
			}
			break;

		case STMT_for:   // for (...)
			backToken(ctx, ast);
			ast = statement_for(ctx);
			if (ast != NULL) {
				ast->type = ctx->type_vid;
				if (attr & ATTR_stat) {
					ast->kind = STMT_sfor;
					attr &= ~ATTR_stat;
				}
			}
			break;

		case STMT_brk:   // break;
			skipToken(ctx, STMT_end, 1);
			ast->type = ctx->type_vid;
			for (int i = ctx->nest - 1; i >= 0; i -= 1) {
				if (ctx->scopeStack[i]->kind == STMT_for) {
					ast->jmp.scope = ctx->scope;
					ast->jmp.nest = i;
					break;
				}
			}
			break;

		case STMT_con:   // continue;
			skipToken(ctx, STMT_end, 1);
			ast->type = ctx->type_vid;
			for (int i = ctx->nest - 1; i >= 0; i -= 1) {
				if (ctx->scopeStack[i]->kind == STMT_for) {
					ast->jmp.scope = ctx->scope;
					ast->jmp.nest = i;
					break;
				}
			}
			break;

		case STMT_ret:   // return expression;
			if (!skipToken(ctx, STMT_end, 0)) {
				astn result = initializer(ctx);
				symn function = ctx->owner;

				if (result && function && isFunction(function)) {
					dieif(strcmp(function->params->name, ".result") != 0, ERR_INTERNAL_ERROR);
					if (result->kind == STMT_beg) {
						result = expandInitializer(ctx, function->params, result);
						result->type = ctx->type_vid;
					}
					ast->ret.value = opNode(ctx, result->type, ASGN_set, lnkNode(ctx, function->params), result);
				} else {
					// returning from a non-function, or returning a statement?
					error(ctx->rt, ast->file, ast->line, ERR_UNEXPECTED_TOKEN, ast);
				}
				skipToken(ctx, STMT_end, 1);
			}
			ast->ret.func = ctx->owner;
			ast->type = ctx->type_vid;
			check = ast->ret.value;
			break;

		// type, enum and alias declaration
		case INLINE_kwd: // alias
			if (peekToken(ctx, LEFT_par)) {
				goto expr;
			}
			backToken(ctx, ast);
			ast = declare_alias(ctx, attr, doc);
			if (ast != NULL) {
				dieif(ast->kind != TOKEN_var, ERR_INTERNAL_ERROR);
				dieif(doc && ast->id.link->doc != doc, ERR_INTERNAL_ERROR);
				ast->type = ctx->type_rec;
				doc = NULL;
				//ast = expand2Statement(cc, ast, 0);
			}
			break;

		case RECORD_kwd: // struct
			if (peekToken(ctx, LEFT_par)) {
				ast = expression(ctx, 0);
				if (ast == NULL) {
					error(ctx->rt, file, line, ERR_INVALID_TYPE, ast);
					break;
				}
				symn type = typeCheck(ctx, NULL, ast, 1);
				if (type == NULL) {
					error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
					type = ctx->type_vid;
				}
				ast = lnkNode(ctx, type);
				// todo: cleanup
				goto expr;
			}

			backToken(ctx, ast);
			ast = declare_record(ctx, attr, doc);
			if (ast != NULL) {
				dieif(ast->kind != TOKEN_var, ERR_INTERNAL_ERROR);
				logif(doc && ast->id.link->doc != doc, ERR_INTERNAL_ERROR);
				ast->type = ctx->type_rec;
				doc = NULL;
				//ast = expand2Statement(cc, ast, 0);
			}
			attr &= ~ATTR_stat;
			break;

		case ENUM_kwd:   // enum
			backToken(ctx, ast);
			ast = declare_enum(ctx, doc);
			if (ast != NULL) {
				dieif(ast->kind != TOKEN_var, ERR_INTERNAL_ERROR);
				dieif(doc && ast->id.link->doc != doc, ERR_INTERNAL_ERROR);
				ast->type = ctx->type_rec;
				doc = NULL;
				//ast = expand2Statement(cc, ast, 0);
			}
			break;

		expr:
		default:
			backToken(ctx, ast);
			ast = expression(ctx, 0);
			symn type = typeCheck(ctx, NULL, ast, 1);
			if (type == NULL) {
				error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
				type = ctx->type_vid;
			}
			else if (isTypeExpr(ast)) {
				backToken(ctx, ast);
				ast = declaration(ctx, attr | ATTR_mut, NULL, doc);
				if (ast != NULL) {
					dieif(ast->kind != TOKEN_var, ERR_INTERNAL_ERROR);
					dieif(doc && ast->id.link->doc != doc, ERR_INTERNAL_ERROR);
					attr &= ~(ast->id.link->kind & MASK_attr);
					check = ast->id.link->init;
					type = ast->type;
					doc = NULL;
				}
			} else {
				check = ast;
				switch (ast->kind) {
					default:
						warn(ctx->rt, ast->file, ast->line, ERR_STATEMENT_EXPECTED, ast);
						break;

					case OPER_fnc:
					case ASGN_set:
						ast = expand2Statement(ctx, ast, 0);
						type = ctx->type_vid;
						break;
				}
			}
			skipToken(ctx, STMT_end, 1);
			if (ast != NULL) {
				ast->type = type;
			}
			break;
	}

	if (ast != NULL) {
		// raise an error if a statement is inside a record declaration
		if (ctx->owner && !isFunction(ctx->owner) && isTypename(ctx->owner)) {
			if (ast->kind > STMT_beg && ast->kind <= STMT_end && ast->kind != STMT_sif) {
				error(ctx->rt, ast->file, ast->line, ERR_DECLARATION_EXPECTED" in %T", ast, ctx->owner);
			}
		}
	}
	// type check
	if (check != NULL) {
		check->type = typeCheck(ctx, NULL, check, 1);
	}

	if (ast != NULL) {
		file = ast->file;
		line = ast->line;
	}
	if (doc != NULL) {
		warn(ctx->rt, file, line, ERR_INVALID_DOC_COMMENT);
	}
	if (attr != 0) {
		error(ctx->rt, file, line, ERR_UNEXPECTED_ATTR, attr);
	}

	return ast;
}

astn ccAddUnit(ccContext ctx, const char *file, int line, char *text) {
	ctx->unit = file;
	int nest = ctx->nest;
	if (ccParse(ctx, file, line, text) != 0) {
		error(ctx->rt, NULL, 0, ERR_OPENING_FILE, file);
		return NULL;
	}

	astn unit = statement_list(ctx);
	if (unit != NULL) {
		unit = expand2Statement(ctx, unit, 1);

		// add parsed unit to module
		if (ctx->root != NULL && ctx->root->kind == STMT_beg) {
			addTail(ctx->root, unit);
		}
	} else {
		// in case of an empty file return an empty statement
		unit = newNode(ctx, STMT_beg);
	}

	if (ccParse(ctx, NULL, 0, NULL) != 0) {
		return NULL;
	}
	if (nest != ctx->nest) {
		return NULL;
	}
	return unit;
}

symn ccAddCall(ccContext ctx, vmError call(nfcContext), const char *proto) {
	//~ from: int64 zxt(int64 val, int offs, int bits)
	//~ make: inline zxt(int64 val, int offs, int bits) = nfc(42);
	if (call == NULL || proto == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	int nest = ctx->nest;
	if (ccParse(ctx, NULL, 0, (char*)proto) != 0) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}

	astn args = NULL;
	astn init = declaration(ctx, KIND_def | ATTR_mut, &args, type_doc_builtin);

	rtContext rt = ctx->rt;
	if (ccParse(ctx, NULL, 0, NULL) != 0) {
		error(rt, NULL, 0, ERR_INVALID_DECLARATION, proto);
		return NULL;
	}
	if (init == NULL || init->kind != TOKEN_var) {
		error(rt, NULL, 0, ERR_INVALID_DECLARATION, proto);
		return NULL;
	}
	symn sym = init->id.link;
	if (sym == NULL) {
		error(rt, NULL, 0, ERR_INVALID_DECLARATION, proto);
		return NULL;
	}
	if (nest != ctx->nest) {
		error(rt, NULL, 0, ERR_INVALID_DECLARATION, proto);
		return NULL;
	}

	// allocate nodes
	rt->_end -= sizeof(struct list);
	list lst = (list) rt->_end;

	rt->_beg = padPointer(rt->_beg, vm_mem_align);
	libc nfc = (libc) rt->_beg;
	rt->_beg += sizeof(struct libc);

	if(rt->_beg >= rt->_end) {
		fatal(ERR_MEMORY_OVERRUN);
		return NULL;
	}

	size_t nfcPos = 0;
	if (ctx->native != NULL) {
		libc last = (libc) ctx->native->data;
		nfcPos = last->offs + 1;
	}
	lst->data = (void *) nfc;
	lst->next = ctx->native;
	ctx->native = lst;

	init = newNode(ctx, TOKEN_opc);
	init->type = sym->type;
	init->opc.code = opc_nfc;
	init->opc.args = nfcPos;

	sym->kind = ATTR_stat | KIND_def;
	sym->init = init;
	// native calls are undocumented public symbols
	sym->unit = NULL;

	// update the native call
	nfc->proto = proto;
	nfc->call = call;
	nfc->sym = sym;
	nfc->offs = nfcPos;
	nfc->out = 0;
	nfc->in = 0;

	if (isInvokable(sym)) {
		init->type = sym->params->type;
		sym->params->kind = (sym->params->kind & MASK_attr) | KIND_def;
		for (symn param = sym->params->next; param != NULL; param = param->next) {
			// TODO: param->kind = (param->kind & MASK_attr) | KIND_def;
			param->tag = NULL;

			// the result of a native functions is not pushed to the stack
			param->offs -= sym->params->size;

			// input is the highest parameter offset
			if (nfc->in < param->offs) {
				nfc->in = param->offs;
			}
		}
		nfc->in /= vm_stk_align;
		nfc->out = sym->params->size / vm_stk_align;
	}
	else {
		nfc->out = sym->type->size / vm_stk_align;
	}

	dbgInfo("nfc: %02X, in: %U, out: %U, func: %T", nfcPos, nfc->in, nfc->out, nfc->sym);
	return sym;
}
