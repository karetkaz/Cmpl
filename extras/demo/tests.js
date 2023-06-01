let projects = {
	'test.cmplGfx' : {
		base: '/Cmpl/cmplGfx/test/',
		main: 'testGfx.ci',
		flags: ['libGfx'],
		files: [
			'asset/font/NokiaPureText.ttf',
			'asset/image/download.jpg',
			'asset/image/david.png',
			'asset/image/forest.jpg',
			'asset/image/garden.png',
			'asset/image/heightmap.png',
			'asset/image/texture.png',
			'asset/image/texture_nature_01.png',
			'asset/lut3d/identity.png',
			'asset/lut3d/obs-guide-lut.png',
			'asset/mesh/teapot.obj',

			'demo/2d.draw.bezier.ci',
			'demo/2d.draw.test.ci',
			'demo/2d.L-System.ci',
			'demo/2048.ci',
			'demo/AnalogClock.ci',
			'demo/Biorhythm.ci',
			'demo/FloodIt.ci',
			'demo/Pattern8x8.ci',
			'demo/RayTracerI.ci',
			'demo/StarField.ci',

			'demo.procedural/2d.ImageDiff.ci',
			'demo.procedural/Blobs.ci',
			'demo.procedural/Complex.ci',
			'demo.procedural/Mandelbrot.ci',
			'demo.procedural/YinYang.ci',

			'demo.widget/ColorPicker.ci',
			'demo.widget/DatePicker.ci',
			'demo.widget/layout.Align.ci',
			'demo.widget/layout.Test.ci',
			'demo.widget/rounded.rect.ci',
			'demo.widget/TreeView.ci',

			'2d.draw.image.ci',
			'3d.draw.mesh.ci',
			'3d.height.map.ci',
			'3d.parametric.ci',
			'Calculator.ci',
			'fx.color.blend.ci',
			'fx.color.dither.ci',
			'fx.color.lookup.ci',
			'fx.color.lookup-ease.ci',
			'fx.color.lookup-levels.ci',
			'fx.color.lookup-lut3d.ci',
			'fx.color.lookup-manual.ci',
			'fx.color.matrix.ci',
			'fx.color.select-hsl.ci',
			'fx.histogram.ci',
			'fx.image.blur.ci',
			'fx.image.transform.ci',
			'PlaneDeform.ci',
			'RayTracer.ci',
			'testGfx.ci',
		]
	},
	'test.cmplStd' : {
		base: '/Cmpl/cmplStd/test/',
		main: 'test.ci',
		flags: ['libFile'],
		files: [ //$ find cmplStd/test -type f | sort
			'benchmark/DNA K-mers/k-mers.ci',
			'benchmark/DNA K-mers/k-mers.cpp',
			'benchmark/DNA K-mers/k-mers.py',
			'benchmark/DNA K-mers/results.txt',
			'benchmark/simd.complex.mul.ci',
			'benchmark/todo.txt',

			'demo/BitwiseArithmetic.ci',
			'lang/emit.ci',
			'lang/function.ci',
			'lang/init.array.ci',
			'lang/init.member.ci',
			'lang/init.method.ci',
			'lang/init.reference.ci',
			'lang/init.variable.ci',
			'lang/inlineMacros.ci',
			'lang/memory.ci',
			'lang/overload.inline.ci',
			'lang/recPacking.ci',
			'lang/recUnion.ci',
			'lang/reflect.ci',
			'lang/stmt.for.ci',
			'lang/stmt.if.ci',
			'lang/sys.Platform.ci',
			'lang/tryExec.ci',
			'lang/useOperator.ci',
			'math/test.Bits.ci',
			'math/test.Complex.ci',
			'math/test.Math.ci',
			'math/test.Polynomial.ci',
			'math/test.ext.Bits.ci',
			'math/test.ext.Fixed.ci',
			'math/test.ext.Math.ci',
			'text/test.format.decimal.ci',
			'text/test.format.number.ci',
			'time/test.Calendar.ci',
			'time/test.Datetime.ci',
			'time/test.Timestamp.ci',
			'time/test.Timeunit.ci',
			'test.ci',
		]
	},
	'test.cmplFile' : {
		base: '/Cmpl/cmplFile/test/',
		main: 'hello.world.ci',
		flags: ['libFile'],
		files: [
			{path: '~/out/', content: ''}, // create output directory
			{path: '/cmplGfx/test/asset/image/david.png', url: '/Cmpl/cmplGfx/test/asset/image/david.png'},

			'file.write.ci',
			'hello.world.ci',
			'test.base64.ci',
		]
	},
};

(function () {
	// add scripts saved in indexed database
	if (indexedDB.databases != null) {
		workspaceName.innerText = 's';
		indexedDB.databases().then(function(dbs) {
			let prefix = '/workspace-';
			for (db of dbs) {
				let name = db.name;
				if (!name.startsWith(prefix)) {
					continue;
				}
				name = name.substr(prefix.length);
				let url = window.location.origin + window.location.pathname + '#workspace=' + name;
				fileList.innerHTML += '<li class="project" onclick="params.update({workspace: \'' + name + '\'});">' + name +
					'<button class="right" onclick="event.stopPropagation(); rmWorkspace(\'' + name + '\')" title="Remove workspace">-</button>' +
					'</li>'
			}
		}).then(function() {
			for (let demo in projects) {
				fileList.innerHTML += '<li class="project" onclick="params.update(toProject(projects[this.textContent]));">' + demo + '</li>'
			}
		});
	} else {
		terminal.append('Failed to list workspaces, indexedDB.databases not available');
		for (let demo in projects) {
			fileList.innerHTML += '<li class="project" onclick="params.update(toProject(projects[this.textContent]));">' + demo + '</li>'
		}
	}
}());

function toProject(proj) {
	let project = {};

	// set libraries to be used
	if (proj.flags != null) {
		for (let flag of proj.flags) {
			project[flag] = '';
		}
	}

	// set files to be downloaded
	if (proj.files != null) {
		let files = [];
		for (let file of proj.files) {
			if (file != null && file.constructor === Object) {
				files.push(file);
				continue;
			}
			if (file.startsWith(proj.base)) {
				file = file.substring(proj.base.length, file.length);
			}
			let base = (proj.root || '') + (proj.base || '');
			if (!base.endsWith('/')) {
				base += '/';
			}
			files.push({
				path: file,
				url: base + file
			});
		}
		project.project = btoa(JSON.stringify(files));
	}

	// set file to be opened after project is loaded
	if (proj.main != null) {
		let main = proj.main;
		if (main.startsWith(proj.base)) {
			main = main.substring(proj.base.length, main.length);
		}
		project.path = main;
	}

	return project;
}
