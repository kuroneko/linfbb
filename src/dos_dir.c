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

#include <serv.h>
#ifdef __LINUX__
#include <utime.h>
#endif

static int dir_time (char *);

static long parcours (char, char *);

int is_dir (char *chaine)
{
#ifdef __LINUX__
	struct stat st;
	int ret;

	if ((strlen (chaine) > 2) && (chaine[1] == ':'))
		chaine += 2;
	ret = stat (back2slash (chaine), &st);
	if (ret == -1)
		return (0);

	return (S_ISDIR (st.st_mode));
#endif
#if defined(__FBBDOS__) || defined (__WINDOWS__)
	int hand;
	struct dfree dtable;


/* Teste la racine car NOVELL ne reconnait pas C:\ comme repertoire !!! */

	if ((isalpha (chaine[0])) && (chaine[1] == ':') && (chaine[2] == '\\') && (!chaine[3]))
	{
		getdfree (toupper (chaine[0]) - '@', &dtable);
		if (dtable.df_sclus == 0xffff)
		{
			/* Disque non valide ? */
			return (0);
		}
		else
			return (1);
	}

/* Fin du rajout NOVELL */

	if (access (chaine, 0) == 0)
	{
		hand = open (chaine, O_RDONLY);		/* Repertoire ou fichier ? */
		if (hand == -1)
		{
			return (1);
		}
		else
		{
			close (hand);
			return (0);
		}
	}
	return (0);
#endif
}


void prompt_dos (void)
{
	char *scan;
	char *ptr = local_path (pvoie->dos_path);

	if ((scan = strchr (ptr, ':')) != NULL)
		ptr = scan + 1;
	var_cpy (0, ptr);
	texte (T_DOS + 0);
	maj_niv (9, 0, 0);
}


static int dir_time (char *indic)
{
	int lg = strlen (indic);
	int pos;
	int c[3];

	for (pos = 0; pos < 3; c[pos++] = '\0')
		;

	pos = 0;

	while (lg--)
	{
		if (isalpha (indic[lg]))
			c[pos++] = (int) (indic[lg] - 'A');
		if (pos == 3)
			break;
	}

	return (c[0] | (c[1] << 5) | (c[2] << 10));
}


void wr_dir (char *fichier, char *indic)
{
#if defined(__FBBDOS__) || defined(__WINDOWS__)
	int fd;
	int dt = dir_time (indic);
	long temps = time (NULL);
	struct tm *sdate;
	struct ftime dirtime;

	sdate = localtime (&temps);
	dirtime.ft_year = sdate->tm_year %100;
	dirtime.ft_day = sdate->tm_mday;
	dirtime.ft_month = sdate->tm_mon + 1;
	dirtime.ft_hour = dt >> 11;
	dirtime.ft_min = (dt >> 5) & 0x3f;
	dirtime.ft_tsec = dt & 0x1f;

	if ((fd = open (fichier, O_RDONLY)) != -1)
	{
		setftime (fd, &dirtime);
		close (fd);
	}
#endif
#ifdef __LINUX__
	struct utimbuf buf;
	long temps = time (NULL);
	struct tm *sdate;

/*	int dt = dir_time (indic);
	int heure = dt >> 11;
	int minute = (dt >> 5) & 0x3f;
	int seconde = (dt & 0x1f) << 1;
*/
	sdate = localtime (&temps);
/*	sdate->tm_hour = heure;
	sdate->tm_min = minute;
	sdate->tm_sec = seconde;
*/
	buf.actime = buf.modtime = mktime (sdate);
	if (utime (back2slash (fichier), &buf) != 0)
		perror ("utime");
#endif
}


int aut_dir (char *fichier, char *indic)
{
	if (droits (SUPFIC))
	{
		return (1);
	}

	else
	{
#if defined(__FBBDOS__) || defined(__WINDOWS__)
		union
		{
			struct ftime dirtime;
			struct
			{
				int time;
				int date;
			}
			dtime;
		}
		utime;
		int fd;

		if ((fd = open (fichier, O_RDONLY)) == -1)
			return (1);
		getftime (fd, &(utime.dirtime));
		close (fd);
		return (utime.dtime.time == dir_time (indic));
#endif
#ifdef __LINUX__
		int fd;
		struct stat buf;
		time_t temps;
		struct tm *sdate;
		int dt;

		if ((fd = open (fichier, O_RDONLY)) == -1)
			return (1);
		fstat (fd, &buf);
		temps = buf.st_mtime;
		close (fd);
		sdate = localtime (&temps);
		dt = (sdate->tm_hour << 11) + (sdate->tm_min << 5) + (sdate->tm_sec >> 1);
		return (dt == dir_time (indic));
#endif
	}
}

static int protected_dir(char *file)
{
	int protected = 0;
	char nomfich[256];
	FILE *fptr;
	char *ptr, *scan;
	char ligne[81];

#ifdef __FBBDOS__
	if ((fptr = fopen (c_disque ("prot_d.sys"), "r")) == NULL)
#endif
#ifdef __WINDOWS__
	if ((fptr = fopen (c_disque ("prot_w.sys"), "r")) == NULL)
#endif
#ifdef __LINUX__
	if ((fptr = fopen (c_disque ("prot_l.sys"), "r")) == NULL)
#endif
	{
		fptr = fopen (c_disque ("protect.sys"), "r");
	}

	if (fptr)
	{
		while (fgets (ligne, 80, fptr))
		{
			ptr = scan = ligne;
			while ((*scan) && (!ISGRAPH (*scan)))
				++scan;
			while (ISGRAPH (*scan))
				*ptr++ = *scan++;
			*ptr = '\0';

			if (*ligne == '#' || *ligne == '\0')
				continue;

			strcpy (nomfich, tot_path (file, pvoie->dos_path));
#ifdef __LINUX__
			ptr = strrchr (nomfich, '/');
			if (ptr == nomfich)
				strcpy (nomfich, "/");
#else
			ptr = strrchr (nomfich, '\\');
			if (ptr == nomfich)
				strcpy (nomfich, "\\");
			else
				*ptr = '\0';
#endif

#ifdef __LINUX__
			ptr = ligne;
			scan = nomfich;
			if (ptr[1] != ':' && nomfich[1] == ':')
				scan += 2;
#else
			ptr = slash2back(ligne);
			scan = nomfich;
#endif

			if (strncmpi (ptr, scan, strlen(ptr)) == 0)
			{
				protected = 1;
				break;
			}
		}
		ferme (fptr, 77);
	}

	return protected;
}

int aut_ecr (char *fichier, int check_dir)
{
	int supp = TRUE;
	char *ptr, *scan;
	char nomfich[256];

	if (droits (ACCESDOS) && droits (SUPFIC) && (*fichier == '+'))
	{
		if (*fichier == '+')
		{
			/* Deletes the '+' character */
			ptr = scan = fichier;
			++scan;
			while ((*ptr++ = *scan++) != '\0')
				;
		}
	}
	else
	{
		if (check_dir)
			supp = !protected_dir(fichier);
		if ((supp) && (!droits (ACCESDOS)))
		{
			strcpy (nomfich, tot_path (fichier, pvoie->dos_path));
			supp = aut_dir (nomfich, pvoie->sta.indicatif.call);
		}
	}

	if (!supp)
	{
		strcpy (pvoie->appendf, fichier);
		texte (T_ERR + 23);
	}
	return (supp);
}


/*
   static int   exist(char*file)
   {
   return (access(local_path(tot_path(file, pvoie->dos_path)), 0) == 0) ;
   }
 */

void remove_dir (void)
{
	char *ptr;

	get_nextparam();
	if ((ptr = get_nextparam()) == NULL)
	{
		texte (T_ERR + 25);
	}
	else
	{
		if (tst_point (ptr))
		{
			if (aut_ecr (ch_slash (ptr), 1))
			{
				var_cpy (0, ptr);
#ifdef __LINUX__
				if (fbb_rmdir (tot_path (ptr, pvoie->dos_path)) == 0)
#else
				if (rmdir (tot_path (ptr, pvoie->dos_path)) == 0)
#endif
					texte (T_DOS + 1);
				else
					texte (T_ERR + 24);
			}
		}
	}
	prompt_dos ();
}


void make_dir (void)
{
	char *ptr, *path;

	get_nextparam();
	if ((ptr = get_nextparam()) == NULL)
	{
		texte (T_ERR + 25);
	}
	else
	{
#ifdef __LINUX__
		path = tot_path (ptr, pvoie->dos_path);
		
		if (fbb_mkdir (path, 0777) != 0)
		{
			var_cpy (0, ptr);
			texte (T_ERR + 26);
		}
		else
		{
			wr_dir (path, pvoie->sta.indicatif.call);
		}
#endif
#if defined(__FBBDOS__) || defined(__WINDOWS__)
		ch_slash (ptr);
		if (mkdir (tot_path (ptr, pvoie->dos_path)) != 0)
		{
			var_cpy (0, ptr);
			texte (T_ERR + 26);
		}
#endif
	}
	prompt_dos ();
}


static void teste_file_name (char *ptr)
{
/*
	int ok;
	int lg = 0;
	int max = 8;
	char *ptro = ptr;

	while (*ptr)
	{

		ok = 1;
		if (*ptr == '.')
		{
			lg = 0;
			max = 3;
		}
		else if (*ptr == '\\')
		{
			lg = 0;
			max = 8;
		}
		else if (++lg > max)
		{
			ok = 0;
		}

		if (ok)
			*ptro++ = *ptr;
		++ptr;
	}
	*ptro = '\0';
*/
}

void change_dir (void)
{
	int nb;
	char local_dir[256], *ptr, *scan, *cur_dir;

	strcpy (local_dir, pvoie->dos_path);
	get_nextparam();
	if ((ptr = get_nextparam()) == NULL)
	{
		nb = strlen (local_dir);
		if (nb > 1)
		{
			--nb;
			local_dir[nb] = '\0';
		}
		outln (local_dir, nb);
	}
	else
	{
#ifdef __LINUX__
		teste_file_name (ptr);
		if (*ptr == '/')
#else
		ch_slash (ptr);
		teste_file_name (ptr);
		if (*ptr == '\\')
#endif
		{
			if (tst_point (ptr))
			{
				strcpy (local_dir, ptr);
				if (strlen (local_dir) > 1)
#ifdef __LINUX__
					strcat (local_dir, "/");
#else
					strcat (local_dir, "\\");
#endif
			}
		}
		else
		{
#ifdef __LINUX__
			if ((scan = strtok (ptr, "\n")) != NULL)
#else
			if ((scan = strtok (ptr, "\\\r")) != NULL)
#endif
			{
				do
				{
					if (strcmp (scan, ".") == 0)
						continue;
					if (strncmp (scan, "..", 2) == 0)
					{
#ifdef __LINUX__
						cur_dir = strrchr (local_dir, '\\');
#else
						cur_dir = strrchr (local_dir, "\\");
#endif
						if (cur_dir != local_dir)
						{
							do
							{
								*cur_dir-- = '\0';
							}
#ifdef __LINUX__
							while (*cur_dir != '\\');
#else
							while (*cur_dir != "\\");
#endif
						}
					}
					else if (*scan != '~')
					{
						strcat (local_dir, scan);
#ifdef __LINUX__
						strcat (local_dir, "/");
#else
						strcat (local_dir, "\\");
#endif
					}
				}
#ifdef __LINUX__
				while ((scan = strtok (NULL, "\n")) != NULL);
#else
				while ((scan = strtok (NULL, "\\\r")) != NULL);
#endif
			}
		}
		if (strlen (local_dir) > 255)
			texte (T_ERR + 28);
		else
		{
			char ldir[256];
			char *ptr;
			strcpy(ldir, tot_path ("\0", local_dir));
			ptr = long_filename(NULL, ldir);
			if (is_dir (local_path (ptr)) > 0)
				strcpy (pvoie->dos_path, ptr + strlen(ptr) - strlen(local_dir));
			else
				texte (T_ERR + 29);
		}
	}
	prompt_dos ();
}


char *dir_date (int date)
{
	int jour, mois, annee;
	static char ch_date[11];
	jour = date & 0x1f;
	mois = (date >> 5) & 0x0f;
	annee = ((date >> 9) & 0x3f);
	sprintf (ch_date, "%02d-%02d-%02d", jour, mois, annee % 100);
	return (ch_date);
}

int dir_suite (char *masque)
{
	int n;
	char chaine[257];
	char temp[257];

	if (masque)
	{

		strcpy (temp, tot_path (ch_slash (masque), pvoie->dos_path));

		n = strlen (temp);
		if ((n > 3) && (temp[n - 1] == '\\'))
			temp[n - 1] = '\0';
		if (is_dir (temp))
		{
			if ((strlen (masque) == 3) && (masque[2] == '\\'))
				masque[2] = '\0';
#ifdef __LINUX__
			sprintf (chaine, "%s/*", masque);
#else
			sprintf (chaine, "%s/*.*", masque);
#endif
			masque = chaine;
		}

		if (findfirst (tot_path (ch_slash (masque), pvoie->dos_path), &(pvoie->dirblk), FA_DIREC))
		{
			texte (T_DOS + 2);
			return (FALSE);
		}
		if (*pvoie->dirblk.ff_name == '.')
		{
			findnext (&(pvoie->dirblk));
			if (findnext (&(pvoie->dirblk)))
			{
				texte (T_DOS + 2);
				return (FALSE);
			}
		}
		var_cpy (3, pvoie->dirblk.ff_name);
		var_cpy (2, dir_date (pvoie->dirblk.ff_fdate));
		*varx[0] = '\0';
		*varx[4] = *varx[5] = *varx[6] = *varx[7] = '\0';
		if ((pvoie->dirblk.ff_attrib & FA_DIREC) != 0)
		{
			var_cpy (1, "<DIR>  ");
		}
		else
		{
			long size = pvoie->dirblk.ff_fsize;
			if (size > (99999 * 1024))
				sprintf (varx[1], "%5ld M", size / (1024 * 1024));
			else if (size > 999999)
				sprintf (varx[1], "%5ld K", size / 1024);
			else
				sprintf (varx[1], "%7ld", size);
		}

		/* if (findnext (&(pvoie->dirblk)))
		{
			*varx[4] = *varx[5] = *varx[6] = *varx[7] = '\0';
			texte (T_DOS + 3);
			return (FALSE);
		}

		sprintf (varx[4], "%-13s", pvoie->dirblk.ff_name);
		var_cpy (6, dir_date (pvoie->dirblk.ff_fdate));
		*varx[7] = '\0';
		if ((pvoie->dirblk.ff_attrib & FA_DIREC) != 0)
		{
			var_cpy (5, "<DIR>  ");
		}
		else
		{
			sprintf (varx[5], "%7ld", pvoie->dirblk.ff_fsize);
		}*/
		texte (T_DOS + 3);
	}

	while (1)
	{
		if (findnext (&(pvoie->dirblk)))
			return (FALSE);
/*		sprintf (varx[0], "%-13s", pvoie->dirblk.ff_name); */
		var_cpy (3, pvoie->dirblk.ff_name);
		var_cpy (2, dir_date (pvoie->dirblk.ff_fdate));
		*varx[0] = '\0';
		*varx[4] = *varx[5] = *varx[6] = *varx[7] = '\0';
		if ((pvoie->dirblk.ff_attrib & FA_DIREC) != 0)
		{
			var_cpy (1, "<DIR>  ");
		}
		else
		{
			long size = pvoie->dirblk.ff_fsize;
			if (size > (99999 * 1024))
				sprintf (varx[1], "%5ld M", size / (1024 * 1024));
			else if (size > 999999)
				sprintf (varx[1], "%5ld K", size / 1024);
			else
				sprintf (varx[1], "%7ld", size);
		}

		/* if (findnext (&(pvoie->dirblk)))
		{
			*varx[4] = *varx[5] = *varx[6] = *varx[7] = '\0';
			texte (T_DOS + 3);
			return (FALSE);
		}

		sprintf (varx[4], "%-13s", pvoie->dirblk.ff_name);
		var_cpy (6, dir_date (pvoie->dirblk.ff_fdate));
		*varx[7] = '\0';
		if ((pvoie->dirblk.ff_attrib & FA_DIREC) != 0)
		{
			var_cpy (5, "<DIR>  ");
		}
		else
		{
			sprintf (varx[5], "%7ld", pvoie->dirblk.ff_fsize);
		}*/
		texte (T_DOS + 3);
	}
	/*   return(TRUE) ; */
}


void retour_dir (char vdisk)
{
	int disk;

	if (vdisk == 8)
		disk = (pvoie->finf.priv[1] == ':') ? pvoie->finf.priv[0] - '@' : getdisk () + 1;
	else if ((voiecur == CONSOLE) || (pvoie->niv1 == N_YAPP))
		disk = vdisk + 1;
	else
#ifdef __LINUX__
		disk = vdisk + 1;
#else
		disk = PATH[vdisk][0] - '@';
#endif
	ultoa (free_disk (disk) * 1024UL, varx[0], 10);
	texte (T_DOS + 11);
	retour_dos ();
}

void dir (void)
{
	char vdisk;
	char *ptr;

#ifdef __LINUX__
	char temp[] = "X:*";

#else
	char temp[] = "X:*.*";

#endif

	get_nextparam();
	pvoie->noenr_menu = 0L;
	ptr = get_nextparam();

	if (ptr == NULL)
	{
		temp[0] = (pvoie->vdisk == 8) ? 'P' : pvoie->vdisk + 'A';
		ptr = temp;
	}

	vdisk = pvoie->vdisk;

	if ((*(ptr + 1) == ':') && (*(ptr + 2) == '\0'))
	{
		temp[0] = *ptr;
		vdisk = *ptr - 'A';
		if (vdisk == 15)
			vdisk = 8;
		ptr = temp;
	}

	if (!tst_point (ptr))
		retour_dos ();

	else if (dir_suite (ptr))
	{
		texte (T_TRT + 11);
		maj_niv (9, 1, 1);
	}
	else
		retour_dir (vdisk);
}


void list (void)
{
	char vdisk;
	char *ptr;

#ifdef __LINUX__
	char temp[] = "X:*";

#else
	char temp[] = "X:*.*";

#endif

	get_nextparam();
	ptr = get_nextparam();
	if (ptr == NULL)
	{
		temp[0] = (pvoie->vdisk == 8) ? 'P' : pvoie->vdisk + 'A';
		ptr = temp;
	}

	vdisk = pvoie->vdisk;

	if ((*(ptr + 1) == ':') && (*(ptr + 2) == '\0'))
	{
		temp[0] = *ptr;
		vdisk = *ptr - 'A';
		if (vdisk == 13)
			vdisk = 8;
		ptr = temp;
	}

#if 0
	if ((ptr = strtok (NULL, " \r")) == NULL)
	{
		ptr = temp;
	}
	liste_label ();
	if (!tst_point (ptr))
		retour_dos ();

	if (dir_yapp (ptr))
	{
		texte (T_TRT + 11);
		ch_niv3 (1);
	}
	else
	{
		pvoie->noenr_menu = 0L;
		libere_label (voiecur);
		retour_dir ();
	}
#endif
	if (tst_point (ptr))
	{
		dir_yapp (ptr);
		retour_dir (vdisk);
	}
	else
		retour_dos ();
}

static long parcours (char vdisk, char *path)
{
	struct ffblk ffblk;
	long total = 0L;
	long local = 0L;
	char rech[80];
	char temp[128];
	int done = 1;
	int pos;

	strcpy (rech, path);
#ifdef __LINUX__
	strcat (rech, "*");
#else
	strcat (rech, "*.*");
#endif

	if (voiecur == CONSOLE)
		pos = 0;
	else
	{
		char *ptr = PATH[(int)vdisk];
		pos = strlen (ptr) - 1;
		if (ptr[1] == ':')
			pos -= 2;
	}

	done = findfirst (rech, &ffblk, FA_DIREC);
	while (!done)
	{
		if (*ffblk.ff_name != '.')
		{
#ifdef __LINUX__
			if ((ffblk.ff_attrib & FA_DIREC) && ((ffblk.ff_attrib & FA_LINK) == 0))
#else
			if (ffblk.ff_attrib & FA_DIREC)
#endif
			{
				strcpy (temp, path);
				strcat (temp, ffblk.ff_name);
				strcat (temp, "\\");
				if (strncmp (temp + 1, ":\\PROC\\", 7) != 0)
				{
					total += parcours (vdisk, temp);
				}
			}
			else
			{
				local += ffblk.ff_fsize;
			}
		}
		done = findnext (&ffblk);
	}
	total += local;
	sprintf (temp, "%8ld KB  %8ld KB %s", (total + 1023)/1024, (local + 1023)/1024, path + pos);
	outln (temp, strlen (temp));

	return (total);
}

void du (void)
{
	char vdisk;
	char *ptr;
	char temp[] = "X:";
	char path[256];

	get_nextparam();
	ptr = get_nextparam();
	if (ptr == NULL)
	{
		temp[0] = (pvoie->vdisk == 8) ? 'P' : pvoie->vdisk + 'A';
		ptr = temp;
	}

	vdisk = pvoie->vdisk;

	if ((*(ptr + 1) == ':') && (*(ptr + 2) == '\0'))
	{
		temp[0] = *ptr;
		vdisk = *ptr - 'A';
		if (vdisk == 13)
			vdisk = 8;
		ptr = temp;
	}

	if (tst_point (ptr))
	{
		strcpy (path, tot_path (ch_slash (ptr), pvoie->dos_path));
		parcours (vdisk, path);
		retour_dir (vdisk);
	}
	else
		retour_dos ();
}
