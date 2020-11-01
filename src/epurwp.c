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

/*

 *  Mise a jour des WP en fonction de la date
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef __linux__
#include <stdlib.h>
#include <unistd.h>
#else
#include <alloc.h>
#include <io.h>
#endif

#include <fbb_conf.h>
#include <config.h> 

#ifdef __linux__

#define __a2__ __attribute__ ((packed, aligned(2)))
#define O_BINARY 0
#define _read read
#define _write write

#else

#define __a2__

#endif

#define uchar unsigned char
#define lcall unsigned long

typedef struct
{								/* 194 bytes */
	char callsign[7];
	char name[13];
	uchar free;
	uchar changed;
	unsigned short seen;
	long last_modif __a2__;
	long last_seen __a2__;
	char first_homebbs[41];
	char secnd_homebbs[41];
	char first_zip[9];
	char secnd_zip[9];
	char first_qth[31];
	char secnd_qth[31];
}
Wps;

lcall zone[1000];

int addr_check = 1;
int ext_call = 0;

unsigned deb_zone;

long tst_date;
long kill_date;
long heure;
long record = 0L;
long lines_out = 0L;
long record_in = 0L;
long record_out = 0L;
long update = 0L;
long destroy = 0L;
long dupes = 0L;

Wps rec;

char wp_sys[256];
char wp_old[256];
char wp_mess[256];
char upd_file[256];
char compte_rendu[256];

void defauts (int, int);
void print_compte_rendu (void);
void strn_cpy (char *, char *, int);
void init_zone (long);
void add_zone (Wps *);

char *date_mbl_new (long);
char *strdt (long);
void wp_message (Wps *);

lcall call2l (char *);

int addr_ok (char *s);
int find (char *);
int copy_ (char *, char *);
int check_record (Wps * rec);

FILE *fpti;
FILE *fpto;
FILE *fptr_mess;
FILE *fptr_upd;

#ifdef __linux__

char *strupr (char *str)
{
	char *tmp = str;

	while (*tmp)
	{
		if (islower (*tmp))
			*tmp = toupper (*tmp);
		++tmp;
	}
	return str;
}

char *strlwr (char *str)
{
	char *tmp = str;

	while (*tmp)
	{
		if (isupper (*tmp))
			*tmp = tolower (*tmp);
		++tmp;
	}
	return str;
}

long filelength (int fd)
{
	struct stat st;
	int val;

	val = fstat (fd, &st);
	if (val == -1)
		return (-1L);

	return (st.st_size);
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
	int status;
	int jours = 40;
	int obsolete = 90;
	int deb_zone = 0xffff;

	*upd_file = '\0';

	if ((ac >= 2) && (strcmp (strupr (av[1]), "/H") == 0))
	{
		fprintf (stderr, "\nEPURWP V%s\n", VERSION);
		fprintf (stderr, "format  : EPURWP [upd-days [kill-days [update-file]]]\n");
		fprintf (stderr, "defaults: upd-days = 40\n          kill-days= 90\n\n");
		fprintf (stderr, "If temporary part is stable during \"upd-days\" days,\n");
		fprintf (stderr, "temporary part updates main part of WP\n\n");
		fprintf (stderr, "If the callsign is not seen during \"kill-days\" days,\n");
		fprintf (stderr, "callsign is deleted from WP database\n\n");
		fprintf (stderr, "\"update-file\" stores the WP records corresponding to the\n");
		fprintf (stderr, "updates in a file which can be processed later (eg for statistics)\n\n");
		exit (0);
	}

	if (strncmp (strupr (av[ac - 1]), "/NOC", 4) == 0)
	{
		addr_check = 0;
		--ac;
	}

	if (ac >= 2)
		jours = atoi (av[1]);

	if (ac >= 3)
		obsolete = atoi (av[2]);

	if (ac >= 4)
		strcpy (upd_file, av[3]);

	/*
	   if (ac >= 4)
	   deb_zone = (unsigned)atoi(av[3]) * 1000;
	 */

	fprintf (stderr, "\nEpurwp V%s\n\n", VERSION);

	heure = time (NULL);

	defauts (jours, obsolete);

#ifdef ENGLISH
	fprintf (stderr, "Updating white pages database    \n");
	fprintf (stderr, "  Update time is %d days\n", jours);
	if (obsolete)
		fprintf (stderr, "  Delete time is %d days\n\n", obsolete);
#else
	fprintf (stderr, "Mise … jour de la base de donn‚es\n");
	fprintf (stderr, "  D‚lai       : %d jours\n", jours);
	if (obsolete)
		fprintf (stderr, "  Suppression : %d jours\n\n", obsolete);
#endif

	if ((fptr_mess = fopen (wp_mess, "at")) == NULL)
	{
#ifdef ENGLISH
		fprintf (stderr, "Cannot create '%s'  \n", wp_mess);
#else
		fprintf (stderr, "Erreur creation '%s'\n", wp_mess);
#endif

		exit (1);
	}

#ifdef ENGLISH
	fprintf (stderr, "%s - Saves WP.SYS file into WP.OLD file    \n\n", date_mbl_new (time (NULL)));
#else
	fprintf (stderr, "%s - Sauvegarde du fichier WP.SYS en WP.OLD\n\n", date_mbl_new (time (NULL)));
#endif

	if (!copy_ (wp_sys, wp_old))
	{
		unlink (wp_old);
		exit (2);
	}

	if ((fpti = fopen (wp_old, "rb")) == NULL)
	{
#ifdef ENGLISH
		fprintf (stderr, "Cannot open '%s'     \n", wp_old);
#else
		fprintf (stderr, "Erreur ouverture '%s'\n", wp_old);
#endif
		exit (3);
	}

	if ((fpto = fopen (wp_sys, "wb")) == NULL)
	{
#ifdef ENGLISH
		fprintf (stderr, "Cannot open '%s'     \n", wp_sys);
#else
		fprintf (stderr, "Erreur ouverture '%s'\n", wp_sys);
#endif
		exit (4);
	}

	if (*upd_file)
	{
		if ((fptr_upd = fopen (upd_file, "ab")) == NULL)
		{
#ifdef ENGLISH
			fprintf (stderr, "Cannot open '%s'     \n", upd_file);
#else
			fprintf (stderr, "Erreur ouverture '%s'\n", upd_file);
#endif
			exit (5);
		}
	}
	else
		fptr_upd = NULL;

	init_zone (filelength (fileno (fpti)) / (long) sizeof (Wps));

	while (fread (&rec, sizeof (Wps), 1, fpti))
	{

		status = check_record (&rec);

		if (record_in >= deb_zone)
			add_zone (&rec);

		if (status == 2)
			++update;

		if (status)
		{
			if (fwrite (&rec, sizeof (Wps), 1, fpto) == 0)
			{

#ifdef ENGLISH
				fprintf (stderr, "Error while writing file '%s'\n", wp_sys);
#else
				fprintf (stderr, "Erreur ecriture fichier '%s'  \n", wp_sys);
#endif

				exit (1);
			}
			++record_out;
		}
		else if (rec.last_modif)
			++destroy;

		if ((*rec.callsign) && ((++record % 100) == 0))
#ifdef __linux__
			fprintf (stderr, "\nRecord %ld", record);
#else
			fprintf (stderr, "\rRecord %ld", record);
#endif

		++record_in;
	}

#if 0
	memset (&rec, '\0', sizeof (Wps));

	while (record_out < record_in)
	{
		fseek (fpto, record_out * sizeof (Wps), SEEK_SET);
		if (fwrite (&rec, sizeof (Wps), 1, fpto) == 0)
		{

#ifdef ENGLISH
			fprintf (stderr, "Error while writing file '%s'\n", wp_sys);
#else
			fprintf (stderr, "Erreur ecriture fichier '%s'  \n", wp_sys);
#endif

			exit (1);
		}
		++record_out;
	}
#endif

	if (fptr_upd)
		fclose (fptr_upd);

	fclose (fpti);
	fclose (fpto);
	fclose (fptr_mess);

	fprintf (stderr, "\rRecord %ld\n\n", record);

	print_compte_rendu ();

	return (0);
}

void init_zone (long lg)
{
	int i;

	long nbbloc = (lg / 1000L) + 1L;

	if (deb_zone == 0xffff)
		deb_zone = 1000 * (unsigned) (heure % nbbloc);

/*
   printf("%ld records, %ld blocs, heure = %ld zone = %u\n",
   lg, nbbloc, heure, deb_zone);
 */

	for (i = 0; i < 1000; i++)
		zone[i] = 0L;
}

int is_dupe (Wps * rec)
{
	lcall lc;
	int i;

	lc = call2l (rec->callsign);
	for (i = 0; i < 1000; i++)
	{
		if (lc == zone[i])
		{
			++dupes;
			fprintf (stderr, "\rRecord %ld (%s) duplicated\n", record, rec->callsign);
			return (1);
		}
	}
	return (0);
}

void add_zone (Wps * rec)
{
	static int index = 0;

	if (index == 1000)
		return;

	zone[index++] = call2l (rec->callsign);
}

int check_record (Wps * rec)
{
	int modif = 1;

	rec->callsign[6] = '\0';
	rec->name[12] = '\0';
	rec->first_homebbs[40] = '\0';
	rec->secnd_homebbs[40] = '\0';
	rec->first_zip[8] = '\0';
	rec->secnd_zip[8] = '\0';
	rec->first_qth[30] = '\0';
	rec->secnd_qth[30] = '\0';

	if ((!find (rec->callsign)) || (*rec->first_homebbs == '\0') || (*rec->first_homebbs == '?'))
		return (0);

	if ((addr_check) && (!addr_ok (rec->first_homebbs)))
	{
		return (0);
	}

	if (is_dupe (rec))
		return (0);

	if (rec->last_seen < kill_date)
	{
		/* Entree obsolete */
		return (0);
	}

	if (rec->last_modif < tst_date)
	{
		/* Mise a jour */
		if (strncmp (rec->first_homebbs, rec->secnd_homebbs, 40) != 0)
		{
			strn_cpy (rec->first_homebbs, rec->secnd_homebbs, 40);
			modif = 2;
		}
		if (strncmp (rec->first_zip, rec->secnd_zip, 8) != 0)
		{
			strn_cpy (rec->first_zip, rec->secnd_zip, 8);
			modif = 2;
		}
		if (strncmp (rec->first_qth, rec->secnd_qth, 30) != 0)
		{
			strn_cpy (rec->first_qth, rec->secnd_qth, 30);
			modif = 2;
		}
	}

	if (rec->changed)
	{
		wp_message (rec);
		rec->changed = 0;
	}

	return (modif);
}

void print_compte_rendu (void)
{
	FILE *fcr;

#ifdef ENGLISH

	printf ("\n");
	printf ("WP updated : %5ld total records        \n", record);
	printf ("           : %5ld updated record(s)    \n", update);
	printf ("           : %5ld deleted record(s)    \n", destroy);
	printf ("           : %5ld WP update line(s)    \n\n", lines_out);

	if ((fcr = fopen (compte_rendu, "wt")) != NULL)
	{
		fprintf (fcr, "%ld\n\n", heure);
		fprintf (fcr, "WP updated : %5ld total record(s)      \n", record);
		fprintf (fcr, "           : %5ld updated record(s)    \n", update);
		fprintf (fcr, "           : %5ld deleted records(s)   \n", destroy);
		fprintf (fcr, "           : %5ld WP update line(s)    \n\n", lines_out);

		fprintf (fcr, "Start computing     : %s\n", strdt (heure));
		fprintf (fcr, "End computing       : %s\n", strdt (time (NULL)));
		fclose (fcr);
	}

#else

	printf ("\n");
	printf ("Pages Blanches : %5ld enregistrement(s)\n", record);
	printf ("               : %5ld mise(s) … jour   \n", update);
	printf ("               : %5ld suppression(s)   \n", destroy);
	printf ("               : %5ld lignes de m.a.j. \n\n", lines_out);

	if ((fcr = fopen (compte_rendu, "wt")) != NULL)
	{
		fprintf (fcr, "%ld\n\n", heure);
		fprintf (fcr, "Pages Blanches : %5ld enregistrement(s)\n", record);
		fprintf (fcr, "               : %5ld mise(s) … jour   \n", update);
		fprintf (fcr, "               : %5ld suppression(s)   \n", destroy);
		fprintf (fcr, "               : %5ld lignes de m.a.j. \n\n", lines_out);

		fprintf (fcr, "Debut du traitement : %s\n", strdt (heure));
		fprintf (fcr, "Fin du traitement   : %s\n", strdt (time (NULL)));
		fclose (fcr);
	}

#endif

}


void defauts (int jours, int obsolete)
{
	unsigned int flag;
	char *ptr;
	char system_dir[256];
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
		fprintf (stderr, "Pas de numéro dans le fbb.conf\n");
#endif
		exit (1);
	}
	sprintf (temp, "FBB%s", VERSION);

	/* Only test the major number ... */
	if (strncasecmp (temp, ptr, 4) != 0)
	{
#ifdef ENGLISH
		fprintf (stderr, "Wrong version number in fbb.conf\n");
#else
		fprintf (stderr, "Numéro de version erroné dans fbb.conf\n");
#endif
		exit (1);
	}
#ifdef __linux__
	fprintf (stderr, "Configuration version : %s\n", ptr);
#else
	fprintf (stderr, "Configuration version : %s\r", ptr);
#endif
	
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

#ifdef __linux__
	sprintf (wp_sys, "%swp/wp.sys", system_dir);
	sprintf (wp_old, "%swp/wp.old", system_dir);
	sprintf (wp_mess, "%swp/mess.wp", system_dir);
	strcpy (compte_rendu, "epurwp.res");
#else
	sprintf (wp_sys, "%sWP\\WP.SYS", system_dir);
	sprintf (wp_old, "%sWP\\WP.OLD", system_dir);
	sprintf (wp_mess, "%sWP\\MESS.WP", system_dir);
	strcpy (compte_rendu, "EPURWP.RES");
#endif


	tst_date = heure - (long) jours *86400L;
	kill_date = heure - (long) obsolete *86400L;
}


char *strdt (long temps)
{
	struct tm *sdate;
	static char cdate[80];

	sdate = localtime (&temps);

#ifdef ENGLISH
	sprintf (cdate, "%02d-%02d-%02d %02d:%02d",
			 sdate->tm_year % 100,
			 sdate->tm_mon + 1,
			 sdate->tm_mday,
			 sdate->tm_hour,
			 sdate->tm_min);
#else
	sprintf (cdate, "%02d/%02d/%02d %02d:%02d",
			 sdate->tm_mday,
			 sdate->tm_mon + 1,
			 sdate->tm_year % 100,
			 sdate->tm_hour,
			 sdate->tm_min);
#endif

	return (cdate);
}


char *date_mbl_new (long temps)
{
	struct tm *sdate;
	static char cdate[40];

	sdate = localtime (&temps);
#ifdef ENGLISH
	sprintf (cdate, "%02d-%02d-%02d", sdate->tm_year % 100, sdate->tm_mon + 1, sdate->tm_mday);
#else
	sprintf (cdate, "%02d/%02d/%02d", sdate->tm_mday, sdate->tm_mon + 1, sdate->tm_year % 100);
#endif
	return (cdate);
}

void strn_cpy (char *dest, char *source, int len)
{
	for (;;)
	{
		if ((len-- == 0) || (*source == '\0'))
			break;
		*dest++ = *source++;
	}
	*dest = '\0';
}

int find (char *s)
{
	char *t = s;
	int n = 0;
	int dernier = 0, chiffre = 0, lettre = 0;

	while (isalnum (*t))
	{
		*t = toupper (*t);
		dernier = (isdigit (*t));

		if (isdigit (*t))
			++chiffre;
		else
			++lettre;

		++t;
		++n;
	}
	*t = '\0';

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

	return (1);
}


static char *wp_date_mbl (long temps)
{
	struct tm *sdate;
	static char cdate[7];

	sdate = localtime (&temps);
	sprintf (cdate, "%02d%02d%02d", sdate->tm_year % 100, sdate->tm_mon + 1, sdate->tm_mday);
	return (cdate);
}


void wp_message (Wps * rec)
{
	if (fptr_upd)
	{
		fwrite (rec, sizeof (Wps), 1, fptr_upd);
	}

	if ((rec->changed == 'U') || (rec->changed == 'G') || (rec->changed == 'I'))
	{
		fprintf (fptr_mess, "On %s %s/%c @ %s zip %s %s %s\n",
				 wp_date_mbl (rec->last_modif),
				 rec->callsign,
				 rec->changed,
				 (*rec->secnd_homebbs) ? rec->secnd_homebbs : "?",
				 (*rec->secnd_zip) ? rec->secnd_zip : "?",
				 (*rec->name) ? rec->name : "?",
				 (*rec->secnd_qth) ? rec->secnd_qth : "?"
			);

		++lines_out;
	}
}

#if 0
int copy_ (char *oldfich, char *newfich)
{
	char s[256];

#ifdef __linux__
	sprintf (s, "cp %s %s > /dev/null", oldfich, newfich);
#else
	sprintf (s, "COPY %s %s > NUL", oldfich, newfich);
#endif
	system (s);
	return (1);
}
#endif


#define TAIBUF 16384
static char buffer[TAIBUF];

int copy_ (char *oldfich, char *newfich)
{
	int retour = 1;
	int fd_orig;
	int fd_dest;
	int nb_lus;
	int ret;
	int dest_access;

	if ((fd_orig = open (oldfich, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == EOF)
	{
		fprintf (stderr, "Cannot find %s\n", oldfich);
		return (0);
	}

	dest_access = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY;
	if ((fd_dest = open (newfich, dest_access, S_IREAD | S_IWRITE)) == EOF)
	{
		close (fd_orig);
		fprintf (stderr, "Cannot create %s\n", newfich);
		return (0);
	}

	for (;;)
	{

		nb_lus = _read (fd_orig, buffer, TAIBUF);

		if (nb_lus == -1)
		{
			retour = 0;
			break;
		}

		if (nb_lus == 0)
		{
			retour = 1;
			break;
		}

		ret = _write (fd_dest, buffer, nb_lus);

		if (ret != nb_lus)
		{
			retour = 0;
			break;
		}

	}

	close (fd_orig);
	close (fd_dest);

	return (retour);
}


lcall call2l (char *callsign)
{
	char *ptr = callsign;
	int c;
	lcall val = 0L;

	while ((c = (int) *ptr++) != 0)
	{
		if (c < 48)
			return (0xffffffffL);
		c -= 47;
		if (c > 10)
		{
			c -= 7;
			if (c > 36)
			{
				return (0xffffffffL);
			}
			else if (c < 11)
			{
				return (0xffffffffL);
			}
		}
		val *= 37;
		val += c;
	}
	return (val);
}

int addr_ok (char *s)
{
	int nb = 0;
	int total = 0;

	while (*s)
	{
		if (*s == '.')
		{
			nb = 0;
		}
		else
		{
			if (nb == 6)
			{
				return (0);
			}
			++nb;
		}
		++s;
		if (++total == 31)
			return (0);
	}
	return (1);
}
