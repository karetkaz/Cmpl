function Inspector(data, callback) {
    "use strict";

    if (data != null && data.constructor === String) {
        let result = undefined;
        let xhr = new XMLHttpRequest();
        xhr.onload = function() {
            if (!xhr.response || xhr.status < 200 || xhr.status >= 300) {
                throw new Error(xhr.status + ' (' + xhr.statusText + ')');
            }

            result = Inspector(JSON.parse(xhr.response));
            if (callback != null && callback.constructor === Function) {
                callback(result)
            }
        };
        xhr.open('GET', data, callback == null);
        xhr.send(null);
        return result;
    }

    let functions = Object.create(null);
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

    if (data != null) {
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

        let calls = [{
            enter: +Infinity,
            leave: -Infinity,
            samples: {},
            callTree: null,
            //excluded: ['Halt', 'ToDays'],
            ticksPerSec: data.ticksPerSec
        }];

        for (let i = 0; i < data.functions?.length; ++i) {
            let symbol = data.functions[i];
            if (symbol.proto == null) {
                symbol.proto = symbol[''];
            }
            functions[symbol.offs] = symbol;
        }

        let lastTick = 0;
        for (let i = 0; i < data.callTree?.length; i += recSize) {
            let offs = data.callTree[i + funIndex];
            let tick = tickIndex < 0 ? undefined : data.callTree[i + tickIndex];
            let heap = heapIndex < 0 ? undefined : data.callTree[i + heapIndex];

            if (i === 0 && offs === 0 && tick === 0 && heap === 0) {
                // skip the first entry if all values are zero
                continue;
            }

            // if a function executes in zero time we are in trouble displaying, so correct it to 1 unit
            // this happens with old compilers, with low resolution `clock()` function
            if (tick <= lastTick) {
                lastTick += 1;
                tick = lastTick;
            }
            lastTick = tick;
            if (offs !== -1) {
                let callee = enter(calls, offs, tick);
                calls.push(callee);
            }
            else {
                let callee = calls.pop();
                let caller = calls[calls.length - 1];
                leave(calls, caller, callee, tick, heap);
                Object.freeze(callee);
            }
        }

        while (calls.length > 0) {
            samples = calls.pop();
            if (calls.length > 1) {
                let caller = calls[calls.length - 1];
                leave(calls, caller, samples, undefined);
            }
            Object.freeze(samples);
        }
    }

    let result = {
        // json: { functions, data },
        main: data.main,
        samples,
        ticks: samples && samples.leave - samples.enter,
        ticksPerSec: data && data.ticksPerSec,
        walkSymbolList: function(callback) {
            for (let sym of data.symbols || []) {
                if (!sym.name) {
                    continue;
                }
                callback(sym, data);
            }
        },
        createSymbolLinks: null, // 'window.location.href="#${file}:${line}";',
        sourceCodeMap: null, // set this path to download source files,
    }
    return result;
}

function exportMarkdown(button, inspector) {
    let label = button.innerHTML;
    button.innerHTML = 'exporting...';
    try {
        let markdown = '<!-- do not edit, autogenerated from: /extras/docs/*.md -->';

        inspector.walkSymbolList((sym, data) => {
            if (sym.name === '.doc-inline' || sym.name === '.doc-statement') {
                if (sym.doc != null) {
                    markdown += '\n' + sym.doc + '\n';
                }
                if (sym.source != null) {
                    markdown += '\n```cmpl\n';
                    markdown += sym.source;
                    markdown += '```\n';
                }
            }
        });

        let name = (inspector.main?.unit || 'documentation').split('/').pop().replace(/\.cmpl$/, '') + '.md';
        let link = document.createElement('a');
        link.download = name;
        link.href = URL.createObjectURL(new Blob([markdown], { type: 'text/markdown' }));
        link.click();
        URL.revokeObjectURL(link.href);
    } finally {
        button.innerHTML = label;
    }
}

function buildSymbolList(inspector) {
    function symTitle(sym) {
        if (sym.args != null) {
            return sym[''];
        }
        return sym[''] + ': ' + sym.type;
    }

    function createChildElement(parent, tag) {
        let item = document.createElement(tag);
        parent.appendChild(item);
        return item;
    }

    function sourcePath(file) {
        // project files are mapped as `./file.cmpl`, otherwise use the root url
        let path = inspector.sourceCodeMap[file] ?? inspector.sourceCodeMap['./' + file];
        if (path == null) {
            path = inspector.sourceCodeMap['/'] + file;
        }
        return path;
    }

    function appendSourceView(item, sym, path, transform) {
        let code = createChildElement(item, 'textArea');
        fetchSource(path, (response) => {
            let editor = CodeMirror.fromTextArea(code, {
                mode: 'text/x-cmpl',
                lineNumbers: true,
                tabSize: 4,
                indentUnit: 4,
                indentWithTabs: true,

                styleActiveLine: true,
                matchBrackets: true,
                showTrailingSpace: true,
                readOnly: true,
                viewportMargin: Infinity,

                foldGutter: true,
                gutters: ['CodeMirror-linenumbers', 'breakpoints', 'CodeMirror-foldgutter'],
                highlightSelectionMatches: {showToken: /\w/, annotateScrollbar: true}
            });
            let source = transform(response);
            editor.setValue(source);
            editor.setSize('100%', '100%');
            sym.source = source;
        });
    }

    function appendDetailCardSymbol(list, sym) {
        let item = createChildElement(list, 'div');

        let title = createChildElement(item, 'div');
        setStyle(title, "title-node");
        if (inspector.createSymbolLinks && sym.file && sym.line) {
            let href = inspector.createSymbolLinks
            .replaceAll('${file}', sym.file)
            .replaceAll('${line}', sym.line)
            title.innerHTML = '<a href="' + href + '">' + symTitle(sym) + '</a>'
        } else {
            title.innerHTML = '<a href="javascript:;">' + symTitle(sym) + '</a>'
            setStyle(item, 'builtin');
        }

        if (sym.name === '.doc-inline' || sym.name === '.doc-statement') {
            setStyle(item, 'comment');
        }

        if (sym.doc != null) {
            let body = createChildElement(item, 'div');
            setStyle(body, "doc-node");
            body.innerHTML = sym.doc

            if (inspector.sourceCodeMap == null) {
                return item;
            }

            let placeholder = sym.name === '.doc-inline' && sym.ast?.kind === ".literal" && sym.ast.value.match(/^"\*(.*)"$/);
            let example = placeholder && placeholder[1].match(/^example:(tail|\d+)$/);
            if (example) {
                // `inline "*example:tail";` the rest of the file, `inline "*example:<n>";` the next n lines
                let file = sym.ast.file;
                let line = sym.ast.line;
                let count = example[1] === 'tail' ? undefined : line + Number(example[1]);
                appendSourceView(item, sym, sourcePath(file), (text) => {
                    return text.split('\n').slice(line, count).join('\n').replace(/^\s*\n/, '');
                });
                if (inspector.createSymbolLinks) {
                    let href = inspector.createSymbolLinks
                    .replaceAll('${file}', file)
                    .replaceAll('${line}', line + 1)
                    body.innerHTML = '<a href="' + href + '">' + sym.doc + '</a>'
                }
            }
            else if (placeholder) {
                // other placeholders are replaced with generated text only by the compiler
                let note = createChildElement(item, 'div');
                setStyle(note, "doc-node");
                note.innerHTML = '<i>generated by `cmpl -dump.md`: ' + placeholder[1] + '</i>';
            }
            else if (sym.name === '.doc-inline' && sym.doc?.trim() === '**Example**' && sym.ast?.kind === ".literal") {
                let file = sym.ast.value;
                if (file.startsWith('"') && file.endsWith('"')) {
                    file = file.substring(1, file.length - 1);
                }

                let path = inspector.sourceCodeMap[file];
                if (path == null && (file.startsWith('./') || file.startsWith('../'))) {
                    path = inspector.sourceCodeMap['/'];
                    path += sym.file.substring(0, sym.file.lastIndexOf('/') + 1);
                    path += file;
                }

                appendSourceView(item, sym, path, (text) => text);
                if (inspector.createSymbolLinks) {
                    let href = inspector.createSymbolLinks
                    .replaceAll('${file}', file.replace(/^\.\//, ''))
                    .replaceAll('${line}', '')
                    body.innerHTML = '<a href="' + href + '">' + sym.doc + '</a>'
                }
            }
        } else {
            setStyle(item, "private");
        }

        return item;
    }

    function appendDetailCardDetails(list, sym) {
        let item = createChildElement(list, 'div');
        setStyle(item, "details");
        for (let atr in sym) {
            if (atr === 'args') continue;
            if (atr === 'doc') continue;
            if (atr === 'ast') continue;
            if (atr === 'asm') continue;
            if (atr === '') continue;
            item.innerHTML += '<br><span class="key">' + atr + '</span>: <span>' + sym[atr] + '</span>'
        }

        return item;
    }

    function appendDetailCardArguments(list, sym) {
        if (sym.args == null) {
            return;
        }
        for (let arg of sym.args) {
            let item = appendDetailCardSymbol(list, arg);
            appendDetailCardDetails(item, arg);
            setStyle(item, "argument");
        }
    }

    function addHeader(list) {
        let head = createChildElement(list, 'div');
        setStyle(head, "filter-text");
        let filter = createChildElement(head, 'input');
        filter.oninput = filter.onkeypress = function(event) {
            if (event.constructor !== InputEvent) {
                if (event.key !== "Enter") {
                    return;
                }
            }

            const val = filter.value.toLowerCase();
            for (node of list.childNodes) {
                if (node.filterText != null && !node.filterText.includes(val)) {
                    setStyle(node, "gone");
                } else {
                    setStyle(node, "-gone");
                }
            }
        };

        let priv = createChildElement(head, 'span');
        setStyle(priv, "filter-private");
        priv.innerHTML = 'private';
        priv.onclick = function() { setStyle(list, "~private"); };

        let libs = createChildElement(head, 'span');
        setStyle(libs, "filter-library");
        libs.innerHTML = 'library';
        libs.onclick = function() { setStyle(list, "~library"); };

        let details = createChildElement(head, 'span');
        setStyle(details, "filter-details");
        details.innerHTML = 'details';
        details.onclick = function() { setStyle(list, "~details"); };

        head.appendChild(document.createTextNode(' '));
        let exportMd = head.appendChild(document.createElement('a'));
        setStyle(exportMd, "filter-export");
        exportMd.innerHTML = '💾';
        exportMd.onclick = function() { exportMarkdown(exportMd, inspector); };
        exportMd.title = 'Export the symbol data in MarkDown file format';
        exportMd.href = 'javascript:;';
    }

    let sourceCache = Object.create(null);
    function fetchSource(path, callback) {
        if (sourceCache[path] != null) {
            callback(sourceCache[path]);
            return;
        }
        let xhr = new XMLHttpRequest();
        xhr.onload = function () {
            if (!xhr.response || xhr.status < 200 || xhr.status >= 300) {
                console.error('failed to fetch: ' + path + ' (' + xhr.status + ')');
                return;
            }
            sourceCache[path] = xhr.response;
            callback(xhr.response);
        };
        xhr.open('GET', path, true);
        xhr.send(null);
    }

    let list = document.createElement('div');
    list.setAttribute('class', 'syms');
    addHeader(list);
    inspector.walkSymbolList((sym, data) => {
        let item = appendDetailCardSymbol(list, sym);
        appendDetailCardDetails(item, sym);
        appendDetailCardArguments(item, sym);
        item.data = sym;
        item.filterText = (symTitle(sym) + '\n' + sym.doc).toLowerCase();
        if (inspector.main && sym.unit !== inspector.main.unit) {
            setStyle(item, "library");
        }
    });
    return list;
}

function exportTraceEvents(button, inspector) {
    let label = button.innerHTML;
    button.innerHTML = 'exporting...';
    try {
        if (inspector?.samples?.callTree == null) {
            return URL.createObjectURL(new Blob([]));
        }

        let exportText = '';
        function us(ticks) { return ticks * 1000000 / inspector.ticksPerSec; }
        function printCall(node) {
            if (exportText !== '') {
                exportText += ",";
            }
            exportText += "{";
            exportText += "\"cat\":\"function\",";
            exportText += "\"dur\":" + us(node.leave - node.enter) + ',';
            exportText += "\"name\":\"" + (node.func || {})[''] + "\",";
            exportText += "\"ph\":\"X\",";
            exportText += "\"pid\":0,";
            exportText += "\"tid\":" + 0 + ",";
            exportText += "\"ts\":" + us(node.enter - inspector.samples.enter);
            exportText += "}\n";
            for (let child of (node.callTree || [])) {
                try {
                    printCall(child);
                } catch (e) {
                    // ignore stack overflows
                }
            }
        }
        printCall(inspector.samples.callTree[0]);

        let name = (inspector.main?.unit || 'documentation').split('/').pop().replace(/\.cmpl$/, '') + '.json';
        let link = document.createElement('a');
        link.download = name;
        link.href = URL.createObjectURL(new Blob(["{\"traceEvents\":[" + exportText + "]}"], {type: 'application/octet-stream'}));
        link.click();
        URL.revokeObjectURL(link.href);
    } finally {
        button.innerHTML = label;
    }
}

function buildCallTree(inspector) {
    function createTreeRow(node, data, depth) {
        function toCost(ticks) { return (ticks * 100 / data.ticks).toFixed(2) + '%'; }
        function toTime(ticks) { return (ticks * 1000 / data.ticksPerSec).toFixed(2) + ' ms'; }

        let row = document.createElement("tr");
        let hits = row.appendChild(document.createElement("td"));
        let self = row.appendChild(document.createElement("td"));
        let total = row.appendChild(document.createElement("td"));
        let func = row.appendChild(document.createElement("td"));
        row.rowData = { row, hits, self, total, func };

        if (node == null) {
            hits = hits.appendChild(document.createElement("a"));
            hits.innerText = 'Hit Count';
            hits.href = 'javascript:;';
            hits.onclick = () => data.display((a, b) => b.hits - a.hits);
            hits.title = "Show all functions sorted in reverse order by execution count";

            self = self.appendChild(document.createElement("a"));
            self.innerText = 'Self Time';
            self.href = 'javascript:;';
            self.onclick = () => data.display((a, b) => b.self - a.self);
            self.title = "Show all functions sorted in reverse order by self execution time (excluding other calls)";

            total = total.appendChild(document.createElement("a"));
            total.innerText = 'Total Time';
            total.href = 'javascript:;';
            total.onclick = () => data.display((a, b) => b.total - a.total);
            total.title = "Show all functions sorted in reverse order by total execution time (including other calls)";

            let calls = func.appendChild(document.createElement("a"));
            calls.innerText = 'Call Tree';
            calls.href = 'javascript:;';
            calls.onclick = () => data.display();
            calls.title = "Show the tree of function execution, sorted in the execution order";

            func.appendChild(document.createTextNode(' '));
            let exportTr = func.appendChild(document.createElement('a'));
            setStyle(exportTr, "filter-export");
            exportTr.innerHTML = '💾';
            exportTr.onclick = function() { exportTraceEvents(exportTr, data); };
            exportTr.title = 'Export the profiler data in chrome inspector compatible format';
            exportTr.href = 'javascript:;';

            return row;
        }

        const samples = data.samples;
        // fall back to the node's own function info if the sample is not found
        let sample = samples.samples[node.func.offs] || { func: node.func || {} };

        hits.innerText = node.hits;
        self.innerText = toCost(node.self);
        total.innerText = toCost(node.total);
        func.style.paddingLeft = ((depth || 0) + .5) + 'em';

        let title = 'Function: ' + sample.func.name;

        let fun = data?.json?.functions[node.func.offs];
        if (sample.func.file && sample.func.line) {
            title += '(' + sample.func.file + ':' + sample.func.line + ')';
        } else {
            title += '<@0x' + (sample.func.offs || 0).toString(16) + '>'
        }
        if (sample !== node && sample.hits > 1) {
            title += '\nHit Count: ' + node.hits + ' / ' + sample.hits;
            if (fun != null && sample.hits !== fun.hits) {
                console.log("hits", sample.hits, fun.hits, sample.func.proto);
                title += '\n\tDiff: ' + (sample.hits - fun.hits) + ' / ' + fun.hits;
            }

            title += '\nSelf Cost: ' + self.innerText + ' / ' + toCost(sample.self);
            title += '\nSelf Time: ' + toTime(node.self) + ' / ' + toTime(sample.self);
            if (fun != null && sample.self !== fun.time) {
                console.log("time.self", sample.self, fun.time, sample.func.proto);
                title += '\n\tDiff: ' + toTime(sample.self - fun.time) + ' / ' + toTime(fun.time);
            }

            title += '\nTotal Cost: ' + total.innerText + ' / ' + toCost(sample.total);
            title += '\nTotal Time: ' + toTime(node.total) + ' / ' + toTime(sample.total);
            if (fun != null && sample.total !== fun.total) {
                console.log("time.total", sample.total, fun.total, sample.func.proto);
                title += '\n\tDiff: ' + toTime(sample.total - fun.total) + ' / ' + toTime(fun.total);
            }
        } else {
            title += '\nHit Count: ' + sample.hits;
            if (fun != null && sample.hits !== fun.hits) {
                console.log("hits", sample.hits, fun.hits, sample.func.proto);
                title += '\n\tDiff: ' + (sample.hits - fun.hits) + ' / ' + fun.hits;
            }

            title += '\nSelf Cost: ' + toCost(sample.self);
            title += '\nSelf Time: ' + toTime(sample.self);
            if (fun != null && sample.self !== fun.time) {
                console.log("time.self", sample.self, fun.time, sample.func.proto);
                title += '\n\tDiff: ' + toTime(sample.self - fun.time) + ' / ' + toTime(fun.time);
            }

            title += '\nTotal Cost: ' + toCost(sample.total);
            title += '\nTotal Time: ' + toTime(sample.total);
            if (fun != null && sample.total !== fun.total) {
                console.log("time.total", sample.total, fun.total, sample.func.proto);
                title += '\n\tDiff: ' + toTime(sample.total - fun.total) + ' / ' + toTime(fun.total);
            }
        }
        if (sample !== node) {
            title += '\nExecution Time: ' + toTime(node.enter - samples.enter) + ' - ' + toTime(node.leave - samples.enter);
            title += '\nMemory Usage: ' + node.memory;
        }
        row.title = title;

        let expand = func.appendChild(document.createElement('span'));
        expand.setAttribute('class', 'expander');

        if (data.createSymbolLinks && sample.func.file && sample.func.line) {
            let proto = func.appendChild(document.createElement('a'));
            proto.appendChild(document.createTextNode(sample.func.proto));
            proto.href = data.createSymbolLinks
            .replaceAll('${file}', sample.func.file)
            .replaceAll('${line}', sample.func.line)
        } else {
            func.appendChild(document.createTextNode(sample.func.proto));
        }

        if (node.callTree == null) {
            // row is not expandable
            return row;
        }

        row.classList.add('expandable');
        expand.onclick = function () {
            row.rowData.children = [];
            let parentNode = row.parentNode;
            let nextSibling = row.nextSibling;
            for (let sub of node.callTree) {
                let r = createTreeRow(sub, data, (depth || 0) + 1);
                parentNode.insertBefore(r, nextSibling);
                row.rowData.children.push(r.rowData);
            }
            row.rowData.expandTree = expandTree;
            expand.onclick = collapseTree;
            row.classList.add('expanded');

            function collapseTree() {
                row.rowData.expandTree = null;
                row.classList.remove('expanded');
                expand.onclick = expandTree;
                for (let sub = row.nextSibling; sub !== nextSibling; sub = sub.nextSibling) {
                    sub.classList.remove('gone');
                    sub.classList.add('gone');
                }
            }
            function expandTree() {
                row.rowData.expandTree = expandTree;
                expand.onclick = collapseTree;
                row.classList.add('expanded');
                for (let sub of row.rowData.children) {
                    sub.row.classList.remove('gone');
                    if (sub.expandTree != null) {
                        sub.expandTree();
                    }
                }
            }
        }
        return row;
    }

    function displayCallTree(treeDiv, sortList) {
        if (inspector?.samples?.callTree == null) {
            return;
        }

        let samples = inspector.samples
        let data2 = {
            ...inspector,
            display: function(sorting) {displayCallTree(treeDiv, sorting)}
        }

        treeDiv.replaceChildren(createTreeRow(null, data2));
        if (sortList != null) {
            let array = Object.values(samples.samples);
            array.sort(sortList);
            for (let sample of array) {
                treeDiv.appendChild(createTreeRow(sample, data2));
            }
            return;
        }

        for (let node of samples.callTree) {
            treeDiv.appendChild(createTreeRow(node, data2));
        }
    }

    let tbl = document.createElement('table');
    tbl.setAttribute('class', 'tree');
    displayCallTree(tbl.appendChild(document.createElement('tbody')));
    return tbl;
}