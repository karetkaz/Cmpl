function JsArgs(paramSeparator, onHashChange) {

	// if not specified use local hash to read/save params.
	var valueSeparator = '&';
	var oldHashChange = window.onhashchange;

	if (paramSeparator !== '?') {
		paramSeparator = '#';
	}

	// read values from url
	function read(args) {
		var values = window.location.href;
		var pos = values.indexOf(paramSeparator);
		if (pos >= 0) {
			values = values.substr(pos + 1).split(valueSeparator);
		}
		else {
			values = [];
		}
		for (var i = 0; i < values.length; i += 1) {
			pos = values[i].indexOf('=');
			if (pos > 0) {
				var key = values[i].substr(0, pos);
				var value = values[i].substr(pos + 1);
				args[key] = decodeURIComponent(value);
			}
		}
		return args;
	}

	// write values to url
	function save(args, forceReload) {
		var values = '';
		for (var key in args) {
			if (!args.hasOwnProperty(key)) {
				// exclude prototype properties
				continue;
			}

			var value = args[key];
			if (value === null || value === undefined) {
				// exclude undefined and null values
				continue;
			}
			if (value.constructor !== String && value.constructor !== Number && value.constructor !== Boolean) {
				// exclude non string, number and boolean values.
				continue;
			}

			if (values !== '') {
				values += valueSeparator;
			}
			values += key + '=' + encodeURIComponent(value);
		}

		if (forceReload === true) {
			window.onHashChange = oldHashChange;
			window.location.href = paramSeparator + values;
			window.location.reload();
		}
		window.location.href = paramSeparator + values;
		return args;
	}

	// callbacks triggered on insert, update, delete
	// var onUpdate = undefined;

	var canUpdate = false;
	var result = Object.create({
		reload: function(values) {
			// remove old entries.
			for (var key in this) {
				if (!this.hasOwnProperty(key)) {
					// do not delete properties from prototype
					continue;
				}
				delete this[key];
			}
			for (var key in values) {
				if (!values.hasOwnProperty(key)) {
					// do not delete properties from prototype
					continue;
				}
				this[key] = String(values[key]);

			}
			return save(this, true);
		},
		update: function(values) {
			var changes = Object.create(null);
			var changed = false;

			if (values === undefined) {
				values = read({});

				// remove old entries.
				for (var key in this) {
					if (!this.hasOwnProperty(key)) {
						// do not delete properties from prototype
						continue;
					}
					if (values.hasOwnProperty(key)) {
						// do not delete properties to be updated
						continue;
					}

					changes[key] = {'old': this[key]};
					delete this[key];
					changed = true;
				}
			}

			// update existing values.
			for (var key in values) {
				if (!values.hasOwnProperty(key)) {
					// do not update properties in prototype
					continue;
				}
				var oldValue = this[key];
				var newValue = String(values[key]);
				if (oldValue === newValue) {
					// do not update properties whith same values
					continue;
				}
				changes[key] = {'new': newValue, 'old': oldValue};
				this[key] = newValue;
				changed = true;
			}
			save(this);
			if (canUpdate && changed && onHashChange !== undefined) {
				onHashChange(this, changes);
			}
			return this;
		}
	});

	read(result);
	if (onHashChange !== undefined) {
		window.onhashchange = function() {
			result.update(undefined);
			if (oldHashChange != null) {
				oldHashChange(result);
			}
		};
		onHashChange(result);
	}

	canUpdate = true;
	return result;
}
