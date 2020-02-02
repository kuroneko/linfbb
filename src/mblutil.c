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
 * MBLUTIL.C
 *
 */

#include <serv.h>
#include <stdarg.h>

#ifdef TRACE
static int fd_trace = -1;

open_trace ()
{

	fd_trace = open ("TRACE.MON", O_WRONLY | O_APPEND | O_CREAT | O_TEXT, S_IREAD | S_IWRITE);

}

close_trace ()
{
	if (fd_trace != -1)
		close (fd_trace);
}

static trace (char *chaine, int lg, int date)
{
	char *ptr = chaine;
	int i;
	long temps;
	struct tm *sdate;
	char cdate[20];
	static char ret = '\n';


	if (fd_trace == -1)
		return;

	for (i = 0; i < lg; i++)
	{
		if (*ptr == '\r')
			*ptr = '\n';
		++ptr;
	}

	write (fd_trace, chaine, lg);
	if (date)
	{
		temps = time (NULL);
		sdate = localtime (&temps);
		sprintf (cdate, " (%02d:%02d:%02d)", sdate->tm_hour, sdate->tm_min, sdate->tm_sec);
		write (fd_trace, cdate, 11);
	}
}

#endif

void force_deconnexion (int voie, int mode)
{
	df ("force_deconnexion", 2);

	if ((p_port[no_port (voie)].typort == TYP_HST) && (P_TOR (voie)))
	{
		/* WinDebug("Immediate disconnect request\r\n"); */
		tnc_commande (voie, "!DD", SNDCMD);
		dec_voie (voie);
	}
	else
	{
		deconnexion (voie, mode);
		deconnexion (voie, mode);
	}

	ff ();
}


void debug_mode (void)
{
	if (debug_on)
	{
		if (!debug_fptr)
			debug_fptr = fopen ("DEBUG.BUG", "at");
	}
	else
	{
		if (debug_fptr)
			fclose (debug_fptr);
		debug_fptr = NULL;
	}
}

int virt_canal (int nocan)
{
	int canal;

	if (nocan > 1)
		canal = nocan - 1;
	else if (nocan == 1)
		canal = 99;
	else
		canal = 0;
	return (canal);
}

void aff_chaine (int color, int xpos, int ypos, char *chaine)
{
#ifdef __FBBDOS__
	struct text_info sav_fen;

	df ("aff_chaine", 5);

	deb_io ();
	gettext_info (&sav_fen);
	fen_haut (color);
	curoff ();
	gotoxy (xpos, ypos);
	cputs (chaine);
	puttext_info (&sav_fen);
	fin_io ();
	ff ();
#endif
}

int nbport (void)
{
	int nbp = 0, port;

	df ("nbport", 0);

	for (port = 1; port < NBPORT; port++)
		if (p_port[port].pvalid)
			nbp++;
	ff ();
	return (nbp);
}


int read_only (void)
{
	if (pvoie->read_only)
	{
		outln ("*** Error : read-only mode", 26);
		/* texte(T_ERR + 0); */
		return (1);
	}

	return (0);
}


char *lf_to_cr (char *texte)
{
	char *ptr = texte;
	char *optr = texte;

	while (*ptr)
	{
		if ((*ptr == '\r') && (*(ptr + 1) == '\n'))
		{
			++ptr;
		}
		else
		{
			if (*ptr == '\n')
				*optr++ = '\r';
			else
				*optr++ = *ptr;
			++ptr;
		}
	}
	*optr = '\0';
	return (texte);
}


void while_space (void)
{
	while (isspace (*indd))
		++indd;
}


int strmatch (char *chaine, char *masque)
{
	while (1)
	{
		switch (*masque)
		{
		case '\0':
			return (toupper (*masque) == toupper (*chaine));
		case '&':
			if ((*chaine == '\0') || (*chaine == '.'))
				return (1);
			break;
		case '?':
			if (!isalnum (*chaine))
				return (0);
			break;
		case '#':
			if ((*chaine != '#') && (!isdigit (*chaine)))
				return (0);
			break;
		case '@':
			if (!isalpha (*chaine))
				return (0);
			break;
		case '=':
			if (!ISGRAPH (*chaine))
				return (0);
			break;
		case '*':
			while (*++masque == '*')
				;
			if (*masque == '\0')
				return (1);
			while (!strmatch (chaine, masque))
				if (*++chaine == '\0')
					return (0);
			break;
		default:
			if ((toupper (*chaine)) != (toupper (*masque)))
				return (0);
			break;
		}
		++chaine;
		++masque;
	}
}


void ch_niv3 (int niveau)
{
	maj_niv (pvoie->niv1, pvoie->niv2, niveau);
}


void ch_niv2 (int niveau)
{
	maj_niv (pvoie->niv1, niveau, 0);
}


void ch_niv1 (int niveau)
{
	maj_niv (niveau, 0, 0);
}


int droits (unsigned int droit)
{
	if (voiecur == CONSOLE)
		return (1);
	return (droit & pvoie->droits);
}


void ch_bit (ind_noeud * noeud, info * frec, unsigned bit, char rep)
{
	FILE *fptr;
	int c = toupper (rep);

	if ((c != Oui) && (c != Non) && (c != '\0'))
		return;

	if (pvoie->read_only)
		return;

	fptr = ouvre_nomenc ();
	fseek (fptr, (long) noeud->coord * sizeof (info), 0);
	fread (frec, sizeof (info), 1, fptr);
	rep = toupper (rep);
	if (c == Oui)
	{
		frec->flags |= bit;
	}
	else if (c == Non)
	{
		frec->flags &= (~bit);
	}
	else if (c == '\0')
	{
		if (frec->flags & bit)
		{
			frec->flags &= (~bit);
		}
		else
		{
			frec->flags |= bit;
		}
	}
	if (noeud->coord == 0xffff)
		dump_core ();
	fseek (fptr, (long) noeud->coord * sizeof (info), 0);
	fwrite (frec, sizeof (info), 1, fptr);
	ferme (fptr, 42);
	noeud->val = (uchar) (EXC (frec->flags) == 0);
}


void cmd_err (char *ptri)
{
	int i = 0;
	char *ptr = varx[0];

	while (ISGRAPH (*ptri))
	{
		if (++i == 40)
			break;
		else
			*ptr++ = *ptri++;
	}
	*ptr = '\0';
	texte (T_ERR + 1);
	*varx[0] = '\0';
	if ((FOR (pvoie->mode)) || (++pvoie->nb_err == MAX_ERR))
		pvoie->deconnect = 6;
	else
		retour_menu (pvoie->niv1);
}


void strn_cpy (int longueur, char *dest, char *orig)
{
	while (longueur--)
	{
		if (!ISPRINT (*orig))
			break;
		*dest++ = toupper (*orig);
		++orig;
	}
	*dest = '\0';
}


void n_cpy (int longueur, char *dest, char *orig)
{
	while (longueur--)
	{
		if (*orig == '\0')
			break;
		*dest++ = *orig++;
	}
	*dest = '\0';
}


void init_recliste (int voie)
{
	tlist *ptlist = &svoie[voie]->recliste;

	ptlist->l = FALSE;
	ptlist->type = '\0';
	ptlist->status = '\0';
	ptlist->route = '\0';
	*ptlist->exp = '\0';
	*ptlist->dest = '\0';
	*ptlist->bbs = '\0';
	*ptlist->find = '\0';
	ptlist->debut = 0L;
	ptlist->fin = 0x7fffffffL;
	ptlist->avant = 0L;
	ptlist->apres = 0x7fffffffL;
	ptlist->last = 0x7fffffffL;

	if (*svoie[voie]->finf.filtre)
		strn_cpy (6, ptlist->dest, svoie[voie]->finf.filtre);
}


int droit_ok (bullist * lbul, int mode)
{

	/*
	 * mode 1 : consultation (lecture)
	 * mode 2 : suppression (ecriture)
	 * mode 3 : archivage
	 * mode 4 : idem mode 2 mais autorise tuer le message "Non lu"
	 */


	int kill_new = 0;

	if (mode == 4)
	{
		mode = 2;
		kill_new = 1;
	}

	if (mode == 3)
	{
		if (lbul->status == 'A')
			return (0);
		else
			return (1);
	}

	if (lbul->status == 'K')
	{
		if (mode == 2)
			return (0);
	}

	if (droits (SUPMES))
		return (1);

	if ((mode == 1) && (droits (CONSMES) || PRV (pvoie->finf.flags)))
		if ((strcmp (lbul->desti, "KILL") != 0) && (lbul->status != 'H'))
			return (1);

	if (lbul->status == 'H')
	{
		if ((SYS (pvoie->finf.flags)) || indcmp (lbul->exped, pvoie->sta.indicatif.call))
			return (1);
		else
			return (0);
	}

	if (strcmp (lbul->desti, "WP") == 0)
		return (0);

	switch (lbul->type)
	{
	case 'A':
	case 'P':
		switch (mode)
		{
		case 2:
			if (indcmp (lbul->desti, pvoie->sta.indicatif.call))
			{
				if ((kill_new) || (lbul->status != 'N'))
				{
					return (1);
				}
			}
			if (indcmp (lbul->exped, pvoie->sta.indicatif.call))
				return (1);
			break;
		case 1:
			if ((indcmp (lbul->desti, pvoie->sta.indicatif.call)) ||
				(indcmp (lbul->exped, pvoie->sta.indicatif.call)))
				return (1);
			break;
		}
		break;
	case ' ':
	case 'B':
		if (mode == 2)
		{
			if ((indcmp (lbul->exped, pvoie->sta.indicatif.call)) ||
				(indcmp (lbul->desti, pvoie->sta.indicatif.call)))
				return (1);
		}
		else if (lbul->status == 'A')
			return (0);
		else if (strcmp (lbul->desti, "KILL") != 0)
			return (1);
		break;
	case 'T':
		if (lbul->status == 'A')
			return (0);
		else
			return (1);
	default:
		return (0);
	}
	return (0);
}


void retour_mbl (void)
{
	libere_tread (voiecur);
	libere_tlist (voiecur);
	pvoie->mbl = 1;
	pvoie->private_dir = 0;
	set_bs (voiecur, TRUE);
	retour_menu (N_MBL);
}


int teste_rep (char *chaine)
{
	if (iscntrl (*chaine))
		return (0);
	if (*chaine == ' ')
		return (1);
	else
	{
		texte (T_ERR + 2);
		return (0);
	}
}


long lit_chiffre (int ajuste)
{
	long chif;
	char *ptr;

	if (*indd == '\0')
		return (0L);
	teste_espace ();
	if (isdigit (*indd))
	{
		ptr = indd;
		while ((*indd) && (isdigit (*indd)))
			++indd;
		chif = atol (ptr);
		if ((ajuste) && (chif < ECART))
			chif += (1000L * (long) pvoie->finf.on_base);
		return (chif);
	}
	else if (*indd)
	{
		if (*indd == '#')
		{
			++indd;
			return (ptmes->numero);
		}
		else
			texte (T_ERR + 3);
	}
	return (0L);
}


int is_espace (char *ptr)
{
	int espace = (*ptr == ' ');

	while ((*ptr) && (*ptr == ' '))
		++ptr;
	if (ISPRINT (*ptr))
		return (espace);
	else
		return (1);
}


int teste_espace (void)
{
	int ok = 0;

	if (*indd)
	{
		if (*indd == ' ')
			ok = 1;
		while ((*indd) && (!ISGRAPH (*indd)))
			++indd;
	}
	return (ok);
}


long supp_header (FILE * fptr, int ok)
{
	int c, first = 1, nb = 0, call = 0;
	char ligne[90];
	char *ptr = ligne;
	int flag = FALSE;
	long record = 0L;
	short int postexte = 0;

	record = ftell (fptr);

	while ((c = fgetc (fptr)) != EOF)
	{
		if ((flag) && (c == '\n'))
		{
			record = ftell (fptr);
			postexte = 0;
			call = 0;
			flag = FALSE;
		}
		else
		{
			switch (call)
			{
			case 0:
				break;
			case 1:
				if (isalnum (c))
				{
					*ptr++ = c;
					nb++;
					call = 2;
				}
				break;
			case 2:
				if (isalnum (c))
				{
					*ptr++ = c;
					nb++;
				}
				else
				{
					*ptr++ = '!';
					nb++;
					call = 0;
					if (nb >= 65)
					{
						outln (ligne, nb);
						nb = 0;
						ptr = ligne;
						first = 2;
					}
				}
				break;
			}
			if (postexte == 0)
			{
				if (c != 'R')	/*return(record) */
					break;
				else
					flag = TRUE;
			}
			if ((postexte == 1) && (flag) && (c != ':'))	/*return(record) */
				break;
			++postexte;
		}
		if ((ok) && (flag) && (c == '@'))
		{
			if (first)
			{
				if (first == 1)
					out ("Path: !", 7);
				else
					out ("      !", 7);
				first = 0;
			}
			call = 1;
		}
	}
	if (nb)
		outln (ligne, nb);
	/*  return(ftell(fptr)) ; */
	return (record);
}


bullist *ch_record (bullist * list, long no, char status)
{
	mess_noeud *lptr;
	static bullist ltemp;
	bullist *lbul;
	ind_noeud *noeud;
	char temp = '\0';
	int ok = 0;
	int kill_new = 0;
	int no_type = 0;

	if (status == 'Z')
	{
		no_type = 1;
		status = 'A';
	}

	if (status & 0x80)
	{
		kill_new = 1;
		status &= 0x7f;
	}

	if (list)
		lbul = list;
	else
		lbul = &ltemp;

	lbul->numero = no;

	if ((lptr = findmess (no)) != NULL)
	{
		ouvre_dir ();
		read_dir (lptr->noenr, lbul);
		if (lbul->type)
		{
			if (status == 'Y')
			{
				++lbul->nblu;
				temp = lbul->status;
			}

			if ((status) && (status != lbul->status))
			{
				switch (status)
				{
				case 'Y':
					if ((lbul->status == 'N') || (lbul->status == '$'))
					{
						if ((lbul->type == 'P') || (lbul->type == 'A'))
						{
							if (indcmp (pvoie->sta.indicatif.call, lbul->desti))
							{
								noeud = cher_noeud (lbul->desti);
								--(pvoie->ncur->nbnew);
							}
							else
								break;
						}
						else
						{
							if ((lbul->status == '$') ||
								(indcmp (pvoie->sta.indicatif.call, lbul->exped)))
								break;
						}
						temp = lbul->status;
						lbul->status = status;
					}
					break;
				case 'F':
					if (lbul->status == 'K')
						break;
					if (lbul->status == 'X')
						break;
					if (lbul->status == 'A')
						break;
					noeud = cher_noeud (lbul->desti);
					if (lbul->status == 'N')
						--(noeud->nbnew);
					if ((lbul->type == 'P') && (lbul->status == '$'))
						--(noeud->nbnew);
					chg_mess (0xffff, no);
					temp = lbul->status;
					lbul->status = status;
					break;
				case 'A':
					lbul->datech = 0L;
				case 'K':
					if ((!kill_new) && (lbul->datech))
					{
						if (!droit_ok (lbul, 2))
							break;
						else if (!droit_ok (lbul, 3))
							break;
					}
					noeud = cher_noeud (lbul->desti);
					if ((!kill_new) && (!droits (SUPMES)))
					{
						if ((indcmp (pvoie->sta.indicatif.call, lbul->desti)))
						{
							if (lbul->status == 'N')
								break;
						}
					}
					if (lbul->status == 'N')
					{
						if (noeud->nbnew > 0)
							--(noeud->nbnew);
					}
					/*
					   if (lbul->status == 'H') {
					   --nb_hold;
					   }
					 */
					if ((lbul->status != 'K') && (lbul->status != 'A'))
					{
						if (noeud->nbmess > 0)
							--(noeud->nbmess);
						chg_mess (0xffff, no);
					}
					temp = lbul->status;
					lbul->status = status;
					break;
				case 'H':
					if (!droit_ok (lbul, 2))
						break;
					noeud = cher_noeud (lbul->desti);
					if (((lbul->status != 'N') || (lbul->type != 'P')) &&
						((lbul->status != '$') || (lbul->type != 'B')))
						break;
					if (lbul->status == 'N')
					{
						--(noeud->nbnew);
					}
					if ((lbul->status != 'K') && (lbul->status != 'A'))
					{
						--(noeud->nbmess);
						/* chg_mess(0xffff, no); */
					}
					temp = lbul->status;
					lbul->status = status;
					break;
				}
			}
			if (temp)
			{
				if (lbul->datech)
					lbul->datech = time (NULL);
				if (no_type)
					lbul->type = '\0';
				write_dir (lptr->noenr, lbul);
				lbul->status = temp;
				aff_msg_cons ();
			}
			ok = 1;
		}
		ferme_dir ();
	}
	*ptmes = *lbul;
	if (ok)
		return (lbul);
	else
		return (NULL);
}

int entete_liste (void)
{
	return (texte (T_MBL + 36));
}


int indcmp (char *ind1, char *ind2)
{
	char *ptr1 = ind1;
	char *ptr2 = ind2;

	if (*ptr1 != *ptr2)
		return (FALSE);

	++ptr1;
	++ptr2;
	if ((!isalnum (*ptr1)) && (!isalnum (*ptr2)))
		return (TRUE);

	if (*ptr1++ != *ptr2++)
		return (FALSE);

	while ((isalnum (*ptr1)) || (isalnum (*ptr2)))
	{
		if (*ptr1++ != *ptr2++)
			return (FALSE);
	}
	return (TRUE);
}

/*
 * teste si ind2 est compris dans ind1
 * en tenant compte des champs de hierarchie
 *
 */
int hiecmp (char *ind1, char *ind2)
{
	char str[80];
	char *ptr;

	strcpy (str, ind1);

	for (;;)
	{
		if (strcmpi (str, ind2) == 0)
			return (TRUE);

		ptr = strrchr (str, '.');
		if (ptr == NULL)
			break;
		*ptr = '\0';
	}
	return (FALSE);
}


char *sup_ln (char *buf)
{
	int nb = strlen (buf);

	if (nb > 0)
	{
		while (nb--)
		{
			if (ISGRAPH (buf[nb]))
				break;
			if (buf[nb])
				buf[nb] = '\0';
		}
	}
	return (buf);
}


char *ch_slash (char *chaine)
{
	char *ptr = chaine;

	while (*ptr)
	{
		if (*ptr == '/')
			*ptr = '\\';
		++ptr;
	}
	return (chaine);
}


char *tot_path (char *nom_fic, char *source)
{
	int lg;
	char vdisk;
	static char temp[256];

	*temp = '\0';

	/*
	char *ptr;
	ptr = nom_fic;
	while (*ptr)
	{
		if (islower (*ptr))
			*ptr = toupper (*ptr);
		++ptr;
	}
	*/

	vdisk = pvoie->vdisk;
	if (nom_fic[1] == ':')
	{
		if (((voiecur == CONSOLE) || (pvoie->kiss == -2)) && (!pvoie->cmd_new))
		{
			vdisk = nom_fic[0] - 'A';
		}
		else
		{
			vdisk = nom_fic[0] - 'A';
			if (vdisk == 15)
				vdisk = 8;

			if (vdisk > 8)
				vdisk = 0;
			if ((vdisk < 8) && (*PATH[(int)vdisk] == '\0'))
				vdisk = 0;
		}
		nom_fic += 2;
	}

	if ((pvoie->kiss == -2) || ((droits (ACCESDOS)) && (pvoie->temp1 != N_YAPP) && (!pvoie->cmd_new)))
	{
		sprintf (temp, "%c:", vdisk + 'A');
#ifdef __linux__	
		if (*nom_fic != '/')
#else		
		if (*nom_fic != '\\')
#endif
			strcat (temp, source);
	}
	else
	{
		if (pvoie->temp1 == N_YAPP)
		{
			strcpy (temp, YAPPDIR);
		}
		else
		{
			if (vdisk == 8)
			{
				if (pvoie->finf.priv[1] != ':')
					sprintf (temp, "%c:%s", 'A' + getdisk (), pvoie->finf.priv);
				else
					strcpy (temp, pvoie->finf.priv);
			}
			else
				strcpy (temp, PATH[(int)vdisk]);
		}
#ifdef __linux__	
		if (((lg = strlen (temp)) > 1) && (temp[lg - 1] == '/'))
			temp[lg - 1] = '\0';
		if (*nom_fic != '/')
#else		
		if (((lg = strlen (temp)) > 1) && (temp[lg - 1] == '\\'))
			temp[lg - 1] = '\0';
		if (*nom_fic != '\\')
#endif
			strcat (temp, source);
	}
/* DEBUG F6BVP 
	printf ("tot_path() temp '%s' source '%s' nom_fic '%s'\n", temp, source, nom_fic);*/
	strcat (temp, nom_fic);
	strcpy(temp, long_filename(NULL, temp));
	vir_path (temp);
	return temp;
}


void tester_masque (void)
{
	char *t = pvoie->ch_temp;
	int cpt = 0;

	if (*indd == '\0')
		*t++ = '*';

	else
	{
		while (isalnum (*indd))
		{
			if (cpt++ < 7)
			{
				*t++ = toupper (*indd);
				++indd;
			}
			else
				break;
		}
	}
	*t = '\0';
}

char *comp_bid (char *bid)
{
	static char out_bid[BIDCOMP];
	char in_bid[BIDLEN];
	char *ibid, *obid;
	int i, fin = 0;

	for (i = 0; i < BIDLEN; i++)
	{
		if ((!fin) && (*bid))
		{
			in_bid[i] = (*bid - 32) & 0x3f;
		}
		else
		{
			fin = 1;
			in_bid[i] = '\0';
		}
		++bid;
	}

	ibid = in_bid;
	obid = out_bid;

	for (i = 0; i < BIDCOMP; i += 3)
	{
		obid[0] = (ibid[0] << 2) + (ibid[1] >> 4);
		obid[1] = (ibid[1] << 4) + (ibid[2] >> 2);
		obid[2] = (ibid[2] << 6) + (ibid[3]);
		ibid += 4;
		obid += 3;
	}

	return (out_bid);
}


void var_cpy (int novar, char *texte)
{
	if ((novar >= 0) && (novar < 10))
		n_cpy (80, varx[novar], texte);
}


void fbb_log (int voie, char event, char *texte)
{
	if (svoie[voie]->log)
		port_log (voie, 1, event, texte);
}


void port_log (int voie, int val, char event, char *texte)
{
	int lvoie;
	struct tm *sdate;
	char *ptr;
	long temps = time (NULL);
	char com[256], buf[80];

	if (comlog)
	{
		lvoie = (val) ? voie : 0;
		sdate = localtime (&temps);
		n_cpy (66, buf, sup_ln (texte));
		ptr = buf;
		while (*ptr)
		{						/* Supression des caracteres de controle */
			if (iscntrl (*ptr))
				*ptr = '_';
			++ptr;
		}
		sprintf (com, "%02d%02d%02d%02d%02d%02d%c%s\r\n",
		   sdate->tm_mon + 1, sdate->tm_mday, sdate->tm_hour, sdate->tm_min,
				 sdate->tm_sec, lvoie, event, buf);

		if (log_ptr == NULL)
			ouvre_log ();
		if (log_ptr)
			fputs (com, log_ptr);
	}
}

/* Table de conversion engendrée mécaniquement par Free «recode» 3.5
   pour séquence «IBM850..ISO-8859-1 (réversible)».  */
 
unsigned char const IBM850_ISO_8859_1[256] =
{
  0,   1,   2,   3,   4,   5,   6,   7,     /*   0 -   7  */
  8,   9,  10,  11,  12,  13,  14,  15,     /*   8 -  15  */
 16,  17,  18,  19,  20,  21,  22,  23,     /*  16 -  23  */
 24,  25,  26,  27,  28,  29,  30,  31,     /*  24 -  31  */
 32,  33,  34,  35,  36,  37,  38,  39,     /*  32 -  39  */
 40,  41,  42,  43,  44,  45,  46,  47,     /*  40 -  47  */
 48,  49,  50,  51,  52,  53,  54,  55,     /*  48 -  55  */
 56,  57,  58,  59,  60,  61,  62,  63,     /*  56 -  63  */
 64,  65,  66,  67,  68,  69,  70,  71,     /*  64 -  71  */
 72,  73,  74,  75,  76,  77,  78,  79,     /*  72 -  79  */
 80,  81,  82,  83,  84,  85,  86,  87,     /*  80 -  87  */
 88,  89,  90,  91,  92,  93,  94,  95,     /*  88 -  95  */
 96,  97,  98,  99, 100, 101, 102, 103,     /*  96 - 103  */
104, 105, 106, 107, 108, 109, 110, 111,     /* 104 - 111  */
112, 113, 114, 115, 116, 117, 118, 119,     /* 112 - 119  */
120, 121, 122, 123, 124, 125, 126, 127,     /* 120 - 127  */
199, 252, 233, 226, 228, 224, 229, 231,     /* 128 - 135  */
234, 235, 232, 239, 238, 236, 196, 197,     /* 136 - 143  */
201, 230, 198, 244, 246, 242, 251, 249,     /* 144 - 151  */
255, 214, 220, 248, 163, 216, 215, 159,     /* 152 - 159  */
225, 237, 243, 250, 241, 209, 170, 186,     /* 160 - 167  */
191, 174, 172, 189, 188, 161, 171, 187,     /* 168 - 175  */
155, 157, 141, 129, 139, 193, 194, 192,     /* 176 - 183  */
169, 150, 132, 140, 148, 162, 165, 151,     /* 184 - 191  */
156, 145, 147, 128, 142, 143, 227, 195,     /* 192 - 199  */
131, 144, 146, 133, 138, 153, 158, 164,     /* 200 - 207  */
240, 208, 202, 203, 200, 134, 205, 206,     /* 208 - 215  */
207, 137, 130, 136, 154, 166, 204, 152,     /* 216 - 223  */
211, 223, 212, 210, 245, 213, 181, 254,     /* 224 - 231  */
222, 218, 219, 217, 253, 221, 175, 180,     /* 232 - 239  */
173, 177, 149, 190, 182, 167, 247, 184,     /* 240 - 247  */
176, 168, 183, 185, 179, 178, 135, 160,     /* 248 - 255  */
};
  
/* Conversion table generated mechanically by Free `recode' 3.5
   for sequence ISO-8859-1..IBM850 (reversible).  */
 
unsigned char const ISO_8859_1_IBM850[256] =
{
  0,   1,   2,   3,   4,   5,   6,   7,     /*   0 -   7  */
  8,   9,  10,  11,  12,  13,  14,  15,     /*   8 -  15  */
 16,  17,  18,  19,  20,  21,  22,  23,     /*  16 -  23  */
 24,  25,  26,  27,  28,  29,  30,  31,     /*  24 -  31  */
 32,  33,  34,  35,  36,  37,  38,  39,     /*  32 -  39  */
 40,  41,  42,  43,  44,  45,  46,  47,     /*  40 -  47  */
 48,  49,  50,  51,  52,  53,  54,  55,     /*  48 -  55  */
 56,  57,  58,  59,  60,  61,  62,  63,     /*  56 -  63  */
 64,  65,  66,  67,  68,  69,  70,  71,     /*  64 -  71  */
 72,  73,  74,  75,  76,  77,  78,  79,     /*  72 -  79  */
 80,  81,  82,  83,  84,  85,  86,  87,     /*  80 -  87  */
 88,  89,  90,  91,  92,  93,  94,  95,     /*  88 -  95  */
 96,  97,  98,  99, 100, 101, 102, 103,     /*  96 - 103  */
104, 105, 106, 107, 108, 109, 110, 111,     /* 104 - 111  */
112, 113, 114, 115, 116, 117, 118, 119,     /* 112 - 119  */
120, 121, 122, 123, 124, 125, 126, 127,     /* 120 - 127  */
195, 179, 218, 200, 186, 203, 213, 254,     /* 128 - 135  */
219, 217, 204, 180, 187, 178, 196, 197,     /* 136 - 143  */
201, 193, 202, 194, 188, 242, 185, 191,     /* 144 - 151  */
223, 205, 220, 176, 192, 177, 206, 159,     /* 152 - 159  */
255, 173, 189, 156, 207, 190, 221, 245,     /* 160 - 167  */
249, 184, 166, 174, 170, 240, 169, 238,     /* 168 - 175  */
248, 241, 253, 252, 239, 230, 244, 250,     /* 176 - 183  */
247, 251, 167, 175, 172, 171, 243, 168,     /* 184 - 191  */
183, 181, 182, 199, 142, 143, 146, 128,     /* 192 - 199  */
212, 144, 210, 211, 222, 214, 215, 216,     /* 200 - 207  */
209, 165, 227, 224, 226, 229, 153, 158,     /* 208 - 215  */
157, 235, 233, 234, 154, 237, 232, 225,     /* 216 - 223  */
133, 160, 131, 198, 132, 134, 145, 135,     /* 224 - 231  */
138, 130, 136, 137, 141, 161, 140, 139,     /* 232 - 239  */
208, 164, 149, 162, 147, 228, 148, 246,     /* 240 - 247  */
155, 151, 163, 150, 129, 236, 231, 152,     /* 248 - 255  */
};
  
uchar *ansi_to_ibm(uchar *buffer, int len)
{
	uchar *ptr = buffer;
	
	while (len--)
	{
		*ptr = ISO_8859_1_IBM850[(int)*ptr];
		++ptr;
	}
	
	return buffer;
}

uchar *ibm_to_ansi(uchar *buffer, int len)
{
	uchar *ptr = buffer;
	
	while (len--)
	{
		*ptr = IBM850_ISO_8859_1[(int)*ptr];
		++ptr;
	}
	
	return buffer;
}

#undef fopen
#undef fclose
#undef fprintf

/*
void debuglog(char *fmt,...)
{
	time_t temps = time(NULL);
	va_list argptr;
	FILE *fptr = fopen("/tmp/debug.log", "a+");

	if (!fptr)
		return;

	fprintf (fptr, "\n%s", asctime (localtime (&temps)));

	va_start (argptr, fmt);
	vfprintf (fptr, fmt, argptr);
	va_end (argptr);
	
	fclose(fptr);
}

*/
