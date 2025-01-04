#include <Windows.h>
#include "../cmpl.h"
#include "../printer.h"
#include "../runtime.h"
#include "../type.h"

static struct pluginLib {
	struct pluginLib *next;		// next plugin
	void (*onClose)(rtContext ctx);
	HANDLE handle;
} *pluginLibs = NULL;

int importLib(rtContext ctx, const char *path) {
	HANDLE library = LoadLibraryA(path);
	if (library == NULL) {
		error(ctx, NULL, 0, "Error executing LoadLibrary(`%s`): 0x%08x", path, GetLastError());
		return -2;
	}

	int (*install)(rtContext) = (void*)GetProcAddress(library, pluginLibInstall);
	if (install == NULL) {
		error(ctx, NULL, 0, "Error executing GetProcAddress(`%s`, `%s`): 0x%08x", path, pluginLibInstall, GetLastError());
		FreeLibrary(library);
		return -1;
	}
	struct pluginLib *lib = malloc(sizeof(struct pluginLib));
	if (lib == NULL) {
		error(ctx, NULL, 0, "Error opening library: %s", "out of memory");
		FreeLibrary(library);
		return -1;
	}

	lib->onClose = (void*)GetProcAddress(library, pluginLibDestroy);
	lib->handle = library;
	lib->next = pluginLibs;
	pluginLibs = lib;

	if (install(ctx) != 0) {
		return -1;
	}

	const char *import = (void*)GetProcAddress(library, pluginLibImport);
	if (import != NULL) {
		char inlineUnit[512];
		snprintf(inlineUnit, sizeof(inlineUnit), "inline \"%s\"?;", import);
		if (!ccAddUnit(ctx->cc, NULL, 0, inlineUnit)) {
			return 1;
		}
	}
	return 0;
}

void closeLibs(rtContext ctx) {
	while (pluginLibs != NULL) {
		struct pluginLib *lib = pluginLibs;
		pluginLibs = lib->next;

		if (lib->onClose != NULL) {
			lib->onClose(ctx);
		}
		if (lib->handle) {
			FreeLibrary(lib->handle);
		}
		free(lib);
	}
}
