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
 *  MODULE INFORMATIONS
 */

#include <serv.h>

#define LGINF		128
#define LGLABEL		31
#define MAXLABEL	258
#define NBINF		MAXLABEL-2

static char *read_doc_label (char *);

static int open_doc_label (void);
static int selecte (int);
static int trie_docs (void);

static void affich_docs (void);
static void close_doc_label (void);
static void docs (void);
static void retour_doc (void);
static void retour_docs (void);
static void send_doc (char *);
static void write_doc_label (void);

typedef struct
{
	char dir;
	char nom[LGINF];
}
inf;

typedef struct
{
	char nom[LGINF];
	char label[LGLABEL];
}
doc_label;

static inf *ptete;
static doc_label *label_ptr;
static unsigned nb_label;

static void docs (void)
{

	limite_commande ();
	switch (toupper (*indd))
	{

	case 'R':
		retour_docs ();
		retour_doc ();
		break;

	case 'F':
		retour_menu (N_MENU);
		break;

	case 'B':
		maj_niv (N_MENU, 0, 0);
		sortie ();
		break;

	case 'L':
	case 'W':
		affich_docs ();
		retour_doc ();
/*      retour_menu(N_MENU); */
		break;

	case 'D':
		if (droits (MODLABEL))
		{
			write_doc_label ();
			retour_doc ();
		}
		else
			cmd_err (indd);
		break;

	default:
		if (!defaut ())
		{
			if (isdigit (*indd))
			{
				if (!selecte (atoi (indd)))
					retour_doc ();
			}
			else
			{
				if (ISGRAPH (*indd))
					cmd_err (indd);
				else
					retour_doc ();
				/*
				   texte(T_INF+3);
				   retour_doc();
				 */
			}
		}
		break;

	}
}


static void retour_docs (void)
{
	char *ptr;
#ifdef __linux__
	ptr = strrchr (pvoie->ch_temp, '/');
#else
	ptr = strrchr (pvoie->ch_temp, '\\');
#endif
	if (ptr == NULL)
		*pvoie->ch_temp = '\0';
	else
		*ptr = '\0';
}


static int selecte (int numero)
{
	char s[80];
	inf *pdoc;
	int fic = 0;
	int nb = trie_docs ();

	--numero;

	if ((numero >= 0) && (numero < nb))
	{
		pdoc = ptete + numero;
		if (pdoc->dir == 'D')
		{
#ifdef __linux__
			strcat (pvoie->ch_temp, "/");
#else
			strcat (pvoie->ch_temp, "\\");
#endif
			strcat (pvoie->ch_temp, pdoc->nom);
		}
		else
		{
#ifdef __linux__
			sprintf (s, "%s%s%s", DOCSDIR, pvoie->ch_temp, pdoc->nom);
#else
			sprintf (s, "%s%s\\%s", DOCSDIR, pvoie->ch_temp, pdoc->nom);
#endif
			fic = 1;
		}
	}
	else
	{
		texte (T_INF + 3);
	}
	m_libere (ptete, sizeof (inf) * NBINF);
	if (fic)
	{
		ch_niv2 (1);
		send_doc (s);
	}
	return (fic);
}


static int trie_docs (void)
{
	char localdir[MAXLABEL];
	int fin, nbinf = 0;
	inf *pdoc;
	struct ffblk dirblk;

	pdoc = ptete = (inf *) m_alloue (sizeof (inf) * NBINF);

#ifdef __linux__
	sprintf (localdir, "%s%s*.*", DOCSDIR, pvoie->ch_temp);
#else
	sprintf (localdir, "%s%s\\*.*", DOCSDIR, pvoie->ch_temp);
#endif
	fin = findfirst (localdir, &dirblk, FA_DIREC);
	while (!fin)
	{
		if ((*dirblk.ff_name != '.') && (strcmp (dirblk.ff_name, "@@.LBL")))
		{
			strcpy (pdoc->nom, dirblk.ff_name);
			pdoc->dir = (dirblk.ff_attrib & FA_DIREC) ? 'D' : 'F';
			++pdoc;
			++nbinf;
		}
		if (nbinf == NBINF)
			break;
		fin = findnext (&dirblk);
	}

	if (nbinf)
		qsort ((void *) ptete, nbinf, sizeof (inf), (int (*)(const void *, const void *)) strcmp);

	return (nbinf);
}


static void affich_docs (void)
{
	int pos, i, nbinf, dir = 1, dde_cr = 0;
	inf *pdoc;
	char s[80];
	char *ptr, *sptr;

	nbinf = trie_docs ();
	pdoc = ptete;
	i = 1;
	pos = 0;
	while (nbinf--)
	{
		if (pdoc->dir == 'F')
		{
			if (dir)
			{
				if (open_doc_label ())
				{
					if (pos != 0)
						cr ();
					cr ();
					dir = 0;
					pos = 0;
				}				/*else
								   break; */
			}
			ptr = read_doc_label (pdoc->nom);
			if (droits (MODLABEL))
			{
				if (ptr)
				{
					sprintf (s, "%2d:%-12s %-24s", i, pdoc->nom, ptr);
				}
				else
				{
					sprintf (s, "%2d:%-36s", i, pdoc->nom);
				}
				dde_cr = 1;
			}
			else
			{
				if (ptr)
				{
					sprintf (s, "%2d:%-32s", i, ptr);
					dde_cr = (++pos == 2);
				}
				else
					*s = '\0';
			}
		}
		else
		{
			sptr = pdoc->nom;
			while (*sptr)
			{
				if (*sptr == '_')
					*sptr = ' ';
				++sptr;
			}
			if ((sptr = strchr (pdoc->nom, '.')) != NULL)
			{
				while ((*sptr = *(sptr + 1)) != '\0')
					++sptr;
			}
			sprintf (s, "%2d:%-12s", i, pdoc->nom);
			dde_cr = (++pos == 5);
		}
		out (s, strlen (s));
		if (dde_cr)
		{
			cr ();
			pos = 0;
			dde_cr = 0;
		}
		++pdoc;
		++i;
	}
	if (pos != 0)
		cr ();
	cr ();
	if (dir == 0)
		close_doc_label ();
	m_libere (ptete, sizeof (inf) * NBINF);
}


static int open_doc_label (void)
{
	int fd;
	doc_label *plabel;
	char nom_label[MAXLABEL];

	nb_label = 0;
#ifdef __linux__
	sprintf (nom_label, "%s%s@@.LBL", DOCSDIR, pvoie->ch_temp);
#else
	sprintf (nom_label, "%s%s\\@@.LBL", DOCSDIR, pvoie->ch_temp);
#endif
	if ((fd = open (nom_label, O_RDONLY | O_BINARY)) != -1)
	{
		nb_label = (unsigned) (filelength (fd) / sizeof (doc_label));

		if (nb_label == 0)
			return (0);

		plabel = label_ptr = (doc_label *) m_alloue (nb_label * sizeof (doc_label));
		while (read (fd, plabel, sizeof (doc_label)))
			++plabel;
		close (fd);
		return (1);
	}
	else
		return (0);
}


static void write_doc_label (void)
{
	int i, nb, fd;
	char *ptr;
	doc_label label, tmp_label;
	char nom_label[MAXLABEL];

	incindd ();
	nb = 0;
	if ((ptr = strtok (indd, " \r")) == NULL)
	{
		texte (T_ERR + 20);
		return;
	}
	++nb;

#ifdef __linux__
	n_cpy (LGINF - 1, tmp_label.nom, ptr);
	sprintf (nom_label, "%s%s%s", DOCSDIR, pvoie->ch_temp, tmp_label.nom);
#else
	strn_cpy (LGINF - 1, tmp_label.nom, ptr);
	sprintf (nom_label, "%s%s\\%s", DOCSDIR, pvoie->ch_temp, tmp_label.nom);
#endif
	if (access (nom_label, 0) != 0)
	{
		strcpy (pvoie->appendf, tmp_label.nom);
		texte (T_ERR + 11);
		return;
	}

	if ((ptr = strtok (NULL, "\r")) != NULL)
	{
		++nb;
		n_cpy (LGLABEL - 1, tmp_label.label, ptr);
	}
	if (nb == 2)
	{
#ifdef __linux__
		sprintf (nom_label, "%s%s@@.LBL", DOCSDIR, pvoie->ch_temp);
#else
		sprintf (nom_label, "%s%s\\@@.LBL", DOCSDIR, pvoie->ch_temp);
#endif
		if ((fd = open (nom_label, O_CREAT | O_RDWR | O_BINARY, S_IREAD | S_IWRITE)) != -1)
		{
			i = 0;
			while (read (fd, &label, sizeof (doc_label)))
			{
				if (strcmp (label.nom, tmp_label.nom) == 0)
					break;
				++i;
			}
			lseek (fd, (long) i * sizeof (doc_label), 0);
			write (fd, &tmp_label, sizeof (doc_label));
			close (fd);
		}
	}
	else
		texte (T_ERR + 0);
}


static void close_doc_label (void)
{
	if (nb_label)
		m_libere (label_ptr, nb_label * sizeof (doc_label));
}


static char *read_doc_label (char *nom)
{
	int i;
	doc_label *plabel = label_ptr;


	for (i = 0; i < nb_label; i++, plabel++)
	{
		if (strcmp (plabel->nom, nom) == 0)
			return (plabel->label);
	}
	return (NULL);
}


static void send_doc (char *doc)
{
	switch (pvoie->niv3)
	{
	case 0:
		strcpy (pvoie->sr_fic, doc);
		pvoie->enrcur = 0L;
		if (senddata (0))
			retour_doc ();
		else
			ch_niv3 (1);
		break;

	case 1:
		if (senddata (0))
			retour_doc ();
		break;
	}
}


void doc_path (void)
{
	char *ptr;
	char *p;

	var_cpy (0, pvoie->ch_temp);
	ptr = varx[0];
	while (*ptr)
	{
		/* if (*ptr == '\\')
		   *ptr = '|';
		   else */
		if (*ptr == '_')
			*ptr = ' ';
		else if (*ptr == '.')
		{
			p = ptr;
			while ((*p = *(p + 1)) != '\0')
				++p;
		}
		++ptr;
	}
}

void retour_doc (void)
{
	doc_path ();
	retour_menu (N_INFO);
}


void documentations (void)
{
	switch (pvoie->niv2)
	{
	case 0:
		docs ();
		break;
	case 1:
		send_doc (NULL);
		break;
	}
}
