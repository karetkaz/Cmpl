function hasStyle(element, ...styles) {
	if (element && element.constructor === String) {
		// allow to reference using id.
		element = document.getElementById(element);
		if (element == null) {
			console.debug("invalid element");
			return false;
		}
	}

	for (let style of styles) {
		if (element.classList.contains(style)) {
			return true;
		}
	}
	return false;
}
function setStyle(element, ...styles) {
	if (element && element.constructor === String) {
		// allow to reference using id.
		element = document.getElementById(element);
		if (element == null) {
			console.debug("invalid element");
			return;
		}
	}

	let prevChanged = false;
	for (let style of styles) {
		if (style == null || style === '' || style.constructor !== String) {
			continue;
		}

		// conditional apply
		if (style.startsWith('?')) {
			if (!prevChanged) {
				continue;
			}
			style = style.substring(1);
		}

		prevChanged = false;
		// remove style
		if (style.startsWith('-')) {
			style = style.substring(1);
			if (element.classList.contains(style)) {
				element.classList.remove(style);
				prevChanged = true;
			}
			continue;
		}

		// toggle style
		if (style.startsWith('~')) {
			style = style.substring(1);
			if (element.classList.contains(style)) {
				element.classList.remove(style);
			} else {
				element.classList.add(style);
			}
			prevChanged = true;
			continue;
		}

		if (element.classList.contains(style)) {
			// style already set
			continue;
		}
		element.classList.add(style);
		prevChanged = true;
	}

	if (element === document.body) {
		editor.refresh();
	}
	return prevChanged;
}

var props = props || {};
props.title = document.title;
//[0: match, 1: host, 2: path, 3: file, 4: line, 5: column, 6: query, 7: hash]
const pathMather = /(?=[\w\/])((?:\w+:\/\/)(?:[^:@\n\r"'`]+(?:\:\w+)?@)?(?:[^:\/?#\n\r"'`]+)(?:\:\d+)?(?=[\/?#]))?([^<>=:;,?#*|\n\r"'`]*\/)?([^<>=:;,?#*|\n\r"'`]*)?(?:\:(?:(\d+)(?:\:(\d+))?))?(?:\?([^#\n\r"'`]*))?(?:\#([^\n\r"'`]*))?/;

let editor = CodeMirror.fromTextArea(input, {
	mode: 'text/x-cmpl',
	lineNumbers: true,
	tabSize: 4,
	indentUnit: 4,
	indentWithTabs: true,
	keyMap: 'extraKeys',

	styleActiveLine: true,
	matchBrackets: true,
	showTrailingSpace: true,

	foldGutter: true,
	gutters: ['CodeMirror-linenumbers', 'breakpoints', 'CodeMirror-foldgutter'],
	highlightSelectionMatches: {showToken: /\w/, annotateScrollbar: true}
});
let terminal = Terminal(output, function(escaped, text) {
	return escaped.replace(new RegExp(pathMather, 'g'), function (match, host, path, file, line, column, query, hash) {
		if (file === undefined) {
			return match;
		}

		if (host !== undefined) {
			return '<a href="' + encodeURI(match) + '" target="_blank">' + match + '</a>';
		}
		path = path || '';
		line = +(line || 0);
		column = +(column || 0);
		if (line > 0) {
			return '<a href="javascript:void(params.update({ content: null, path:\'' + path + file + ':' + line + '\'}));">' + match + '</a>';
		}
		return match;
	});
});
let params = JsArgs('#', function (params, changes) {
	//console.trace('params: ', changes, params);

	function content(value) {
		if (value != null) {
			try {
				return atob(value);
			} catch (e) {
				console.debug(e);
			}
		}
		return value;
	}
	function project(value) {
		value = content(value);
		if (value != null) {
			if (value.startsWith('[{')) {
				return JSON.parse(value);
			}

			let result = [];
			for (let url of value.split(';')) {
				result.push({ url });
			}
			return result;
		}
		return value;
	}

	/// get the full file path, without line
	function getPath(defValue) {
		let path = params.path;
		if (path == null) {
			// not defined
			return defValue;
		}
		if (path.endsWith('/')) {
			// it's a directory
			return path;
		}
		let match = path.match(pathMather);
		if (match && match[2] && match[3]) {
			return match[2] + match[3];
		}
		if (match && match[3]) {
			return match[3];
		}
		if (match && match[2]) {
			return match[2];
		}
		return defValue;
	}

	/// get file name only, no folder and no line
	function getFile(defValue) {
		let path = params.path;
		if (path == null) {
			// not defined
			return defValue;
		}
		if (path.endsWith('/')) {
			// it's a directory
			return defValue;
		}

		let match = path.match(pathMather);
		if (match && match[3]) {
			return match[3];
		}
		return defValue;
	}

	/// get the line number
	function getLine(defValue) {
		let path = params.path;
		if (path == null) {
			// not defined
			return defValue;
		}
		if (path.endsWith('/')) {
			// it's a directory
			return defValue;
		}
		let match = path.match(pathMather);
		if (match && match[4]) {
			return match[4];
		}
		return defValue;
	}

	// set window title
	if (params.path != null) {
		document.title = getFile('?') + ' - ' + props.title;
	} else {
		document.title = props.title;
	}

	setStyle(libFile, params.libFile != null ? 'checked' : '-checked');
	setStyle(libGfx, params.libGfx != null ? 'checked' : '-checked');

	// no changes => page loaded
	if (changes === undefined) {
		params.__proto__.getPath = getPath;
		params.__proto__.getFile = getFile;
		params.__proto__.getLine = getLine;
		// setup theme, only after loading
		let theme = undefined;
		let mode = undefined;
		if (params.theme != null) {
			switch (params.theme) {
				case 'light':
				case 'dark':
					theme = params.theme;
					// mode = undefined;
					break;

				case 'embedded-light':
					theme = 'light';
					mode = 'embedded';
					break;

				case 'embedded-dark':
					theme = 'dark';
					mode = 'embedded';
					break;

				case 'embedded':
				case 'normal':
				case 'mobile':
				case 'dbg':
					// theme = undefined;
					mode = params.theme;
					break;
			}
		}

		// check if the theme is defined in the html, in case it is, use that one
		if (theme === undefined) {
			if (document.body.classList.contains('light')) {
				theme = 'light';
			}
			else if (document.body.classList.contains('dark')) {
				theme = 'dark';
			}
		}

		// theme is not defined in params or in the html, so request it from browser
		if (window.matchMedia) {
			let lightScheme = window.matchMedia('(prefers-color-scheme: light)');
			lightScheme.addEventListener('change', function (e) {
				if (params.theme === undefined) {
					// keep the theme defined in the params
					setStyle(document.body, '-dark', e.matches ? 'light' : '-light');
				}
			});
			if (theme === undefined) {
				theme = lightScheme.matches ? 'light' : 'dark';
			}
		}

		// mode is not defined, try to guess it
		if (mode === undefined) {
			if ('ontouchstart' in document.documentElement) {
				mode = 'mobile';
			} else {
				mode = 'normal';
			}
		}

		setStyle(document.body, '-dark', '-light', theme);

		// custom layout, only after loading
		props.mobile = mode === 'mobile';
		switch (mode) {
			case 'normal':
				if (params.workspace != null || params.project != null) {
					setStyle(document.body, 'output');
				} else {
					setStyle(document.body, '-output');
				}
				editor.setSize('100%', '100%');
				editor.focus();
				break;

			case 'mobile':
				// for mobile show the left menu, in case there is no content loaded
				if (params.content != null || params.path != null) {
					setStyle(document.body, '-left-pin', '-left-bar', '-output');
				} else {
					setStyle(document.body, '-left-pin', 'left-bar', '-output');
				}
				document.body.style.fontSize = '1.2em';
				editor.setSize('100%', '100%');
				editor.setOption('tabSize', 2);
				editor.setOption('indentUnit', 2);
				editor.focus();
				break;

			case 'embedded':
				setStyle(document.body, 'autoheight', '-left-bar', '-output');
				editor.setOption('viewportMargin', Infinity);
				break;

			case 'dbg':
				setStyle(document.body, 'output', 'left-pin', 'left-bar', 'right-pin', 'right-bar', 'bottom-pin', 'bottom-bar');
				editor.setSize('100%', '100%');
				editor.focus();
				break;
		}

		if (params.project != null) {
			setStyle(projectOptions, '-hidden');
		}
		if (params.workspace != null) {
			workspaceName.innerText = ': ' + params.workspace;
		}

		// setup editor content, only after loading
		if (params.content != null) {
			setContent(content(params.content || ''), getFile(), getLine(1));
		} else {
			showPosition();
		}
		return;
	}

	if (changes.theme !== undefined) {
		setStyle(document.body, '-dark', '-light', params.theme);
	}

	if (changes.useWebWorker !== undefined) {
		// reload page
		params.update(true);
		return;
	}

	// reload page if the project was modified
	if (changes.project !== undefined) {
		if (changes.project != params.project) {
			// page needs to be reloaded to clean up old files
			// keep the workspace clean, only one project
			params.update(true);
			return;
		}
		if (changes.workspace === undefined) {
			changes.workspace = true;
		}
	}

	if (changes.workspace !== undefined) {
		if (changes.workspace != params.workspace) {
			// page needs to be reloaded to clean up old files
			// mounting one folder second time will fail
			params.update(true);
			return;
		}
		openProjectFile({
			workspace: params.workspace,
			project: project(params.project),
			content: content(params.content),
			path: getPath()
		});
		if (params.workspace) {
			params.project = null;
			params.content = null;
			params.path = null;
			return true;
		}
		return;
	}

	if (changes.path !== undefined) {
		if (params.path == null && params.content == null) {
			// close the current file
			setContent('', '');
			return;
		}
		openProjectFile({
			path: getPath()
		});
		return;
	}

	if (changes.content !== undefined) {
		setContent(content(params.content), getFile(), getLine(1));
	}
});

function rmWorkspace(workspace) {
	if (!confirm('Remove workspace: ' + workspace)) {
		return;
	}
	if (!workspace.startsWith('/workspace-')) {
		workspace = '/workspace-' + workspace;
	}
	var req = indexedDB.deleteDatabase(workspace);
	req.onsuccess = function(event) {
		if (event.oldVersion === 0) {
			return;
		}
		params.update(true);
	}
	req.onerror = console.log;
	req.onblocked = console.log;
}

// usually content changed, show save icon
editor.on('change', function(cm, change) {
	setStyle(document.body, 'edited');
});
// show position and selection in document
editor.on('cursorActivity', showPosition);
// add / remove breakpoint
editor.on('gutterClick', function(cm, n) {
	let info = cm.lineInfo(n);
	if (info.gutterMarkers == null) {
		let marker = document.createElement('div');
		marker.classList.add('breakpoint');
		marker.innerHTML = '●';
		marker.value = n + 1;
		cm.setGutterMarker(n, 'breakpoints', marker);
	} else {
		cm.setGutterMarker(n, 'breakpoints', null);
	}
});
// notify main window iframing the editor to resize it's height
editor.on('viewportChange', function(cm, from, to) {
	let body = document.body;
	let html = document.documentElement;
	let height = Math.max(body.scrollHeight
		, body.offsetHeight, html.clientHeight
		, html.scrollHeight, html.offsetHeight
	);
	window.parent.postMessage({frameHeight: height}, '*');
});

window.onkeydown = function(event) {
	// escape to editor
	if (event.key === 'Escape') {
		setStyle(document.body, '-right-bar');
		if (editor.hasFocus()) {
			setStyle(document.body, '~output');
		}
		if (!document.body.classList.contains('canvas')) {
			editor.setSelection(editor.getCursor());
			editor.focus();
		}
		return false;
	}
	// Ctrl|Cmd + Enter => Focus command: [search, jump, fold, run, ...]
	if ((event.ctrlKey|event.metaKey) && !event.altKey && !event.shiftKey && event.key === 'Enter') {
		completeAction();
		return false;
	}
	// Ctrl|Cmd + Shift + Enter => Execute script
	if ((event.ctrlKey|event.metaKey) && !event.altKey && event.shiftKey && event.key === 'Enter') {
		onExecClick();
		return false;
	}
}

function setContent(content, file, line, column) {
	let contentSet = false;
	if (content != null && content != editor.getValue()) {
		editor.setValue(content);
		editor.clearHistory();
		setStyle(document.body, '-edited');
		contentSet = true;
	}
	if (file != null) {
		edtFileName.value = file;
		for (let li of fileList.children) {
			setStyle(li, li.innerText === file ? 'active' : '-active');
		}
	}
	if (line != null) {
		var middleHeight = editor.getScrollerElement().offsetHeight / 2;
		var t = editor.charCoords({line, ch: 0}, 'local').top;
		editor.scrollTo(null, t - middleHeight - 5);
		editor.setCursor((line || 1) - 1, column);
	}
	return contentSet;
}

function showPosition() {
	let pos = editor.getCursor();
	let sel = editor.listSelections();
	if (sel.length !== 1) {
		// multiple selections
		editPosition.innerText = (pos.line+1) + ':' + (pos.ch + 1) +
			' [' + (sel.length) + ' selections]';
		return;
	}
	if (sel[0].anchor !== sel[0].head) {
		// single selection show selected [lines:characters]
		let head = sel[0].head;
		let anchor = sel[0].anchor;
		if (head.line === anchor.line) {
			if (head.ch < anchor.ch) {
				let tmp = head;
				head = anchor;
				anchor = tmp;
			}
		}
		else if (head.line < anchor.line) {
			let tmp = head;
			head = anchor;
			anchor = tmp;
		}
		let lines = head.line - anchor.line + 1;
		let chars = editor.getRange(anchor, head).length;
		editPosition.innerText = (pos.line+1) + ':' + (pos.ch + 1) +
			' [' + (lines) + ':' + (chars) + ']';
		return;
	}

	// no selection show document [lines:characters]
	let lines = 0;
	let chars = 0;
	editor.doc.iter(function(line) {
		chars += line.text.length;
		lines += 1;
	});
	editPosition.innerText = (pos.line+1) + ':' + (pos.ch + 1) +
		' [' + (lines) + ':' + (chars) + ']';
}

function hideOverlay() {
	if (!document.body.classList.contains('left-pin')) {
		document.body.classList.remove('left-bar');
	}
	if (!document.body.classList.contains('right-pin')) {
		document.body.classList.remove('right-bar');
	}
	if (!document.body.classList.contains('bottom-pin')) {
		document.body.classList.remove('bottom-bar');
	}
}

function saveInput(saveAs) {
	let path = params.getPath();
	if (path == null || saveAs === true) {
		path = prompt('Save file as:', path || 'untitled.ci');
		if (path == null) {
			return null;
		}
	}
	let content = editor.getValue();
	openProjectFile({ content, path });
	setStyle(document.body, '-edited');
	if (params.content !== undefined) {
		let oldValue = params.content;
		let encB64 = true;
		try {
			oldValue = atob(oldValue);
		} catch (err) {
			encB64 = false;
			console.debug(err);
		}
		if (oldValue !== content) {
			params.update({ content: encB64 ? btoa(content) : content });
		} else {
			params.update({ path });
		}
	} else {
		params.update({ path });
	}
	return path;
}

function editProject() {
	if (params.project != null) {
		let files = [];
		let content = params.project;

		try {
			if (content != null) {
				content = atob(content);
			}
		} catch (e) {
			console.warn(e);
		}

		if (content.startsWith('[{')) {
			files = JSON.parse(content);
		} else {
			for (let url of content.split(';')) {
				files.push({ url });
			}
		}
		content = '';
		for (let file of files) {
			if (content !== '') {
				content += ', ';
			}
			content += JSON.stringify(file, null, '\t');
		}
		content = '[' + content + ']';
		params.update({ content: btoa(content), project: undefined, path: undefined });
	}
}

function selectOutputTab(select) {
	let item = select.options[select.selectedIndex];
	if (item.onchange != null) {
		if (item.keepSelected !== true) {
			select.selectedIndex = 0;
		}
		item.onchange();
	}
}

function pinOutput(pinOption) {
	let newOption = document.createElement('option');
	newOption.value = terminal.innerHtml();
	pinOption.value = +(+pinOption.value || 0) + 1;
	newOption.innerText = 'Output-' + pinOption.value;
	newOption.keepSelected = true;
	newOption.onchange = function () {
		terminal.innerHtml(newOption.value);
	}
	selOutput.insertBefore(newOption, pinOption);
	selOutput.selectedIndex = Array.prototype.indexOf.call(selOutput.children, newOption);
}

function editOutput() {
	params.update({ content: null, path: null });
	setContent(terminal.text());
	if (props.mobile) {
		setStyle(document.body, '-output');
	}
}

function shareInput() {
	setStyle(document.body, 'output');
	let content = editor.getValue();
	terminal.append('decoded uri: ' + decodeURIComponent(window.location));
	let hash = 'content=' + btoa(editor.getValue());
	if (params.path != null) {
		hash = 'path=' + params.path + '&' + hash;
	}
	if (params.exec != null) {
		hash = 'exec=' + params.exec + '&' + hash;
	}
	if (params.libFile != null) {
		hash = 'libFile&' + hash;
	}
	if (params.libGfx != null) {
		hash = 'libGfx&' + hash;
	}
	terminal.append('current file: ' + window.location.origin + window.location.pathname + '#' + hash);
	if (content.startsWith('[{')) {
		terminal.append('project file: ' + window.location.origin + window.location.pathname + '#project=' + btoa(content));
	}
}

function execute(cmd, args) {
	let execArgs = undefined;
	if (typeof(args) !== 'object') {
		args = {
			file: args !== false,
			libs: args !== false,
			prms: args !== false,
			dump: false
		}
	}

	if (cmd != null) {

		execArgs = [];
		if (args.prms !== false) {
			let experimentalFlags = params.X;
			if (experimentalFlags == null) {
				experimentalFlags = '-stdin';
				if (!hasStyle('preferNativeCalls', 'checked')) {
					experimentalFlags += '-native';
				}
			}
			// do not use standard input
			execArgs.push('-X' + experimentalFlags);
		}

		for (let arg of cmd.split(' ')) {
			if (arg === '') {
				continue;
			}
			execArgs.push(arg);
		}

		if (args.dump !== false && args.dump != null) {
			if (args.dump.constructor !== String) {
				throw 'dump must be a string';
			}
			if (args.dump.endsWith('.json')) {
				execArgs.push('-dump.json');
			} else {
				execArgs.push('-dump');
			}
			execArgs.push(args.dump);
		}

		if (args.libs !== false) {
			if (params.libFile != null) {
				execArgs.push('libFile.wasm');
			}
			if (params.libGfx != null) {
				execArgs.push('libGfx.wasm');
			}
		}

		if (args.file !== false) {
			args.file = saveInput();
			if (args.file == null) {
				return;
			}

			execArgs.push(args.file);
			for (let ln of document.getElementsByClassName('breakpoint')) {
				execArgs.push('-b/P/' + ln.value);
			}
		}

		terminal.command = execArgs.join(' ');
	}

	terminal.clear();
	selOutput.selectedIndex = 0;
	execInput(execArgs || ['--help'], args);
}

function process(data) {
	if (data == null) {
		return;
	}
	if (data.error !== undefined) {
		terminal.append(data.error);
		actionError();
	}
	if (data.print !== undefined) {
		terminal.append(data.print);
	}
	if (data.link !== undefined) {
		const link = document.createElement('a');
		link.innerText = data.path || 'Download file';
		link.download = data.path || 'file.bin';
		link.href = data.link;
		const line = document.createElement('p');
		line.textContent = 'Download link: '
		line.appendChild(link);
		terminal.append(line);
		link.click();
		return;
	}
	if (data.list !== undefined) {
		fileList.innerHTML = '';
		for (let path of data.list) {
			const li = document.createElement('li');
			li.innerText = path;
			if (path.endsWith('/')) {
				li.classList.add('folder');
				li.onclick = function() {
					// update params to preserve history navigation
					params.update({ path });
				};
			} else {
				li.classList.add('file');
				li.onclick = function() {
					// update params to preserve history navigation
					params.update({ path, content: null });
				};
				li.ondblclick = function() {
					openProjectFile({ path, link: true });
				};
			}
			if (path === params.getPath()) {
				li.classList.add('active');
			}
			fileList.appendChild(li);
		}
	}

	if (data.content || data.path) {
		let path = data.path;
		let line = params.getLine();
		if (!isNaN(line)) {
			path += ':' + line;
		}
		if (setContent(data.content, data.path, params.getLine(1))) {
			params.update({ path, content: params.content ? data.content : null });
		} else {
			params.update({ path });
		}
	}
	if (data.initialized) {
		spnStatus.innerText = null;
		params.update('workspace', 'project', 'content', 'path');
	}
	if (data.progress !== undefined) {
		if (data.progress > 0) {
			spnStatus.innerText = 'Downloading files: ' + data.progress + ' / ' + data.total;
			return;
		}
		spnStatus.innerText = null;
		params.update('content', 'path');
	}
}

function uploadFiles(input) {
	for (let file of input.files) {
		const reader = new FileReader();
		reader.addEventListener("load", function () {
			let path = params.getPath('');
			let match = path.match(pathMather);
			if (match && match[2]) {
				path = match[2];
			} else {
				path = '';
			}
			path += file.name;
			openProjectFile({
				path: path,
				reopen: path === params.getPath(),
				content: new Uint8Array(reader.result)
			});
		}, false);
		reader.readAsArrayBuffer(file);
	}
	input.value = null;
}

// allow sidebar resizing
if (window.addEventListener) {
	function dragStart(e) {
		e.preventDefault();
		props.dragging = e;
	}
	function dragStop() {
		if (props.dragging == null) {
			return;
		}
		props.dragging = null;
		editor.refresh();
	}
	function dragMove(event) {
		let evDown = props.dragging;
		if (evDown == null) {
			return;
		}

		if (evDown.target.id === 'output-pan-sidebar') {
			if (!hasStyle(document.body, 'output')) {
				return;
			}
			let y = event.pageY - evDown.offsetY;
			if (y !== y && event.touches !== undefined) {
				y = event.touches[0].pageY;
			}
			document.documentElement.style.setProperty('--bottom-bar-size', (window.innerHeight - y) + "px");
			return;
		}
		if (evDown.target.id === 'left-pan-sidebar') {
			let x = event.pageX;
			if (event.pageX === undefined) {
				x = event.touches[0].pageX;
			}
			var percentage = (x + 5) / window.innerWidth * 100;
			if (percentage < 10 || percentage > 90) {
				return;
			}
			document.documentElement.style.setProperty('--left-bar-size', percentage + "%");
			return;
		}
		console.log(event);
	}

	document.getElementById("output-pan-sidebar").addEventListener("mousedown", dragStart);
	document.getElementById("output-pan-sidebar").addEventListener("touchstart", dragStart);
	document.getElementById("left-pan-sidebar").addEventListener("mousedown", dragStart);
	document.getElementById("left-pan-sidebar").addEventListener("touchstart", dragStart);
	window.addEventListener("mousemove", dragMove);
	window.addEventListener("touchmove", dragMove);
	window.addEventListener("mouseup", dragStop);
	window.addEventListener("touchend", dragStop);
}

// allow drag and drop files
document.ondragover = function (e) {
	return false;
};
document.ondragend = function (e) {
	return false;
};
document.ondrop = function (e) {
	uploadFiles(e.dataTransfer);
	return false;
};
