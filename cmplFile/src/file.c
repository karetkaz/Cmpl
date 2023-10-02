/*******************************************************************************
 *   File: file.c
 *   Date: 2017.02.17
 *   Desc: standard file library
 *******************************************************************************
 */

#include "cmpl.h"

static const char *const proto_file = "File";
static const char *const proto_file_open = "File open(const char path[*])";
static const char *const proto_file_create = "File create(const char path[*])";
static const char *const proto_file_append = "File append(const char path[*])";
static vmError FILE_open(nfcContext ctx) {
	char *name = nextArg(ctx).ref;
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

	FILE *file = fopen(name, mode);
	if (file == NULL) {
		return nativeCallError;
	}

	rethnd(ctx, file);
	return noError;
}

static const char *const proto_file_read_buff = "int read(File file, uint8 buff[])";
static vmError FILE_read(nfcContext ctx) {
	FILE *file = (FILE *) nextArg(ctx).ref;
	rtValue buff = nextArg(ctx);
	size_t n = fread(buff.ref, 1, buff.length, file);
	reti32(ctx, n);
	return noError;
}

static const char *const proto_file_write_buff = "int write(File file, const uint8 buff[])";
static vmError FILE_write(nfcContext ctx) {
	FILE *file = (FILE *) nextArg(ctx).ref;
	rtValue buff = nextArg(ctx);
	size_t len = fwrite(buff.ref, 1, buff.length, file);
	reti32(ctx, len);
	return noError;
}

static const char *const proto_file_flush = "void flush(File file)";
static vmError FILE_flush(nfcContext ctx) {
	FILE *file = (FILE *) arghnd(ctx, 0);
	if (fflush(file) != 0) {
		return nativeCallError;
	}
	return noError;
}

static const char *const proto_file_close = "void close(File file)";
static vmError FILE_close(nfcContext ctx) {
	FILE *file = (FILE *) arghnd(ctx, 0);
	rtContext rt = ctx->rt;
	if (file == rt->logFile) {
		rt->api.raise(rt, raiseError, "can not close log file (not allowed)");
		return noError;
	}
	if (file == stdin || file == stdout || file == stderr) {
		rt->api.raise(rt, raiseWarn, "closing standard file (in, out, err)");
	}
	fclose(file);
	return noError;
}

static const char *const proto_file_seek_set = "File seek(File file, int64 position)";
static const char *const proto_file_seek_cur = "File seekCur(File file, int64 position)";
static const char *const proto_file_seek_end = "File seekEnd(File file, int64 position)";
static vmError FILE_seek(nfcContext ctx) {
	FILE *file = (FILE *) nextArg(ctx).ref;
	ssize_t pos = nextArg(ctx).i64;
	if (ctx->proto == proto_file_seek_set) {
		if (fseek(file, pos, SEEK_SET) == 0) {
			rethnd(ctx, file);
			return noError;
		}
	}
	else if (ctx->proto == proto_file_seek_cur) {
		if (fseek(file, pos, SEEK_CUR) == 0) {
			rethnd(ctx, file);
			return noError;
		}
	}
	else if (ctx->proto == proto_file_seek_end) {
		if (fseek(file, pos, SEEK_END) == 0) {
			rethnd(ctx, file);
			return noError;
		}
	}
	return nativeCallError;
}

static const char *const proto_file_tell = "int64 tell(File file)";
static vmError FILE_tell(nfcContext ctx) {
	FILE *file = (FILE *) arghnd(ctx, 0);
	ssize_t pos = ftell(file);
	if (pos < 0) {
		return nativeCallError;
	}
	reti64(ctx, pos);
	return noError;
}

static const char *const proto_file_peek = "int peek(File file)";
static vmError FILE_peek(nfcContext ctx) {
	FILE *file = (FILE *) arghnd(ctx, 0);
	int chr = ungetc(getc(file), file);
	reti32(ctx, chr);
	return noError;
}

static const char *const proto_file_get_stdIn = "File in";
static const char *const proto_file_get_stdOut = "File out";
static const char *const proto_file_get_stdErr = "File err";
static const char *const proto_file_get_logOut = "File log";
static vmError FILE_stream(nfcContext ctx) {
	if (ctx->proto == proto_file_get_stdIn) {
		rethnd(ctx, stdin);
		return noError;
	}
	if (ctx->proto == proto_file_get_stdOut) {
		rethnd(ctx, stdout);
		return noError;
	}
	if (ctx->proto == proto_file_get_stdErr) {
		rethnd(ctx, stderr);
		return noError;
	}
	if (ctx->proto == proto_file_get_logOut) {
		rethnd(ctx, ctx->rt->logFile);
		return noError;
	}

	return nativeCallError;
}

const char cmplUnit[] = "/cmplFile/lib.ci";
int cmplInit(rtContext rt) {
	ccContext cc = rt->cc;
	symn type = rt->api.ccAddType(cc, proto_file, sizeof(FILE*), 0);
	int err = type == NULL;

	if (rt->api.ccExtend(cc, type) != NULL) {

		err = err || !rt->api.ccAddCall(cc, FILE_open, proto_file_open);
		err = err || !rt->api.ccAddCall(cc, FILE_open, proto_file_create);
		err = err || !rt->api.ccAddCall(cc, FILE_open, proto_file_append);

		err = err || !rt->api.ccAddCall(cc, FILE_peek, proto_file_peek);
		err = err || !rt->api.ccAddCall(cc, FILE_tell, proto_file_tell);
		err = err || !rt->api.ccAddCall(cc, FILE_seek, proto_file_seek_set);
		err = err || !rt->api.ccAddCall(cc, FILE_seek, proto_file_seek_cur);
		err = err || !rt->api.ccAddCall(cc, FILE_seek, proto_file_seek_end);

		err = err || !rt->api.ccAddCall(cc, FILE_read, proto_file_read_buff);
		err = err || !rt->api.ccAddCall(cc, FILE_write, proto_file_write_buff);

		err = err || !rt->api.ccAddCall(cc, FILE_flush, proto_file_flush);
		err = err || !rt->api.ccAddCall(cc, FILE_close, proto_file_close);

		err = err || !rt->api.ccAddCall(cc, FILE_stream, proto_file_get_stdIn);
		err = err || !rt->api.ccAddCall(cc, FILE_stream, proto_file_get_stdOut);
		err = err || !rt->api.ccAddCall(cc, FILE_stream, proto_file_get_stdErr);
		err = err || !rt->api.ccAddCall(cc, FILE_stream, proto_file_get_logOut);

		rt->api.ccEnd(cc, type);
	}

	return err;
}
