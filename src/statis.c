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

/*
 *  MODULE STATIS.C
 */

#include <serv.h>

#ifdef THIRTYTWOBITDATA

static struct tm *fbb_localtime(int *time)
{
long ltime = *time;
return localtime(&ltime);
}


#else
#define fbb_localtime localtime
#endif


static void menu_statistiques (void);

static void histo_jour (void)
{
	FILE *fptr;
	long hist[7], max, sommet, nbc, debtime;
	int i, d;
	struct tm *sdate;
	statis buffstat;

	for (i = 0; i < 7; hist[i++] = 0L)
		;

	incindd ();
	tester_masque ();
	debtime = time (NULL) - (86400L * 14L);
	fptr = ouvre_stats ();
	fflush (fptr);
	nbc = filelength (fileno (fptr));

	do
	{
		nbc -= (MAXSTAT * (long) sizeof (buffstat));
		if (nbc < 0L)
		{
			nbc = 0;
			break;
		}
		fseek (fptr, nbc, 0);
		fread ((char *) &buffstat, sizeof (buffstat), 1, fptr);
	}
	while (buffstat.datcnx > debtime);

	do
	{
		if (buffstat.datcnx >= debtime)
		{
			char cal[8];

			n_cpy (6, cal, buffstat.indcnx);
			if (strmatch (cal, pvoie->ch_temp))
			{
				sdate = fbb_localtime (&buffstat.datcnx);
				hist[(sdate->tm_wday + 6) % 7] += buffstat.tpscnx;
			}
		}
	}
	while (fread ((char *) &buffstat, sizeof (buffstat), 1, fptr) == 1);

	ferme (fptr, 13);
	sommet = max = 10L;
	for (i = 0; i < 7; i++)
		if (hist[i] > max)
			max = hist[i];
	for (i = 0; i < 7; i++)
		hist[i] = (hist[i] * sommet) / max;
	texte (T_STA + 3);
	for (i = (int) sommet; i > 0; i--)
	{
		out (" ", 1);
		for (d = 0; d < 7; d++)
		{
			if (hist[d] >= i)
				out (" *** ", 5);
			else
				out ("     ", 5);
		}
		out ("\r", 1);
	}
	outln ("  ----------------------------------", 35);
	texte (T_STA + 4);
	retour_menu (N_STAT);
}


static void histo_heure (void)
{
	long hist[24], max, sommet;
	FILE *fptr;
	int i, h;
	long nbc, nbc_total;
	struct tm *sdate;
	statis buffstat;
	long record;

	for (i = 0; i < 24; hist[i++] = 0L)
		;

	incindd ();
	tester_masque ();

	fptr = ouvre_stats ();
	fflush (fptr);
	record = filelength (fileno (fptr));
	nbc_total = record / (long) sizeof (statis);
	nbc = record - (MAXSTAT * (long) sizeof (statis));
	if (nbc < 0L)
		nbc = 0L;
	fseek (fptr, nbc, 0);
	if (record)
	{
		nbc = MAXSTAT;
	}
	else
	{
		nbc = nbc_total;
	}

	while (fread ((char *) &buffstat, sizeof (buffstat), 1, fptr) == 1)
	{
		char cal[8];

		n_cpy (6, cal, buffstat.indcnx);
		if (strmatch (cal, pvoie->ch_temp))
		{
			sdate = fbb_localtime (&buffstat.datcnx);
			hist[sdate->tm_hour] += buffstat.tpscnx;
		}
	}

	ferme (fptr, 14);

	sommet = max = 10L;
	for (i = 0; i < 24; i++)
		if (hist[i] > max)
			max = hist[i];
	for (i = 0; i < 24; i++)
		hist[i] = (hist[i] * sommet) / max;
	texte (T_STA + 5);
	for (i = (int) sommet; i > 0; i--)
	{
		for (h = 0; h < 24; h++)
		{
			if (hist[h] >= i)
				out ("*", 1);
			else
				out (" ", 1);
		}
		out ("\r", 1);
	}
	outln ("------------------------", 24);
	texte (T_STA + 6);
	texte (T_STA + 7);
	retour_menu (N_STAT);
}


static void generalites (void)
{
	struct tm *sdate;
	statis buffstat;
	FILE *fptr;
	long nbc = (long) MAXSTAT, nbc_total;
	int i, nbjour = 0, h1 = 0, h2 = 0;
	long hist[24], somme_tps = 0L, max1 = 0L, max2 = 0L;
	long record, depart = 0;

	for (i = 0; i < 24; hist[i++] = 0L)
		;

	fptr = ouvre_stats ();
	fflush (fptr);
	record = filelength (fileno (fptr));
	nbc_total = record / (long) sizeof (statis);
	nbc = record - (MAXSTAT * (long) sizeof (statis));
	if (nbc < 0L)
		nbc = 0L;
	fseek (fptr, nbc, 0);

	nbc = 0L;
	nbjour = 0L;
	while (fread ((char *) &buffstat, sizeof (buffstat), 1, fptr) == 1)
	{
		if (nbjour == 0)
		{
			/* Compte le nombre de jours */
			nbjour = (int) ((time (NULL) - buffstat.datcnx) / 86400L);

			/* Fait le calcul sur un nombre de jours entier */
			depart = time (NULL) - (long) nbjour *86400L;
		}

		sdate = fbb_localtime (&buffstat.datcnx);
		if (buffstat.datcnx > depart)
		{
			hist[sdate->tm_hour] += (long) buffstat.tpscnx;
			++nbc;
		}
	}

	for (i = 0; i < 24; i++)
	{
		somme_tps += hist[i];
		if (hist[i] > max1)
		{
			h2 = h1;
			max2 = max1;
			h1 = i;
			max1 = hist[i];
		}
		else if (hist[i] > max2)
		{
			h2 = i;
			max2 = hist[i];
		}
	}

	rewind (fptr);
	fread ((char *) &buffstat, sizeof (buffstat), 1, fptr);

	ferme (fptr, 15);

	texte (T_STA + 8);
	ltoa (nbc_total, varx[0], 10);
	if (nbc_total)
		ptmes->date = buffstat.datcnx;
	else
		ptmes->date = time (NULL);
	texte (T_STA + 9);
	if (nbc)
		i = (int) (somme_tps / (long) nbc);
	else
		i = 0;
	itoa (i / 60, varx[0], 10);
	sprintf (varx[1], "%02d", i % 60);
	texte (T_STA + 10);

	if (nbjour == 0)
		nbjour = 1;
	ltoa (nbc / (long) nbjour, varx[0], 10);
	texte (T_STA + 11);
	itoa (h1, varx[0], 10);
	itoa (h2, varx[1], 10);
	texte (T_STA + 12);
	retour_menu (N_STAT);
}


static void occupation (void)
{
	long total, stotal;
	float ftotal;

	stotal = stemps[N_DOS] + stemps[N_QRA] + stemps[N_INFO] +
		stemps[N_STAT] + stemps[N_NOMC] + stemps[N_TRAJ];
	total = stotal + stemps[N_MBL];

	ftotal = ((float) total) / 100.0;
	if (ftotal == 0.0)
	{
		ftotal = 1E-5;
	}

	texte (T_STA + 13);

	sprintf (varx[0], "%4.1f", ((float) stemps[N_MBL]) / ftotal);
	texte (T_STA + 14);
	sprintf (varx[0], "%4.1f", ((float) stotal) / ftotal);
	texte (T_STA + 15);

	ftotal = (float) (stotal) / 100.0;
	if (ftotal == 0.0)
		ftotal = 1E-5;

	texte (T_STA + 16);
	sprintf (varx[0], "%4.1f", ((float) stemps[N_DOS]) / ftotal);
	texte (T_STA + 17);
	sprintf (varx[0], "%4.1f", ((float) stemps[N_QRA]) / ftotal);
	texte (T_STA + 18);
	sprintf (varx[0], "%4.1f", ((float) stemps[N_INFO]) / ftotal);
	texte (T_STA + 19);
	sprintf (varx[0], "%4.1f", ((float) stemps[N_STAT]) / ftotal);
	texte (T_STA + 20);
	sprintf (varx[0], "%4.1f", ((float) stemps[N_NOMC]) / ftotal);
	texte (T_STA + 21);
	sprintf (varx[0], "%4.1f", ((float) stemps[N_TRAJ]) / ftotal);
	texte (T_STA + 22);
	retour_menu (N_STAT);
}


static void liste_connect (void)
{
	char c;
	FILE *fptr;

	pvoie->lignes = -1;
	switch (pvoie->niv3)
	{
	case 0:
		incindd ();
		tester_masque ();
		fptr = ouvre_stats ();
		fseek (fptr, 0L, 2);
		pvoie->noenr_menu = ftell (fptr);
		if (page_connect ('\0', fptr))
		{
			texte (T_TRT + 11);
			maj_niv (3, 1, 1);
		}
		else
			retour_menu (N_STAT);
		ferme (fptr, 16);
		break;
	case 1:
		c = toupper (*indd);
		if ((c == 'A') || (c == 'F') || (c == Non))
		{
			maj_niv (3, 0, 0);
			incindd ();
			menu_statistiques ();
		}
		else
		{
			fptr = ouvre_stats ();
			if (!page_connect ('\0', fptr))
				retour_menu (N_STAT);
			else
				texte (T_TRT + 11);
			ferme (fptr, 17);
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "LIST-CONN", pvoie->niv3);
		break;
	}
}


/*
 *  MENUS - PREMIER NIVEAU STATISTIQUES
 */

static void menu_statistiques (void)
{
	int error = 0;
	char com[80];

	limite_commande ();
	while (*indd && (!ISGRAPH (*indd)))
		indd++;
	strn_cpy (70, com, indd);

	switch (toupper (*indd))
	{
	case 'L':
		maj_niv (3, 1, 0);
		liste_connect ();
		break;
	case 'I':
		maj_niv (3, 2, 0);
		incindd ();
		liste_indic ();
		break;
	case 'J':
		maj_niv (3, 3, 0);		/* maj_niv non necessaire */
		histo_jour ();			/* histo traite en un seul bloc */
		break;
	case 'H':
		maj_niv (3, 4, 0);
		histo_heure ();
		break;
	case 'G':
		maj_niv (3, 5, 0);
		generalites ();
		break;
	case 'O':
		maj_niv (3, 6, 0);
		occupation ();
		break;
	case 'B':
		maj_niv (N_MENU, 0, 0);
		sortie ();
		break;
	case 'F':
		maj_niv (0, 1, 0);
		incindd ();
		choix ();
		break;
	case '\0':
		prompt (pvoie->finf.flags, pvoie->niv1);
		break;
	default:
		if (!defaut ())
			error = 1;
		break;
	}
	if (error)
	{
		cmd_err (indd);
	}
}


void statistiques (void)
{
	switch (pvoie->niv2)
	{
	case 0:
		menu_statistiques ();
		break;
	case 1:
		liste_connect ();
		break;
	case 2:
		liste_indic ();
		break;
	case 3:
		histo_jour ();
		break;
	case 4:
		histo_heure ();
		break;
	case 5:
		generalites ();
		break;
	case 6:
		occupation ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "STATS", pvoie->niv2);
		break;
	}
}
