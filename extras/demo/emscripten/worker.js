var Module = {
	initialized: false,
	relativeUrl: function(path) {
		let worker = '/extras/demo/emscripten/worker.js';
		if (location.href.endsWith(worker) && !path.startsWith('/')) {
			return location.href.substr(0, location.href.length - worker.length + 1) + path;
		}
		return path;
	},
	locateFile: function(path) {
		return './' + path;
	},
	dynamicLibraries: [
		'libFile.wasm'
	],
	print: function(text) {
		postMessage({ print: text });
	},
	onRuntimeInitialized: function () {
		ENV.CMPL_HOME = '/';
		FS.mkdirTree(Module.workspace);
		FS.chdir(Module.workspace);
		postMessage({initialized: true});
		Module.initialized = true;
	}
};

importScripts("module.js");
importScripts("cmpl.js");

onmessage = function(event) {
	let data = event.data;
	//console.log(data);
	let result = {};
	try {
		if (!Module.initialized) {
			throw "Module not initialized";
		}

		function onSyncWorkSpace(error) {
			if (error != null) {
				postMessage({error: 'error: ' + error});
				return;
			}
		}
		function onInitWorkSpace(error) {
			if (error != null) {
				postMessage({error: 'error: ' + error});
				return;
			}
			onmessage(event);
		}

		// setup custom workspace directory
		if (Module.initWorkspace(data.workspace, onInitWorkSpace)) {
			return;
		}

		let list = null;
		let sync = false;

		// download and store project files
		if (data.project != null) {
			Module.wgetFiles(data.project);
			list = [Module.workspace];
			sync = true;
		}

		// list files from custom directory
		if (data.folder != null) {
			list = data.folder;
		}

		// open, save, download file
		if (data.file !== undefined) {
			let operation = 'operation';
			try {
				let path = data.file;
				if (data.content != null) {
					if (!Module.pathExists(path)) {
						list = [Module.workspace];
					}
					// save file content
					operation = 'save';
					Module.saveFile(path, data.content);
					sync = true;
				}
				else if (data.url != null) {
					if (!Module.pathExists(path)) {
						list = [Module.workspace];
					}
					operation = 'download';
					Module.wgetFiles([data]);
					sync = true;
				}
				else {
					// read file content
					operation = 'read';
					result.content = Module.readFile(path);
					result.file = data.file;
					result.line = data.line;
				}
			} catch (err) {
				result.error = 'File ' + operation + ' failed[' + data.file + ']: ' + err;
				console.trace(err);
			}
		}

		// execute
		if (data.execute !== undefined) {
			try {
				callMain(data.execute);
				result.exitCode = 0;
				sync = true;
			} catch (error) {
				result.exitCode = -1;
			}
		}

		if (list != null) {
			result.list = Module.listFiles(list);
		}
		if (sync) {
			FS.syncfs(onSyncWorkSpace);
		}
	} catch (err) {
		result.error = 'operation failed: ' + err;
		console.trace(err);
	}
	postMessage(result);
};