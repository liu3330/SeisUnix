/* SUSYNLV: $Revision: 1.3 $ ; $Date: 92/10/22 16:43:59 $	*/

/*----------------------------------------------------------------------
 * Copyright (c) Colorado School of Mines, 1990.
 * All rights reserved.
 *
 * This code is part of SU.  SU stands for Seismic Unix, a processing line
 * developed at the Colorado School of Mines, partially based on Stanford
 * Exploration Project (SEP) software.  Inquiries should be addressed to:
 *
 *  Jack K. Cohen, Center for Wave Phenomena, Colorado School of Mines,
 *  Golden, CO 80401  (jkc@dix.mines.colorado.edu)
 *----------------------------------------------------------------------
 */
#include "su.h"
#include "segy.h"

/*********************** self documentation **********************/
char *sdoc[] = {
"									",
" SUSYNLV - SYNthetic seismograms for Linear Velocity function		",
"									",
" susynlv >outfile [optional parameters]				",
"									",
" Optional Parameters:							",
" nt=101                 number of time samples				",
" dt=0.04                time sampling interval (sec)			",
" ft=0.0                 first time (sec)				",
" nxo=1                  number of source-receiver offsets		",
" dxo=0.05               offset sampling interval (km)			",
" fxo=0.0                first offset (km, see notes below)		",
" xo=fxo,fxo+dxo,...     array of offsets (use only for non-uniform offsets)",
" nxm=101                number of midpoints (see notes below)		",
" dxm=0.05               midpoint sampling interval (km)		",
" fxm=0.0                first midpoint (km)				",
" nxs=101                number of shotpoints (see notes below)		",
" dxs=0.05               shotpoint sampling interval (km)		",
" fxs=0.0                first shotpoint (km)				",
" x0=0.0                 distance x at which v00 is specified		",
" z0=0.0                 depth z at which v00 is specified		",
" v00=2.0                velocity at x0,z0 (km/sec)			",
" dvdx=0.0               derivative of velocity with distance x (dv/dx)	",
" dvdz=0.0               derivative of velocity with depth z (dv/dz)	",
" fpeak=0.2/dt           peak frequency of symmetric Ricker wavelet (Hz)",
" ref=\"1:1,2;4,2\"        reflector(s):  \"amplitude:x1,z1;x2,z2;x3,z3;...\"",
" smooth=0               =1 for smooth (piecewise cubic spline) reflectors",
" er=0                   =1 for exploding reflector amplitudes		",
" ls=0                   =1 for line source; default is point source	",
" ob=1                   =1 to include obliquity factors		",
" tmin=10.0*dt           minimum time of interest (sec)			",
" ndpfz=5                number of diffractors per Fresnel zone		",
" verbose=1              =1 to print some useful information		",
"									",
"Notes:								",
"Offsets are signed - may be positive or negative.  Receiver locations	",
"are computed by adding the signed offset to the source location.	",
"									",
"Specify either midpoint sampling or shotpoint sampling, but not both.	",
"If neither is specified, the default is the midpoint sampling above.	",
"									",
"More than one ref (reflector) may be specified.  When obliquity factors",
"are included, then only the left side of each reflector (as the x,z	",
"reflector coordinates are traversed) is reflecting.  For example, if x	",
"coordinates increase, then the top side of a reflector is reflecting.	",
"Note that reflectors are encoded as quoted strings, with an optional	",
"reflector amplitude: preceding the x,z coordinates of each reflector.	",
"Default amplitude is 1.0 if amplitude: part of the string is omitted.	",
NULL};
/**************** end self doc ***********************************/

/*
 * Credits: CWP Dave Hale, 09/17/91,  Colorado School of Mines
 */


/* define structures */
typedef struct ReflectorSegmentStruct {
	float x;	/* x coordinate of segment midpoint */
	float z;	/* z coordinate of segment midpoint */
	float s;	/* x component of unit-normal-vector */
	float c;	/* z component of unit-normal-vector */
} ReflectorSegment;
typedef struct ReflectorStruct {
	int ns;			/* number of reflector segments */
	float ds;		/* segment length */
	float a;		/* amplitude of reflector */
	ReflectorSegment *rs;	/* array[ns] of reflector segments */
} Reflector;
typedef struct WaveletStruct {
	int lw;			/* length of wavelet */
	int iw;			/* index of first wavelet sample */
	float *wv;		/* wavelet sample values */
} Wavelet;

/* parameters for half-derivative filter */
#define LHD 20
#define NHD 1+2*LHD

/* prototypes for functions defined and used internally */
static void decodeReflectors (int *nrPtr,
	float **aPtr, int **nxzPtr, float ***xPtr, float ***zPtr);
static int decodeReflector (char *string,
	float *aPtr, int *nxzPtr, float **xPtr, float **zPtr);
static void breakReflectors (int *nr, float **ar, 
	int **nu, float ***xu, float ***zu);
static void makeref (float dsmax, int nr, float *ar, 
	int *nu, float **xu, float **zu, Reflector **r);
static void makeone (float v00, float dvdx, float dvdz, 
	int ls, int er, int ob, Wavelet *w,
	float xs, float zs, float xg, float zg,
	int nr, Reflector *r, int nt, float dt, float ft, float *trace);
static void raylv2 (float v00, float dvdx, float dvdz,
	float x0, float z0, float x, float z,
	float *c, float *s, float *t, float *q);
static void addsinc (float time, float amp,
	int nt, float dt, float ft, float *trace);
static void makericker (float fpeak, float dt, Wavelet **w);

/* segy trace */
segychdr ch;
segybhdr bh;
segy tr;

main (int argc, char **argv)
{
	int nr,er,ob,ir,ixz,ls,smooth,ndpfz,ns,
		ixo,ixsm,nxo,nxs,nxm,nt,nxsm,
		shots,midpoints,verbose,tracl,
		*nxz;
	float x0,z0,v00,dvdx,dvdz,vmin,tmin,tminr,
		x,z,v,t,dsmax,fpeak,
		dxs,dxm,dxo,dt,fxs,fxm,fxo,ft,dxsm,
		xs,zs,xg,zg,
		*trace,*xo,*ar,**xr,**zr;
	Reflector *r;
	Wavelet *w;

	/* hook up getpar to handle the parameters */
	initargs(argc,argv);
	requestdoc(0);

	/* get parameters */
	if (!getparint("nt",&nt)) nt = 101;
	if (!getparfloat("dt",&dt)) dt = 0.04;
	if (!getparfloat("ft",&ft)) ft = 0.0;
	if ((nxo=countparval("xo"))!=0) {
		xo = ealloc1float(nxo);
		getparfloat("xo",xo);
	} else {
		if (!getparint("nxo",&nxo)) nxo = 1;
		if (!getparfloat("dxo",&dxo)) dxo = 0.05;
		if (!getparfloat("fxo",&fxo)) fxo = 0.0;
		xo = ealloc1float(nxo);
		for (ixo=0; ixo<nxo; ++ixo)
			xo[ixo] = fxo+ixo*dxo;
	}
	shots = (getparint("nxs",&nxs) || 
		getparfloat("dxs",&dxs) || 
		getparfloat("fxs",&fxs));
	midpoints = (getparint("nxm",&nxm) || 
		getparfloat("dxm",&dxm) || 
		getparfloat("fxm",&fxm)); 
	if (shots && midpoints)
		err("cannot specify both shot and midpoint sampling!\n");
	if (shots) {
		if (!getparint("nxs",&nxs)) nxs = 101;
		if (!getparfloat("dxs",&dxs)) dxs = 0.05;
		if (!getparfloat("fxs",&fxs)) fxs = 0.0;
		nxsm = nxs;
		dxsm = dxs;
	} else {
		midpoints = 1;
		if (!getparint("nxm",&nxm)) nxm = 101;
		if (!getparfloat("dxm",&dxm)) dxm = 0.05;
		if (!getparfloat("fxm",&fxm)) fxm = 0.0;
		nxsm = nxm;
		dxsm = dxm;
	}
	if (!getparint("nxm",&nxm)) nxm = 101;
	if (!getparfloat("dxm",&dxm)) dxm = 0.05;
	if (!getparfloat("fxm",&fxm)) fxm = 0.0;
	if (!getparfloat("x0",&x0)) x0 = 0.0;
	if (!getparfloat("z0",&z0)) z0 = 0.0;
	if (!getparfloat("v00",&v00)) v00 = 2.0;
	if (!getparfloat("dvdx",&dvdx)) dvdx = 0.0;
	if (!getparfloat("dvdz",&dvdz)) dvdz = 0.0;
	if (!getparfloat("fpeak",&fpeak)) fpeak = 0.2/dt;
	if (!getparint("ls",&ls)) ls = 0;
	if (!getparint("er",&er)) er = 0;
	if (!getparint("ob",&ob)) ob = 1;
	if (!getparfloat("tmin",&tmin)) tmin = 10.0*dt;
	if (!getparint("ndpfz",&ndpfz)) ndpfz = 5;
	if (!getparint("smooth",&smooth)) smooth = 0;
	if (!getparint("verbose",&verbose)) verbose = 1;
	decodeReflectors(&nr,&ar,&nxz,&xr,&zr);
	if (!smooth) breakReflectors(&nr,&ar,&nxz,&xr,&zr);

	/* convert velocity v(x0,z0) to v(0,0) */
	v00 -= dvdx*x0+dvdz*z0;
	
	/* determine minimum velocity and minimum reflection time */
	for (ir=0,vmin=FLT_MAX,tminr=FLT_MAX; ir<nr; ++ir) {
		for (ixz=0; ixz<nxz[ir]; ++ixz) {
			x = xr[ir][ixz];
			z = zr[ir][ixz];
			v = v00+dvdx*x+dvdz*z;
			if (v<vmin) vmin = v;
			t = 2.0*z/v;
			if (t<tminr) tminr = t;
		}
	}

	/* determine maximum reflector segment length */
	tmin = MAX(tmin,MAX(ft,dt));
	dsmax = vmin/(2*ndpfz)*sqrt(tmin/fpeak);
 	
	/* make reflectors */
	makeref(dsmax,nr,ar,nxz,xr,zr,&r);

	/* count reflector segments */
	for (ir=0,ns=0; ir<nr; ++ir)
		ns += r[ir].ns;

	/* make wavelet */
	makericker(fpeak,dt,&w);
	
	/* if requested, print information */
	if (verbose) {
		fprintf(stderr,"\nSYNLVXZ:\n");
		fprintf(stderr,
			"Minimum possible reflection time (assuming sources\n"
			"and receivers are at the surface Z=0) is %g s.\n"
			"You may want to adjust the \"minimum time of \n"
			"interest\" parameter.\n",tminr);
		fprintf(stderr,
			"Total number of small reflecting\n"
			"segments is %d.\n",ns);
		fprintf(stderr,"\n");
	}
	
	/* create id headers and output */
        idhdrs(&ch,&bh,nt);
        puthdr(&ch,&bh);
	
	/* set constant segy trace header parameters */
	bzero(&tr,sizeof(tr));
	tr.trid = 1;
	tr.counit = 1;
	tr.ns = nt;
	tr.dt = 1.0e6*dt;
	tr.delrt = 1.0e3*ft;
	
	/* loop over shots or midpoints */
	for (ixsm=0,tracl=0; ixsm<nxsm; ++ixsm) {
	
		/* loop over offsets */
		for (ixo=0; ixo<nxo; ++ixo) {
		
			/* compute source and receiver coordinates */
			if (shots)
				xs = fxs+ixsm*dxs;
			else
				xs = fxm+ixsm*dxm-0.5*xo[ixo];
			zs = 0.0;
			xg = xs+xo[ixo];
			zg = 0.0;
			
			/* set segy trace header parameters */
			tr.tracl = tr.tracr = ++tracl;
			if (shots) {
				tr.fldr = 1+ixsm;
				tr.tracf = 1+ixo;
			} else {
				tr.cdp = 1+ixsm;
				tr.cdpt = 1+ixo;
			}
			tr.offset = NINT(1000.0*(dxsm>0.0?xo[ixo]:-xo[ixo]));
			tr.sx = NINT(1000.0*xs);
			tr.gx = NINT(1000.0*xg);
				
			/* make one trace */
			makeone(v00,dvdx,dvdz,ls,er,ob,w,
				xs,zs,xg,zg,
				nr,r,nt,dt,ft,tr.data);
			
			/* write trace */
			puttr(&tr);
		}
	}
}

/* parse reflectors parameter string */
static void decodeReflectors (int *nrPtr,
	float **aPtr, int **nxzPtr, float ***xPtr, float ***zPtr)
{
	int nr,*nxz,ir;
	float *a,**x,**z;
	char t[1024],*s;

	/* count reflectors */
	nr = countparname("ref");
	if (nr==0) nr = 1;
	
	/* allocate space */
	a = ealloc1(nr,sizeof(float));
	nxz = ealloc1(nr,sizeof(int));
	x = ealloc1(nr,sizeof(float*));
	z = ealloc1(nr,sizeof(float*));

	/* get reflectors */
	for (ir=0; ir<nr; ++ir) {
		if (!getnparstring(ir+1,"ref",&s)) s = "1:1,2;4,2";
		strcpy(t,s);
		if (!decodeReflector(t,&a[ir],&nxz[ir],&x[ir],&z[ir]))
			err("Reflector number %d specified "
				"incorrectly!\n",ir+1);
	}

	/* set output parameters before returning */
	*nrPtr = nr;
	*aPtr = a;
	*nxzPtr = nxz;
	*xPtr = x;
	*zPtr = z;
}

/* parse one reflector specification; return 1 if valid, 0 otherwise */
static int decodeReflector (char *string,
	float *aPtr, int *nxzPtr, float **xPtr, float **zPtr)
{
	int nxz,ixz;
	float a,*x,*z;
	char *s,*t;

	/* if specified, get reflector amplitude; else, use default */
	s = string;
	if (strchr(s,':')==NULL) {
		a = 1.0;
		s = strtok(s,",;\0");
	} else {
		if (strcspn(s,":")>=strcspn(s,",;\0")) return 0;
		a = atof(s=strtok(s,":"));
		s = strtok(NULL,",;\0");
	}

	/* count x and z values, while splitting string into tokens */
	for (t=s,nxz=0; t!=NULL; ++nxz)
		t = strtok(NULL,",;\0");
	
	/* total number of values must be even */
	if (nxz%2) return 0;

	/* number of (x,z) pairs */
	nxz /= 2;

	/* 2 or more (x,z) pairs are required */
	if (nxz<2) return 0;

	/* allocate space */
	x = ealloc1(nxz,sizeof(float));
	z = ealloc1(nxz,sizeof(float));

	/* convert (x,z) values */
	for (ixz=0; ixz<nxz; ++ixz) {
		x[ixz] = atof(s);
		s += strlen(s)+1;
		z[ixz] = atof(s);
		s += strlen(s)+1;
	}

	/* set output parameters before returning */
	*aPtr = a;
	*nxzPtr = nxz;
	*xPtr = x;
	*zPtr = z;
	return 1;
}

/* Break up reflectors by duplicating interior (x,z) points */
static void breakReflectors (int *nr, float **ar, 
	int **nu, float ***xu, float ***zu)
{
	int nri,nro,*nui,*nuo,ir,jr,iu;
	float *ari,*aro,**xui,**zui,**xuo,**zuo;

	/* input reflectors */
	nri = *nr;
	ari = *ar;
	nui = *nu;
	xui = *xu;
	zui = *zu;

	/* number of output reflectors */
	for (ir=0,nro=0; ir<nri; ++ir)
		nro += nui[ir]-1;

	/* make output reflectors and free space for input reflectors */
	aro = ealloc1float(nro);
	nuo = ealloc1int(nro);
	xuo = ealloc1(nro,sizeof(float*));
	zuo = ealloc1(nro,sizeof(float*));
	for (ir=0,jr=0; ir<nri; ++ir) {
		for (iu=0; iu<nui[ir]-1; ++iu,++jr) {
			aro[jr] = ari[ir];
			nuo[jr] = 2;
			xuo[jr] = ealloc1float(2);
			zuo[jr] = ealloc1float(2);
			xuo[jr][0] = xui[ir][iu];
			zuo[jr][0] = zui[ir][iu];
			xuo[jr][1] = xui[ir][iu+1];
			zuo[jr][1] = zui[ir][iu+1];
		}
		free1float(xui[ir]);
		free1float(zui[ir]);
	}
	free1float(ari);
	free1int(nui);
	free1(xui);
	free1(zui);

	/* output reflectors */
	*nr = nro;
	*ar = aro;
	*nu = nuo;
	*xu = xuo;
	*zu = zuo;
}

static void makeref (float dsmax, int nr, float *ar, 
	int *nu, float **xu, float **zu, Reflector **r)
/*****************************************************************************
Make piecewise cubic reflectors
******************************************************************************
Input:
dsmax		maximum length of reflector segment
nr		number of reflectors
ar		array[nr] of reflector amplitudes
nu		array[nr] of numbers of (x,z) pairs; u = 0, 1, ..., nu[ir]
xu		array[nr][nu[ir]] of reflector x coordinates x(u)
zu		array[nr][nu[ir]] of reflector z coordinates z(u)

Output:
r		array[nr] of reflectors
******************************************************************************
Notes:
Space for the ar, nu, xu, and zu arrays is freed by this function, since
they are only used to construct the reflectors.

This function is meant to be called only once, so it need not be very
efficient.  Once made, the reflectors are likely to be used many times, 
so the cost of making them is insignificant.
*****************************************************************************/
{
	int ir,iu,nuu,iuu,ns,is;
	float x,z,xlast,zlast,dx,dz,duu,uu,ds,fs,rsx,rsz,rsxd,rszd,
		*u,*s,(*xud)[4],(*zud)[4],*us;
	ReflectorSegment *rs;
	Reflector *rr;
	
	/* allocate space for reflectors */
	*r = rr = ealloc1(nr,sizeof(Reflector));

	/* loop over reflectors */
	for (ir=0; ir<nr; ++ir) {

		/* compute cubic spline coefficients for uniformly sampled u */
		u = ealloc1float(nu[ir]);
		for (iu=0; iu<nu[ir]; ++iu)
			u[iu] = iu;
		xud = (float(*)[4])ealloc1float(4*nu[ir]);
		csplin(nu[ir],u,xu[ir],xud);
		zud = (float(*)[4])ealloc1float(4*nu[ir]);
		csplin(nu[ir],u,zu[ir],zud);

		/* finely sample x(u) and z(u) and compute length s(u) */
		nuu = 20*nu[ir];
		duu = (u[nu[ir]-1]-u[0])/(nuu-1);
		s = ealloc1float(nuu);
		s[0] = 0.0;
		xlast = xu[ir][0];
		zlast = zu[ir][0];
		for (iuu=0,uu=0.0,s[0]=0.0; iuu<nuu; ++iuu,uu+=duu) {
			intcub(0,nu[ir],u,xud,1,&uu,&x);
			intcub(0,nu[ir],u,zud,1,&uu,&z);
			dx = x-xlast;
			dz = z-zlast;
			s[iuu] = s[iuu-1]+sqrt(dx*dx+dz*dz);
			xlast = x;
			zlast = z;
		}

		/* compute u(s) from s(u) */
		ns = 1+s[nuu-1]/dsmax;
		ds = s[nuu-1]/ns;
		fs = 0.5*ds;
		us = ealloc1float(ns);
		yxtoxy(nuu,duu,0.0,s,ns,ds,fs,0.0,(float)(nu[ir]-1),us);

		/* compute reflector segments uniformly sampled in s */
		rs = ealloc1(ns,sizeof(ReflectorSegment));
		for (is=0; is<ns; ++is) {
			intcub(0,nu[ir],u,xud,1,&us[is],&rsx);
			intcub(0,nu[ir],u,zud,1,&us[is],&rsz);
			intcub(1,nu[ir],u,xud,1,&us[is],&rsxd);
			intcub(1,nu[ir],u,zud,1,&us[is],&rszd);
			rs[is].x = rsx;
			rs[is].z = rsz;
			rs[is].c = rsxd/sqrt(rsxd*rsxd+rszd*rszd);
			rs[is].s = -rszd/sqrt(rsxd*rsxd+rszd*rszd);
		}
		
		/* fill in reflector structure */
		rr[ir].ns = ns;
		rr[ir].ds = ds;
		rr[ir].a = ar[ir];
		rr[ir].rs = rs;

		/* free workspace */
		free1float(us);
		free1float(s);
		free1float(u);
		free1float((float*)xud);
		free1float((float*)zud);

		/* free space replaced by reflector segments */
		free1(xu[ir]);
		free1(zu[ir]);
	}

	/* free space replaced by reflector segments */
	free1(nu);
	free1(xu);
	free1(zu);
}

static void makeone (float v00, float dvdx, float dvdz, 
	int ls, int er, int ob, Wavelet *w,
	float xs, float zs, float xg, float zg,
	int nr, Reflector *r, int nt, float dt, float ft, float *trace)
/*****************************************************************************
Make one synthetic seismogram for linear velocity v(x,z) = v00+dvdx*x+dvdz*z
******************************************************************************
Input:
v00		velocity v at (x=0,z=0)
dvdx		derivative dv/dx of velocity v with respect to x
dvdz		derivative dv/dz of velocity v with respect to z
ls		=1 for line source amplitudes; =0 for point source
er		=1 for exploding, =0 for normal reflector amplitudes
ob		=1 to include cos obliquity factors; =0 to omit
w		wavelet to convolve with trace
xs		x coordinate of source
zs		z coordinate of source
xg		x coordinate of receiver group
zg		z coordinate of receiver group
nr		number of reflectors
r		array[nr] of reflectors
nt		number of time samples
dt		time sampling interval
ft		first time sample

Output:
trace		array[nt] containing synthetic seismogram
*****************************************************************************/
{
	int it,ir,is,ns;
	float ar,ds,xd,zd,cd,sd,vs,vg,vd,cs,ss,ts,qs,cg,sg,tg,qg,
		ci,cr,time,amp,*temp;
	ReflectorSegment *rs;
	int lhd=LHD,nhd=NHD;
	static float hd[NHD];
	static int madehd=0;
	
	/* if half-derivative filter not yet made, make it */
	if (!madehd) {
		mkhdiff(dt,lhd,hd);
		madehd = 1;
	}

	/* zero trace */
	for (it=0; it<nt; ++it)
		trace[it] = 0.0;
	
	/* velocities at source and receiver */
	vs = v00+dvdx*xs+dvdz*zs;
	vg = v00+dvdx*xg+dvdz*zg;

	/* loop over reflectors */
	for (ir=0; ir<nr; ++ir) {

		/* amplitude, number of segments, segment length */
		ar = r[ir].a;
		ns = r[ir].ns;
		ds = r[ir].ds;
		rs = r[ir].rs;
	
		/* loop over diffracting segments */
		for (is=0; is<ns; ++is) {
		
			/* diffractor midpoint, unit-normal, and length */
			xd = rs[is].x;
			zd = rs[is].z;
			cd = rs[is].c;
			sd = rs[is].s;
			
			/* velocity at diffractor */
			vd = v00+dvdx*xd+dvdz*zd;

			/* ray from shot to diffractor */
			raylv2(v00,dvdx,dvdz,xs,zs,xd,zd,&cs,&ss,&ts,&qs);

			/* ray from receiver to diffractor */
			raylv2(v00,dvdx,dvdz,xg,zg,xd,zd,&cg,&sg,&tg,&qg);

			/* cosines of incidence and reflection angles */
			if (ob) {
				ci = cd*cs+sd*ss;
				cr = cd*cg+sd*sg;
			} else {
				ci = 1.0;
				cr = 1.0;
			}

			/* if either cosine is negative, skip diffractor */
			if (ci<0.0 || cr<0.0) continue;

			/* two-way time and amplitude */
			time = ts+tg;
			if (er) {
				amp = sqrt(vg*vd/qg);
			} else {
				if (ls)
					amp = sqrt((vs*vd*vd*vg)/(qs*qg));
				else
					amp = sqrt((vs*vd*vd*vg)/
						(qs*qg*(qs+qg)));
			}
			amp *= (ci+cr)*ar*ds;
				
			/* add sinc wavelet to trace */
			addsinc(time,amp,nt,dt,ft,trace);
		}
	}
	
	/* allocate workspace */
	temp = ealloc1float(nt);
	
	/* apply half-derivative filter to trace */
	conv(nhd,-lhd,hd,nt,0,trace,nt,0,temp);

	/* convolve wavelet with trace */
	conv(w->lw,w->iw,w->wv,nt,0,temp,nt,0,trace);
	
	/* free workspace */
	free1float(temp);
}

static void raylv2 (float v00, float dvdx, float dvdz,
	float x0, float z0, float x, float z,
	float *c, float *s, float *t, float *q)
/*****************************************************************************
Trace ray between two points, for linear velocity v = v00+dvdx*x+dvdz*z
******************************************************************************
Input:
v00		velocity at (x=0,z=0)
dvdx		derivative dv/dx of velocity with respect to x
dvdz		derivative dv/dz of velocity with respect to z
x0		x coordinate at beginning of ray
z0		z coordinate at beginning of ray
x		x coordinate at end of ray
z		z coordinate at end of ray

Output:
c		cosine of propagation angle at end of ray
s		sine of propagation angle at end of ray
t		time along raypath
q		integral of velocity along raypath
*****************************************************************************/
{
	float a,oa,v0,v,cr,sr,r,or,c0,mx,temp;
	
	/* if (x,z) same as (x0,z0) */
	if (x==x0 && z==z0) {
		*c = 1.0;
		*s = 0.0;
		*t = 0.0;
		*q = 0.0;;
		return;
	}

	/* if constant velocity */
	if (dvdx==0.0 && dvdz==0.0) {
		x -= x0;
		z -= z0;
		r = sqrt(x*x+z*z);
		or = 1.0/r;
		*c = z*or;
		*s = x*or;
		*t = r/v00;
		*q = r*v00;
		return;
	}

	/* if necessary, rotate coordinates so that v(x,z) = v0+a*(z-z0) */
	if (dvdx!=0.0) {
		a = sqrt(dvdx*dvdx+dvdz*dvdz);
		oa = 1.0/a;
		cr = dvdz*oa;
		sr = dvdx*oa;
		temp = cr*x0-sr*z0;
		z0 = cr*z0+sr*x0;
		x0 = temp;
		temp = cr*x-sr*z;
		z = cr*z+sr*x;
		x = temp;
	} else {
		a = dvdz;
	}

	/* velocities at beginning and end of ray */
	v0 = v00+a*z0;
	v = v00+a*z;
	
	/* if either velocity not positive */
	if (v0<0.0 || v<0.0) {
		*c = 1.0;
		*s = 0.0;
		*t = FLT_MAX;
		*q = FLT_MAX;
		return;
	}

	/* translate (x,z) */
	x -= x0;
	z -= z0;
	
	/* if raypath is parallel to velocity gradient */
	if (x*x<0.000001*z*z) {

		/* if ray is going down */
		if (z>0.0) {
			*s = 0.0;
			*c = 1.0;
			*t = log(v/v0)/a;
			*q = 0.5*z*(v+v0);
		
		/* else, if ray is going up */
		} else {
			*s = 0.0;
			*c = -1.0;
			*t = -log(v/v0)/a;
			*q = -0.5*z*(v+v0);
		}
	
	/* else raypath is circular with finite radius */
	} else {

		/* save translated -x to avoid roundoff error below */
		mx = -x;
		
		/* translate to make center of circular raypath at (0,0) */
		z0 = v0/a;
		z += z0;
		x0 = -(x*x+z*z-z0*z0)/(2.0*x);
		x += x0;
		
		/* signed radius of raypath; if ray going clockwise, r > 0 */
		if (a>0.0 && mx>0.0 || a<0.0 && mx<0.0)
			r = sqrt(x*x+z*z);
		else
			r = -sqrt(x*x+z*z);
		
		/* cosine at (x0,z0), cosine and sine at (x,z) */
		or = 1.0/r;
		c0 = x0*or;
		*c = x*or;
		*s = -z*or;

		/* time along raypath */
		*t = log((v*(1.0+c0))/(v0*(1.0+(*c))))/a;
		if ((*t)<0.0) *t = -(*t);

		/* integral of velocity along raypath */
		*q = a*r*mx;
	}

	/* if coordinates were rotated, unrotate cosine and sine */
	if (dvdx!=0.0) {
		temp = cr*(*s)+sr*(*c);
		*c = cr*(*c)-sr*(*s);
		*s = temp;
	}
}

static void addsinc (float time, float amp,
	int nt, float dt, float ft, float *trace)
/*****************************************************************************
Add sinc wavelet to trace at specified time and with specified amplitude
******************************************************************************
Input:
time		time at which to center sinc wavelet
amp		peak amplitude of sinc wavelet
nt		number of time samples
dt		time sampling interval
ft		first time sample
trace		array[nt] containing sample values

Output:
trace		array[nt] with sinc added to sample values
*****************************************************************************/
{
	static float sinc[101][8];
	static int nsinc=101,madesinc=0;
	int jsinc;
	float frac;
	int itlo,ithi,it,jt;
	float tn,*psinc;

	/* if not made sinc coefficients, make them */
	if (!madesinc) {
		for (jsinc=1; jsinc<nsinc-1; ++jsinc) {
			frac = (float)jsinc/(float)(nsinc-1);
			mksinc(frac,8,sinc[jsinc]);
		}
		for (jsinc=0; jsinc<8; ++jsinc)
			sinc[0][jsinc] = sinc[nsinc-1][jsinc] = 0.0;
		sinc[0][3] = 1.0;
		sinc[nsinc-1][4] = 1.0;
		madesinc = 1;
	}
	tn = (time-ft)/dt;
	jt = tn;
	jsinc = (tn-jt)*(nsinc-1);
	itlo = jt-3;
	ithi = jt+4;
	if (itlo>=0 && ithi<nt) {
		psinc = sinc[jsinc];
		trace[itlo] += amp*psinc[0];
		trace[itlo+1] += amp*psinc[1];
		trace[itlo+2] += amp*psinc[2];
		trace[itlo+3] += amp*psinc[3];
		trace[itlo+4] += amp*psinc[4];
		trace[itlo+5] += amp*psinc[5];
		trace[itlo+6] += amp*psinc[6];
		trace[itlo+7] += amp*psinc[7];
	} else if (ithi>=0 && itlo<nt) {
		if (itlo<0) itlo = 0;
		if (ithi>=nt) ithi = nt-1;
		psinc = sinc[jsinc]+itlo-jt+3;
		for (it=itlo; it<=ithi; ++it)
			trace[it] += amp*(*psinc++);
	}
}

static void makericker (float fpeak, float dt, Wavelet **w)
/*****************************************************************************
Make Ricker wavelet
******************************************************************************
Input:
fpeak		peak frequency of wavelet
dt		time sampling interval

Output:
w		Ricker wavelet
*****************************************************************************/
{
	int iw,lw,it,jt;
	float t,x,*wv;
	
	iw = -(1+1.0/(fpeak*dt));
	lw = 1-2*iw;
	wv = ealloc1float(lw);
	for (it=iw,jt=0,t=it*dt; jt<lw; ++it,++jt,t+=dt) {
		x = PI*fpeak*t;
		x = x*x;
		wv[jt] = exp(-x)*(1.0-2.0*x);
	}
	*w = ealloc1(1,sizeof(Wavelet));
	(*w)->lw = lw;
	(*w)->iw = iw;
	(*w)->wv = wv;
}
