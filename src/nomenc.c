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
 *  MODULE NOMENC.C
 */

#include <serv.h>

static int ecrire_indic (tri *, int);

static void affich_nom (info *);
static void menu_nomenclature (void);
static void fin_liste_indic (void);
static void parcours_liste (void);

static void affich_indic (info * pinfo)
{
	int i = 0;
	char s[40];

	sprintf (varx[0], "%-6s-%d", pinfo->indic.call, pinfo->indic.num);
	texte (T_NOM + 3);
	if (*(pinfo->relai[i].call))
	{
		out ("via ", 4);
		while ((i < 8) && *(pinfo->relai[i].call))
		{
			if (i == 4)
				out ("$W    ", 6);
			sprintf (s, " %-6s-%d", pinfo->relai[i].call, pinfo->relai[i].num);
			out (s, strlen (s));
			++i;
		}
		out ("$W", 2);
	}
}


static void affich_coord (info * pinfo)
{
	affich_indic (pinfo);
	affich_nom (pinfo);
	ptmes->date = pinfo->hcon;
	texte (T_NOM + 4);
}


static void affich_nom (info * pinfo)
{
	var_cpy (0, pinfo->nom);
	var_cpy (1, pinfo->prenom);
	var_cpy (2, pinfo->adres);
	var_cpy (3, pinfo->ville);
	var_cpy (4, pinfo->qra);
	var_cpy (5, pinfo->teld);
	var_cpy (6, pinfo->telp);
	var_cpy (7, pinfo->home);
	var_cpy (8, pinfo->zip);
	texte (T_NOM + 5);
}


static void rech_nomenc (void)
{
	unsigned r;
	FILE *fptr;
	info buf_info;

	*varx[0] = '\0';
	if (*indd == '\0')
	{
		texte (T_NOM + 3);
	}
	else
	{
		if (find (sup_ln (indd)))
		{
			if ((r = chercoord (indd)) == 0xffff)
			{
				var_cpy (0, indd);
				texte (T_NOM + 13);
			}
			else
			{
				fptr = ouvre_nomenc ();
				fseek (fptr, (long) r * sizeof (info), 0);
				fread ((char *) &buf_info, sizeof (info), 1, fptr);
				ferme (fptr, 11);
				affich_coord (&buf_info);
			}
			maj_niv (5, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else
		{
			var_cpy (0, indd);
			texte (T_NOM + 13);
			maj_niv (5, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
	}
}

static void lit_coord (unsigned record, info * frec)
{
	FILE *fptr;

	fptr = ouvre_nomenc ();
	fseek (fptr, (long) record * sizeof (info), 0);
	fread (frec, sizeof (info), 1, fptr);
	ferme (fptr, 39);
}

static void ecrit_coord (unsigned record, info * frec)
{
	FILE *fptr;

	if (record == 0xffff)
		dump_core ();
	fptr = ouvre_nomenc ();
	fseek (fptr, (long) record * sizeof (info), 0);
	fwrite (frec, sizeof (info), 1, fptr);
	ferme (fptr, 39);
}

static void modif_nomenc (void)
{
	unsigned num_indic;
	int err = 0;
	info frec;
	char c;
	char s[80];
	char temp[80];
	char *ptr;
	char *scan;

	if (read_only ())
	{
		prompt (pvoie->finf.flags, pvoie->niv1);
		maj_niv (5, 0, 0);
		return;
	}

	if (pvoie->niv3 > 1)
		lit_coord (pvoie->emis->coord, &frec);

	switch (pvoie->niv3)
	{
	case 0:
		incindd ();
		if ((*indd) && (droits (COSYSOP)))
		{
			if (find (sup_ln (indd)))
			{
				pvoie->emis = insnoeud (indd, &num_indic);
				if (pvoie->emis->coord == 0xffff)
					err = 1;
			}
			else
				err = 1;
			if (err)
			{
				var_cpy (0, indd);
				texte (T_NOM + 13);
				prompt (pvoie->finf.flags, pvoie->niv1);
				maj_niv (5, 0, 0);
				return;
			}
		}
		else
		{
			pvoie->emis = pvoie->ncur;
		}
		lit_coord (pvoie->emis->coord, &frec);
		affich_nom (&frec);
		texte (T_QST + 1);
		ch_niv3 (1);
		break;
	case 1:
		c = toupper (*indd);
		if (c == Oui)
		{
			/* majinfo(voiecur, 1) ; */
			texte (T_MBL + 9);
			ch_niv3 (2);
		}
		else if ((c == 'A') || (c == 'F') || (c == Non))
		{
			maj_niv (5, 0, 0);
			incindd ();
			menu_nomenclature ();
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_QST + 1);
		}
		break;
	case 2:
		if (ISGRAPH (*indd))
		{
			epure (frec.prenom, 12);
			scan = frec.prenom;
			while (*scan)
			{
				if (*scan == ' ')
					*scan = '-';
				++scan;
			}
		}
		texte (T_NOM + 6);
		ch_niv3 (3);
		break;
	case 3:
		if (ISGRAPH (*indd))
			epure (frec.nom, 17);
		texte (T_NOM + 7);
		ch_niv3 (4);
		break;
	case 4:
		if (ISGRAPH (*indd))
			epure (frec.adres, 60);
		texte (T_MBL + 54);
		ch_niv3 (5);
		break;
	case 5:
		if (ISGRAPH (*indd))
		{
			epure (frec.zip, 8);
			scan = frec.zip;
			while (*scan)
			{
				if (*scan == ' ')
					*scan = '-';
				++scan;
			}
		}
		texte (T_NOM + 8);
		ch_niv3 (6);
		break;
	case 6:
		if (ISGRAPH (*indd))
			epure (frec.ville, 30);
		texte (T_NOM + 9);
		ch_niv3 (7);
		break;
	case 7:
		if (ISGRAPH (*indd))
			epure (frec.teld, 12);
		texte (T_NOM + 10);
		ch_niv3 (8);
		break;
	case 8:
		if (ISGRAPH (*indd))
			epure (frec.telp, 12);
		texte (T_MBL + 53);
		ch_niv3 (9);
		break;
	case 9:
		if (ISGRAPH (*indd))
		{
			strupr (epure (s, 40));

			ptr = strchr (s, '.');
			if (ptr)
				*ptr = '\0';

			ptr = strchr (s, '-');
			if (ptr)
			{
				*ptr = '\0';
			}
			strcpy (temp, s);
			if (find (temp))
			{
				strcpy (frec.home, s);
				texte (T_NOM + 11);
				ch_niv3 (10);
			}
			else
			{
				texte (T_ERR + 7);
				texte (T_MBL + 53);
			}
		}
		else
		{
			texte (T_NOM + 11);
			ch_niv3 (10);
		}
		break;
	case 10:
		if (ISGRAPH (*indd))
		{
			strupr (epure (s, 6));
			if (tstqra (s))
			{
				strcpy (frec.qra, s);
				ecrit_coord (pvoie->emis->coord, &frec);
			}
			else
			{
				texte (T_NOM + 11);
				break;
			}
		}
		user_wp (&frec);
		texte (T_QST + 2);
		prompt (pvoie->finf.flags, pvoie->niv1);
		maj_niv (5, 0, 0);
		break;
	default:
		fbb_error (ERR_NIVEAU, "MODIF-NOMENC", pvoie->niv3);
		break;
	}

	if (pvoie->niv3 > 2)
	{
		ecrit_coord (pvoie->emis->coord, &frec);
		if (pvoie->emis == pvoie->ncur)
			pvoie->finf = frec;
	}
}


/*
 *   LISTE DES INDICATIFS
 */

static void fin_liste_indic (void)
{
	itoa (pvoie->temp1, varx[0], 10);
	texte (T_NOM + 12);
}


int ecrire_indic (tri * inf_ptr, int nb)
/*
 * en sortie : vrai si l'indicatif a ete ecrit
 *             faux sinon
 */
{
	char s[81];
	bloc_indic *temp;
	int ind, val;
	int i, j, pos = 0;

	for (i = 0; i < nb; i++)
	{
		ind = 0;
		val = inf_ptr->pos;
		if (val == 0)
			break;

		temp = racine;
		while ((ind + T_BLOC_INFO) <= val)
		{
			ind += T_BLOC_INFO;
			temp = temp->suiv;
		}
		j = val - ind;
		sprintf (s, "%-6s ", temp->st_ind[j].indic);
		out (s, strlen (s));
		if (++pos == 11)
		{
			pos = 0;
			out ("\n", 1);
		}
		++inf_ptr;
	}
	return (TRUE);
}


static void parcours_liste (void)
{
	bloc_indic *bptr;
	tri *inf_ptr = NULL, *pos_ptr = NULL;
	unsigned offset;
	unsigned num_indic;
	int pass;
	int nb = 0;

	tri t_ptr;

	pvoie->temp1 = 0;

	for (pass = 0; pass <= 1; ++pass)
	{
		bptr = racine;
		offset = num_indic = 0;

		if (pass == 0)
			pos_ptr = &t_ptr;

		while (bptr)
		{
			if (*(bptr->st_ind[offset].indic) == '\0')
				break;
			if (bptr->st_ind[offset].coord != 0xffff)
			{
				if (bptr->st_ind[offset].val)
				{
					if (strmatch (bptr->st_ind[offset].indic, indd))
					{
						if (pass)
						{
							pos_ptr->pos = num_indic;
							n_cpy (6, pos_ptr->ind, bptr->st_ind[offset].indic);
							++pos_ptr;
						}
						++nb;
					}
				}
			}
			if (++offset == T_BLOC_INFO)
			{
				offset = 0;
				bptr = bptr->suiv;
			}
			++num_indic;
		}

		if (nb == 0)
			break;


		if (pass == 0)
		{
			pvoie->temp1 = nb;
			pos_ptr = inf_ptr = (tri *) m_alloue (nb * sizeof (tri));
			nb = 0;
		}
	}

	if (nb)
		qsort ((void *) inf_ptr, (size_t) nb, (size_t) sizeof (tri), (int (*)(const void *, const void *)) strcmp);

	ecrire_indic (inf_ptr, nb);

	if (nb)
		m_libere (inf_ptr, nb * sizeof (tri));
}


void liste_indic (void)
{
	pvoie->temp2 = pvoie->temp1 = 0;
	strupr (sup_ln (indd));
	if (*indd == '\0')
	{
		*indd = '*';
		*(indd + 1) = '\0';
	}
	parcours_liste ();
	fin_liste_indic ();
	retour_menu (pvoie->niv1);
}

void saisie_infos (void)
{
	int change = 0;

	char s[80];
	char temp[80];
	char *ptr;
	char *scan;

	switch (pvoie->niv3)
	{

	case 0:
		break;

	case 1:
		if (ISGRAPH (*indd))
		{
			epure (pvoie->finf.prenom, 12);
			scan = pvoie->finf.prenom;
			while (*scan)
			{
				if (*scan == ' ')
					*scan = '-';
				++scan;
			}
			change = 1;
		}
		break;

	case 2:
		if (ISGRAPH (*indd))
		{
			epure (pvoie->finf.ville, 30);
			change = 1;
		}
		break;

	case 3:
		if (ISGRAPH (*indd))
		{
			strupr (epure (s, 40));

			/* Seul l'indicatif est enregistre */
			ptr = strchr (s, '.');
			if (ptr)
				*ptr = '\0';

			ptr = strchr (s, '-');
			if (ptr)
			{
				*ptr = '\0';
			}

			strcpy (temp, s);
			if (find (temp))
			{
				strcpy (pvoie->finf.home, extend_bbs (s));
			}
			else
			{
				texte (T_ERR + 7);
			}
			change = 1;
		}
		break;

	case 4:
		if (ISGRAPH (*indd))
		{
			epure (pvoie->finf.zip, 8);
			scan = pvoie->finf.zip;
			while (*scan)
			{
				if (*scan == ' ')
					*scan = '-';
				++scan;
			}
			change = 1;
		}
		break;

	default:
		fbb_error (ERR_NIVEAU, "SAISIE-INFO", pvoie->niv3);
		break;
	}

	if (*pvoie->finf.prenom == '\0')
	{
		texte (T_MBL + 9);
		ch_niv3 (1);
	}
	else if (*pvoie->finf.ville == '\0')
	{
		texte (T_NOM + 8);
		ch_niv3 (2);
	}
	else if (*pvoie->finf.home == '\0')
	{
		texte (T_MBL + 53);
		ch_niv3 (3);
	}
	else if (*pvoie->finf.zip == '\0')
	{
		texte (T_MBL + 54);
		ch_niv3 (4);
	}
	else
	{
		if (change)
		{
			if (pvoie->ncur)
			{
				ecrit_coord (pvoie->ncur->coord, &pvoie->finf);
				user_wp (&pvoie->finf);
			}
			texte (T_QST + 2);
		}
		finentete ();
		ptr = pvoie->sta.indicatif.call;
		if (pvoie->sniv1 == N_CONF)
		{
			pvoie->niv1 = pvoie->sniv1;
			pvoie->niv2 = pvoie->sniv2;
			pvoie->niv3 = pvoie->sniv3;
			pvoie->conf = 1;
		}
		else
		{
			maj_niv (N_MBL, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
	}
}

/*
 *  MENUS - PREMIER NIVEAU NOMENCLATURE
 */

static void menu_nomenclature (void)
{
	int error = 0;
	char com[80];

	limite_commande ();
	sup_ln (indd);
	while (*indd && (!ISGRAPH (*indd)))
		indd++;
	strn_cpy (70, com, indd);

	switch (toupper (*indd))
	{
	case 'N':
		maj_niv (5, 1, 0);
		modif_nomenc ();
		break;
	case 'R':
		maj_niv (5, 3, 0);
		incindd ();
		rech_nomenc ();
		break;
	case 'I':
		maj_niv (5, 4, 0);
		incindd ();
		liste_indic ();
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


void nomenclature (void)
{
	switch (pvoie->niv2)
	{
	case 0:
		menu_nomenclature ();
		break;
	case 1:
		modif_nomenc ();
		break;
	case 3:
		rech_nomenc ();
		break;
	case 4:
		liste_indic ();
		break;
	case 5:
		saisie_infos ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "NOMENC", pvoie->niv2);
		break;
	}
}
