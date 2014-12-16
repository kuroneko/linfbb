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
 *    MODULE FORWARDING OVERLAY 5
 */

#include <serv.h>

#define MPAR MESSDIR

typedef struct
{
	long val;
	int num;
	char bbs[7];
	char bid[13];
	long taille;
}
Rec_enc;

#define	MAX_ENCOURS	20

typedef struct type_encours
{
	Rec_enc enc[MAX_ENCOURS];
	struct type_encours *next;
}
Encours;

static Encours *tete_encours = NULL;
static long part_num;

static char *r_bloc (int numero)
{
	Broute *pcur = tbroute;

	df ("r_bloc", 1);

#ifndef __WINDOWS__
	if (EMS_HRT_OK ())
	{
		ff ();
		return (sel_bloc (HROUTE, numero));
	}
#endif

	while (numero--)
	{
		pcur = pcur->suiv;
		if (pcur == NULL)
		{
			ff ();
			return (NULL);
		}
	}
	ff ();
	return (pcur->b_route);
}

static int r_bloc_size (void)
{
	return (EMS_HRT_OK ()? EMS_BLOC : MAX_BROUTE);
}

static char *new_bloc_route (void)
{
	char *ptr;
	Broute *pcur = tbroute;

	df ("new_bloc_route", 0);

#ifndef __WINDOWS__
	if (EMS_HRT_OK ())
	{
		ptr = new_bloc (HROUTE);
	}
	else
	{
#endif
		if (pcur)
		{
			while (pcur->suiv)
				pcur = pcur->suiv;
			pcur->suiv = (Broute *) m_alloue (sizeof (Broute));
			pcur = pcur->suiv;
		}
		else
		{
			pcur = tbroute = (Broute *) m_alloue (sizeof (Broute));
		}
		pcur->suiv = NULL;
		ptr = pcur->b_route;
#ifndef __WINDOWS__
	}
#endif
	ff ();
	return (ptr);
}

void cree_routes (void)
{
	int pos;
	int len;
	int nb_routes = 0;
	FILE *fptr;
	char *mptr;
	char status;
	char ligne[80];
	char hroute[80];
	int numero = 0;
	int max_broute = r_bloc_size ();

	if (!h_ok)
		return;

	df ("cree_routes", 0);
	new_bloc_route ();

	if ((fptr = fopen (d_disque ("HROUTE.SYS"), "rt")) != NULL)
	{
#ifdef ENGLISH
		cprintf ("H-ROUTES set-up...         \r\n");
#else
		cprintf ("Initialisation des H-ROUTES\r\n");
#endif
		mptr = r_bloc (numero);
		if (mptr)
		{
			memset (mptr, 0, r_bloc_size ());
			pos = 0;
			while (fgets (ligne, 80, fptr))
			{
				status = '#';
				strupr (ligne);
				sscanf (ligne, "%c %s", &status, hroute);
				if (status == '#')
					continue;

				len = strlen (hroute) + 2;

				if ((pos + len) >= max_broute)
				{
					new_bloc_route ();
					mptr = r_bloc (++numero);
					if (mptr == NULL)
						break;
					memset (mptr, 0, r_bloc_size ());
					pos = 0;
				}
				*mptr++ = status;
				strcpy (mptr, hroute);
				mptr += (len - 1);
				pos += len;
				if ((nb_routes % 100) == 0)
					cprintf ("\r%d routes", nb_routes);
				++nb_routes;
			}
		}
		ferme (fptr, 76);
		cprintf ("\r%d routes\r\n", nb_routes);
	}
	ff ();
}

static void kill_file_route (char *route)
{
	FILE *fptr;
	char ligne[80];
	char hroute[80];
	long pos = 0L;
	char type;

	df ("kill_file_route", 2);

	if ((fptr = fopen (d_disque ("HROUTE.SYS"), "r+t")) != NULL)
	{
		while (fgets (ligne, 80, fptr))
		{
			sscanf (ligne, "%c %s", &type, hroute);
			if ((type == 'V') && (strcmp (hroute, route) == 0))
			{
				fseek (fptr, pos, 0);
				fputc ('#', fptr);
				break;
			}
			pos = ftell (fptr);
		}
		ferme (fptr, 75);
	}
	ff ();
}

static void new_file_route (char *route)
{
	FILE *fptr;
	char ligne[80];

	df ("new_file_route", 2);

	if ((fptr = fappend (d_disque ("HROUTE.SYS"), "b")) != NULL)
	{
		sprintf (ligne, "V %s\r\n", route);
		fputs (ligne, fptr);
		ferme (fptr, 74);
	}
	ff ();
}

static void new_route (char *route)
{
	char *scan;
	char *mptr = NULL;
	int len;
	unsigned pos = 0;
	int numero = 0;
	int max_broute = r_bloc_size ();

	/*
	   char temp[80];
	 */

	df ("new_route", 2);

	len = strlen (route) + 2;

	while ((scan = r_bloc (numero)) != NULL)
	{
		pos = 0;
		for (;;)
		{

			mptr = scan;
			if (*scan == '\0')
				break;

			while (*scan)
			{
				++scan;
				++pos;
			}
			++scan;
			++pos;

			if (pos >= max_broute)
				break;
		}
		++numero;
	}
	if ((pos + len) >= max_broute)
	{
		new_bloc_route ();
		mptr = r_bloc (numero);
		memset (mptr, 0, r_bloc_size ());
	}
	*mptr++ = 'V';
	strcpy (mptr, route);
	new_file_route (route);
	ff ();
}

int cherche_route (bullist * pbull)
{
	char *ptr;
	int trouve = 0;
	char *scan;
	char *mptr = NULL;
	char stat;
	unsigned pos;
	int numero = 0;
	int max_broute = r_bloc_size ();

	df ("cherche_route", 2);

	if ((*pbull->bbsv == '\0') || (strchr (pbull->bbsv, '.')))
	{
		ff ();
		return (0);
	}

	if (route_wp_hier (pbull))
	{
		var_cpy (0, "WP");
		ff ();
		return (1);
	}

	if (!h_ok)
	{
		ff ();
		return (0);
	}

	var_cpy (0, "H");

	while ((scan = r_bloc (numero)) != NULL)
	{
		pos = 0;
		for (;;)
		{
			stat = *scan++;
			++pos;
			mptr = scan;
			if (stat == '\0')
				break;
			ptr = pbull->bbsv;
			for (;;)
			{
				if ((!isalnum (*scan)) && (*ptr == '\0'))
					trouve = 1;
				if (*scan != *ptr)
					break;
				++scan;
				++ptr;
				++pos;
			}
			if (trouve)
				break;
			while (*scan)
			{
				++scan;
				++pos;
			}
			++scan;
			++pos;
			if (pos >= max_broute)
				break;
		}
		if (trouve)
			break;
		++numero;
	}
	if (trouve)
	{
		strn_cpy (40, pbull->bbsv, mptr);
	}
	ff ();
	return (trouve);
}

char *extend_bbs (char *bbs)
{
	static char ext_bbs[41];
	bullist bull;

	df ("extend_bbs", 2);

	strn_cpy (40, bull.bbsv, bbs);
	strn_cpy (6, bull.desti, bbs);

	if (cherche_route (&bull))
	{
		strn_cpy (40, ext_bbs, bull.bbsv);
		return (ext_bbs);
	}

	ff ();
	return (bbs);
}

static void add_route (char *route)
{
	char *ptr;
	int trouve = 0;
	char stat = '\0';
	char *scan;
	char *mptr = NULL;
	char *bbsv;
	unsigned pos;
	int numero = 0;
	int max_broute = r_bloc_size ();

	df ("add_route", 2);

	bbsv = bbs_via (strupr (route));

	while ((scan = r_bloc (numero)) != NULL)
	{
		pos = 0;
		for (;;)
		{
			stat = *scan++;
			++pos;
			mptr = scan;
			if (stat == '\0')
				break;
			ptr = bbsv;
			for (;;)
			{
				if ((!isalnum (*scan)) && (*ptr == '\0'))
					trouve = 1;
				if (*scan != *ptr)
					break;
				++scan;
				++ptr;
				++pos;
			}
			if (trouve)
				break;
			while (*scan)
			{
				++scan;
				++pos;
			}
			++scan;
			++pos;
			if (pos >= max_broute)
				break;
		}
		if (trouve)
			break;
		++numero;
	}
	if (trouve)
	{
		if ((stat != 'P') && (strcmp (route, mptr) != 0))
		{
			kill_file_route (mptr);
			while (*mptr)
				*mptr++ = '@';
			new_route (route);
		}
	}
	else
	{
		new_route (route);
	}
	ff ();
}

int hupdate (void)
{
	Hroute *pcurr;

	df ("hupdate", 0);

	if (throute)
	{
		aff_etat ('W');
		pcurr = throute;
		if (strchr (throute->route, '.'))
			add_route (throute->route);
		throute = throute->suiv;
		m_libere (pcurr, sizeof (Hroute));
		aff_etat ('A');
	}

	ff ();
	return (throute != NULL);
}

/* Teste si la route existe dans les headers */
int is_route (char *route)
{
	Route *r_ptr = pvoie->r_tete;
	char *ptr;
	int nb;

	df ("is_route", 2);
	while (r_ptr)
	{
		for (nb = 0; nb < NBROUTE; nb++)
		{
			ptr = r_ptr->call[nb];
			if (*ptr == '\0')
				break;
			if (strcmp (route, ptr) == 0)
			{
				ff ();
				return (1);
			}
		}
		r_ptr = r_ptr->suite;
	}
	ff ();
	return (0);
}

void old_part (char *bbs, char *bid)
{
	int pos;
	Encours *eptr = tete_encours;

	df ("old_part", 4);
	*pvoie->sr_fic = '\0';

	/* Cherche l'entree */
	while (eptr)
	{
		for (pos = 0; pos < MAX_ENCOURS; pos++)
		{
			if (*eptr->enc[pos].bbs == '\0')
				continue;
			if ((strcmp (eptr->enc[pos].bbs, bbs) == 0) &&
				(strcmp (eptr->enc[pos].bid, bid) == 0))
			{
				sprintf (pvoie->sr_fic, "%s%08ld.TMP", MPAR, eptr->enc[pos].val);
				break;
			}
		}
		eptr = eptr->next;
	}

	if (*pvoie->sr_fic == '\0')
	{
		fbb_error (ERR_CREATE, bid, 0);
	}

	ff ();

}

void mod_part (char *bbs, long size, char *bid)
{
	int pos;
	char file[80];
	Encours *eptr = tete_encours;

	df ("mod_part", 6);
	/* Cherche l'entree */
	while (eptr)
	{
		for (pos = 0; pos < MAX_ENCOURS; pos++)
		{
			if (*eptr->enc[pos].bbs == '\0')
				continue;
			if ((strcmp (eptr->enc[pos].bbs, bbs) == 0) &&
				(strcmp (eptr->enc[pos].bid, bid) == 0))
			{
				if (size == 0L)
				{
					*eptr->enc[pos].bbs = 0;
					sprintf (file, "%s%08ld.TMP", MPAR, eptr->enc[pos].val);
					unlink (file);
				}
				else
				{
					eptr->enc[pos].taille = size;
				}
				break;
			}
		}
		eptr = eptr->next;
	}

	ff ();
}

static void add_part (long part_val, char *bbs, long size, char *bid)
{
	int fin = 0;
	int pos = 0;
	Encours *eptr = tete_encours;

	df ("add_part", 8);
	if (!eptr)
	{
		tete_encours = eptr = (Encours *) m_alloue (sizeof (Encours));
	}

	/* Cherche une entree libre */
	while (eptr)
	{
		for (pos = 0; pos < MAX_ENCOURS; pos++)
		{
			if (*eptr->enc[pos].bbs == 0)
			{
				fin = 1;
				break;
			}
		}
		if (fin)
			break;
		if (!eptr->next)
		{
			eptr->next = (Encours *) m_alloue (sizeof (Encours));
		}
		eptr = eptr->next;
	}

	strn_cpy (13, eptr->enc[pos].bid, bid);
	strn_cpy (6, eptr->enc[pos].bbs, bbs);
	eptr->enc[pos].val = part_val;
	eptr->enc[pos].taille = size;
	ff ();
}

int part_file (char *bbs, char *bid)
{
	int fd;

	df ("part_file", 4);
	part_num = (part_num + 1L) & 0x3ffffffL;
	sprintf (pvoie->sr_fic, "%s%08ld.TMP", MPAR, part_num);
	add_part (part_num, bbs, 0L, bid);

	fd = open (pvoie->sr_fic, O_WRONLY | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
	if (fd == -1)
	{
		write_error (pvoie->sr_fic);
		ff ();
		return (0);
	}

	if (write (fd, bbs, 7) != 7)
	{
		write_error (pvoie->sr_fic);
		ff ();
		return (0);
	}

	if (write (fd, bid, 13) != 13)
	{
		write_error (pvoie->sr_fic);
		ff ();
		return (0);
	}

	close (fd);

	ff ();
	return (1);
}

void del_part (int voie, char *bid)
{
	int pos;
	char file[80];
	Encours *eptr = tete_encours;

	df ("del_part", 2);
	while (eptr)
	{
		for (pos = 0; pos < MAX_ENCOURS; pos++)
		{
			if (*eptr->enc[pos].bbs == 0)
				continue;
			if (strcmp (eptr->enc[pos].bid, bid) == 0)
			{
				int v;

				*eptr->enc[pos].bbs = 0;
				sprintf (file, "%s%08ld.TMP", MPAR, eptr->enc[pos].val);
				for (v = 1; v < NBVOIES; v++)
				{
					if (v == voie)
						continue;
					if ((svoie[v]->sta.connect) && (strcmp (file, svoie[v]->sr_fic) == 0))
						break;
				}
				if (v == NBVOIES)
					unlink (file);
				/*
				   else
				   {
				   char buffer[80];
				   sprintf(buffer, "del_part ch %d : %s found on %d", voie, file, v);
				   write_error(buffer);
				   }
				 */
			}
		}
		eptr = eptr->next;
	}

	ff ();
}

void end_parts (void)
{
	Encours *eptr;

	while (tete_encours)
	{
		eptr = tete_encours;
		tete_encours = tete_encours->next;
		m_libere (eptr, sizeof (Encours));
	}
}

void init_part (void)
{
	int fd;
	int test;
	char path[80];
	char file[80];
	char bid[14];
	char call[7];
	bullist bul;
	struct ffblk dirblk;

	df ("init_part", 0);

	part_num = time (NULL);

	memset (&bul, 0, sizeof (bullist));

	sprintf (path, "%s*.TMP", MPAR);
	if (findfirst (path, &dirblk, FA_DIREC))
	{
		ff ();
		return;
	}

	do
	{
		if ((dirblk.ff_attrib & FA_DIREC) == 0)
		{
			sprintf (file, "%s%s", MPAR, dirblk.ff_name);
			if ((fd = open (file, O_RDONLY)) > 0)
			{
				if ((read (fd, call, 7) == 7) && (read (fd, bid, 13) == 13))
				{
					strcpy (bul.bid, bid);
					if ((dirblk.ff_fsize == 20) || (deja_recu (&bul, 1, &test)))
					{
						unlink (file);
					}
					else
					{
						struct tm *tm;
						time_t temps = time (NULL);
						int jour;
						int delai;

						jour = dirblk.ff_fdate & 0x1f;
						tm = gmtime (&temps);
						delai = tm->tm_mday - jour;
						if (jour < 0)
							jour += 31;
						if (jour > 15)
							unlink (file);
						else
							add_part (atol (dirblk.ff_name), call, dirblk.ff_fsize - 20, bid);
					}
				}
				close (fd);
			}
		}
	}
	while (findnext (&dirblk) == 0);

	ff ();
}

/* Teste si les messages sont en partie recus pour la BBS concernee */
void part_recu (bullist * fb_mess, int nb, int *t_res)
{
	Encours *eptr;
	int i, pos;

	df ("part_recu", 5);

	for (i = 0; i < nb; i++)
	{
		eptr = tete_encours;
		while (eptr)
		{
			for (pos = 0; pos < MAX_ENCOURS; pos++)
			{

				if (*eptr->enc[pos].bbs == '\0')
					continue;

				if ((strcmp (pvoie->sta.indicatif.call, eptr->enc[pos].bbs) == 0) &&
					(strcmp (fb_mess[i].bid, eptr->enc[pos].bid) == 0))
				{
					if ((t_res[i] == 1) || (t_res[i] == 4))		/* NO ou REJ */
					{
						del_part (voiecur, fb_mess[i].bid);
					}
					else if (eptr->enc[pos].taille)
					{
						if (t_res[i] != 2)	/* OK ou HELD */
						{
							t_res[i] = 3;
							fb_mess[i].taille = eptr->enc[pos].taille;
						}
					}
					else
					{
						del_part (voiecur, fb_mess[i].bid);
					}
				}
			}
			eptr = eptr->next;
		}
	}
	ff ();
}


void print_part (void)
{
	int pos;
	int ok = 0;
	char stemp[80];
	Encours *eptr = tete_encours;

	df ("print_part", 0);

	/* Cherche l'entree */
	while (eptr)
	{
		for (pos = 0; pos < MAX_ENCOURS; pos++)
		{
			if (*eptr->enc[pos].bbs == '\0')
				continue;
			sprintf (stemp, "BBS %-6s - BID %-12s (%ld bytes received)",
					 eptr->enc[pos].bbs, eptr->enc[pos].bid,
					 eptr->enc[pos].taille);
			printf ("BBS %-6s - BID %-12s (%ld bytes received)\n",
					 eptr->enc[pos].bbs, eptr->enc[pos].bid,
					 eptr->enc[pos].taille);
			outln (stemp, strlen (stemp));
			ok = 1;
		}
		eptr = eptr->next;
	}

	if (!ok)
		outln ("None", 4);

	ff ();
}
