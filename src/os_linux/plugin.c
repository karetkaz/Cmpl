#include <dlfcn.h>
#include <cmpl.h>
#include "../printer.h"
#include "../code.h"
#include "../type.h"

static struct pluginLib {
	struct pluginLib *next;		// next plugin
	void (*onClose)(rtContext ctx);
	void *handle;
} *pluginLibs = NULL;

int importLib(rtContext ctx, ccContext cc, const char *path) {
	void *library = dlopen(path, RTLD_NOW);
	if (library == NULL) {
		error(ctx, NULL, 0, "Error executing dlopen(`%s`): %s", path, dlerror());
		return -2;
	}

	int (*install)(rtContext, ccContext) = dlsym(library, pluginLibInstall);
	if (install == NULL) {
		error(ctx, NULL, 0, "Error executing dlsym(`%s`, `%s`): %s", path, pluginLibInstall, dlerror());
		dlclose(library);
		return -1;
	}
	struct pluginLib *lib = malloc(sizeof(struct pluginLib));
	if (lib == NULL) {
		error(ctx, NULL, 0, "Error opening library: %s", "out of memory");
		dlclose(library);
		return -1;
	}

	lib->onClose = dlsym(library, pluginLibDestroy);
	lib->handle = library;
	lib->next = pluginLibs;
	pluginLibs = lib;

	if (install(ctx, cc) != 0) {
		return -1;
	}

	const char *import = dlsym(library, pluginLibImport);
	if (import != NULL) {
		char inlineUnit[1024];
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
			dlclose(lib->handle);
		}
		free(lib);
	}
}
