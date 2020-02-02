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
 *    MODULE FORWARDING OVERLAY 1
 */

#include <serv.h>
#define MAXNOT 10

static char *BID_STRING = "%ld_%s";
static char *ALT_STRING = "%ld-%s";

typedef struct pnot
{
	int type[MAXNOT];
	char texte[MAXNOT][41];
	struct pnot *suite;
}
snot;


static int is_not (snot *, int, char *);
static int test_forward_nts (int);

static void libere_not (snot *);

static int nb_bbs;

long next_num (void)
{
	char type;
	unsigned record;
	unsigned num_indic;

	if (ptmes->numero == 0L)
	{
		ptmes->numero = ++nomess;
		if ((ptmes->numero % 0x10000L) == 0)
			ptmes->numero = ++nomess;

		pvoie->mess_recu = 0;
		insnoeud (ptmes->desti, &num_indic);

		type = ptmes->type;
		ptmes->type = '\0';

		ouvre_dir ();
		write_dir (0, ptmes);

		record = length_dir ();

		write_dir (record, ptmes);
		ferme_dir ();

		ptmes->type = type;
		insmess (record, num_indic, ptmes->numero);
	}
	return (ptmes->numero);
}


static int host_bbs (bullist * pbull)
{
	int retour = 0;
	ind_noeud *pnoeud;
	unsigned no_indic;
	FILE *fptr;
	info frec;

	if ((is_serv (pbull->desti)) || (strcmp (pbull->desti, "SYSOP") == 0) || (!find (pbull->desti)))
		return (2);

	var_cpy (0, "\0");
	if ((retour = route_wp_home (pbull)) != 0)
	{
		route_wp_hier (pbull);
		var_cpy (0, "WP");
	}
	else
	{
		pnoeud = insnoeud (pbull->desti, &no_indic);
		if (pnoeud->coord != 0xffff)
		{
			fptr = ouvre_nomenc ();
			fseek (fptr, (long) pnoeud->coord * sizeof (frec), 0);
			fread (&frec, sizeof (info), 1, fptr);
			ferme (fptr, 39);
			if (*frec.home)
			{
				strcpy (pbull->bbsv, frec.home);
				retour = 1;
				var_cpy (0, "H");
			}
		}
	}
	return (retour);
}

int chercher_voie (nom)
	 char *nom;
{
	int i;

	/* en sortie : -1 si nom n'est pas connecte, numero de voie si connecte */

	for (i = 1; i < NBVOIES - 1; i++)
		if ((svoie[i]->sta.connect) && (indcmp (svoie[i]->sta.indicatif.call, nom)))
			return (i);
	return (-1);
}


void make_bid (void)
{
	char ligne[80];
	long num;
	char *bid_str = BID_STRING;

	if (std_header & 1024)
		bid_str = ALT_STRING;

	if (!isalnum (*ptmes->bid))
	{
		if ((*pvoie->mess_bid) && ((std_header & 64) == 0))
			strn_cpy (12, ligne, pvoie->mess_bid);
		else if (pvoie->mess_num > 0L)
		{
			num = (pvoie->mess_num % 0x10000L);
			sprintf (ligne, bid_str, num, bbs_via (pvoie->mess_home));
		}
		else
		{
			num = (ptmes->numero % 0x10000L);
			sprintf (ligne, bid_str, num, mycall);
		}
		strn_cpy (12, ptmes->bid, ligne);
	}
}

int fwd_get (char *buffer)
{
	if (EMS_FWD_OK ())
	{
		return (read_exms_string (FORWARD, buffer));
	}
	if (*fwd_scan == '\032')
		return (0);
	while ((*buffer++ = *fwd_scan++) != '\0');
	return (1);
}

#define BBS_LEN 6

static void in_bbs (char *bbs)
{
	int i;
	char *ptr = data;

	for (i = 0; i < nb_bbs; i++)
	{
		if (strcmp (bbs, ptr) == 0)
			return;
		ptr += (BBS_LEN + 1);
	}

	strn_cpy (BBS_LEN, ptr, bbs);
	++nb_bbs;
}

static void out_bbs (void)
{
	int i;
	char *ptr = data;

	if (nb_bbs == 0)
		out (" ??????  ", 8);
	else
	{
		qsort ((void *) data, nb_bbs, BBS_LEN + 1, (int (*)(const void *, const void *)) strcmp);

		for (i = 0; i < nb_bbs; i++)
		{
			out (" ", 1);
			out (ptr, strlen (ptr));
			ptr += (BBS_LEN + 1);
		}
		out ("  ", 2);
	}
}


int already_forw (char *masque, int nobbs)
{
	return (masque[(nobbs - 1) / 8] & (1 << ((nobbs - 1) % 8)));
}


void set_bit_fwd (char *masque, int nobbs)
{
	if (nobbs)
		masque[(nobbs - 1) / 8] |= (1 << ((nobbs - 1) % 8));
}


void clr_bit_fwd (char *masque, int nobbs)
{
	if (nobbs)
		masque[(nobbs - 1) / 8] &= (~(1 << ((nobbs - 1) % 8)));
}


static int is_not (snot * t_not, int type, char *texte)
{
	int nb;
	char *ptr;

	while (t_not)
	{
		for (nb = 0; nb < MAXNOT; nb++)
		{
			ptr = t_not->texte[nb];
			if (*ptr == '\0')
				break;
			if ((t_not->type[nb] == type) && (strmatch (texte, ptr)))
			{
				return (1);
			}
		}
		t_not = t_not->suite;
	}

	return (0);
}


static void libere_not (snot * t_not)
{
	snot *p_not;

	/* Libere la listes NOT */
	while (t_not)
	{
		p_not = t_not->suite;
		m_libere (t_not, sizeof (snot));
		t_not = p_not;
	}
}

void libere_route (int voie)
{
	Svoie *vptr = svoie[voie];
	Route *r_ptr;

	while ((r_ptr = vptr->r_tete) != NULL)
	{
		vptr->r_tete = r_ptr->suite;
		m_libere (r_ptr, sizeof (Route));
	}
}



int test_forward (int valid)
{
	char *pcom;
	char ligne[80], combuf[80];
	int nobbs = 0, type, nb, i, lig = 0;
	long num;
	int route = 0, fwd_ok = 0;
	int not_b = 0, not_g = 0, not_h = 0;
	int ping_pong = 0;
	int route_bbs = 0;

	char *bbs_v;
	snot *t_not, *p_not;

	char *bid_str = BID_STRING;

	if (ptmes->type == 'T')
		return (test_forward_nts (valid));

	if (std_header & 1024)
		bid_str = ALT_STRING;

	t_not = p_not = NULL;
	nb = MAXNOT;

	bbs_v = bbs_via (ptmes->bbsv);

	nb_bbs = 0;

	rewind_fwd ();

	if ((ptmes->type != 'B') && (strcmp (ptmes->desti, "SYSOP") != 0))
		pvoie->messdate = 0L;	/* TEST SUR LES BULLETINS */

	if ((pvoie->messdate == 0L) || ((pvoie->messdate) && ((time (NULL) - pvoie->messdate) < (86400L * nb_jour_val))))
	{
		while (fwd_get (combuf))
		{
			pcom = combuf;
			++lig;
			switch (type = *pcom++)
			{

			case '!':			/* Not ! */
				if (!route)
					break;
				switch (type = *pcom++)
				{
				case 'B':
				case 'F':
				case 'G':
				case 'H':
					if (nb == MAXNOT)
					{
						if (p_not)
						{
							p_not->suite = (snot *) m_alloue (sizeof (snot));
							p_not = p_not->suite;
						}
						else
						{
							t_not = p_not = (snot *) m_alloue (sizeof (snot));
						}
						for (i = 0; i < MAXNOT; i++)
							*(p_not->texte[i]) = '\0';
						p_not->suite = NULL;
						nb = 0;
					}
					p_not->type[nb] = type;
					strn_cpy (40, p_not->texte[nb], pcom);
					++nb;
					break;
				}
				break;

			case 'A':			/* recuperer le No et le nom de la BBS destinataire */
				route = 0;
				route_bbs = 0;
				nobbs = num_bbs (pcom);
				if (is_route (pcom))
				{
					set_bit_fwd (ptmes->forw, nobbs);
					break;
				}
				strcpy (ligne, pcom);
				if (already_forw (ptmes->forw, nobbs))
					break;
				route = 1;
				libere_not (t_not);
				t_not = p_not = NULL;
				nb = MAXNOT;
				not_b = not_g = not_h = 0;
				break;

			case 'G':			/* indication de groupe */
				if (!route)
					break;
				route_bbs = 0;
				if ((*bbs_v) && (ptmes->status == '$'))
				{
					if ((not_g) || (is_not (t_not, type, bbs_v)))
					{
						not_g = 1;
					}
					else if (strmatch (bbs_v, pcom))
					{
						if (indcmp (pvoie->sta.indicatif.call, ligne) == 0)
						{
							if (!(FOR (pvoie->mode)))
							{
								in_bbs (ligne);
							}
							set_bit_fwd (ptmes->fbbs, nobbs);
							fwd_ok = 1;
						}
						else
						{
							set_bit_fwd (ptmes->forw, nobbs);
						}
					}
				}
				break;

			case 'H':			/* indication de Hierarchie */
				if (!route)
					break;
				route_bbs = 0;
				if ((*ptmes->bbsv) && (ptmes->status != '$'))
				{
					if ((not_h) || (is_not (t_not, type, ptmes->bbsv)))
					{
						not_h = 1;
					}
					else if (strmatch (ptmes->bbsv, pcom))
					{
						if (((pvoie->mode & F_FOR) == 0) ||
							(indcmp (pvoie->sta.indicatif.call, ligne) == 0))
						{
							set_bit_fwd (ptmes->fbbs, nobbs);
							fwd_ok = 1;
							if (!(FOR (pvoie->mode)))
							{
								in_bbs (ligne);
							}
						}
						else
						{		/* Ping Pong ! */
							/* dde_warning(W_PPG); */
							ping_pong = 1;
						}
					}
				}
				break;

			case 'B':			/* indication de BBS */
				if (!route)
					break;
				route_bbs = 1;
				if ((*bbs_v) && (ptmes->status != '$'))
				{
					if ((not_b) || (is_not (t_not, type, bbs_v)))
					{
						not_b = 1;
					}
					else if (strmatch (bbs_v, pcom))
					{
						if (((pvoie->mode & F_FOR) == 0) ||
							(indcmp (pvoie->sta.indicatif.call, ligne) == 0))
						{
							set_bit_fwd (ptmes->fbbs, nobbs);
							fwd_ok = 1;
							if (!(FOR (pvoie->mode)))
							{
								in_bbs (ligne);
							}
						}
						else
						{		/* Ping Pong ! */
							/* dde_warning(W_PPG); */
							ping_pong = 1;
						}
					}
				}
				break;

			case 'F':			/* indication de routage */
				if (!route)
					break;
				if (is_not (t_not, type, pcom))
					break;
				if ((*bbs_v == '\0') && (find (ptmes->desti)))
				{
					/* Redirection d'un destinataire prive */
					if ((!not_b) && (strmatch (ptmes->desti, pcom)))
					{
						if (((pvoie->mode & F_FOR) == 0) ||
							(indcmp (pvoie->sta.indicatif.call, ligne) == 0))
						{
							set_bit_fwd (ptmes->fbbs, nobbs);
							fwd_ok = 1;
							if (!(FOR (pvoie->mode)))
							{
								in_bbs (ligne);
							}
						}
						else
						{		/* Ping Pong ! */
							/* dde_warning(W_PPG); */
							ping_pong = 1;
						}
					}
				}
				else if (route_bbs == 0)
				{
					/* Routage sur le destinataire */
					if ((!not_b) && (strmatch (ptmes->desti, pcom)))
					{
						if (indcmp (pvoie->sta.indicatif.call, ligne) == 0)
						{
							set_bit_fwd (ptmes->fbbs, nobbs);
							fwd_ok = 1;
							if (!(FOR (pvoie->mode)))
							{
								in_bbs (ligne);
							}
						}
					}
				}
				break;

			default:
				break;
			}
		}
	}
	else
	{
		ptmes->status = 'X';
	}
	if (*(ptmes->bid) == '\0')
		strcpy (ptmes->bid, " ");

	if (!FOR (pvoie->mode))
	{
		if (fwd_ok)
		{
			texte (T_MBL + 40);
			out_bbs ();
		}
	}

	if ((valid) && (*ptmes->bid))
	{
		if (*ptmes->bid == ' ')
		{
			if ((*pvoie->mess_bid) && ((std_header & 64) == 0))
				strn_cpy (12, ptmes->bid, pvoie->mess_bid);
			else if (pvoie->mess_num > 0L)
			{
				num = (pvoie->mess_num % 0x10000L);
				sprintf (ligne, bid_str, num, bbs_via (pvoie->mess_home));
			}
			else
			{
				num = (ptmes->numero % 0x10000L);
				sprintf (ligne, bid_str, num, mycall);
			}
			strn_cpy (12, ptmes->bid, ligne);
		}
		if (!FOR (pvoie->mode))
		{
			if (ptmes->type == 'P')
				out ("Mid: $R  ", 9);
			else
				out ("Bid: $R  ", 9);
		}
		if (valid == 1)
			w_bid ();
	}

	if ((ping_pong) && (!fwd_ok))
		dde_warning (W_PPG);

	libere_not (t_not);
	libere_route (voiecur);
	return (fwd_ok);
}


static int test_forward_nts (int valid)
{
	char *pcom;
	char desti[80], ligne[80], combuf[80];

/*  char    wtexte[256] ; */
	int nobbs = 0, type, nb, i, lig = 0;
	long num;
	int route = 0, fwd_ok = 0;
	int not_z = 0;
	int ping_pong = 0;
	snot *t_not, *p_not;
	char *bid_str = BID_STRING;

	if (std_header & 1024)
		bid_str = ALT_STRING;

	t_not = p_not = NULL;
	nb = MAXNOT;

	strcpy (desti, ptmes->desti);

	rewind_fwd ();

	if ((pvoie->messdate == 0L) || ((pvoie->messdate) && ((time (NULL) - pvoie->messdate) < (86400L * nb_jour_val))))
	{
		while (fwd_get (combuf))
		{
			pcom = combuf;
			++lig;
			switch (type = *pcom++)
			{

			case '!':			/* Not ! */
				if (!route)
					break;
				switch (type = *pcom++)
				{
				case 'Z':
					if (nb == MAXNOT)
					{
						if (p_not)
						{
							p_not->suite = (snot *) m_alloue (sizeof (snot));
							p_not = p_not->suite;
						}
						else
						{
							t_not = p_not = (snot *) m_alloue (sizeof (snot));
						}
						for (i = 0; i < MAXNOT; i++)
							*(p_not->texte[i]) = '\0';
						p_not->suite = NULL;
						nb = 0;
					}
					p_not->type[nb] = type;
					strn_cpy (40, p_not->texte[nb], pcom);
					++nb;
					break;
				}
				break;

			case 'A':			/* recuperer le No et le nom de la BBS destinataire */
				route = 0;
				nobbs = num_bbs (pcom);
				if (is_route (pcom))
				{
					set_bit_fwd (ptmes->forw, nobbs);
					break;
				}
				strcpy (ligne, pcom);
				if (already_forw (ptmes->forw, nobbs))
					break;
				route = 1;
				libere_not (t_not);
				t_not = p_not = NULL;
				nb = MAXNOT;
				not_z = 0;
				break;

			case 'Z':			/* indication de route */
				if (!route)
					break;
				if ((not_z) || (is_not (t_not, type, desti)))
				{
					not_z = 1;
				}
				else if (strmatch (desti, pcom))
				{
					if (((pvoie->mode & F_FOR) == 0) ||
						(indcmp (pvoie->sta.indicatif.call, ligne) == 0))
					{
						set_bit_fwd (ptmes->fbbs, nobbs);
						fwd_ok = 1;
						if (!(FOR (pvoie->mode)))
						{
							in_bbs (ligne);
						}
					}
					else
					{			/* Ping Pong ! */
						ping_pong = 1;
					}
				}
				break;

			default:
				break;
			}
		}
	}
	else
	{
		ptmes->status = 'X';
	}

	if (!FOR (pvoie->mode))
	{
		if (fwd_ok)
		{
			texte (T_MBL + 40);
			out_bbs ();
		}
	}

	if (*(ptmes->bid) == '\0')
		strcpy (ptmes->bid, " ");
	if ((valid) && (!FOR (pvoie->mode)))
		out ("  ", 2);

	if ((valid) && (*ptmes->bid))
	{
		if (*ptmes->bid == ' ')
		{
			if ((*pvoie->mess_bid) && ((std_header & 64) == 0))
				strn_cpy (12, ptmes->bid, pvoie->mess_bid);
			else if (pvoie->mess_num > 0L)
			{
				num = (pvoie->mess_num % 0x10000L);
				sprintf (ligne, bid_str, num, bbs_via (pvoie->mess_home));
			}
			else
			{
				num = (ptmes->numero % 0x10000L);
				sprintf (ligne, bid_str, num, mycall);
			}
			strn_cpy (12, ptmes->bid, ligne);
		}
		if (!FOR (pvoie->mode))
		{
			if (ptmes->type == 'P')
				out ("Mid: $R  ", 9);
			else
				out ("Bid: $R  ", 9);
		}
		if (valid != 2)
			w_bid ();
	}

	if ((ping_pong) && (!fwd_ok))
		dde_warning (W_PPG);

	libere_not (t_not);
	libere_route (voiecur);
	return (fwd_ok);
}


int reacheminement (void)
{
/*	static int boucle = 0;*/
	int achemine_trouve;
	int nts_trouve;
	int route_bbs;
	int type = 0;
	int nb;
	int i;
	char *pcom;
	char combuf[80];
	char ligne[257];
	char desti[10];
	char *bbs_v;
	int not_b = 0, not_g = 0, not_h = 0, not_z = 0;
	snot *t_not, *p_not;

	df ("reacheminement", 0);
	pvoie->warning = 0;
	route_bbs = 0;

	t_not = p_not = NULL;
	nb = MAXNOT;

	strcpy (desti, ptmes->desti);

	bbs_v = bbs_via (ptmes->bbsv);
	if (hiecmp (mypath, ptmes->bbsv))
	{
		*ptmes->bbsv = '\0';
		ff ();
		return (1);
	}

	/* S'il y a une route est specifiee, l'acheminement existe-t-il ? */
	achemine_trouve = (*ptmes->bbsv == '\0');

	nts_trouve = (ptmes->type != 'T');

	if (!nts_trouve)
	{
		/* L'acheminement est trouve, routage sur le desti ... */
		achemine_trouve = TRUE;
	}
	else if (achemine_trouve)
	{
		switch (host_bbs (ptmes))
		{
		case 2:
			break;
		case 1:
			if ((!FOR (pvoie->mode)) && (ptmes->type != 'B') && (*ptmes->bbsv))
				texte (T_MBL + 41);
			/* l'acheminement existe-t-il pour le home BBS ? */
			achemine_trouve = 0;
			break;
		case 0:
			if ((!FOR (pvoie->mode)) && (EMS_WPG_OK ()) && (find (ptmes->desti)) && (*ptmes->bbsv == '\0'))
				texte (T_MBL + 56);
		}
	}

	if (hiecmp (mypath, ptmes->bbsv))
	{
		*ptmes->bbsv = '\0';
		ff ();
		return (1);
	}

	if (cherche_route (ptmes))
	{
		if (!FOR (pvoie->mode))
		{
			texte (T_MBL + 41);
		}
	}

	bbs_v = bbs_via (ptmes->bbsv);

	rewind_fwd ();

	while (fwd_get (combuf))
	{
		pcom = combuf;
		switch (*pcom++)
		{

		case '!':				/* Not ! */
			switch (type = *pcom++)
			{
			case 'B':
			case 'G':
			case 'H':
			case 'Z':
			case 'F':
				if (nb == MAXNOT)
				{
					if (p_not)
					{
						p_not->suite = (snot *) m_alloue (sizeof (snot));
						p_not = p_not->suite;
					}
					else
					{
						t_not = p_not = (snot *) m_alloue (sizeof (snot));
					}
					for (i = 0; i < MAXNOT; i++)
						*(p_not->texte[i]) = '\0';
					p_not->suite = NULL;
					nb = 0;
				}
				p_not->type[nb] = type;
				strn_cpy (40, p_not->texte[nb], pcom);
				++nb;
				break;
			}
			break;

		case 'A':
			route_bbs = 0;
			libere_not (t_not);
			t_not = p_not = NULL;
			nb = MAXNOT;
			not_b = not_g = not_h = not_z = 0;

		case 'B':				/* recuperer le nom de la BBS destinataire */
			route_bbs = 1;
			strcpy (ligne, pcom);
			if ((not_b) || (is_not (t_not, type, bbs_v)))
			{
				not_b = 1;
				break;
			}
			if ((ptmes->status != '$') && (!achemine_trouve) && (strmatch (bbs_v, pcom)))
			{
				achemine_trouve = TRUE;
			}
			break;

		case 'H':				/* test des hierarchies */
			route_bbs = 0;
			if ((not_h) || (is_not (t_not, type, bbs_v)))
			{
				not_h = 1;
				break;
			}
			if ((ptmes->status != '$') && (!achemine_trouve) && (strmatch (ptmes->bbsv, pcom)))
			{
				achemine_trouve = TRUE;
			}
			break;

		case 'G':				/* test du groupe */
			route_bbs = 0;
			if ((not_g) || (is_not (t_not, type, bbs_v)))
			{
				not_g = 1;
				break;
			}
			if ((ptmes->status == '$') && (!achemine_trouve) && (strmatch (bbs_v, pcom)))
			{
				achemine_trouve = TRUE;
			}
			break;

		case 'Z':				/* Route NTS */
			route_bbs = 1;
			if ((not_z) || (is_not (t_not, type, desti)))
			{
				not_z = 1;
				break;
			}
			if ((ptmes->type == 'T') && (!nts_trouve) && (strmatch (desti, pcom)))
				nts_trouve = TRUE;
			break;

		case 'F':				/* chercher les messages de l'OM associe a la BBS */
			if (is_not (t_not, type, pcom))
				break;
			if ((*(ptmes->bbsv) == '\0') && (strmatch (ptmes->desti, pcom)))
			{
				/* (indcmp(ptmes->desti, pcom))) { */
				if ((route_bbs) && (find (ptmes->desti)))
				{
					strcpy (ptmes->bbsv, ligne);
				}
				achemine_trouve = TRUE;
				if (cherche_route (ptmes))
				{
					if ((!FOR (pvoie->mode)) && (ptmes->type != 'B'))
					{
						texte (T_MBL + 41);
					}
				}
			}
			break;

		default:
			break;

		}
	}

	libere_not (t_not);

	if (!achemine_trouve)
	{
		dde_warning (W_ROU);
		if (!FOR (pvoie->mode))
			texte (T_MBL + 49);
	}
	if (!nts_trouve)
	{
		dde_warning (W_NTS);
		if (!FOR (pvoie->mode))
		{
			strcpy (ligne, ptmes->bbsv);
			strcpy (ptmes->bbsv, desti);
			texte (T_MBL + 49);
			strcpy (ptmes->bbsv, ligne);
		}
	}

	ff ();
/*	boucle = 0;*/
	return (1);
}
