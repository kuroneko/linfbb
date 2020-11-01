/************************************************************************
    Copyright (C) 1986-2000 by

    F6FBB - Jean-Paul ROUBELAT
    jpr@f6fbb.org

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

#define ENGLISH

#ifdef __linux__
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define __NO_CTYPE
#define INF_NEW "inf.new"
#define INF_SYS "inf.sys"
#define __a2__ __attribute__ ((packed, aligned(2)))
#else

#define __a2__
#define INF_NEW "inf.new"
#define INF_SYS "inf.sys"

#endif

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <config.h> 
#include <fbb_conf.h>

#define FALSE 0
#define TRUE  1

#define uchar unsigned char
#define ushort unsigned short
#define EXCLUDED(buf) (buf.flags & 0x1)

typedef struct typindic
{
	char call[7];
	char num;
}
indicat;

typedef struct
{
	indicat indic;				/* 8  Callsign */
	indicat relai[8];			/* 64 Digis path */
	long lastmes __a2__;		/* 4  Last L number */
	long nbcon __a2__;			/* 4  Number of connexions */
	long hcon __a2__;			/* 4  Last connexion date */
	long lastyap __a2__;		/* 4  Last YN date */
	ushort flags;				/* 2  Flags */
	ushort on_base;				/* 2  ON Base number */

	uchar nbl;					/* 1  Lines paging */
	uchar lang;					/* 1  Language */

	long newbanner __a2__;		/* 4  Last Banner date */
	ushort download;			/* 2  download size (KB) = 100 */
	char free[20];				/* 20 Reserved */
	char theme;					/* 1  Current topic */

	char nom[18];				/* 18 1st Name */
	char prenom[13];			/* 13 Christian name */
	char adres[61];				/* 61 Address */
	char ville[31];				/* 31 City */
	char teld[13];				/* 13 home phone */
	char telp[13];				/* 13 modem phone */
	char home[41];				/* 41 home BBS */
	char qra[7];				/* 7  Qth Locator */
	char priv[13];				/* 13 PRIV directory */
	char filtre[7];				/* 7  LC choice filter */
	char pass[13];				/* 13 Password */
	char zip[9];				/* 9  Zipcode */

}
info;							/* Total : 360 bytes */

typedef struct typindictri
{
	char indic[7];
	long pos;
	struct typindictri *suiv;
}
indictri;

FILE *fichi, *ficho;
char temp[500];
char system_dir[256];
indictri *tete;
int rejet, nbindic;
int ext_call = 0;

static void defauts (void);
static int find (char *s);
static void check (info * bul);
static void err_alloc (void);
static void insere (char *ind, long pos);
static void ouvre_nomenc (void);


#ifdef __linux__
char *strupr (char *txt)
{
	char *scan = txt;

	while (*scan)
	{
		if (islower (*scan))
			*scan = toupper (*scan);
		++scan;
	}

	return (txt);
}

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
#endif

char *test_back_slash (char *chaine)
{
	static char temp[256];
	strcpy(temp, chaine);

	if ((strlen(temp) == 0)
#ifdef __linux__
		|| (temp[strlen (temp) - 1] != '/'))
			strcat(temp, "/");
#else
		|| (temp[strlen (temp) - 1] != '\\'))
			strcat(temp, "\\");
#endif
	return (temp);
}

void err_keyword(char *keyword)
{
	fprintf(stderr, "Error : keyword \"%s\"missing in fbb.conf file\n", keyword);
	exit(1);
}

void epure (char *ligne)
{
	int lg;

	lg = strlen (ligne);
	if (ligne[lg - 1] == '\n')
		ligne[lg - 1] = '\0';
	lg = strlen (ligne);
	if (ligne[lg - 1] == '\r')
		ligne[lg - 1] = '\0';
	if (*ligne == '\032')
		*ligne = '\0';
}

int main (int ac, char **av)
{
	int rep;
	int auto_rep = 0;
	int monthes;
	info buf;
	long rinfo = 0;
	long i = 0;
	indictri *ptr;
	long temps = time (NULL);
	long timout;

	fprintf (stderr, "\nMAINTINF V%s\n\n", VERSION);

#ifdef ENGLISH
	fprintf (stderr, "FBB software MUST be stopped !!    \n\n");
#else
	fprintf (stderr, "Le logiciel FBB Doit etre arr‚t‚ !!\n\n");
#endif

	defauts ();

	rejet = nbindic = 0;
	timout = 86400L * 31L;		/* 31 jours / mois */

	if (ac == 1)
	{
#ifdef ENGLISH
		fprintf (stderr, "Format : MAINTINF Monthes [/A]\n\nMonthes is \"time check\" to kill old callsigns (0 = no check)     \n                 \n");
#else
		fprintf (stderr, "Format : MAINTINF Mois [/A]\n\nMois est le \"Test des dates\" pour supprimer les indicatifs obsoletes\n(0 = pas de test)\n");
#endif
		exit (0);
	}

	monthes = atoi (av[1]);

	timout *= monthes;

	if (ac >= 3)
	{
		ext_call = (strcmp (strupr (av[2]), "/N") == 0);
		if (ext_call)
		{
			--ac;
			++av;
#ifdef ENGLISH
			printf ("No callsign check      \n");
#else
			printf ("Pas de test d'indicatif\n");
#endif
		}
	}

	if (ac == 3)
		auto_rep = (strcmp (strupr (av[2]), "/A") == 0);

#ifdef ENGLISH
	if (monthes)
		printf ("Time check : %d monthes\n", monthes);
	else
		printf ("No Time check    \n");
#else
	if (monthes)
		printf ("Test dates : %d mois   \n", monthes);
	else
		printf ("Pas de test dates\n");
#endif
	sleep (2);

	ouvre_nomenc ();
	if ((tete = (indictri *) malloc (sizeof (indictri))) == NULL)
		err_alloc ();
	tete->suiv = NULL;
	rewind (fichi);
	while (fread ((char *) &buf, (int) sizeof (buf), 1, fichi))
	{
		if ((!EXCLUDED (buf)) && (timout) && ((temps - buf.hcon) > timout))
		{
#ifdef ENGLISH
			printf ("rejects <%s> : Old callsign    \n", buf.indic.call);
#else
			printf ("rejŠte <%s> : Indicatif inusit‚\n", buf.indic.call);
#endif
			++rejet;
		}
		else if (find (buf.indic.call))
		{
			insere (buf.indic.call, rinfo);
		}
		else
		{

#ifdef ENGLISH
			printf ("rejects <%s> : Incorrect callsign\n", buf.indic.call);
#else
			printf ("rejŠte <%s> : Indicatif incorrect\n", buf.indic.call);
#endif
			++rejet;
		}
		++rinfo;
	}

#ifdef ENGLISH
	printf ("%d callsigns ok      \n", nbindic);
	printf ("%d callsigns rejected\n", rejet);
	printf ("---------------------------------------------\n\n");
#else
	printf ("%d indicatifs inse‚re‚s\n", nbindic);
	printf ("%d indicatifs rejete‚s\n", rejet);
	printf ("----------------------------------           \n\n");
#endif

	if (!auto_rep)
	{
		while (TRUE)
		{
#ifdef ENGLISH
			fprintf (stderr, "Create %s      (Y/N) : ", INF_NEW);
#else
			fprintf (stderr, "Creation de %s (O/N) : ", INF_NEW);
#endif
			rep = getchar ();
#ifdef ENGLISH
			if ((rep == 'y') || (rep == 'Y') || (rep == 'n') || (rep == 'N'))
#else
			if ((rep == 'o') || (rep == 'O') || (rep == 'n') || (rep == 'N'))
#endif
				break;
		}
		putchar ('\n');
	}
#ifdef ENGLISH
	if ((auto_rep) || (toupper (rep) == 'Y'))
	{
#else
	if ((auto_rep) || (toupper (rep) == 'O'))
	{
#endif
		char filename[256];

		sprintf (filename, "%s%s", system_dir, INF_NEW);
		if ((ficho = fopen (filename, "wb")) == NULL)
		{
#ifdef ENGLISH
			fprintf (stderr, "Error creating %s     \n", filename);
#else
			fprintf (stderr, "Impossible de creer %s\n", filename);
#endif
		}
		else
		{
			ptr = tete;
			while ((ptr = ptr->suiv) != NULL)
			{
				fseek (fichi, ptr->pos * (long) sizeof (buf), 0);
				fread (&buf, sizeof (buf), 1, fichi);
				check (&buf);
				fwrite (&buf, sizeof (buf), 1, ficho);
				if ((++i % 10) == 0)
#ifdef ENGLISH
					fprintf (stderr, "\r%ld callsigns  ", i);
#else
					fprintf (stderr, "\r%ld indicatifs ", i);
#endif
			}
#ifdef ENGLISH
			fprintf (stderr, "\r%ld callsigns  ", i);
#else
			fprintf (stderr, "\r%ld indicatifs ", i);
#endif
			fclose (ficho);
			fputc ('\r', stderr);
		}
#ifdef ENGLISH
		printf ("\nFile %s created\n\n", INF_NEW);
		printf ("\nYou must now change %s with %s !!\n", INF_SYS, INF_NEW);
		printf ("---------------------------------------------\n\n");
#else
		printf ("\nFichier %s cr‚‚\n\n", INF_NEW);
		printf ("\nRemplacer %s par %s !!           \n", INF_SYS, INF_NEW);
		printf ("----------------------------------           \n\n");
#endif
	}
	fclose (fichi);
	
	return 0;
}


static void check (info * bul)
{
	int i;

	bul->indic.call[6] = '\0';
	for (i = 0; i < 8; i++)
		bul->relai[i].call[6] = '\0';
	bul->nom[17] = '\0';
	bul->prenom[12] = '\0';
	bul->adres[60] = '\0';
	bul->ville[30] = '\0';
	bul->teld[12] = '\0';
	bul->telp[12] = '\0';
	bul->home[40] = '\0';
	bul->qra[6] = '\0';
	bul->priv[12] = '\0';
	bul->filtre[6] = '\0';
	bul->pass[12] = '\0';
	bul->zip[8] = '\0';

	if ((bul->nbcon < 0) || (bul->nbcon > 100000L))
		bul->nbcon = 0L;
	if ((bul->nbl < (uchar) 4) || (bul->nbl > (uchar) 50))
		bul->nbl = (uchar) 25;
	if (bul->lang > (uchar) 99)
		bul->lang = (uchar) 1;
}


static void insere (char *ind, long pos)
{
	int cmp;
	indictri *pprec, *ptr, *ptemp;

	pprec = ptr = tete;
	while ((ptr = ptr->suiv) != NULL)
	{
		cmp = strcmp (ind, ptr->indic);
		if (cmp == 0)
		{
			printf ("rejects <%s> : already inserted\n", ind);
			++rejet;
			return;
		}
		else if (cmp < 0)
			break;
		pprec = ptr;
	}
	if ((ptemp = (indictri *) malloc (sizeof (indictri))) == NULL)
		err_alloc ();

	++nbindic;
	ptemp->suiv = ptr;
	pprec->suiv = ptemp;
	ptemp->pos = pos;
	strcpy (ptemp->indic, ind);
}


static int find (char *s)
{
	char *t = s;
	int n = 0;
	int dernier = 0, chiffre = 0, lettre = 0;

	while (*t)
	{
		if (!isalnum (*t))
			return (FALSE);
		*t = toupper (*t);

		dernier = (isdigit (*t));

		if (isdigit (*t))
			++chiffre;
		else
			++lettre;

		++t;
		++n;
	}

	/*
	   * L'indicatif doit avoir entre 3 et 6 caracteres .
	   *             doit contenir 1 ou 2 chiffres .
	   *             ne doit pas se terminer par un chiffre .
	 */

	if (ext_call)
	{
		/*
		 * L'indicatif doit avoir entre 3 et 6 caracteres .
		 *             doit contenir au moins un chiffre
		 *                  doit contenir au moins une lettre
		 */
		if ((n < 3) || (n > 6) || (chiffre < 1) || (lettre < 1))
			return (0);
	}
	else
	{
		/*
		   * L'indicatif doit avoir entre 3 et 6 caracteres .
		   *             doit contenir 1 ou 2 chiffres .
		   *             ne doit pas se terminer par un chiffre .
		 */

		if ((n < 3) || (n > 6) || (chiffre < 1) || (chiffre > 2) || dernier)
			return (0);
	}

	t = temp;
	while (isalnum (*s))
		*t++ = *s++;
	*t = '\0';
	t = temp;
	return (TRUE);
}


static void ouvre_nomenc (void)
{
	char filename[256];

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
}


static void err_alloc (void)
{
#ifdef ENGLISH
	fprintf (stderr, "Memory allocation error  \n");
#else
	fprintf (stderr, "Erreur allocation memoire\n");
#endif
	exit (1);
}

static void defauts (void)
{
	unsigned int flag;
	char *ptr;
	char temp[20];

	if (read_fbb_conf(NULL) > 0)
	{
#ifdef ENGLISH
		fprintf (stderr, "Cannot open fbb.conf file        \n");
#else
		fprintf (stderr, "Erreur ouverture fichier fbb.conf\n");
#endif
		exit (1);				/* and users base directory */
	}

	ptr = find_fbb_conf("vers", 0);
	if (ptr == NULL)
	{
#ifdef ENGLISH
		fprintf (stderr, "Version number missing in fbb.conf\n");
#else
		fprintf (stderr, "Pas de numéro de version dans fbb.conf\n");
#endif
		exit (1);
	}

/*	sprintf (temp, "FBB%s", VERSION);
	if (strncasecmp (temp, ptr, 7) != 0)
	{
#ifdef ENGLISH
		fprintf (stderr, "Wrong version number '%s' in fbb.conf\n", ptr);
#else
		fprintf (stderr, "Numéro de version erroné '%s' dans fbb.conf\n", ptr);
#endif
		exit (1);
	}
	fprintf (stderr, "Configuration version : %s\r\n", ptr);
*/
	/* path of conf files */
	ptr = find_fbb_conf("data", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("data");
	if (ptr == NULL)
		err_keyword("data");
	strcpy (system_dir, test_back_slash(strlwr (ptr)));

	/* flags */
	ptr = find_fbb_conf("fbbf", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("fbbf");
	if (ptr == NULL)
		err_keyword("fbbf");
	flag = 0;
	sscanf (ptr, "%s %u", temp, &flag);
	ext_call = ((flag & 4096) != 0);
}
