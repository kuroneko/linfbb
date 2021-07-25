/************************************************************************
	Copyright (C) 1986-2000	by Jean-Paul Roubelat (F6FBB) <jpr@f6fbb.org>
	Copyright (C) 2000-2020	by Bernard Pidoux (F6BVP)     <f6bvp@free.fr>
	Copyright (C) 2020-2021	by Dave van der Locht       <dave.is@home.nl>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	Parts of code have been taken from many other softwares.
	Thanks for the help.
************************************************************************/

/* 
 * This code is slightly modified from the procps package.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "sysinfo.h"


#define UPTIME_FILE  "/proc/uptime"
#define LOADAVG_FILE "/proc/loadavg"
#define MEMINFO_FILE "/proc/meminfo"

static char buf[800];

/* This macro opens FILE only if necessary and seeks to 0 so that successive
   calls to the functions are more efficient.  It also reads the current
   contents of the file into the global buf.
*/
#define FILE_TO_BUF(FILE) {									\
	static int n, fd = -1;									\
	if (fd == -1 && (fd = open(FILE, O_RDONLY)) == -1) {	\
	fprintf(stderr, "%s %d\n", FILE, errno);				\
	close(fd);												\
	return 0;												\
	}														\
	lseek(fd, 0L, SEEK_SET);								\
	if ((n = read(fd, buf, sizeof buf - 1)) < 0) {			\
	fprintf(stderr, "%s %d\n", FILE, errno);				\
	close(fd);												\
	fd = -1;												\
	return 0;												\
	}														\
	buf[n] = '\0';											\
}

#define SET_IF_DESIRED(x,y)  if (x) *(x) = (y)	/* evals 'x' twice */

int uptime(double *uptime_secs, double *idle_secs) {
	double up=0, idle=0;

	FILE_TO_BUF(UPTIME_FILE)

	if (sscanf(buf, "%lf %lf", &up, &idle) < 2) 
	{
		fprintf(stderr, "Bad data in " UPTIME_FILE "\n");
		return 0;
	}

	SET_IF_DESIRED(uptime_secs, up);
	SET_IF_DESIRED(idle_secs, idle);

	return up;	/* assume never be zero seconds in practice */
}

int loadavg(double *av1, double *av5, double *av15) {
	double avg_1=0, avg_5=0, avg_15=0;

	FILE_TO_BUF(LOADAVG_FILE)

	if (sscanf(buf, "%lf %lf %lf", &avg_1, &avg_5, &avg_15) < 3) {
		fprintf(stderr, "Bad data in " LOADAVG_FILE "\n");
		return 0;
	}

	SET_IF_DESIRED(av1,  avg_1);
	SET_IF_DESIRED(av5,  avg_5);
	SET_IF_DESIRED(av15, avg_15);

	return 1;
}

/* The following /proc/meminfo parsing routine assumes the following format:
   [ <label> ... ]				# header lines
   [ <label> ] <num> [ <num> ... ]		# table rows
   [ repeats of above line ]
   
   Any lines with fewer <num>s than <label>s get trailing <num>s set to zero.
   The return value is a NULL terminated unsigned** which is the table of
   numbers without labels.  Convenient enumeration constants for the major and
   minor dimensions are available in the header file.  Note that this version
   requires that labels do not contain digits.  It is readily extensible to
   labels which do not *begin* with digits, though.
*/

#define MAX_ROW 30

char *meminfo(void)
{
	*buf = 0;

	FILE_TO_BUF(MEMINFO_FILE)

	return buf;		/* NULL return ==> error */
}