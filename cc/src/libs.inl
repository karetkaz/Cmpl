#if defined(USEPLUGINS)
static struct pluginLib *pluginLibs = NULL;
static const char* pluginLibInstall = "ccvmInit";
static const char* pluginLibDestroy = "ccvmDone";

static int installDll(rtContext rt, int ccApiMain(rtContext)) {
	return ccApiMain(rt);
}

#if (defined(WIN32) || defined(_WIN32))
#include <windows.h>
struct pluginLib {
	struct pluginLib *next;		// next plugin
	void (*onClose)();
	HANDLE lib;
};
static void closeLibs() {
	while (pluginLibs != NULL) {
		struct pluginLib *lib = pluginLibs;
		pluginLibs = lib->next;

		if (lib->onClose) {
			lib->onClose();
		}
		if (lib->lib) {
			FreeLibrary((HINSTANCE)lib->lib);
		}
		free(lib);
	}
}
static int importLib(rtContext rt, const char* path) {
	int result = -1;
	HANDLE library = LoadLibraryA(path);
	if (library != NULL) {
		void *install = (void*)GetProcAddress(library, pluginLibInstall);
		void *destroy = (void*)GetProcAddress(library, pluginLibDestroy);
		if (install != NULL) {
			struct pluginLib *lib = malloc(sizeof(struct pluginLib));
			if (lib != NULL) {
				result = installDll(rt, install);
				lib->lib = library;
				lib->onClose = destroy;
				lib->next = pluginLibs;
				pluginLibs = lib;
			}
		}
		else {
			result = -2;
		}
	}
	return result;
}
#elif (defined(__linux__))
#include <dlfcn.h>
//#include <stddef.h>

struct pluginLib {
	struct pluginLib *next;		// next plugin
	void (*onClose)();
	void *lib;
};
static void closeLibs() {
	while (pluginLibs != NULL) {
		struct pluginLib *lib = pluginLibs;
		pluginLibs = lib->next;

		if (lib->onClose) {
			lib->onClose();
		}
		if (lib->lib) {
			dlclose(lib->lib);
		}
		free(lib);
	}
}
static int importLib(rtContext rt, const char* path) {
	int result = -1;
	void* library = dlopen(path, RTLD_NOW);
	if (library != NULL) {
		void* install = dlsym(library, pluginLibInstall);
		void* destroy = dlsym(library, pluginLibDestroy);
		if (install != NULL) {
			struct pluginLib *lib = malloc(sizeof(struct pluginLib));
			result = installDll(rt, install);
			lib->onClose = destroy;
			lib->next = pluginLibs;
			lib->lib = library;
			pluginLibs = lib;
		}
		else {
			result = -2;
		}
	}
	return result;
}
#else
static void closeLibs() {}
static int importLib(rtContext rt, const char* path) {
	error(rt, path, 1, "dynamic linking is not available in this build.");
	(void)rt;
	(void)path;
	(void)pluginLibInstall;
	(void)pluginLibDestroy;
	return -1;
}
#endif
#else
static void closeLibs() {}
static int importLib(rtContext rt, const char* path) {
	error(rt, path, 1, "dynamic linking is not available in this build.");
	(void)rt;
	(void)path;
	return -1;
}
#endif
