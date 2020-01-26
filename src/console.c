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
 *   Gestion de la console
 */

#include <serv.h>

static int dde_cnx = 0;
static int dde_trait = 0;

int connect_console (void)
{
	Svoie *vptr = svoie[CONSOLE];

	if (vptr->sta.connect)
	{
#ifdef ENGLISH
		cprintf ("Can't connect ! Console channel is busy.   \r\n");
#else
		cprintf ("Connexion Impossible. Voie Console occup‚e.\r\n");
#endif
		return (0);
	}
	init_etat ();
	selvoie (CONSOLE);			/* F2 = Connexion  */

	if (arret)
		vptr->dde_marche = TRUE;
	else
		vptr->dde_marche = FALSE;
	vptr->timout = time_n;
	init_timout (CONSOLE);
	/*  lastaff = -1 ; */
	vptr->aut_linked = 1;
	vptr->private_dir = 0;
	vptr->read_only = 0;
	vptr->vdisk = 2;
	vptr->log = 1;
	vptr->deconnect = FALSE;
	vptr->tstat = vptr->debut = time (NULL);
	maj_niv (0, 0, 0);
	vptr->tmach = 0L;
	vptr->nb_err = vptr->seq = vptr->stop = vptr->sr_mem = 0;
	vptr->maj_ok = 0;
	vptr->rev_param = 0;
	vptr->ch_mon = vptr->cross_connect = -1;
	vptr->msg_held = 0;
	vptr->binary = vptr->conf = 0;
	vptr->mess_recu = vptr->xferok = 1;
	vptr->mbl = 1;
	vptr->entmes.numero = 0L;
	vptr->entmes.theme = 0;
	strn_cpy (6, vptr->sta.indicatif.call, cons_call.call);
	vptr->sta.indicatif.num = cons_call.num;
	vptr->r_tete = NULL;
	vptr->mbl_ext = 1;
	vptr->prot_fwd = FWD_MBL;
	vptr->nb_egal = 0;
	*vptr->passwd = '\0';
	console_on ();
	aff_event (CONSOLE, 1);
	vptr->sta.stat = vptr->sta.connect = 16;
	curseur ();
	clear_inbuf (CONSOLE);
	connexion (voiecur);
	new_om = nouveau (voiecur);
	/*  pvoie->finf.nbl = 16 ; */
	/* vptr->mode = vptr->finf.flags ; */
	vptr->mode = 0;
	vptr->l_mess = 0L;
	vptr->l_yapp = 0L;
	if (fbb_fwd)
	{
		vptr->mode |= F_NFW;
		if (bin_fwd)
			vptr->mode |= F_BIN;
	}
	/*  mbl[CONSOLE] = TRUE ; */
	/*  cprintf("Connecte\r\n") ;      */
	change_droits (voiecur);
	strcpy (vptr->dos_path, "\\");
	aff_nbsta ();
	fbb_log (CONSOLE, 'C', "@ CONSOL");
	dde_cnx = 1;
	return (TRUE);
}

void console_inbuf (char *data, int len)
{
	in_buf (CONSOLE, data, len);
	dde_trait = 1;
}

void justifie (uchar *buffer)
{
	uchar ligne[83];
	uchar *ptr, *ptri, *ptro;
	int nb_sp = 0;
	int nb_mot = 1;
	int nb_car = 0;
	int k, sp_int, sp_rst, i_sp, j_sp, k_sp, ds_mot, nb_int;

	df ("justifie", 2);

	ptri = buffer;
	ptro = ligne;
	while ((*ptri) && (*ptri == ' '))
	{
		++nb_car;
		*ptro++ = *ptri++;
	}

	if (*ptri)
	{

		ptr = ptri;
		ds_mot = TRUE;

		while (*ptr)
		{
			++nb_car;
			if (*ptr == ' ')
			{
				++nb_sp;
				if (ds_mot)
				{
					ds_mot = FALSE;
					nb_mot++;
				}
			}
			else
				ds_mot = TRUE;
			++ptr;
		}

		if (nb_mot > 1)
		{
			nb_int = nb_mot - 1;
			/* nb_sp += (80 - nb_car) ; */
			nb_sp += (78 - nb_car);
			sp_int = nb_sp / nb_int;
			sp_rst = nb_sp % nb_int;
			if (sp_rst > (nb_int / 2))
			{
				k_sp = FALSE;
				sp_rst = nb_int - sp_rst;
				i_sp = (sp_rst % 2) ? nb_int / (sp_rst + 1) : nb_int / sp_rst;
			}
			else
			{
				k_sp = TRUE;
				if (sp_rst)
					i_sp = (sp_rst % 2) ? nb_int / (sp_rst + 1) : nb_int / sp_rst;
				else
					i_sp = 0;
			}
			j_sp = 0;

			while (*ptri)
			{
				if (*ptri == '\r')
					break;
				if ((just) && (*ptri == ' '))
				{
					for (k = 0; k < sp_int; k++)
						*ptro++ = ' ';
					while (*++ptri == ' ')
						;
					if (k_sp)
					{
						if (sp_rst)
						{
							if (++j_sp == i_sp)
							{
								*ptro++ = ' ';
								j_sp = 0;
								sp_rst--;
							}
						}
					}
					else
					{
						if (sp_rst)
						{
							if (++j_sp != i_sp)
							{
								*ptro++ = ' ';
							}
							else
							{
								j_sp = 0;
								sp_rst--;
							}
						}
						else
						{
							*ptro++ = ' ';
						}
					}
				}
				else
					*ptro++ = *ptri++;
			}
			*ptro = '\0';
			if (!just)
			{
				nb_car = strlen (ligne);
				while (nb_car++ < 78)
				{
				}
			}
			/* *ptro++ = '\r'; */
			*ptro = '\0';
			ptr = ligne;
			while ((*buffer++ = *ptr++) != '\0');
		}
	}
	ff ();
}

int kb_vide (void)
{
	return 1;
}

void curseur (void)
{
}

void connect_fen (void)
{
}

void winputs (int voie, int attr, char *ptr)
{
	int val;
	int header;

	if (attr == -1)
	{
		attr = W_CHNI;
		header = 1;
	}
	else
		header = 0;

	val = (attr & 8) ? 255 : 127;
	window_write (voie, ptr, strlen (ptr), attr, header);
}


void put_nr (uchar *texte, int attr, int nbcar)
{
	uchar chaine[400];
	uchar *ptr = chaine;
	uchar *scan = texte;
	uchar ch;
	int i, nb, pcode, txt = 0;

	if (nbcar > 256)
		nbcar = 256;

	texte[nbcar] = '\0';
	if ((*scan != 0xff) && (*scan != 0xfe))
	{
		strcpy (ptr, "(NRom: ");
		ptr += 7;
		for (i = 0; i < 6; i++)
			*ptr++ = (*scan++ >> 1);
		while (*(ptr - 1) == ' ')
			--ptr;
		*ptr++ = '-';
		ch = (*scan++ >> 1) | 0x30;
		if (ch > '9')
		{
			*ptr++ = '1';
			ch -= 10;
		}
		*ptr++ = ch;

		*ptr++ = ' ';

		for (i = 0; i < 6; i++)
			*ptr++ = (*scan++ >> 1);
		while (*(ptr - 1) == ' ')
			--ptr;
		*ptr++ = '-';
		ch = (*scan++ >> 1) | 0x30;
		if (ch > '9')
		{
			*ptr++ = '1';
			ch -= 10;
		}
		*ptr++ = ch;

		*ptr++ = ' ';

		itoa ((int) (*scan++), ptr, 10);
		while (*ptr)
		{
			++ptr;
		}

		*ptr++ = ' ';

		itoa ((int) (*scan++), ptr, 10);
		while (*ptr)
		{
			++ptr;
		}

		*ptr++ = ' ';

		itoa ((int) (*scan++), ptr, 10);
		while (*ptr)
		{
			++ptr;
		}

		*ptr++ = ' ';

		scan += 2;
		pcode = (int) (*scan++) & 0xf;

		switch (pcode)
		{
		case 0:
			*ptr++ = 'I';
			*ptr++ = 'P';
			txt = 20;
			break;

		case 1:
			*ptr++ = 'C';
			*ptr++ = ' ';
			++scan;
			for (i = 0; i < 6; i++)
				*ptr++ = (*scan++ >> 1);
			while (*(ptr - 1) == ' ')
				--ptr;
			*ptr++ = '-';
			ch = (*scan++ >> 1) | 0x30;
			if (ch > '9')
			{
				*ptr++ = '1';
				ch -= 10;
			}
			*ptr++ = ch;
			*ptr++ = ' ';
			for (i = 0; i < 6; i++)
				*ptr++ = (*scan++ >> 1);
			while (*(ptr - 1) == ' ')
				--ptr;
			*ptr++ = '-';
			ch = (*scan++ >> 1) | 0x30;
			if (ch > '9')
			{
				*ptr++ = '1';
				ch -= 10;
			}
			*ptr++ = ch;
			txt = 0;
			break;

		case 2:
			*ptr++ = 'C';
			*ptr++ = ' ';
			*ptr++ = 'A';
			*ptr++ = 'K';
			txt = 0;
			break;

		case 3:
			*ptr++ = 'D';
			txt = 0;
			break;

		case 4:
			*ptr++ = 'D';
			*ptr++ = ' ';
			*ptr++ = 'A';
			*ptr++ = 'K';
			txt = 0;
			break;

		case 5:
			*ptr++ = 'I';
			txt = 20;
			break;

		case 6:
			*ptr++ = 'I';
			*ptr++ = ' ';
			*ptr++ = 'A';
			*ptr++ = 'K';
			txt = 0;
			break;

		default:
			return;
		}
		*ptr++ = ')';
		*ptr = '\0';
		put_ui (chaine, attr, strlen (chaine));

		if (txt > 0)
		{
			scan = texte + txt;
			put_ui (scan, attr, nbcar - txt);
		}
	}
	else
	{
		scan = texte + 7;
		nbcar -= 7;
		nb = 0;
		ptr = chaine;
		while (nbcar > 0)
		{
			for (i = 0; i < 6; i++)
				*ptr++ = (*scan++ >> 1);
			*ptr++ = '-';
			ch = (*scan++ >> 1) | 0x30;
			if (ch > '9')
			{
				*ptr++ = '1';
				ch -= 10;
			}
			else
				*ptr++ = ' ';
			*ptr++ = ch;
			*ptr++ = ':';
			for (i = 0; i < 6; i++)
				*ptr++ = *scan++;
			*ptr++ = ' ';
			*ptr++ = '-';
			*ptr++ = '>';
			*ptr++ = ' ';
			for (i = 0; i < 6; i++)
				*ptr++ = (*scan++ >> 1);
			*ptr++ = '-';
			ch = (*scan++ >> 1) | 0x30;
			if (ch > '9')
			{
				*ptr++ = '1';
				ch -= 10;
			}
			else
				*ptr++ = ' ';
			*ptr++ = ch;
			*ptr++ = ' ';
			sprintf (ptr, "%3d ", (int) (*scan++));
			/*          itoa((int)(*scan++), ptr, 10) ; */
			while (*ptr)
				++ptr;
			if (++nb == 2)
			{
				*ptr = '\0';
				put_ui (chaine, attr, strlen (chaine));
				ptr = chaine;
				/* *ptr++ = '\r' ; */
				nb = 0;
			}
			else
			{
				*ptr++ = ' ';
				*ptr++ = ' ';
				*ptr++ = ' ';
			}
			nbcar -= 21;
		}
	}
}

int put_ui (uchar *texte, int attr, int nb)
{
	uchar buf[600];
	uchar *ptr = buf;
	uchar c;
	int pos = 0;

	deb_io ();

	/* Commence toujours par un cr */
	c = '\r';
	++nb;

	while (nb--)
	{

		/* Le dernier CR n'est pas pris en compte */
		if ((nb == 0) && (c == '\r'))
			break;

		if (c == '\r')
		{
			*ptr++ = c;
			*ptr = '\0';
			window_write (MMONITOR, buf, strlen (buf), attr, 0);
			ptr = buf;
			pos = 0;
		}
		else
		{
			if (++pos == 80)
			{
				*ptr++ = '\r';
				*ptr = '\0';
				window_write (MMONITOR, buf, strlen (buf), attr, 0);
				ptr = buf;
				pos = 0;
			}
			if (c >= ' ')
			{
				*ptr++ = c;
			}
			else
			{
				*ptr++ = c + '@';
			}
		}
		c = *texte++;
	}
	*ptr = '\0';
	window_write (MMONITOR, buf, strlen (buf), attr, 0);
	return (1);
}


int attend_caractere (int secondes)
{
	return (0);
}


int forwarding_bbs (int nobbs)
{
	int port;
	Forward *pfwd;

	for (port = 0; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			pfwd = p_port[port].listfwd;
			while (pfwd)
			{
				if (pfwd->no_bbs == nobbs)
					return (1);
				pfwd = pfwd->suite;
			}
		}
	}
	return (0);
}

static int is_nb(char *ptr)
{
	int val = isdigit (*ptr);
	
	while (*ptr)
	{
		if (!ISGRAPH (*ptr))
			break;
		if (!isdigit(*ptr))
			val = 0;
		++ptr;
	}
	
	return val;
}

int val_fwd (char *bbs, int *port_fwd, int reverse)
{
	Forward *pfwd;
	int i, port, noport;
	int nobbs;

	strupr (sup_ln (bbs));
/*	if ((isdigit (*bbs)) && (!ISGRAPH (*(bbs + 1)))) */
	if (is_nb(bbs))
	{
/*		*port_fwd = noport = *bbs - '0'; */
		*port_fwd = noport = atoi(bbs);
		if (noport == 0)
		{						/* Lance le forward sur tous les ports */
			*bbs = '\0';
			for (port = 0; port < NBPORT; port++)
			{
				if (p_port[port].pvalid)
				{
					for (i = 0; i < NBMASK; i++)
						p_port[port].fwd[i] = '\0';
					pfwd = p_port[port].listfwd;
					while (pfwd)
					{
						if ((pfwd->forward == 0) && (ch_voie (port, 0) != -1))
						{
							*pfwd->fwdbbs = '\0';
							pfwd->forward = -1;
							pfwd->fwdpos = 0;
							pfwd->fin_fwd = pfwd->cptif = pfwd->fwdlig = 0;
							pfwd->reverse = reverse;
						}
						pfwd = pfwd->suite;
					}
				}
			}
		}
		else if (noport > 0)
		{						/* Selectionne un port */
			*bbs = '\0';
			if (p_port[noport].pvalid)
			{
				for (i = 0; i < NBMASK; i++)
					p_port[noport].fwd[i] = '\0';
				pfwd = p_port[noport].listfwd;
				while (pfwd)
				{
					if ((pfwd->forward == 0) && (ch_voie (noport, 0) != -1))
					{
						*pfwd->fwdbbs = '\0';
						pfwd->forward = -1;
						pfwd->fwdpos = 0;
						pfwd->fin_fwd = pfwd->cptif = pfwd->fwdlig = 0;
						pfwd->reverse = reverse;
					}
					pfwd = pfwd->suite;
				}
			}
		}
	}
	else
	{
		nobbs = num_bbs (bbs);
		if (nobbs)
		{						/* Selectionne une BBS */
			if ((chercher_voie (bbs) == -1) && !forwarding_bbs (nobbs))
			{
				if ((port = what_port (nobbs)) != -1)
				{
					*port_fwd = port;
					pfwd = p_port[port].listfwd;
					while (pfwd)
					{
						if ((pfwd->forward == 0) && (ch_voie (port, 0) != -1))
						{
							strn_cpy (6, pfwd->fwdbbs, bbs);
							clr_bit_fwd (p_port[port].fwd, nobbs);
							pfwd->forward = -1;
							pfwd->fwdpos = 0;
							pfwd->fin_fwd = pfwd->cptif = pfwd->fwdlig = 0;
							pfwd->reverse = reverse;
							break;
						}
						pfwd = pfwd->suite;
					}
					if (pfwd == NULL)
						noport = -1;
					else
						noport = port;
				}
				else
					noport = -2;
			}
			else
				noport = -4;
		}
		else
			noport = -3;
	}
	return (noport);
}



int traite_console ()
{
	df ("traite_console", 0);

	if (dde_cnx)
	{
		dde_cnx = 0;
		ff ();
		return (1);
	}

	if (dde_trait)
	{
		dde_trait = 0;
		ff ();
		return (1);
	}

	ff ();
	return (0);
}

void free_use (void)
{
	FbbMem (0);
}

int aff_etat (int lettre)
{
	int prec;
	char s[20];

	df ("aff_etat", 1);

	prec = let_prec;
	if (lettre != let_prec)
	{
		let_prec = lettre;
		switch (lettre)
		{
#ifdef ENGLISH
		case 'A':
			sprintf (s, "Idle   ");
			break;
		case 'B':
			sprintf (s, "Beacon ");
			break;
		case 'C':
			sprintf (s, "Cron   ");
			break;
		case 'D':
			sprintf (s, "Disconn");
			break;
		case 'E':
			sprintf (s, "Sending");
			break;
		case 'F':
			sprintf (s, "Forward");
			break;
		case 'G':
			sprintf (s, "F_send ");
			break;
		case 'I':
			sprintf (s, "Import ");
			break;
		case 'J':
			sprintf (s, "Export ");
			break;
		case 'K':
			sprintf (s, "WaitAck");
			break;
		case 'L':
			sprintf (s, "Reading");
			break;
		case 'M':
			sprintf (s, "Unproto");
			break;
		case 'N':
			sprintf (s, "Inbuf  ");
			break;
		case 'O':
			sprintf (s, ">%-6ld", ptmes->numero);
			let_prec = 'A';
			break;
		case 'R':
			sprintf (s, "Receive");
			break;
		case 'S':
			sprintf (s, "Saving ");
			break;
		case 'T':
			sprintf (s, "Process");
			break;
		case 'W':
			sprintf (s, "H-Route");
			break;
		case 'X':
			sprintf (s, "WP-Upd ");
			break;
		case 'Y':
			sprintf (s, "Kernel ");
			break;
		case 'Z':
			sprintf (s, "W-Share");
			break;
#else
		case 'A':
			sprintf (s, "Attend ");
			break;
		case 'B':
			sprintf (s, "Balise ");
			break;
		case 'C':
			sprintf (s, "Cron   ");
			break;
		case 'D':
			sprintf (s, "D‚conn.");
			break;
		case 'E':
			sprintf (s, "Envoie ");
			break;
		case 'F':
			sprintf (s, "Forward");
			break;
		case 'G':
			sprintf (s, "F_env  ");
			break;
		case 'I':
			sprintf (s, "Import ");
			break;
		case 'J':
			sprintf (s, "Export ");
			break;
		case 'K':
			sprintf (s, "Att.Ack");
			break;
		case 'L':
			sprintf (s, "LitTnc ");
			break;
		case 'M':
			sprintf (s, "Monitor");
			break;
		case 'N':
			sprintf (s, "Inbuf  ");
			break;
		case 'O':
			sprintf (s, ">%-6ld", ptmes->numero);
			let_prec = 'A';
			break;
		case 'R':
			sprintf (s, "Recoit ");
			break;
		case 'S':
			sprintf (s, "Sauve  ");
			break;
		case 'T':
			sprintf (s, "Traite ");
			break;
		case 'W':
			sprintf (s, "H-Route");
			break;
		case 'X':
			sprintf (s, "MAJ-WP ");
			break;
		case 'Y':
			sprintf (s, "Kernel ");
			break;
		case 'Z':
			sprintf (s, "W-Share");
			break;
#endif
		}
		win_status (s);
	}
	ff ();
	return (prec);
}

void aff_msg_cons ()
{
	unsigned num_indic;
	ind_noeud *noeud;
	int priv = -1;
	int hold = -1;

	noeud = insnoeud (cons_call.call, &num_indic);

	if (noeud->nbnew != msg_cons)
		priv = msg_cons = noeud->nbnew;

	if (nb_hold != hold_cons)
		hold = hold_cons = nb_hold;

	win_msg_cons (priv, hold);
}

enum meminfo_row
{
	meminfo_main = 0,
	meminfo_swap
};

enum meminfo_col
{
	meminfo_total = 0, meminfo_used, meminfo_free,
	meminfo_shared, meminfo_buffers, meminfo_cached
};

/* Code from linuxnode */

#define MEMINFO_FILE "/proc/meminfo"

static char buf[1000];

#define MAX_ROW 3				/* these are a little liberal for flexibility */
#define MAX_COL 7

unsigned **meminfo (void)
{
	static unsigned *row[MAX_ROW + 1];	/* row pointers */
	static unsigned num[MAX_ROW * MAX_COL];		/* number storage */
	char *p;
	int i, j, k, l;

	static int n, fd = -1;

	if (fd == -1 && (fd = open (MEMINFO_FILE, O_RDONLY)) == -1)
	{
		/* node_perror(FILE, errno); */
		close (fd);
		return 0;
	}
	lseek (fd, 0L, SEEK_SET);
	if ((n = read (fd, buf, sizeof buf - 1)) < 0)
	{
		close (fd);
		fd = -1;
		return 0;
	}
	buf[n] = '\0';

	if (!row[0])				/* init ptrs 1st time through */
		for (i = 0; i < MAX_ROW; i++)	/* std column major order: */
			row[i] = num + MAX_COL * i;
	p = buf;
	for (i = 0; i < MAX_ROW; i++)	/* zero unassigned fields */
		for (j = 0; j < MAX_COL; j++)
			row[i][j] = 0;
	if (!strncmp("MemTotal:", p, 8)) {
/*		printf("Asuming 2.6 Kernel\n"); */
		while (*p && !isdigit(*p)) p++;
		l = sscanf(p, "%u%n", &row[meminfo_main][meminfo_total], &k);
		p += k;
		while (*p && !strncmp("MemFree:", p, 8)) p++;
		while (*p && !isdigit(*p)) p++;
		l = sscanf(p, "%u%n", &row[meminfo_main][meminfo_free], &k);
		p += k;
		while (*p && !strncmp("Buffers:", p, 8)) p++;
		while (*p && !isdigit(*p)) p++;
		l = sscanf(p, "%u%n", &row[meminfo_main][meminfo_buffers], &k);
		p += k;
		while (*p && !strncmp("Cached:", p, 7)) p++;
		while (*p && !isdigit(*p)) p++;
		l = sscanf(p, "%u%n", &row[meminfo_main][meminfo_cached], &k);
		p += k;
		while (*p && !strncmp("SwapFree:", p, 9)) p++;
		while (*p && !isdigit(*p)) p++;
		l = sscanf(p, "%u%n", &row[meminfo_swap][meminfo_free], &k);
		p += k;
		return row;
	}
	for (i = 0; i < MAX_ROW && *p; i++)
	{							/* loop over rows */
		while (*p && !isdigit (*p))
			p++;				/* skip chars until a digit */
		for (j = 0; j < MAX_COL && *p; j++)
		{						/* scanf column-by-column */
			l = sscanf (p, "%u%n", row[i] + j, &k);
			p += k;				/* step over used buffer */
			if (*p == '\n' || l < 1)	/* end of line/buffer */
				break;
		}
	}
	return row;					/* NULL return ==> error */
}

void free_mem (void)
{
	unsigned **mem;

	if (!(mem = meminfo ()) || mem[meminfo_main][meminfo_total] == 0)
	{
		/* cannot normalize mem usage */
		tot_mem = 0L;
	}
	else
	{
		tot_mem =
			mem[meminfo_main][meminfo_free] +
			mem[meminfo_main][meminfo_buffers] +
			mem[meminfo_total][meminfo_cached] +
			mem[meminfo_swap][meminfo_free];
	}

	FbbMem (0);
}

void aff_forward (void)
{
	fbb_list (FALSE);
}


int aff_nbsta (void)
{
	return (fbb_list (FALSE));
}


char *stat_voie (int voie)
{
	static char s[15];

	switch (svoie[voie]->sta.stat)
	{
	case 0:
		strcpy (s, "Disconne");
		break;
	case 1:
		strcpy (s, "Link Set");
		break;
	case 2:
		strcpy (s, "Fram Rej");
		break;
	case 3:
		strcpy (s, "Disc Req");
		break;
	case 4:
		strcpy (s, "Transfer");
		break;
	case 5:
		strcpy (s, "FRej Snd");
		break;
	case 6:
		if (p_port[no_port (voie)].typort == TYP_KAM)
			sprintf (s, "Retry %-2d", svoie[voie]->sta.ret + 1);
		else
			sprintf (s, "Retry %-2d", svoie[voie]->sta.ret);
		break;
	case 7:
		strcpy (s, "Dev Busy");
		break;
	case 8:
		strcpy (s, "RdevBusy");
		break;
	case 9:
		strcpy (s, "BdevBusy");
		break;
	case 10:
		strcpy (s, "WAck Dbs");
		break;
	case 11:
		strcpy (s, "WAckRbsy");
		break;
	case 12:
		strcpy (s, "WAckBbsy");
		break;
	case 13:
		strcpy (s, "RFrmDbsy");
		break;
	case 14:
		strcpy (s, "RFrmRbsy");
		break;
	case 15:
		strcpy (s, "RFrmBbsy");
		break;
	case 16:
		strcpy (s, "Console ");
		break;
	case 17:
		strcpy (s, "Ch Busy ");
		break;
	default:
		strcpy (s, "Error st");
		break;
	}
	return (s);
}


void status (int voie)
{
	char buf[255];
	char call[80];
	char ret[80];
	Svoie *ptvoie = svoie[voie];

	if (operationnel)
	{
		if ((v_aff < 0) || (voie == v_aff))
		{
			if (voie > 1)
			{
				sprintf (ret, "(%03d)", ptvoie->sta.mem);
				sprintf (buf, "Buf %03d/%03d %s %02d-%02d-%02d %s",
						 ptvoie->sta.ack, ptvoie->maxbuf,
						 ret, ptvoie->niv1, ptvoie->niv2, ptvoie->niv3, stat_voie (voie));

#ifdef ENGLISH
				sprintf (call, "Ch %02d %s-%d",
						 virt_canal (voie), ptvoie->sta.indicatif.call, ptvoie->sta.indicatif.num);
#else
				sprintf (call, "Vo %02d %s-%d",
						 virt_canal (voie), ptvoie->sta.indicatif.call, ptvoie->sta.indicatif.num);
#endif

				FbbStatus (call, buf);
			}
			free_mem ();
		}
		user_status (voie);
	}
	svoie[voie]->ch_status = 0;
}


int aff_yapp (int voie)
{
	return 1;
}

char *yapp_str (int voie, char *s)
{
#define YAPPLEN 44
#define MAX_AFF 12

	static char stdesc[MAX_AFF][11] =
	{
		"SendInit  ",
		"SendInitRt",
		"SendHeader",
		"SendData  ",
		"SendEof   ",
		"SendEOT   ",
		"RcvWait   ",
		"RcvHeader ",
		"RcvData   ",
		"SndABORT  ",
		"WaitAbtAck",
		"RcdABORT  "
	};
	static char *yapp_name[2] =
	{"Yapp", "YapC"};
	Svoie *pv = svoie[voie];
	int i;
	int retour = 0;
	int niv = pv->niv2;
	long offset = pv->enrcur;

	*s = '\0';

	if (!operationnel)
		return (0);

	if (offset < 0L)
		offset = 0L;

	*s = '\0';
	if (v_aff < 0)
	{
		if ((svoie[voie]->sta.connect) && (svoie[voie]->niv1 == N_YAPP))
		{
			if ((niv > 0) && (niv < MAX_AFF))
			{
				sprintf (s, "%s:%s %s %ld/%ld", yapp_name[pv->type_yapp], stdesc[pv->niv2], pv->appendf,
						 offset, pv->tailm);
				retour = 1;
			}
		}
		if (retour == 0)
		{
			for (i = 0; i < NBVOIES; i++)
			{
				if ((svoie[i]->sta.connect) && (svoie[i]->niv1 == N_YAPP))
				{
					if ((niv > 0) && (niv < MAX_AFF))
					{
						pv = svoie[i];
						sprintf (s, "%s:%s %s %ld/%ld", yapp_name[pv->type_yapp], stdesc[pv->niv2], pv->appendf,
								 offset, pv->tailm);
						break;
					}
				}
			}
		}
	}
	else if (voie == v_aff)
	{
		if ((svoie[voie]->sta.connect) && (svoie[voie]->niv1 == N_YAPP))
		{
			if ((niv > 0) && (niv < MAX_AFF))
			{
				sprintf (s, "%s:%s %s %ld/%ld", yapp_name[pv->type_yapp], stdesc[pv->niv2], pv->appendf,
						 offset, pv->tailm);
				retour = 1;
			}
		}
	}
	
	return (s);
}


void aff_ind_console (void)
{
	char s[80];

	sprintf (s, "Console : %6s-%-2d\n", cons_call.call, cons_call.num);
	aff_chaine (DEF, 25, 1, s);
}

void affich_logo (int att)
{
	char chaine[80];

#ifdef ENGLISH
	sprintf (chaine, "MULTICONNECT BBS   F6FBB V%s", version ());
#else
	sprintf (chaine, "SERVEUR MULTIVOIES F6FBB V%s", version ());
#endif
	aff_chaine (att, 55 - strlen (version ()), 1, chaine);
}

void aff_date (void)
{
	char buffer[300];
	char cdate[19];
	long temps;
	struct tm *sdate;

#ifdef ENGLISH
	char jour[] = "SunMonTueWedThuFriSat";

#else
	char jour[] = "DimLunMarMerJeuVenSam";

#endif

	temps = time (NULL);
	sdate = localtime (&temps);
#ifdef ENGLISH
	sprintf (cdate, "    %02d-%02d-%02d %02d:%02d",
			 sdate->tm_year %100, sdate->tm_mon + 1, sdate->tm_mday,
			 sdate->tm_hour, sdate->tm_min);
#else
	sprintf (cdate, "    %02d/%02d/%02d %02d:%02d",
			 sdate->tm_mday, sdate->tm_mon + 1, sdate->tm_year %100,
			 sdate->tm_hour, sdate->tm_min);
#endif
	memcpy (cdate, jour + (sdate->tm_wday * 3), 3);
	if (com_error)
		sprintf (buffer, "%s  %02d", cdate, com_error);
	else
		sprintf (buffer, "%s", cdate);
	aff_chaine (DEF, 1, 1, buffer);
}

void maj_options (void)
{
	FILE *fptr;

	if ((fptr = fopen (d_disque ("OPTIONS.SYS"), "wb")) != NULL)
	{
		fwrite (&bip, sizeof (short), 1, fptr);
		fwrite (&ok_tell, sizeof (short), 1, fptr);
		fwrite (&ok_aff, sizeof (short), 1, fptr);
		fwrite (&separe, sizeof (short), 1, fptr);
		fwrite (&doub_fen, sizeof (short), 1, fptr);
		fwrite (&gate, sizeof (short), 1, fptr);
		fwrite (&just, sizeof (short), 1, fptr);
		fwrite (&p_forward, sizeof (short), 1, fptr);
		fwrite (&sed, sizeof (short), 1, fptr);
		fwrite (&aff_inexport, sizeof (short), 1, fptr);
		fwrite (&aff_popsmtp, sizeof (short), 1, fptr);

		fclose (fptr);
	}

	maj_menu_options ();
}

int ind_console (int type, char *chaine)	/* Type : 0 = decimal, 1 = hexa */
{
	int c = 0;
	char *ptr = chaine;

	while (isalnum (*ptr))
		++ptr;
	if (*ptr)
	{
		*ptr++ = '\0';
		while ((*ptr) && (!isxdigit (*ptr)))
			++ptr;
	}

	if (find (chaine))
	{
		strcpy (cons_call.call, chaine);
		if (*ptr)
		{
			if (type)
				sscanf (ptr, "%x", &c);
			else
				sscanf (ptr, "%d", &c);
			if ((c < 0) || (c > 15))
				return (0);
		}
		cons_call.num = c;
		return (1);
	}
	return (0);
}

void house_keeping (void)
{
	long caltemps;

	save_fic = 1;
	set_busy ();
	time (&caltemps);
	stop_min = minute (caltemps);
	type_sortie = 3;
}

#if 0
int read_wp (unsigned record, Wp * wp)
{
	return (0);
}

int write_wp (unsigned record, Wp * wp)
{
	return (0);
}

unsigned search_wp_record (lcall icall, int what, unsigned first_record)
{
	return (0xffff);
}

#endif

void console_off (void)
{
	CloseFbbWindow (0);
}

void console_on (void)
{
}

void FbbRequestUserList (void)
{
	bloc_indic *bptr;
	unsigned offset;
	int nb = 0;
	char buffer[8];

	/* Envoie les indicatifs dans la ListBox */

	bptr = racine;
	offset = 0;

	while (bptr)
	{
		if (*(bptr->st_ind[offset].indic) == '\0')
			break;
		if (bptr->st_ind[offset].coord != 0xffff)
		{
			if (bptr->st_ind[offset].val)
			{
				n_cpy (6, buffer, bptr->st_ind[offset].indic);
				AddUserList (buffer);
			}
		}
		if (++offset == T_BLOC_INFO)
		{
			offset = 0;
			bptr = bptr->suiv;
		}
	}

	/* Envoie les langues dans la ListBox */
	for (nb = 0; nb < maxlang; nb++)
	{
		AddUserLang (nomlang + nb * LG_LANG);
	}
}

int GetUserInfos (char *callsign, info * frec)
{
	ind_noeud *noeud;
	FILE *fptr;
	unsigned num_indic;

	noeud = insnoeud (callsign, &num_indic);
	if (noeud->coord == 0xffff)
	{
		return (0);
	}
	fptr = ouvre_nomenc ();
	fseek (fptr, (long) noeud->coord * sizeof (info), 0);
	fread (frec, sizeof (info), 1, fptr);
	ferme (fptr, 39);
	return (1);
}

int SetUserInfos (char *callsign, info * frec)
{
	int voie;
	ind_noeud *noeud;
	FILE *fptr;
	unsigned num_indic;

	/* Met a jour l'utilisateur eventuellement connecte */
	for (voie = 0; voie < NBVOIES; ++voie)
	{
		if (svoie[voie]->sta.connect &&
			indcmp (svoie[voie]->sta.indicatif.call, callsign))
		{
			svoie[voie]->finf = *frec;
		}
	}

	/* Met a jour le record de l'utilisateur */
	noeud = insnoeud (callsign, &num_indic);
	if (noeud->coord == 0xffff)
	{
		return (0);
	}
	fptr = ouvre_nomenc ();
	fseek (fptr, (long) noeud->coord * sizeof (info), 0);
	fwrite (frec, sizeof (info), 1, fptr);
	ferme (fptr, 39);
	return (1);
}

void FbbRequestMessageList (void)
{
	char buf[40];
	unsigned offset;
	bloc_mess *bptr = tete_dir;

	ouvre_dir ();
	/* pvoie->typlist = 0; */


	if (bptr)
	{
		/* Goto the end of the list */
		while (bptr->suiv)
			bptr = bptr->suiv;

		/* Scans the list */
		offset = T_BLOC_MESS;
		while (bptr)
		{
			--offset;
			if (bptr->st_mess[offset].noenr)
				AddMessageList (ltoa (bptr->st_mess[offset].nmess, buf, 10));
			if (offset == 0)
			{
				bptr = prec_dir (bptr);
				offset = T_BLOC_MESS;
			}
		}
	}
	ferme_dir ();
}

int GetMsgInfos (bullist * plig, long numero)
{
	mess_noeud *mptr;

	mptr = findmess (numero);
	if (mptr == NULL)
		return 0;

	ouvre_dir ();
	read_dir (mptr->noenr, plig);
	ferme_dir ();
	return (1);
}

int SetMsgInfo (bullist * plig, long numero)
{
	mess_noeud *mptr;

	mptr = findmess (numero);
	if (mptr == NULL)
		return 0;

	ouvre_dir ();
	write_dir (mptr->noenr, plig);
	ferme_dir ();
	return (1);
}

char *MessPath (void)
{
	return (MESSDIR);
}

void fwd_encours (void)
{
	int i, priv, bull, kb;
	atfwd *nbmess;
	char ifwd[NBBBS][7];
	char maxfwd[NBBBS + 1];
	char typfwd[NBBBS + 1];
	char typdat[NBBBS + 1];

	ch_bbs (1, ifwd);

	fwd_value (maxfwd, typfwd, typdat);

	for (i = 0; i < NBBBS; i++)
	{
		nbmess = attend_fwd (i + 1, maxfwd[i + 1], 0, typfwd[i + 1], typdat[i + 1]);
		if (nbmess > 0)
		{
			priv = nbmess->nbpriv;
			bull = nbmess->nbbul;
			kb = nbmess->nbkb;
		}
		else
			priv = bull = kb = 0;
		AddPendingLine (ifwd[i], priv, bull, kb);
	}
}

void compress_display (int type, long value, long numero)
{
	CompressPosition (type, (int) value, numero);
}


void maintenance (void)
{
	int i;
	int suite;
	int port, voie, flag, strm;
	char txt[80];
	char chaine[300];
	FILE *fpinit;

	df ("maintenance", 0);

	InfoMessage (-1, "Halting communication", "Halt");

	operationnel = FALSE;
	for (port = 1; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			wsprintf (txt, "Halting port %d", port);
			InfoMessage (-1, txt, NULL);

			selcanal (port);
			switch (p_port[port].typort)
			{
			case TYP_DED:
			case TYP_HST:
				vide (port, 0);	/* Supprime le dernier ACK de sonde */
				tnc_commande (port, "Y0", PORTCMD);
				break;
			case TYP_PK:
				tnc_commande (port, "UR1", PORTCMD);
				break;
			case TYP_MOD:
				/* modem_stop(port) ; */
				break;
			case TYP_KAM:
				tnc_commande (port, "USERS 0/0", PORTCMD);
				break;
			case TYP_BPQ:
				for (voie = 1; voie < NBVOIES; voie++)
				{
					if (svoie[voie]->affport.port == port)
					{
						strm = no_canal (voie);
						flag = 6;
						cprintf ("Devalidating stream %d\r\n", strm);
						sta_drv (voie, SETFLG, (void *) &flag);
					}
				}
				break;
			}
			for (voie = 1; voie < NBVOIES; voie++)
			{
				if ((svoie[voie]->affport.port == port) && (svoie[voie]->sta.connect))
				{
					deconnexion (voie, 1);
#ifdef __linux__
#ifdef ENGLISH
					cprintf ("Disconnecting Port %d Channel %d\n", port, virt_canal (voie));
#else
					cprintf ("Deconnection du Port %d Voie %d \n", port, virt_canal (voie));
#endif
#else
#ifdef ENGLISH
					cprintf ("Disconnecting Port %d Channel %d\r\n", port, virt_canal (voie));
#else
					cprintf ("Deconnection du Port %d Voie %d \r\n", port, virt_canal (voie));
#endif
#endif
				}
			}
			if (!BPQ (port))
			{
				sprintf (chaine, "MAINT%d.SYS", port);
				if ((fpinit = fopen (c_disque (chaine), "r")) != NULL)
				{
					while (fgets (chaine, 256, fpinit))
					{
						if (*chaine == '#')
							continue;
						sup_ln (chaine);
						if (*chaine)
							tnc_commande (port, chaine, PORTCMD);
					}
					fclose (fpinit);
				}
				switch (p_port[port].typort)
				{
				case TYP_DED:
				case TYP_HST:
					suite = 0;
					if (DRSI (port))
					{
						for (i = port + 1; i < NBPORT; i++)
						{
							if (DRSI (i))
								suite = 1;
						}
					}
					if (suite == 0)
					{
						cls_drv (port);
					}
					break;
				case TYP_PK:
					tnc_commande (port, "HON", PORTCMD);
					break;
				case TYP_KAM:
					break;
				}
			}
		}
	}
	closecom ();
	InfoMessage (-1, NULL, NULL);
	sprintf (chaine, "Q *** BBS Quit   (%s)", version());
	port_log (0, 0, 'S', chaine);
	/*	port_log (0, 0, 'S', "Q *** BBS Quit");*/
	ferme_log ();
	ff ();
}

void set_busy (void)
{
	int port, voie, flag, strm;

	InfoMessage (-1, "Stopping connections", "Halt");

	for (port = 1; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			selcanal (port);

			switch (p_port[port].typort)
			{
			case TYP_DED:
			case TYP_HST:
				tnc_commande (port, "Y0", PORTCMD);
				break;
			case TYP_PK:
				tnc_commande (port, "UR1", PORTCMD);
				break;
			case TYP_MOD:
				for (voie = 0; voie < NBVOIES; voie++)
				{
					if ((no_port (voie) == port) && (!svoie[voie]->sta.connect))
					{
						tnc_commande (port, "ATH0=0", PORTCMD);
					}
				}
				break;
			case TYP_KAM:
				tnc_commande (port, "USERS 0/0", PORTCMD);
				break;
			case TYP_BPQ:
				for (voie = 1; voie < NBVOIES; voie++)
				{
					if (no_port (voie) == port)
					{
						flag = 0;
						strm = no_canal (voie);
						cprintf ("Devalidating stream %d (%02x)\r\n", strm, flag);
						sta_drv (voie, SETFLG, (void *) &flag);
					}
				}
				break;
			case TYP_SCK:
			case TYP_TCP:
			case TYP_ETH:
				for (voie = 1; voie < NBVOIES; voie++)
				{
					if (no_port (voie) == port)
					{
						sta_drv (voie, SETBUSY, NULL);
					}
				}
				break;
			}
		}
	}
	InfoMessage (-1, "Connections stopped", NULL);
	InfoMessage (-1, NULL, NULL);
}
