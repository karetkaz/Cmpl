function Instrument() {

	var data = null;

	var functions = null;
	var samples = null;

	function enter(calls, id, tick) {
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

	function getSamples(subtree) {
		return samples;
	}

	function getSciteApi() {
		var scite_api = '';

		if (data == null) {
			return 'No data available!';
		}

		function writeSym(sym) {
			if (sym.declaredIn && sym.kind !== 'param') {
				scite_api += sym.declaredIn + '.';
			}
			scite_api += sym.name;
			if (sym.args !== undefined) {
				scite_api += '(';
				if (!(sym.args.length === 1 && sym.args[0].size == 0)) {
					for (var i = 0; i < sym.args.length; i += 1) {
						if (i > 0) {
							scite_api += ', ';
						}
						writeSym(sym.args[i]);
					}
				}
				scite_api += ')';
			}
			if (sym.args !== undefined || sym.kind === 'param') {
				// Scite handles only function return types
				scite_api += ": " + sym.type;
			}
		}

		var api = data.symbols;
		for (var i = 0; i < api.length; i += 1) {
			var sym = api[i];
			/*if (sym.kind === 'opcode') {
				continue;
			}*/
			writeSym(sym);
			scite_api += '\n';
		}
		return scite_api;
	}

	function getFiltered(checkSymbol) {
		var scite_api = '';
		if (data == null) {
			return 'No data available!';
		}

		var api = data.symbols;
		for (var i = 0; i < api.length; i += 1) {
			var sym = api[i];
			if (!checkSymbol(sym)) {
				continue;
			}
			if (sym.declaredIn != undefined) {
				scite_api += sym.declaredIn + '.';
			}
			scite_api += sym.name;
			if (sym.args !== undefined) {
				scite_api += '(';
				if (!(sym.args.length === 1 && sym.args[0].size == 0)) {
					for (var j = 0; j < sym.args.length; j += 1) {
						var arg = sym.args[j];
						if (j > 0) {
							scite_api += ', ';
						}
						scite_api += arg.type + ' ';
						scite_api += arg.name;
						scite_api += ": " + arg.type;
					}
				}
				scite_api += ')';
			}
			scite_api += ": " + sym.type + '\n';
		}
		return scite_api;
	}

	return {
		api: {
			getSciteApi: getSciteApi,
			getTypenames: function() {
				return getFiltered(function(sym) {
					return sym.kind === '.rec';
				});
			},
			getVariables: function() {
				return getFiltered(function(sym) {
					if (sym.args !== undefined) {
						return false;
					}
					return sym.kind === '.ref';
				});
			},
			getFunctions: function() {
				return getFiltered(function(sym) {
					if (sym.args === undefined) {
						return false;
					}
					return sym.kind === '.ref';
				});
			},
			getOpcodes: function() {
				return getFiltered(function(sym) {
					return sym.kind === 'emit';
				});
			},
			getAll: function () {
				return getFiltered(function(sym) {
					return true;
				});
			}
		},
		loadFile: function(file, action) {
			var reader = new FileReader();
			reader.onload = function(){
				data = JSON.parse(reader.result);
				var i, j, calls = [{
					enter: +Infinity,
					leave: -Infinity,
					samples: {},
					calltree: null,
					//excluded: ['Halt', 'ToDays'],
				}];;

				functions = Object.create(null);	// empty object no prototype.
				for (var i = 0; i < data.profile.functions.length; ++i) {
					var symbol = data.profile.functions[i].symbol;
					/*if (!symbol.static) {
						continue;
					}*/
					if (symbol.offs == 0) {
						//continue;
					}
					if (symbol.kind === '.rec') {
						//continue;
					}
					if (symbol.kind === 'emit') {
						//continue;
					}
					functions[symbol.offs] = symbol;
				}

				samples = null;
				for (i = 0; i < data.profile.callTree.length; i += 2) {
					var time = data.profile.callTree[i + 0];
					var offs = data.profile.callTree[i + 1];

					if (offs !== -1) {
						var callee = enter(calls, offs, time);
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

				if (action != null) {
					action();
				}
			};
			reader.readAsText(file);
		},
		getSamples: getSamples
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
