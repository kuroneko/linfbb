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
#include <fbb_conf.h>
/*
 * Module emulation WA7MBL
 */

static void aff_aide (FILE *);
static int user_dump (unsigned);
static int heardcmp (const void *, const void *);

/* Commande ? -> Help */

void help (char *cmd)
{
	if (*cmd)
		out_help (cmd);
	else
	{
		char str[10];

		strcpy (str, "?");
		out_help (str);
	}
	retour_menu (pvoie->niv1);
}


int out_help (char *cmde)
{
	char buffer[257];
	char str_aide[255];
	char *ptr;
	char *cmd;
	static char qm[2] = "?";
	FILE *fptr;
	int niveau;
	int position;
	int aide = FALSE;
	char command[80];

	n_cpy (78, command, cmde);
	cmd = command;
	
	pvoie->aut_nc = 1;

	if ((!SYS (pvoie->finf.flags)) && (!LOC (pvoie->finf.flags)) && (P_GUEST (voiecur)))
	{
		position = -1;
		cmd = qm;
	}
	else
		position = pvoie->niv1;

	sprintf (buffer, "LANG\\%s.HLP", nomlang + nlang * LG_LANG);
	if ((fptr = fopen (c_disque (buffer), "rt")) != NULL)
	{
		strupr (sup_ln (cmd));
		if (ISGRAPH (*cmd))
		{
			while (fgets (buffer, 256, fptr))
			{
				if (*buffer == '#')
					continue;
				if ((buffer[0] == '@') && (buffer[1] == '@'))
				{
					sscanf (buffer, "%*s %d %s", &niveau, str_aide);
					if (niveau == position)
					{
						for (ptr = strtok (str_aide, "|"); ptr; ptr = strtok (NULL, "|"))
						{
							if (strncmp (ptr, cmd, 80) == 0)
							{
								aff_aide (fptr);
								aide = TRUE;
								break;
							}
						}
						if (aide)
							break;
					}
				}
			}
		}
		ferme (fptr, 35);
	}
	if ((*cmd) && (!aide))
	{
		var_cpy (0, cmd);
		texte (T_ERR + 5);
	}
	return (aide);
}


static void aff_aide (FILE * fptr)
{
	char buffer[257];
	char *ptr;
	int nb;

	while (fgets (buffer, 256, fptr))
	{
		if (*buffer == '#')
			continue;
		nb = 0;
		if ((*buffer == '@') && (*(buffer + 1) == '@'))
			break;
		ptr = buffer;
		while (*ptr)
		{
			if (*ptr == '\n')
				*ptr = '\r';
			++ptr;
			++nb;
		}
		out (buffer, nb);
	}
}


/* Commande D -> DOS ou dump fichiers systeme */

int mbl_dump (void)
{
	int erreur = 0;
	unsigned masque = 0;

	switch (pvoie->niv3)
	{
	case 0:
		if (!ISGRAPH (*indd))
		{
			sup_ln (indd);
			if (*indd)
			{					/* Download */
				--indd;
				pvoie->temp1 = pvoie->niv1;
				maj_niv (N_DOS, 2, 0);
				send_file (1);
			}
			else
			{					/* DOS */
				if (miniserv & 2)
				{
					maj_niv (N_DOS, 0, 0);
					ptmes->date = time (NULL);
					pvoie->mbl = 0;
					texte (T_DOS + 8);
					dos ();
				}
				else
					return (1);
			}
			return (0);
		}
		if (droits (COSYSOP))
		{
			*indd = toupper (*indd);
			switch (*indd)
			{
			case 'B':
				masque = F_BBS;
				break;
			case 'E':
				masque = F_EXC;
				break;
			case 'F':
				masque = F_PMS;
				break;
			case 'L':
				masque = F_LOC;
				break;
			case 'M':
				masque = F_MOD;
				break;
			case 'P':
				masque = F_PAG;
				break;
			case 'S':
				masque = F_SYS;
				break;
			case 'U':
				masque = 0xffff;
				break;
			case 'X':
				masque = F_EXP;
				break;
			default:
				erreur = 1;
			}
			if (!erreur)
			{
				pvoie->temp2 = 0;
				pvoie->temp3 = masque;
				pvoie->enrcur = 0L;
				incindd ();
				strn_cpy (9, pvoie->appendf, sup_ln (indd));

				if (user_dump (masque))
					ch_niv3 (1);
				else
					retour_mbl ();
				return (0);
			}
		}
		break;

	case 1:
		if (user_dump (pvoie->temp3) == 0)
		{
			pvoie->temp3 = 0;
			retour_mbl ();
		}
		return (0);
		/*
		   varx[0][0] = 'D' ;
		   strn_cpy(79, varx[0] + 1, indd) ;
		   texte(T_ERR + 1) ;
		   retour_mbl() ; */
	}
	return (1);
}


char *strflags (info * frec)
{
#define NB_FLAG	11
	int i;
	static char flag[NB_FLAG + 1];

	for (i = 0; i < NB_FLAG; i++)
		flag[i] = '.';
	flag[NB_FLAG] = '\0';

	if (NEW (frec->flags))
		flag[10] = 'I';
	if (UNP (frec->flags))
		flag[9] = 'U';
	if (MOD (frec->flags))
		flag[8] = 'M';
	if (EXC (frec->flags))
		flag[7] = 'E';
	if (PMS (frec->flags))
		flag[6] = 'F';
	if (LOC (frec->flags))
		flag[5] = 'L';
	if (EXP (frec->flags))
		flag[4] = 'X';
	if (SYS (frec->flags))
		flag[3] = 'S';
	if (BBS (frec->flags))
		flag[2] = 'B';
	if (PAG (frec->flags))
		flag[1] = 'P';
	if (PRV (frec->flags))
		flag[0] = 'R';

	return (flag);
}

void affiche_user (info * frec, int mode)
{
	char s[200];
	char *flag = strflags (frec);

	sprintf (s, "%-6s-%-2d %s/%s %5ld %s %-12s %-12s %-12s",
			 frec->indic.call, frec->indic.num,
			 date_mbl (frec->hcon), heure_mbl (frec->hcon),
			 frec->nbcon, flag,
			 frec->prenom, frec->pass, frec->priv);
	outln (s, strlen (s));
	if (mode)
	{
		var_cpy (0, frec->home);
		var_cpy (1, frec->zip);
		texte (T_MBL + 55);
	}
}


static int user_dump (unsigned masque)
{
	FILE *fptr;
	info frec;
	int ind = FALSE;
	int retour = 0;
	int match;
	char w_masque[10];

	pvoie->sr_mem = pvoie->seq = FALSE;

	strcpy (w_masque, pvoie->appendf);
	match = (int) w_masque[0];

	if ((match) && (find (pvoie->appendf)))
		ind = TRUE;

	fptr = ouvre_nomenc ();
	fseek (fptr, pvoie->enrcur, 0);
	while (fread ((char *) &frec, sizeof (info), 1, fptr))
	{
		if (find (frec.indic.call))
		{
			if ((!ind && !match) || ((match) && (strmatch (frec.indic.call, w_masque))))
			{
				if ((masque == 0xffff) || ((int) frec.flags & masque))
				{
					if (pvoie->temp2 == 0)
						texte (T_MBL + 11);
					affiche_user (&frec, ind);
					pvoie->temp2 = 1;
				}
			}
		}

		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 1;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 1;
			break;
		}

	}
	pvoie->enrcur = ftell (fptr);
	ferme (fptr, 37);
	if ((retour == 0) && (pvoie->temp2 == 0))
		texte (T_ERR + 19);
	return (retour);
}


/* Commande J -> Liste des dernieres connexions */

int mbl_jheard (void)
{
	int port;

	if (isdigit (*indd))
	{
		port = 0;

		while (*indd && (isdigit (*indd)))
		{
			port = (port * 10) + (*indd & 0xF);
			++indd;
		}

		j_list (port, 0);
		retour_mbl ();
		return (0);
	}
	else
	{
		if (ISGRAPH (*(indd + 1)))
			return (1);				// erreur

		*indd = toupper (*indd);
		if (((*indd >= 'A') && (*indd <= 'J')) || (*indd == 'K'))	// expanded from H to J to accomodate a little more ports
		{
			j_list (0, *indd);
			retour_mbl ();
			return (0);
		}
	}

	return (1);					// erreur
}


int page_connect (char port, FILE * fptr)
{
#define TBUF 20
	long pos, nbc = MAXSTAT;
	int nblig, nbrec, lg;
	unsigned char valport;
	statis buffstat[TBUF];
	char valcall[8];

	pvoie->lignes = -1;
	nblig = nbl_page (voiecur);
	if (nblig > MAXLIGNES)
		nblig = MAXLIGNES;
	nblig--;
	if (nblig < 3)
		nblig = 3;

	nbrec = 0;

	while (--nbc)
	{
		if (pvoie->noenr_menu == 0L)
			return (FALSE);

		if (nbrec == 0)
		{
			pos = pvoie->noenr_menu - (long) (sizeof (statis) * TBUF);
			if (pos < 0L)
				pos = 0L;
			fseek (fptr, pos, 0);
			nbrec = fread ((char *) buffstat, sizeof (statis), TBUF, fptr);
			if (nbrec == 0)
				return (FALSE);
		}
		--nbrec;
		pvoie->noenr_menu -= (long) sizeof (statis);
		lg = (strlen (pvoie->ch_temp));
		if (strmatch (buffstat[nbrec].indcnx, pvoie->ch_temp))
		{
			/* test de la validite du port */
			valport = buffstat[nbrec].port + 'A';
			if ((!port) || (valport == port))
			{
				n_cpy (6, valcall, buffstat[nbrec].indcnx);
				/* FlexNet Poll filtering - N1URO and WB2CMF */
				if (strcmp(valcall, mycall)) {
				sprintf (varx[0], "%c", valport);
				sprintf (varx[1], "%02d", buffstat[nbrec].voie);
				sprintf (varx[2], "%4ld", pvoie->noenr_menu / (long) sizeof (statis));
				sprintf (varx[3], "%-6s", valcall);
				sprintf (varx[4], "%2d", buffstat[nbrec].tpscnx / 60);
				sprintf (varx[5], "%02d", buffstat[nbrec].tpscnx % 60);
				ptmes->date = buffstat[nbrec].datcnx;
				texte (T_STA + 23);
				if (--nblig == 0)
					break;
				}
			}
		}
	}
	if (nbc == 0)
		return (0);
	return (1);
}


static int heardcmp (const void *a, const void *b)
{
	long result;

	result = ((Heard *) a)->last - ((Heard *) b)->last;
	return ((result > 0L) ? 1 : ((result == 0L) ? 0 : -1));
}


void j_list (int portnum, char portlet)
{
	FILE *fptr;
	char buffer[259];
	char date[80];
	char indic[10];
	int i, port;
	Heard *pheard;

	if (portnum)
	{
		port = portnum;
		
		if ((port < 1) || (port >= NBPORT) || (p_port[port].pvalid == 0))
		{
			texte (T_ERR + 14);
			return;
		}

		pheard = p_port[port].heard;
		qsort (pheard, NBHEARD, sizeof (Heard), heardcmp);
		for (i = 0; i < NBHEARD; i++)
		{
			if (pheard->last)
			{
				pheard->indic.call[6] = '\0';
				if (pheard->indic.num > 15)
					pheard->indic.num = 0;
				sprintf (indic, "%s-%d",
						 pheard->indic.call, pheard->indic.num);
				strcpy (date, datheure_mbl (pheard->first));
				sprintf (buffer, "%-9s  %s  %s",
						 indic, date, datheure_mbl (pheard->last));
				outln (buffer, strlen (buffer));
			}
			++pheard;
		}

	}
	else
	{
		incindd ();
		tester_masque ();
		if (portlet == 'K')
			port = '\0';
		else
			port = portlet;
		if ((port < 'A') || (port > 'J') || (p_port[port - 'A' + 1].pvalid == 0))
		{
			texte (T_ERR + 14);
			return;
		}
		fptr = ouvre_stats ();
		fseek (fptr, 0L, 2);
		pvoie->noenr_menu = ftell (fptr);
		page_connect (port, fptr);
		ferme (fptr, 38);
	}
}

/*
void j_list (char type)
{
	FILE *fptr;
	char buffer[259];
	char date[80];
	char indic[10];
	int i, port;
	Heard *pheard;

	if (isdigit (type))
	{
		port = type - '0';
		if ((port < 1) || (port >= NBPORT) || (p_port[port].pvalid == 0))
		{
			texte (T_ERR + 14);
			return;
		}

		pheard = p_port[port].heard;
		qsort (pheard, NBHEARD, sizeof (Heard), heardcmp);
		for (i = 0; i < NBHEARD; i++)
		{
			if (pheard->last)
			{
				pheard->indic.call[6] = '\0';
				if (pheard->indic.num > 15)
					pheard->indic.num = 0;
				sprintf (indic, "%s-%d",
						 pheard->indic.call, pheard->indic.num);
				strcpy (date, datheure_mbl (pheard->first));
				sprintf (buffer, "%-9s  %s  %s",
						 indic, date, datheure_mbl (pheard->last));
				outln (buffer, strlen (buffer));
			}
			++pheard;
		}

	}
	else
	{
		incindd ();
		tester_masque ();
		if (type == 'K')
			port = '\0';
		else
			port = type;
		if ((port) && ((port < 'A') || (port > 'H') ||
					   (p_port[port - 'A' + 1].pvalid == 0)))
		{
			texte (T_ERR + 14);
			return;
		}
		fptr = ouvre_stats ();
		fseek (fptr, 0L, 2);
		pvoie->noenr_menu = ftell (fptr);
		page_connect (port, fptr);
		ferme (fptr, 38);
	}
}
*/ 
 
