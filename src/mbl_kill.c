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

/*
 * Module Nø1 emulation WA7MBL
 */

static int mbl_kill_liste (void);
static int mbl_kx (void);
static int teste_liste (bullist *);

static char *ltitre (int mode, bullist * pbul)	/* Mode = 1 pour LS */
{
	int lg = 80;
	static char buf[100];

	if (pvoie->typlist)
	{
		sprintf (buf, "%-12s ", pbul->bid);
		if (mode)
			strn_cpy (lg, buf + 13, pbul->titre);
		else
			n_cpy (lg, buf + 13, pbul->titre);
	}
	else
	{
		if (mode)
			strn_cpy (lg, buf, pbul->titre);
		else
			n_cpy (lg, buf, pbul->titre);
	}
	return (buf);
}


static int strfind (bullist * pbul, char *cherche)
{
	return (strmatch (ltitre (1, pbul), cherche));
}


static int teste_liste (bullist * lbul)
{
	if (lbul->numero < pvoie->recliste.debut)
		return (0);
	if (lbul->numero > pvoie->recliste.fin)
		return (0);
	if (lbul->date < pvoie->recliste.avant)
		return (0);
	if (lbul->date > pvoie->recliste.apres)
		return (0);

	if (!droits (COSYSOP))
	{
		if (strcmp (lbul->desti, "KILL") == 0)
			return (0);
		if (lbul->type == 'A')
			return (0);
		if ((lbul->status == 'K') && (pvoie->recliste.status != 'K'))
			return (0);
		if ((lbul->status == 'A') && (pvoie->recliste.status != 'A'))
			return (0);
	}

	if (*pvoie->recliste.find)
		return (strfind (lbul, pvoie->recliste.find));

	if ((pvoie->recliste.type) && (pvoie->recliste.type != lbul->type))
		return (0);
	if ((pvoie->recliste.status) && (pvoie->recliste.status != lbul->status))
		return (0);
	if ((*pvoie->recliste.exp) && (!strmatch (lbul->exped, pvoie->recliste.exp)))
		return (0);
	if ((*pvoie->recliste.dest) && (!strmatch (lbul->desti, pvoie->recliste.dest)))
		return (0);
	if (*pvoie->recliste.bbs)
	{
		if (*pvoie->recliste.bbs == '-')
		{
			if (*lbul->bbsv)
				return (0);
		}
		else
		{
			if (!strmatch (lbul->bbsv, pvoie->recliste.bbs))
				return (0);
		}
	}

	return (1);
}


/* Commande 'K' -> Kill messages  ou 'H' -> hold message */

int supp_nomess (long no, int archive)
{
	int i;
	int kill_new = 0;
	bullist lbul;
	char s[80];
	char type = '\0';

	if (archive >= 0x80)
	{
		archive -= 0x80;
		kill_new = 1;
	}

	switch (archive)
	{
	case 0:
		type = 'K';
		break;
	case 1:
		type = 'A';
		break;
	case 2:
		type = 'H';
		break;
	}

	if (!ch_record (&lbul, no, 0))
		return (2);

	if (archive == 2)
	{
		if ((lbul.status != '$') && (lbul.status != 'N') && (lbul.status != 'Y'))
			return (2);
	}

	if (!droit_ok (&lbul, (archive == 1) ? 3 : ((kill_new) ? 4 : 2)))
		return (3);

	ch_record (&lbul, no, (kill_new) ? (type | 0x80) : type);

	sprintf (s, "%c %ld", type, lbul.numero);
	fbb_log (voiecur, 'M', s);
	clear_fwd (lbul.numero);
	lbul.status = type;
	for (i = 0; i < NBMASK; i++)
	{
		lbul.fbbs[i] = '\0';
	}
	maj_rec (lbul.numero, &lbul);
	if (archive == 2)
		++nb_hold;
	return (1);
}


static void kill_mine (int archive)
{
	/* Suppression des messages personnels */
	int trouve = 0;
	unsigned num_ind = pvoie->no_indic;
	unsigned offset = 0;
	bullist bul;
	bloc_mess *bptr = tete_dir;
	mess_noeud *mptr;

	ouvre_dir ();

	while (bptr)
	{
		mptr = &(bptr->st_mess[offset]);
		if (mptr->noenr == 0)
			break;
		if (mptr->no_indic == num_ind)
		{
			read_dir (mptr->noenr, &bul);
			*ptmes = bul;
			if (bul.status != 'H')
			{
				trouve = 1;
				if (bul.status != 'N')
				{
					switch (supp_nomess (bul.numero, archive))
					{
					case 1:
						texte (T_MBL + 7);
						break;
					case 2:	/* Deja tue ! */
						break;
					case 3:
						texte (T_ERR + 12);
						break;
					}
				}
				else
					texte (T_ERR + 12);
			}
		}
		if (++offset == T_BLOC_MESS)
		{
			bptr = bptr->suiv;
			offset = 0;
		}
	}

	ferme_dir ();

	if (!trouve)
		texte (T_MBL + 3);
}


static int mbl_kill_liste (void)
{
	int retour = 1;
	bullist ligne;
	unsigned offset = pvoie->recliste.offset;
	bloc_mess *bptr = pvoie->recliste.ptemp;
	int archive = pvoie->recliste.l;
	mess_noeud *mptr;

	pvoie->sr_mem = pvoie->seq = FALSE;
	ouvre_dir ();
	while (bptr)
	{
		mptr = &(bptr->st_mess[offset]);
		if (mptr->noenr == 0)
			break;
		read_dir (mptr->noenr, &ligne);
		if (ligne.numero < pvoie->recliste.debut)
			break;
		if (teste_liste (&ligne))
		{
			if (pvoie->recliste.last-- == 0L)
				break;
			if (droit_ok (&ligne, 2))
			{
				switch (supp_nomess (ligne.numero, archive))
				{
				case 1:
					if (archive == 2)
						outln ("Msg #$M held.", 13);
					else
						texte (T_MBL + 7);
					break;
				case 2:		/* Deja tue ! */
					ptmes->numero = ligne.numero;
					break;
				case 3:
					ptmes->numero = ligne.numero;
					if (archive == 2)
						texte (T_ERR + 13);
					else
						texte (T_ERR + 12);
					break;
				}
				pvoie->temp1 = 0;
			}
		}
		if (++offset == T_BLOC_MESS)
		{
			bptr = bptr->suiv;
			offset = 0;
		}
		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 0;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 0;
			break;
		}
	}
	ferme_dir ();

	pvoie->recliste.offset = offset;
	pvoie->recliste.ptemp = bptr;

	if ((retour) && (pvoie->temp1))
		texte (T_MBL + 3);
	return (retour);
}


int mbl_kill (void)
{
	int error = 0;

	switch (pvoie->niv3)
	{
	case 0:
		error = mbl_kx ();
		break;
	case 1:
		if (mbl_kill_liste ())
			retour_mbl ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "MESS-KILL", pvoie->niv3);
	}
	return (error);
}


static int mbl_kx (void)
{
	long no;
	char c;
	int archive = 0, fin = 1, suite = 1;
	int error = 0;

	sup_ln (indd);
	c = toupper (*indd);
	init_recliste (voiecur);

	pvoie->aut_nc = 1;

	if (droits (SUPMES))
	{

		if (c == 'K')
		{
			++indd;
			c = toupper (*indd);
			archive = 1;
		}

		switch (c)
		{

		case '>':
			++indd;
			suite = 0;
			if (teste_espace ())
			{
				strn_cpy (6, pvoie->recliste.dest, indd);
				fin = 0;
			}
			else
			{
				texte (T_ERR + 2);
			}
			break;

		case '<':
			++indd;
			suite = 0;
			if (teste_espace ())
			{
				strn_cpy (6, pvoie->recliste.exp, indd);
				fin = 0;
			}
			else
			{
				texte (T_ERR + 2);
			}
			break;

		case 'F':
			++indd;
			suite = 0;
			pvoie->recliste.status = 'F';
			fin = 0;
			break;

		case '@':
			++indd;
			suite = 0;
			if (teste_espace ())
			{
				strn_cpy (6, pvoie->recliste.bbs, indd);
				fin = 0;
			}
			else
			{
				texte (T_ERR + 2);
			}
			break;

		default:
			break;
		}
		if ((!fin) && (!suite))
		{
			pvoie->recliste.ptemp = tete_dir;
			pvoie->recliste.offset = 0;
			pvoie->recliste.l = archive;
			pvoie->temp1 = 1;
			if (mbl_kill_liste ())
				fin = 1;
			else
				ch_niv3 (1);
		}
	}
	if (suite)
	{
		if (c == 'M')
		{
			kill_mine (archive);
		}
		else
		{
			if (teste_espace ())
			{
				while ((no = lit_chiffre (1)) != 0L)
				{
					/* Autorise la suppression des messages non lus */
					switch (supp_nomess (no, archive + 0x80))
					{
					case 1:
						texte (T_MBL + 7);
						break;
					case 2:
						texte (T_ERR + 10);
						break;
					case 3:
						texte (T_ERR + 12);
						break;
					}
				}
			}
			else
			{
				/* texte(T_ERR + 3) ; */
				error = 1;
				fin = 0;
			}
		}
	}
	if (fin)
		retour_mbl ();
	return (error);
}

/*
 * Archive = 0 -> K
 *           1 -> A
 *           2 -> H
 */
int hold_kill (int archive)
{
	long no;
	char c;
	int fin = 1, suite = 1;
	int error = 0;

	c = toupper (*indd);
	switch (c)
	{
	case '>':
		++indd;
		suite = 0;
		if (teste_espace ())
		{
			strn_cpy (6, pvoie->recliste.dest, indd);
			fin = 0;
		}
		else
		{
			texte (T_ERR + 2);
		}
		break;
	case '<':
		++indd;
		suite = 0;
		if (teste_espace ())
		{
			strn_cpy (6, pvoie->recliste.exp, indd);
			fin = 0;
		}
		else
		{
			texte (T_ERR + 2);
		}
		break;
	case 'F':
		if (archive != 2)
		{
			++indd;
			suite = 0;
			pvoie->recliste.status = 'F';
			fin = 0;
		}
		break;
	case '@':
		++indd;
		suite = 0;
		if (teste_espace ())
		{
			strn_cpy (6, pvoie->recliste.bbs, indd);
			fin = 0;
		}
		else
		{
			texte (T_ERR + 2);
		}
		break;
	default:
		break;
	}
	if ((!fin) && (!suite))
	{
		pvoie->recliste.ptemp = tete_dir;
		pvoie->recliste.offset = 0;
		pvoie->recliste.l = archive;
		pvoie->temp1 = 1;
		if (mbl_kill_liste ())
			fin = 1;
		else
			ch_niv3 (1);
	}
	if (suite)
	{
		if (teste_espace ())
		{
			while ((no = lit_chiffre (1)) != 0L)
			{
				switch (supp_nomess (no, archive))
				{
				case 1:
					outln ("Msg #$M held.", 13);
					break;
				case 2:
					break;
				case 3:
					texte (T_ERR + 10);
					break;
				}
			}
		}
		else
		{
			error = 1;
			fin = 0;
		}
	}
	if (fin)
		retour_mbl ();
	return (error);
}
