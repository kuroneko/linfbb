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
 * Initialisation du buffer forward.
 */

#include <serv.h>

#define NBCAS 3

/*static int niveau;*/
static int etat_bloc;
static int nbif[NBCAS];

static int erreur_cron (int);
static int fwd_commande (char *);
static int tst_ligne (int);

static char *analyse (char *, int *, int *);

static void bip_select (char *);
static void error_file (char *);
static void gate_select (char *);
static void init_buf_fwd_ems (void);
static void list_select (char *);
static void port_select (char *);
static void reset_fwd_pointeurs (void);
static void talk_select (char *);
static void yapp_select (char *);

#define MAX_NIVEAU 	4
#define REJECT_FILE	"REJECT.SYS"

typedef struct
{
	FILE *fp;
	char name[80];
	int nolig;
}
File;

static File file[MAX_NIVEAU];

static void error_file (char *str)
{
#ifdef ENGLISH
	fprintf (stderr, "*** FILE ERROR *** in file %s line %d  : %s\n", file[niveau].name, file[niveau].nolig, str);
#else
	fprintf (stderr, "*** ERREUR FICHIER *** dans fichier %s ligne %d : %s\n", file[niveau].name, file[niveau].nolig, str);
#endif
}

#ifdef __FBBDOS__
static char *forward_sys[2] =
{"forw_d.sys", "forward.sys"};

#endif

#ifdef __WINDOWS__
static char *forward_sys[2] =
{"forw_w.sys", "forward.sys"};

#endif

#ifdef __linux__
static char *forward_sys[2] =
{"forw_l.sys", "forward.sys"};

#endif


void test_buf_fwd (void)
{
	int nb_include = 0;
	int c;
	int scane = 0;
	int choix;
	char com_buf[80];
	struct stat st;

	niveau = 0;
	memset (file, 0, sizeof (File) * MAX_NIVEAU);
	file[niveau].nolig = 0;

	choix = 0;
	file[niveau].fp = fopen (c_disque (forward_sys[choix]), "rt");
	if (file[niveau].fp == NULL)
		choix = 1;
	file[niveau].fp = fopen (c_disque (forward_sys[choix]), "rt");
	strcpy (file[niveau].name, forward_sys[choix]);

	reset_fwd_pointeurs ();
	etat_bloc = nbif[0] = nbif[1] = 0;

	if (file[niveau].fp)
	{
		fstat (fileno (file[niveau].fp), &st);
		if (time_include[nb_include++] != st.st_mtime)
			++scane;

		do
		{
			while (fgets (com_buf, 80, file[niveau].fp))
			{
				++file[niveau].nolig;
				sup_ln (com_buf);
				c = fwd_commande (com_buf);
				if (c != '<')
					continue;
				if (nb_include >= include_size)
				{
					++scane;
					break;
				}
				else
				{
					if (++niveau >= MAX_NIVEAU)
					{
						--niveau;
						break;
					}
					file[niveau].nolig = 0;
					strcpy (file[niveau].name, com_buf);
					if ((file[niveau].fp = fopen (c_disque (file[niveau].name), "rt")) != NULL)
					{
						fstat (fileno (file[niveau].fp), &st);
						if (time_include[nb_include++] != st.st_mtime)
							++scane;
					}
					else
					{
						char str[256];

						sprintf (str, "Cannot find include file %s", file[niveau].name);
						error_file (str);
						--niveau;
						break;
					}
				}
			}
			fclose (file[niveau].fp);
		}
		while (niveau--);


	}

	if (scane == 0)
		return;

#ifdef __WINDOWS__
	scan_fwd (1);
#else
	p_forward = 1;
	maj_options ();
	init_buf_fwd ();
	aff_nbsta ();
#endif
}


static int tst_ligne (int c)
{
	int retour = 0;
	int c_save = c;
	char str[256];

#if 1
	static unsigned char etat[NBCAS][29] =
	{
		{2, 0, 0, 1, 5, 0, 0, 0, 3, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 4},	/* Hors bloc */
		{0, 1, 1, 1, 5, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 7, 4},	/* Dans bloc */
		{0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}		/* Not (!)   */
	};

#else
	static unsigned char etat[NBCAS][29] =
	{
		{2, 0, 0, 1, 5, 0, 0, 0, 3, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 4},
		{0, 0, 1, 1, 5, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 6, 7, 4},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

#endif
	if (c == '-')
		c = 26;
	else if (c == '!')
		c = 27;
	else if (c == '@')
		c = 28;
	else
	{
		if (islower (c))
			c -= 'a';
		else
			c -= 'A';
	}

	switch (etat[etat_bloc][c])
	{

	case 0:					/* Erreur */
		sprintf (str, "Unknown command %c", c_save);
		error_file (str);
		retour = 1;
		break;

	case 1:					/* Commande OK */
		if (etat_bloc == 2)
			etat_bloc = 1;
		break;

	case 2:					/* Debut de bloc */
		etat_bloc = 1;
		nbif[1] = 0;
		break;

	case 3:					/* IF */
		nbif[(etat_bloc > 0) ? 1 : 0]++;
		break;

	case 4:					/* ELSE */
		if (nbif[(etat_bloc > 0) ? 1 : 0] == 0)
		{
			error_file ("else without if");
			retour = 1;
		}
		break;

	case 5:					/* ENDIF */
		if (nbif[(etat_bloc > 0) ? 1 : 0]-- == 0)
		{
			error_file ("endif without if");
			retour = 1;
		}
		break;

	case 6:					/* Fin de bloc */
		if ((etat_bloc == 0) || (nbif[1] != 0))
		{
			error_file ("end of a non-existing block");
			retour = 1;
		}
		etat_bloc = 0;
		break;

	case 7:					/* NOT */
		etat_bloc = 2;
		break;
	}
	return retour;
}

void end_fwd (void)
{
	if (time_include)
	{
		m_libere (time_include, include_size * sizeof (long));

		time_include = NULL;
	}
}

static void init_buf_fwd_ems (void)
{
	int nb_include;
	int c;
	int choix;
	char com_buf[80];
	char ligne[80];
	char *ptr, *fwd_ptr;
	unsigned nb_car = 1;
	struct stat st;

	nb_include = 1;
	reset_fwd_pointeurs ();
	seek_exms_string (FORWARD, 0L);
	etat_bloc = nbif[0] = nbif[1] = 0;
	niveau = 0;
	memset (file, 0, sizeof (File) * MAX_NIVEAU);

	file[niveau].nolig = 0;
	choix = 0;
	file[niveau].fp = fopen (c_disque (forward_sys[choix]), "rt");
	if (file[niveau].fp == NULL)
	{
		choix = 1;
		file[niveau].fp = fopen (c_disque (forward_sys[choix]), "rt");
	}
	strcpy (file[niveau].name, forward_sys[choix]);

	if (file[niveau].fp)
	{

		do
		{
			while (fgets (com_buf, 80, file[niveau].fp))
			{
				++file[niveau].nolig;
				sup_ln (com_buf);
				c = fwd_commande (com_buf);
				if ((c == '*') || (c == '#') || (c == '\0'))
					continue;
				if (c == '<')
				{
					++nb_include;
					if (++niveau >= MAX_NIVEAU)
					{
						--niveau;
						error_file ("Too much include levels");
						for (niveau = 1; niveau < MAX_NIVEAU; ++niveau)
						{
							if (file[niveau].fp)
								fclose (file[niveau].fp);
						}
						nb_car = 1;
						niveau = 0;
						break;
					}
					file[niveau].nolig = 0;
					strcpy (file[niveau].name, com_buf);
					if ((file[niveau].fp = fopen (c_disque (file[niveau].name), "rt")) == NULL)
					{
						char str[256];

						sprintf (str, "Cannot find include file %s", file[niveau].name);
						error_file (str);
						for (niveau = 1; niveau < MAX_NIVEAU; ++niveau)
						{
							if (file[niveau].fp)
								fclose (file[niveau].fp);
						}
						nb_car = 1;
						niveau = 0;
						break;
					}
					continue;
				}
				if (tst_ligne (c))
				{
					nb_car = 1;
					break;
				}
				if (c == '!')
				{
					c = fwd_commande (com_buf);
					if (tst_ligne (c))
					{
						nb_car = 1;
						break;
					}
					++nb_car;
				}
				nb_car += (strlen (com_buf) + 2);
			}
			fclose (file[niveau].fp);
		}
		while (niveau--);

		if (nbif[0] != 0)
		{
			error_file ("endif missing");
			nb_car = 1;
		}

	}

	if (nb_include != include_size)
	{
		if (time_include)
			m_libere (time_include, include_size * sizeof (long));

		include_size = nb_include;
		time_include = (long *) m_alloue (include_size * sizeof (long));
	}

	if (nb_car > 1)
	{
		niveau = 0;
		nb_include = 0;

		file[niveau].nolig = 0;

		file[niveau].fp = fopen (c_disque (forward_sys[choix]), "rt");
		strcpy (file[niveau].name, forward_sys[choix]);

		if (file[niveau].fp)
		{
			fstat (fileno (file[niveau].fp), &st);
			time_include[nb_include++] = st.st_mtime;
			do
			{
				while (fgets (com_buf, 80, file[niveau].fp))
				{
					fwd_ptr = ligne;
					sup_ln (com_buf);
					c = fwd_commande (com_buf);
					if ((c == '*') || (c == '#') || (c == '\0'))
						continue;
#if defined(__WINDOWS__) || defined(__linux__)
					if (c == 'A')
					{
						InitText (com_buf);
					}
#endif
					if (c == '<')
					{
						++niveau;
						file[niveau].nolig = 0;
						strcpy (file[niveau].name, com_buf);
						file[niveau].fp = fopen (c_disque (file[niveau].name), "rt");
						if (file[niveau].fp == NULL)
						{
							for (niveau = 1; niveau < MAX_NIVEAU; ++niveau)
							{
								if (file[niveau].fp)
									fclose (file[niveau].fp);
							}
							nb_car = 1;
							niveau = 0;
							break;
						}
						fstat (fileno (file[niveau].fp), &st);
						time_include[nb_include++] = st.st_mtime;
						continue;
					}
					*fwd_ptr++ = c;
					if (c == '!')
					{
						c = fwd_commande (com_buf);
						*fwd_ptr++ = c;
					}
					ptr = com_buf;
					while ((*fwd_ptr++ = *ptr++) != '\0');
					write_exms_string (FORWARD, ligne);
				}
				fclose (file[niveau].fp);
			}
			while (niveau--);
		}

	}
	ligne[0] = '\032';
	ligne[1] = '\0';
	write_exms_string (FORWARD, ligne);

	seek_exms_string (FORWARD, 0L);
}

void init_buf_fwd (void)
{
	int nb_include;
	int c;
	int choix;
	char com_buf[80];
	char *ptr, *fwd_ptr;
	unsigned nb_car = 1;
	struct stat st;

	if (EMS_FWD_OK ())
	{
		init_buf_fwd_ems ();
		return;
	}

	nb_include = 1;
	reset_fwd_pointeurs ();
	etat_bloc = nbif[0] = nbif[1] = 0;
	niveau = 0;
	memset (file, 0, sizeof (File) * MAX_NIVEAU);

	file[niveau].nolig = 0;
	choix = 0;
	file[niveau].fp = fopen (c_disque (forward_sys[choix]), "rt");
	if (file[niveau].fp == NULL)
	{
		choix = 1;
		file[niveau].fp = fopen (c_disque (forward_sys[choix]), "rt");
	}
	strcpy (file[niveau].name, forward_sys[choix]);

	if (file[niveau].fp)
	{

		do
		{
			while (fgets (com_buf, 80, file[niveau].fp))
			{
				++file[niveau].nolig;
				sup_ln (com_buf);
				c = fwd_commande (com_buf);
				if ((c == '*') || (c == '#') || (c == '\0'))
					continue;
				if (c == '<')
				{
					++nb_include;
					if (++niveau >= MAX_NIVEAU)
					{
						--niveau;
						error_file ("too much include levels");
						for (niveau = 1; niveau < MAX_NIVEAU; ++niveau)
						{
							if (file[niveau].fp)
								fclose (file[niveau].fp);
						}
						nb_car = 1;
						niveau = 0;
						break;
					}
					file[niveau].nolig = 0;
					strcpy (file[niveau].name, com_buf);
					if ((file[niveau].fp = fopen (c_disque (file[niveau].name), "rt")) == NULL)
					{
						char str[256];

						sprintf (str, "Cannot find include file %s", file[niveau].name);
						error_file (str);
						for (niveau = 1; niveau < MAX_NIVEAU; ++niveau)
						{
							if (file[niveau].fp)
								fclose (file[niveau].fp);
						}
						nb_car = 1;
						niveau = 0;
						break;
					}
					continue;
				}
				if (tst_ligne (c))
				{
					nb_car = 1;
					break;
				}
				if (c == '!')
				{
					c = fwd_commande (com_buf);
					if (tst_ligne (c))
					{
						nb_car = 1;
						break;
					}
					++nb_car;
				}
				nb_car += (strlen (com_buf) + 2);
			}
			fclose (file[niveau].fp);
		}
		while (niveau--);

		if (nbif[0] != 0)
		{
			error_file ("endif missing");
			nb_car = 1;
		}

	}
	if (nb_car >= fwd_size)
	{
		if (fwd_file)
			m_libere (fwd_file, fwd_size);
		fwd_size = nb_car + nb_car / 5;
		fwd_file = m_alloue (fwd_size);
	}
	fwd_ptr = fwd_file;

	if (nb_include != include_size)
	{
		if (time_include)
			m_libere (time_include, include_size * sizeof (long));

		include_size = nb_include;
		time_include = (long *) m_alloue (include_size * sizeof (long));
	}

	if (nb_car > 1)
	{
		niveau = 0;
		nb_include = 0;

		file[niveau].nolig = 0;
		/* strcpy (file[niveau].name, "FORWARD.SYS"); */
		file[niveau].fp = fopen (c_disque (file[niveau].name), "rt");
		fstat (fileno (file[niveau].fp), &st);
		time_include[nb_include++] = st.st_mtime;
		if (file[niveau].fp)
		{
			do
			{
				while (fgets (com_buf, 80, file[niveau].fp))
				{
					sup_ln (com_buf);
					c = fwd_commande (com_buf);
					if ((c == '*') || (c == '#') || (c == '\0'))
						continue;
					if (c == '<')
					{
						++niveau;
						file[niveau].nolig = 0;
						strcpy (file[niveau].name, com_buf);
						file[niveau].fp = fopen (c_disque (file[niveau].name), "rt");
						fstat (fileno (file[niveau].fp), &st);
						time_include[nb_include++] = st.st_mtime;
						continue;
					}
					*fwd_ptr++ = c;
					if (c == '!')
					{
						c = fwd_commande (com_buf);
						*fwd_ptr++ = c;
					}
					ptr = com_buf;
					while ((*fwd_ptr++ = *ptr++) != 0);
				}
				fclose (file[niveau].fp);
			}
			while (niveau--);
		}

	}

	*fwd_ptr = '\032';

}

static void init_buf_rej_ems (void)
{
	static int old_record_nb = 0;

	FILE *fp;
	int c;
	char mode[3];
	char type;
	char com_buf[80];
	char exped[80];
	char route[80];
	char desti[80];
	char bid[80];
	int size;
	struct stat st;
	Rej_rec rej;
	unsigned record = 0;

	if ((stat (c_disque (REJECT_FILE), &st) == 0) && (st.st_mtime == t_rej))
		return;

	t_rej = st.st_mtime;

	niveau = 0;
	memset (file, 0, sizeof (File) * MAX_NIVEAU);
	file[0].nolig = 0;
	strcpy (file[0].name, REJECT_FILE);

	fp = fopen (c_disque (REJECT_FILE), "rt");
	if (fp)
	{
		while (fgets (com_buf, 80, fp))
		{
			file[0].nolig++;
			sup_ln (strupr (com_buf));
			c = *com_buf;
			if ((c == '#') || (c == '\0'))
				continue;
			if (sscanf (com_buf, "%s %c %s %s %s %s %d",
						mode, &type, exped, route, desti, bid, &size) != 7)
			{
				error_file ("bad number of fields");
			}

			// convert mode to upper case
			char *s = mode;
			while (*s) {
				*s = toupper((unsigned char) *s);
				s++;
			}

			if ((strcmp(mode, "H") != 0) && (strcmp(mode, "R") != 0) && (strcmp(mode, "L") != 0)
				&& (strcmp(mode, "!H") != 0) && (strcmp(mode, "!R") != 0) && (strcmp(mode, "!L") !=0)) 
			{
				error_file ("bad mode (H, R, L or !H, !R, !L)");
			}

			strn_cpy (2, rej.mode, mode);
			rej.type = toupper (type);
			rej.size = size;
			strn_cpy (6, rej.exped, exped);
			strn_cpy (6, rej.via, route);
			strn_cpy (6, rej.desti, desti);
			strn_cpy (12, rej.bid, bid);
			write_rej (record, &rej);
			++record;
		}
		fclose (fp);

		/* Invalide les anciens records */

		rej.mode[0] = '\0';
		while (record < old_record_nb)
		{
			write_rej (record, &rej);
			++record;
		}
		old_record_nb = record;
	}

}

void init_buf_rej (void)
{
	if (EMS_REJ_OK ())
	{
		init_buf_rej_ems ();
		return;
	}
	else
	{
		FILE *fp;
		int c;
		char com_buf[80], tp[80];
		char *ptr, *rej_ptr;
		unsigned nb_car = 1;
		struct stat st;

		if ((stat (c_disque (REJECT_FILE), &st) == 0) && (st.st_mtime == t_rej))
			return;

		t_rej = st.st_mtime;

		niveau = 0;
		memset (file, 0, sizeof (File) * MAX_NIVEAU);
		file[0].nolig = 0;
		strcpy (file[0].name, REJECT_FILE);

		fp = fopen (c_disque (REJECT_FILE), "rt");
		if (fp)
		{
			while (fgets (com_buf, 80, fp))
			{
				sup_ln (com_buf);
				c = *com_buf;
				if ((c == '#') || (c == '\0'))
					continue;
				nb_car += (strlen (com_buf) + 1);
			}
		}

		if (nb_car >= rej_size)
		{
			if (rej_file)
				m_libere (rej_file, rej_size);
			rej_size = nb_car + nb_car / 5;
			rej_file = m_alloue (rej_size);
		}
		rej_ptr = rej_file;

		if (fp)
		{
			rewind (fp);
			while (fgets (com_buf, 80, fp))
			{
				file[0].nolig++;
				sup_ln (strupr (com_buf));
				c = *com_buf;
				if ((c == '#') || (c == '\0'))
					continue;
				if (sscanf (com_buf, "%s %s %s %s %s %s %s", tp, tp, tp, tp, tp, tp, tp) != 7)
				{
					error_file ("bad number of fields");
				}
				ptr = com_buf;
				while ((*rej_ptr++ = *ptr++) != '\0');
			}
			fclose (fp);
		}

		*rej_ptr = '\032';
	}
}

void end_swap (void)
{
	if (swap_file)
		m_libere (swap_file, fwd_size);
}

void init_buf_swap (void)
{
	FILE *fp;
	int c;
	char com_buf[80];
	char *ptr, *swap_ptr;
	unsigned nb_car = 1;
	struct stat st;

	niveau = 0;
	memset (file, 0, sizeof (File) * MAX_NIVEAU);
	file[0].nolig = 0;
	strcpy (file[0].name, "SWAPP.SYS");

	ptr = c_disque ("SWAPP.SYS");
	strcpy (com_buf, ptr);
	if ((stat (com_buf, &st) == 0) && (st.st_mtime == t_swap))
		return;

	t_swap = st.st_mtime;

	fp = fopen (c_disque ("SWAPP.SYS"), "rb");
	if (fp)
	{
		while (fgets (com_buf, 80, fp))
		{
			sup_ln (com_buf);
			c = fwd_commande (com_buf);
			if ((c == '#') || (c == '\0'))
				continue;
			nb_car += (strlen (com_buf) + 2);
		}
	}

	if (nb_car >= swap_size)
	{
		if (swap_file)
			m_libere (swap_file, fwd_size);
		swap_size = nb_car + nb_car / 5;
		swap_file = m_alloue (swap_size);
	}
	swap_ptr = swap_file;

	if (fp)
	{
		rewind (fp);
		while (fgets (com_buf, 80, fp))
		{
			file[0].nolig++;
			sup_ln (strupr (com_buf));
			c = fwd_commande (com_buf);
			if ((c == '#') || (c == '\0'))
				continue;
			if ((c != '<') && (c != '>') && (c != '@'))
			{
				error_file ("bad field specification (<, > of @)");
			}
			*swap_ptr++ = c;
			ptr = com_buf;
			while ((*swap_ptr++ = *ptr++) != '\0');
		}
		fclose (fp);

	}
	*swap_ptr = '\032';
}


static void reset_fwd_pointeurs (void)
{
	int port;
	Forward *pfwd;

	for (port = 0; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			pfwd = p_port[port].listfwd;
			while (pfwd)
			{
				pfwd->fwdpos = 0;
				pfwd = pfwd->suite;
			}
		}
	}
}

static int fwd_commande (char *com_buf)
{
	int type;
	char *ptr = com_buf, *lptr = com_buf;

	sup_ln (com_buf);

	while ((*ptr) && (!ISGRAPH (*ptr)))
		++ptr;
	type = toupper (*ptr);

	++ptr;
	if ((type == 'D') || (type == 'L') || (type == 'X'))
	{
		if (*ptr == 'C')
		{
			type = tolower (type);
			++ptr;
		}
	}

	if ((type == 'E') && (toupper (*ptr) == 'L'))
		type = '@';

	if (type != '!')
		while (ISGRAPH (*ptr))
			++ptr;

	while ((*ptr) && (!ISGRAPH (*ptr)))
		++ptr;

	if (type == 'P')
	{
		swap_port (ptr);
	}

	while ((*lptr++ = *ptr++) != '\0');
	return (type);
}

void end_bbs (void)
{
	if (bbs_ptr)
		m_libere (bbs_ptr, 7 * NBBBS);
	bbs_ptr = NULL;
}

void init_bbs (void)
{
	FILE *fichier;
	char chaine[80], bbs[80];
	char *ptr;
	int i;
	int nb = 0;
	int tot = 0;
	struct stat st;

	if ((stat (c_disque ("BBS.SYS"), &st) == 0) && (st.st_mtime == t_bbs))
		return;

	t_bbs = st.st_mtime;

#ifdef __linux__
#ifdef ENGLISH
	if (!operationnel)
		cprintf ("BBS set-up        \n");
#else
	if (!operationnel)
		cprintf ("Initialisation BBS\n");
#endif
#else
#ifdef ENGLISH
	if (!operationnel)
		cprintf ("BBS set-up        \r\n");
#else
	if (!operationnel)
		cprintf ("Initialisation BBS\r\n");
#endif
#endif

	if (bbs_ptr == NULL)
	{
		bbs_ptr = m_alloue (7 * NBBBS);
	}

	ptr = bbs_ptr;
	if ((fichier = fopen (c_disque ("BBS.SYS"), "rt")) != NULL)
	{
		while (fgets (chaine, 78, fichier))
		{
			int itmp;

			sup_ln (chaine);
			if (*chaine == '#')
				continue;
			*bbs = '\0';
			sscanf (chaine, "%d %s", &itmp, bbs);
#if defined(__WINDOWS__) || defined(__linux__)
			if (*bbs)
			{
				char text[80];

				++tot;
				sprintf (text, "%d: %s", tot, bbs);
				InitText (text);
			}
#endif
			for (i = 0; i < 6; i++)
				*ptr++ = bbs[i];
			*ptr++ = '\001';
			if (++nb == NBBBS)
				break;
		}
		fclose (fichier);
	}
}


static int type_commande (char *com_buf)
{
	int type;
	char *ptr = com_buf, *lptr = com_buf;

	while ((*ptr) && (!ISGRAPH (*ptr)))
		++ptr;
	type = toupper (*ptr);
	if ((type == 'E') && (toupper (*(ptr + 1)) == 'L'))
		type = '@';
	while (ISGRAPH (*ptr))
		++ptr;
	while ((*ptr) && (!ISGRAPH (*ptr)))
		++ptr;
	while ((*lptr++ = *ptr++) != '\0');
	sup_ln (com_buf);
	return (type);
}


void cron (long h_time)
{
	FILE *fptr;
	char com_buf[80];
	int cptif = 0;
	int lig = 0;
	int fin = 0;
	int modif = 0;
	int temp;
	char *cronname;

	aff_etat ('C');
#ifdef __FBBDOS__
	cronname = "cron_d.sys";
#endif
#ifdef __WINDOWS__
	cronname = "cron_w.sys";
#endif
#ifdef __linux__
	cronname = "cron_l.sys";
#endif
	if ((fptr = fopen (c_disque (cronname), "rt")) == NULL)
	{
		if ((fptr = fopen (c_disque ("cron.sys"), "rt")) == NULL)
		{
			aff_etat ('A');
			return;
		}
	}

	while ((!fin) && (fgets (com_buf, 80, fptr)))
	{
		++lig;
		sup_ln (com_buf);
		if (*com_buf == '#')
			continue;
		switch (type_commande (com_buf))
		{
		case 'E':				/* ENDIF */
			if (cptif)
				--cptif;
			else
				fin = erreur_cron (lig);
			break;
		case '@':				/* ELSE */
			if (cptif)
			{
				temp = cptif - 1;
				while (cptif != temp)
				{
					if (fgets (com_buf, 80, fptr) == NULL)
					{
						fin = erreur_cron (lig);
						break;
					}
					++lig;
					switch (type_commande (com_buf))
					{
					case 'I':
						++cptif;
						break;
					case 'E':
						if (cptif)
							--cptif;
						else
							fin = erreur_cron (lig);
						break;
					default:
						break;
					}
				}
			}
			else
				fin = erreur_cron (lig);
			break;
		case 'I':
			++cptif;
			if (tst_fwd (com_buf, 0, h_time, 0, NULL, 1, -1) == FALSE)
			{
				temp = cptif - 1;
				while (cptif != temp)
				{
					if (fgets (com_buf, 80, fptr) == NULL)
					{
						fin = erreur_cron (lig);
						break;
					}
					++lig;
					switch (type_commande (com_buf))
					{
					case 'I':
						++cptif;
						break;
					case 'E':
						if (cptif)
							--cptif;
						else
							fin = erreur_cron (lig);
						break;
					case '@':
						if (cptif == (temp + 1))
							++temp;
						break;
					default:
						break;
					}
				}
			}
			break;
		case 'D':				/* Dos  */
			if (strncmpi (com_buf, "PTCTRX", 6) == 0)
			{
				ptctrx (0, com_buf);
			}
			else
			{
#if defined(__WINDOWS__) || defined(__FBBDOS__)
				send_dos (1, com_buf, NULL);
#endif
#ifdef __linux__
				char *pptr = com_buf;

				call_nbdos (&pptr, 1, NO_REPORT_MODE, NULL, TOOLDIR, NULL);
#endif
			}
			break;
		case 'X':				/* Dos  */
			if (strncmpi (com_buf, "PTCTRX", 6) == 0)
			{
				ptctrx (0, com_buf);
			}
			else
			{
#if defined(__WINDOWS__) || defined(__FBBDOS__)
				send_dos (2, com_buf, NULL);
#endif
#ifdef __linux__
				char *pptr = com_buf;

				call_nbdos (&pptr, 1, NO_REPORT_MODE, NULL, TOOLDIR, NULL);
#endif
			}
			break;
		case 'T':				/* Talk */
			talk_select (com_buf);
			modif = 1;
			break;
		case 'B':				/* Bip  */
			bip_select (com_buf);
			modif = 1;
			break;
		case 'G':				/* Gate */
			gate_select (com_buf);
			modif = 1;
			break;
		case 'L':				/* Unproto Lists */
			list_select (com_buf);
			break;
		case 'M':				/* Modification des parametres du port */
			port_select (com_buf);
			break;
		case 'Y':				/* Yapp */
			yapp_select (com_buf);
			break;
		}
	}
	fclose (fptr);
	aff_etat ('A');
#if defined(__WINDOWS__) || defined(__linux__)
	if (modif)
	{
		maj_menu_options ();
	}
#endif
}


static char *analyse (char *text, int *port, int *rep)
{
	char temp[40];
	char val[40];
	int i;

	*port = 0;

	for (i = 0; *text; ++i, ++text)
	{
		if (isspace (*text))
			break;
		temp[i] = *text;
	}
	temp[i] = '\0';

	while ((*text) && (isspace (*text)))
		++text;

	for (i = 0; *text; ++i, ++text)
	{
		if (isspace (*text))
			break;
		val[i] = *text;
	}
	val[i] = '\0';

	while ((*text) && (isspace (*text)))
		++text;

	if (*val)
		*rep = *val;
	else
		*rep = *temp;

	if (*temp)
	{
		swap_port (temp);
		if (isdigit (*temp))
			*port = *temp - '0';
		else
			*port = *temp - '@';
	}

	return (text);
}


static void talk_select (char *text)
{
	int port, rep;

	analyse (text, &port, &rep);
	ok_tell = (rep != 'N');
}


static void bip_select (char *text)
{
	int port, rep;

	analyse (text, &port, &rep);
	bip = (rep != 'N');
}


static void gate_select (char *text)
{
	int p;
	int port, rep;

	analyse (text, &port, &rep);
	for (p = 1; p < NBPORT; p++)
	{
		if ((port == 0) || (p == port))
		{
			if (rep == 'N')
				p_port[p].moport &= (~0x10);
			else
				p_port[p].moport |= 0x10;
		}
	}
}


static void yapp_select (char *text)
{
	int p;
	int port, rep;

	analyse (text, &port, &rep);
	for (p = 1; p < NBPORT; p++)
	{
		if ((port == 0) || (p == port))
		{
			if (rep == 'N')
				p_port[p].moport &= (~0x04);
			else
				p_port[p].moport |= 0x04;
		}
	}
}


static void list_select (char *text)
{
	int p;
	int port, rep;

	analyse (text, &port, &rep);
	for (p = 1; p < NBPORT; p++)
	{
		if ((port == 0) || (p == port))
		{
			if (rep == 'N')
				p_port[p].moport &= (~0x20);
			else
				p_port[p].moport |= 0x20;
		}
	}
}


static void port_select (char *text)
{
	int p;
	int port, rep;
	int min, per;
	char *fwd_mp;

	fwd_mp = analyse (text, &port, &rep);

	for (p = 1; p < NBPORT; p++)
	{
		if ((port == 0) || (p == port))
		{

			switch (rep)
			{
			case 'G':
				p_port[p].moport &= 0xf8;
				p_port[p].moport |= 1;
				break;
			case 'B':
				p_port[p].moport &= 0xf8;
				p_port[p].moport |= 2;
				break;
			case 'U':
				p_port[p].moport &= 0xf8;
				break;
			}

			if (*fwd_mp)
			{
				if (sscanf (fwd_mp, "%d/%d", &min, &per) == 2)
				{
					p_port[p].min_fwd = min;
					p_port[p].per_fwd = per;
				}
			}

		}
	}
}


static int erreur_cron (int lig)
{
	char wtexte[200];

	deb_io ();
#ifdef ENGLISH
	sprintf (wtexte, "\r\nError in CRON.SYS file line %d  \r\n\a", lig);
	if (w_mask & W_FILE)
		mess_warning (admin, "*** FILE ERROR ***    ", wtexte);
#else
	sprintf (wtexte, "\r\nErreur fichier CRON.SYS ligne %d\r\n\a", lig);
	if (w_mask & W_FILE)
		mess_warning (admin, "*** ERREUR FICHIER ***", wtexte);
#endif
	cprintf ("%s\r\n",wtexte);
	sleep_ (5);
	fin_io ();
	return (1);
}
