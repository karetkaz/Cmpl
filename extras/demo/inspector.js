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

        for (let i = 0; i < data.functions.length; ++i) {
            let symbol = data.functions[i];
            if (symbol.proto == null) {
                symbol.proto = symbol[''];
            }
            functions[symbol.offs] = symbol;
        }

        let lastTick = 0;
        for (let i = 0; i < data.callTree.length; i += recSize) {
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
        samples,
        ticks: samples && samples.leave - samples.enter,
        ticksPerSec: data && data.ticksPerSec,
        createCallTree: function displayCallTree(treeDiv, sortList) {
            let data2 = {
                ...result,
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
        },
        createSymbolList: function(parentNode) {
            parentNode.replaceChildren();
            for (let sym of data.symbols || []) {
                if (!sym.name) {
                    continue;
                }
                parentNode.appendChild(createSymbolRow(sym, result));
            }
        },
        createSymbolLinks: null, // 'window.location.href="#${file}:${line}";',
        createTraceEventsUrl: function() {
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
                    try {
                        printCall(child);
                    } catch (e) {
                        // ignore stack overflows
                    }
                }
            }

            if (samples == null) {
                return null;
            }

            printCall(samples.callTree[0]);
            let blob = new Blob(["{\"traceEvents\":[" + value + "]}"], {type: 'application/octet-stream'});
            return URL.createObjectURL(blob);
        }
    }
    return result;
}

function createSymbolRow(sym, data) {
    let row = document.createElement('ul');
    row.classList.add('expandable');

    function addToList(classAttribute, term, def) {
        let listNode = row.appendChild(document.createElement('li'));
        listNode.setAttribute('class', classAttribute);

        let termNode = null;
        if (term != null) {
            termNode = listNode.appendChild(document.createElement('span'));
            termNode.setAttribute('class', 'term');
            if (term instanceof HTMLElement) {
                termNode.appendChild(term);
            } else {
                termNode.appendChild(document.createTextNode(term));
            }
        }

        let defNode = null;
        if (def != null) {
            defNode = listNode.appendChild(document.createElement('span'));
            if (def instanceof HTMLElement || def instanceof DocumentFragment) {
                defNode.appendChild(def);
            } else {
                defNode.appendChild(document.createTextNode(def));
            }
        }
        return defNode;
    }

    let func = document.createDocumentFragment();
    let expand = func.appendChild(document.createElement('span'));
    expand.setAttribute('class', 'expander');
    expand.onclick = function () {
        if (row.classList.contains('expanded')) {
            row.classList.remove('expanded');
        } else {
            row.classList.add('expanded');
        }
    }

    if (data.createSymbolLinks && sym.file && sym.line) {
        let proto = func.appendChild(document.createElement('a'));
        proto.appendChild(document.createTextNode(sym['']));
        proto.href = data.createSymbolLinks
            .replaceAll('${file}', sym.file)
            .replaceAll('${line}', sym.line)
    } else {
        func.appendChild(document.createTextNode(sym['']));
    }
    addToList('description', null, func);

    // sym.doc: "Returns the pointer incremented with the given value"
    if (sym.doc) {
        let def = addToList('documentation', 'Documentation', '');
        def.innerHTML = sym.doc.trim().replace(/\n/g, '<br>');
    }

    if (sym.args) {
        let params = sym.args.length;
        /* todo list parameters
        let params = document.createElement('ol');
        for (let param of sym.args) {
            params.appendChild(createSymbolRow(param));
        }*/
        addToList('parameters', 'Parameters', params);
    }

    let kind = '';
    if (sym.static) {
        kind += 'static ';
    }
    if (sym.const) {
        kind += 'const ';
    }
    kind += sym.kind;
    if (sym.cast != null && sym.cast !== 'inline') {
        kind += ' -> ' + sym.cast;
    }

    addToList('kind', 'Kind', kind);
    addToList('type', 'Type', sym.type);
    addToList('size', 'Size', sym.size);
    addToList('offs', 'Offset', sym.offs);

    if (sym.asm) {
        addToList('code', 'Instructions', sym.asm.length);
        /* todo list instructions
        for (let instruction of sym.asm) {
            code.appendChild(document.createTextNode(instruction.instruction+'\n'));
        }*/
    }

    return row;
}

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

        func = func.appendChild(document.createElement("a"));
        func.innerText = 'Call Tree';
        func.href = 'javascript:;';
        func.onclick = () => data.display();
        func.title = "Show the tree of function execution, sorted in the execution order";
        return row;
    }

    const samples = data.samples;
    let sample = samples.samples[node.func.offs] || {};	// make everything undefined if function is not found

    hits.innerText = node.hits;
    self.innerText = toCost(node.self);
    total.innerText = toCost(node.total);
    func.style.paddingLeft = ((depth || 0) + .5) + 'em';

    let title = 'Function: ' + sample.func.name;
    if (sample.func.file && sample.func.line) {
        title += '(' + sample.func.file + ':' + sample.func.line + ')';
    } else {
        title += '<@0x' + sample.func.offs.toString(16) + '>'
    }
    if (sample !== node && sample.hits > 1) {
        title += '\nHit Count: ' + node.hits + ' / ' + sample.hits;
        title += '\nSelf Cost: ' + self.innerText + ' / ' + toCost(sample.self);
        title += '\nSelf Time: ' + toTime(node.self) + ' / ' + toTime(sample.self);
        title += '\nTotal Cost: ' + total.innerText + ' / ' + toCost(sample.total);
        title += '\nTotal Time: ' + toTime(node.total) + ' / ' + toTime(sample.total);
    } else {
        title += '\nHit Count: ' + sample.hits;
        title += '\nSelf Cost: ' + toCost(sample.self);
        title += '\nSelf Time: ' + toTime(sample.self);
        title += '\nTotal Cost: ' + toCost(sample.total);
        title += '\nTotal Time: ' + toTime(sample.total);
    }
    if (sample !== node) {
        title += '\nExecution Time: ' + toTime(node.enter - samples.enter) + ' - ' + toTime(node.leave - samples.enter);
        title += '\nMemory Usage: ' + node.memory;
    }
    row.title = title + '\n' + sample.func.proto;

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
