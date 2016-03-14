/* Copyright (c) Colorado School of Mines, 1990.
/* All rights reserved.                       */

/*
FUNCTION:  printer plot of a 1-dimensional array

PARAMETERS:
fp			i file pointer for output (e.g., stdout, stderr, etc.)
title		i title of plot
lx			i length of x
ifx			i index of first x
x			i array to be plotted

AUTHOR:  Dave Hale, Colorado School of Mines, 06/02/89
*/

#include "cwp.h"

#define IFC 0
#define ILC 60

void pp1d (FILE *fp, char *title, int lx, int ifx, float x[])
{
	int ilx=ifx+lx-1,ix,ibase,icx,ic;
	float xmin,xmax,xscale,xbase;

	/* print title */
	fprintf(fp,"\n");
	fprintf(fp,"%s\n",title);

	/* minimum and maximum x */
	xmin = x[0];
	xmax = x[0];
	for (ix=1; ix<lx; ix++) {
		xmin = MIN(xmin,x[ix]);
		xmax = MAX(xmax,x[ix]);
	}
	fprintf(fp,"minimum = %g\n",xmin);
	fprintf(fp,"maximum = %g\n",xmax);

	/* determine scale factors and shifts for converting x values to *s */
	if (xmin==xmax)
		xscale = 1.0;
	else
		xscale = (ILC-IFC)/(xmax-xmin);
	if (xmin<0.0 && xmax<0.0) {
		ibase = ILC;
		xbase = xmax;
	} else if (xmin<0.0 && xmax>=0.0) {
		ibase = IFC+(0.0-xmin)*xscale;
		xbase = 0.0;
	} else {
		ibase = IFC;
		xbase = xmin;
	}

	/* loop over x values */
	for (ix=0; ix<lx; ix++) {

		/* determine column corresponding to x value */
		icx = ibase+NINT((x[ix]-xbase)*xscale);
		icx = MAX(IFC,MIN(ILC,icx));

		/* print the index, x value, and row of *s */
		fprintf(fp,"%4d %13.6e ",ifx+ix,x[ix]);
		for (ic=IFC; ic<MIN(ibase,icx); ic++)
			fprintf(fp," ");
		for (ic=MIN(ibase,icx); ic<=MAX(ibase,icx); ic++)
			fprintf(fp,"*");
		fprintf(fp,"\n");
	}
}
