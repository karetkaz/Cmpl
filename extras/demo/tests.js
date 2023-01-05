let projects = {
	'test.cmplStd' : {
		base: 'cmplStd/test/',
		main: 'cmplStd/test/test.ci',
		flags: ['useWebWorker', 'libFile'],
		files: [
			'benchmark/DNA K-mers/k-mers.cpp',
			'benchmark/DNA K-mers/k-mers.ci',
			'benchmark/DNA K-mers/k-mers.py',
			'benchmark/DNA K-mers/results.txt',
			'benchmark/simd.complex.mul.ci',

			'demo/BitwiseArithmetic.ci',

			'lang/emit.ci',
			'lang/function.ci',
			'lang/init.array.ci',
			'lang/init.member.ci',
			'lang/init.method.ci',
			'lang/init.reference.ci',
			'lang/init.variable.ci',
			'lang/inlineMacros.ci',
			'lang/overload.inline.ci',
			'lang/recPacking.ci',
			'lang/recUnion.ci',
			'lang/reflect.ci',
			'lang/stmt.for.ci',
			'lang/stmt.if.ci',
			'lang/useOperator.ci',

			'std/math.Bits.ci',
			'std/math.Complex.ci',
			'std/math.Polynomial.ci',
			'std/math.Trigonometry.ci',
			'std/memory.ci',
			'std/number.ci',
			'std/sys.Platform.ci',
			'std/test.math.ci',
			'std/tryExec.ci',

			'test.ci',
		]
	},
	'test.cmplGfx' : {
		base: 'cmplGfx/test/',
		main: 'cmplGfx/test/testGfx.ci',
		flags: ['libGfx'],
		files: [
			'cmplGfx/test/asset/font/NokiaPureText.ttf',
			'cmplGfx/test/asset/image/david.png',
			'cmplGfx/test/asset/image/forest.jpg',
			'cmplGfx/test/asset/image/garden.png',
			'cmplGfx/test/asset/image/heightmap.png',
			'cmplGfx/test/asset/image/texture.png',
			'cmplGfx/test/asset/image/texture_nature_01.png',
			'cmplGfx/test/asset/lut3d/identity.png',
			'cmplGfx/test/asset/lut3d/obs-guide-lut.png',
			'cmplGfx/test/asset/mesh/teapot.obj',

			'cmplGfx/test/demo/2d.draw.bezier.ci',
			'cmplGfx/test/demo/2d.draw.rrect.ci',
			'cmplGfx/test/demo/2d.draw.test.ci',
			'cmplGfx/test/demo/2d.L-System.ci',
			'cmplGfx/test/demo/2048.ci',
			'cmplGfx/test/demo/AnalogClock.ci',
			'cmplGfx/test/demo/Biorhythm.ci',
			'cmplGfx/test/demo/FloodIt.ci',
			'cmplGfx/test/demo/RayTracer0.ci',
			'cmplGfx/test/demo/RayTracerI.ci',
			'cmplGfx/test/demo/StarField.ci',

			'cmplGfx/test/demo.procedural/2d.ImageDiff.ci',
			'cmplGfx/test/demo.procedural/Blobs.ci',
			'cmplGfx/test/demo.procedural/Complex.ci',
			'cmplGfx/test/demo.procedural/Mandelbrot.ci',
			'cmplGfx/test/demo.procedural/YinYang.ci',

			'cmplGfx/test/demo.widget/ColorPicker.ci',
			'cmplGfx/test/demo.widget/DatePicker.ci',
			'cmplGfx/test/demo.widget/layout.Align.ci',
			'cmplGfx/test/demo.widget/layout.Test.ci',
			'cmplGfx/test/demo.widget/rounded.rect.ci',
			'cmplGfx/test/demo.widget/TreeView.ci',

			'cmplGfx/test/2d.draw.image.ci',
			'cmplGfx/test/3d.draw.mesh.ci',
			'cmplGfx/test/3d.height.map.ci',
			'cmplGfx/test/3d.parametric.ci',
			'cmplGfx/test/Calculator.ci',
			'cmplGfx/test/fx.color.blend.ci',
			'cmplGfx/test/fx.color.dither.ci',
			'cmplGfx/test/fx.color.lookup.ci',
			'cmplGfx/test/fx.color.lookup-ease.ci',
			'cmplGfx/test/fx.color.lookup-levels.ci',
			'cmplGfx/test/fx.color.lookup-lut3d.ci',
			'cmplGfx/test/fx.color.lookup-manual.ci',
			'cmplGfx/test/fx.color.matrix.ci',
			'cmplGfx/test/fx.color.select-hsl.ci',
			'cmplGfx/test/fx.histogram.ci',
			'cmplGfx/test/fx.image.blur.ci',
			'cmplGfx/test/fx.image.transform.ci',
			'cmplGfx/test/RayTracer.ci',
			'cmplGfx/test/testGfx.ci',
		]
	},
	'test.cmplFile' : {
		base: 'cmplFile/test/',
		main: 'hello.world.ci',
		flags: ['useWebWorker', 'libFile'],
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
