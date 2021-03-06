/* Copyright (c) Colorado School of Mines, 2011.*/
/* All rights reserved.                       */

/*----------------------------------------------------------------------
 * 
 *
 * WARNING: These selfdoc routines are obsolete as of 28 Oct 1992, but
 *       but have been kept to provide backward compatibility with older code.
 *	 The routines 'requestdoc()' and 'pagedoc()' found in newdocpkge.c
 *	 replace 'askdoc()' and 'selfdoc()', respectively, in this package.
 *
 */

/*----------------------------------------------------------------------
 *
 * Copyright (c) Colorado School of Mines, 1989.
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

#include "par.h"

/* askdoc - give selfdoc on user request
 *
 * Synopsis:
 *	void askdoc(flag);
 *	int flag;
 *
 * Credit:
 *	CWP: Shuki, Jack
 *	HRC: Lyle
 *
 * Note:
 *	In the usual case, stdin is used to pass in data.  However,
 *	some programs (eg. synthetic data generators) don't use stdin
 *	to pass in data and some programs require two or more arguments
 *	besides the command itself (eg. sudiff) and don't use stdin.
 *	In this last case, we give selfdoc whenever too few arguments
 *	are given, since these usages violate the usual SU syntax.
 *	In all cases, selfdoc can be requested by giving only the
 *	program name.
 *
 *	The flag argument distinguishes these cases:
 *		flag = 0; fully defaulted, no stdin
 *		flag = 1; usual case
 *		flag = n > 1; no stdin and n extra args required
 *
 *
 */

void askdoc(int flag)
{
	switch(flag) {
	case 1:
		if (xargc == 1 && isatty(STDIN)) selfdoc();
	break;
	case 0:
		if (xargc == 1 && isatty(STDIN) && isatty(STDOUT)) selfdoc();
	break;
	default:
		if (xargc <= flag) selfdoc();
	}
	return;
}

/*----------------------------------------------------------------------
 * Copyright (c) Colorado School of Mines, 1989.
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

#include "par.h"

/* selfdoc - print self documentation string
 *
 * Returns:
 *	void
 *
 * Synopsis:
 *	void selfdoc()
 *
 * Credits:
 *	SEP: Einar, Stew
 *	CWP: Jack, Shuki
 *
 * Example:
 *	if (xargc != 3) selfdoc();
 *
 *
 */


void selfdoc(void)
{
	extern char *sdoc;
	FILE *fp;

	efflush(stdout);
	fp = epopen("more -22 1>&2", "w");
	(void) fprintf(fp, "%s", sdoc);
	epclose(fp);

	exit(EXIT_FAILURE);
}
