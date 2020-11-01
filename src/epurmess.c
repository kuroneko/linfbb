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

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <string.h>

#ifdef __linux__
#include <unistd.h>
#else
#include <io.h>
#include <dos.h>
#include <process.h>
#include <dir.h>
#endif

#include <fbb_conf.h>
#include <config.h> 

#define DEBUG   1
#define TRUE    1
#define FALSE   0

#ifdef ENGLISH

#define EXPI "Expi"
#define ARCH "Arch"
#define LECT "Read"
#define SUPP "Kill"
#define DETR "Dest"

#else

#define EXPI "Expi"
#define ARCH "Arch"
#define LECT "Lect"
#define SUPP "Supp"
#define DETR "Detr"

#endif

#ifdef __linux__

#define _read read
#define _write write
#define O_BINARY 0
#define __a2__ __attribute__ ((packed, aligned(2)))
#define stricmp strcasecmp
#define strcmpi strcasecmp
#define strnicmp strncasecmp
#define strncmpi strncasecmp

#else

#define __a2__

#endif

char cdate[80];
long numtot, numenr, nomess, numsup, numarc, nbtot;
long nbp, nbb;

char mycall[7], expediteur[7], destinataire[7];
char mypath[80];
long mdate;

#define MAX_RENUM (14L * 0x10000L)
#define DEL_RENUM (MAX_RENUM - 0x20000L)

#define NBBBS	80
#define NBMASK	NBBBS/8
#define	LGFREE	5

typedef struct
{								/* Longueur = 194 octets */
	char type;
	char status;
	long numero __a2__;
	long taille __a2__;
	long date __a2__;
	char bbsf[7];
	char bbsv[41];
	char exped[7];
	char desti[7];
	char bid[13];
	char titre[61];
	char bin;
	char free[5];
	long grpnum __a2__;
	unsigned short nblu;
	long theme __a2__;
	long datesd __a2__;
	long datech __a2__;
	char fbbs[NBMASK];
	char forw[NBMASK];
}
bullist;

typedef struct typ_serlist
{
	char nom[7];
	char delai;
	struct typ_serlist *suiv;
}
deslist;

typedef struct
{
	long pn;
	long py;
	long pf;
	long px;
	long pk;
	long bn;
	long by;
	long bf;
	long bx;
	long bk;
	long bd;
	long rt;
	long rr;
}
param;

#define	BUFFSIZE	257
#define	NB_AUTOMESS	2

int auto_mess[NB_AUTOMESS];
char text_rt[NB_AUTOMESS][BUFFSIZE];
char *mail_in;
char *callsign;
int nb_return_lines;

void aff_status (bullist *);
void change_defauts (param *, bullist *);
int copy_ (char *, char *);
void defauts (void);
void ent_arch (FILE *, bullist *);
void entete_liste (void);
void erreur_arg (int);
void init_liste (char *, int);
void message_retour (int, bullist *, long);
void newname (char *, char *);
void print_compte_rendu (void);
void print_zone(void);

char *date_mbl_new (long);
char *strdt (long);

int semaine (void);
int is_route (char *fbbs);

deslist *tete_exped, *tete_route, *tete_desti;
int *tabrec;
char *dirmes_sys;
char *dirmes_old;
char *dirmes_new;
char *compte_rendu;
char *old_mail;
char *mail;
char *binmail;
int archive_p, archive_b;
int old_format;
int ext_call = 0;
long heure;

param max, def;

FILE *fptr, *dptr;

extern long timezone;

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

char *back2slash (char *str)
{
	char *tmp = str;

	while (*tmp)
	{
		if (isupper (*tmp))
			*tmp = tolower (*tmp);
		if (*tmp == '\\')
			*tmp = '/';
		++tmp;
	}
	return str;
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
	fprintf(stderr, "Error : keyword \"%s\" missing in fbb.conf file\n", keyword);
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

static int fbb_unlink (char *filename)
{
	int ret = unlink (filename);

	return (ret);
}

int main (int ac, char **av)
{
	int i;
	char *s;
	char *old_name;
	char *new_name;
	char *bin_name;
	char *buffer;
	int ecrit, unite;
	bullist entete;
	long record;
	FILE *fcr, *fpti, *fpto;
	long minimum;
	long maximum;
	int renum = 0;

	mail_in      = (char *) (calloc(80 , sizeof(char)));
	callsign     = (char *) (calloc(40 , sizeof(char)));
	s            = (char *) (calloc(BUFFSIZE , sizeof(char)));
	old_name     = (char *) (calloc(BUFFSIZE , sizeof(char)));
	new_name     = (char *) (calloc(BUFFSIZE , sizeof(char)));
	bin_name     = (char *) (calloc(BUFFSIZE , sizeof(char)));
	buffer       = (char *) (calloc(BUFFSIZE , sizeof(char)));
	dirmes_sys   = (char *) (calloc(BUFFSIZE , sizeof(char)));
	dirmes_old   = (char *) (calloc(BUFFSIZE , sizeof(char)));
	dirmes_new   = (char *) (calloc(BUFFSIZE , sizeof(char)));
	compte_rendu = (char *) (calloc(BUFFSIZE , sizeof(char)));
	old_mail     = (char *) (calloc(BUFFSIZE , sizeof(char)));
	mail         = (char *) (calloc(BUFFSIZE , sizeof(char)));
	binmail      = (char *) (calloc(BUFFSIZE , sizeof(char)));

	old_format = 0;
	tete_exped = tete_route = tete_desti = NULL;
	for (i = 0; i < NB_AUTOMESS; i++)
		auto_mess[i] = 0;

	printf ("Epurmess V%s\n\n", VERSION);

	for (i = 1; i < ac; i++)
	{
		if (strnicmp ("/O", av[i], 2) == 0)
		{
			old_format = 1;
			printf ("Archive in old format\n");
		}
	}

	defauts ();

	if (access (dirmes_sys, 0))
	{
		fprintf (stderr, "Cannot find %s\n", dirmes_sys);
		exit (0);
	}

	heure = time (NULL);
	minimum = heure - (10L * 12L * 30L * 24L * 3600L);
	maximum = heure + (10L * 12L * 30L * 24L * 3600L);

	if ((fcr = fopen (compte_rendu, "rt")) != NULL)
	{
		if (fread (s, 80, 1, fcr))
		{
			fclose (fcr);
/*			if ((heure - atol (s)) > 864000L)
			{
				fcr = fopen (compte_rendu, "wt");
#ifdef ENGLISH
				fprintf (fcr, "0\n\n");
				fprintf (fcr, "EPURMESS was not done because your last EPURMESS was done more than 24h ago.  \n\n");
				fprintf (fcr, "Please verify the system clock!!!\n\n");
				fprintf (fcr, "Kill this file (%s) to validate EPURMESS.         \n", compte_rendu);
				printf ("Last update > 24h. EPURMESS was not done.                 \n");
#else
				fprintf (fcr, "0\n\n");
				fprintf (fcr, "EPURMESS n'a pas ‚t‚ valid‚ car la derniŠre mise a jour remonte … plus de 24h.\n\n");
				fprintf (fcr, "V‚rifiez votre horloge !!!       \n\n");
				fprintf (fcr, "Supprimer ce fichier (%s) pour revalider EPURMESS.\n", compte_rendu);
				printf ("Date de derniŠre mise … jour > 24h. EPURMESS non effectu‚.\n");
#endif
				fclose (fcr);
				exit (1);
			}
		}
		else
			fclose (fcr);
*/
		}

	}

#ifdef ENGLISH
	printf ("%s - Saves dirmes.sys file into dirmes.old file    \n\n", date_mbl_new (time (NULL)));
#else
	printf ("%s - Sauvegarde du fichier dirmes.sys en dirmes.old\n\n", date_mbl_new (time (NULL)));
#endif

	if (!copy_ (dirmes_sys, dirmes_old))
	{
#ifdef ENGLISH
		printf ("%s - Cannot save dirmes.sys file into dirmes.old file !!         \n\n", date_mbl_new (time (NULL)));
#else
		printf ("%s - Sauvegarde du fichier dirmes.sys en dirmes.old impossible !!\n\n", date_mbl_new (time (NULL)));
#endif

	}

	if ((fptr = fopen (dirmes_sys, "rb")) == NULL)
	{

#ifdef ENGLISH
		printf ("Cannot open '%s'     \n", dirmes_sys);
#else
		printf ("Erreur ouverture '%s'\n", dirmes_sys);
#endif

		exit (1);
	}

	if ((dptr = fopen (dirmes_new, "wb")) == NULL)
	{
		exit (1);
	}

	nbp = nbb = numtot = numenr = numsup = numarc = 0L;

	entete_liste ();

	if (fread (&entete, sizeof (bullist), 1, fptr) == 0)
	{

#ifdef ENGLISH
		printf ("Error while reading file '%s'\n", dirmes_sys);
#else
		printf ("Erreur lecture fichier '%s'  \n", dirmes_sys);
#endif

		exit (1);
	}

	if (entete.numero > MAX_RENUM)
	{
		renum = 1;
		entete.numero -= DEL_RENUM;
	}

	if (fwrite (&entete, sizeof (bullist), 1, dptr) == 0)
	{
#ifdef ENGLISH
		printf ("Error while writing file '%s'\n", dirmes_new);
#else
		printf ("Erreur ecriture fichier '%s'  \n", dirmes_new);
#endif

		exit (1);
	}
	record = 1L;
	strncpy (mycall, entete.exped, sizeof (mycall));

	while (1)
	{
		ecrit = 1;
		if (fread (&entete, sizeof (bullist), 1, fptr) == 0)
			break;

		entete.bbsf[6] = '\0';
		entete.bbsv[40] = '\0';
		entete.exped[6] = '\0';
		entete.desti[6] = '\0';
		entete.bid[12] = '\0';
		entete.titre[60] = '\0';

		if (renum)
		{
			if ((entete.numero - DEL_RENUM) < 1L)
			{
				entete.status = 'A';
			}
		}

		memset (entete.free, '\0', LGFREE);

		unite = (unsigned int) (entete.numero % 10);

		if (entete.type == '\0')
		{
#ifdef __linux__
			sprintf (old_name, "%smail%d/m_%06ld.mes", mail, unite, entete.numero);
			sprintf (bin_name, "%smail%d/m_%06ld.mes", binmail, unite, entete.numero);
#else
			sprintf (old_name, "%sMAIL%d\\M_%06ld.MES", mail, unite, entete.numero);
			sprintf (bin_name, "%sMAIL%d\\M_%06ld.MES", binmail, unite, entete.numero);
#endif
			fbb_unlink (old_name);
			fbb_unlink (bin_name);
			continue;
		}

		aff_status (&entete);
		def = max;
		change_defauts (&def, &entete);

		if ((entete.datech < minimum) || (entete.datech > maximum))
			entete.status = 'A';

		if ((entete.date < minimum) || (entete.date > maximum))
			entete.status = 'A';

		if ((entete.datesd < minimum) || (entete.datesd > maximum))
			entete.datesd = entete.date;

		/* Message erronne */
		if ((entete.taille > 10000000L) || (entete.taille < 0))
			entete.status = 'A';

		if (entete.datech > heure)
			entete.datech = heure;

		if (entete.date > heure)
			entete.date = heure;

#ifdef __linux__
		sprintf (old_name, "%smail%d/m_%06ld.mes", mail, unite, entete.numero);
		sprintf (bin_name, "%smail%d/m_%06ld.mes", binmail, unite, entete.numero);
#else
		sprintf (old_name, "%sMAIL%d\\M_%06ld.MES", mail, unite, entete.numero);
		sprintf (bin_name, "%sMAIL%d\\M_%06ld.MES", binmail, unite, entete.numero);
#endif

		if ((entete.type == 'P') && (entete.status == 'N') && (*entete.bbsv))
		{
			if (!is_route (entete.fbbs) && (def.rr) && (heure - entete.date >= def.rr))
			{
				message_retour (1, &entete, heure);
				entete.status = 'A';
			}
			else if (!is_route (entete.forw) && (def.rt) && (heure - entete.date >= def.rt))
			{
				message_retour (0, &entete, heure);
				entete.status = 'A';
			}
		}

		if ((entete.type == 'A') && (entete.status != 'N'))
		{
			/* Destruction immediate */
			printf (DETR);
			fbb_unlink (old_name);
			fbb_unlink (bin_name);
			++numsup;
			ecrit = 0;
		}
		else if ((entete.type == 'P') || (entete.type == 'A'))
		{
			if ((entete.status == '$') && (heure - entete.datech >= def.bd))
			{
				entete.status = 'X';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (EXPI);
			}
			if ((entete.status == 'N') && (heure - entete.datech >= def.pn))
			{
				entete.status = 'X';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (EXPI);
			}
			if ((entete.status == 'Y') && (heure - entete.datech >= def.py))
			{
				entete.status = 'X';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (EXPI);
			}
			if ((entete.status == 'F') && (heure - entete.datech >= def.pf))
			{
				entete.status = 'K';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (SUPP);
			}
			if ((entete.status == 'X') && (heure - entete.datech >= def.px))
			{
				entete.status = 'K';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (SUPP);
			}
			if ((entete.status == 'K') && (heure - entete.datech >= def.pk))
			{
				entete.status = 'A';
				entete.datech = heure;
			}
			if (entete.status == 'A')
			{
				/* Archivage immediat */
				fbb_unlink (bin_name);
				if (archive_p)
				{
					sprintf (new_name, "%sPRIV.%02d", old_mail, semaine ());
					if ((fpto = fopen (new_name, "at")) != NULL)
					{
						ent_arch (fpto, &entete);
						if ((fpti = fopen (old_name, "rt")) != NULL)
						{
							while (fgets (buffer, 256, fpti))
							{
								fputs (buffer, fpto);
							}
							fclose (fpti);
						}
						fprintf (fpto, "\n/EX\n");
						fclose (fpto);
					}
					printf (ARCH);
					++numarc;
				}
				else
				{
					printf (DETR);
					++numsup;
				}
				fbb_unlink (old_name);
				ecrit = 0;
			}
			if (ecrit)
				++nbp;
		}
		else
		{
			if ((entete.status == 'N') && (heure - entete.datech >= def.bn))
			{
				entete.status = 'X';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (EXPI);
			}
			if ((entete.status == '$') && (heure - entete.datech >= def.bd))
			{
				entete.status = 'X';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (EXPI);
			}
			if ((entete.status == 'Y') && (heure - entete.datech >= def.by))
			{
				entete.status = 'X';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (EXPI);
			}
			if ((entete.status == 'F') && (heure - entete.datech >= def.bf))
			{
				entete.status = 'K';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (SUPP);
			}
			if ((entete.status == 'X') && (heure - entete.datech >= def.bx))
			{
				entete.status = 'K';
				entete.datech = heure;
				fbb_unlink (bin_name);
				printf (SUPP);
			}
			if ((entete.status == 'K') && (heure - entete.datech >= def.bk))
			{
				entete.status = 'A';
				entete.datech = heure;
			}
			if (entete.status == 'A')
			{
				/* Archivage immediat */
				fbb_unlink (bin_name);
				if (archive_b)
				{
					sprintf (new_name, "%sBULL.%02d", old_mail, semaine ());
					if ((fpto = fopen (new_name, "at")) != NULL)
					{
						ent_arch (fpto, &entete);
						if ((fpti = fopen (old_name, "rt")) != NULL)
						{
							while (fgets (buffer, 256, fpti))
								fputs (buffer, fpto);
							fclose (fpti);
						}
						fprintf (fpto, "\n/EX\n");
						fclose (fpto);
					}
					printf (ARCH);
					++numarc;
				}
				else
				{
					printf (DETR);
					++numsup;
				}
				fbb_unlink (old_name);
				ecrit = 0;
			}
			if (ecrit)
				++nbb;
		}
		if (ecrit)
		{
			++numtot;
			if (entete.status != 'K')
				++numenr;
			++record;
		}

		if (ecrit)
		{
			if (renum)
			{
				char ren_name[257];

				entete.numero -= DEL_RENUM;
				unite = (unsigned int) (entete.numero % 10);

#ifdef __linux__
				sprintf (ren_name, "%smail%d/m_%06ld.mes", mail, unite, entete.numero);
#else
				sprintf (ren_name, "%sMAIL%d\\M_%06ld.MES", mail, unite, entete.numero);
#endif

				/* delete the binary file */
				unlink (bin_name);

				/* rename the ascii file */
				newname (old_name, ren_name);
			}

			if (fwrite (&entete, sizeof (bullist), 1, dptr) == 0)
			{
#ifdef ENGLISH
				printf ("Error while writing file '%s'\n", dirmes_old);
#else
				printf ("Erreur ecriture fichier '%s'  \n", dirmes_old);
#endif
				exit (1);
			}
		}
		putchar ('\n');
	}
	
	putchar ('\n');

	fclose (dptr);
	fclose (fptr);

#ifdef ENGLISH
	printf ("Saves into dirmes.old - Copies dirmes.new into dirmes.sys      \n");
#else
	printf ("Sauvegarde dans dirmes.old - Recopie dirmes.new dans dirmes.sys\n");
#endif

	if (!copy_ (dirmes_new, dirmes_sys))
	{
#ifdef ENGLISH
		printf ("%s - Cannot save dirmes.new file into dirmes.sys file !!         \n\n", date_mbl_new (time (NULL)));
#else
		printf ("%s - Sauvegarde du fichier dirmes.new en dirmes.sys impossible !!\n\n", date_mbl_new (time (NULL)));
#endif
	}

	print_compte_rendu ();
	return (0);
}


void print_compte_rendu ()
{
	FILE *fcr;

#ifdef ENGLISH

	printf ("\n");
	printf ("File cleared  : %4ld private message(s)    \n", nbp);
	printf ("              : %4ld bulletin message(s)   \n", nbb);
	printf ("              : %4ld active message(s)     \n", numenr);
	printf ("              : %4ld killed message(s)     \n", numtot - numenr);
	printf ("              : %4ld total message(s)      \n", numtot);
	printf ("              : %4ld archived message(s)   \n", numarc);
	printf ("              : %4ld destroyed message(s)  \n", numsup);
	printf ("              : %4d Timed-out message(s)  \n", auto_mess[0]);
	printf ("              : %4d No-Route message(s)   \n\n", auto_mess[1]);

	if ((fcr = fopen (compte_rendu, "wt")) != NULL)
	{
		fprintf (fcr, "%ld\n\n", heure);
		fprintf (fcr, "File cleared  : %4ld private message(s)    \n", nbp);
		fprintf (fcr, "              : %4ld bulletin message(s)   \n", nbb);
		fprintf (fcr, "              : %4ld active message(s)     \n", numenr);
		fprintf (fcr, "              : %4ld killed message(s)     \n", numtot - numenr);
		fprintf (fcr, "              : %4ld total message(s)      \n", numtot);
		fprintf (fcr, "              : %4ld archived message(s)   \n", numarc);
		fprintf (fcr, "              : %4ld destroyed message(s)  \n", numsup);
		fprintf (fcr, "              : %4d Timed-out message(s)  \n", auto_mess[0]);
		fprintf (fcr, "              : %4d No-Route message(s)   \n\n", auto_mess[1]);

		fprintf (fcr, "Start computing     : %s\n", strdt (heure));
		fprintf (fcr, "End computing       : %s\n", strdt (time (NULL)));
		fclose (fcr);
	}

#else

	printf ("\n");
	printf ("Fichier ‚pur‚ : %4ld message(s) priv‚(s)   \n", nbp);
	printf ("              : %4ld message(s) bulletin(s)\n", nbb);
	printf ("              : %4ld message(s) actif(s)   \n", numenr);
	printf ("              : %4ld message(s) supprim‚(s)\n", numtot - numenr);
	printf ("              : %4ld message(s) total      \n", numtot);
	printf ("              : %4ld message(s) archiv‚(s) \n", numarc);
	printf ("              : %4ld message(s) d‚truit(s) \n", numsup);
	printf ("              : %4d message(s) oubli‚(s)  \n", auto_mess[0]);
	printf ("              : %4d message(s) sans route \n\n", auto_mess[1]);

	if (fcr = fopen (compte_rendu, "wt"))
	{
		fprintf (fcr, "%ld\n\n", heure);
		fprintf (fcr, "Fichier ‚pur‚ : %4ld message(s) priv‚(s)   \n", nbp);
		fprintf (fcr, "              : %4ld message(s) bulletin(s)\n", nbb);
		fprintf (fcr, "              : %4ld message(s) actif(s)   \n", numenr);
		fprintf (fcr, "              : %4ld message(s) supprim‚(s)\n", numtot - numenr);
		fprintf (fcr, "              : %4ld message(s) total      \n", numtot);
		fprintf (fcr, "              : %4ld message(s) archiv‚(s) \n", numarc);
		fprintf (fcr, "              : %4ld message(s) d‚truit(s) \n", numsup);
		fprintf (fcr, "              : %4d message(s) oubli‚(s)  \n", auto_mess[0]);
		fprintf (fcr, "              : %4d message(s) sans route \n\n", auto_mess[1]);

		fprintf (fcr, "Debut du traitement : %s\n", strdt (heure));
		fprintf (fcr, "Fin du traitement   : %s\n", strdt (time (NULL)));
		fclose (fcr);
	}

#endif

}


void change_defauts (param * def, bullist * bull)
{
	deslist *lptr;

	lptr = tete_exped;
	while (lptr)
	{
		if (strcmp (lptr->nom, bull->exped) == 0)
		{
			def->bf = def->bx = def->pf = def->px = (long) lptr->delai * 86400L;
			break;
		}
		lptr = lptr->suiv;
	}

	lptr = tete_route;
	while (lptr)
	{
		if (strcmp (lptr->nom, bull->bbsv) == 0)
		{
			def->bf = def->bx = def->pf = def->px = (long) lptr->delai * 86400L;
			break;
		}
		lptr = lptr->suiv;
	}

	lptr = tete_desti;
	while (lptr)
	{
		if (strcmp (lptr->nom, bull->desti) == 0)
		{
			def->bf = def->bx = def->pf = def->px = (long) lptr->delai * 86400L;
			break;
		}
		lptr = lptr->suiv;
	}
}


void entete_liste (void)
{
#ifdef ENGLISH
	printf ("Act    Msg# TS  Dim. To    @ BBS   From   Date     Datexp   Subject\n");
#else
	printf ("Act    Msg# TS  Dim. Pour  @ BBS   Exp.   Date     Datexp   Sujet  \n");
#endif
}


char *bbs_via (char *s)
{
	int nb = 0;
	static char bbs[80];

	while ((*s) && (*s != '.'))
	{
		if (nb == 6)
			break;
		bbs[nb++] = *s++;
	}
	bbs[nb] = '\0';
	return (bbs);
}


void aff_status (bullist * ligne)
{
	int i;
	char *ptri, *ptro;
	char bbs_v[42], date[40], datech[40], titre[21];

	if (*(ligne->bbsv))
		sprintf (bbs_v, "@%-6s", bbs_via (ligne->bbsv));
	else
		*bbs_v = '\0';

	strcpy (datech, date_mbl_new (ligne->datech));
	strcpy (date, date_mbl_new (ligne->date));

	ptri = ligne->titre;
	ptro = titre;

	for (i = 0; i < 20; i++)
	{
		if (*ptri == '\0')
			break;
		if (*ptri == '\a')
			*ptro = ' ';
		else
			*ptro = *ptri;
		++ptri;
		++ptro;
	}
	*ptro = '\0';

#ifdef ENGLISH
	printf ("Read %6ld %c%c %5ld %-6s%7s %-6s %-6s %s %1.19s\r",
			ligne->numero, ligne->type, ligne->status,
			ligne->taille, ligne->desti, bbs_v, ligne->exped,
			date, datech, titre);
#else
	printf ("Lect %6ld %c%c %5ld %-6s%7s %-6s %-6s %s %1.19s\r",
			ligne->numero, ligne->type, ligne->status,
			ligne->taille, ligne->desti, bbs_v, ligne->exped,
			date, datech, titre);
#endif
}


int find (char *s)
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

void print_zone()
{
  char *tz;
      printf( "TZ: %s ", (tz = getenv( "TZ" )) ? tz : "not set" );
      printf( "-  daylight: %d ", daylight );
      printf( "-  timezone: %ld ", timezone );
      printf( "-  time zone names: %s %s\n",
      tzname[0], tzname[1] );
}

void defauts (void)
{
	int i;
	unsigned int flag;
	int init = 1;
	int nolig, lig;
	FILE *fptr;
	char *c_path;
	char *ligne;
	char *str;
	static char *tzlig;
	char *ptr;
	char *temp;

	c_path  = (char *) (calloc(BUFFSIZE , sizeof(char)));
	ligne   = (char *) (calloc(BUFFSIZE , sizeof(char)));
	str     = (char *) (calloc(80 , sizeof(char)));
	tzlig   = (char *) (calloc(80 , sizeof(char)));
	temp    = (char *) (calloc(20 , sizeof(char)));

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

	ptr = find_fbb_conf("call", 0);
	if (ptr == NULL)
		err_keyword("call");
		
	strcpy (callsign, strupr (ptr));
	strcpy (mypath, strupr (ptr));
	if ((ptr = strchr (callsign, '.')) != NULL)
		*ptr = '\0';
#ifdef ENGLISH
	fprintf (stderr, "Callsign       : %s\n", callsign);
	fprintf (stderr, "Address        : %s\n", mypath);
#else
	fprintf (stderr, "Indicatif      : %s\n", callsign);
	fprintf (stderr, "Addresse       : %s\n", mypath);
#endif

	/* Mail.in */
	ptr = find_fbb_conf("impo", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("impo");
	if (ptr == NULL)
		err_keyword("impo");
	strcpy (mail_in, strupr (ptr));
#ifdef __linux__
	back2slash (mail_in);
#endif
#ifdef ENGLISH
	fprintf (stderr, "MAIL.IN file   : %s\n", mail_in);
#else
	fprintf (stderr, "fichier MAIL.IN: %s\n", mail_in);
#endif

	/* path of conf files */
	ptr = find_fbb_conf("conf", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("conf");
	if (ptr == NULL)
		err_keyword("conf");
	strcpy (c_path, test_back_slash(ptr));
#ifdef ENGLISH
	fprintf (stderr, "Conf directory : %s\n", c_path);
#else
	fprintf (stderr, "Configuration  : %s\n", c_path);
#endif

	/* flags */
	ptr = find_fbb_conf("fbbf", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("fbbf");
	if (ptr == NULL)
		err_keyword("fbbf");
	flag = 0;
	sscanf (ptr, "%s %u", str, &flag);
	ext_call = ((flag & 4096) != 0);

	/* diff GMT */
	ptr = find_fbb_conf("loca", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("loca");
	if (ptr == NULL)
		err_keyword("loca");
#ifdef ENGLISH
	fprintf (stderr, "GMT difference : %d - ", atoi (ptr));
#else
	fprintf (stderr, "Diff‚rence GMT : %d - ", atoi (ptr));
#endif
	tzset ();		/* get timezone info */
	print_zone();

	putchar ('\n');
	sleep (2);

	lig = nolig = 0;
	
	sprintf(ligne, "%sepurmess.ini", c_path);
#ifdef __linux__
	if ((fptr = fopen (ligne, "r")) != NULL)
#else
	if ((fptr = fopen (ligne, "rt")) != NULL)
#endif
	{
		while (fgets (ligne, 256, fptr))
		{
			epure (ligne);
			nolig++;
			if ((*ligne == '#') || (*ligne == '\0') || (*ligne == '\032'))
				continue;

			lig++;

			if ((lig < 20) && (lig > 21))
				strupr (ligne);

			if (init)
			{
				switch (lig)
				{
				case 1:
					strcpy (mail, ligne);
#ifdef __linux__
					back2slash (mail);
#endif
					break;
				case 2:
					strcpy (binmail, ligne);
#ifdef __linux__
					back2slash (binmail);
#endif
					break;
				case 3:
					strcpy (old_mail, ligne);
#ifdef __linux__
					back2slash (old_mail);
#endif
					break;
				case 4:
					strcpy (dirmes_sys, ligne);
#ifdef __linux__
					back2slash (dirmes_sys);
#endif
					break;
				case 5:
					strcpy (dirmes_old, ligne);
#ifdef __linux__
					back2slash (dirmes_old);
#endif
					break;
				case 6:
					strcpy (dirmes_new, ligne);
#ifdef __linux__
					back2slash (dirmes_new);
#endif
					break;
				case 7:
					strcpy (compte_rendu, ligne);
#ifdef __linux__
					back2slash (compte_rendu);
#endif
					break;
				case 8:
					archive_p = archive_b = 1;
					sscanf (ligne, "%d %d", &archive_p, &archive_b);
					break;
				case 9:
					max.pn = atol (ligne) * 86400L;
					break;
				case 10:
					max.py = atol (ligne) * 86400L;
					break;
				case 11:
					max.pf = atol (ligne) * 86400L;
					break;
				case 12:
					max.px = atol (ligne) * 86400L;
					break;
				case 13:
					max.pk = atol (ligne) * 86400L;
					break;
				case 14:
					max.bn = atol (ligne) * 86400L;
					break;
				case 15:
					max.bd = atol (ligne) * 86400L;
					break;
				case 16:
					max.by = atol (ligne) * 86400L;
					break;
				case 17:
					max.bx = atol (ligne) * 86400L;
					break;
				case 18:
					max.bf = atol (ligne) * 86400L;
					break;
				case 19:
					max.bk = atol (ligne) * 86400L;
					break;
				case 20:
					sscanf (ligne, "%ld %[^\n]", &max.rt, text_rt[0]);
					if (max.rt == 0)
						*text_rt[0] = '\0';
					max.rt *= 86400L;
					break;
				case 21:
					sscanf (ligne, "%ld %[^\n]", &max.rr, text_rt[1]);
					if (max.rr == 0)
						*text_rt[1] = '\0';
					max.rr *= 86400L;
					break;
				case 22:
					nb_return_lines = atoi (ligne);
					break;
				case 23:
					if (*ligne != '-')
						erreur_arg (nolig);
					init = 0;
					break;
				default:
					erreur_arg (nolig);
					break;
				}
			}
			else
			{
				if (*ligne == '-')
					break;
				init_liste (ligne, nolig);
			}
		}

		for (i = 0; i < NB_AUTOMESS; i++)
		{
			char *ptri;
			char *ptro;

			ptri = ptro = text_rt[i];

			while (*ptri)
			{
				if ((*ptri == '$') && (*(ptri + 1) == 'W'))
				{
					*ptro = '\n';
					++ptri;
				}
				else
					*ptro = *ptri;

				++ptri;
				++ptro;
			}
			*ptro = '\0';
		}

	}
	else
	{

#ifdef ENGLISH
		fprintf (stderr, "Cannot open EPURMESS.INI file       \n");
#else
		fprintf (stderr, "Erreur ouverture fichier EPURMESS.INI\n");
#endif

		exit (1);
	}
}


void init_liste (char *ligne, int nolig)
{
	char *nom;
	int temps;
	int init = 0;
	char *mode;
	deslist *lptr = tete_desti;

	nom     = (char *) (calloc(80 , sizeof(char)));
	mode    = (char *) (calloc(80 , sizeof(char)));

	if (sscanf (ligne, "%s %s %d", mode, nom, &temps) != 3)
		erreur_arg (nolig);

	switch (toupper (*mode))
	{
	case 'F':
	case '<':
		lptr = tete_exped;
		if (lptr == NULL)
		{
			tete_exped = lptr = malloc (sizeof (deslist));
			init = 1;
		}
		break;
	case 'V':
	case '@':
		lptr = tete_route;
		if (lptr == NULL)
		{
			tete_route = lptr = malloc (sizeof (deslist));
			init = 1;
		}
		break;
	case 'T':
	case '>':
		lptr = tete_desti;
		if (lptr == NULL)
		{
			tete_desti = lptr = malloc (sizeof (deslist));
			init = 1;
		}
		break;
	default:
		erreur_arg (nolig);
		break;
	}

	if (init == 0)
	{
		while (lptr->suiv)
			lptr = lptr->suiv;
		lptr->suiv = malloc (sizeof (deslist));
		lptr = lptr->suiv;
	}
	lptr->suiv = NULL;
	strncpy (lptr->nom, nom, 6);
	lptr->nom[6] = '\0';
	lptr->delai = temps;
}

#if 0
void copy_ (char *oldfich, char *newfich)
{
	char s[256];

#ifdef __linux__
	sprintf (s, "cp %s %s > /dev/null", oldfich, newfich);
#else
	sprintf (s, "copy %s %s", oldfich, newfich);
#endif
	system (s);
}
#endif

#define TAIBUF 16384
/* static char buffer[TAIBUF]; */

int copy_ (char *oldfich, char *newfich)
{
	int retour = 1;
	int fd_orig;
	int fd_dest;
	int nb_lus;
	int ret;
	int dest_access;
	char *buffer;

	buffer       = (char *) (calloc(TAIBUF , sizeof(char)));

	if (buffer == NULL) {
		fprintf (stderr, "Not enough memory for epurmess file copy function\n");
		return (0);
	}

	if ((fd_orig = open (oldfich, O_RDONLY | O_BINARY, S_IREAD | S_IWRITE)) == EOF)
	{
		fprintf (stderr, "Cannot find %s\n", oldfich);
		free (buffer);
		return (0);
	}

	dest_access = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY;
	if ((fd_dest = open (newfich, dest_access, S_IREAD | S_IWRITE)) == EOF)
	{
		close (fd_orig);
		fprintf (stderr, "Cannot create %s\n", newfich);
		free (buffer);
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

	free (buffer);

	return (retour);
}

void newname (char *oldfich, char *newfich)
{
	rename (oldfich, newfich);
}

int is_route (char *fbbs)
{
	int i;

	for (i = 0; i < NBMASK; i++)
		if (fbbs[i])
			return (1);
	return (0);
}


void erreur_arg (int numero)
{

#ifdef ENGLISH
	printf ("Error EPURMESS.INI in line Nb %d      \n", numero);
#else
	printf ("Erreur fichier EPURMESS.INI ligne Nø%d\n", numero);
#endif

	exit (1);
}


int semaine (void)
{
	long temps = time (NULL);
	struct tm *sdate = localtime (&temps);

#if 0
	int ny = sdate->tm_yday - 1;
	int nw = sdate->tm_wday;
	int first_day;

	if (nw == 0)
		nw = 6;
	else
		--nw;

	if (ny <= nw)
		first_day = nw - ny;
	else
		first_day = 7 - ((ny - nw) % 7);

	return ((((ny + first_day) / 7) % 52) + 1);
#endif

	int ny = sdate->tm_yday;	/* Numero du jour dans l'annee */
	int nw = sdate->tm_wday;	/* Numero du jour dans la semaine */

	if (nw == 0)
		nw = 6;
	else
		--nw;					/* 0 = dimanche -> 0 = lundi */

	if (ny < nw)				/* Premiere semaine de l'annee ? */
	{
		temps -= (3600L * 24L * (ny + 1));
		sdate = localtime (&temps);
		ny = sdate->tm_yday;	/* Numero du jour de l'annee precedente */
		nw = sdate->tm_wday;	/* Numero du jour de la semaine avant */
		if (nw == 0)
			nw = 6;
		else
			--nw;				/* 0 = dimanche -> 0 = lundi */
	}
	return ((7 - nw + ny) / 7);
}


void ent_arch (FILE * fptr, bullist * pm)
{
	char *bbsv;
	long temps = pm->date;
	struct tm *sdate = gmtime (&temps);

	bbsv       = (char *) (calloc(80 , sizeof(char)));

	if (old_format)
	{
		fprintf (fptr, "Msg #%ld  Type:%c  Stat:%c  To:%s@%s  From:%s  Date:%s \n",
				 pm->numero, pm->type, pm->status, pm->desti,
				 pm->bbsv, pm->exped, strdt (pm->date));
		fprintf (fptr, "Bid: %s  Subject: %s\n", pm->bid, pm->titre);
	}
	else
	{

		if (*pm->bbsv)
			sprintf (bbsv, " @ %s", pm->bbsv);
		else
			*bbsv = '\0';

		fprintf (fptr, "S%c %s%s < %s $%s\n",
				 pm->type, pm->desti, bbsv, pm->exped, pm->bid);
		fprintf (fptr, "%s\n", pm->titre);
	}
	fprintf (fptr, "R:%02d%02d%02d/%02d%02dZ @:%s\n",
			 sdate->tm_year % 100, sdate->tm_mon + 1, sdate->tm_mday,
			 sdate->tm_hour, sdate->tm_min, mypath);
}


void message_retour (int num_txt, bullist * entete, long heure)
{
	FILE *mess;
	FILE *fptr;
	int unite;
	int i;
	int nrl;
	char *bbs;
	char *ligne;
	char *scan;

	ligne       = (char *) (calloc(BUFFSIZE , sizeof(char)));
	bbs         = (char *) (calloc(42 , sizeof(char)));
	
	/*
	   printf("\nDemande de message %d\n", num_txt);
	   printf("Message = <%s>\n", text_rt[num_txt]);
	   sleep(5);
	 */

	if (*text_rt[num_txt] == '\0')
		return;

	if (num_txt >= NB_AUTOMESS)
		return;

	nrl = nb_return_lines;
	*bbs = '\0';
	++auto_mess[num_txt];

	unite = entete->numero % 10;
	sprintf (ligne, "%sMAIL%d\\M_%06ld.MES", mail, unite, entete->numero);

	if ((mess = fopen (ligne, "rt")) == NULL)
	{
		return;
	}

	while (fgets (ligne, 256, mess))
	{
		if (strncmp (ligne, "R:", 2) != 0)
			break;
		scan = ligne;
		while ((*scan) && (*scan != '@'))
			++scan;
		++scan;
		if (*scan == ':')
			++scan;
		i = 0;
		while (isgraph (*scan))
		{
			bbs[i] = toupper (*scan);
			++scan;
			if (++i == 40)
				break;
		}
		bbs[i] = '\0';
	}

	if (*bbs == '\0')
		strcpy (bbs, callsign);

	if ((fptr = fopen (mail_in, "at")) != NULL)
	{
		fprintf (fptr, "SP %s @ %s < %s\n", entete->exped, bbs, callsign);
		fprintf (fptr, "Undelivered mail in %s\n\n", callsign);
		fprintf (fptr, "%s BBS, %s%s",
				 callsign, ctime (&heure), text_rt[num_txt]);


		fprintf (fptr, "\n\nSP %s @ %s < %s\n",
				 entete->desti, entete->bbsv, entete->exped);
		fprintf (fptr, "%s\n", entete->titre);
		while (fgets (ligne, 256, mess))
		{
			if (nrl-- <= 0)
				break;
			if (strncmpi (ligne, "/ACK", 4) != 0)
				fputs (ligne, fptr);
		}
		fprintf (fptr, "/EX\n");

	}

	fclose (fptr);
	fclose (mess);
}
