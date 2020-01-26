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
 * Module MBL_EDIT.C
 */

static void ch_home (ind_noeud *, info *);
static void ch_password (ind_noeud *, info *);
static void ch_prenom (ind_noeud *, info *);
static void ch_private (ind_noeud *, info *);
static void ch_zip (ind_noeud *, info *);
static void header_edit (void);
static void modif_header (void);
static void modif_user (info *);
static void user_edit (void);
static void option_edit (void);

static char *lit_ind (char *);

static int lit_prenom (void);
static int lit_homebbs (void);
static int lit_locator (void);
static int lit_zip (void);

#include "aff_stat.c"

int mbl_edit (void)
{
	int error = 0;

	if (pvoie->niv3 == 0)
	{
		switch (toupper (*indd))
		{
		case ' ':
			ch_niv3 (20);
			pvoie->typlist = 0;
			header_edit ();
			break;
		case 'O':
			ch_niv3 (40);
			option_edit ();
			break;
		case 'U':
			ch_niv3 (0);
			user_edit ();
			break;
		case 'M':
			if (voiecur == CONSOLE)
			{
				++indd;
				if ((teste_espace ()) && (*indd) && (*indd == '#' || isdigit (*indd)))
				{
					if ((pvoie->nmess = lit_chiffre (1)) != 0L)
					{
						if (ch_record (ptmes, pvoie->nmess, '\0'))
						{
							reply = 4;
							pvoie->enrcur = pvoie->nmess;
#ifdef __WINDOWS__
							if (win_edit () == 5)
								end_win_edit ();
							error = 0;
#endif
#ifdef __linux__
							error = 1;
#endif
#ifdef __FBBDOS__
							if (mini_edit () == 5)
								end_mini_edit ();
							error = 0;
#endif
							break;
						}
						else
						{
							ptmes->numero = pvoie->nmess;
							texte (T_ERR + 10);
						}
					}
					else
						texte (T_ERR + 3);
				}
				else
					texte (T_ERR + 3);
				retour_mbl ();
			}
			else
				error = 1;
			break;
		default:
			/*
			   varx[0][0] = 'E' ;
			   strn_cpy(79, varx[0] + 1, indd) ;
			   texte(T_ERR + 1) ;
			   retour_mbl() ;
			 */
			error = 1;
			break;
		}
	}
	else
	{
		if (*indd)
			indd[strlen (indd) - 1] = '\0';
		switch (pvoie->niv3 / 20)
		{
		case 0:
			user_edit ();
			break;
		case 1:
			header_edit ();
			break;
		case 2:
			option_edit ();
			break;
		}
	}
	return (error);
}

static void option_display(void)
{
	char buf[80];
	int i;

	i = sprintf(buf, "System Options : ");
	buf[i++] = (bip) ? 'B' : '.';
	buf[i++] = (gate) ? 'G' : '.';
	buf[i++] = (sed) ? 'M' : '.';
	buf[i++] = (ok_tell) ? 'T' : '.';
	buf[i++] = (aff_inexport) ? 'X' : '.';
	buf[i++] = (aff_popsmtp) ? 'P' : '.';
	buf[i] = '\0';
	outln(buf, strlen(buf));
	sprintf(buf, "(B)eep, (T)alk, (G)ateway, (M)sgEdit, e(X)port, (P)opSmtp <return> : ");
	out(buf, strlen(buf));
}

static void option_edit (void)
{
	int ok;
	
	switch (pvoie->niv3)
	{
	case 40:
		option_display();
		ch_niv3 (41);
		break;
	case 41:
		if (*indd)
		{
			ok = 1;
			
			if (islower(*indd))
				*indd = toupper(*indd);
			
			switch (*indd)
			{
			case 'B' :
				bip = !bip;
				break;
			case 'G' :
				gate = !gate;
				break;
			case 'M' :
				sed = !sed;
				break;
			case 'T' :
				ok_tell = !ok_tell;
				break;
			case 'X' :
				aff_inexport = !aff_inexport;
				break;
			case 'P' :
				aff_popsmtp = !aff_popsmtp;
				break;
			default :
				texte (T_ERR + 0);
				ok = 0;
				break;
			}
			
			if (ok)
				maj_options();
			option_display();
		}
		else
			retour_mbl ();
		break;
	}
}


static void header_edit (void)
{
	switch (pvoie->niv3)
	{
	case 20:
		if ((teste_espace ()) && (*indd) && (*indd == '#' || isdigit (*indd)))
		{
			if ((pvoie->nmess = lit_chiffre (1)) != 0L)
			{
				if (ch_record (ptmes, pvoie->nmess, '\0'))
				{
					entete_liste ();
					aff_status (ptmes);
					texte (T_MBL + 39);
					texte (T_MBL + 27);
					ch_niv3 (21);
					break;
				}
				else
				{
					ptmes->numero = pvoie->nmess;
					texte (T_ERR + 10);
				}
			}
			else
				texte (T_ERR + 3);
		}
		else
			texte (T_ERR + 3);
		retour_mbl ();
		break;
	case 21:
		if (*indd)
		{
			modif_header ();
			entete_liste ();
			aff_status (ptmes);
			texte (T_MBL + 39);
			texte (T_MBL + 27);
		}
		else
			retour_mbl ();
		break;
	}
}


static void modif_header (void)
{
	char c;
	int i;
	char *sptr;
	unsigned num_indic;
	ind_noeud *noeud;

	while_space ();
	c = toupper (*indd);
	incindd ();
	switch (c)
	{
	case 'T':					/* Desti */
		if ((sptr = lit_ind (indd)) != NULL)
		{
			noeud = insnoeud (ptmes->desti, &num_indic);
			if (ptmes->status == 'N')
				--(noeud->nbnew);
			--(noeud->nbmess);
			strcpy (ptmes->desti, sptr);
			ptmes->status = 'N';
			maj_rec (pvoie->nmess, ptmes);
			num_indic = insarbre (ptmes);
			chg_mess (num_indic, ptmes->numero);
			ins_iliste (ptmes);
		}
		else
			texte (T_ERR + 0);
		break;
	case 'V':					/* Via */
		if (*indd)
		{
			if (*indd == '.')
				*ptmes->bbsv = '\0';
			else
				strn_cpy (39, ptmes->bbsv, indd);
			clear_fwd (ptmes->numero);
			for (i = 0; i < NBMASK; i++)
			{
				ptmes->forw[i] = ptmes->fbbs[i] = '\0';
			}
			if (ptmes->type == 'B')
				ptmes->status = '$';
			else
			{
				if (ptmes->status != 'N')
				{
					/* increment counter of new messages */
					noeud = insnoeud (ptmes->desti, &num_indic);
					++(noeud->nbnew);
					aff_msg_cons ();
				}
				ptmes->status = 'N';
			}
			if (*ptmes->bbsv)
			{
				swapp_bbs (ptmes);
				if (cherche_route (ptmes))
				{
					texte (T_MBL + 41);
				}
				if (test_forward (1))
					ins_fwd (ptmes);
				cr ();
			}
			maj_rec (pvoie->nmess, ptmes);
		}
		break;
	case 'F':					/* Exped */
		if (*indd)
		{
			if ((sptr = lit_ind (indd)) != NULL)
			{
				strcpy (ptmes->exped, sptr);
				maj_rec (pvoie->nmess, ptmes);
			}
			else
				texte (T_ERR + 0);
		}
		break;
	case 'I':					/* Titre */
		if (*indd)
		{
			n_cpy (60, ptmes->titre, indd);
			maj_rec (pvoie->nmess, ptmes);
		}
		break;
	case 'Y':					/* Type */
		if (*indd)
		{
			*indd = toupper (*indd);
			if (*indd == 'A' || *indd == 'B' || *indd == 'P' || *indd == 'T')
			{
				ptmes->type = *indd;
				maj_rec (pvoie->nmess, ptmes);
			}
			else
				texte (T_ERR + 0);
		}
		break;
	case 'B':					/* Bid */
		if (*indd)
		{
			strn_cpy (12, ptmes->bid, indd);
			maj_rec (pvoie->nmess, ptmes);
		}
		break;
	case 'S':					/* Status */
		if (*indd)
		{
			*indd = toupper (*indd);

			noeud = insnoeud (ptmes->desti, &num_indic);
			if ((ptmes->status == 'N') && (*indd != 'N'))
			{
				--(noeud->nbnew);
			}
			else if ((ptmes->status != 'N') && (*indd == 'N'))
			{
				++(noeud->nbnew);
			}

			if ((ptmes->status != 'A') && (ptmes->status != 'K') &&
				((*indd == 'A') || (*indd == 'K')))
			{
				--(noeud->nbmess);
				chg_mess (0xffff, ptmes->numero);
			}
			else if (((ptmes->status == 'A') || (ptmes->status == 'K')) &&
					 (*indd != 'A') && (*indd != 'K'))
			{
				++(noeud->nbmess);
				chg_mess (num_indic, ptmes->numero);
			}

			if (*indd == 'Y' || *indd == 'F' || *indd == 'N' || *indd == '$' || *indd == 'K' || *indd == 'A')
			{
				ptmes->status = *indd;
				/*
				   if (ptmes->status == 'H') {
				   for (i = 0 ; i < NBMASK ; i++)
				   ptmes->fbbs[i] = '\0';
				   }
				 */
				maj_rec (pvoie->nmess, ptmes);
				aff_msg_cons ();
			}
			else
				texte (T_ERR + 0);
		}
		break;
	default:					/* Erreur */
		texte (T_ERR + 0);
		break;
	}
}


void ch_info (void)
{
	FILE *fptr;
	unsigned r = pvoie->ncur->coord;

	if (r == 0xffff)
		dump_core ();
	fptr = ouvre_nomenc ();
	fseek (fptr, ((long) r) * ((long) sizeof (info)), 0);
	fwrite ((char *) &(pvoie->finf), (int) sizeof (info), 1, fptr);
	ferme (fptr, 19);
}


static char *lit_ind (char *indic)
{
	while ((*indic) && (!ISGRAPH (*indic)))
		++indic;
	strupr (indic);
	if (strlen (indic) > 6)
		return (NULL);
	return (indic);
}


int tstqra (qra)
	 char *qra;
{
	if (strlen (qra) == 6)
		return (isalpha (qra[0]) && isalpha (qra[1]) &&
				isdigit (qra[2]) && isdigit (qra[3]) &&
				isalpha (qra[4]) && isalpha (qra[5]));
	else
		return (0);
}


static int lit_locator (void)
{
	char s[80];

	if (ISGRAPH (*indd))
	{
		strupr (epure (s, 6));
		if ((*s == '.') || tstqra (s))
		{
			if (*s == '.')
			{
				s[0] = '?';
				s[1] = '\0';
			}
			strcpy (pvoie->finf.qra, s);
			return (2);
		}
		else
		{
			texte (T_NOM + 14);
		}
	}
	return (1);
}


static int lit_zip (void)
{
	char s[80];

	if (ISGRAPH (*indd))
	{
		strupr (epure (s, 8));
		if (*s == '.')
			*s = '\0';
		strcpy (pvoie->finf.zip, s);
		user_wp (&pvoie->finf);
		/* pvoie->wp = 1; */
		return (2);
	}
	return (1);
}


static int lit_qth (void)
{
	char s[80];

	if (ISGRAPH (*indd))
	{
		epure (s, 30);
		if (*s == '.')
			*s = '\0';
		strcpy (pvoie->finf.ville, s);
		user_wp (&pvoie->finf);
		/* pvoie->wp = 1; */
		return (2);
	}
	return (1);
}


static int lit_homebbs (void)
{
	char s[80];
	char temp[80];
	char *ptr;

	if (ISGRAPH (*indd))
	{
		strupr (epure (s, 40));
		if (*s == '.')
			*s = '\0';

		/* Seul l'indicatif est enregistre */
		ptr = strchr (s, '.');
		if (ptr)
			*ptr = '\0';

		/* Pas de SSID */
		ptr = strchr (s, '-');
		if (ptr)
			*ptr = '\0';

		strcpy (temp, s);
		if (find (temp))
		{
			/* Seul l'indicatif est enregistre */
			strcpy (pvoie->finf.home, s);
			user_wp (&pvoie->finf);
			/* pvoie->wp = 1; */
			return (2);
		}
		else
			texte (T_ERR + 7);
	}
	return (1);
}


static int lit_prenom (void)
{
	info frec;

	if (ISGRAPH (*indd))
	{
		ch_prenom (pvoie->ncur, &frec);
		strcpy (pvoie->finf.prenom, frec.prenom);
		return (2);
	}
	return (1);
}


int mbl_name (void)
{
	int modif = 0;
	int error = 0;

	switch (pvoie->niv3)
	{
	case 0:
		if (toupper (*indd) == 'P')
		{
			int p = no_port (voiecur);

			if ((p_port[p].typort == TYP_MOD) || (p_port[p].typort == TYP_TCP))
			{
				if (read_only ())
				{
					modif = 1;
					break;
				}
				out ("Enter old password :", 20);
				maj_niv (N_MOD, 3, 0);
			}
			else
				error = 1;
		}
		else if (toupper (*indd) == 'L')
		{
			if (read_only ())
			{
				modif = 1;
				break;
			}
			++indd;
			if (teste_rep (sup_ln (indd)))
			{
				incindd ();
				modif = lit_locator ();
			}
			else
			{
				texte (T_NOM + 11);
				ch_niv3 (2);
			}
		}
		else if (toupper (*indd) == 'Q')
		{
			if (read_only ())
			{
				modif = 1;
				break;
			}
			++indd;
			if (teste_rep (sup_ln (indd)))
			{
				incindd ();
				modif = lit_qth ();
			}
			else
			{
				texte (T_NOM + 8);
				ch_niv3 (5);
			}
		}
		else if (toupper (*indd) == 'Z')
		{
			if (read_only ())
			{
				modif = 1;
				break;
			}
			++indd;
			if (teste_rep (sup_ln (indd)))
			{
				incindd ();
				modif = lit_zip ();
			}
			else
			{
				texte (T_MBL + 54);
				ch_niv3 (3);
			}
		}
		else if (toupper (*indd) == 'H')
		{
			if (read_only ())
			{
				modif = 1;
				break;
			}
			++indd;
			if (teste_rep (sup_ln (indd)))
			{
				incindd ();
				modif = lit_homebbs ();
			}
			else
			{
				texte (T_MBL + 53);
				ch_niv3 (4);
			}
		}
		else if (!ISGRAPH (*indd))
		{
			if (read_only ())
			{
				modif = 1;
				break;
			}
			if (teste_rep (sup_ln (indd)))
			{
				incindd ();
				modif = lit_prenom ();
			}
			else
			{
				texte (T_MBL + 9);
				ch_niv3 (1);
			}
		}
		else
			error = 1;
		break;

	case 1:
		modif = lit_prenom ();
		break;

	case 2:
		modif = lit_locator ();
		break;

	case 3:
		modif = lit_zip ();
		break;

	case 4:
		modif = lit_homebbs ();
		break;
	case 5:
		modif = lit_qth ();
		break;
	}
	if (modif)
	{
		if (modif == 2)
		{
			ch_info ();
			texte (T_MBL + 10);
		}
		retour_mbl ();
	}
	return (error);
}


static void ch_private (ind_noeud * noeud, info * frec)
{
	FILE *fptr;

	fptr = ouvre_nomenc ();
	if (noeud->coord == 0xffff)
		dump_core ();
	fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
	fread (frec, sizeof (info), 1, fptr);
	/* strupr (indd); */
	if (!iscntrl (*indd))
	{
		if (*indd == '.')
			*indd = '\0';
		epure (frec->priv, 12);
		fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
		fwrite (frec, sizeof (info), 1, fptr);
	}
	ferme (fptr, 36);
}


static void ch_home (ind_noeud * noeud, info * frec)
{
	char *ptr;
	char temp[80];
	FILE *fptr;

	if (noeud->coord == 0xffff)
		dump_core ();
	fptr = ouvre_nomenc ();
	fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
	fread (frec, sizeof (info), 1, fptr);
	if (!iscntrl (*indd))
	{
		strupr (indd);
		if (*indd == '.')
			*indd = '\0';

		ptr = strchr (indd, '.');
		if (ptr)
			*ptr = '\0';

		ptr = strchr (indd, '-');
		if (ptr)
			*ptr = '\0';

		strn_cpy (40, temp, indd);
		if (find (temp))
		{
			epure (frec->home, 40);
			fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
			fwrite (frec, sizeof (info), 1, fptr);
			user_wp (frec);
			/* pvoie->wp = 1; */
		}
		else
			texte (T_ERR + 7);
	}
	ferme (fptr, 36);
}


static void ch_zip (ind_noeud * noeud, info * frec)
{
	FILE *fptr;
	char *scan;

	if (noeud->coord == 0xffff)
		dump_core ();
	fptr = ouvre_nomenc ();
	fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
	fread (frec, sizeof (info), 1, fptr);
	strupr (indd);
	if (!iscntrl (*indd))
	{
		if (*indd == '.')
			*indd = '\0';
		epure (frec->zip, 8);
		scan = frec->zip;
		while (*scan)
		{
			if (*scan == ' ')
				*scan = '-';
			++scan;
		}
		fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
		fwrite (frec, sizeof (info), 1, fptr);
		user_wp (frec);
		/* pvoie->wp = 1; */
	}
	ferme (fptr, 36);
}


static void ch_password (ind_noeud * noeud, info * frec)
{
	FILE *fptr;

	if (noeud->coord == 0xffff)
		dump_core ();
	fptr = ouvre_nomenc ();
	fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
	fread (frec, sizeof (info), 1, fptr);
	strupr (indd);
	if (!iscntrl (*indd))
	{
		if (*indd == '.')
			*indd = '\0';
		epure (frec->pass, 12);
		fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
		fwrite (frec, sizeof (info), 1, fptr);
	}
	ferme (fptr, 36);
}


static void ch_prenom (ind_noeud * noeud, info * frec)
{
	FILE *fptr;
	char *scan;

	if (noeud->coord == 0xffff)
		dump_core ();
	fptr = ouvre_nomenc ();
	fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
	fread (frec, sizeof (info), 1, fptr);
	if (!iscntrl (*indd))
	{
		if (*indd == '.')
			*indd = '\0';
		epure (frec->prenom, 12);
		scan = frec->prenom;
		while (*scan)
		{
			if (*scan == ' ')
				*scan = '-';
			++scan;
		}
		fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
		fwrite (frec, sizeof (info), 1, fptr);

		user_wp (frec);
		/* pvoie->wp = 1; */
	}
	ferme (fptr, 36);
}


static void user_edit (void)
{
	int voie;
	char s[80];
	char *ind, *ptr;
	indicat indic;
	unsigned num_indic;
	FILE *fptr;
	info frec;

	sup_ln (indd);
	switch (pvoie->niv3)
	{
	case 0:
		++indd;
		if ((teste_espace ()) && (*indd) && find (indd))
		{
			/*
			   if (num_voie(indd) != -1) {
			   texte(T_MBL + 24) ;
			   retour_mbl() ;
			   break ;
			   }
			 */
			pvoie->emis = insnoeud (indd, &num_indic);
			if (pvoie->emis->coord == 0xffff)
			{
				var_cpy (0, indd);
				texte (T_MBL + 29);
			}
			else
			{
				fptr = ouvre_nomenc ();
				fseek (fptr, (long) pvoie->emis->coord * sizeof (frec), 0);
				fread (&frec, sizeof (info), 1, fptr);
				ferme (fptr, 39);
				texte (T_MBL + 11);
				affiche_user (&frec, 1);
				var_cpy (0, indd);
				texte (T_MBL + 30);
			}
			ch_niv3 (1);
			break;
		}
		else
			texte (T_ERR + 7);
		retour_mbl ();
		break;
	case 1:
		if (toupper (*indd) == Oui)
		{
			if (pvoie->emis->coord == 0xffff)
			{
				pvoie->emis->coord = rinfo++;
				/*      cprintf("Rinfo : %ld\r\n", rinfo) ; */
				ind = s;
				ptr = pvoie->emis->indic;
				pvoie->emis->val = 1;
				/*
				   *ind++ = *ptr++;
				   if (pvoie->emis->lettre)
				   *ind++ = pvoie->emis->lettre ;
				 */
				while ((*ind++ = *ptr++) != '\0');
				indic.num = extind (s, indic.call);
				init_info (&frec, &indic);
				if (pvoie->emis->coord == 0xffff)
					dump_core ();
				fptr = ouvre_nomenc ();
				fseek (fptr, (long) pvoie->emis->coord * ((long) sizeof (info)), 0);
				fwrite ((char *) &frec, (int) sizeof (info), 1, fptr);
				ferme (fptr, 40);

				texte (T_MBL + 11);
				affiche_user (&frec, 1);
				texte (T_MBL + 28);
				ch_niv3 (2);
				break;
			}
			else
			{
				if (pvoie->emis->coord == 0xffff)
					dump_core ();
				fptr = ouvre_nomenc ();
				fseek (fptr, (long) pvoie->emis->coord * ((long) sizeof (info)), 0);
				fread ((char *) &frec, sizeof (info), 1, fptr);
				*(frec.indic.call) = '\0';
				fseek (fptr, (long) pvoie->emis->coord * ((long) sizeof (info)), 0);
				fwrite ((char *) &frec, (int) sizeof (info), 1, fptr);
				ferme (fptr, 41);
				pvoie->emis->coord = 0xffff;
				retour_mbl ();
			}
		}
		else
		{
			if (pvoie->emis->coord == 0xffff)
				retour_mbl ();
			else
			{
				texte (T_MBL + 28);
				ch_niv3 (2);
			}
		}
		break;
	case 2:
		if (*indd)
		{
			fptr = ouvre_nomenc ();
			fseek (fptr, (long) pvoie->emis->coord * sizeof (frec), 0);
			fread (&frec, sizeof (info), 1, fptr);
			ferme (fptr, 39);
			modif_user (&frec);
			texte (T_MBL + 11);
			affiche_user (&frec, 1);
			texte (T_MBL + 28);
			for (voie = 0; voie < NBVOIES; ++voie)
			{
				if (svoie[voie]->sta.connect &&
				indcmp (svoie[voie]->sta.indicatif.call, frec.indic.call))
				{
					svoie[voie]->finf = frec;
				}
			}
		}
		else
			retour_mbl ();
		break;
	}
}


static void modif_user (info * frec)
{
	char c;

	while_space ();
	c = toupper (*indd);
	incindd ();
	switch (c)
	{
	case 'B':
		ch_bit (pvoie->emis, frec, F_BBS, '\0');
		break;
	case 'E':
		ch_bit (pvoie->emis, frec, F_EXC, '\0');
		break;
	case 'G':
		if (isdigit (*indd))
		{
			ch_language (atoi (indd), pvoie->emis, frec);
		}
		break;
	case 'F':
		ch_bit (pvoie->emis, frec, F_PMS, '\0');
		break;
	case 'H':
		ch_home (pvoie->emis, frec);
		break;
	case 'I':
		ch_bit (pvoie->emis, frec, F_NEW, '\0');
		break;
	case 'L':
		ch_bit (pvoie->emis, frec, F_LOC, '\0');
		break;
	case 'M':
		ch_bit (pvoie->emis, frec, F_MOD, '\0');
		break;
	case 'N':
		ch_prenom (pvoie->emis, frec);
		break;
	case 'P':
		ch_bit (pvoie->emis, frec, F_PAG, '\0');
		break;
	case 'R':
		ch_bit (pvoie->emis, frec, F_PRV, '\0');
		break;
	case 'S':
		ch_bit (pvoie->emis, frec, F_SYS, '\0');
		break;
	case 'U':
		ch_bit (pvoie->emis, frec, F_UNP, '\0');
		break;
	case 'V':
		ch_private (pvoie->emis, frec);
		break;
	case 'W':
		ch_password (pvoie->emis, frec);
		break;
	case 'X':
		ch_bit (pvoie->emis, frec, F_EXP, '\0');
		break;
	case 'Z':
		ch_zip (pvoie->emis, frec);
		break;
	default:					/* Erreur */
		texte (T_ERR + 0);
		break;
	}
}


void maj_rec (long nomess, bullist * pbul)
{
	mess_noeud *mptr = findmess (nomess);

	if (mptr)
	{
		ouvre_dir ();
		/********* Test overflow **********
		if ((long) mptr->noenr > maxrec) exit_prg(-1901);
		 ********** Fin du test ***********/
		write_dir (mptr->noenr, pbul);
		ferme_dir ();
	}
}
