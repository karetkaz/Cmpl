let projects = {
	'test.cmplGfx' : {
		base: '/Cmpl/cmplGfx/test/',
		main: 'testGfx.cmpl',
		flags: ['libGfx', 'libFile'],
		files: [
			'./asset/font/NokiaPureText.ttf',
			'./asset/font/NokiaPureTextBold.ttf',
			'./asset/font/NokiaPureTextLight.ttf',
			'./asset/font/Release_notes_NokiaPure Text.txt',
			'./asset/image/david.png',
			'./asset/image/download.jpg',
			'./asset/image/forest.jpg',
			'./asset/image/garden.png',
			'./asset/image/heightmap.png',
			'./asset/image/texture.png',
			'./asset/image/texture_nature_01.png',
			'./asset/lut3d/exo-identity.png',
			'./asset/lut3d/identity.png',
			'./asset/lut3d/obs-guide-lut.png',
			'./asset/mesh/teapot.obj',
			'./demo/Biorhythm.cmpl',
			'./demo/Clock.cmpl',
			'./demo/ImageViewer.cmpl',
			'./demo/Magnifyer.cmpl',
			'./demo/Planets.cmpl',
			'./demo/Primes.cmpl',
			'./demo/StarField.cmpl',
			'./demo/Sunflower.cmpl',
			'./draw.2d/L-System.cmpl',
			'./draw.2d/Pattern8x8.cmpl',
			'./draw.2d/PlotFixed.cmpl',
			'./draw.2d/draw.bezier.cmpl',
			'./draw.2d/draw.image.cmpl',
			'./draw.2d/draw.test.cmpl',
			'./draw.3d/RayTracer.cmpl',
			'./draw.3d/RayTracer.interactive.cmpl',
			'./draw.3d/draw.mesh.cmpl',
			'./draw.3d/height.map.cmpl',
			'./draw.3d/parametric.cmpl',
			'./games/2048.cmpl',
			'./games/FloodIt.cmpl',
			'./image.fx/Blending.cmpl',
			'./image.fx/ColorMatrix.cmpl',
			'./image.fx/Dithering.cmpl',
			'./image.fx/Histogram.cmpl',
			'./image.fx/MaskedBlur.cmpl',
			'./image.fx/Transform.cmpl',
			'./image.fx/lookup-ease.cmpl',
			'./image.fx/lookup-levels.cmpl',
			'./image.fx/lookup-linear.cmpl',
			'./image.fx/lookup-lut3d.cmpl',
			'./image.fx/lookup-manual.cmpl',
			'./image.fx/select-hsl.cmpl',
			'./image.pd/2d.ImageDiff.cmpl',
			'./image.pd/Blobs.cmpl',
			'./image.pd/ComplexDomainColoring.cmpl',
			'./image.pd/Mandelbrot.cmpl',
			'./image.pd/PlaneDeform.cmpl',
			'./image.pd/YinYang.cmpl',
			'./testGfx.cmpl',
			'./widget/Calculator.cmpl',
			'./widget/ColorPicker.cmpl',
			'./widget/DatePicker.cmpl',
			'./widget/TreeView.cmpl',
			'./widget/layout.Align.cmpl',
			'./widget/layout.Test.cmpl',
			'./widget/rounded.rect.cmpl',
		]
	},
	'test.cmplStd' : {
		base: '/Cmpl/cmplStd/test/',
		main: 'test.cmpl',
		flags: ['libFile'],
		files: [
			'./demo/BitwiseArithmetic.cmpl',
			'./extra/bench/DNA.K-mers/k-mers.cmpl',
			'./extra/bench/DNA.K-mers/k-mers.cpp',
			'./extra/bench/DNA.K-mers/k-mers.py',
			'./extra/bench/DNA.K-mers/results.txt',
			'./extra/bench/simd.complex.mul.cmpl',
			'./extra/unit/decl.var-refdefinition.cmpl',
			'./extra/unit/if.then.else-opt.all-any.cmpl',
			'./extra/unit/if.then.else-opt.branching.cmpl',
			'./extra/unit/test.polyEval.cmpl',
			'./extra/unit/test.raise.array.args.cmpl',
			'./extra/unit/test.return.cmpl',
			'./lang/emit.cmpl',
			'./lang/function.cmpl',
			'./lang/init.array.cmpl',
			'./lang/init.member.cmpl',
			'./lang/init.method.cmpl',
			'./lang/init.reference.cmpl',
			'./lang/init.variable.cmpl',
			'./lang/inlineMacros.cmpl',
			'./lang/lookup.cmpl',
			'./lang/memory.cmpl',
			'./lang/op.cmp.float.cmpl',
			'./lang/overload.inline.cmpl',
			'./lang/pointer.cmpl',
			'./lang/recPacking.cmpl',
			'./lang/recUnion.cmpl',
			'./lang/reflect.basic.cmpl',
			'./lang/reflect.globals.cmpl',
			'./lang/reflect.serialize.cmpl',
			'./lang/stmt.for.cmpl',
			'./lang/stmt.if.cmpl',
			'./lang/sys.Platform.cmpl',
			'./lang/testCast.cmpl',
			'./lang/tryExec.cmpl',
			'./lang/useOperator.cmpl',
			'./math/test.Complex.cmpl',
			'./math/test.Math.cmpl',
			'./math/test.Numbers.cmpl',
			'./math/test.Polynomial.cmpl',
			'./math/test.ext.Fixed32.cmpl',
			'./math/test.ext.Float64.cmpl',
			'./math/test.ext.Int16.cmpl',
			'./math/test.ext.Int32.cmpl',
			'./math/test.ext.Int64.cmpl',
			'./math/test.ext.Int8.cmpl',
			'./math/test.ext.Integer.cmpl',
			'./test.cmpl',
			'./text/test.fmt.Fixed32.cmpl',
			'./text/test.fmt.Float64.cmpl',
			'./text/test.fmt.Number.cmpl',
			'./time/test.Datetime.cmpl',
			'./time/test.Timestamp.cmpl',
			'./time/test.Timeunit.cmpl',
		]
	},
	'test.cmplFile' : {
		base: '/Cmpl/cmplFile/test/',
		main: 'hello.world.cmpl',
		flags: ['libFile'],
		files: [
			{path: '~/out/', content: ''}, // create the output directory for temporary files
			{path: '/cmplGfx/test/asset/image/david.png', url: '/Cmpl/cmplGfx/test/asset/image/david.png'},

			'./file.write.cmpl',
			'./hello.world.cmpl',
			'./test.base64.cmpl',
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
