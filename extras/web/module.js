Module.workspace = '/workspace';
Module.locateFile = function(path) {
	return Module.locatePath + path;
};

Module.importScripts(Module.locateFile('cmpl.js'));

// root folder of the file tree, `null` means the workspace itself
Module.listRoot = null;

Module.rootDir = function() {
	return Module.listRoot || (Module.workspace + '/');
};

/// files inside the workspace are addressed relative to it, any other root uses absolute paths
Module.rootPrefix = function(root) {
	root = root || Module.rootDir();
	return root === Module.workspace + '/' ? '' : root;
};

/// list the content of `root` in depth first order: folders first, then files, both sorted by name
Module.listFiles = function(root, recursive) {
	if (root == null || root.constructor !== String) {
		root = Module.rootDir();
	}
	if (!root.endsWith('/')) {
		root += '/';
	}

	let prefix = Module.rootPrefix(root);
	let result = [];

	function listRecursive(path, relative) {
		let dir = FS.analyzePath(path);
		if (!dir || !dir.exists || !dir.object) {
			return;
		}
		let entries = [];
		for (let file in dir.object.contents) {
			entries.push({
				name: file,
				folder: !!dir.object.contents[file].isFolder
			});
		}
		entries.sort(function(lhs, rhs) {
			if (lhs.folder !== rhs.folder) {
				return lhs.folder ? -1 : 1;
			}
			return lhs.name.localeCompare(rhs.name);
		});

		for (let entry of entries) {
			if (!entry.folder) {
				result.push(prefix + relative + entry.name);
				continue;
			}
			// the folder is part of the tree even when it is empty
			result.push(prefix + relative + entry.name + '/');
			if (recursive) {
				listRecursive(path + entry.name + '/', relative + entry.name + '/');
			}
		}
	}

	listRecursive(root, '');
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
	return path.substring(0, path.lastIndexOf('/') + 1);
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
	// the file tree follows the new workspace
	Module.listRoot = null;
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

	// refresh the file tree, it is always listed from `Module.rootDir()`
	let list = false;
	let sync = false;

	let result = {};
	// download and store project files
	if (data.project != null) {
		sync = !Module.wgetFiles(data.project);
		list = true;
	}

	// open, save, download file
	else if (data.path !== undefined) {
		let operation = 'operation';
		try {
			let open = true;
			let path = Module.absolutePath(data.path);

			// not a file, make it the root of the file tree
			if (path.endsWith('/')) {
				open = false;
				Module.listRoot = path;
				list = true;
			}

			if (data.content === null) {
				if (Module.pathExists(path)) {
					// file deleted, refresh file list
					list = true;
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
					list = true;
				}
				// save file content
				Module.saveFile(path, data.content);
				operation = 'saved';
				open = false;
				sync = true;
			}
			else if (data.url != null) {
				if (!Module.pathExists(path)) {
					list = true;
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

				// opening a file never re-roots the tree, unless the file is outside of it
				if (!path.startsWith(Module.rootDir())) {
					Module.listRoot = Module.parentDir(path);
				}
				// file opened, refresh file list
				list = true;
			}
			Module.printLog('File ' + operation + ': ' + Module.absolutePath(path));
		} catch (err) {
			Module.printErr('File ' + operation + ' failed[' + data.path + ']: ' + err);
		}
	}

	// list the whole file tree, starting at the root folder
	if (list) {
		result.root = Module.rootPrefix();
		result.list = Module.listFiles(Module.rootDir(), true);
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
	function fileDone() {
		inProgress -= 1;
		if (Module.onFileDownloaded != null) {
			Module.onFileDownloaded(inProgress, files.length);
		}
		if (inProgress === 0) {
			// persist all files at once, do not start a sync per file
			FS.syncfs(function(error) {
				if (error != null) {
					Module.printErr('error: ' + error);
				}
			});
			Module.printLog("Project file(s) download complete.");
		}
	}

	if (Module.onFileDownloaded != null) {
		Module.onFileDownloaded(files.length, files.length);
	}
	for (let file of files) {
		let path = file.file || file.path;
		try {
			if (file.content != null) {
				Module.saveFile(path, file.content);
				Module.print('Created file: ' + path);
				fileDone();
				continue;
			}

			// download the file
			if (path == null) {
				path = file.url.substring(file.url.lastIndexOf('/') + 1);
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
				if (request.status < 200 || request.status >= 300) {
					Module.printErr('Download failed: `' + request.responseURL + '`: ' + request.status + ' (' + request.statusText + ')');
					fileDone();
					return;
				}
				Module.saveFile(path, new Uint8Array(request.response));
				Module.printLog('Downloaded file: ' + path + ': ' + request.responseURL);
				fileDone();
			}
			request.send();
		}
		catch (err) {
			Module.printErr('Download failed: `' + path + '`: ' + err);
			fileDone();
		}
	}
	return isAsync && files.length > 0;
};

Module.onFileDownloaded = function(progress, total) {
	let root = Module.rootPrefix();
	let list = Module.listFiles(Module.rootDir(), true);
	Module.process({ progress, total, root, list });
};
