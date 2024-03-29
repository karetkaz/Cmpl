Module.workspace = '/workspace';
Module.locateFile = function(path) {
	return (Module.locatePath || '/Cmpl/build/wasm/') + path;
};

Module.importScripts(Module.locateFile('cmpl.js'));

Module.listFiles = function(folders, recursive) {
	let cwd = FS.cwd() + '/';
	let result = [];

	function listRecursive(path) {
		let dir = FS.analyzePath(path);
		if (dir && dir.exists && dir.object) {
			if (!path.endsWith('/')) {
				path = path + '/';
			}
			for (let file in dir.object.contents) {
				let filepath = path + file;
				if (dir.object.contents[file].isFolder) {
					filepath += '/';
					if (recursive) {
						listRecursive(filepath);
						filepath = null;
					}
				}
				if (filepath) {
					if (filepath.startsWith(cwd) && filepath !== cwd) {
						result.push(filepath.substr(cwd.length));
					} else {
						result.push(filepath);
					}
				}
			}
		}
	}

	if (folders && folders.constructor === String) {
		folders = [folders];
	}

	for (let folder of folders) {
		let tail = result.length;
		listRecursive(folder);
		let sorted = result.slice(tail).sort(function(lhs, rhs) {
			let lhsDir = lhs.endsWith("/");
			let rhsDir = rhs.endsWith("/");
			if (lhsDir !== rhsDir) {
				return lhsDir ? -1 : 1;
			}
			return lhs.localeCompare(rhs);
		});
		result = result.slice(0, tail).concat(sorted);

		if (folders.length === 1 && folders[0] === Module.workspace) {
			for (let i = 0; i < result. length; ++i) {
				result[i] = result[i].substr(Module.workspace.length + 1);
			}
		}
	}
	return result;
};

Module.pathExists = function(path) {
	return FS.analyzePath(path).exists;
};

Module.absolutePath = function(path) {
	if (path == null) {
		return path;
	}
	if (path.startsWith('/')) {
		// already absolute path
		return path;
	}

	if (path.startsWith('~/')) {
		// relative workspace path
		return Module.workspace + path.substr(1);
	}

	// relative path
	return Module.workspace + '/' + path;
};

Module.parentDir = function(path) {
	return path.replace(/^(.*[/])?(.*)(\..*)$/, "$1");
};

Module.saveFile = function(path, content) {
	// persist the content of the file
	path = Module.absolutePath(path);
	FS.mkdirTree(Module.parentDir(path));
	FS.writeFile(path, content, {encoding: 'binary'});
};

Module.readFile = function(path) {
	// persist the content of the file
	return FS.readFile(path, {encoding: 'utf8'});
};

Module.onRuntimeInitialized = function() {
	ENV.CMPL_HOME = '/';
	FS.mkdirTree(Module.workspace);
	FS.chdir(Module.workspace);
	Module.initialized = true;
	Module.process({initialized: true});
};

Module.initWorkspace = function(name, callback) {
	if (name == null || name === "") {
		return false;
	}
	let prefix = '/workspace-'
	if (!name.startsWith(prefix)) {
		name = prefix + name;
	}
	if (Module.workspace === name) {
		return false;
	}
	Module.workspace = name;
	FS.mkdirTree(Module.workspace);
	FS.chdir(Module.workspace);
	FS.mount(IDBFS, {}, Module.workspace);
	FS.syncfs(true, callback || function() {
		console.log("Workspace initialized");
	});
	return true;
};

Module.openProjectFile = function(data, callBack) {
	if (!Module.initialized) {
		throw "Module not initialized";
	}

	function onInitWorkSpace(error) {
		if (error != null) {
			Module.printErr('error: ' + error);
			return;
		}
		if (data.path == null) {
			data.path = Module.workspace + '/';
		}
		try {
			// TODO: remove recursive calls
			Module.openProjectFile(data, callBack);
		} catch (err) {
			Module.printErr('operation failed: ' + err);
		}
	}

	// setup custom workspace directory
	if (Module.initWorkspace(data.workspace, onInitWorkSpace)) {
		return;
	}

	let list = null;
	let sync = false;

	let result = {};
	// download and store project files
	if (data.project != null) {
		sync = !Module.wgetFiles(data.project);
		list = [Module.workspace];
	}

	// open, save, download file
	else if (data.path !== undefined) {
		let operation = 'operation';
		try {
			let open = true;
			let path = Module.absolutePath(data.path);

			// not a file, list the content of the directory
			if (path.endsWith('/')) {
				open = false;
				list = [path];
			}

			if (data.content === null) {
				if (Module.pathExists(path)) {
					// file deleted, refresh file list
					list = [Module.parentDir(path)]
				}
				// delete file content
				FS.unlink(Module.absolutePath(path));
				operation = 'delete';
				open = false;
				sync = true;
			}
			else if (data.content !== undefined) {
				if (!Module.pathExists(path)) {
					// file saved, refresh file list
					list = [Module.parentDir(path)]
				}
				// save file content
				Module.saveFile(path, data.content);
				operation = 'saved';
				open = false;
				sync = true;
			}
			else if (data.url != null) {
				if (!Module.pathExists(path)) {
					list = [Module.workspace];
				}
				operation = 'download';
				sync = Module.wgetFiles([data]);
				open = false;
			}
			if (open || data.reopen) {
				// read file content
				operation = 'read';
				result.content = Module.readFile(path);
				result.path = data.path;
				if (data.link === true) {
					let data = FS.readFile(path, {encoding: 'binary'});
					const blob = new Blob([data], {type: 'application/octet-stream'});
					result.link = URL.createObjectURL(blob);
				}

				// file opened, refresh file list
				list = [Module.parentDir(path)]
			}
			Module.printLog('File ' + operation + ': ' + Module.absolutePath(path));
		} catch (err) {
			Module.printErr('File ' + operation + ' failed[' + data.path + ']: ' + err);
		}
	}

	// list files from custom directory
	if (list != null) {
		result.list = Module.listFiles(list);
	}
	if (sync) {
		FS.syncfs(function(error) {
			if (error == null) {
				return;
			}
			Module.printErr('error: ' + error);
		});
	}
	return callBack(result);
};

Module.wgetFiles = function(files) {
	let isAsync = Module.onFileDownloaded != null;
	let inProgress = files.length;
	function saveFile(path, content) {
		Module.saveFile(path, content);
		FS.syncfs(function(error) {
			if (error == null) {
				return;
			}
			Module.printErr('error: ' + error);
		});
	}

	if (Module.onFileDownloaded != null) {
		Module.onFileDownloaded(files.length, files.length);
	}
	for (let file of files) {
		let path = file.file || file.path;
		try {
			if (file.content != null) {
				saveFile(path, file.content);
				Module.print('Created file: ' + path);
				continue;
			}

			// download the file
			if (path == null) {
				path = file.url.replace(/^(.*[/])?(.*)(\..*)$/, "$2$3");
			}
			let url = file.url;
			if (!(url.startsWith('http://') || url.startsWith('https://'))) {
				if (Module.relativeUrl != null) {
					url = Module.relativeUrl(url);
				}
			}
			let request = new XMLHttpRequest();
			request.open('GET', url, isAsync);
			request.responseType = "arraybuffer";
			request.overrideMimeType("application/octet-stream");
			request.onreadystatechange = function () {
				if (request.readyState !== 4) {
					return;
				}
				inProgress -= 1;
				if (request.status < 200 || request.status >= 300) {
					let err = new Error(request.status + ' (' + request.statusText + ')')
					Module.printErr('Download failed: `' + request.responseURL + '`: ' + err);
					if (Module.onFileDownloaded != null) {
						Module.onFileDownloaded(inProgress, files.length);
					}
					return err;
				}
				saveFile(path, new Uint8Array(request.response));
				Module.printLog('Downloaded file: ' + path + ': ' + request.responseURL);
				if (Module.onFileDownloaded != null) {
					Module.onFileDownloaded(inProgress, files.length);
				}
				if (inProgress === 0) {
					Module.printLog("Project file(s) download complete.");

				}
			}
			request.send();
		}
		catch (err) {
			Module.printErr('Download failed: `' + path + '`: ' + err);
			inProgress -= 1;
			if (Module.onFileDownloaded != null) {
				Module.onFileDownloaded(inProgress, files.length);
			}
		}
	}
	return isAsync && files.length > 0;
};

Module.onFileDownloaded = function(progress, total) {
	let list = Module.listFiles(Module.workspace + '/');
	Module.process({ progress, total, list });
};
