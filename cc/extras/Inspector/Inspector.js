function Instrument() {

	var data = null;

	var samples = null;

	function enter(calls, functions, id, tick) {
		var call = {
			hits: 1,
			self: 0,
			total: 0,
			enter: tick,
			leave: -1,
			caller: null,
			calltree: null,
			//offs: id,
			func: functions[id],
		};

		var samples = calls[0];

		// update global execution time
		if (samples.enter != null && samples.enter > tick) {
			samples.enter = tick;
		}

		if (samples.samples != null) {
			if (functions[id] == null) {
				console.log('invalid offset: ' + id);
			}
			var sample = samples.samples[id] || (samples.samples[id] = {
				func: functions[id],
				hits: 0,
				self: 0,
				total: 0,
				get others() {
					return this.total - this.self;
				},
				callers: {},
				callees: {},
			});
			if (sample.hits != null) {
				sample.hits += 1;
			}
		}
		return call;
	}

	function leave(calls, caller, callee, tick) {
		var recursive = false;
		var time = tick - callee.enter;
		var calleeId = callee.func.offs;
		var callerId = caller.func && caller.func.offs;

		for (j = 1; j < calls.length; ++j) {
			if (calleeId === calls[j].func.offs) {
				if (calls[j].hits != null) {
					calls[j].hits += 1;
				}
				recursive = true;
			}
		}

		callee.caller = caller;
		if (callee.leave === -1) {
			callee.leave = tick;
		}
		if (callee.total != null) {
			callee.total = time;
		}
		//*
		if (callee.self != null) {
			callee.self = callee.total;
			if (callee.calltree != null) {
				for (j = 0; j < callee.calltree.length; ++j) {
					callee.self -= callee.calltree[j].total;
				}
			}
		}// */
		var samples = calls[0];
		if (samples.excluded == null || samples.excluded.indexOf(callee.name) < 0) {
			caller.calltree = (caller.calltree || []);
			caller.calltree.push(callee);
		}

		// update global execution time
		if (samples.leave != null && samples.leave < tick) {
			samples.leave = tick;
		}

		// compute global information
		if (samples.samples != null) {
			var sample = samples.samples[calleeId];
			// update time spent in function
			if (sample.total != null && !recursive) {
				sample.total += time;
			}
			if (sample.self != null) {
				sample.self += time;
			}

			// update list of functions wich have calls to this function
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

	function getSamples() {
		return samples;
	}

	function getFiltered(checkSymbol) {
		var result = '';
		if (data == null) {
			return 'No data available!';
		}

		function symToString(sym, qual, type) {
			var result = '';
			if (sym.declaredIn && qual === true) {
				result += sym.declaredIn + '.';
			}
			result += sym.name;
			if (sym.args !== undefined) {
				result += '(';
				if (!(sym.args.length === 1 && sym.args[0].size == 0)) {
					for (var i = 0; i < sym.args.length; i += 1) {
						if (i > 0) {
							result += ', ';
						}
						result += symToString(sym.args[i], false, true);
					}
				}
				result += ')';
			}
			if (type === true) {
				result += ": " + sym.type;
			}
			return result;
		}

		var api = data.symbols || [];
		for (var i = 0; i < api.length; i += 1) {
			var sym = api[i];
			if (!checkSymbol(sym)) {
				continue;
			}
			result += symToString(sym, true, true);
			result += '\n';
		}
		return result;
	}

    function loadData(dataIn) {
    	if (dataIn.profile == null) {
    		dataIn.profile = {
    			callTree: [],
    			functions: [],
				statements: [], 
				ticksPerSec: -1, 
				functionCount: 0, 
				statementCount: 0
    		};
    	}
        var i, j, calls = [{
            enter: +Infinity,
            leave: -Infinity,
            samples: {},
            calltree: null,
            //excluded: ['Halt', 'ToDays'],
            ticksPerSec: dataIn.profile.ticksPerSec
        }];;

        data = dataIn;
        var functions = Object.create(null);	// empty object no prototype.
        for (var i = 0; i < dataIn.profile.functions.length; ++i) {
            var symbol = dataIn.profile.functions[i];
            functions[symbol.offs] = symbol;
        }

        samples = null;
        for (i = 0; i < dataIn.profile.callTree.length; i += 2) {
            var time = dataIn.profile.callTree[i + 0];
            var offs = dataIn.profile.callTree[i + 1];

            if (offs !== -1) {
                var callee = enter(calls, functions, offs, time);
                calls.push(callee);
            }
            else {
                var callee = calls.pop();
                var caller = calls[calls.length - 1];
                leave(calls, caller, callee, time);
                Object.freeze(callee);
            }
        }

        if (calls.length === 1) {
            samples = calls.pop();
            Object.freeze(samples);
        }
    }

	return {
		api: {
			getAll: function () {
				return getFiltered(function(sym) {
					return true;
				});
			},
			getFiltered: function (types, funcs, vars, defs, opcodes) {
				return getFiltered(function(sym) {
					if (types && sym.kind === '.rec') {
						return true;
					}
					if (funcs && sym.kind === '.ref' && sym.args != null) {
						return true;
					}
					if (vars && sym.kind === '.ref' && sym.args == null) {
						return true;
					}
					if (defs && sym.kind === '.def') {
						return true;
					}
					if (opcodes && sym.kind === 'emit') {
						return true;
					}
					console.log("symbol was filtered: " + sym.proto);
					return false;
				});
			}
		},
		loadFile: function(file, action) {
			var reader = new FileReader();
			reader.onload = function(){
				loadData(JSON.parse(reader.result));
				if (action != null) {
					action();
				}
			};
			reader.readAsText(file);
		},
		getSamples: getSamples,

		// to be removed
		loadData: loadData,
	}
}

function toJsString(obj, prefix) {
	var objects = [];	// prevent recursion
	function toJsStringRecursive(obj, prefix) {
		if (obj === null || obj === undefined) {
			return obj === null ? 'null' : 'undefined';
		}
		if (obj.constructor === Function) {
			//~ return 'function' + obj.toString().match(/\(.*\)/) + ' { ... }';
			return null;
		}
		if (obj.constructor === Object) {
			if (objects.indexOf(obj) >= 0) {
				return null;
			}
			var result = '';
			var prefix2 = '';
			if (prefix === null || prefix === undefined) {
				prefix = '';
			}
			else if (prefix !== '') {
				prefix2 = prefix + '  ';
			}
			objects.push(obj);
			for (var key in obj) {
				var inner = toJsStringRecursive(obj[key], prefix2);
				if (inner !== null) {
					if (result !== '') {
						result += ', ';
					}
					result += prefix2 + key + ': ' + inner;
				}
			}
			if (prefix !== '' && result.indexOf(prefix) == 0) {
				result += prefix;
			}
			return '{' + result + '}';
		}
		if (obj.constructor === Array) {
			var result = '';
			if (prefix === null || prefix === undefined) {
				prefix = '';
			}
			for (var idx = 0; idx < obj.length; idx += 1) {
				var inner = toJsStringRecursive(obj[idx], prefix);
				if (inner !== null) {
					if (result !== '') {
						result += ', ';
					}
					result += inner;
				}
			}
			if (prefix !== '' && result.indexOf(prefix) == 0) {
				result += prefix;
			}
			return '[' + result + ']';
		}

		if (obj.constructor === String) {
			return '"' + obj.toString().replace(/\"/g, '\\"') + '"';
		}

		// Boolean, Number, Date, Math, RegExp, Global, ... ???
		return obj.toString();
	}
	return toJsStringRecursive(obj, prefix);
}
