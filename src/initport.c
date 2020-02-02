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

static int lang_len (char *);
static void errport (int, char *);
static void err_fic (char *, int, char *);


static void init_forward (Forward * pfwd)
{
	int i;
	int j;

	pfwd->reverse = 0;
	pfwd->fwdpos = 0;
	pfwd->lastpos = 0;
	pfwd->fwdlig = 0;
	pfwd->cptif = 0;
	pfwd->forward = 0;
	pfwd->no_con = 0;
	pfwd->no_bbs = 0;
	pfwd->fin_fwd = 0;
	for (i = 0; i < 8; i++)
	{
		*pfwd->con_lig[i] = '\0';
	}
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 3; j++)
		{
			*pfwd->mesnode[i][j] = '\0';
		}
	}
	*pfwd->txt_con = '\0';
	*pfwd->fwdbbs = '\0';
	pfwd->suite = NULL;
}

void end_ports (void)
{
	int i;
	Forward *pfwd;

	if (p_port == NULL)
		return;

	for (i = 0; i < NBPORT; i++)
	{
		while (p_port[i].listfwd)
		{
			pfwd = p_port[i].listfwd;
			p_port[i].listfwd = pfwd->suite;
			m_libere (pfwd, sizeof (Forward));
		}
	}
	m_libere (p_port, sizeof (defport) * NBPORT);

	p_port = NULL;
}

void initport (void)
{
	FILE *fptr;
	Forward *pprec, *pfwd;
	char *ptr;
	char ligne[256], sfreq[81], mode[81];
	char portname[81];
	int voie;

#ifdef __linux__
	int linux_canal = 1;

#endif
	int drsi_canal = 1;
	int bpq_canal = 1;
	int agw_canal = 1;
	int hst_canal = 1;
	int lig;
	int j;
	unsigned int p1, p2, p3, p4, p5, p6, p7, p8, p9;
	long lp4;
	int mx;
	int nbvalid = 0, i = 0;
	int nb, nbl, tnc = 0, nbport;
	int local_mode, nbtnc, fvoie;	/* flag de lecture d'affectation des voies */
	unsigned al;
	char *portfile;

	lig = 0;

	al = sizeof (defcom) * NBPORT;
	p_com = (defcom *) m_alloue (al);
	memset (p_com, 0, sizeof (defcom) * NBPORT);

#ifdef __FBBDOS__
	portfile = "port_d.sys";
#endif
#ifdef __WINDOWS__
	portfile = "port_w.sys";
#endif
#ifdef __linux__
	portfile = "port_l.sys";
#endif
	if ((fptr = fopen (c_disque (portfile), "r")) == NULL)
	{
		if ((fptr = fopen (c_disque ("port.sys"), "r")) == NULL)
		{
#ifdef ENGLISH
			errport (lig, "File PORT.SYS not found");
#else
			errport (lig, "Fichier PORT.SYS inexistant");
#endif
			return;
		}
	}
#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Ports set-up            \n");
#else
	cprintf ("Initialisation des ports\n");
#endif
#else
#ifdef ENGLISH
	cprintf ("Ports set-up            \r\n");
#else
	cprintf ("Initialisation des ports\r\n");
#endif
#endif

	al = sizeof (defport) * NBPORT;
	p_port = (defport *) m_alloue (al);

	for (i = 0; i < NBPORT; i++)
	{
		p_port[i].mem = 32000;
		p_port[i].pvalid = p_port[i].nb_voies = 0;
		p_port[i].transmit = 0;
		p_port[i].cur_can = 0;
		p_port[i].portind = 0;
		p_port[i].synchro = 0;
		p_port[i].frame = 0;
		for (j = 0; j < NBHEARD; j++)
			p_port[i].heard[j].last = 0L;
	}
	p_port[0].pvalid = 1;		/* Port console valide */
	p_port[0].typort = TYP_DED;
	p_port[0].moport = 0;
	p_port[0].frame = 4;
/******************* A PARAMETRER *******************/
	p_port[0].min_fwd = 0;
	p_port[0].per_fwd = 15;
	strcpy (p_port[0].freq, "CONSOLE");
	p_port[0].listfwd = (Forward *) m_alloue (sizeof (Forward));
	init_forward (p_port[0].listfwd);

	svoie[0]->sta.mem = 500;
	svoie[0]->affport.port = 0xfe;
	svoie[0]->affport.canal = -2;

	/* voie pour import/export */
	init_voie (NBVOIES);
	svoie[NBVOIES]->sta.mem = 500;
	svoie[NBVOIES]->affport.port = 0;
	svoie[NBVOIES]->affport.canal = 1;
	++NBVOIES;

	fvoie = 1;
	nbl = 0;
	while (fgets (ligne, 250, fptr))
	{
		++lig;
		ptr = ligne;
		while (*ptr && (*ptr == ' '))
			++ptr;
		if (*ptr == '#')
			continue;
		if (!isdigit (*ptr))
		{
#ifdef ENGLISH
			errport (lig, "Syntax error     ");
#else
			errport (lig, "Erreur de syntaxe");
#endif
			return;
		}
		switch (fvoie)
		{
		case 1:
			nb = sscanf (ptr, "%d %d", &nbport, &nbtnc);
			if (nb != 2)
			{
#ifdef ENGLISH
				errport (lig, "Wrong number of parameters    ");
#else
				errport (lig, "Nombre de paramŠtres incorrect");
#endif
				return;
			}
			if ((nbport <= 0) || (nbport > (NBPORT - 1)))
			{
#ifdef ENGLISH
				errport (lig, "Port number error     ");
#else
				errport (lig, "Nombre de ports erron‚");
#endif
				return;
			}
			if (nbtnc < nbport)
			{
#ifdef ENGLISH
				errport (lig, "TNC number error     ");
#else
				errport (lig, "Nombre de TNCs erron‚");
#endif
				return;
			}
			fvoie = 2;
			break;
		case 2:
			nb = sscanf (ptr, "%d %d %s %ld", &p1, &p3, portname, &lp4);
			if (nb != 4)
			{
#ifdef ENGLISH
				errport (lig, "Wrong number of parameters    ");
#else
				errport (lig, "Nombre de paramŠtres incorrect");
#endif
				return;
			}
			if ((p1 <= 0) || (p1 > NBCOM))
			{
#ifdef ENGLISH
				errport (lig, "COM Nb error ");
#else
				errport (lig, "Nø COM erron‚");
#endif
				return;
			}
			p2 = 0;
			sscanf (portname, "%x", &p2);
			n_cpy (19, p_com[p1].name, portname);
			p_com[p1].delai = 0;
			p_com[p1].cbase = p2;
			p_com[p1].pactor_st = 247;
#ifdef __linux__
			if (p3 != P_LINUX)
			{
#ifdef ENGLISH
				errport (lig, "Wrong interface number   ");
#else
				errport (lig, "Num‚ro d'interface erron‚");
#endif
				return;
			}
#endif
#ifdef __FBBDOS__
			if (p3 > P_TFPC)
			{
#ifdef ENGLISH
				errport (lig, "Wrong interface number   ");
#else
				errport (lig, "Num‚ro d'interface erron‚");
#endif
				return;
			}
#endif
#ifdef __WINDOWS__
			if (p3 > P_TFWIN)
			{
#ifdef ENGLISH
				errport (lig, "Wrong interface number   ");
#else
				errport (lig, "Num‚ro d'interface erron‚");
#endif
				return;
			}
#endif
			p_com[p1].combios = p3;
			p_com[p1].port = (int) lp4;
#ifdef __WINDOWS__
			if (p3 == P_WINDOWS)
			{
				p_com[p1].baud = lp4;
			}
			else
			{
#endif
#if defined(__linux__) || defined(__FBBDOS__)
			if (p3)
			{
#endif
				switch (lp4)
				{
				case 300L:
					p_com[p1].baud = 0x43;
					p_com[p1].options = 0;
					break;
				case 600L:
					p_com[p1].baud = 0x63;
					p_com[p1].options = 0;
					break;
				case 1200L:
					p_com[p1].baud = 0x83;
					p_com[p1].options = 0;
					break;
				case 2400L:
					p_com[p1].baud = 0xA3;
					p_com[p1].options = 0;
					break;
				case 4800L:
					p_com[p1].baud = 0xC3;
					p_com[p1].options = 0;
					break;
				case 0L:
				case 9600L:
					p_com[p1].baud = 0xE3;
					p_com[p1].options = 0;
					break;
				case 14400L:
					p_com[p1].baud = 0x23;
					p_com[p1].options = 0x20;
					break;
				case 19200L:
					p_com[p1].baud = 0x43;
					p_com[p1].options = 0x20;
					break;
				case 28800L:
					p_com[p1].baud = 0x63;
					p_com[p1].options = 0x20;
					break;
				case 38400L:
					p_com[p1].baud = 0x83;
					p_com[p1].options = 0x20;
					break;
				case 57600L:
					p_com[p1].baud = 0xA3;
					p_com[p1].options = 0x20;
					break;
				case 115200L:
					p_com[p1].baud = 0xC3;
					p_com[p1].options = 0x20;
					break;
				case 330400L:
					p_com[p1].baud = 0xE3;
					p_com[p1].options = 0x20;
					break;
				default:
#ifdef __linux__
					break;
#else
#ifdef ENGLISH
					errport (lig, "Wrong baud rate    ");
#else
					errport (lig, "Baud rate incorrect");
#endif
					return;
#endif
				}
			}
#ifdef __FBBDOS__
			else
			{
				p_com[p1].options = 0x0;
				switch (lp4)
				{
				case 300L:
					p_com[p1].baud = 384;
					break;
				case 600L:
					p_com[p1].baud = 192;
					break;
				case 1200L:
					p_com[p1].baud = 96;
					break;
				case 2400L:
					p_com[p1].baud = 48;
					break;
				case 4800L:
					p_com[p1].baud = 24;
					break;
				case 9600L:
					p_com[p1].baud = 12;
					break;
				case 14400L:
					p_com[p1].baud = 9;
					break;
				case 19200L:
					p_com[p1].baud = 6;
					break;
				case 28800L:
					p_com[p1].baud = 4;
					break;
				case 38400L:
					p_com[p1].baud = 3;
					break;
				case 57600L:
					p_com[p1].baud = 2;
					break;
				case 115200L:
					p_com[p1].baud = 1;
					break;
				case 330400L:
					p_com[p1].baud = 0;
					break;
				default:
#ifdef ENGLISH
					errport (lig, "Wrong baud rate    ");
#else
					errport (lig, "Baud rate incorrect");
#endif
					return;
				}
			}
#endif /* __FBBDOS__ */
			if (++nbl == nbport)
			{
				fvoie = 3;
				nbl = 0;
				tnc = 0;
				voie = 1;
			}
			break;
		case 3:
			p5 = 0;
			nb = sscanf (ptr, "%d %d %d %s %d %d %d %d %d/%d %s %s",
						 &p1, &p2, &p4, portname, &p6, &p7, &p9, &mx, &p3, &p8, mode, sfreq);
			if (nb != 12)
			{
#ifdef ENGLISH
				errport (lig, "Wrong number of parameters    ");
#else
				errport (lig, "Nombre de paramŠtres incorrect");
#endif
				return;
			}

			if (p1 > NBPORT)
			{
#ifdef ENGLISH
				errport (lig, "Wrong port number ");
#else
				errport (lig, "Num‚ro port erron‚");
#endif
				return;
			}
			if (p2 > MAXVOIES)
			{
#ifdef ENGLISH
				errport (lig, "Number of channels");
#else
				errport (lig, "Nombre de voies   ");
#endif
				return;
			}
			if (p2 > 0)
			{
				sscanf (portname, "%d", &p5);
				if (p5 > 99)
					p5 = 0;

				n_cpy (19, p_port[p1].name, portname);
				if ((p4 < 1) || (p4 > NBCOM))
				{
#ifdef ENGLISH
					errport (lig, "COM Nb error ");
#else
					errport (lig, "Nø COM erron‚");
#endif
					return;
				}
#ifndef __linux__
				if (p5 > 7)
				{
#ifdef ENGLISH
					errport (lig, "Channel must be from 0 to 7");
#else
					errport (lig, "Le canal doit etre de 0 … 7");
#endif
					return;
				}
#endif
				if ((p6 != 0) && ((p6 < 30) || (p6 > 250)))
				{
#ifdef ENGLISH
					errport (lig, "Paclen must be from 30 to 250  ");
#else
					errport (lig, "Le paquet doit etre de 30 … 250");
#endif
					return;
				}
				if ((p7 < 1) || (p7 > 7))
				{
#ifdef ENGLISH
					errport (lig, "Maxframe must be from 1 to 7          ");
#else
					errport (lig, "Le nombre de trames doit etre de 1 … 7");
#endif
					return;
				}
				if (p9 > p2)
				{
#ifdef ENGLISH
					errport (lig, "Too many forward channels");
#else
					errport (lig, "Trop de voies forward    ");
#endif
					return;
				}
				if (mx <= 0)
				{
#ifdef ENGLISH
					errport (lig, "Forwarding block size");
#else
					errport (lig, "Taille bloc forward  ");
#endif
					return;
				}
				if (strlen (sfreq) > 9)
				{
#ifdef ENGLISH
					errport (lig, "9 characters maximum for the frequency");
#else
					errport (lig, "9 caracteres maximum pour la frequence");
#endif
					return;
				}
				if (p3 > 59)
				{
#ifdef ENGLISH
					errport (lig, "Forward minute");
#else
					errport (lig, "Minute forward");
#endif
					return;
				}
				if ((p8 < 1) || (p8 > 60))
				{
#ifdef ENGLISH
					errport (lig, "Forward period ");
#else
					errport (lig, "P‚riode forward");
#endif
					return;
				}
			}

			/* Gestion du multiplexeur sur le COM (DED) */
			p_com[p4].multi[p5] = p1;

			if (p1 == 0)
			{
				if (p3 > 59)
				{
#ifdef ENGLISH
					errport (lig, "Forward minute");
#else
					errport (lig, "Minute forward");
#endif
					return;
				}
				if ((p8 < 1) || (p8 > 60))
				{
#ifdef ENGLISH
					errport (lig, "Forward period ");
#else
					errport (lig, "P‚riode forward");
#endif
					return;
				}
				p_port[p1].min_fwd = p3;
				p_port[p1].per_fwd = p8;
				break;
			}

			if (p_port[p1].pvalid)
			{
				errport (lig, "TNC number already declared");
			}

			p_port[p1].ccom = p4;
			p_port[p1].ccanal = p5;
			p_port[p1].maxbloc = mx;
			ptr = mode;
			p_port[p1].typort = TYP_DED;
			p_port[p1].moport = 0;
			while (*ptr)
			{
				switch (toupper (*ptr))
				{
				case 'G':
					p_port[p1].moport |= 1;
					break;
				case 'B':
					p_port[p1].moport |= 2;
					break;
				case 'Y':
					p_port[p1].moport |= 4;
					break;
				case 'M':
					p_port[p1].moport |= 8;
					p_port[p1].typort = TYP_MOD;	/* MODEM */
					break;
				case 'W':
					p_port[p1].moport |= 0x10;
					break;
				case 'L':
					p_port[p1].moport |= 0x20;
					break;
				case 'R':
					p_port[p1].moport |= 0x40;
					break;
#ifdef __linux__
				case 'S':
					p_port[p1].typort = TYP_POP;	/* POP SOCKET */
					p_port[p1].ccanal = 0;
					p5 = 0;
					break;
				case 'X':
					p_port[p1].typort = TYP_SCK;	/* AX25 SOCKET */
					p_port[p1].ccanal = 0;
					p5 = 0;
					break;
#endif
				case 'H':
					p_port[p1].typort = TYP_HST;	/* PTC-HOST */
					break;

				case 'D':
					p_port[p1].typort = TYP_DED;	/* DED */
					if ((p_com[p4].combios == P_COMBIOS) && ((p5 < 1) || (p5 > 4)))
					{
#ifdef ENGLISH
						errport (lig, "Mux channel must be from 1 to 4");
#else
						errport (lig, "Le canal MUX doit etre de 1 a 4");
#endif
						return;
					}
					break;
				case 'P':
					p_port[p1].typort = TYP_PK;		/* PK232 */
					if (p2 > 9)
						p2 = 9;
					break;
				case 'K':
					p_port[p1].typort = TYP_KAM;	/* KAM */
					break;
				case 'Q':
					p_port[p1].typort = TYP_BPQ;	/* BPQ4 */
					p_port[p1].ccanal = p5 + 1;
					break;
#if defined(__WINDOWS__) || defined(__linux__)
				case 'T':
					p_port[p1].typort = TYP_TCP;	/* TCP-IP */
					break;
				case 'E':
					p_port[p1].typort = TYP_ETH;	/* ETHER-LINK */
					break;
#endif
#if defined(__WINDOWS__)
				case 'A':
					p_port[p1].typort = TYP_AGW;	/* TCP-AGW */
					break;
#endif
#if defined(__WINDOWS__) || defined(__FBBDOS__)
				case 'F':
					p_port[p1].typort = TYP_FLX;	/* FLEXNET */
					break;
#endif
				case 'U':
					break;
				default:
#ifdef ENGLISH
					errport (lig, "Wrong port mode    ");
#else
					errport (lig, "Mode du port erron‚");
#endif
					return;
				}
				++ptr;
			}

			p_port[p1].pk_t = p6;

			if ((p_port[p1].typort == TYP_HST) && (p_port[p1].ccanal == 0))
			{
				/* Pactor port */
				p_port[p1].moport |= 0x80;
			}

			if (p6)
				p_port[p1].beacon_paclen = p6;
			else
				p_port[p1].beacon_paclen = 128;
			switch (p_com[p4].combios)
			{
			case 3:
				p_port[p1].typort = TYP_MOD;
				p_port[p1].moport |= 8;
				p_port[p1].pk_t = 128;
				break;
			case 4:
				p_port[p1].typort = TYP_DED;
				break;
			default:
				break;
			}
			p_port[p1].pvalid = p_port[p1].nb_voies = p2;
			p_port[p1].frame = p7;
			p_port[p1].listfwd = pprec = NULL;
			p_port[p1].min_fwd = p3;
			p_port[p1].per_fwd = p8;
			p_port[p1].pr_voie = NBVOIES;

			while (p9--)
			{
				pfwd = (Forward *) m_alloue (sizeof (Forward));
				if (p_port[p1].listfwd)
				{
					pprec->suite = pfwd;
					pprec = pfwd;
				}
				else
				{
					p_port[p1].listfwd = pprec = pfwd;
				}
				init_forward (pprec);
			}

			strn_cpy (9, p_port[p1].freq, sfreq);

			if (p2)
			{
				int tot;

#if defined(__WINDOWS__) || defined(__linux__)
				{
					char buf[80];

					sprintf (buf, "%d %d ch", p1, p2);
					InitText (buf);
				}
#endif
#ifdef __FBBDOS__
#ifdef ENGLISH
				cprintf ("Port %d ok : %d channel(s) \r\n", p1, p2);
#else
				cprintf ("Port %d valide : %d voie(s)\r\n", p1, p2);
#endif
#endif
				nbvalid += p2;
				tot = 0;

				for (i = 1; i <= p2; i++)
				{
					init_voie (NBVOIES);
					svoie[NBVOIES]->affport.port = p1;
#ifdef __linux__
					if (S_LINUX (p1))
					{
						tot = linux_canal++;
						svoie[NBVOIES]->affport.canal = tot;
					}
					else
#endif
					if (DRSI (p1))
					{
						tot = drsi_canal++;
						svoie[NBVOIES]->affport.canal = tot;
					}
					else if (BPQ (p1))
					{
						tot = bpq_canal++;
						svoie[NBVOIES]->affport.canal = tot;
					}
					else if (AGW (p1))
					{
						tot = agw_canal++;
						svoie[NBVOIES]->affport.canal = tot;
					}
					else if (HST (p1))
					{
						if (p_port[p1].ccanal == 0)
						{
							tot = 1;
							svoie[NBVOIES]->affport.canal = PACTOR_CH;
						}
						else
						{
							/* Include the pactor channel */
							tot = hst_canal++;
							svoie[NBVOIES]->affport.canal = tot;
							if (tot == PACTOR_CH)
							{
#ifdef ENGLISH
								errport (lig, "Too many channels !           ");
#else
								errport (lig, "Nombre de voies trop important");
#endif
							}
						}
					}
					else
					{
						tot = i;
						svoie[NBVOIES]->affport.canal = tot;
					}
					if (++NBVOIES == MAXVOIES)
					{
#ifdef ENGLISH
						errport (lig, "Too many channels !           ");
#else
						errport (lig, "Nombre de voies trop important");
#endif
						return;
					}
				}
				p_port[p1].tt_can = tot;
			}

			if (++tnc == nbtnc)
			{
				fvoie = 4;
				nbl = 0;
			}
			break;
		case 4:
			nb = sscanf (ptr, "%d %d %s %s", &p1, &p2, sfreq, mode);
			if (nb != 4)
			{
#ifdef ENGLISH
				errport (lig, "Wrong parameter number        ");
#else
				errport (lig, "Nombre de paramŠtres incorrect");
#endif
				return;
			}
			if (p2 > p_port[p1].nb_voies)
			{
#ifdef ENGLISH
				errport (lig, "Not enough channels on this port");
#else
				errport (lig, "Pas assez de voies sur ce port  ");
#endif
				return;
			}
			ptr = mode;
			local_mode = 0;
			while (*ptr)
			{
				switch (toupper (*ptr))
				{
				case 'G':
					local_mode |= 1;
					break;
				case 'B':
					local_mode |= 2;
					break;
				case 'Y':
					local_mode |= 4;
					break;
				case 'U':
					break;
				default:
#ifdef ENGLISH
					errport (lig, "Wrong port mode    ");
#else
					errport (lig, "Mode du port erron‚");
#endif
					return;
				}
				++ptr;
			}
			i = NBVOIES;
			while (p2)
			{
				if (--i == 0)
					break;
				if (no_port (i) == p1)
				{
					svoie[i]->sta.callsign.num =
						extind (sfreq, svoie[i]->sta.callsign.call);
					svoie[i]->localmode = local_mode;
#ifdef ENGLISH
					cprintf ("Channel %d on %s     \r\n", i, sfreq);
#else
					cprintf ("Voie %d affectee a %s\r\n", i, sfreq);
#endif
					--p2;
				}
			}
			break;
		case 5:
			break;
		}
	}

	for (voie = 0; voie < NBVOIES; voie++)
	{
		svoie[voie]->paclen = p_port[no_port (voie)].pk_t;
	}
#ifdef __linux__
#ifdef ENGLISH
	cprintf ("%d channels ok  \n", nbvalid);
#else
	cprintf ("%d voies valides\n", nbvalid);
#endif
#else
#ifdef ENGLISH
	cprintf ("%d channels ok  \r\n", nbvalid);
#else
	cprintf ("%d voies valides\r\n", nbvalid);
#endif
#endif

	MWARNING = NBVOIES;
	init_voie (MWARNING);

	ferme (fptr, 6);
	attend_caractere (1);
}


static void errport (int lig, char *texte)
{
#ifdef __FBBDOS__
#ifdef ENGLISH
	cprintf ("Error file PORT.SYS line %d :      \r\n< %s >\r\n\a", lig, texte);
#else
	cprintf ("Erreur fichier PORT.SYS ligne %d : \r\n< %s >\r\n\a", lig, texte);
#endif
	attend_caractere (10);
#endif
#if defined(__WINDOWS__) || defined(__linux__)
	char msg[80];

#ifdef ENGLISH
	sprintf (msg, "Error file PORT.SYS line %d :      \n< %s >", lig, texte);
#else
	sprintf (msg, "Erreur fichier PORT.SYS ligne %d : \n< %s >", lig, texte);
#endif
	WinMessage (5, msg);
#endif
	fbb_error (ERR_SYNTAX, c_disque ("PORT.SYS"), lig);
}


static void err_fic (char *nomfic, int ligne, char *texte)
{
#ifdef __FBBDOS__
	cprintf ("\r\n%s (%d) : %s\r\n\a", nomfic, ligne, texte);
	attend_caractere (10);
#endif
#if defined(__WINDOWS__) || defined(__linux__)
	char msg[80];

	sprintf (msg, "%s (%d) :\n%s", nomfic, ligne, texte);
	WinMessage (0, msg);
#endif
	fbb_error (ERR_SYNTAX, nomfic, ligne);
}

static unsigned int tail_lang = 0;

void end_textes (void)
{
	int cpt;

	if (langue == NULL)
		return;

	m_libere (nomlang, LG_LANG * maxlang);
	for (cpt = 0; cpt < NBLANG; cpt++)
	{
		m_libere (langue[cpt]->plang[0], tail_lang);
		m_libere (langue[cpt], sizeof (tlang));
	}
	m_libere (langue, NBLANG * sizeof (tlang *));

	langue = NULL;
	vlang = -1;
}

void initexte (void)
{
	int cpt, nb, niv, nbc, ligne;
	unsigned int taille = 0;
	FILE *fptr;
	char s[81], nomfic[81];

#ifdef __FBBDOS__
	fen *fen_ptr;

#endif

	ligne = niv = 0;

	strcpy (nomfic, "LANGUE.SYS");
	if ((fptr = fopen (c_disque (nomfic), "rt")) == NULL)
	{
#ifdef ENGLISH
		err_fic (nomfic, ligne, "File LANGUE.SYS not found  ");
#else
		err_fic (nomfic, ligne, "Erreur ouverture LANGUE.SYS");
#endif
		return;
	}

#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Texts set-up             \n");

#else
	cprintf ("Initialisation des textes\n");

#endif
#else
#ifdef ENGLISH
	cprintf ("Texts set-up             \r\n");

#else
	cprintf ("Initialisation des textes\r\n");

#endif
#endif
	nbc = cpt = 0;
#ifdef ENGLISH
	strcpy (s, "      Texts set-up       ");
#else
	strcpy (s, "Initialisation des textes");
#endif
#ifdef __FBBDOS__
	fen_ptr = open_win (10, 6, 60, 16, INIT, s);
#endif
	while (fgets (s, 80, fptr))
	{
		++ligne;
		if (*s == '#')
			continue;
		switch (niv)
		{
		case 0:
			if (isdigit (*s))
			{
				nb = sscanf (s, "%d %d %d", &nbc, &NBLANG, &deflang);

				if (nb != 3)
				{
					deflang = 0;
					NBLANG = 3;
				}

				if (deflang > nbc)
					deflang = 0;
				else
					deflang--;

				/*
				   if (NBLANG < 2)
				   NBLANG = 2 ;
				 */
				if (NBLANG > nbc)
					NBLANG = nbc;
				if (nbc < 1)
#ifdef ENGLISH
					err_fic (nomfic, ligne, "Error languages number  ");
#else
					err_fic (nomfic, ligne, "Erreur nombre de langues");
#endif
			}
			else
#ifdef ENGLISH
				err_fic (nomfic, ligne, "Error : No languages number");
#else
				err_fic (nomfic, ligne, "Manque nombre de langues   ");
#endif
			++niv;
			nomlang = (char *) m_alloue (LG_LANG * nbc);
#ifdef __linux__
#ifdef ENGLISH
			cprintf ("%d language buffers allocated\n", NBLANG);
#else
			cprintf ("%d buffers langue allou‚s    \n", NBLANG);
#endif
#else
#ifdef ENGLISH
			cprintf ("%d language buffers allocated\r\n", NBLANG);
#else
			cprintf ("%d buffers langue allou‚s    \r\n", NBLANG);
#endif
#endif
			break;
		case 1:
			if (!ISPRINT (*nomfic))
				break;
			strn_cpy (LG_LANG - 1, nomlang + cpt * LG_LANG, sup_ln (s));
#ifdef __WINDOWS__
			InitText (nomlang + cpt * LG_LANG);
#endif
#ifdef __FBBDOS__
			cprintf ("\r\n%8s ", nomlang + cpt * LG_LANG);
#endif
#ifdef __linux__
			printf ("Init lang %9s\n", nomlang + (cpt * LG_LANG));
#endif
			nb = lang_len (nomlang + (cpt * LG_LANG));
			if (taille < nb)
				taille = nb;
			if (++cpt == nbc)
				++niv;
			break;
		}
		if (niv == 2)
			break;
	}
	fclose (fptr);
	maxlang = cpt;
	tail_lang = taille;

	/* Alloue les NBLANG buffers de pointeurs et de textes */
	langue = (tlang * *)m_alloue (NBLANG * sizeof (tlang *));

	for (cpt = 0; cpt < NBLANG; cpt++)
	{
		langue[cpt] = (tlang *) m_alloue (sizeof (tlang));
		langue[cpt]->plang[0] = (char *) m_alloue (taille);
		langue[cpt]->numlang = -1;
	}
	swap_langue (0, deflang);
#ifdef __FBBDOS__
	close_win (fen_ptr);
#endif
}


static int lang_len (char *nom_lang)
{
	char nomfic[81];
	char texte[81];
	char chaine[300];
	int nb, taille;
	FILE *fpl;

	sprintf (nomfic, "LANG\\%s.TXT", nom_lang);
	if ((fpl = fopen (c_disque (nomfic), "rb")) == NULL)
	{
#ifdef ENGLISH
		sprintf (chaine, "File %s not found", nomfic);
#else
		sprintf (chaine, "Erreur fichier %s", nomfic);
#endif
		err_fic (nomfic, 0, chaine);
	}
	taille = (size_t) 0;

	nb = 0;
	while (fgets (chaine, 257, fpl))
	{
		if ((*chaine == '#') || (*chaine == '\0') || (*chaine == '\032'))
			continue;
		taille += (unsigned int) strlen (chaine);
		if (++nb > NBTEXT)
			break;
#ifdef __FBBDOS__
		if ((nb % 10) == 0)
			putch ('.');
#endif
	}
	if (nb != NBTEXT)
	{
#ifdef ENGLISH
		sprintf (texte, "Wrong line number : need %d - found %d             ", NBTEXT, nb);
#else
		sprintf (texte, "Nombre de lignes incorrect : attendu %d - trouve %d", NBTEXT, nb);
#endif
		err_fic (nomfic, nb, texte);
	}
	fclose (fpl);
	return (taille + 256);
}


void swap_langue (int num_buf, int num_lang)
{
	char nomfic[81];
	char chaine[300];
	char *txt_ptr;
	int nb;
	FILE *fpl;

#ifdef __FBBDOS__
	fen *fen_ptr;

#endif

	if (langue[num_buf]->numlang == num_lang)
		return;

	txt_ptr = langue[num_buf]->plang[0];
	sprintf (nomfic, "LANG\\%s.TXT", nomlang + num_lang * LG_LANG);
	if ((fpl = fopen (c_disque (nomfic), "rb")) == NULL)
	{
#ifdef ENGLISH
		sprintf (chaine, "File %s not found", nomfic);
#else
		sprintf (chaine, "Erreur fichier %s", nomfic);
#endif
		err_fic (nomfic, 0, chaine);
	}

	deb_io ();

#ifdef __FBBDOS__
#ifdef ENGLISH
	fen_ptr = open_win (55, 4, 75, 6, INIT, " Text ");
#else
	fen_ptr = open_win (55, 4, 75, 6, INIT, "Textes");
#endif
#endif
	nb = 0;
	while (fgets (chaine, 257, fpl))
	{
		sup_ln (chaine);
		if ((*chaine == '#') || (*chaine == '\0'))
			continue;
#ifdef __FBBDOS__
		if ((nb % 50) == 0)
			putch ('.');
#endif
		strcpy (txt_ptr, chaine);
		langue[num_buf]->plang[nb] = txt_ptr;
		txt_ptr += strlen (chaine) + 1;
		++nb;
		if (nb > NBTEXT)
			break;
	}

	fclose (fpl);
	langue[num_buf]->numlang = num_lang;
#ifdef __FBBDOS__
	close_win (fen_ptr);
#endif
	fin_io ();
}


void read_heard (void)
{
	int i;
	FILE *fptr;

	if ((fptr = fopen (d_disque ("HEARD.BIN"), "rb")) != NULL)
	{
		for (i = 0; i < NBPORT; i++)
		{
			fread (p_port[i].heard, sizeof (Heard), NBHEARD, fptr);
		}
		fclose (fptr);
	}
}


void write_heard (void)
{
	int i;
	FILE *fptr;

	if (p_port == NULL)
		return;

	if ((fptr = fopen (d_disque ("HEARD.BIN"), "wb")) != NULL)
	{
		for (i = 0; i < NBPORT; i++)
		{
			fwrite (p_port[i].heard, sizeof (Heard), NBHEARD, fptr);
		}
		fclose (fptr);
	}
}

