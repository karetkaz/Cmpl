var data = Inspector();

function Inspector(data) {
	"use strict";

	let functions = Object.create(null);	// empty object without prototype.
	let samples = null;

	function enter(calls, id, tick) {
		let call = {
			hits: 1,
			self: 0,
			total: null,
			enter: tick,
			leave: null,
			memory: null,
			caller: null,
			callTree: null,
			func: functions[id] || {}
		};

		let samples = calls[0];

		// update global execution time
		if (samples.enter != null && samples.enter > tick) {
			samples.enter = tick;
		}

		if (samples.samples != null) {
			if (functions[id] == null) {
				console.log('invalid offset: ' + id);
			}
			let sample = samples.samples[id] || (samples.samples[id] = {
					func: functions[id] || {},
					hits: 0,
					self: 0,
					total: 0,
					get others() {
						return this.total - this.self;
					},
					callers: {},
					callees: {}
				});
			if (sample.hits != null) {
				sample.hits += 1;
			}
		}
		return call;
	}

	function leave(calls, caller, callee, tick, memory) {
		let j, recursive = false;
		let time = tick - callee.enter;
		let calleeId = callee.func.offs;
		let callerId = caller.func && caller.func.offs;

		for (j = 1; j < calls.length; ++j) {
			if (calleeId === calls[j].func.offs) {
				if (calls[j].hits != null) {
					calls[j].hits += 1;
				}
				recursive = true;
			}
		}

		if (callee.caller !== null ||
			callee.memory !== null ||
			callee.leave !== null ||
			callee.total !== null
		) {
			throw "invalid state"
		}
		callee.leave = tick;
		callee.caller = caller;
		callee.memory = memory;
		callee.total = time;

		if (callee.self != null) {
			callee.self = callee.total;
			if (callee.callTree != null) {
				for (j = 0; j < callee.callTree.length; ++j) {
					callee.self -= callee.callTree[j].total;
				}
			}
		}
		let samples = calls[0];
		if (samples.excluded == null || samples.excluded.indexOf(callee.name) < 0) {
			caller.callTree = (caller.callTree || []);
			caller.callTree.push(callee);
		}

		// update global execution time
		if (samples.leave != null && samples.leave < tick) {
			samples.leave = tick;
		}

		// compute global information
		if (samples.samples != null) {
			let sample = samples.samples[calleeId] || {};
			// update time spent in function
			if (sample.total != null && !recursive) {
				sample.total += time;
			}
			if (sample.self != null) {
				sample.self += time;
			}

			// update list of functions which have calls to this function
			if (callerId != null) {
				if (sample.callers != null) {
					sample.callers[callerId] = (sample.callers[callerId] || 0) + 1;
				}

				sample = samples.samples[callerId];
				// update time spent in function
				if (sample.self != null) {
					sample.self -= time;
				}

				// update list of functions invoked from this function
				if (sample.callees != null) {
					sample.callees[calleeId] = (sample.callees[calleeId] || 0) + 1;
				}
			}
		}
	}

	function buildCallTree() {
		let recSize = 2;
		let funIndex = 1;
		let tickIndex = 0;
		let heapIndex = -1;

		if (data.callTreeData != null) {
			recSize = data.callTreeData.length;
			funIndex = data.callTreeData.indexOf("ctFunIndex");
			tickIndex = data.callTreeData.indexOf("ctTickIndex");
			heapIndex = data.callTreeData.indexOf("ctHeapIndex");
			if (!(recSize > 0)) {
				throw "callTreeData header is invalid";
			}
		}

		let i, caller, callee, calls = [{
			enter: +Infinity,
			leave: -Infinity,
			samples: {},
			callTree: null,
			//excluded: ['Halt', 'ToDays'],
			ticksPerSec: data.ticksPerSec
		}];

		for (i = 0; i < data.functions.length; ++i) {
			let symbol = data.functions[i];
			if (symbol.proto == null) {
				symbol.proto = symbol[''];
			}
			functions[symbol.offs] = symbol;
		}

		var lastTick = 0;
		for (i = 0; i < data.callTree.length; i += recSize) {
			let offs = data.callTree[i + funIndex];
			let tick = tickIndex < 0 ? undefined : data.callTree[i + tickIndex];
			let heap = heapIndex < 0 ? undefined : data.callTree[i + heapIndex];

			// if a function executes in zero time we are in trouble displaying, so correct it to 1 unit
			// this happens with old compilers, with low resolution `clock()` function
			if (tick <= lastTick) {
				lastTick += 1;
				tick = lastTick;
			}
			lastTick = tick;
			if (offs !== -1) {
				callee = enter(calls, offs, tick);
				calls.push(callee);
			}
			else {
				callee = calls.pop();
				caller = calls[calls.length - 1];
				leave(calls, caller, callee, tick, heap);
				Object.freeze(callee);
			}
		}

		while (calls.length > 0) {
			samples = calls.pop();
			if (calls.length > 1) {
				caller = calls[calls.length - 1];
				leave(calls, caller, samples, undefined);
			}
			Object.freeze(samples);
		}
	}

	function getFiltered(checkSymbol) {
		let result = '';
		if (data == null) {
			return 'No data available!';
		}

		function symToString(sym, qual, type, doc) {
			let result = '';
			if (sym.owner && qual === true) {
				result += sym.owner + '.';
			}
			result += sym.name;
			if (sym.args !== undefined) {
				result += '(';
				for (let i = 0; i < sym.args.length; i += 1) {
					if (i > 0) {
						result += ', ';
					}
					result += symToString(sym.args[i], false, true, doc);
				}
				result += ')';
			}
			if (type === true) {
				result += ": " + sym.type;
			}
			if (doc === true && sym.doc != null) {
				result += "; " + sym.doc.trim().replace(/\n/g, '\\n');
			}
			return result;
		}

		let api = data.symbols || [];
		for (let i = 0; i < api.length; i += 1) {
			let sym = api[i];
			if (!checkSymbol(sym)) {
				continue;
			}
			result += symToString(sym, true, true, true);
			result += '\n';
		}
		return result;
	}

	if (data != null) {
		buildCallTree();
	}

	return {
		getSymbols: function(types, funcs, vars, defs) {
			return getFiltered(function (sym) {
				if (types && sym.kind === 'typename') {
					return true;
				}
				if (funcs && sym.kind === 'function') {
					return true;
				}
				if (vars && sym.kind === 'variable') {
					return true;
				}
				if (defs && sym.kind === 'inline') {
					return true;
				}
				//console.log("symbol was filtered: " + sym.proto);
				console.log("symbol[" + sym.kind + "] was filtered: " + sym.proto);
				return false;
			});
		},
		samples,
		ticks: samples == null ? 0 : samples.leave - samples.enter,
		ticksPerSec: data && data.ticksPerSec
	}
}

function exportChromeTracing() {
	let value = '';
	function ms(ticks) { return ticks * 1000 / data.ticksPerSec; }
	function printCall(node) {
		if (value !== '') {
			value += ",";
		}
		value += "{";
		value += "\"cat\":\"function\",";
		value += "\"dur\":" + ms(node.leave - node.enter) + ',';
		value += "\"name\":\"" + (node.func || {})[''] + "\",";
		value += "\"ph\":\"X\",";
		value += "\"pid\":0,";
		value += "\"tid\":" + 0 + ",";
		value += "\"ts\":" + ms(node.enter);
		value += "}\n";
		for (let child of (node.callTree || [])) {
			printCall(child);
		}
	}

	if (data.samples == null) {
		setActiveTab(null, null, document.getElementById("empty"));
		return;
	}

	printCall(data.samples.callTree[0]);

	const blob = new Blob(["{\"traceEvents\":[" + value + "]}"], {type: 'application/octet-stream'});
	const link = document.createElement('a');
	link.download = 'ChromeTracing.json';
	link.href = URL.createObjectURL(blob);
	link.click();
}

function loadFile(input) {
	let file = null;

	function onFinished(error, fileName, fileData) {
		if (error != null) {
			throw error;
		}
		try {
			let fileElement = document.getElementById('file_name');
			fileElement.innerText = fileName;
			data = Inspector(JSON.parse(fileData));
			// TODO: build the dom here to speed up the inpector
			setActiveTab(null, null, document.getElementById("loaded"));
		}
		catch (err) {
			setActiveTab(null, null, document.getElementById("error"));
			console.log(err);
			throw err;
		}
	}

	if (input != null && input.constructor === String) {
		let xhr = new XMLHttpRequest();
		xhr.open('GET', input, true);
		xhr.onload = function() {
			let error = undefined;
			if (!xhr.response || xhr.status < 200 || xhr.status >= 300) {
				error = new Error(xhr.status + ' (' + xhr.statusText + ')');
			}
			onFinished(error, input, xhr.response);
		};
		setActiveTab(null, null, document.getElementById("loading"));
		xhr.send(null);
	}
	else if (input.files && input.files.length === 1) {
		file = input.files[0];
		var reader = new FileReader();
		reader.onload = function() {
			onFinished(null, file.name, reader.result);
		};
		setActiveTab(null, null, document.getElementById("loading"));
		reader.readAsText(file);
	}
	else {
		setActiveTab(null, null, document.getElementById("error"));
		throw "Single file required.";
	}
}

function setActiveTab(tab, ext, node, contentWriter) {
	let tabs = document.getElementById('tabs').children;
	for (let element of tabs) {
		if (element === tab) {
			element.classList.add('active');
		}
		else if (element === ext) {
			element.classList.add('active');
		}
		else {
			element.classList.remove('active');
		}
	}
	if (node != null) {
		let elements = document.getElementById('content').children;
		for (let element of elements) {
			if (element === node.parentElement) {
				element.classList.add('active');
			}
			else {
				element.classList.remove('active');
			}
		}
	}
	if (contentWriter != null) {
		let args = [];
		for (let i = 4; i < arguments.length; ++i) {
			args[i - 4] = arguments[i];
		}
		let text = contentWriter.apply(this, args);
		if (text != null && node != null) {
			node.innerText = text;
		}
	}
}

// contentWriter for setActiveTab function
function printSymbols() {
	if (data == null || data.samples == null) {
		setActiveTab(null, null, document.getElementById("empty"));
		return;
	}
	var types = document.getElementById('apiTyp').checked;
	var funcs = document.getElementById('apiFun').checked;
	var vars = document.getElementById('apiVar').checked;
	var defs = document.getElementById('apiDef').checked;
	return data.getSymbols(types, funcs, vars, defs);
}

// build callChart and callTree
function displayCallTree(treeDiv, sortList) {
	function toMs(ticks) {
		// return ticks + ' ticks';
		return (ticks * 1000 / data.ticksPerSec).toFixed(2) + ' ms';
	}

	function createRow(hitsText, selfTimeText, totalTimeText, funcText) {
		let row = document.createElement("tr");

		let hits = document.createElement("td");
		row.appendChild(hits);
		if (hitsText != null) {
			hits.innerText = hitsText;
		}

		let self = document.createElement("td");
		row.appendChild(self);
		if (selfTimeText != null) {
			self.innerText = selfTimeText;
		}

		let total = document.createElement("td");
		row.appendChild(total);
		if (totalTimeText != null) {
			total.innerText = totalTimeText;
		}

		let func = document.createElement("td");
		row.appendChild(func);
		if (funcText != null) {
			func.innerText = funcText;
		}

		return {row, hits, self, total, func};
	}

	function createTreeRow(node, depth) {
		let sample = data.samples.samples[node.func.offs] || {};	// make everything undefined if function is not found

		let callSelfCost = (node.self * 100 / data.ticks).toFixed(2) + '%';
		let callTotalCost = (node.total * 100 / data.ticks).toFixed(2) + '%';

		let row = createRow(node.hits, callSelfCost, callTotalCost);
		row.func.style.paddingLeft = ((depth || 0) + .5) + 'em';

		let sampleSelfCost = (sample.self * 100 / data.ticks).toFixed(2) + '%';
		let sampleTotalCost = (sample.total * 100 / data.ticks).toFixed(2) + '%';
		let title = 'Function: ' + sample.func.name;
		if (sample.func.file && sample.func.line) {
			title += '(' + sample.func.file + ':' + sample.func.line + ')';
		} else {
			title += '<@0x' + sample.func.offs.toString(16) + '>'
		}
		if (sample !== node && sample.hits > 1) {
			title += '\nHit Count: ' + node.hits + ' / ' + sample.hits;
			title += '\nSelf Cost: ' + callSelfCost + ' / ' + sampleSelfCost;
			title += '\nSelf Time: ' + toMs(node.self) + ' / ' + toMs(sample.self);
			title += '\nTotal Cost: ' + callTotalCost + ' / ' + sampleTotalCost;
			title += '\nTotal Time: ' + toMs(node.total) + ' / ' + toMs(sample.total);
		} else {
			title += '\nHit Count: ' + sample.hits;
			title += '\nSelf Cost: ' + sampleSelfCost;
			title += '\nSelf Time: ' + toMs(sample.self);
			title += '\nTotal Cost: ' + sampleTotalCost;
			title += '\nTotal Time: ' + toMs(sample.total);
		}
		if (sample !== node) {
			title += '\nExecution Time: ' + toMs(node.enter - data.samples.enter) + ' - ' + toMs(node.leave - data.samples.enter);
			title += '\nMemory Usage: ' + node.memory;
		}
		title += '\n' + sample.func.proto;
		row.row.title = title;

		if (node.callTree == null) {
			// row is not expandable
			row.func.appendChild(document.createTextNode(sample.func.proto));
			return row;
		}

		let expand = row.func.appendChild(document.createElement("a"));
		expand.appendChild(document.createTextNode(sample.func.proto));
		expand.href = 'javascript:;';

		expand.onclick = function () {
			row.children = [];
			let parentNode = row.row.parentNode;
			let nextSibling = row.row.nextSibling;
			for (let sub of node.callTree) {
				let r = createTreeRow(sub, (depth || 0) + 1);
				parentNode.insertBefore(r.row, nextSibling);
				row.children.push(r);
			}
			row.expandTree = expandTree;
			expand.onclick = collapseTree;

			function collapseTree() {
				row.expandTree = null;
				expand.onclick = expandTree;
				for (let sub = row.row.nextSibling; sub !== nextSibling; sub = sub.nextSibling) {
					sub.classList.remove('gone');
					sub.classList.add('gone');
				}
			}
			function expandTree() {
				row.expandTree = expandTree;
				expand.onclick = collapseTree;
				for (let sub of row.children) {
					sub.row.classList.remove('gone');
					if (sub.expandTree != null) {
						sub.expandTree();
					}
				}
			}
		}
		return row;
	}

	if (data == null || data.samples == null) {
		setActiveTab(null, null, document.getElementById("empty"));
		return;
	}

	let head = createRow('Hit Count', 'Self Time', 'Total Time', 'Call Tree');
	head.func.onclick = () => displayCallTree(treeDiv);
	head.hits.onclick = () => displayCallTree(treeDiv, (a, b) => b.hits - a.hits);
	head.self.onclick = () => displayCallTree(treeDiv, (a, b) => b.self - a.self);
	head.total.onclick = () => displayCallTree(treeDiv, (a, b) => b.total - a.total);
	head.func.title = "Show the tree of function execution, sorted in the execution order"
	head.hits.title = "Show all functions sorted in reverse order by execution count"
	head.self.title = "Show all functions sorted in reverse order by self execution time (excluding other calls)"
	head.total.title = "Show all functions sorted in reverse order by total execution time (including other calls)"
	treeDiv.replaceChildren(head.row);

	if (sortList != null) {
		let array = Object.values(data.samples.samples);
		array.sort(sortList);
		for (let sample of array) {
			treeDiv.appendChild(createTreeRow(sample).row);
		}
		return;
	}

	for (let node of data.samples.callTree) {
		treeDiv.appendChild(createTreeRow(node).row);
	}
}
