/**
 *
 */
function indexData(data, options) {
	// default options.
	if (options === undefined) {
		options = {
			groups: {},
			filters: [],
			ignoreCase: true,
			ignoreAccent: true
		};
	}

	function walkObject(obj, callback) {
		var stack = [];
		function impl(obj) {
			if (obj === null || obj === undefined) {
				callback(stack, obj);
			}
			else if (obj.constructor === Function) {
			}
			else if (obj.constructor === Object) {
				//stack.push('*');
				callback(stack, obj);
				//stack.pop();
				for (var key in obj) {
					stack.push(key);
					impl(obj[key]);
					stack.pop();
				}
			}
			else if (obj.constructor === Array) {
				stack.push('*');
				callback(stack, obj);
				stack.pop();
				for (var idx = 0; idx < obj.length; idx += 1) {
					impl(obj[idx]);
				}
			}
			else {
				// Boolean, Number, Date, Math, RegExp, Global, ... ???
				callback(stack, obj);
			}
		}
		impl(obj);
	}

	function makeMapper(options) {
		var str = "ÀÁÂÃÄÅàáâãäåĀāĂăĄąÇçĆćĈĉĊċČčÐðĎďĐđÈÉÊËèéêëĒēĔĕĖėĘęĚěĜĝĞğĠġĢģĤĥĦħÌÍÎÏìíîïĨĩĪīĬĭĮįİıĴĵĶķĸĹĺĻļĽľĿŀŁłÑñŃńŅņŇňŉŊŋÒÓÔÕÖØòóôõöøŌōŎŏŐőŔŕŖŗŘřŚśŜŝŞşŠšſŢţŤťŦŧÙÚÛÜùúûüŨũŪūŬŭŮůŰűŲųŴŵÝýÿŶŷŸŹźŻżŽž";
		var map = "AAAAAAaaaaaaAaAaAaCcCcCcCcCcDdDdDdEEEEeeeeEeEeEeEeEeGgGgGgGgHhHhIIIIiiiiIiIiIiIiIiJjKkkLlLlLlLlLlNnNnNnNnnNnOOOOOOooooooOoOoOoRrRrRrSsSsSsSssTtTtTtUUUUuuuuUuUuUuUuUuUuWwYyyYyYZzZzZz";
		var filter = options.ignoreAccent;
		var i, mapper = Object.create(null);

		if (filter === undefined || filter === null) {
			// ignore accents by default in search.
			filter = true;
		}

		if (filter === true || filter === false) {
			filter = filter ? map : '';
		}
		else if (filter.constructor !== String) {
			throw "Invalid argument for filtering. ignoreAccent: " + ignoreAccent.constructor;
		}

		// mapper will be sometnig like: {'a': 'ÀÁÂÃÄÅàáâãäåĀāĂăĄą', 'c': 'ÇçĆćĈĉĊċČč', ...}
		for (i = 0; i < map.length; ++i) {

			var chr = map[i];

			if (options.ignoreCase === true) {
				chr = chr.toLowerCase();
			}

			if (filter.indexOf(chr) < 0) {
				continue;
			}

			mapper[chr] = (mapper[chr] || '') + str[i];
		}

		// transform to regular expressions to replace all occurences.
		for (i in mapper) {
			mapper[i] = new RegExp('[' + mapper[i] + ']', 'g');
		}

		return function(str) {
			if (options.ignoreCase === true) {
				str = str.toLowerCase();
			}
			for (var r in mapper) {
				str = str.replace(mapper[r], r);
			}
			return str;
		}
	}

	function makeMatcher(matcher) {
		if (matcher === undefined || matcher === null) {
			return undefined;
		}
		if (matcher.constructor === Function) {
			return matcher;
		}
		if (matcher.constructor === Object) {
			// match all.
			var m, matchers = {};
			for (var k in matcher) {
				m = makeMatcher(matcher[k]);
				if (m !== undefined && m != null) {
					matchers[k] = m;
				}
			}
			return function(value) {
				// match all.
				for (var k in matchers) {
					if (!matchers[k](value[k])) {
						return false;
					}
				}
				return true;
			};
		}
		if (matcher.constructor === Array) {
			// match any.
			var m, matchers = [];
			for (var k = 0; k < matcher.length; ++k) {
				m = makeMatcher(matcher[k]);
				if (m !== undefined && m != null) {
					matchers.push(m);
				}
			}
			return function(value) {
				for (var k = 0; k < matchers.length; ++k) {
					if (matchers[k](value)) {
						return true;
					}
				}
				return false;
			};
		}
		return function(value) {
			// enable coercion
			return matcher == value;
		};
	}

	// cerate indexes
	var index = [];
	var mapstr = makeMapper(options);
	var filterBy = options.filters || [];

	var groupBy = {};
	if (options.groups !== undefined) {
		var groups = options.groups;
		if (groups.constructor === String) {
			groups = {};
			groups[options.groups] = options.groups;
		}
		if (groups.constructor === Array) {
			groups = {};
			for (var key = 0; key < options.groups.length; ++key) {
				groups[options.groups[key]] = options.groups[key];
			}
		}
		for (var key in groups) {
			groupBy[groups[key]] = {
				key: key,
				items: [],
				values: []
			};
		}
	}

	for (i = 0; i < data.length; ++i) {
		var str = '';
		walkObject(data[i], function(stack, obj) {
			if (obj === null || obj === undefined) {
				return;
			}
			var path = stack.join('.');
			if (groupBy.hasOwnProperty(path)) {
				var group = groupBy[path];
				var objStr = JSON.stringify(obj);
				if (group.values.indexOf(objStr) < 0) {
					group.values.push(objStr);
					group.items.push(obj);
				}
			}
			if (filterBy.indexOf(path) < 0) {
				return;
			}
			if (str !== '') {
				str += '\n';
			}
			str += obj;
		});
		index[i] = mapstr(str);
	}

	var result = {};
	for (var key in groupBy) {
		var group = groupBy[key];
		group.items.sort();
		result[group.key] = group.items;
	}

	result.filter = function(options) {
		if (options === undefined || options === null) {
			return data;
		}
		else if (options.constructor === String) {
			options = { query: options };
		}
		else if (options.constructor !== Object) {
			throw "Invalid argument for filtering.";
		}

		var match = makeMatcher(options.match);
		if (match === undefined) {
			match = function() {
				return true;
			};
		}
		var str = mapstr(options.query);
		var i, k, result = [];
		// collect intexes that matches
		for (i = 0; i < data.length; ++i) {
			if (index[i].indexOf(str) < 0) {
				continue;
			}
			if (!match(data[i])) {
				continue;
			}
			result.push(i)
		}

		// sort entries based on index
		if (options.sort !== undefined) {
			result.sort(function(a, b) {
				var strA = index[a];
				var strB = index[b];
				var begA = str === strA.slice(0, str.length);
				var begB = str === strB.slice(0, str.length);

				if (begA !== begB) {
					return begA ? -1 : +1;
				}
				return strA.localeCompare(strB);
			});
		}

		// replace indexes with objects
		for (i = 0; i < result.length; ++i) {
			result[i] = data[result[i]];
		}
		return result;
	};

	result.index = index;
	return result;
}
