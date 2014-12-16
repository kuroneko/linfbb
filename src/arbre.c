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
 * ARBRE.C
 *
 */

#include <serv.h>

static void aff_ind (char *, char *);

void end_arbre (void)
{
	bloc_indic *bptr;
	iliste *temp;
	iliste *c_iliste = t_iliste.suiv;

	/* Libere l'arbre des indicatifs */
	while (racine)
	{
		bptr = racine;
		racine = racine->suiv;
		m_libere (bptr, sizeof (bloc_indic));
	}

	/* Libere la liste des indicatifs balise */
	while (c_iliste)
	{
		temp = c_iliste;
		c_iliste = c_iliste->suiv;
		m_libere (temp, sizeof (iliste));
	}

   t_iliste.suiv = NULL;
}

ind_noeud *insnoeud (char *indic_om, unsigned *no_indic)
{
	int i;
	int trouve = 0;

	char om[10];
	char *ptr_om = om;
	int offset = 0;
	unsigned num_ind = 0;
	bloc_indic *bptr = racine;
	static ind_noeud *pnoeud;

	i = 0;
	while ((*ptr_om++ = *indic_om++) != '\0')
	{
		if (i++ == 9)
		{
			*ptr_om = '\0';
			break;
		}
	}

	while (bptr)
	{
		pnoeud = &(bptr->st_ind[offset]);
		if (*(pnoeud->indic) == '\0')
			break;
		if (strncmp (om, pnoeud->indic, 6) == 0)
		{
			trouve = 1;
			break;
		}
		if (++offset == T_BLOC_INFO)
		{
			if (!bptr->suiv)
				bptr->suiv = new_bloc_info ();
			bptr = bptr->suiv;
			offset = 0;
		}
		++num_ind;
	}
	if (!trouve)
	{
		n_cpy (6, pnoeud->indic, om);
		pnoeud->coord = 0xffff;
		pnoeud->nbmess = (short) 0;
		pnoeud->nbnew = (short) 0;
		pnoeud->val = 1;
	}
	*no_indic = num_ind;

	return (pnoeud);
}


/*
 * Pointe sur le dernier bloc
 */

bloc_mess *last_dir ()
{
	bloc_mess *temp = tete_dir;

	while (temp->suiv)
		temp = temp->suiv;		/* aller au dernier bloc */
	return (temp);
}


/*
 * Pointe sur le bloc precedent
 */

bloc_mess *prec_dir (bloc_mess * bptr)
{
	bloc_mess *temp = tete_dir;

	while (temp->suiv)
	{
		if (temp->suiv == bptr)
			return (temp);
		temp = temp->suiv;
	}
	return (NULL);
}


/*
 * Insere un message dans la liste de directory
 */

int insmess (unsigned r, unsigned num_ind, long numero)
{
	int i;
	bloc_mess *temp = tete_dir;

	while (temp->suiv)
		temp = temp->suiv;		/* aller au dernier bloc */

	for (i = 0; ((i < T_BLOC_MESS) && (temp->st_mess[i].nmess)); i++)
		;

	if (i == T_BLOC_MESS)
	{
		temp->suiv = new_bloc_mess ();
		temp = temp->suiv;
		i = 0;
	}

	temp->st_mess[i].nmess = numero;
	temp->st_mess[i].noenr = r;
	temp->st_mess[i].no_indic = num_ind;
	return (1);
}


/*
 * Cherche un message deja insere dans la liste de directory
 */

mess_noeud *findmess (long numero)
{
	int i;
	bloc_mess *temp = tete_dir;

	if (numero == 0L)
		return (NULL);

	while (temp->suiv)
	{
		if (temp->suiv->st_mess[0].nmess > numero)
			break;
		temp = temp->suiv;
	}

	for (i = 0; i < T_BLOC_MESS; i++)
	{
		if (temp->st_mess[i].nmess == numero)
			return (&temp->st_mess[i]);
	}
	return (NULL);
}


/*
 * Valide un message deja insere dans la liste de directory
 */

void valmess (bullist * bptr)
{
	mess_noeud *mptr;

	if ((mptr = findmess (bptr->numero)) != NULL)
	{
		ouvre_dir ();
		write_dir (mptr->noenr, bptr);
		ferme_dir ();
		insarbre (bptr);
	}
}


/*
 * Change le numero du destinataire d'un message dans la liste de directory
 */

void chg_mess (unsigned num_ind, long numero)
{
	mess_noeud *mptr;

	if ((mptr = findmess (numero)) != NULL)
		mptr->no_indic = num_ind;
}


unsigned insarbre (bullist * pbuf)
{
	char temp[2];
	unsigned no_indic;
	ind_noeud *noeud;

	if ((pbuf->status == 'A') || (pbuf->status == 'K'))
		return (0xffff);

	if (isdigit (pbuf->type))
	{
		temp[0] = pbuf->type;
		temp[1] = '\0';
		noeud = insnoeud (temp, &no_indic);
	}
	else
	{
		noeud = insnoeud (pbuf->desti, &no_indic);
	}
	if (pbuf->status != 'H')
	{
		++(noeud->nbmess);
		if (pbuf->status == 'N')
			++(noeud->nbnew);
	}
	return (no_indic);
}


void inscoord (unsigned r, info * pbuf, ind_noeud * pnoeud)
{
	char om[80];
	char *indic_om = pbuf->indic.call;
	char *ptr_om = om;

	if ((isdigit (*indic_om)) && (strlen (indic_om) == 1))
	{
		strcpy (om, indic_om);
	}
	else
	{
		while ((*ptr_om++ = *indic_om++) != '\0');
	}

	n_cpy (6, pnoeud->indic, om);
	pnoeud->coord = r;
	pnoeud->val = (uchar) (EXC (pbuf->flags) == 0);
	pnoeud->nbmess = (short) 0;
	pnoeud->nbnew = (short) 0;
}


void connexion (int voie)
{
	svoie[voie]->ncur = insnoeud (svoie[voie]->sta.indicatif.call, &(svoie[voie]->no_indic));
	aff_nbsta ();
}


ind_noeud *cher_noeud (char *indic_om)
{
	int trouve = 0;
	int offset = 0;
	bloc_indic *bptr = racine;
	ind_noeud *pnoeud = NULL;

	while (bptr)
	{
		pnoeud = &(bptr->st_ind[offset]);
		if (*(pnoeud->indic) == '\0')
			break;
		if (strncmp (indic_om, pnoeud->indic, 6) == 0)
		{
			trouve = 1;
			break;
		}
		if (++offset == T_BLOC_INFO)
		{
			bptr = bptr->suiv;
			offset = 0;
		}
	}
	if (!trouve)
		pnoeud = NULL;
	return (pnoeud);
}


unsigned chercoord (char *indic_om)
{

	ind_noeud *noeud = cher_noeud (indic_om);

	if (noeud)
		return (noeud->coord);
	else
		return (0xffff);
}


/*
 * Liste des destinataires prives -> balise
 */

void ins_iliste (bullist * buf)
{
	int cmp;
	iliste *temp, *prec;
	iliste *c_iliste = &t_iliste;

	if ((*buf->bbsv) && (!hiecmp (mypath, buf->bbsv)))
		return;

	if (buf->status == 'H')
		return;

	if ((buf->type == 'P') && (buf->status != 'N'))
		return;

	if (strcmp (buf->desti, "WP") == 0)
		return;

	if ((buf->type != 'P') && (
								  (balbul == 0) ||
		(buf->status == 'X') || (buf->status == 'K') || (buf->status == 'A')
		))
		return;

	prec = c_iliste;
	while ((c_iliste = c_iliste->suiv) != NULL)
	{
		cmp = strcmp (c_iliste->indic, buf->desti);
		if (cmp == 0)
			return;
		if (cmp > 0)
			break;
		prec = c_iliste;
	}
	temp = (iliste *) m_alloue (sizeof (iliste));
	prec->suiv = temp;
	temp->suiv = c_iliste;
	strcpy (temp->indic, buf->desti);
}


void list_new (char *chaine)
{
	iliste *prec;
	int flag_l, nombre = max_indic;
	ind_noeud *noeud;
	unsigned num_indic;

	*chaine = '\0';
	flag_l = FALSE;

	while (nombre)
	{
		if (t_iliste.suiv == NULL)
			break;
		prec = p_iliste;
		if (p_iliste == &t_iliste)
			flag_l = TRUE;
		p_iliste = p_iliste->suiv;
		if (p_iliste == NULL)
		{
			p_iliste = &t_iliste;
			if (flag_l)
				break;
			continue;
		}
		noeud = insnoeud (p_iliste->indic, &num_indic);
		if (noeud->nbnew == 0)
		{
			prec->suiv = p_iliste->suiv;
			m_libere (p_iliste, sizeof (iliste));
			p_iliste = prec;
			continue;
		}
		aff_ind (noeud->indic, chaine);
		nombre--;
	}
	if (*chaine == '\0')
		strcpy (chaine, " None");
}


static void aff_ind (char *indptr, char *indch)
{
	strcat (indch, " ");
	strcat (indch, indptr);
}


bloc_indic *new_bloc_info (void)
{
	int i;

	bloc_indic *bptr = (bloc_indic *) m_alloue (sizeof (bloc_indic));

	bptr->suiv = NULL;
	for (i = 0; i < T_BLOC_INFO; i++)
		*(bptr->st_ind[i].indic) = '\0';
	return (bptr);
}
