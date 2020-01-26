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
 *    MODULE FORWARDING OVERLAY 6
 */

#include <serv.h>

static int lit_champ (char *, int, int);
static void lit_fich (char *, int);

void entete_saisie (void)
{
	pvoie->msgtete = NULL;
	ptmes->taille = 0L;
	pvoie->messdate = time (NULL);
	pvoie->mess_recu = 0;
	pvoie->entete = 1;
	pvoie->header = 0;
	pvoie->mess_num = -1;
	*pvoie->mess_bid = '\0';
	strn_cpy (39, pvoie->mess_home, mypath);
	del_temp (voiecur);
}


int rcv_titre (void)
{
#define MAX_TITRE 60

	int nb = MAX_TITRE;
	char *scan;

	entete_saisie ();

	/* Efface un eventuel fichier existant
	 */

	scan = ptmes->titre;

	while ((*indd) && (*indd != '\n') && (*indd != '\r'))
	{
		if ((nb) && (*indd != 0x1a))
		{
			nb--;
			*scan++ = *indd;
		}
		++indd;
	}
	if (*indd)
		++indd;
	*scan = '\0';
	if ((!FOR (pvoie->mode)) && (nb == MAX_TITRE))
	{
		texte (T_MBL + 21);
		return (0);
	}
	return (1);
}


static int swap_get (char *buffer)
{
	if (*swap_scan == '\032')
		return (0);
	while ((*buffer++ = *swap_scan++) != '\0');
	return (1);
}



static void swapp_champ (bullist * pbul, char *nouveau, char sw_champ)
{
	switch (sw_champ)
	{
	case '@':
		strn_cpy (40, pbul->bbsv, nouveau);
		break;

	case '>':
		strn_cpy (6, pbul->desti, nouveau);
		break;

	case '<':
		strn_cpy (6, pbul->exped, nouveau);
		break;
	}
}

int swapp_bbs (bullist * pbul)
{
	char champ;
	char sw_champ;
	char ligne[81], ancien[80], nouveau[80];
	char *optr = nouveau;

	int ret = 0;

	df ("swapp_bbs", 2);
	swap_scan = swap_file;
	while (swap_get (ligne))
	{
		*ancien = *nouveau = '\0';
		champ = sw_champ = '\0';
		if (sscanf (ligne, "%c%s %c%s\n",
					&champ, ancien, &sw_champ, nouveau))
		{

			optr = nouveau;

			if (sw_champ == '\0')
				sw_champ = champ;

			switch (sw_champ)
			{

			case '@':
			case '>':
			case '<':
				break;

			default:
				sprintf (ligne, "%c%s", sw_champ, nouveau);
				sw_champ = champ;
				optr = ligne;

			}

			switch (champ)
			{

			case '@':
				/* if (indcmp (pbul->bbsv, ancien)) */
				if (hiecmp (pbul->bbsv, ancien))
				{
					swapp_champ (pbul, optr, sw_champ);
					/* ret = 1; */
				}
				break;

			case '>':
				if (indcmp (pbul->desti, ancien))
				{
					swapp_champ (pbul, optr, sw_champ);
					/* ret = 1; */
				}
				break;

			case '<':
				if (indcmp (pbul->exped, ancien))
				{
					swapp_champ (pbul, optr, sw_champ);
					/* ret = 1; */
				}
				break;

			}
		}
		if (ret)
		{
			break;
		}
	}

	/* if (indcmp (mycall, bbs_via (pbul->bbsv))) */
	if (hiecmp (mypath, pbul->bbsv))
	{
		*pbul->bbsv = '\0';
		ret = 0;
	}

	ff ();
	return (ret);
}


void ini_champs (int voie)
{
	int i;

	svoie[voie]->entmes.status = 'N';
	svoie[voie]->entmes.type = ' ';
	svoie[voie]->entmes.taille = 0L;
	svoie[voie]->entmes.bin = 0;
	svoie[voie]->m_ack = 0;
	svoie[voie]->messdate = time (NULL);
	svoie[voie]->mess_num = -1;
	svoie[voie]->entmes.numero = 0L;
	svoie[voie]->entmes.theme = 0;
	*(svoie[voie]->mess_bid) = '\0';
	*(svoie[voie]->entmes.desti) = '\0';
	*(svoie[voie]->entmes.bbsv) = '\0';
	*(svoie[voie]->entmes.bbsf) = '\0';
	*(svoie[voie]->entmes.bid) = '\0';
	*(svoie[voie]->appendf) = '\0';
	for (i = 0; i < NBMASK; i++)
	{
		svoie[voie]->entmes.fbbs[i] = svoie[voie]->entmes.forw[i] = '\0';
	}
	strn_cpy (6, svoie[voie]->entmes.exped, svoie[voie]->sta.indicatif.call);
}


int lit_com_fwd (void)
{
	ini_champs (voiecur);
	return (scan_com_fwd ());
}


int scan_com_fwd (void)
{
	char exped[10];
	int autotype = 0;

	sup_ln (indd);
	//strupr (indd);

	if (!is_room ())
	{
		outln ("*** Disk full !", 15);
		return (0);
	}

	if (ISGRAPH (*indd))
	{
		*indd = toupper (*indd);
		if ((*indd == 'T') || (*indd == 'B') || (*indd == 'P') || (FOR (pvoie->mode)) || (BBS (pvoie->finf.flags)))
		{
			ptmes->type = *indd;
		}
		else if ((*indd == 'E') || (isdigit (*indd)))
			ptmes->type = 'E';
		++indd;
	}

	if (!is_espace (indd))
	{
		texte (T_ERR + 2);
		return (0);
	}

	incindd ();

	if (!lit_champ (ptmes->desti, 6, 1))
		return (0);

	if (ptmes->type == ' ')
	{
		if ((find (ptmes->desti)) || (is_serv (ptmes->desti)))
		{
			ptmes->type = 'P';
		}
		else
		{
			ptmes->type = 'B';
		}
		autotype = 1;
	}

	if (*indd == ' ')
		incindd ();

	/* Transforme le 1er "AT" en "@" */
	if (strlen (indd) > 3)
	{
		if ((indd[0] == 'A') && (indd[1] == 'T') && (isspace (indd[2])))
		{
			indd[0] = '@';
			indd[1] = ' ';
		}
	}

	while (*indd)
	{
		switch (*indd)
		{
		case '@':
			incindd ();
			if (!lit_champ (ptmes->bbsv, 31, 2))
				return (0);
			if (*indd == ' ')
				incindd ();
			if ((ptmes->type != 'P') &&
				(ptmes->type != 'T') &&
				(!isdigit (ptmes->type)))
			{
				ptmes->type = 'B';
			}
			break;
		case '<':
			incindd ();
			if (!lit_champ (exped, 6, 1))
				return (0);
			if (*exped)
			{
				if ((strcmp (exped, pvoie->sta.indicatif.call) != 0) && (!forward_auth (voiecur)))
				{
					varx[0][0] = '<';
					varx[0][1] = '\0';
					texte (T_ERR + 9);
					return (0);
				}
				strcpy (ptmes->exped, exped);
			}
			if (*indd == ' ')
				incindd ();
			break;
		case '+':
			if (!droits (ACCESDOS))
			{
				if (!(FOR (pvoie->mode)))
				{
					varx[0][0] = '+';
					varx[0][1] = '\0';
					texte (T_ERR + 9);
				}
				return (0);
			}
			incindd ();
			lit_fich (pvoie->appendf, 79);
			if (access (pvoie->appendf, 0) != 0)
			{
				texte (T_ERR + 11);
				return (FALSE);
			}
			if (*indd == ' ')
				incindd ();
			break;
		case '$':
			incindd ();
			if (*indd)
			{
				if (!lit_champ (ptmes->bid, 12, 0))
					return (0);
			}
			else
				sprintf (ptmes->bid, " ");
			if (*indd == ' ')
				incindd ();
			break;
		default:
			if (*indd != '\r')
			{
				if (!(FOR (pvoie->mode)))
				{
					varx[0][0] = *indd;
					varx[0][1] = '\0';
					texte (T_ERR + 9);
				}
				return (0);
			}
			++indd;
			break;
		}
	}
	if (*ptmes->desti == '\0')
	{
		if (!(FOR (pvoie->mode)))
			texte (T_ERR + 6);
		return (0);
	}

	/* Prive sur routage de groupe ! */
	if ((*ptmes->bbsv) && (ptmes->type != 'T') &&
		(!msg_find (bbs_via (ptmes->bbsv))) && (find (ptmes->desti)))
	{
		if ((strcmp ("SYSOP", ptmes->desti) != 0) && (strcmp ("WP", ptmes->desti) != 0) && (!(FOR (pvoie->mode))))
		{
			texte (T_MBL + 21);
			return (0);
		}
	}

	ptmes->status = 'N';
	if (*ptmes->bbsv)
	{
		if ((autotype) && (ptmes->type == 'P') && (!find (ptmes->desti)) && (!find (bbs_via (ptmes->bbsv))))
			ptmes->type = 'B';
		if ((*ptmes->bbsv) &&
			(!find (bbs_via (ptmes->bbsv))) &&
			(((ptmes->type != 'P') &&
			  (ptmes->type != 'A')) ||
			 (strcmp (ptmes->desti, "SYSOP") == 0) || (strcmp ("WP", ptmes->desti) == 0)))
		{
			ptmes->status = '$';
		}
	}

	swapp_bbs (ptmes);

	if ((*ptmes->bbsv == '\0') && (ptmes->type == 'P'))
	{
		pvoie->m_ack = 1;
	}

	if (!addr_check (ptmes->bbsv))
	{
		return (0);
	}

	if (!reacheminement ())
		return (0);

	if ((*ptmes->bbsv == '\0') && (ptmes->type == 'A'))
	{
		ptmes->type = 'P';
	}

	if (!is_bid (ptmes->bid))
	{
		return (0);
	}

	return (1);
}


static int rej_get (char *buffer)
{
	if (*rej_scan == '\032')
		return (0);
	while ((*buffer++ = *rej_scan++) != '\0');
	return (1);
}

static int tst_mes_ems (int mode, bullist * ptmes)
{
	long taille;
	int retour = 0;
	unsigned record = 0;
	Rej_rec rej;

	while (read_rej (record, &rej))
	{
		if (rej.mode == mode)
		{
			taille = (long) rej.size * 1000L;
			retour = (
						 ((rej.type == '*') || (rej.type == ptmes->type)) &&
						 (strmatch (ptmes->exped, rej.exped)) &&
						 (strmatch (bbs_via (ptmes->bbsv), rej.via)) &&
						 (strmatch (ptmes->desti, rej.desti)) &&
						 (strmatch (ptmes->bid, rej.bid)) &&
						 (ptmes->taille >= taille)
				);
			if (retour)
				break;

		}
		++record;

	}
	return (retour);
}

static int tst_mes (int mode, bullist * ptmes)
{
	if (ptmes->type == 'A')
		return (0);

	if (EMS_REJ_OK ())
	{
		return (tst_mes_ems (mode, ptmes));
	}
	else
	{
		char chaine[82], smode[20], type[20], exped[20];
		char via[50], desti[20], bid[20];
		long taille;
		int retour = 0;

		rej_scan = rej_file;

		while (rej_get (chaine))
		{
			sscanf (chaine, "%s %s %s %s %s %s %ld",
					smode, type, exped, via, desti, bid, &taille);
			if (toupper (*smode) != mode)
				continue;

			taille *= 1000;
			retour = (
						 ((type[0] == '*') || (type[0] == ptmes->type)) &&
						 (strmatch (ptmes->exped, exped)) &&
						 (strmatch (bbs_via (ptmes->bbsv), via)) &&
						 (strmatch (ptmes->desti, desti)) &&
						 (strmatch (ptmes->bid, bid)) &&
						 (ptmes->taille >= taille)
				);
			if (retour)
				break;
		}
		return (retour);
	}
}


int rejet (bullist * ptmes)
{
	int retour;

	retour = tst_mes ('R', ptmes);

	if ((svoie[voiecur]->fbb >= 2) || (svoie[voiecur]->mbl_ext))
		return ((retour) ? 4 : 0);
	else
		return ((retour) ? 1 : 0);
}

int retenu (bullist * ptmes)
{
	int retour;

	retour = tst_mes ('H', ptmes);

	if (svoie[voiecur]->fbb >= 2)
		return ((retour) ? 5 : 0);
	else
		return (0);
}

int hold (bullist * ptmes)
{
	int retenu = 0;

	if ((!pvoie->header) || (pvoie->mode == 0))
	{
		retenu = tst_mes ('L', ptmes);
	}
	if (!retenu)
	{
		retenu = tst_mes ('H', ptmes);
	}
	return (retenu);
}

/*

 * Mode = 0 : Tous caracteres entre 0x21 et 0xff
 * Mode = 1 : champ sans '@' ni '.'
 * Mode = 2 : champ d'adresse hierarchique sans '@'
 *
 */

static int lit_champ (char *champ, int nb, int mode)
{
	int i;
	int error = 0;
	char *str = indd;
	char last = '\0';

	while ((*indd) && (!isspace (*indd)))
	{
		if (!ISGRAPH (*indd))
		{
			error = 1;
			break;
		}

		if (mode == 1)
		{
			if (((std_header & 1) == 0) && (*indd == '@'))
			{
				break;
			}
			if (*indd == '.')
			{
				error = 1;
				break;
			}
		}
		else if (mode == 2)
		{
			if (*indd == '@')
			{
				error = 1;
				break;
			}
		}

		if (--nb < 0)
			break;


		*champ++ = toupper(*indd);

		last = *indd++;
	}
	*champ = '\0';

	if (nb < 0)
		error = 2;

	if ((mode == 2) && ((std_header & 2) == 0) && (last == '.'))
		error = 1;

	if (!error)
		return (1);

	if (FOR (pvoie->mode))
		return (0);

	for (i = 0; i < 40; ++str)
	{
		if (isspace (*str))
			break;
		varx[0][i++] = toupper (*str);
	}
	varx[0][i] = '\0';

	switch (error)
	{
	case 1:					/* Champ errone */

		texte (T_ERR + 17);
		break;

	case 2:					/* Champ trop long */

		texte (T_ERR + 16);
		break;
	}

	return (0);
}

static void lit_fich (char *champ, int nb)
{
	while ((nb--) && (ISGRAPH (*indd)))
	{
		*champ++ = *indd++;
	}
	*champ = '\0';
}


void ch_bbs (int mode, char ifwd[NBBBS][7])
{
	int i, bbs, tempif, cptif = 0, nb_fwd = 0;
	char nombbs[80];
	char combuf[80];
	char *pcom;

	/*  init_bbs() ; */
	bbs = 0;
	for (i = 0; i < NBMASK * 8; i++)
		*ifwd[i] = '\0';

	rewind_fwd ();

	while (fwd_get (combuf))
	{
		pcom = combuf;
		switch (*pcom++)
		{
		case 'A':				/* recuperer le nom de la BBS destinataire */
			strcpy (nombbs, pcom);
			bbs = num_bbs (nombbs);
			strn_cpy (6, ifwd[bbs - 1], nombbs);
			nb_fwd++;
			break;
		case 'E':				/* ENDIF */
			if (!mode)
				--cptif;
			break;
		case 'I':				/* IF */
			if (mode)
				break;
			++cptif;
			if (tst_fwd (pcom, bbs, time (NULL), 0, NULL, 1, -1) == FALSE)
			{
				tempif = cptif - 1;
				while (cptif != tempif)
				{
					if (fwd_get (combuf) == 0)
					{
						break;
					}
					pcom = combuf;
					switch (*pcom++)
					{
					case 'I':
						++cptif;
						break;
					case 'E':
						--cptif;
						break;
					case '@':
						if (cptif == (tempif + 1))
							++tempif;
						break;
					default:
						break;
					}
				}
			}
			break;
		case '@':				/* ELSE */
			if (mode)
				break;
			if (cptif == 0)
				break;
			tempif = cptif - 1;
			while (cptif != tempif)
			{
				if (fwd_get (combuf) == 0)
				{
					break;
				}
				pcom = combuf;
				switch (*pcom++)
				{
				case 'I':
					++cptif;
					break;
				case 'E':
					--cptif;
					break;
				default:
					break;
				}
			}
			break;
		default:
			break;
		}
		if (nb_fwd == NBMASK * 8)
			break;
	}
}

int test_linked (void)
{
	strupr (sup_ln (indd));
	if ((pvoie->aut_linked) && (strncmp (indd, "LINKED TO ", 10) == 0))
	{
		strtok (indd, " ");
		libere_zones_allouees (voiecur);	/* Vide les eventuelles listes */
		maj_niv (0, 0, 0);
		con_voie (voiecur, indd);
		return (1);
	}
	return (0);
}

void clear_fwd (long numero)
{
	recfwd *prec;
	int pos = 0;
	lfwd *ptr_fwd = tete_fwd;

	if (fast_fwd)
	{
		while (1)
		{
			if (pos == NBFWD)
			{
				ptr_fwd = ptr_fwd->suite;
				if (ptr_fwd == NULL)
					break;
				pos = 0;
			}
			prec = &ptr_fwd->fwd[pos];
			if ((prec->type) && (prec->nomess == numero))
			{
				prec->type = '\0';
				break;
			}
			pos++;
		}
	}
}
