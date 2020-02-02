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

#include <serv.h>

#define	CTRL_Z		'\032'

static int out_disk (FILE *, char *);

static int mess_export (FILE * fptr, int voie, long temps, long no_message)
{
	char lastchar = '\0';
	char type;
	char s[255];
	char ret[2];
	char header[160];
	char via[80];
	FILE *f_mptr;
	char buffer[300];

	ret[0] = 13;
	ret[1] = 0;


	if (*svoie[voie]->entmes.bbsv)
		sprintf (via, "@ %s ", svoie[voie]->entmes.bbsv);
	else
		*via = '\0';

	type = svoie[voie]->entmes.type;
	if (type == 'A')
		type = 'P';

	sprintf (s, "S%c %s %s< %s",
	 type, svoie[voie]->entmes.desti, via, svoie[voie]->entmes.exped);
	if (!out_disk (fptr, s))
		return (0);
	if (*(svoie[voie]->entmes.bid))
	{
		sprintf (s, " $%s", svoie[voie]->entmes.bid);
		if (!out_disk (fptr, s))
			return (0);
	}
	if (!out_disk (fptr, ret))
		return (0);
	if (!out_disk (fptr, svoie[voie]->entmes.titre))
		return (0);
	if (!out_disk (fptr, ret))
		return (0);

	deb_io ();
	make_header (&(svoie[voie]->entmes), header);
	entete_mess_fwd (&(svoie[voie]->entmes), header);
	if (!out_disk (fptr, msg_header))
		return (0);
	fin_io ();

	if ((f_mptr = ouvre_mess (O_TEXT, svoie[voie]->entmes.numero, svoie[voie]->entmes.status)) != NULL)
	{
		while (1)
		{
			if (fgets (buffer, 256, f_mptr) == NULL)
				break;
			lastchar = buffer[strlen (buffer) - 1];
			if (!out_disk (fptr, lf_to_cr (buffer)))
			{
				ferme (f_mptr, 31);
				return (0);
			}
		}
		ferme (f_mptr, 31);
	}
	else
	{
		dde_warning (W_ASC);
		sprintf (s, "\rMessage file %s missing in %s\r", pvoie->sr_fic, mycall);
		if (!out_disk (fptr, s))
			return (0);
	}
	if ((lastchar != '\n') && (!out_disk (fptr, "\r")))
		return (0);
	if (!out_disk (fptr, "/EX\r"))
		return (0);
	tst_warning (ptmes);
	return (1);
}



static int out_disk (FILE * fptr, char *texte)
{
	int nb = 0;
	int retour;
	char buffer[300];
	char *optr = buffer;

	aff_bas (voiecur, W_SNDT, texte, strlen (texte));
	while (*texte)
	{
		if (*texte == '\r')
		{
			*optr = '\r';
			++optr;
			if (++nb == 300)
				break;
			*optr = '\n';
		}
		else if (*texte != '\n')
			*optr = *texte;
		++optr;
		++texte;
		if (++nb == 300)
			break;
	}
	if (nb == 0)
		return (1);
	deb_io ();
	retour = ((int) fwrite (buffer, nb, 1, fptr));
	fin_io ();
	return (retour > 0);
}


int export_message (char *nom_fich)
{
	int i;
	int fd;
	FILE *fptr;
	char temp[200];

	aff_etat ('J');
	aff_header (voiecur);
	aff_forward ();
	pvoie->lignes = -1;
	pvoie->finf.lang = langue[0]->numlang;

	for (i = 0; i < 2; i++)
	{
		/* Checks for the lock file and creates it */
		fd = open (lfile (nom_fich), O_EXCL | O_CREAT, S_IREAD | S_IWRITE);
		if (fd == -1)
		{
#ifdef __WIN32__
			long tt;
			WIN32_FIND_DATA ff;
			SYSTEMTIME t;
			FILETIME ft;

			if (errno != EEXIST)
			{
				sprintf (temp, "*** error : Cannot create lock file %s\r", lfile (nom_fich));
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				break;
			}

			if (FindFirstFile ((char *) lfile (nom_fich), &ff) != (HANDLE) - 1)
			{
				GetSystemTime (&t);
				SystemTimeToFileTime (&t, &ft);

				/* Add one hour to File time (approximative) */
				ff.ftLastWriteTime.dwHighDateTime += 9;
				if (CompareFileTime (&ft, &ff.ftLastWriteTime) > 0L)
				{
					/* If more than 1 hour, delete the lock file if possible */
					unlink (lfile (nom_fich));
					sprintf (temp, "*** file %s was locked since more than 1 hour, lock deleted !!\r",
							 nom_fich);
					aff_bas (voiecur, W_SNDT, temp, strlen (temp));
				}
				else
				{
					sprintf (temp, "*** file %s locked !!\r", nom_fich);
					aff_bas (voiecur, W_SNDT, temp, strlen (temp));
					break;
				}
			}
#else
			/* Checks the date of the lock file */
			struct stat statbuf;
			long t = time (NULL);

			if (errno != EEXIST)
			{
				sprintf (temp, "*** error : Cannot create lock file %s\r", lfile (nom_fich));
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				break;
			}

			if ((stat (lfile (nom_fich), &statbuf) == 0) && ((t - statbuf.st_ctime) > 3600L))
			{
				/* If more than 1 hour, delete the lock file if possible */
				unlink (lfile (nom_fich));
				sprintf (temp, "*** file %s was locked from more than 1 hour (%ld), lock deleted (%ld)!!\r",
						 nom_fich, statbuf.st_ctime, t);
				aff_bas (voiecur, W_SNDT, temp, strlen (temp));
			}
			else
			{
				sprintf (temp, "*** file %s locked !!\r", nom_fich);
				aff_bas (voiecur, W_SNDT, temp, strlen (temp));
				break;
			}
#endif
		}
		else
			break;
	}

	if (fd == -1)
	{
		aff_bas (voiecur, W_SNDT, "*** done\r", 9);
		inexport = 0;
		deconnexion (voiecur, 1);
		init_etat ();
		status (voiecur);
		return (0);
	}

	close (fd);

	if ((fptr = fappend (nom_fich, "b")) == NULL)
	{
#ifdef ENGLISH
		sprintf (temp, "  File error %s  ", nom_fich);
#else
		sprintf (temp, "Erreur fichier %s", nom_fich);
#endif
		win_message (5, temp);
		aff_bas (voiecur, W_RCVT, "*** disk error !!\r", 18);
		aff_bas (voiecur, W_SNDT, "*** done\r", 9);
		inexport = 0;
		deconnexion (voiecur, 1);
		init_etat ();
		status (voiecur);
		unlink (lfile (nom_fich));
		return (0);
	}
	if (mess_suiv (mail_ch))
	{
		if (mess_export (fptr, mail_ch, svoie[mail_ch]->entmes.date, svoie[mail_ch]->entmes.numero))
		{
			fin_envoi_fwd (mail_ch);
			aff_bas (voiecur, W_RCVT, "Export>\r", 8);
		}
		else
		{
			fclose (fptr);
			unlink (lfile (nom_fich));
#ifdef ENGLISH
			sprintf (temp, "  File error %s  ", nom_fich);
#else
			sprintf (temp, "Erreur fichier %s", nom_fich);
#endif
			aff_bas (voiecur, W_RCVT, "*** disk error !!\r", 18);
			aff_bas (voiecur, W_SNDT, "*** done\r", 9);
			inexport = 0;
			deconnexion (voiecur, 1);
			/* ferme(fptr, 7) ; */
#if defined(__WINDOWS__) || defined(__linux__)
			window_disconnect (voiecur);
#endif
			init_etat ();
			status (voiecur);
			return (0);
		}
	}
	else
	{
		aff_bas (voiecur, W_SNDT, "*** done\r", 9);
		inexport = 0;
		deconnexion (voiecur, 1);
		init_etat ();
	}
	ferme (fptr, 7);
	unlink (lfile (nom_fich));
	status (voiecur);
	return (1);
}
