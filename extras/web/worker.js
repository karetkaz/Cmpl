var Module = {
	process: function(args) {
		postMessage(args)
	},
	preRun: [() => {ENV.CMPL_HOME = "/"}],
	initialized: false,
	dynamicLibraries: [
		'libFile.wasm'
	],
	relativeUrl: function(path) {
		let worker = '/Cmpl/extras/web/worker.js';
		if (location.href.endsWith(worker) && !path.startsWith('/')) {
			return location.href.substr(0, location.href.length - worker.length + 1) + path;
		}
		return path;
	},
	importScripts: function (...urls) {
		importScripts(...urls);
	},
	print: function(message) {
		// console.log(message);
		postMessage({ print: message });
	},
	printLog: function(message) {
		console.debug(message);
		// postMessage({ debug: message });
	},
	printErr: function(message) {
		console.error(message);
		postMessage({ error: message });
	},
};

importScripts("module.js");

/* communication with worker/args
request => {
	list - download these files to the workspace directory [{path|file, url|content}]
	path - list, open or save the content of the file
	url - download and save content
	content - save content
	workspace - switch workspace
	execute - arguments for the compiler
}

response => {
	list - list of files in the workspace
	data - content of the requested file
	print - print the message to the output
	error - print the error to the output
	exitcode - response to execute
	initialized
}

args => {
	project - download the content of the given file list
	content - overwrite the content of the editor
	file - name of file to be opened
	line - line position to jump to
	theme - override default: &theme=light|dark
	show - override default: &show=editor|output

	exec - override default: &exec=-profile/G/H/P/T
	dump - dump output to file: &dump=dump.txt|dump.json
	log - log output to file: &log=dump.txt
	mem - if 2MB memory is not enough: &mem=128M
	X
}*/
onmessage = function(event) {
	let data = event.data;
	//console.log(data);
	try {
		Module.openProjectFile(data, function(result) {
			// execute
			if (data.execute !== undefined) {
				let currentPath = FS.currentPath;
				try {
					FS.chdir(data.path || Module.workspace);
					result.exitCode = callMain(data.execute);
					if (result.exitCode === 0 && data.dump != null) {
						if (data.dump.constructor === String) {
							const content = Module.readFile(data.dump);
							const blob = new Blob([content], {type: 'application/octet-stream'});
							result.dump = URL.createObjectURL(blob);
							result.path = data.dump;
						}
					}
					sync = true;
				} catch (error) {
					result.error = '' + error;
					result.exitCode = -1;
					console.error(error);
				} finally {
					FS.chdir(currentPath);
				}
			}
			postMessage(result);
		});
	} catch (err) {
		Module.printErr('operation failed: ' + err);
	}
};
