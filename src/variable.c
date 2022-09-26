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

/*
 * VARIABLE.C
 *
 */

#include <serv.h>

static char locbuf[600];

char *strdate (long temps)
{
	static char cdate[19];
	char jour[4];
	struct tm *sdate;

	df ("strdate", 2);

	sdate = localtime (&temps);
	if (vlang == -1)
		*jour = '\0';
	else
	{
		strncpy (jour, (langue[vlang]->plang[JOUR - 1]) + (sdate->tm_wday * 3), 3);
		jour[3] = '\0';
	}
	sprintf (cdate, "%s %02d/%02d/%02d %02d:%02d",
			 jour, sdate->tm_mday,
			 sdate->tm_mon + 1, sdate->tm_year % 100,
			 sdate->tm_hour, sdate->tm_min);
	ff ();
	return (cdate);
}


char *strdt (long temps)
{
	struct tm *sdate;
	static char cdate[15];

	df ("strdt", 2);

	sdate = localtime (&temps);
	sprintf (cdate, "%02d/%02d/%02d %02d:%02d",
			 sdate->tm_mday,
			 sdate->tm_mon + 1, sdate->tm_year % 100,
			 sdate->tm_hour, sdate->tm_min);
	ff ();
	return (cdate);
}


static char *date_heure_fbb (long temps)
{
	struct tm *sdate;
	static char cdate[15];

	df ("data_heure_fbb", 2);

	sdate = localtime (&temps);
	sprintf (cdate, "%02d%02d/%02d%02d",
			 sdate->tm_mon + 1, sdate->tm_mday,
			 sdate->tm_hour, sdate->tm_min);
	ff ();
	return (cdate);
}


char *date_mbl (long temps)
{
	struct tm *sdate;
	static char cdate[7];

	df ("data_mbl", 2);

	sdate = localtime (&temps);
	sprintf (cdate, "%02d%02d%02d", sdate->tm_year % 100, sdate->tm_mon + 1, sdate->tm_mday);
	ff ();
	return (cdate);
}


char *heure_mbl (long temps)
{
	struct tm *sdate;
	static char cdate[5];

	df ("heure_mbl", 2);

	sdate = localtime (&temps);
	sprintf (cdate, "%02d%02d", sdate->tm_hour, sdate->tm_min);
	ff ();
	return (cdate);
}


static char *date_mbl_new (long temps)
{
	char mois[4];
	struct tm *sdate;
	static char cdate[7];

	df ("date_mbl_new", 2);

	sdate = localtime (&temps);
	if (vlang == -1)
		*mois = '\0';
	else
	{
		strncpy (mois, (langue[vlang]->plang[MOIS - 1]) + (sdate->tm_mon * 3), 3);
		mois[3] = '\0';
	}
	sprintf (cdate, "%02d-%s", sdate->tm_mday, mois);
	ff ();
	return (cdate);
}


static char *annee_mbl (long temps)
{
	struct tm *sdate;
	static char cdate[7];

	df ("annee_mbl", 2);

	sdate = localtime (&temps);
	sprintf (cdate, "%02d", sdate->tm_year % 100);
	ff ();
	return (cdate);
}


char *datheure_mbl (long temps)
{
	static char cdate[13];

	df ("datheure_mbl", 2);

	sprintf (cdate, "%s %s", date_mbl_new (temps), strheure (temps));
	ff ();
	return (cdate);
}


char *strheure (long temps)
{
	struct tm *sdate;
	static char cdate[6];

	df ("str_heure", 2);

	sdate = localtime (&temps);
	sprintf (cdate, "%02d:%02d", sdate->tm_hour, sdate->tm_min);
	ff ();
	return (cdate);
}


int jour (long temps)
{
	struct tm *sdate;

	df ("jour", 2);

	sdate = localtime (&temps);
	ff ();
	return (sdate->tm_wday);
}


int nojour (long temps)
{
	struct tm *sdate;

	df ("nojour", 2);

	sdate = localtime (&temps);
	ff ();
	return (sdate->tm_mday);
}


int heure (long temps)
{
	struct tm *sdate;

	df ("heure", 2);

	sdate = localtime (&temps);
	ff ();
	return (sdate->tm_hour);
}


int gmt_heure (long temps)
{
	struct tm *sdate;

	df ("heure", 2);

	sdate = gmtime (&temps);
	ff ();
	return (sdate->tm_hour);
}


int minute (long temps)
{
	struct tm *sdate;

	df ("minute", 2);

	sdate = localtime (&temps);
	ff ();
	return (sdate->tm_min);
}

static int check_fwd (char *bbs, int *nbbul, int *nbpriv, int *nbkilo)
{
	char maxfwd[NBBBS + 1];
	char typfwd[NBBBS + 1];
	char typdat[NBBBS + 1];
	atfwd *mess;
	int nobbs;

	static unsigned task_ident = 0xffff;
	static atfwd smess;

	*nbbul = *nbpriv = *nbkilo = 0;

	nobbs = n_bbs (bbs);
	if (nobbs == 0)
		return (FALSE);

	if (task_ident != tid)
	{
		task_ident = tid;

		fwd_value (maxfwd, typfwd, typdat);

		if ((mess = attend_fwd (nobbs, maxfwd[nobbs], 0, typfwd[nobbs], typdat[nobbs])) != NULL)
		{
			smess = *mess;
		}
		else
		{
			memset (&smess, 0, sizeof (atfwd));
		}
	}

	*nbpriv = smess.nbpriv;
	*nbbul = smess.nbbul;
	*nbkilo = smess.nbkb;

	return (TRUE);
}

char *variable (char var)
{
	int nb;
	long t_cnx;
	char *ptr = locbuf;

	df ("variable", 1);

	*ptr = '\0';

	switch (var)
	{

	case '$':
		*ptr++ = '$';
		*ptr = '\0';
		break;

		/* Variables red‚finies */

	case '0':
		n_cpy (80, ptr, varx[0]);
		break;

	case '1':
		n_cpy (80, ptr, varx[1]);
		break;

	case '2':
		n_cpy (80, ptr, varx[2]);
		break;

	case '3':
		n_cpy (80, ptr, varx[3]);
		break;

	case '4':
		n_cpy (80, ptr, varx[4]);
		break;

	case '5':
		n_cpy (80, ptr, varx[5]);
		break;

	case '6':
		n_cpy (80, ptr, varx[6]);
		break;

	case '7':
		n_cpy (80, ptr, varx[7]);
		break;

	case '8':
		n_cpy (80, ptr, varx[8]);
		break;

	case '9':
		n_cpy (80, ptr, varx[9]);
		break;

		/* Variable pr‚d‚finies */

	case 'A':
		sprintf (ptr, "%-6s", bbs_via (ptmes->bbsv));
		break;

	case 'a':
		strcpy (ptr, annee_mbl (time (NULL)));
		break;

	case 'B':
		*ptr++ = '\a';
		*ptr = '\0';
		break;

	case 'b':
		strcpy (ptr, pvoie->finf.zip);
		break;

	case 'C':
		ltoa (nomess + 1, ptr, 10);
		break;

	case 'c':
		strcpy (ptr, my_city);
		break;

	case 'D':
		strcpy (ptr, date_mbl (time (NULL)));
		break;

	case 'd':
		strcpy (ptr, date_mbl_new (time (NULL)));
		break;

	case 'E':
		strcpy (ptr, version ());
		break;

	case 'e':
		strcpy (ptr, pvoie->finf.ville);
		break;

	case 'F':
		itoa (p_port[no_port (voiecur)].min_fwd, ptr, 10);
		break;

	case 'f':
		strcpy (ptr, pvoie->appendf);
		break;

	case 'G':
		sprintf (ptr, "%-6s", ptmes->desti);
		break;

	case 'g':
		itoa (nbgate (), ptr, 10);
		break;

	case 'H':
		strcpy (ptr, strheure (time (NULL)));
		break;

	case 'h':
		strcpy (ptr, pvoie->finf.home);
		break;

	case 'I':
		if (*(pvoie->finf.prenom))
			strcpy (ptr, pvoie->finf.prenom);
		else
			strcpy (ptr, "???");
		break;

	case 'i':
		strcpy (ptr, date_heure_fbb (ptmes->date));
		break;

	case 'J':
		strcpy (ptr, date_mbl (ptmes->date));
		break;

	case 'j':
		strcpy (ptr, date_mbl_new (ptmes->date));
		break;

	case 'K':
		strcpy (ptr, strheure (ptmes->date));
		break;

	case 'k':
		strcpy (ptr, k_var ());
		break;

	case 'L':
		ltoa (nomess, ptr, 10);
		break;

	case 'l':
		if (*pvoie->finf.filtre)
			strcpy (ptr, pvoie->finf.filtre);
		else
		{
			*ptr++ = '*';
			*ptr = '\0';
		}
		break;

	case 'M':
		sprintf (ptr, "%-6ld", ptmes->numero);
		break;

	case 'm':
		strcpy (ptr, p_port[no_port (voiecur)].freq);
		break;

	case 'N':
		ltoa (nbmess, ptr, 10);
		break;

	case 'n':
		sprintf (ptr, "%5ld", ptmes->taille);
		break;

	case 'O':
		strcpy (ptr, mycall);
		break;

	case 'o':
		itoa (myssid, ptr, 10);
		break;

	case 'P':
		sprintf (ptr, "%-6s", ptmes->exped);
		break;

	case 'p':
		if (PAG (pvoie->finf.flags))
			ltoa (pvoie->finf.nbl, ptr, 10);
		else
		{
			ptr[0] = ptr[1] = '-';
			ptr[2] = '\0';
		}
		break;

	case 'Q':
		list_new (ptr);
		break;

	case 'q':
		ltoa (1000L * (long) pvoie->finf.on_base, ptr, 10);
		break;

	case 'R':
		strcpy (ptr, ptmes->bid);
		break;

	case 'r':
		*ptr++ = (*ptmes->bbsf) ? ' ' : 'L';
		*ptr = '\0';
		break;

	case 'S':
		strcpy (ptr, ptmes->titre);
		break;

	case 's':
		*ptr++ = ptmes->status;
		*ptr = '\0';
		break;

	case 'T':
		strcpy (ptr, strheure (time (NULL)));
		break;

	case 't':
		*ptr++ = ptmes->type;
		*ptr = '\0';
		break;

	case 'U':
		strcpy (ptr, pvoie->sta.indicatif.call);
		break;

	case 'u':
		*ptr++ = (pvoie->vdisk == 8) ? 'P' : pvoie->vdisk + 'A';
		*ptr = '\0';
		break;

	case 'V':
		strcpy (ptr, my_name);
		break;

	case 'v':
		strcpy (ptr, ptmes->bbsv);
		break;

	case 'W':
		*ptr++ = '\r';
		*ptr = '\0';
		break;

	case 'w':
		*ptr++ = '\033';
		*ptr = '\0';
		break;

	case 'X':
		strcpy (ptr, date_mbl (pvoie->finf.hcon));
		break;

	case 'x':
		strcpy (ptr, date_mbl_new (pvoie->finf.hcon));
		break;

	case 'Y':
		strcpy (ptr, strheure (pvoie->finf.hcon));
		break;

	case 'y':
		strcpy (ptr, annee_mbl (ptmes->date));
		break;

	case 'Z':
		ltoa (pvoie->finf.lastmes, ptr, 10);
		break;

	case 'z':
		strcpy (ptr, my_zip);
		break;

	case '*':
		nb = actif (1);
		itoa (nb, ptr, 10);
		break;

	case '=':
		itoa (virt_canal (voiecur), ptr, 10);
		break;

	case '!':
		itoa (no_port (voiecur), ptr, 10);
		break;

	case '^':
		itoa (nbport (), ptr, 10);
		break;

	case '?':
		strcpy (ptr, qra_locator);
		break;

	case '%':
		who (ptr);
		break;

	case ':':
		t_cnx = time (NULL) - pvoie->debut;
		if (t_cnx < 60)
			sprintf (ptr, "%2lds", t_cnx);
		else
			sprintf (ptr, "%ldmn %02lds", t_cnx / 60, t_cnx % 60);
		break;

	case '.':
		if (pvoie->tmach < 60)
			sprintf (ptr, "%2lds", pvoie->tmach);
		else
			sprintf (ptr, "%ldmn %02lds", pvoie->tmach / 60, pvoie->tmach % 60);
		break;

	default:
		sprintf (ptr, "$%c", var);
		break;

	}
	ff ();
	return (locbuf);
}

char *alt_variable (char var)
{
	int nbbul, nbpriv, nbkilo;
	char *ptr = locbuf;
	Wps *wps;

	df ("alt_variable", 1);

	*ptr = '\0';

	switch (var)
	{

	case '%':
		*ptr++ = '%';
		*ptr = '\0';
		break;

		/* Variable pr‚d‚finies */

	case 'A':
		sprintf (ptr, "%s", bbs_via (ptmes->bbsv));
		break;

	case 'C':
		sprintf (ptr, "%-3u", ptmes->nblu);
		break;

	case 'd':
		itoa (pvoie->finf.download, ptr, 10);
		break;

	case 'E':
		strcpy (ptr, os ());
		break;

	case 'e':
		strcpy (ptr, date ());
		break;

	case 'G':
		sprintf (ptr, "%s", ptmes->desti);
		break;

	case 'I':
		/* Get the name from WP */				
		wps = wp_find(ptmes->exped, 0);
		if ((wps) && (*wps->name) && (*wps->name != '?'))
			strcpy (ptr, wps->name);
		else
			strcpy (ptr, "???");
		break;

	case 'i':
		strcpy (ptr, date_heure_fbb (ptmes->datesd));
		break;

	case 'J':
		strcpy (ptr, date_mbl (ptmes->datesd));
		break;

	case 'j':
		strcpy (ptr, date_mbl_new (ptmes->datesd));
		break;

	case 'K':
		strcpy (ptr, strheure (ptmes->datesd));
		break;

	case 'k':
		check_fwd (pvoie->sta.indicatif.call, &nbbul, &nbpriv, &nbkilo);
		itoa (nbkilo, ptr, 10);
		break;

	case 'l':
		check_fwd (pvoie->sta.indicatif.call, &nbbul, &nbpriv, &nbkilo);
		itoa (nbpriv + nbbul, ptr, 10);
		break;

	case 'M':
		sprintf (ptr, "%ld", ptmes->numero);
		break;

	case 'm':
		itoa (P_MODM (voiecur) ? max_mod : max_yapp, ptr, 10);
		break;

	case 'N':
		sprintf (ptr, "%ld", ptmes->numero % 0x10000L);
		break;

	case 'n':
		sprintf (ptr, "%ld", ptmes->taille);
		break;

	case 'O':
		strcpy (ptr, admin);
		break;

	case 'P':
		sprintf (ptr, "%s", ptmes->exped);
		break;

	case 'R':
		strcpy (ptr, mypath);
		break;

	case 'r':
		*ptr++ = (ptmes->bin) ? 'D' : ' ';
		*ptr = '\0';
		break;

	case 'T':
		strcpy (ptr, cur_theme (voiecur));
		break;

	case 't':
		itoa (nbull_theme (voiecur), ptr, 10);
		break;

	case 'X':
		itoa (pvoie->ncur->nbmess, ptr, 10);
		break;

	case 'x':
		itoa (pvoie->ncur->nbnew, ptr, 10);
		break;

	case 'y':
		strcpy (ptr, annee_mbl (ptmes->datesd));
		break;

	default:
		sprintf (ptr, "%%%c", var);
		break;

	}
	ff ();
	return (locbuf);
}
