/*int mlcolor(double pos[3], double Normal[3]) {
	/ *	VertexColor(r, g, b, alpha)  =
			emis_M + 
			//~ ambi_LIGHTMODEL * M.ambi +
			attn_i * spotlighteffect_i *
			(
				L[i].ambi * M.ambi +
				L[i].diff * M.diff * max(dot(L, n), 0) +
				L[i].spec * M.spec * pow(max(dot(s, n), 0)), M.SHININESS)
			)
			Sum(i = 0; N - 1; i++) {
				//~ dist = pos_L - pos_V;
				//~ attn = 1 / (K1 + K2 · dist + K3 · dist ** 2);
				//~ L = unit vector from vertex to light position
				//~ n = unit normal (at a vertex)
				//~ s = (x  + (0, 0, 1) ) / | x + (0, 0, 1) |
				//~ x = vector indicating direction from light source (or direction of directional light)
				attn * (spotlighteffect)i
				* [ ambi_L * ambi_M  +
				max (dot(L, n), 0)  * diff_L * diff_M  +
				max (dot(s, n), 0) ) ** SHININESS     *  
				specular_L  *  specular_M] 
			} i
	* /
	double col[3];
	unsigned i, n = sizeof(Lights) / sizeof(*Lights);
	dv3cpy(col, material.emis);
	for (i = 0; i < n; i++) {
		double tmp[3];//, dir[3];
		//~ double dist = dv3dot(dv3sub(tmp, Lights[i].pos, pos), Normal);
		double diff = -dv3dot(dv3nrm(tmp, dv3sub(tmp, pos, Lights[i].pos)), Normal);
		//~ double spec;
		//~ dv3sub(tmp, pos, Lights[i].pos);
		//~ dv3nrm(dir, dv3sub(tmp, pos, Lights[i].pos));								// dir of light
		//~ spec = dv3dot(dv3nrm(tmp, dv3add(tmp, dv3set(tmp, 0,0,1), dir)), Normal);
			// l = pow(max(dot(s, n), 0)), SHININESS)
			// s = directional light ? light.dir : normalize (vector indicating direction from light source + (0, 0, 1))
		//~ double spec = dv3dot(Normal, s);
		//~ spec := dot(Normal, Lights[i].H);
		dv3add(col, col, dv3mul(tmp, material.ambi, Lights[i].ambi));
		if (diff > 0) dv3add(col, col, dv3sca(tmp, dv3mul(tmp, material.diff, Lights[i].diff), diff));
		//~ if (spec > 0) dv3add(col, col, dv3sca(tmp, dv3mul(tmp, material.spec, Lights[i].spec), pow(spec, material.spec[3])));
	}// * /
	if (col[0] > 1) col[0] = 1; else if (col[0] < 0) col[0] = 0;
	if (col[1] > 1) col[1] = 1; else if (col[1] < 0) col[1] = 0;
	if (col[2] > 1) col[2] = 1; else if (col[2] < 0) col[2] = 0;
	return (0.299 * col[0] + 0.587 * col[1] + 0.114 * col[2]) * 255;
}

//*/

int addvtxdv(double pos[3], double nrm[3], double tex[2]) {
	if (mesh.vtxcnt >= mesh.maxvtx) {
		mesh.vtxptr = (struct vtx*)realloc(mesh.vtxptr, sizeof(struct vtx) * (mesh.maxvtx <<= 1));
		if (!mesh.vtxptr) {
			k3d_exit();
			return -1;
		}
	}
	vecldf(&mesh.vtxptr[mesh.vtxcnt].pos, pos[0], pos[1], pos[2], 1);
	if (nrm) vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, nrm[0], nrm[1], nrm[2], 0);
	else vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, 0, 0, 0, 0);
	if (tex) {
		mesh.vtxptr[mesh.vtxcnt].tex.s = tex[0] * 65535;
		mesh.vtxptr[mesh.vtxcnt].tex.t = tex[1] * 65535;
	}
	else {
		mesh.vtxptr[mesh.vtxcnt].tex.s = 0;
		mesh.vtxptr[mesh.vtxcnt].tex.t = 0;
	}
	return mesh.vtxcnt++;
}

/*int addvtxfv(float pos[3], float nrm[3]) {
	if (mesh.vtxcnt >= mesh.maxvtx) {
		mesh.vtxptr = (struct vtx*)realloc(mesh.vtxptr, sizeof(struct vtx) * (mesh.maxvtx <<= 1));
		if (!mesh.vtxptr) {
			k3d_exit();
			return -1;
		}
	}
	vecldf(&mesh.vtxptr[mesh.vtxcnt].pos, pos[0], pos[1], pos[2], 1);
	if (nrm) vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, nrm[0], nrm[1], nrm[2], 0);
	else vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, 0, 0, 0, 0);
	return mesh.vtxcnt++;
}

int addvtxf(float px, float py, float pz, float nx, float ny, float nz) {
	if (mesh.vtxcnt >= mesh.maxvtx) {
		mesh.vtxptr = (struct vtx*)realloc(mesh.vtxptr, sizeof(struct vtx) * (mesh.maxvtx <<= 1));
		if (!mesh.vtxptr) {
			k3d_exit();
			return -1;
		}
	}
	vecldf(&mesh.vtxptr[mesh.vtxcnt].pos, px, py, pz, 1);
	vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, nx, ny, nz, 0);
	return mesh.vtxcnt++;
}*/

int vtxequ(int i, int j) {
	if (mesh.vtxptr[i].pos.x != mesh.vtxptr[j].pos.x) return 0;
	if (mesh.vtxptr[i].pos.y != mesh.vtxptr[j].pos.y) return 0;
	if (mesh.vtxptr[i].pos.z != mesh.vtxptr[j].pos.z) return 0;
	if (mesh.vtxptr[i].tex.s != mesh.vtxptr[j].tex.s) return 0;
	if (mesh.vtxptr[i].tex.t != mesh.vtxptr[j].tex.t) return 0;
	return 1;
}

int addtri(unsigned p1, unsigned p2, unsigned p3) {
	if (p1 >= mesh.vtxcnt || p2 >= mesh.vtxcnt || p3 >= mesh.vtxcnt) return -1;
	if (mesh.tricnt >= mesh.maxtri) {
		mesh.triptr = (struct tri*)realloc(mesh.triptr, sizeof(struct tri) * (mesh.maxtri <<= 1));
		if (!mesh.triptr) {
			k3d_exit();
			return -1;
		}
	}
	mesh.triptr[mesh.tricnt].i1 = p1;
	mesh.triptr[mesh.tricnt].i2 = p2;
	mesh.triptr[mesh.tricnt].i3 = p3;
	return mesh.tricnt ++;
}

void mesheval(double* evalP(double [3], double s, double t), unsigned sdiv, unsigned tdiv, unsigned closed) {
	unsigned si, ti, mp[256][256];
	double pos[3], s, ds = 1. / (sdiv - 1);
	double nrm[3], t, dt = 1. / (tdiv - 1);
	if (sdiv > 256 || tdiv > 256) return;
	for (t = ti = 0; ti < tdiv; ti++, t += dt) {
		for (s = si = 0; si < sdiv; si++, s += ds) {
			double tex[2]; tex[0] = t; tex[1] = s;
			mp[ti][si] = addvtxdv(evalP(pos, s, t), evalN(nrm, s, t, evalP), tex);
		}
	}
	for (ti = 0; ti < tdiv - 1; ti += 1) {
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si + 0][ti + 0];		// 		v0--v3
			int v1 = mp[si + 0][ti + 1];		// 		| \  |
			int v3 = mp[si + 1][ti + 0];		// 		|  \ |
			int v2 = mp[si + 1][ti + 1];		// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
		if (closed & 1) {			// closed s
			int v0 = mp[sdiv - 1][ti + 0];		// 		v0--v3
			int v1 = mp[sdiv - 1][ti + 1];		// 		| \  |
			int v3 = mp[0][ti + 0];				// 		|  \ |
			int v2 = mp[0][ti + 1];				// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}// */
	}
	if (closed & 2) {			// closed t
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si + 0][tdiv - 1];		// 		v0--v3
			int v1 = mp[si + 0][0];				// 		| \  |
			int v3 = mp[si + 1][tdiv - 1];		// 		|  \ |
			int v2 = mp[si + 1][0];				// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
		if (closed & 1) {			// closed s
			int v0 = mp[sdiv - 1][tdiv - 1];		// 		v0--v3
			int v1 = mp[sdiv - 1][0];				// 		| \  |
			int v3 = mp[0][tdiv - 1];				// 		|  \ |
			int v2 = mp[0][0];						// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
	}// */
}

int readf(char* name) {
	FILE *f = fopen(name, "r");
	int cnt, n;
	if (f == NULL) return -1;

	fscanf(f, "%d", &cnt);
	if (fgetc(f) != ';') return 1;
	n = cnt;
	for (n = 0; n < cnt; n += 1) {
		double pos[3];
		fscanf(f, "%lf", &pos[0]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &pos[1]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &pos[2]);
		if (fgetc(f) != ';') return 1;
		switch (fgetc(f)) {
			default  : return -4;
			case ';' : if (n != cnt-1) {printf("pos: n != cnt : %d != %d\n", n, cnt); return -2;}
			case ',' : break;
		}
		if (n != addvtxdv(pos, NULL, NULL)) return -99;
	}
	for (n = 0; n < cnt; n += 1) {
		double nrm[3];
		fscanf(f, "%lf", &nrm[0]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &nrm[1]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &nrm[2]);
		if (fgetc(f) != ';') return 1;
		switch (fgetc(f)) {
			default  : return -4;
			case ';' : if (n != cnt-1) {printf("pos: n != cnt : %d != %d\n", n, cnt); return -2;}
			case ',' : break;
		}
		vecldf(&mesh.vtxptr[n].nrm, nrm[0], nrm[1], nrm[2], 0);
		//setvtx(n, NULL, nrm);
	}
	fscanf(f, "%d", &cnt);
	if (fgetc(f) != ';') return 1;
	while (cnt--) {
		unsigned n, tri[3];
		fscanf(f, "%d", &n);
		if (fgetc(f) != ';') return 1;
		if (n == 3) {
			fscanf(f, "%d", &tri[0]);
			if (fgetc(f) != ',') return 2;
			fscanf(f, "%d", &tri[1]);
			if (fgetc(f) != ',') return 2;
			fscanf(f, "%d", &tri[2]);
			if (fgetc(f) != ';') return 2;
			switch (fgetc(f)) {
				default  : return -5;
				case ',' : break;
				case ';' : if (cnt) return -3;
			}
			addtri(tri[0], tri[1], tri[2]);
		}
		else 
			fprintf(stdout, "tri : cnt:%d n:%d\n", cnt, n);
	}
	return 0;
}

int saveMesh(char* name) {
	unsigned n;
	FILE *f = fopen(name, "w");
	if (f == NULL) return -1;

	fprintf(f, "%d\n", mesh.vtxcnt);
	for (n = 0; n < mesh.vtxcnt; n += 1) {
		fprintf(f,   "%f %f %f", mesh.vtxptr[n].pos.x, mesh.vtxptr[n].pos.y, mesh.vtxptr[n].pos.z);
		fprintf(f, ", %f %f %f", mesh.vtxptr[n].nrm.x, mesh.vtxptr[n].nrm.y, mesh.vtxptr[n].nrm.z);
		fprintf(f,    ", %f %f", ((mesh.vtxptr[n].tex.s / 65535.)*2)-1, ((mesh.vtxptr[n].tex.t / 65535.)*2)-1);
		//~ fprintf(f,    ", %f %f", mesh.vtxptr[n].tex.s / 65535., mesh.vtxptr[n].tex.t / 65535.);
		//~ fprintf(f,    ", %u %u", mesh.vtxptr[n].tex.s, mesh.vtxptr[n].tex.t);
		fprintf(f, ";\n");
	}
	fprintf(f, "%d\n", mesh.tricnt);
	for (n = 0; n < mesh.tricnt; n += 1) {
		fprintf(f, "%d %d %d\n", mesh.triptr[n].i1, mesh.triptr[n].i2, mesh.triptr[n].i3);
	}
	fclose(f);
	return 0;
}

int readMesh(char* name) {
	int cnt, n;
	FILE *f = fopen(name, "r");
	if (f == NULL) return -1;
	fscanf(f, "%d", &cnt);
	for (n = 0; n < cnt; n += 1) {
		double pos[3], nrm[3], tex[2];
		fscanf(f, "%lf%lf%lf", &pos[0], &pos[1], &pos[2]);
		if (fgetc(f) != ',') return -9;
		fscanf(f, "%lf%lf%lf", &nrm[0], &nrm[1], &nrm[2]);
		if (fgetc(f) != ',') return -9;
		fscanf(f, "%lf%lf", &tex[0], &tex[1]);
		if (fgetc(f) != ';') return -9;
		if (feof(f)) break;
		addvtxdv(pos, nrm, tex);
	}
	fscanf(f, "%d", &cnt);
	for (n = 0; n < cnt; n += 1) {
		unsigned tri[3];
		fscanf(f, "%u%u%u", &tri[0], &tri[1], &tri[2]);
		//~ if (fgetc(f) != ';') return -9;
		//~ if (feof(f)) break;
		addtri(tri[0], tri[1], tri[2]);
	}
	fclose(f);
	return 0;
}

int readMobj(char* name) {
	int chr, v[3], i=0, ti = 0;
	char buff[65536];
	FILE *f = fopen(name, "r");
	if (f == NULL) return -1;
	for (;;) {
		fscanf(f, "%s", buff);
		if (feof(f)) break;
		if (*buff == 'v') {
			if (buff[1] == 't') {
				double tex_s, tex_t;
				if (ti >= 3 || (i - ti) < 0) {
					fclose(f);
					return -2;
				}
				fscanf(f, "%lf%lf", &tex_s, &tex_t);
				if (tex_s < 0 || tex_s > +1) tex_s = 0;
				if (tex_t < -1 || tex_t > 0) tex_t = 0;
				mesh.vtxptr[v[ti]].tex.s = +tex_s * (1 << 16);
				mesh.vtxptr[v[ti]].tex.t = -tex_t * (1 << 16);
				ti++;
			}
			else {
				double pos[3];
				if (i >= 3) {
					fclose(f);
					return -2;
				}
				fscanf(f, "%lf%lf%lf", &pos[0], &pos[1], &pos[2]);
				v[i++] = addvtxdv(pos, 0, 0);
			}
		}
		else if (*buff == 'f') {
			vector N;
			if (i != 3) {
				fclose(f);
				return -3;
			}
			//~ mesh.vtxptr[v[0]].pos, v[1], v[2]
			backface(&N, &mesh.vtxptr[v[0]].pos, &mesh.vtxptr[v[1]].pos, &mesh.vtxptr[v[2]].pos);
			mesh.vtxptr[v[0]].nrm = N;
			mesh.vtxptr[v[1]].nrm = N;
			mesh.vtxptr[v[2]].nrm = N;
			addtri(v[0], v[1], v[2]);
			ti = i = 0;
		}
		else /*if (*buff == '#') */ {
			while ((chr = fgetc(f)) != -1) {
				if (chr == '\n') break;
			}
		}
	}
	fclose(f);
	return 0;
}

void centmesh() {
	unsigned i;
	vector min, max, center;
	for (i = 0; i < mesh.vtxcnt; i += 1) {
		if (i == 0) {
			veccpy(&min, veccpy(&max, &mesh.vtxptr[i].pos));
		}
		else {
			vecmax(&max, &max, &mesh.vtxptr[i].pos);
			vecmin(&min, &min, &mesh.vtxptr[i].pos);
		}
	}
	vecadd(&center, &max, &min);
	vecsca(&center, &center, 1./2);
	//~ printf("min : %+f, %+f, %+f\n", min.x, min.y, min.z);
	//~ printf("max : %+f, %+f, %+f\n", max.x, max.y, max.z);
	//~ printf("cen : %+f, %+f, %+f\n", center.x, center.y, center.z);
	for (i = 0; i < mesh.vtxcnt; i += 1) {
		vecsub(&mesh.vtxptr[i].pos, &mesh.vtxptr[i].pos, &center);
		mesh.vtxptr[i].pos.w = 1;
	}
}

void optimesh() {
	unsigned i, j, k, n;
	for (n = i = 1; i < mesh.vtxcnt; i += 1) {
		for (j = 0; j < n && !vtxequ(i, j); j += 1);
		if (j == n) mesh.vtxptr[n++] = mesh.vtxptr[i];
		else vecadd(&mesh.vtxptr[j].nrm, &mesh.vtxptr[j].nrm, &mesh.vtxptr[i].nrm);
		for (k = 0; k < mesh.tricnt; k += 1) {
			if (mesh.triptr[k].i1 == i) mesh.triptr[k].i1 = j;
			if (mesh.triptr[k].i2 == i) mesh.triptr[k].i2 = j;
			if (mesh.triptr[k].i3 == i) mesh.triptr[k].i3 = j;
		}
	}
	for (j = 0; j < n; j += 1) vecnrm(&mesh.vtxptr[j].nrm, &mesh.vtxptr[j].nrm);
	mesh.vtxcnt = n;
}

//}

