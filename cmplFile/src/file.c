/*******************************************************************************
 *   File: file.c
 *   Date: 2017.02.17
 *   Desc: standard file library
 *******************************************************************************
 */

#include <cmpl.h>

static const char *const proto_file = "File";
static const char *const proto_file_open = "File open(char path[])";
static const char *const proto_file_create = "File create(char path[])";
static const char *const proto_file_append = "File append(char path[])";
static vmError FILE_open(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	struct nfcArgArr name = rt->api.nextArg(&al)->arr;
	const char *mode = NULL;
	if (ctx->proto == proto_file_open) {
		mode = "r";
	}
	else if (ctx->proto == proto_file_create) {
		mode = "w";
	}
	else if (ctx->proto == proto_file_append) {
		mode = "a";
	}

	if (strlen(name.ref) > name.length) {
		ctx->rt->api.raise(ctx, raiseFatal, "non zero terminated strings not implemented yet");
		return nativeCallError;
	}

	FILE *file = fopen(name.ref, mode);
	if (file == NULL) {
		return nativeCallError;
	}

	retHnd(ctx, file);
	return noError;
}

static const char *const proto_file_read_buff = "int read(File file, uint8 buff&[])";
static vmError FILE_read(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	FILE *file = rt->api.nextArg(&al)->ref;
	struct nfcArgArr buff = rt->api.nextArg(&al)->arr;
	size_t n = fread(buff.ref, 1, buff.length, file);
	retI32(ctx, n);
	return noError;
}

static const char *const proto_file_write_buff = "int write(File file, uint8 buff[])";
static vmError FILE_write(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	FILE *file = rt->api.nextArg(&al)->ref;
	struct nfcArgArr buff = rt->api.nextArg(&al)->arr;
	size_t len = fwrite(buff.ref, 1, buff.length, file);
	retI32(ctx, len);
	return noError;
}

static const char *const proto_file_flush = "void flush(File file)";
static vmError FILE_flush(nfcContext ctx) {
	FILE *file = argHnd(ctx, 0);
	if (fflush(file) != 0) {
		return nativeCallError;
	}
	return noError;
}

static const char *const proto_file_close = "void close(File file)";
static vmError FILE_close(nfcContext ctx) {
	FILE *file = argHnd(ctx, 0);
	rtContext rt = ctx->rt;
	if (file == rt->logFile) {
		rt->api.raise(ctx, raiseError, "can not close log file (not allowed)");
		return noError;
	}
	if (file == stdin || file == stdout || file == stderr) {
		rt->api.raise(ctx, raiseWarn, "closing standard file (in, out, err)");
	}
	fclose(file);
	return noError;
}

static const char *const proto_file_seek_set = "File seek(File file, int64 position)";
static const char *const proto_file_seek_cur = "File seekCur(File file, int64 position)";
static const char *const proto_file_seek_end = "File seekEnd(File file, int64 position)";
static vmError FILE_seek(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	FILE *file = rt->api.nextArg(&al)->ref;
	ssize_t pos = rt->api.nextArg(&al)->i64;
	if (ctx->proto == proto_file_seek_set) {
		if (fseek(file, pos, SEEK_SET) == 0) {
			retHnd(ctx, file);
			return noError;
		}
	}
	else if (ctx->proto == proto_file_seek_cur) {
		if (fseek(file, pos, SEEK_CUR) == 0) {
			retHnd(ctx, file);
			return noError;
		}
	}
	else if (ctx->proto == proto_file_seek_end) {
		if (fseek(file, pos, SEEK_END) == 0) {
			retHnd(ctx, file);
			return noError;
		}
	}
	return nativeCallError;
}

static const char *const proto_file_tell = "int64 tell(File file)";
static vmError FILE_tell(nfcContext ctx) {
	FILE *file = (FILE *) argHnd(ctx, 0);
	ssize_t pos = ftell(file);
	if (pos < 0) {
		return nativeCallError;
	}
	retI64(ctx, pos);
	return noError;
}

static const char *const proto_file_peek = "int peek(File file)";
static vmError FILE_peek(nfcContext ctx) {
	FILE *file = argHnd(ctx, 0);
	int chr = ungetc(getc(file), file);
	retI32(ctx, chr);
	return noError;
}

static const char *const proto_file_get_stdIn = "File in";
static const char *const proto_file_get_stdOut = "File out";
static const char *const proto_file_get_stdErr = "File err";
static const char *const proto_file_get_logOut = "File log";
static vmError FILE_stream(nfcContext ctx) {
	if (ctx->proto == proto_file_get_stdIn) {
		retHnd(ctx, stdin);
		return noError;
	}
	if (ctx->proto == proto_file_get_stdOut) {
		retHnd(ctx, stdout);
		return noError;
	}
	if (ctx->proto == proto_file_get_stdErr) {
		retHnd(ctx, stderr);
		return noError;
	}
	if (ctx->proto == proto_file_get_logOut) {
		retHnd(ctx, ctx->rt->logFile);
		return noError;
	}

	return nativeCallError;
}

const char cmplUnit[] = "cmplFile/lib.cmpl";
int cmplInit(rtContext ctx, ccContext cc) {
	symn type = ctx->api.ccAddType(cc, proto_file, sizeof(FILE*), 0);
	int err = type == NULL;

	if (ctx->api.ccExtend(cc, type) != NULL) {

		err = err || !ctx->api.ccAddCall(cc, FILE_open, proto_file_open);
		err = err || !ctx->api.ccAddCall(cc, FILE_open, proto_file_create);
		err = err || !ctx->api.ccAddCall(cc, FILE_open, proto_file_append);

		err = err || !ctx->api.ccAddCall(cc, FILE_peek, proto_file_peek);
		err = err || !ctx->api.ccAddCall(cc, FILE_tell, proto_file_tell);
		err = err || !ctx->api.ccAddCall(cc, FILE_seek, proto_file_seek_set);
		err = err || !ctx->api.ccAddCall(cc, FILE_seek, proto_file_seek_cur);
		err = err || !ctx->api.ccAddCall(cc, FILE_seek, proto_file_seek_end);

		err = err || !ctx->api.ccAddCall(cc, FILE_read, proto_file_read_buff);
		err = err || !ctx->api.ccAddCall(cc, FILE_write, proto_file_write_buff);

		err = err || !ctx->api.ccAddCall(cc, FILE_flush, proto_file_flush);
		err = err || !ctx->api.ccAddCall(cc, FILE_close, proto_file_close);

		err = err || !ctx->api.ccAddCall(cc, FILE_stream, proto_file_get_stdIn);
		err = err || !ctx->api.ccAddCall(cc, FILE_stream, proto_file_get_stdOut);
		err = err || !ctx->api.ccAddCall(cc, FILE_stream, proto_file_get_stdErr);
		err = err || !ctx->api.ccAddCall(cc, FILE_stream, proto_file_get_logOut);

		ctx->api.ccEnd(cc, type);
	}

	return err;
}
