   /****************************************************************
    Copyright (C) 1986-2000 by

    F6FBB - Jean-Paul ROUBELAT
    6, rue George Sand
    31120 - Roquettes - France
	jpr@f6fbb.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Parts of code have been taken from many other softwares.
    Thanks for the help.
    ****************************************************************/

#define ENGLISH

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include "version.h"
#include <fbb_conf.h>

#define FALSE 0
#define TRUE  1

#ifdef __LINUX__

#define __a2__ __attribute__ ((packed, aligned(2)))

#else

#define __a2__

#endif

#define INF_SYS "inf.sys"
#define BUFFSIZE 256

typedef struct typindic
{
	char call[7];
	char num;
}
indicat;

typedef struct {

	indicat indic     ;	/* 8  Callsign */
	indicat relai[8]  ;	/* 64 Digis path */
	long	lastmes __a2__  ;	/* 4  Last L number */
	long	nbcon   __a2__  ;	/* 4  Number of connexions */
	long	hcon    __a2__  ;	/* 4  Last connexion date */
	long	lastyap __a2__  ;	/* 4  Last YN date */
	unsigned short	flags    ;	/* 2  Flags */
	unsigned short	on_base  ;	/* 2  ON Base number */

	unsigned char	nbl       ;	/* 1  Lines paging */
	unsigned char	lang      ;	/* 1  Language */

	long	newbanner __a2__;	/* 4  Last Banner date */
	unsigned short 	download  ;	/* 2  download size (KB) = 100 */
	char	free[20]  ;	/* 20 Reserved */
	char	theme     ;	/* 1  Current topic */

	char	nom[18]   ;	/* 18 1st Name */
	char	prenom[13];	/* 13 Christian name */
	char	adres[61] ;	/* 61 Address */
	char	ville[31] ;	/* 31 City */
	char	teld[13]  ;	/* 13 home phone */
	char	telp[13]  ;	/* 13 modem phone */
	char	home[41]  ;	/* 41 home BBS */
	char	qra[7]    ;	/* 7  Qth Locator */
	char	priv[13]  ;	/* 13 PRIV directory */
	char	filtre[7] ;	/* 7  LC choice filter */
	char	pass[13]  ;	/* 13 Password */
	char	zip[9]    ;	/* 9  Zipcode */

} info ;                /* Total : 360 bytes */

FILE *fichi;
char* system_dir;

char *strlwr (char *txt)
{
	char *scan = txt;

	while (*scan)
	{
		if (isupper (*scan))
			*scan = tolower (*scan);
		++scan;
	}

	return (txt);
}

void err_keyword(char *keyword)
{
	fprintf(stderr, "Error : keyword \"%s\" missing in fbb.conf file\n", keyword);
	exit(1);
}

char *test_back_slash (char *chaine)
{
	if ((strlen(chaine) == 0)
#ifdef __LINUX__
		|| (chaine[strlen (chaine) - 1] != '/'))
			strcat(chaine, "/");
#else
		|| (chaine[strlen (chaine) - 1] != '\\'))
			strcat(chaine, "\\");
#endif
	return (chaine);
}

void ouvre_nomenc (void)
{
	char *filename = (char *) (calloc(BUFFSIZE , sizeof(char)));

	sprintf (filename, "%s%s", system_dir, INF_SYS);
	
	if ((fichi = fopen (filename, "rb")) == NULL)
	{
#ifdef ENGLISH
		fprintf (stderr, "Cannot find %s !!     \n", filename);
#else
		fprintf (stderr, "Erreur ouverture %s !!\n", filename);
#endif
		exit (1);
	}
	else
		fprintf (stderr, "%s file found\n",filename);
}


int main (int ac, char **av)
{
	info buf;
	char *ptr;
	long rinfo = 0L;
	long tot_dw = 0L;
	long nb_users = 0L;
	long temps = time (NULL);
	struct tm *tm = localtime (&temps);

	system_dir = (char *) (calloc(BUFFSIZE , sizeof(char)));
#ifdef LETTRE
	fprintf (stderr, "CLR_USER V %d.%d%c\n\n", MAJEUR, MINEUR, LETTRE);
#else
	fprintf (stderr, "CLR_USER V %d.%d\n\n", MAJEUR, MINEUR);
#endif

	/* if (tm->tm_wday != 0)
	   return(0);
	 */

	putchar ('\n');
	puts (asctime (tm));
	
	if (read_fbb_conf(NULL) > 0)
	{
#ifdef ENGLISH
		fprintf (stderr, "Cannot open fbb.conf file        \n");
#else
		fprintf (stderr, "Erreur ouverture fichier fbb.conf\n");
#endif

		exit (1);
	}

	/* path of conf files */
	ptr = find_fbb_conf("data", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("data");
	if (ptr == NULL)
		err_keyword("data");
	strcpy (system_dir, test_back_slash(strlwr (ptr)));

	ouvre_nomenc();

	while (fread ((char *) &buf, (int) sizeof (buf), 1, fichi))
	{
		tot_dw += (long) buf.download;
		if (buf.download)
		{
			printf ("%6s : %dKB\n", buf.indic.call, buf.download);
			buf.download = 0;
			fseek (fichi, rinfo, 0);
			fwrite ((char *) &buf, (int) sizeof (buf), 1, fichi);
		}
		rinfo += sizeof (buf);
		fseek (fichi, rinfo, 0);
		++nb_users;
	}

	fclose (fichi);

#ifdef ENGLISH
	printf ("%ld callsigns ok      \n", nb_users);
	printf ("%ld KB downloaded     \n", tot_dw);
#else
	printf ("%ld indicatifs        \n", nb_users);
	printf ("%ld KB envoy‚s        \n", tot_dw);
#endif
	return (0);
}

