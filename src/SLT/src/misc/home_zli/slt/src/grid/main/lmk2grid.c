/* LMK2GRID program */
#include "usgrid.h"
#include "par.h"

char *sdoc = 
"LMK2GRID - Landmark to grid program					\n"
"\n"
"lmk2grid <landmark.file [parameters] >grid.file			\n" 
"\n"
"Required parameters:							\n"
"landmark.file=     name of Landmark pick file 				\n"
"grid.file=         name of grid file 					\n"
"fx=                first x to output grid				\n"
"nx=                number of x to output grid				\n"
"dx=                increment of x to output grid			\n"
"fy=                first y to output grid				\n"
"ny=                number of y to output grid				\n"
"dy=                increment of y to output grid			\n"
"Optional parameters:							\n"
"ib=0               boundary mode:					\n"
"                   0=use bv outside input region			\n"
"                   1=zero-slope extrapolation outside input region	\n"
"                   -1=constant-slope extrapolation outside input region	\n"
"bv=0               grid value to be used outside landmark pick boundaries \n"
"xpos=2             colume position of landmark x picks 		\n"
"ypos=1             colume position of landmark y picks 		\n"
"tpos=5             colume position of landmark t picks 		\n"
"maxp=250000        maximum number of rows in the landmark pick file	\n"
"interp=2           interpolation mode					\n"
"                   2=linear along x then y				\n"
"                   1=linear along x only 				\n"
"                   0=no interpolation					\n"
"maxgap=2000        max gap (in data points) to interpolate missing data \n"
"extendx=0          number of points extended laterally beyong the 	\n"
"                   edge of input region (via linear extrapolation)	\n"
"                   along x direction					\n"
"extendy=0          number of points extended laterally beyong the 	\n"
"                   edge of input region (via linear extrapolation)	\n"
"                   along y direction					\n"
"extend=0           number of points extended laterally beyong the 	\n"
"                   edge of input region (via linear extrapolation)	\n"
"                   if extendx or extendy is not specified, it will 	\n"
"                   use extend						\n"
"ixyext=1           order of extending (0=x then y; 1=y then x)		\n"
"NOTES:						 			\n"
"\n"
"AUTHOR:		Zhiming Li,       ,	8/25/94   		\n"
;


void lmkread(FILE *infp, float *xs, float *ys, float *ts, int *np, int maxp,
	int xpos, int ypos, int tpos);
void extx(float *grids,int nx,int ny,int extend,float bv,
	float *gmin,float *gmax,int ib);
void exty(float *grids,int nx,int ny,int extend,float bv,
	float *gmin,float *gmax,int ib);

main(int argc, char **argv)
{
	FILE *infp=stdin, *outfp=stdout;

	int maxp=250000,xpos=2,ypos=1,tpos=5;
	int nx,ny,nt,np,ib,ibb,interp;
	int maxgap;
	float dx,dy,fx,fy,x,y,bv;
	usghed usgh;

	float *xs, *ys, *ts;
	int ierr;
	float *grids;
	int ix, iy;

	float *a, *b, *c, *ti, *xi, *yi, *xmin, *xmax;
	int *nxi, *ilive, nmax;

	float gmin, gmax;

	int noinput=0, extend=0;
	float slope;
	int ixx, iyy, i2, jy0;
	int extendx, extendy;
	int ixyext=1;

    	/* initialization */
    	initargs(argc,argv);
    	askdoc(1);

	/* get input parameters */

	if( !getparfloat("fx",&fx) ) err("fx missing"); 
	if( !getparfloat("dx",&dx) ) err("dx missing"); 
	if( !getparint("nx",&nx) ) err("nx missing"); 
	if( !getparfloat("fy",&fy) ) err("fy missing"); 
	if( !getparfloat("dy",&dy) ) err("dy missing"); 
	if( !getparint("ny",&ny) ) err("ny missing"); 
	if( !getparint("xpos",&xpos) ) xpos=2; xpos -= 1;
	if( !getparint("ypos",&ypos) ) ypos=1; ypos -= 1;
	if( !getparint("tpos",&tpos) ) tpos=5; tpos -= 1;
	if( !getparint("maxp",&maxp) ) maxp=250000;
	if( !getparint("ib",&ib) ) ib=0;
	ibb = ib; if(ib==-1) ibb = 1;
	if( !getparint("interp",&interp) ) interp=2;
	if( !getparint("maxgap",&maxgap) ) maxgap=2000;
	if( !getparfloat("bv",&bv) ) bv=0.;
	if( !getparint("extend",&extend) ) extend = 0;
	if( !getparint("extendx",&extendx) ) extendx = extend;
	if( !getparint("extendy",&extendy) ) extendy = extend;
	if( !getparint("ixyext",&ixyext) ) ixyext = 1;

	if(extend>0 && ib==-1) {
		extendx = 0;
		extendy = 0;
	}

	nmax = nx;
	if(nx<ny) nmax = ny;

	/* memory allocations */
        xs = (float*)malloc(maxp*sizeof(int));
        ys = (float*)malloc(maxp*sizeof(int));
        ts = (float*)malloc(maxp*sizeof(float));
        grids = (float*)malloc(nx*ny*sizeof(float));
        xi = (float*)malloc((nx+2)*(ny+2)*sizeof(float));
        yi = (float*)malloc((nx+2)*(ny+2)*sizeof(float));
        ti = (float*)malloc((nx+2)*(ny+2)*sizeof(float));
        ilive = (int*)malloc((nx+2)*(ny+2)*sizeof(int));
        a = (float*)malloc((nmax+2)*sizeof(float));
        b = (float*)malloc((nmax+2)*sizeof(float));
        c = (float*)malloc((nmax+2)*sizeof(float));
        nxi = (int*)malloc((ny+2)*sizeof(int));
        xmin = (float*)malloc((ny+2)*sizeof(float));
        xmax = (float*)malloc((ny+2)*sizeof(float));

        /* read in SC3D cards */
        lmkread(infp,xs,ys,ts,&np,maxp,xpos,ypos,tpos);

	fprintf(stderr," total of %d picks read from input \n",np);
	for(ix=0;ix<np;ix++) fprintf(stderr," xs=%g ys=%g ts=%g \n", xs[ix],ys[ix],ts[ix]);


	/* land mark to grid conversion */
	l2g_(xs,ys,ts,&np,&fx,&dx,&nx,&fy,&dy,&ny,grids,
             &ibb,&bv,xi,yi,ti,ilive,a,b,c,
             nxi,xmin,xmax,&nmax,&interp,&maxgap,&gmin,&gmax);

/*
	for(ix=0;ix<nx*ny;ix++) {
	if(grids[ix]!=0.) fprintf(stderr," grids=%g ix=%d \n",grids[ix],ix);
	}
*/

	if(extendx>0 || extendy>0) {
		if(ixyext==0) {
			if(extendx>0) extx(grids,nx,ny,extendx,bv,&gmin,&gmax,ib);
			if(extendy>0) exty(grids,nx,ny,extendy,bv,&gmin,&gmax,ib);
		} else {
			if(extendy>0) exty(grids,nx,ny,extendy,bv,&gmin,&gmax,ib);
			if(extendx>0) extx(grids,nx,ny,extendx,bv,&gmin,&gmax,ib);
		}
	}

	fprintf(stderr," landmark to grid conversion done \n");

	np = nx * ny;
	
	/* update grid header */
	bzero(&usgh,100);
	usgh.n1 = nx; usgh.o1 = fx; usgh.d1 = dx;
	usgh.n2 = ny; usgh.o2 = fy; usgh.d2 = dy;
	usgh.n3 = 1; usgh.o3 = 0; usgh.d3 = 1;
	usgh.n4 = 1; usgh.n5 = 1; usgh.scale = 1.e-6;
	usgh.dtype = 4; usgh.gmin = gmin; usgh.gmax = gmax;
	usgh.d4 = usgh.d5 = usgh.o4 = usgh.o5 = 0.;
	usgh.orient = usgh.gtype = 0;
	
	/* output grid */
	efwrite(grids,sizeof(float),np,outfp);
	ierr = fputusghdr(outfp, &usgh);
	if(ierr!=0) err("error output grid header ");

	exit(0);
}

void lmkread(FILE *infp, float *xs, float *ys, float *ts, int *np, int maxp,
	int xpos, int ypos, int tpos) {

        int ip, nc=200;
        char *cbuf;
	float *fbuf;

        cbuf = (char *) emalloc(nc*sizeof(char));
        fbuf = (float *) emalloc(10*sizeof(float));

        /* rewind infp */
        efseek(infp,0,0);

        for (ip=0;ip<maxp;ip++) {
                if(feof(infp) !=0) break;
                bzero(cbuf,nc);
                fgets(cbuf,nc,infp);
		sscanf(cbuf,"%f %f %f %f %f",
			&fbuf[0],&fbuf[1],&fbuf[2],&fbuf[3],&fbuf[4]);
		xs[ip] = fbuf[xpos];
		ys[ip] = fbuf[ypos];
		ts[ip] = fbuf[tpos];
        }
	*np = ip - 1; 
        free(cbuf);
        free(fbuf);
}

void extx(float *grids,int nx,int ny,int extend,float bv,
	float *gmin,float *gmax,int ib) {
   int iy, jy0;
   int ix,ixx,i2;
   float slope;

   for(iy=0;iy<ny;iy++) {
	jy0 = iy*nx;
	for(ix=0;ix<nx-1;ix++) {
		if(grids[ix+jy0]!=bv) break;
		if(grids[ix+jy0]==bv && grids[ix+1+jy0]!=bv) {
			if(ix<nx-3) {
				if(grids[ix+2+jy0]!=bv) {
					slope = grids[ix+2+jy0]
					      - grids[ix+1+jy0]; 
				} else {
					slope = 0.;
				}
			} else {
				slope = 0.;
			}
			if(ib==1) slope = 0;
			for(i2=0;i2<extend;i2++) {
				ixx = ix - i2;
				if(ixx>=0 && ixx<nx) {
				   	grids[ixx+jy0] = grids[ix+1+jy0]
						- (i2+1)*slope;
				   	if(grids[ixx+jy0]>*gmax)
						*gmax = grids[ixx+jy0];
				   	if(grids[ixx+jy0]>*gmin)
						*gmin = grids[ixx+jy0];
				}
			}
			break;
		}
	}
	for(ix=nx-1;ix>0;ix--) {
		if(grids[ix+jy0]!=bv) break;
		if(grids[ix+jy0]==bv && grids[ix-1+jy0]!=bv) {
			if(ix>1) {
				if(grids[ix-2+jy0]!=bv) {
					slope = grids[ix-1+jy0]
					      - grids[ix-2+jy0]; 
				} else {
					slope = 0.;
				}
			} else {
				slope = 0.;
			}
			if(ib==1) slope = 0;
			for(i2=0;i2<extend;i2++) {
				ixx = ix + i2;
				if(ixx>=0 && ixx<nx) {
			   		grids[ixx+jy0] = grids[ix-1+jy0]
						+ (i2+1)*slope;
					if(grids[ixx+jy0]>*gmax)
                                		*gmax = grids[ixx+jy0];
                                	if(grids[ixx+jy0]>*gmin)
                                        	*gmin = grids[ixx+jy0];
				}
			}
			break;
		}
	}
   }
}

void exty(float *grids,int nx,int ny,int extend,float bv,
	float *gmin,float *gmax,int ib) {
   int iy,iyy;
   int ix,ixx,i2;
   float slope;
   for(ix=0;ix<nx;ix++) {
	for(iy=0;iy<ny-1;iy++) {
		if(grids[ix+iy*nx]!=bv) break;
		if(grids[ix+iy*nx]==bv && grids[ix+(iy+1)*nx]!=bv) {
			if(iy<ny-3) {
				if(grids[ix+(iy+2)*nx]!=bv) {
					slope = grids[ix+(2+iy)*nx]
					      - grids[ix+(1+iy)*nx]; 
				} else {
					slope = 0.;
				}
			} else {
				slope = 0.;
			}
			if(ib==1) slope = 0;
			for(i2=0;i2<extend;i2++) {
				iyy = iy - i2;
				if(iyy>=0 && iyy<ny) {
				   	grids[ix+iyy*nx]=grids[ix+(iy+1)*nx]
						- (i2+1)*slope;
				   	if(grids[ix+iyy*nx]>*gmax)
						*gmax = grids[ix+iyy*nx];
				   	if(grids[ix+iyy*nx]<*gmin)
						*gmin = grids[ix+iyy*nx];
				}
			}
			break;
		}
	}
	for(iy=ny-1;iy>0;iy--) {
		if(grids[ix+iy*nx]!=bv) break;
		if(grids[ix+iy*nx]==bv && grids[ix+(iy-1)*nx]!=bv) {
			if(iy>1) {
				if(grids[ix+(iy-2)*nx]!=bv) {
					slope = grids[ix+(iy-1)*nx]
					      - grids[ix+(iy-2)*nx]; 
				} else {
					slope = 0.;
				}
			} else {
				slope = 0.;
			}
			if(ib==1) slope = 0;
			for(i2=0;i2<extend;i2++) {
				iyy = iy + i2;
				if(iyy>=0 && iyy<ny) {
				   	grids[ix+iyy*nx]=grids[ix+(iy-1)*nx]
						+ (i2+1)*slope;
				   	if(grids[ix+iyy*nx]>*gmax)
						*gmax = grids[ix+iyy*nx];
				   	if(grids[ix+iyy*nx]<*gmin)
						*gmin = grids[ix+iyy*nx];
				}
			}
			break;
		}
	}
   }
}
