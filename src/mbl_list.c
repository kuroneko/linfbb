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

static int mbl_lx (void);
static int mbl_bloc_liste (void);
static int strfind (bullist *, char *);
static int teste_liste (bullist *);
static int mess_bloc_liste (void);


static void list_mine (char);

/* Commande 'L' -> List messages */

#include "aff_stat.c"

static int strfind (bullist * pbul, char *cherche)
{
	return (strmatch (ltitre (1, pbul), cherche));
}

static int manque_forward (bullist * lbul)
{
	int i;

	if ((*lbul->bbsv) && (lbul->status == 'N'))
	{
		for (i = 0; i < NBMASK; i++)
			if (lbul->fbbs[i])
				return (0);
		return (1);
	}
	return (0);
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

	if ((pvoie->recliste.type) && (pvoie->recliste.type != lbul->type))
		return (0);
	if ((pvoie->recliste.status) && (pvoie->recliste.status != lbul->status))
		return (0);
	if ((pvoie->recliste.route) && (!manque_forward (lbul)))
		return (0);
	if ((*pvoie->recliste.exp) && (!strmatch (lbul->exped, pvoie->recliste.exp)))
		return (0);
	if ((*pvoie->recliste.dest) && (!strmatch (lbul->desti, pvoie->recliste.dest)))
		return (0);
	if (*pvoie->recliste.find)
		return (strfind (lbul, pvoie->recliste.find));

	if (*pvoie->recliste.bbs)
	{
		if (*pvoie->recliste.bbs == '-')
		{
			if (*lbul->bbsv)
				return (0);
		}
		else
		{
			if (!strmatch (bbs_via (lbul->bbsv), pvoie->recliste.bbs))
				return (0);
		}
	}

	return (1);
}


static int mbl_bloc_liste (void)
{
	int retour = 1;
	unsigned offset = pvoie->recliste.offset;
	bloc_mess *bptr = pvoie->recliste.ptemp;
	bullist ligne;

	pvoie->sr_mem = pvoie->seq = FALSE;

	ouvre_dir ();

	while (bptr)
	{
		if (!pvoie->reverse)
			--offset;

		if (bptr->st_mess[offset].noenr)
		{
			read_dir (bptr->st_mess[offset].noenr, &ligne);

			if (ligne.numero < pvoie->recliste.debut)
				break;
			if ((ligne.type) && (droit_ok (&ligne, 1)) && (teste_liste (&ligne)))
			{
				if (pvoie->recliste.last-- == 0L)
					break;
				if (pvoie->temp1)
				{
					pvoie->temp2 -= entete_liste ();
					pvoie->temp1 = 0;
				}
				aff_status (&ligne);
				--pvoie->temp2;
			}
		}

		if (pvoie->reverse)
		{
			if (++offset == T_BLOC_MESS)
			{
				bptr = bptr->suiv;
				offset = 0;
			}
		}
		else
		{
			if (offset == 0)
			{
				bptr = prec_dir (bptr);
				offset = T_BLOC_MESS;
			}
		}

		if (pvoie->temp2 == 0)
		{
			retour = 2;
			break;
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


void mess_liste (int pr)
{
	int mode_list = 1;
	int verbose = 0;

	switch (pvoie->niv3)
	{

	case 0:
		pvoie->temp1 = 1;
		pvoie->temp2 = pvoie->lignes;
		ch_niv3 (1);
		break;

	case 1:
		break;

	case 2:
		while_space ();
		switch (toupper (*indd))
		{
		case 'A':
			mode_list = 0;
			retour_mbl ();
			break;
		case 'V':
			verbose = 1;
		case 'R':
			mode_list = 0;
			incindd ();
			if (isdigit (*indd))
			{
				list_read (verbose);
				pvoie->sr_mem = 1;
				if (mbl_mess_read () == 0)
				{
					pvoie->sr_mem = 0;
					texte (T_QST + 6);
				}
			}
			else
			{
				texte (T_ERR + 3);
				texte (T_QST + 6);
			}
			break;
		default:
			pvoie->temp2 = pvoie->lignes;
			ch_niv3 (1);
			break;
		}
		break;

	case 3:
		mode_list = 0;
		if (read_mess (1) == 0)
			ch_niv3 (4);
		break;

	case 4:
		mode_list = 0;
		pvoie->sr_mem = 1;
		if (mbl_mess_read () == 0)
		{
			pvoie->sr_mem = 0;
			texte (T_QST + 6);
			ch_niv3 (2);
		}
		break;

	default:
		fbb_error (ERR_NIVEAU, "MESS-LIST", pvoie->niv3);
	}
	if (mode_list)
	{
		if (pr)
		{
			pvoie->lignes = -1;
			switch (mess_bloc_liste ())
			{
			case 0:
				break;
			case 1:
				retour_mbl ();
				break;
			case 2:
				texte (T_QST + 6);
				ch_niv3 (2);
				break;
			}
		}
		else
		{
			pvoie->lignes = -1;
#if 0
			switch (mess_bloc_liste ())
			{
			case 0:
				break;
			case 1:
				retour_mbl ();
				break;
			case 2:
				ch_niv3 (1);
				break;
			}
#else
			mess_bloc_liste ();
			pvoie->sr_mem = pvoie->seq = FALSE;
			libere_tread (voiecur);
			libere_tlist (voiecur);
			pvoie->mbl = 1;
			maj_niv (N_MBL, 0, 0);
#endif
		}
	}
}

static int mess_bloc_liste (void)
{
	int retour = 1;
	rd_list *ptemp;
	bullist ligne;
	mess_noeud *mptr;

	pvoie->sr_mem = pvoie->seq = FALSE;

	ouvre_dir ();

	while ((ptemp = pvoie->t_list) != NULL)
	{
		if (pvoie->temp1)
		{
			pvoie->temp1 = 0;
			entete_liste ();
			--pvoie->temp2;
		}
		mptr = findmess (ptemp->nmess);
		if (mptr)
		{
			read_dir (mptr->noenr, &ligne);
			aff_status (&ligne);
		}
		--pvoie->temp2;
		pvoie->t_list = pvoie->t_list->suite;
		m_libere (ptemp, sizeof (rd_list));
		if (pvoie->temp2 == 0)
		{
			retour = 2;
			break;
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
	if ((retour) && (pvoie->temp1))
		texte (T_MBL + 3);
	return (retour);
}

static void list_mine (char mode)
{
	char s[2];
	int nouveau = 0;
	unsigned num_ind;

	if (isdigit (mode))
	{
		s[0] = mode;
		s[1] = '\0';
		insnoeud (s, &num_ind);
	}
	else
	{
		if (mode == 'N')
			nouveau = TRUE;
		num_ind = pvoie->no_indic;
	}
	list_messages (nouveau, num_ind, 1);
}

void send_list(int voie)
{
	unsigned offset;
	bloc_mess *bptr = tete_dir;
	mess_noeud *mptr;
	bullist bul;
	char buf[80];

	sprintf(buf, "S%d", svoie[voie]->ncur->nbmess);
	sta_drv (voie, SNDCMD, buf);
	
	ouvre_dir ();

	/* Read messages */
	while (bptr)
	{
		for (offset = 0 ; offset < T_BLOC_MESS ; offset++)
		{
			mptr = &(bptr->st_mess[offset]);
			if ((mptr->noenr) && (mptr->no_indic == svoie[voie]->no_indic))
			{
				read_dir (mptr->noenr, &bul);
				if (bul.status != 'N' && bul.status != 'K' && bul.status != 'A')
				{
					sprintf(buf, "M%ld %ld %c", bul.numero, bul.taille, bul.status);
					sta_drv (voie, SNDCMD, buf);
				}
			}
		}
		bptr = bptr->suiv;
	}

	bptr = tete_dir;

	/* Unread messages */
	while (bptr)
	{
		for (offset = 0 ; offset < T_BLOC_MESS ; offset++)
		{
			mptr = &(bptr->st_mess[offset]);
			if ((mptr->noenr) && (mptr->no_indic == svoie[voie]->no_indic))
			{
				read_dir (mptr->noenr, &bul);
				if (bul.status == 'N')
				{
					sprintf(buf, "M%ld %ld %c", bul.numero, bul.taille, bul.status);
					sta_drv (voie, SNDCMD, buf);
				}
			}
		}
		bptr = bptr->suiv;
	}

	ferme_dir ();

	sta_drv (voie, SNDCMD, "M0 0 N");
}

void list_messages (int nouveau, unsigned num_ind, int pr)
{
	int trouve = 0;
	unsigned offset = 0;
	bloc_mess *bptr = tete_dir;
	mess_noeud *mptr;
	rd_list *ptemp = NULL;
	bullist bul;

	ouvre_dir ();
	/* pvoie->typlist = 0; */

	while (bptr->suiv)
		bptr = bptr->suiv;

	while (bptr)
	{
		offset = T_BLOC_MESS;
		while (offset--)
		{
			mptr = &(bptr->st_mess[offset]);
			if ((mptr->noenr) && (mptr->no_indic == num_ind))
			{
				read_dir (mptr->noenr, &bul);
				if (((!nouveau) || (nouveau && (bul.status == 'N')))
					&& (bul.status != 'H') && (bul.status != 'A'))
				{
					if (ptemp)
					{
						ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
						ptemp = ptemp->suite;
					}
					else
					{
						pvoie->t_list = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
					}
					ptemp->suite = NULL;
					ptemp->nmess = mptr->nmess;
					ptemp->verb = 1;
					trouve = 1;
				}
			}
		}
		bptr = prec_dir (bptr);
	}
	ferme_dir ();

	if (!trouve)
	{
		if (nouveau)
			texte (T_MBL + 4);
		else
			texte (T_MBL + 3);
		retour_mbl ();
	}
	else
	{
		maj_niv (N_MBL, 16, 0);
		mess_liste (pr);
	}
}

static int copy_word(char *dest, char *str, int max)
{
	int nb = 0;

	while (!isspace(*str) && !iscntrl(*str))
	{
		*dest++ = *str++;
		++nb;
		
		if (max && nb == max)
			break;
	}
	*dest = '\0';
	
	return nb;
}

static int cmd_list(int cmd)
{
	char cde;
	int ok = 1;
	long date;

	switch (cmd)
	{

	case 'A':
		pvoie->recliste.status = 'A';
		break;

	case 'B':
		pvoie->recliste.type = 'B';
		break;

	case 'D':
		cde = *indd++;
		if ((cde == '<') || (cde == '>'))
		{
			if (teste_espace ())
			{
				if ((date = date_to_time (indd)) == 0L)
				{
					ok = 0;
					texte (T_ERR + 3);
				}
				else
				{
					char tmp[20];
					indd += copy_word(tmp, indd, 19);
					switch (cde)
					{
					case '>':
						pvoie->recliste.avant = date /* + 86400L */ ;
						break;
					case '<':
						pvoie->recliste.apres = date - 86400L;
						break;
					}
				}
			}
			else
			{
				texte (T_ERR + 2);
				ok = 0;
			}
		}
		else
		{
			ok = 0;
			sprintf (varx[0], "LD%c", cmd);
			texte (T_ERR + 1);
		}
		break;

	case 'E':
		pvoie->recliste.route = 1;
		break;

	case 'F':
		pvoie->recliste.status = 'F';
		break;

	case 'H':
		pvoie->temp1 = 0;
		list_held ();
		ok = 0;
		break;

	case 'K':
		pvoie->recliste.status = 'K';
		break;

	case 'L':
		if (!ISPRINT (*indd))
		{
			pvoie->recliste.last = 1;
		}
		else if (teste_espace ())
		{
			if (isdigit (*indd))
				pvoie->recliste.last = lit_chiffre (0);
			else
			{
				texte (T_ERR + 3);
				ok = 0;
			}
		}
		else
		{
			texte (T_ERR + 2);
			ok = 0;
		}
		break;

	case 'M':
	case 'N':
		list_mine (cmd);
		ok = 2;
		break;

	case 'P':
		pvoie->recliste.type = 'P';
		break;

	case 'S':
		if (teste_espace ())
		{
			int len = 0;
			char *ptr = indd;
			
			while (*ptr && *ptr != ' ')
			{
				++ptr;
				++len;
			}
			pvoie->recliste.find[0] = '*';
			strn_cpy ((len > 17) ? 17 : len, (pvoie->recliste.find) + 1, strupr (indd));
			indd += len;
			strcat (pvoie->recliste.find, "*");
		}
		else
		{
			texte (T_ERR + 2);
			ok = 0;
		}
		break;

	case 'T':
		pvoie->recliste.type = 'T';
		break;

	case 'U':
		pvoie->recliste.type = 'P';
		pvoie->recliste.status = 'N';
		break;

	case 'X':
		pvoie->recliste.status = 'X';
		break;

	case 'Y':
		pvoie->recliste.status = 'Y';
		break;

	case '$':
		pvoie->recliste.status = '$';
		break;

	case '<':
		if (teste_espace ())
		{
			indd += copy_word(pvoie->recliste.exp, indd, 6);
		}
		else
		{
			texte (T_ERR + 2);
			ok = 0;
		}
		break;

	case '>':
		if (teste_espace ())
		{
			indd += copy_word(pvoie->recliste.dest, indd, 6);
		}
		else
		{
			texte (T_ERR + 2);
			ok = 0;
		}
		break;

	case '@':
		if (teste_espace ())
		{
			indd += copy_word(pvoie->recliste.bbs, indd, 6);
		}
		else
		{
			strcpy (pvoie->recliste.bbs, "-");
		}
		break;

	case 'R':
		pvoie->reverse = 1;
	case ' ':
		teste_espace ();
		if (*indd == '\0')
		{
			pvoie->recliste.l = TRUE;
			pvoie->recliste.debut = pvoie->finf.lastmes + 1;
			pvoie->l_mess = nomess;
			break;
		}
		if (strchr (indd, '-'))
		{
			if (isdigit (*indd))
				pvoie->recliste.debut = lit_chiffre (1);
			else
			{
				texte (T_ERR + 3);
				ok = 0;
				break;
			}
			++indd;
			if (isdigit (*indd))
				pvoie->recliste.fin = lit_chiffre (1);
			if (pvoie->recliste.fin <= pvoie->recliste.debut)
				ok = 0;
		}
		else
		{
			if (isdigit (*indd))
				pvoie->recliste.fin = pvoie->recliste.debut = lit_chiffre (1);
			else
				ok = 0;
		}
		break;

	default:
		if (isdigit (cmd))
		{
			list_mine (cmd);
			ok = 0;
		}
		else
		{
			/*
			   sprintf(varx[0], "L%c", c) ;
			   texte(T_ERR + 1) ;
			 */
			ok = 3;
		}
		break;
	}

	return ok;
}

int mbl_lx (void)
{
	char c;
	int ok = 1;
	int i;
	bloc_mess *temp;
	long numero;

	pvoie->reverse = 0;

	init_recliste (voiecur);

	if (!ISPRINT (*indd))
	{
		pvoie->recliste.debut = pvoie->finf.lastmes + 1;
		pvoie->l_mess = nomess;
	}
	else
	{
		sup_ln (indd);
		fbb_log (voiecur, 'M', strupr (indd - 1));
		c = *indd++;
		for (;;)
		{
			ok = cmd_list(c);
			if (ok == 0)
				break;
			while_space();
			c = *indd++;
			if (c == '\0')
				break;
			if (c != '&')
			{
				ok = 3;
				break;
			}
			incindd();
			c = *indd++;
			if (c != 'L' && c != 'l')
			{
				ok = 3;
				break;
			}
			c = *indd++;
		}
	}
	if (ok == 1)
	{
		if (pvoie->reverse)
		{
			temp = tete_dir;
			numero = pvoie->recliste.debut;
			while (temp->suiv)
			{
				if (temp->suiv->st_mess[0].nmess > numero)
					break;
				temp = temp->suiv;
			}
			for (i = 0; i < T_BLOC_MESS; i++)
			{
				if (temp->st_mess[i].nmess == 0)
				{
					--i;
					break;
				}
				if (temp->st_mess[i].nmess >= numero)
					break;
			}
			if (i == T_BLOC_MESS)
				--i;
			pvoie->recliste.ptemp = temp;
			pvoie->recliste.offset = i;
		}
		else
		{
			pvoie->recliste.ptemp = last_dir ();
			pvoie->recliste.offset = T_BLOC_MESS;
		}
		pvoie->temp1 = 1;
	}
	return (ok);
}


void list_read (int verbose)
{
	long no;
	bullist *pbul;
	rd_list *ptemp = NULL;

	libere_tread (voiecur);

	while ((no = lit_chiffre (1)) != 0L)
	{
		if ((pbul = ch_record (NULL, no, 'Y')) != NULL)
		{
			if (droit_ok (pbul, 1))
			{
				if (ptemp)
				{
					ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
					ptemp = ptemp->suite;
				}
				else
				{
					pvoie->t_read = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
				}
				ptemp->suite = NULL;
				ptemp->nmess = no;
				ptemp->verb = verbose;
			}
			else
				texte (T_ERR + 10);
		}
		else
			texte (T_ERR + 10);
	}
}


int mbl_list (void)
{
	int mode_list = 1;
	int verbose = 0;
	int error = 0;

	switch (pvoie->niv3)
	{
	case 0:
		if (toupper (*indd) == 'C')
		{
			return (list_lc ());
		}

		switch (mbl_lx ())
		{
		case 0:
			retour_mbl ();
		case 2:
			mode_list = 0;
			break;
		case 1:
			pvoie->temp2 = pvoie->lignes;
			ch_niv3 (1);
			break;
		case 3:
			mode_list = 0;
			error = 1;
			--indd;
			break;
		}
		break;
	case 1:
		break;
	case 2:
		while_space ();
		switch (toupper (*indd))
		{
		case 'A':
			mode_list = 0;
			retour_mbl ();
			break;
		case 'V':
			verbose = 1;
		case 'R':
			mode_list = 0;
			incindd ();
			if (isdigit (*indd))
			{
				pvoie->aut_nc = 1;
				list_read (verbose);
				pvoie->sr_mem = 1;
				if (mbl_mess_read () == 0)
				{
					pvoie->sr_mem = 0;
					texte (T_QST + 6);
				}
			}
			else
			{
				texte (T_ERR + 3);
				texte (T_QST + 6);
			}
			break;
		default:
			pvoie->temp2 = pvoie->lignes;
			ch_niv3 (1);
			break;
		}
		break;
	case 3:
		mode_list = 0;
		pvoie->aut_nc = 1;
		if (read_mess (1) == 0)
			ch_niv3 (4);
		break;
	case 4:
		mode_list = 0;
		pvoie->aut_nc = 1;
		pvoie->sr_mem = 1;
		if (mbl_mess_read () == 0)
		{
			pvoie->sr_mem = 0;
			texte (T_QST + 6);
			ch_niv3 (2);
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "MSG-LIST", pvoie->niv3);
	}

	if (mode_list)
	{
		pvoie->lignes = -1;
		switch (mbl_bloc_liste ())
		{
		case 0:
			break;
		case 1:
			retour_mbl ();
			break;
		case 2:
			texte (T_QST + 6);
			ch_niv3 (2);
			break;
		}
	}
	return (error);
}


/* Test du password */

void mbl_passwd (void)
{
	char buffer[80];

	switch (pvoie->niv3)
	{
	case 0:
		snd_passwd (buffer);
		if (*buffer == '\0')
			retour_mbl ();
		else
		{
			outln (buffer, strlen (buffer));
			ch_niv3 (1);
		}
		break;
	case 1:
		if (tst_passwd (indd))
		{
			outln ("Ok", 2);
			change_droits (voiecur);
			sprintf (buffer, "SYS OK");
		}
		else
		{
			*pvoie->passwd = '\0';
			sprintf (buffer, "SYS FAILED");
		}
		fbb_log (voiecur, 'M', buffer);
		retour_mbl ();
		break;
	}
}


/* Commande 'S' -> Send messages */

int mbl_send (void)
{
	int error = 0;

	switch (toupper (*indd))
	{
	case 'X':
		if (!read_only ())
		{
			--indd;
			maj_niv (N_XFWD, 1, 0);
			xfwd ();
		}
		else
			retour_mbl ();
		break;
	case 'Y':
		if ((toupper (indd[1]) == 'S') && (!pvoie->read_only) && (droits (CMDSYS)))
		{
			maj_niv (N_MBL, 14, 0);
			mbl_passwd ();
		}
		else
		{
			error = 1;
			/* cmd_err(--indd) ; */
		}
		break;
	case 'R':
		if (!read_only ())
		{
			*indd = toupper (*indd);
			send_reply ();
		}
		else
			retour_mbl ();
		break;
	case 'C':
		if (!read_only ())
		{
			fbb_log (voiecur, 'S', strupr (indd));
			send_copy ();
		}
		else
			retour_mbl ();
		break;
	default:
		maj_niv (N_FORW, 0, 0);
		error = fwd ();
		break;
	}
	return (error);
}


/* Commande 'I' -> Info */

void mbl_info (void)
{
	char buffer[257];

	sprintf (buffer, "LANG\\%s.INF", nomlang + nlang * LG_LANG);
	if (!outfich (c_disque (buffer)))
		q_mark ();
}
