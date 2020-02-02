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
 *    MODULE FORWARDING OVERLAY 2
 */

#include <serv.h>
#include <config.h>

static int tst_line (char *, int);
static int tst_var (char *);

char *idnt_fwd (void)
{
	static char buffer[25];
	char *ptr = buffer;

	*ptr++ = '[';
	*ptr++ = 'F';
	*ptr++ = 'B';
	*ptr++ = 'B';
	*ptr++ = '-';
	*ptr++ = '0' + PACKAGE_VERSION_MAJOR;
	*ptr++ = '.';
	*ptr++ = '0' + PACKAGE_VERSION_MINOR;
	*ptr++ = '.';
	if (PACKAGE_VERSION_MICRO < 10) {
		*ptr++ = '0' + PACKAGE_VERSION_MICRO;
	} else if (PACKAGE_VERSION_MICRO < 100) {
		*ptr++ = '0' + (PACKAGE_VERSION_MICRO / 10);
		*ptr++ = '0' + (PACKAGE_VERSION_MICRO % 10);
	}
	*ptr++ = '-';
	*ptr++ = 'A';
	if (pvoie->prot_fwd & FWD_FBB)
	{
		if (pvoie->prot_fwd & FWD_BIN)
		{
			*ptr++ = 'B';
			if (pvoie->prot_fwd & FWD_BIN1)
				*ptr++ = '1';
		}
		*ptr++ = 'F';
	}
	*ptr++ = 'H';
	*ptr++ = 'M';
	if (pvoie->mbl_ext)
		*ptr++ = 'R';
	if (pvoie->prot_fwd & FWD_XPRO)
		*ptr++ = 'X';
	*ptr++ = '$';
	*ptr++ = ']';
	*ptr++ = '\r';
	*ptr++ = '\0';
	return (buffer);
}


#ifdef __linux__
int nbcan_linux (void)
{
	int nbcan = 0;
	int i;

	for (i = 1; i < NBPORT; i++)
		if (S_LINUX (i))
			nbcan += p_port[i].nb_voies;
	return (nbcan);
}
#endif


int nbcan_hst (void)
{
	int nbcan = 0;
	int i;

	for (i = 1; i < NBPORT; i++)
		if ((HST (i)) && (p_port[i].ccanal != 0))
			nbcan += p_port[i].nb_voies;
	return (nbcan);
}


int nbcan_drsi (void)
{
	int nbcan = 0;
	int i;

	for (i = 1; i < NBPORT; i++)
		if (DRSI (i))
			nbcan += p_port[i].nb_voies;
	return (nbcan);
}


int nbcan_bpq (void)
{
	int nbcan = 0;
	int i;

	for (i = 1; i < NBPORT; i++)
		if (BPQ (i))
			nbcan += p_port[i].nb_voies;
	return (nbcan);
}


int ch_voie (int port, int canal)
{
	/*
	 * Cherche une voie libre sur un port.
	 * Commence par la derniere voie du port
	 *
	 * Si canal != 0, essaye d'allouer le canal specifie.
	 */

	int i, j;

	if (port == 0)
	{
		if (svoie[INEXPORT]->sta.connect)
			return (-1);
		return (1);
	}

	if (save_fic)
		return (-1);

	if (p_port[port].pvalid == 0)
		return (-1);

	if (DRSI (port))
	{
		if (port_free (port) == 0)
			return (-1);
		if (canal)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == canal) &&
					(DRSI (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)))
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					init_fb_mess (j);
					return (j);
				}
			}
		}
		else
		{
			for (i = nbcan_drsi (); i > 0; i--)
			{
				for (j = 1; j < NBVOIES; j++)
				{
					if ((svoie[j]->affport.canal == i) &&
						(DRSI (no_port (j))) &&
						(!svoie[j]->sta.connect) &&
						(!voie_forward (j)))
					{
						svoie[j]->affport.port = port;
						for (i = 0; i < 8; i++)
							*(svoie[j]->sta.relais[i].call) = '\0';
						init_fb_mess (j);
						return (j);
					}
				}
			}
		}
	}
	else if (BPQ (port))
	{
		if (port_free (port) == 0)
			return (-1);
		if (canal)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == canal) &&
					(BPQ (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)))
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					init_fb_mess (j);
					return (j);
				}
			}
		}
		else
		{
			for (i = nbcan_bpq (); i > 0; i--)
			{
				for (j = 1; j < NBVOIES; j++)
				{
					if ((svoie[j]->affport.canal == i) &&
						(BPQ (no_port (j))) &&
						(!svoie[j]->sta.connect) &&
						(!voie_forward (j)))
					{
						svoie[j]->affport.port = port;
						for (i = 0; i < 8; i++)
							*(svoie[j]->sta.relais[i].call) = '\0';
						init_fb_mess (j);
						return (j);
					}
				}
			}
		}
	}
#ifdef __linux__
	else if (S_LINUX (port))
	{
		if (port_free (port) == 0)
			return (-1);
		if (canal)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == canal) &&
					(S_LINUX (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)))
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					init_fb_mess (j);
					return (j);
				}
			}
		}
		else
		{
			for (i = nbcan_linux (); i > 0; i--)
			{
				for (j = 1; j < NBVOIES; j++)
				{
					if ((svoie[j]->affport.canal == i) &&
						(S_LINUX (no_port (j))) &&
						(!svoie[j]->sta.connect) &&
						(!voie_forward (j)))
					{
						svoie[j]->affport.port = port;
						for (i = 0; i < 8; i++)
							*(svoie[j]->sta.relais[i].call) = '\0';
						init_fb_mess (j);
						return (j);
					}
				}
			}
		}
	}
#endif
	if (HST (port))
	{
		int com = p_port[port].ccom;

		if (port_free (port) == 0)
			return (-1);

		if (p_port[port].ccanal == 0)
		{
			/* Port Pactor. Cherche la voie correspondante */

			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == PACTOR_CH) &&
					(HST (no_port (j))) &&
					(p_port[no_port (j)].ccom == com) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)))
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					init_fb_mess (j);
					return (j);
				}
			}
			return (-1);
		}

		if (canal)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == canal) &&
					(HST (no_port (j))) &&
					(p_port[no_port (j)].ccom == com) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)))
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					init_fb_mess (j);
					return (j);
				}
			}
		}
		else
		{
			for (i = nbcan_hst (); i > 0; i--)
			{
				for (j = 1; j < NBVOIES; j++)
				{
					if ((svoie[j]->affport.canal == i) &&
						(HST (no_port (j))) &&
						(p_port[no_port (j)].ccom == com) &&
						(!svoie[j]->sta.connect) &&
						(!voie_forward (j)))
					{
						svoie[j]->affport.port = port;
						for (i = 0; i < 8; i++)
							*(svoie[j]->sta.relais[i].call) = '\0';
						init_fb_mess (j);
						return (j);
					}
				}
			}
		}
	}
	else
	{
		if (canal)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.port == port) &&
					(svoie[j]->affport.canal == canal) &&
					(!svoie[j]->sta.connect) && (!voie_forward (j)))
				{
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					init_fb_mess (j);
					return (j);
				}
			}
		}
		else
		{
			for (i = p_port[port].nb_voies; i > 0; i--)
			{
				for (j = 1; j < NBVOIES; j++)
				{
					if ((svoie[j]->affport.port == port) &&
						(svoie[j]->affport.canal == i) &&
						(!svoie[j]->sta.connect) && (!voie_forward (j)))
					{
						for (i = 0; i < 8; i++)
							*(svoie[j]->sta.relais[i].call) = '\0';
						init_fb_mess (j);
						return (j);
					}
				}
			}
		}
	}
	return (-1);
}


static char *check_call (char *ptr)
{
	char *scan = ptr;

	while (*scan > ' ')
		++scan;

	if (*scan == '\0')
		scan = ptr;
	else
	{
		while ((*scan) && (isspace (*scan)))
			++scan;

		if (isgraph(*scan) && !isalnum(*scan)) /* Pactor - ! ou % */
			++scan;

		if ((isdigit (*scan)) && (!isalnum (*(scan + 1))))
		{
			++scan;
			while ((*scan) && (!isalnum (*scan)))
				++scan;
		}
	}
	return (scan);
}


int connect_fwd (int voie, Forward * pfwd)
{
	int nb;

	stat_fwd = 0;
	nb = connect_station (voie, 0, pfwd->con_lig[0]);
	svoie[voie]->niv1 = N_FORW;
	svoie[voie]->niv2 = 2;
	svoie[voie]->niv3 = 0;
	
/*	fprintf(stderr, "connect_fwd : voie %d niv1 %d niv2 %d niv3 %d\n", voie, svoie[voie]->niv1, svoie[voie]->niv2, svoie[voie]->niv3);*/
	
	if ((DEBUG) || (!p_port[no_port (voie)].pvalid))
	{
		pfwd->forward = -1;
	}
	return (nb);
}


int connect_station (int voie, int echo, char *ptr)
{
	int port = no_port (voie);
	char buffer[256];
	char *scan = check_call (ptr);
	int nb = 0, sav_voie = voiecur;

	svoie[voie]->debut = time (NULL);
	svoie[voie]->finf.lang = langue[0]->numlang;
	if ((DEBUG) || (!p_port[no_port (voie)].pvalid))
		return (0);

	selvoie (voie);

	svoie[voie]->sta.indicatif.num = extind (scan, svoie[voie]->sta.indicatif.call);
	
/*	fprintf(stderr, "connect_station : %s-%d\n", svoie[voie]->sta.indicatif.call, svoie[voie]->sta.indicatif.num);*/

	strcpy (buffer, ptr);
	deb_io ();
	switch (p_port[port].typort)
	{
	case TYP_DED:				/* DED */
		if (DRSI (port))
		{
			if (strchr (ptr, ':') == NULL)
			{
				while (ISGRAPH (*ptr))
					++ptr;
				while (isspace (*ptr))
					++ptr;
				sprintf (buffer, "C %d:%s", p_port[port].ccanal, ptr);
			}
		}
		tnc_commande (voie, buffer, SNDCMD);
		break;
	case TYP_PK:				/* PK232 */
		buffer[1] = 'O';
		tnc_commande (voie, buffer, SNDCMD);
		break;
	case TYP_HST:				/* PTC-II */
	case TYP_FLX:
		tnc_commande (voie, buffer, SNDCMD);
		break;
#ifndef __linux__
	case TYP_MOD:				/* MODEM */
		md_no_echo (voie);
		svoie[voie]->sta.stat = 1;
		strtok (buffer, " ");
		strtok (NULL, " ");
		scan = strtok (NULL, " ");
		if (scan)
			md_send (no_port (voie), var_txt (scan));
		svoie[voie]->maj_ok = 0;
		break;
#endif
	case TYP_KAM:				/* KAM */
		kam_commande (voie, buffer);
		break;
#ifdef __linux__
	case TYP_SCK:				/* AX25 */
		tnc_commande (voie, buffer, SNDCMD);
		break;
#else
	case TYP_BPQ:				/* BPQ */
		command = 1;
		sta_drv (voie, CMDE, (void *) &command);
		break;
#endif
#ifdef __linux__
	case TYP_TCP:				/* TELNET */
	case TYP_ETH:				/* ETHER-LINK */
		tnc_commande (voie, buffer, SNDCMD);
		break;
#endif
#ifdef __WINDOWS__
	case TYP_ETH:				/* ETHER-LINK */
		{
			tnc_commande (voie, buffer, SNDCMD);
		}
		break;
	case TYP_TCP:				/* TELNET */
		{
			tnc_commande (voie, buffer, SNDCMD);
		}
		break;
	case TYP_AGW:				/* AX25 */
		tnc_commande (voie, buffer, SNDCMD);
		break;
#endif
	}
	fin_io ();

	/* Au cas ou pas de SID ... */
	svoie[voie]->fbb = 0;
	svoie[voie]->mbl_ext = 0;

	svoie[voie]->sta.connect = 1;
	svoie[voie]->maj_ok = 0;
	selvoie (sav_voie);
	if ((echo) && (nb))
		outln (buffer, nb);
	status (voie);
	return (nb);
}


static int tst_line (char *ptr, int val)
{
	int nb;

	while ((*ptr) && (!isdigit (*ptr)))
		ptr++;
	while (*ptr)
	{
		nb = 0;
		while ((*ptr) && (!ISGRAPH (*ptr)))
			ptr++;
		if (*ptr == '*')
			return (FALSE);
		while (isdigit (*ptr))
		{
			nb *= 10;
			nb += (*ptr++ - '0');
		}
		while ((*ptr) && (!ISGRAPH (*ptr)))
			ptr++;
		if (isalpha (*ptr))
		{
			if (val == nb)
				return (TRUE);
			else
				return (FALSE);
		}
		switch (*ptr++)
		{
		case ',':
			if (val == nb)
				return (TRUE);
			break;
		case '-':
			if (val >= nb)
			{
				nb = 0;
				while ((*ptr) && (!ISGRAPH (*ptr)))
					ptr++;
				while (isdigit (*ptr))
				{
					nb *= 10;
					nb += (*ptr++ - '0');
				}
				if (val <= nb)
					return (TRUE);
			}
			break;
		case '\n':
		case 0:
			if (val == nb)
				return (TRUE);
			return (FALSE);
		default:
#ifdef ENGLISH
			cprintf ("Error in time list  \r\n");
#else
			cprintf ("Erreur liste horaire\r\n");
#endif
			return (FALSE);
		}
	}
	return (FALSE);
}


void swap_port (char *port_name)
{
	int port;
	int trouve = 0;

	strupr (port_name);
	for (port = 1; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			if (strcmp (port_name, p_port[port].freq) == 0)
			{
				port_name[0] = port + '@';
				trouve = 1;
				break;
			}
		}
	}

	if ((!trouve) && (port_name[1] != '\0'))
	{
		if (!operationnel)
		{
			/* Erreur, le port n'existe pas ... */
			char buf[80];

			sprintf (buf,
					 "Error : port \"%s\" in forward file does not exist",
					 port_name);
			win_message (5, buf);
		}
		port_name[0] = 'A';
	}
	port_name[1] = '\0';
}


int tst_fwd (char *ptr, int nobbs, long h_time, int port, int *nb_choix, int reverse, int cur_port)
{
	char temp[40];
	int i, choix;
	int val;

	while (isspace (*ptr))
		++ptr;

	switch (val = toupper (*ptr))
	{
	case 'C':					/* Choix links */
		if (nobbs)
		{
			while ((*ptr) && (!isdigit (*ptr)))
				ptr++;
			choix = (int) *ptr - '0';
			if ((nb_choix) && (choix > *nb_choix))
			{
				*nb_choix = choix;
			}
			if (choix == get_link (nobbs))
			{
				return (val);
			}
		}
		return (FALSE);

	case 'D':					/* Choix jour */
		++ptr;
		return ((tst_line (ptr, jour (h_time))) ? val : FALSE);

	case 'N':					/* Choix No du jour */
		++ptr;
		return ((tst_line (ptr, nojour (h_time))) ? val : FALSE);

	case 'F':					/* Choix port libre */
		while (ISGRAPH (*ptr))
			++ptr;

		while (isspace (*ptr))
			++ptr;

		if (*ptr)
		{
			strn_cpy (39, temp, ptr);
			swap_port (temp);
			if (find (strupr (temp)))
			{
				for (i = 0; i < NBVOIES; i++)
				{
					if ((svoie[i]->sta.connect) && (strcmp (svoie[i]->sta.indicatif.call, temp) == 0))
					{
						return (FALSE);
					}
				}
			}
			else
			{
				port = *temp - '@';
				for (i = 0; i < NBVOIES; i++)
				{
					if ((svoie[i]->sta.connect) && (no_port (i) == port))
					{
						return (FALSE);
					}
				}
			}
		}
		else
		{
			for (i = 0; i < NBVOIES; i++)
			{
				if ((svoie[i]->sta.connect) && (no_port (i) == port))
				{
					return (FALSE);
				}
			}
		}
		return (val);

	case 'G':					/* Choix heure GMT */
		++ptr;
		return ((tst_line (ptr, gmt_heure (h_time))) ? val : FALSE);

	case 'H':					/* Choix heure locale */
		++ptr;
		return ((tst_line (ptr, heure (h_time))) ? val : FALSE);

	case 'M':					/* Forward manuel */
		++ptr;
		return ((reverse) ? val : FALSE);

	case 'P':
		while (ISGRAPH (*ptr))
			++ptr;

		while (isspace (*ptr))
			++ptr;

		i = -2;
		if (*ptr)
		{
			strn_cpy (39, temp, ptr);
			swap_port (temp);
			i = *temp - '@';
		}
		return ((i == cur_port) ? val : FALSE);

	case 'V':					/* Test variable environnement */
		++ptr;
		return ((tst_var (ptr)) ? val : FALSE);

	default:
		return ((tst_line (ptr, heure (h_time))) ? val : FALSE);

	}

}

static int tst_var (char *chaine)
{
	char var[80];
	char val[80];
	char *env;

	sscanf (chaine, "%s %s", var, val);
	env = getenv (var);
	if (env == NULL)
		return (0);
	return (strcmp (env, val) == 0);
}

void analyse_idnt (char *chaine)
{
	char *ptr;
	int fin = 0;

	unsigned lprot_fwd = FWD_MBL;

	sup_ln (chaine);
	pvoie->sid = 1;
	pvoie->fbb = 1;
	pvoie->mbl_ext = 0;
	pvoie->mode |= F_FOR;

	if (pvoie->timout == time_n)
		pvoie->timout = time_b;
	ptr = strrchr (chaine, '-');
	if (ptr)
		++ptr;
	else
		ptr = chaine + 1;

	while (!fin)
	{
		switch (*ptr++)
		{
		case '\0':
		case ']':
		case '-':
			fin = 1;
			break;
		case 'H':
			pvoie->mode |= F_HIE;
			break;
		case 'A':
			pvoie->mode |= F_ACQ;
			break;
		case 'B':
			lprot_fwd |= FWD_BIN;
			if (isdigit (*ptr))
			{
				if ((*ptr >= '1') && (bin_fwd == 2))
				{
					pvoie->fbb = 2;
					lprot_fwd |= FWD_BIN1;
				}
				++ptr;
			}
			break;
		case 'C':
			if (pvoie->clock)
				pvoie->clock = 2;
			break;
		case 'F':
			lprot_fwd |= FWD_FBB;
			break;
		case 'M':
			pvoie->mode |= F_MID;
			break;
		case 'R':
			pvoie->mbl_ext = 1;
			break;
		case 'X':
			lprot_fwd |= FWD_XPRO;
			break;
		case '$':
			pvoie->mode |= F_BID;
			break;
		default:
			break;
		}
	}

	if (lprot_fwd & FWD_FBB)
	{
		pvoie->mode |= F_FBB;	/* Protocole FBB valide */

		if (lprot_fwd & FWD_BIN)
		{
			pvoie->mode |= F_BIN;	/* Transfert binaire valide */

		}
	}

	lprot_fwd &= pvoie->prot_fwd;

	if ((lprot_fwd & FWD_XPRO) && (std_header & 512))
	{
		/* XFwd prioritaire */
		pvoie->mode &= (~(F_FBB | F_BIN));
		lprot_fwd &= (~FWD_FBB);
	}

	if ((lprot_fwd & FWD_BIN) == 0)
	{
		lprot_fwd &= (~FWD_BIN1);
	}

	if ((lprot_fwd & FWD_FBB) == 0)
	{
		lprot_fwd &= (~(FWD_BIN | FWD_BIN1));
	}

	pvoie->prot_fwd = lprot_fwd;

	aff_forward ();
}


int att_prompt (void)
{
	char *ptr = indd;
	int modex = FALSE;
	int error = 0;

	if (*indd == '!')
	{
		incindd ();
		new_om = -1;
		accept_cnx ();
		return (FALSE);
	}

	if ((pvoie->nb_prompt == 0) && (*indd == '['))
		modex = TRUE;
	if (*indd == '*')
		error++;
	while (nb_trait--)
	{
		if ((error) && (*ptr == '*'))
		{
			if (++error == 4)
				pvoie->deconnect = 4;
			break;
		}
		else
			error = 0;
		if (*ptr == '\r')
		{
			if ((pvoie->nb_prompt == 0) && (*(ptr - 1) == ']') && (modex))
			{
				*ptr = '\0';

				analyse_idnt (indd);

				/* Choisit le protocole en fonction des options */
				/* si les deux protocoles FBB et XFWD sont OK */
				if ((pvoie->prot_fwd & FWD_FBB) && (pvoie->prot_fwd & FWD_XPRO))
				{
					if (std_header & 512)
					{
						/* XFwd prioritaire */
						pvoie->prot_fwd &= (~(FWD_FBB | FWD_BIN | FWD_BIN1));
					}
					else
					{
						/* FBB prioritaire */
						pvoie->prot_fwd &= (~FWD_XPRO);
					}
				}
			}
			else if (*(ptr - 1) == '>')
			{
				if (pvoie->nb_prompt > 0)
					--pvoie->nb_prompt;
				else
				{
					if (pvoie->sid == 0)
					{
						/* Pas de SID recu -> ni fbb, ni xfwd */
						pvoie->prot_fwd = 0;
					}
					return TRUE;
				}
			}
			else
				modex = FALSE;
		}
		++ptr;
	}
	return FALSE;
}


static int is_duplicate_forward (int nobbs)
{
	Forward *pfwd;
	int port;
	int nb_bbs = 0;

	for (port = 0; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			pfwd = p_port[port].listfwd;
			while (pfwd)
			{
				if (pfwd->no_bbs == nobbs)
					++nb_bbs;
				pfwd = pfwd->suite;
			}
		}
	}

	return (nb_bbs > 1);
}

int mess_suiv (int voie)
/*
 * Y a-t-il encore un message ?
 * Si oui, typ_mess, n_mess, enrdeb et enrcur sont mis a jour. retour TRUE .
 * Si non, retour FALSE .
 */
{
	long no;

	df ("mess_suiv", 1);
	if (save_fic)
	{
		++pvoie->sta.ack;
		svoie[voie]->deconnect = 6;
		ff ();
		return (0);
	}

	if (svoie[voie]->bbsfwd == 0)
	{
		ff ();
		return (0);
	}

	/* teste si un forward est deja en cours -> pas de proposition */
	if (is_duplicate_forward (svoie[voie]->bbsfwd))
	{
		ff ();
		return (0);
	}

	while ((no = msg_fwd_suiv ((int) svoie[voie]->bbsfwd,
					 svoie[voie]->maxfwd, svoie[voie]->oldfwd, svoie[voie]->typfwd, voie)) != 0L)
	{
		if (ch_record (ptmes, no, '\0'))
		{
			if ((ptmes->status == 'N') ||
				(ptmes->status == 'Y') ||
				(ptmes->status == '$'))
			{
				ff ();
				return (TRUE);
			}
		}
		/* Le message n'existe plus : supression de la liste */
		clear_fwd (no);
	}
	ff ();
	return (FALSE);
}


void program_tnc (int voie, char *ptr)
{
	int nb;
	char buffer[300];

	switch (p_port[no_port (voie)].typort)
	{
	case TYP_DED:				/* DED */
	case TYP_HST:				/* PTC */
	case TYP_FLX:
		if (*ptr == 'B')
		{
			++ptr;
			while ((*ptr) && (!ISGRAPH (*ptr)))
				++ptr;
			nb = atoi (ptr);
			if ((nb >= 30) && (nb <= 250))
				svoie[voie]->paclen = nb;
			else
				cprintf ("INVALID VALUE: %s\r\n", ptr);
		}
		else
		{
			if ((!DEBUG) && (p_port[no_port (voie)].pvalid))
			{
				strcpy (buffer, ptr);
				tnc_commande (voie, buffer, ECHOCMD);
				if (*buffer)
				{
					cprintf ("%s\r\n", buffer);
				}
			}
		}
		break;
	case TYP_PK:				/* PK232 */
		if ((*ptr == 'B') && (!isgraph (*(ptr + 1))))
		{
			++ptr;
			while ((*ptr) && (!ISGRAPH (*ptr)))
				++ptr;
			nb = atoi (ptr);
			if ((nb >= 10) && (nb <= 250))
				svoie[voie]->paclen = nb;
			else
				cprintf ("INVALID VALUE: %s\r\n", ptr);
		}
		else
		{
			if ((!DEBUG) && (p_port[no_port (voie)].pvalid))
				tnc_commande (voie, ptr, ECHOCMD);
		}
		break;
	case TYP_KAM:				/* KAM */
		if ((*ptr == 'B') && (!isgraph (*(ptr + 1))))
		{
			++ptr;
			while ((*ptr) && (!ISGRAPH (*ptr)))
				++ptr;
			nb = atoi (ptr);
			if ((nb >= 30) && (nb <= 250))
				svoie[voie]->paclen = nb;
			else
				cprintf ("INVALID VALUE: %s\r\n", ptr);
		}
		else
		{
			if ((!DEBUG) && (p_port[no_port (voie)].pvalid))
				kam_commande (voie, ptr);
		}
		break;
	case TYP_BPQ:				/* BPQ */
		if ((*ptr == 'B') && (!isgraph (*(ptr + 1))))
		{
			++ptr;
			while ((*ptr) && (!ISGRAPH (*ptr)))
				++ptr;
			nb = atoi (ptr);
			if ((nb == 0) || ((nb >= 30) && (nb <= 250)))
				svoie[voie]->paclen = nb;
			else
				cprintf ("INVALID VALUE: %s\r\n", ptr);
		}
		break;
#ifdef __WINDOWS__
	case TYP_AGW:				/* AGW */
		if (*ptr == 'B')
		{
			++ptr;
			while ((*ptr) && (!ISGRAPH (*ptr)))
				++ptr;
			nb = atoi (ptr);
			if ((nb >= 30) && (nb <= 250))
				svoie[voie]->paclen = nb;
			else
				cprintf ("INVALID VALUE: %s\r\n", ptr);
		}
		else
		{
			if ((!DEBUG) && (p_port[no_port (voie)].pvalid))
			{
				strcpy (buffer, ptr);
				tnc_commande (voie, buffer, ECHOCMD);
				if (*buffer)
				{
					cprintf ("%s\r\n", buffer);
				}
			}
		}
		break;
#endif
	}
}

void program_fwd (int affiche, int fwd, typ_pfwd ** ptnc, int voie)
{
	char *ptr;
	typ_pfwd *pcurr, *ptemp;

#ifdef __linux__
	typ_pfwd *prev;
	int done;
	int nbdos;
	char *list[25];				/* 25 taches DOS max */
#endif
#ifdef __WINDOWS__
	typ_pfwd *prev;
	int done;
	int nbdos;
	char *list[25];				/* 25 taches DOS max */
#endif
#ifdef __FBBDOS__
	char s[80];
	fen *fen_ptr;
#endif

	pcurr = *ptnc;

	if (pcurr == NULL)
		return;

#if defined(__WINDOWS__)
	prev = NULL;
	while (pcurr)
	{
		ptr = pcurr->chaine;
		if (voie)
		{
			done = 0;
			if (pcurr->type)
			{					/* Commande DOS */
				if (strncmpi (ptr, "PTCTRX", 6) == 0)
				{
					ptctrx (no_port (voie), ptr);
					done = 1;
				}

				/* DLL Actions */
				else if (call_dll (ptr, NO_REPORT_MODE, NULL, 0, NULL) != -1)
				{
					done = 1;
				}
			}
			else
			{					/* Programm TNC */
				if (fwd)
					program_tnc (voie, ptr);
				done = 1;
			}
		}
		else
			done = 1;

		if (done)
		{
			/* Enleve la cammande de la liste */
			ptemp = pcurr;
			if (prev)
			{
				prev->suiv = pcurr->suiv;
				pcurr = pcurr->suiv;
			}
			else
			{
				/* Tete de liste ... */
				*ptnc = pcurr = pcurr->suiv;
			}
			m_libere (ptemp, sizeof (typ_pfwd));
		}
		else
		{
			prev = pcurr;
			pcurr = pcurr->suiv;
		}
	}

	/* Runs the DOS actions */
	pcurr = *ptnc;

	if (pcurr == NULL)
		return;

	nbdos = 0;
	while (pcurr)
	{
		ptr = pcurr->chaine;
		if (voie)
		{
			if (pcurr->type)
			{
				if (nbdos < 25)
					list[nbdos++] = pcurr->chaine;
			}
		}
		pcurr = pcurr->suiv;
	}

	if (nbdos)
		call_nbdos (list, nbdos, NO_REPORT_MODE, NULL, TOOLDIR, NULL);

	/* Frees the list */
	while (pcurr)
	{
		ptemp = pcurr;
		pcurr = pcurr->suiv;
		m_libere (ptemp, sizeof (typ_pfwd));
	}

#endif
#if defined(__linux__)
	prev = NULL;
	while (pcurr)
	{
		ptr = pcurr->chaine;
		if (voie)
		{
			done = 0;
			if (pcurr->type)
			{					/* Commande DOS */
				if (strncmpi (ptr, "PTCTRX", 6) == 0)
				{
					ptctrx (no_port (voie), ptr);
					done = 1;
				}
			}
			else
			{					/* Programm TNC */
				if (fwd)
					program_tnc (voie, ptr);
				done = 1;
			}
		}
		else
			done = 1;

		if (done)
		{
			/* Enleve la cammande de la liste */
			ptemp = pcurr;
			if (prev)
			{
				prev->suiv = pcurr->suiv;
				pcurr = pcurr->suiv;
			}
			else
			{
				/* Tete de liste ... */
				*ptnc = pcurr = pcurr->suiv;
			}
			m_libere (ptemp, sizeof (typ_pfwd));
		}
		else
		{
			prev = pcurr;
			pcurr = pcurr->suiv;
		}
	}

	/* Runs the DOS actions */
	pcurr = *ptnc;

	if (pcurr == NULL)
		return;

	nbdos = 0;
	while (pcurr)
	{
		ptr = pcurr->chaine;
		if (voie)
		{
			if (pcurr->type)
			{
				if (nbdos < 25)
					list[nbdos++] = pcurr->chaine;
			}
		}
		pcurr = pcurr->suiv;
	}

	if (nbdos)
		call_nbdos (list, nbdos, NO_REPORT_MODE, NULL, TOOLDIR, NULL);

	/* Frees the list */
	while (pcurr)
	{
		ptemp = pcurr;
		pcurr = pcurr->suiv;
		m_libere (ptemp, sizeof (typ_pfwd));
	}

#endif
#ifdef __FBBDOS__
	if (affiche)
	{
		sprintf (s, "TNC Prog. Ch %d", voie);
		fen_ptr = open_win (50, 8, 73, 21, INIT, s);
	}
	while (pcurr)
	{
		ptr = pcurr->chaine;
		if (voie)
		{
			if (pcurr->type)
			{					/* Commande DOS */
				if (strncmpi (ptr, "PTCTRX", 6) == 0)
				{
					ptctrx (no_port (voie), ptr);
				}
				else
				{
					send_dos (pcurr->type, ptr, NULL);
				}
			}
			else
			{					/* Programm TNC */
				if (fwd)
				{
					if (affiche)
						cprintf ("%s\r\n", ptr);
					program_tnc (voie, ptr);
				}
			}
		}
		ptemp = pcurr;
		pcurr = pcurr->suiv;
		m_libere (ptemp, sizeof (typ_pfwd));
	}
#endif
	*ptnc = NULL;
#ifdef __FBBDOS__
	if (affiche)
	{
		sleep_ (1);
		close_win (fen_ptr);
	}
#endif
}
