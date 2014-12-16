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
 * PTC-II HostMode interface
 *
 * It has two ports + 1 pactor channel
 *
 * Pactor channel is renumbered to 31, other ports are limited to 30 channels
 */

#include <serv.h>
#include <fbb_drv.h>

/* Caractere de header */
#define HST_CHAR	170
#define PACTOR_ST 254
/* #define DEBUG_HST */

#define STATUS(p) (p_com[(int)p_port[p].ccom].pactor_st)

static char pactor[5];

static int hst_end_host (int);

static int hst_send_data (int);
static int hst_send_dt (int, int, char *, int);
static int hst_send_ui (int, char *, int, Beacon *);
static int hst_sndcmd (int, int, char *, int);
static int nb_waiting (int);
static int recv_hst (int, int, int *, char *);
static int hst_busy (int port, int *canal, int force);

static void hst_get_ui (int, char *, ui_header *);
static void hst_sonde (int, int);
static int hst_compteur (int);

static unsigned short *crctab;

#define updcrc(cp, crc) (((crc >> 8) & 0xff) ^ crctab[((crc ^ (cp & 0xff)) & 0xff)])

static int DebugCh = 0;
static void DebugCmd (int port)
{
	int type = (p_port[port].last->cmd == COMMAND) ? 1 : 0;

	if (DebugCh)
	{
		type |= p_port[port].last->compteur;
	}
}

#ifdef DEBUG_HST

static void dump_data (FILE * fptr, char *ptr, int nb)
{
	int i;
	int j;

	for (i = 0; i < nb; i += 16)
	{
		fprintf (fptr, "%03d ", i);
		for (j = 0; j < 16; j++)
		{
			if ((i + j) < nb)
				fprintf (fptr, "%02x ", ptr[i + j] & 0xff);
			else
				fprintf (fptr, "   ");
		}
		for (j = 0; j < 16; j++)
		{
			char c = ptr[i + j] & 0xff;

			if ((i + j) < nb)
				putc (isprint (c) ? c : '.', fptr);
			else
				putc (' ', fptr);
		}
		putc ('\n', fptr);
	}
}

static void bin_recv (char *ptr, int nb)
{
	FILE *fptr = fopen ("/tmp/debug.hst", "a+b");
	long temps = time (NULL);

	if (fptr == NULL)
		return;

	fprintf (fptr, "\nRX=%-3d %s", nb, asctime (localtime (&temps)));
	dump_data (fptr, ptr, nb);
	fclose (fptr);
}

static void bin_send (char *ptr, int nb)
{
	FILE *fptr = fopen ("/tmp/debug.hst", "a+b");
	long temps = time (NULL);

	if (fptr == NULL)
		return;

	fprintf (fptr, "\nTX=%-3d %s", nb, asctime (localtime (&temps)));
	dump_data (fptr, ptr, nb);
	fclose (fptr);
}
#endif

static void initcrc (void)
{
	unsigned int i, j;
	unsigned short accu, data;

	crctab = malloc (256 * sizeof (unsigned short));

	for (i = 0; i < 256; i++)
	{
		accu = 0;
		data = i;
		for (j = 0; j < 8; j++)
		{
			if ((data ^ accu) & 1)
				accu = (accu >> 1) ^ 0x8408;
			else
				accu = accu >> 1;
			data >>= 1;
		}
		crctab[i] = accu;
	}
}

int hst_vide (int port, int echo)
{
	int c, nb = 0;

	df ("vide", 2);

	if ((DEBUG) || (!p_port[port].pvalid))
	{
		deb_io ();
		cprintf ("Erreur port %d!\n", port);
		fin_io ();
		ff ();
		return (0);
	}

	p_port[port].portind = 0;

	tempo = 20;
	deb_io ();
	while (tempo)
	{
		if ((c = rec_tnc (port)) >= 0)
		{
			if (echo)
			{
#ifdef __FBBDOS__
				putch (c);
				if (c == '\r')
					putch ('\n');
#endif
			}
			tempo = 20;
			++nb;
		}
#ifdef __WINDOWS__
		else
		{
			BWinSleep (100);
			if (tempo > 0)
				--tempo;
		}
#endif
#ifdef __LINUX__
		else
		{
			usleep (100000);
			if (tempo > 0)
				--tempo;
		}
#endif
	}
	fin_io ();
	ff ();
	return (nb);
}

/*
 * Fonctions g‚n‚riques du driver
 */

int opn_hst (int port, int nb)
{
	static int init = 0;
	long bt;
	int reset = 4;
	int nb_can;
	int i;
	int ok;
	char s[80];

	/* Only one INIT on this tnc */

	p_port[port].last = NULL;

	while (reset)
	{
		ok = 0;
		sprintf (s, "Init PORT %d COM%d-%d PTCII",
				 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__LINUX__)
		InitText (s);
#else
		cprintf ("%s\r\n", s);
#endif
		if (init)
		{
			sleep_ (1);
			return (1);
		}

		init = 1;

		initcrc ();

		sprintf (s, "Init PORT %d COM%d-%d PTCII",
				 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__LINUX__)
		InitText (s);
#else
		cprintf ("%s\r\n", s);
#endif
/*      sleep_(1); */
		tncstr (port, "\r\033\rRESTART\r", 0);

		bt = btime () + 100;
		do
		{
			if (rec_tnc (port) < 0)
			{
				ok = 1;
				break;
			}
		}
		while (btime () < bt);

		hst_vide (port, 1);

		sprintf (s, "\r\033\rPTC %d\r", PACTOR_CH);
		tncstr (port, s, 0);
		
		sprintf (pactor, "(%d)", PACTOR_CH);

		bt = btime () + 20;
		do
		{
			if (rec_tnc (port) < 0)
			{
				ok = 1;
				break;
			}
		}
		while (btime () < bt);

		if (ok == 1)
			break;

		--reset;
	}
	sprintf (s, "Clear PORT %d COM%d-%d PTCII",
			 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__LINUX__)
	InitText (s);
#else
	cprintf ("%s\r\n", s);
#endif
	tncstr (port, "\033JHOST4\r", 0);

	sprintf (s, "Prog PORT %d COM%d-%d PTCII",
			 port, p_port[port].ccom, p_port[port].ccanal);
	hst_vide (port, 1);
#if defined(__WINDOWS__) || defined(__LINUX__)
	InitText (s);
#else
	cprintf ("%s\r\n", s);
#endif
	/* Parametres par defaut */
/*	sprintf (s, "#PTC %d\r", PACTOR_CH);
	hst_sndcmd (port, 0, (char *) s, 1);
	hst_sndcmd (port, 0, (char *) s, 1); */
	
	/* All channels callsign */
	sprintf (s, "I %s-%d", mycall, myssid);
	hst_sndcmd (port, 0, (char *) s, 1);
	hst_sndcmd (port, 0, (char *) s, 1);

	/* Port pactor */
	sprintf (s, "I %s", mycall);
	hst_sndcmd (port, PACTOR_CH, (char *) s, 1);

	if (DRSI (port))
	{
		nb_can = 0;
		for (i = 1; i < NBPORT; i++)
		{
			if (p_port[i].ccom == p_port[port].ccom)
			{
				nb_can += p_port[i].nb_voies;
			}
		}
	}
	else
	{
		nb_can = p_port[port].nb_voies;
	}
	sprintf (s, "Y %d", nb_can);
	hst_sndcmd (port, 0, (char *) s, 1);
	sprintf (s, "O %d", p_port[port].frame);
	hst_sndcmd (port, 0, (char *) s, 1);
	sprintf (s, "%%L 0");
	hst_sndcmd (port, 0, (char *) s, 1);
	sprintf (s, "#PD 0");
	hst_sndcmd (port, PACTOR_CH, (char *) s, 1);
	sprintf (s, "#UML 0");
	hst_sndcmd (port, PACTOR_CH, (char *) s, 1);
	sprintf (s, "#BOX 0");
	hst_sndcmd (port, PACTOR_CH, (char *) s, 1);
	sprintf (s, "#REM 0");
	hst_sndcmd (port, PACTOR_CH, (char *) s, 1);
	sprintf (s, "#MA 0");
	hst_sndcmd (port, PACTOR_CH, (char *) s, 1);
	sprintf (s, "#CMSG 0");
	hst_sndcmd (port, PACTOR_CH, (char *) s, 1);
	return (ok);
}


void debug_state (int port)
{
}


static void FAR hst_iss (int port, void *userdata)
{
	/* Automatic break_in */
	hst_sndcmd (port, PACTOR_CH, "%I", 2);
	p_port[port].t_iss = NULL;
}

static int is_data (int port)
{
	PortData *cmd = p_port[port].cmd;

	while (cmd)
	{
		if (cmd->cmd == DATA)
		{
			return (1);
		}
		cmd = cmd->next;
	}
	return (0);
}

static void test_timings (int port)
{
	if (!ONLINE (port) || (p_port[port].ccanal != 0))
		return;

	if (!ISS (port) && (is_data (port)))
	{
		if (!p_port[port].t_iss)
		{
			p_port[port].t_iss = add_timer (10, port, (void FAR *) hst_iss, NULL);
		}
	}
	else
	{
		del_timer (p_port[port].t_iss);
		p_port[port].t_iss = NULL;
	}
}

int rcv_hst (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui)
{
	int lgbuf=0;
	int can = p_port[*port].cur_can & 0xff;
	int code;
	int valid;
	int next = 1;
	char *ptr;

	/* Header provisoire */
	static ui_header loc_ui[NBPORT];

	*cmd = INVCMD;

	if ((BUSY (*port)) && (hst_busy (*port, canal, FALSE)))
	{
		*cmd = COMMAND;
		sprintf (buffer, "(%d) FREQ-BUSY fm PACTOR", *canal);
		*len = strlen (buffer);
		return (1);
	}

	/* Test si c'est le port correspondant au MUX ... */
	if ((operationnel) && (*port != p_com[(int)p_port[*port].ccom].mult_sel))
		return (-1);


	if ((p_port[*port].polling == 0) && (p_port[*port].last == NULL))
	{
		hst_sonde (*port, next);
		return (0);
	}

	valid = 0;

	deb_io ();
	code = recv_hst (*port, can, &lgbuf, buffer);
	fin_io ();

	switch (code)
	{
	case 0:
		*cmd = NOCMD;
		*len = 0;
		p_port[*port].polling = 0;
		if (p_port[*port].last_cmde == 'G')
		{
			p_port[*port].wait[can] = 0;
		}
		break;
	case 1:
		switch (p_port[*port].last_cmde)
		{
		case 'L':
			{
				/* Retour de statistiques */
				stat_ch sta;
				int nbmes, nbtra, nbatt, nback, nbret, con;

				valid = 2;
				*cmd = STATS;
				*canal = can;
				*len = 0;
				sscanf (buffer, "%d%d%d%d%d%d",
						&nbmes, &nbtra, &nbatt, &nback, &nbret, &con);
				memset (&sta, 0, sizeof (sta));
				sta.stat = con;
				sta.ret = nbret;
				p_port[*port].wait[can] = nbmes + nbtra;
				sta.ack = nbatt + nback + nb_waiting (*port);
				memcpy (buffer, &sta, sizeof (sta));
				if (p_port[*port].wait[can])
					next = 0;
			}
			break;
		case 'B':
			{
				valid = 2;
				*cmd = NBBUF;
				*len = 0;
				*((int *) buffer) = atoi (buffer);
				break;
			}
		case 'T':
			{
				valid = 2;
				*cmd = NBCHR;
				*len = 0;
				*canal = PACTOR_CH;
				*((long *) buffer) = atol (buffer);
				break;
			}
		default:
			valid = 2;
			*len = lgbuf;
			*cmd = ECHOCMD;
			break;
		}
		p_port[*port].polling = 0;
		break;
	case 2:
		valid = 2;
		*len = lgbuf;
		*cmd = ERRCMD;
		p_port[*port].polling = 0;
		if (p_port[*port].wait[can])
			--p_port[*port].wait[can];
		break;
	case 3:
		valid = 2;
		*cmd = COMMAND;
		*canal = can;
		*len = lgbuf;
		p_port[*port].polling = 0;
		if (strncmp (buffer, pactor, strlen (pactor)) == 0)
		{
			char *scan;

			scan = strchr (buffer, ':');
			if (scan && (scan != buffer))
			{
				--scan;
				*scan = '0';
			}
		}

		ptr = strstr (buffer, " CONNECTED");
		if (ptr && (p_port[*port].ccanal == 0))
		{
			/* Connexion sur le pactor. Forcer le statut ! */
			int com = p_port[*port].ccom;

			p_com[com].pactor_st = 0xa2;
			debug_state (*port);
		}

		if ((ptr) && (hst_busy (*port, canal, TRUE)))
		{
			memcpy (ptr, " RECONNECT", 10);
		}

		if (p_port[*port].wait[can])
			--p_port[*port].wait[can];
		break;
	case 4:
		hst_get_ui (*port, buffer, ui);
		valid = 1;
		*cmd = UNPROTO;
		*canal = can;
		*len = 0;
		p_port[*port].idem = 1;
		p_port[*port].polling = 0;
		if (p_port[*port].wait[can])
			--p_port[*port].wait[can];
		break;
	case 5:
		hst_get_ui (*port, buffer, ui);
		/* Sauvegarde l'UI */
		loc_ui[*port] = *ui;
		valid = 0;
		next = 0;
		p_port[*port].polling = 0;
		break;
	case 6:
		/* Recupere l'UI du coup precedant */
		*ui = loc_ui[*port];
		sprintf (ui->txt, " (%d)", lgbuf);
		valid = 1;
		*cmd = UNPROTO;
		*canal = can;
		*len = lgbuf;
		p_port[*port].idem = 1;
		p_port[*port].polling = 0;
		if (p_port[*port].wait[can])
			--p_port[*port].wait[can];
		break;
	case 7:
		if (can == PACTOR_ST)
		{
			int com = p_port[*port].ccom;

			p_com[com].pactor_st = *buffer;
			debug_state (*port);
			test_timings (*port);
			p_port[*port].polling = 0;
			hst_sonde (*port, next);
			return (0);
		}
		valid = 1;
		*cmd = DATA;

		{
			int res = ONLINE (*port);

			if ((can != PACTOR_CH) || (res))
			{
				*cmd = DATA;
			}
			else
			{
				*cmd = UNPROTO;
				memset (ui, 0, sizeof (ui_header));
				ui->port = *port;
				strn_cpy (11, ui->from, "PACTOR");
				ui->ui = 1;
			}
		}

		*canal = can;
		*len = lgbuf;
		p_port[*port].idem = 1;
		p_port[*port].polling = 0;
		if (p_port[*port].wait[can])
			--p_port[*port].wait[can];
		break;
	}

	/* Poll du TNC pour la prochaine fois */
	if (code != -1)
	{
		if (p_port[*port].wait[can])
			next = 0;
		hst_sonde (*port, next);
	}

	return (valid);
}

int cls_hst (int port)
{
	int i;

	/* Vide la file des commandes en attente pour tous les ports du TNC */
	for (i = 1; i < NBPORT; i++)
	{
		if ((HST (i)) && (p_port[port].ccom == p_port[i].ccom))
		{
			while (p_port[i].cmd)
			{
				PortData *cmd = p_port[i].cmd;

				p_port[i].cur_can = cmd->canal;
				switch (cmd->cmd)
				{
				case COMMAND:
					p_port[i].last = cmd;
					p_port[i].last->compteur = 0xc0;
					hst_send_data (i);

					/* Attend la reponse 1 seconde */
					sleep_ (1);

					/* Vide le buffer */
					while (rec_tnc (port) != -1);

					/* La reponse a ete lue et ignoree ... */
					break;
				default:
					break;
				}
				p_port[i].cmd = cmd->next;
				m_libere (cmd, sizeof (PortData));
				p_port[i].last_cmde = '\0';
			}
		}
	}

	/* Fin du mode host */
	hst_end_host (port);
	return (1);
}

int sta_hst (int port, int canal, int cmd, void *ptr)
{
	int val;

	/* Pactor port */
	if (p_port[port].moport & 0x80)
		canal = PACTOR_CH;

	switch (cmd)
	{
/*      case TNCSTAT :
   return(lit_stat_hst(port, canal, (stat_ch *)ptr)); */
	case TOR:
		canal = PACTOR_CH;
		val = (hst_sndcmd (port, canal, (char *) ptr, 0) != -1);
		return (val);
	case SNDCMD:
		return (hst_sndcmd (port, canal, (char *) ptr, 0) != -1);
	case ECHOCMD:
		return (hst_sndcmd (port, canal, (char *) ptr, 1) != -1);
	case PORTCMD:
		return (hst_sndcmd (port, 0, (char *) ptr, 1) != -1);
	case PACLEN:
		*((int *) ptr) = 250;
		return (1);
	}
	return (0);
}

int snd_hst (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	int ret = 0;

	df ("snd_hst", 8);

	if (p_port[port].synchro > 256)
		return (0);

	/* Pactor port */
	if (p_port[port].moport & 0x80)
		canal = PACTOR_CH;

	switch (cmd)
	{
	case COMMAND:
		break;

	case DATA:
		ret = hst_send_dt (port, canal, buffer, len);
		break;

	case UNPROTO:
		ret = hst_send_ui (port, buffer, len, ptr);
		break;
	}
	ff ();
	return (ret);
}

/* Fonctions locales */

static int hst_end_host (int port)
{
	PortData *cmd;
	static int done = 0;

	if (done)
		return (1);

	done = 1;

	cmd = (PortData *) m_alloue (sizeof (PortData));
	cmd->canal = 0;
	cmd->cmd = COMMAND;
	cmd->len = 2;
	strcpy (cmd->buf, "MN");
	p_port[port].last = cmd;

	p_port[port].last->compteur = hst_compteur (port);
	hst_send_data (port);

	m_libere (p_port[port].last, sizeof (PortData));

	cmd = (PortData *) m_alloue (sizeof (PortData));
	cmd->canal = 0;
	cmd->cmd = COMMAND;
	cmd->len = 6;
	strcpy (cmd->buf, "JHOST0");
	p_port[port].last = cmd;

	p_port[port].last->compteur = hst_compteur (port);
	hst_send_data (port);

	m_libere (p_port[port].last, sizeof (PortData));
	p_port[port].last = NULL;

	return (1);
}

/*
 * Fonctions locales
 */

static int hst_compteur (int port)
{
	static int val = 2;
	int com = p_port[port].ccom;

	if (val == 2)
	{
		p_com[com].compteur = 0xc0;
	}
	else
	{
		p_com[com].compteur = (val) ? 0x80 : 0;
	}
	val = !val;

	return p_com[com].compteur;
}

static void warning (unsigned numero, char *texte)
{
}

/* Retourne le port logique correspondant au lport physique d'un TNC
   static int global_port(int com, int lport)
   {
   int p;

   for (p = 1 ; p < NBPORT ; p++)
   {
   if ((p_port[p].ccom == com) && (p_port[p].ccanal == lport))
   {
   return (p);
   }
   }
   return(1);
   }
 */

static int iss (int port)
{
	return (ISS (port));
}

static int online (int port)
{
	return (ONLINE (port));
}

static PortData *get_cmd (int port)
{
	PortData *prc = NULL;
	PortData *cmd = p_port[port].cmd;

	/* On envoie les data ou le CHO uniquement si le PACTOR est en ISS */
	if (p_port[port].ccanal == 0)
	{
		/* Canal PACTOR */
		while (cmd)
		{
			if ((!online (port)) || (iss (port)) || ((cmd->cmd == COMMAND) && (strcmp (cmd->buf, "%O") != 0)))
			{
				if (prc)
					prc->next = cmd->next;
				else
					p_port[port].cmd = cmd->next;
				return (cmd);
			}
			prc = cmd;
			cmd = cmd->next;
		}
		return (NULL);
	}
	else if (cmd)
	{
		p_port[port].cmd = cmd->next;
	}

	return (cmd);
}

static void hst_sonde (int pp, int next)
{
	static int cptr[NBPORT];
	static int first = 1;
	static int debug_cmd = 50;
	PortData *cmd = NULL;

	if ((pp < 0) || (pp >= NBPORT))
		return;

	if (first)
	{
		/* Initializes static variables */
		int i;

		for (i = 0; i < NBPORT; i++)
		{
			cptr[i] = 0;
		}
		first = 0;
	}

	/* if (p_port[pp].cur_can == 0xff)
	   p_port[pp].cur_can = 0xfe; */

	if (p_port[pp].last)
	{
		return;
	}

	if (next == 0)
	{
		cmd = (PortData *) m_alloue (sizeof (PortData));
		cmd->canal = p_port[pp].cur_can;
		cmd->cmd = COMMAND;
		cmd->len = 1;
		strcpy (cmd->buf, "G");
		p_port[pp].last = cmd;
		p_port[pp].last_cmde = 'G';
	}
	else if ((cmd = get_cmd (pp)) != NULL)
	{
		/* Il y a une commande a envoyer */

		if (cmd->cmd != COMMAND)
			cptr[pp] = 0;

		p_port[pp].last = cmd;
		if (strcmp (cmd->buf, "%T") == 0)
			p_port[pp].last_cmde = 'T';
		else
			p_port[pp].last_cmde = '\0';
	}
	else if (operationnel == 1)
	{
		if ((cptr[pp] == 0) && (p_port[pp].cur_can != PACTOR_ST))
		{
			cmd = (PortData *) m_alloue (sizeof (PortData));
			cmd->canal = p_port[pp].cur_can;
			cmd->cmd = COMMAND;
			cmd->len = 2;
			strcpy (cmd->buf, "@B");
			p_port[pp].last = cmd;
			p_port[pp].last_cmde = 'B';
			cptr[pp] = 50;
		}
		else if (p_port[pp].cur_can == PACTOR_CH)
		{
			/* Status */
			cmd = (PortData *) m_alloue (sizeof (PortData));

			p_port[pp].cur_can = PACTOR_ST;
			cmd->canal = p_port[pp].cur_can;
			cmd->cmd = COMMAND;
			cmd->len = 1;
			strcpy (cmd->buf, "G");
			p_port[pp].last = cmd;
			p_port[pp].last_cmde = 'G';
		}
		else
		{
			cmd = (PortData *) m_alloue (sizeof (PortData));

			/* Passe au canal suivant */
			if ((p_port[pp].idem == 0) || (p_port[pp].cur_can == PACTOR_ST))
			{
				int max;

				if (p_port[pp].moport & 0x80)
					max = 1;
				else
				{
					max = p_port[pp].tt_can;
				}

				++(p_port[pp].cur_can);
				if (p_port[pp].cur_can > max)
				{
					int mux_ch;
					int i;
					int com;
					int first_mux;
					int last_mux;

					/* Passe eventuellement au PORT/MUX suivant */
					mux_ch = p_port[pp].ccanal;
					com = p_port[pp].ccom;

					/*
					   first_mux = (DRSI (pp)) ? 0 : 1;
					   last_mux = (DRSI (pp)) ? 7 : 4;
					 */
					first_mux = 0;
					last_mux = 2;

					for (i = first_mux; i < last_mux; i++)
					{
						++mux_ch;
						if (mux_ch > last_mux)
							mux_ch = first_mux;
						if (p_com[com].multi[mux_ch])
						{
							pp = p_com[com].multi[mux_ch];
							p_com[com].mult_sel = pp;
							break;
						}
					}

					p_port[pp].cur_can = 0;

					/* Fait la somme des canaux deja balayes (1 seul TNC !!) */
					if (p_port[pp].ccanal == 0)
					{
						/* Inclut le monitoring */
						p_port[pp].cur_can = 0;
					}
					else
					{
						p_port[pp].cur_can = 1;
						for (i = 1; i < pp; i++)
						{
							if ((HST (i)) && (!IS_PACTOR (i)) && (p_port[i].ccom == p_port[pp].ccom))
							{
								p_port[pp].cur_can += p_port[i].nb_voies;
							}
						}
					}
				}
				if ((p_port[pp].cur_can == 1) && (p_port[pp].moport & 0x80))
					p_port[pp].cur_can = PACTOR_CH;
			}
			else
				p_port[pp].idem = 0;
			cmd->canal = p_port[pp].cur_can;
			cmd->cmd = COMMAND;
			cmd->len = 1;
			strcpy (cmd->buf, "L");
			p_port[pp].last = cmd;
			p_port[pp].last_cmde = 'L';
			if (cptr[pp])
				--cptr[pp];
		}
	}

	DebugCh = 0;
	if (debug_cmd > 0)
	{
		DebugCh = 1;
		--debug_cmd;
	}
	p_port[pp].last->compteur = hst_compteur (pp);
	hst_send_data (pp);
}

static int hst_busy (int port, int *canal, int force)
{
	if (p_port[port].ccanal != 0)
		return (0);

	*canal = PACTOR_CH;

	if (p_port[port].t_busy)
	{
		m_libere (p_port[port].t_busy->userdata, sizeof (PortData));
		del_timer (p_port[port].t_busy);
		p_port[port].t_busy = NULL;
		return (1);
	}
	else if (force && (p_port[port].t_wait))
	{
		/* Supprimer le timer wait */
		m_libere (p_port[port].t_wait->userdata, sizeof (PortData));
		del_timer (p_port[port].t_wait);
		p_port[port].t_wait = NULL;
		return (1);
	}
	return (0);
}

static void FAR hst_startscan (int port, PortData * command)
{
	PortData *cmd = p_port[port].cmd;

	if (ISS (port))
	{
		/* Still sending - postpone the command */
		add_timer (1, port, (void FAR *) hst_startscan, command);
		return;
	}

	/* Insert the command in top of queue */
	command->next = cmd;
	p_port[port].cmd = command;
	pactor_scan[port] = 1;
}

static void FAR hst_connect (int port, PortData * command)
{
	PortData *cmd = p_port[port].cmd;

	p_port[port].t_busy = NULL;

	if (cmd)
	{
		/* Rajoute la commande en fin de queue */
		while (cmd->next)
			cmd = cmd->next;
		cmd->next = command;
	}
	else
		/*cmd = */
		p_port[port].cmd = command;
	aff_forward ();
}

static void FAR hst_wait (int port, PortData * command)
{
	p_port[port].t_wait = NULL;
	if (p_port[port].t_busy)
	{
		m_libere (p_port[port].t_busy->userdata, sizeof (PortData));
		del_timer (p_port[port].t_busy);
	}
	p_port[port].t_busy = add_timer (10, port, (void FAR *) hst_connect, command);
	aff_forward ();
}

static int hst_sndcmd (int port, int canal, char *buffer, int retour)
{
	PortData *cmd = p_port[port].cmd;
	int nb;
	char cmde[80];
	char scan[80];
	char val[80];
	char temp[80];

	/* if (p_port[port].ccanal == 0)
	   canal = PACTOR_CH; */
	if (strlen (buffer) == 0)
		return (0);

	/* Demande de connexion du canal pactor : verifier d'abord le status */
	if (*buffer == 'C')
	{
		if (canal == PACTOR_CH)
		{
			/* Armer le timer = 10 secondes */
			cmd = (PortData *) m_alloue (sizeof (PortData));
			cmd->next = NULL;
			cmd->canal = canal;
			cmd->cmd = COMMAND;
			cmd->len = strlen (buffer);
			strcpy (cmd->buf, buffer);
			if (p_port[port].t_wait)
			{
				m_libere (p_port[port].t_wait->userdata, sizeof (PortData));
				del_timer (p_port[port].t_wait);
			}
			p_port[port].t_wait = add_timer (7, port, (void FAR *) hst_wait, cmd);
			return (1);
		}
		else if (strchr (buffer, ':') == NULL)
		{
			/* Selectionner le bon port */
			++buffer;
			while (isspace (*buffer))
				++buffer;
			sprintf (temp, "C %d:%s", p_port[port].ccanal, buffer);
			buffer = temp;
		}
	}

	*scan = '\0';
	*val = '\0';
	nb = sscanf (buffer, "%s %s %s", cmde, scan, val);

	if (cmde[0] == '!')
		cmde[0] = '#';

	if ((nb == 3) && (strcmpi (cmde, "#TRX") == 0) && (strncmpi (scan, "SCAN", strlen (scan)) == 0))
	{
		if (isdigit (*val)) 
		{
			 if (atoi (val) > 0)
			 {
				cmd = (PortData *) m_alloue (sizeof (PortData));
				cmd->next = NULL;
				cmd->canal = canal;
				cmd->cmd = COMMAND;
				cmd->len = strlen (buffer);
				strcpy (cmd->buf, buffer);
				cmd->buf[0] = '#';		/* LA7ECA Hack */
				hst_startscan (port, cmd);
				return 1;
			}
			pactor_scan[port] = 0;
		}
	}

	if (strcmp (cmde, "%O") == 0)
	{
		/* Un seul change-over en queue ! Enleve le precedent ... */
		PortData *pr = NULL;

		cmd = p_port[port].cmd;
		while (cmd)
		{
			if (strcmp (cmd->buf, "%O") == 0)
			{
				if (pr)
				{
					pr->next = cmd->next;
				}
				else
				{
					p_port[port].cmd = cmd->next;
				}

				/* Libere la structure */
				m_libere (cmd, sizeof (PortData));
				break;
			}
			pr = cmd;
			cmd = cmd->next;
		}
	}

	cmd = p_port[port].cmd;
	if (cmd)
	{
		/* Rajoute la commande en fin de queue */
		while (cmd->next)
			cmd = cmd->next;
		cmd->next = (PortData *) m_alloue (sizeof (PortData));
		cmd = cmd->next;
	}
	else
	{
		p_port[port].cmd = (PortData *) m_alloue (sizeof (PortData));
		cmd = p_port[port].cmd;
	}

	strcpy (cmd->buf, buffer);
	if (cmd->buf[0] == '!')
		cmd->buf[0] = '#';


	if ((nb == 2) && (strncmpi (cmde, "#MYPAC", 6) == 0))
	{
		sprintf(cmd->buf, "I%s", scan);
		canal = PACTOR_CH;
	}

	cmd->next = NULL;
	cmd->canal = canal;
	cmd->cmd = COMMAND;
	cmd->len = strlen (cmd->buf);

	return 1;
}

static int hst_send_dt (int port, int canal, char *buffer, int len)
{

	PortData *cmd = p_port[port].cmd;

	if (len <= 0)
		return (0);

	if (cmd)
	{
		/* Rajoute la commande en fin de queue */
		while (cmd->next)
			cmd = cmd->next;
		cmd->next = (PortData *) m_alloue (sizeof (PortData));
		cmd = cmd->next;
	}
	else
		cmd = p_port[port].cmd = (PortData *) m_alloue (sizeof (PortData));

	cmd->next = NULL;
	cmd->canal = canal;
	cmd->cmd = DATA;
	cmd->len = len > 256 ? 256 : len;
	memcpy (cmd->buf, buffer, cmd->len);
	return (1);
}

static int hst_send_ui (int port, char *buffer, int len, Beacon * beacon)
{
	char commande[300];
	char s[80];
	int i;
	int ptport;
	int canal;
	PortData *cmd;

	if (len <= 0)
		return (0);

	if (p_port[port].ccanal == 0)
	{
		/* No beacon on pactor port ! */
		return (1);
	}
	else
	{
		ptport = p_port[port].ccanal;
		canal = 0;
	}

	/* Selection du port */
	sprintf (commande, "%%P%d", ptport);

	cmd = p_port[port].cmd;
	if (cmd)
	{
		/* Rajoute la commande en fin de queue */
		while (cmd->next)
			cmd = cmd->next;
		cmd->next = (PortData *) m_alloue (sizeof (PortData));
		cmd = cmd->next;
	}
	else
		cmd = p_port[port].cmd = (PortData *) m_alloue (sizeof (PortData));

	cmd->canal = canal;
	cmd->cmd = COMMAND;
	cmd->next = NULL;
	cmd->len = strlen (commande);
	strcpy (cmd->buf, commande);

	sprintf (commande, "C %d:%s-%d", ptport, beacon->desti.call, beacon->desti.num);

	for (i = 0; i < 8; i++)
	{
		if (*beacon->digi[i].call)
		{
			if (i == 0)
			{
				strcat (commande, " VIA");
			}
			sprintf (s, " %s-%d",
					 beacon->digi[i].call, beacon->digi[i].num);
			strcat (commande, s);
		}
	}

	cmd->next = (PortData *) m_alloue (sizeof (PortData));
	cmd = cmd->next;

	cmd->next = NULL;
	cmd->canal = canal;
	cmd->cmd = COMMAND;
	cmd->len = strlen (commande);
	strcpy (cmd->buf, commande);

	cmd->next = (PortData *) m_alloue (sizeof (PortData));
	cmd = cmd->next;

	cmd->next = NULL;
	cmd->canal = canal;
	cmd->cmd = UNPROTO;
	cmd->len = len > 256 ? 256 : len;
	memcpy (cmd->buf, buffer, cmd->len);

	return (1);
}

/* static int hst_send_data (int port, int canal, int type, char *buffer, int len) */
static int hst_send_data (int port)
{
	int i;
	int c;
	int canal, type, len;
	char *buffer;
	char debug_buf[300];
	char *pdb = debug_buf;
	int dlen = 0;

	unsigned short crc = 0xffff;

	if (p_port[port].last == NULL)
		return (0);

	DebugCmd (port);

	canal = p_port[port].last->canal;
	type = (p_port[port].last->cmd == COMMAND) ? 1 : 0;
	type |= p_port[port].last->compteur;
	len = p_port[port].last->len;
	buffer = p_port[port].last->buf;

	send_tnc (port, HST_CHAR);
	*pdb++ = HST_CHAR;
	++dlen;
	send_tnc (port, HST_CHAR);
	*pdb++ = HST_CHAR;
	++dlen;

	crc = updcrc (canal, crc);
	send_tnc (port, canal);
	*pdb++ = canal;
	++dlen;

	crc = updcrc (type, crc);
	send_tnc (port, type);
	*pdb++ = type;
	++dlen;

	c = len - 1;
	crc = updcrc (c, crc);
	send_tnc (port, c);
	*pdb++ = c;
	++dlen;
	if (c == HST_CHAR)
	{
		send_tnc (port, 0);
		*pdb++ = 0;
		++dlen;
	}

	for (i = 0; i < len; i++)
	{
		c = *buffer++;
		crc = updcrc (c, crc);
		send_tnc (port, c);
		*pdb++ = c;
		++dlen;
		if (c == HST_CHAR)
		{
			send_tnc (port, 0);
			*pdb++ = 0;
			++dlen;
		}
	}

	crc = ~crc;

	/* Send crc */
	c = crc & 0xff;
	send_tnc (port, c);
	*pdb++ = c;
	++dlen;
	if (c == HST_CHAR)
	{
		send_tnc (port, 0);
		*pdb++ = 0;
		++dlen;
	}

	c = crc >> 8;
	send_tnc (port, c);
	*pdb++ = c;
	++dlen;
	if (c == HST_CHAR)
	{
		send_tnc (port, 0);
		*pdb++ = 0;
		++dlen;
	}

#ifdef DEBUG_HST
	/* if (p_port[port].last->cmd == COMMAND) */
		bin_send (debug_buf, dlen);
#endif

	p_port[port].polling = 1;
	p_com[(int)p_port[port].ccom].delai = 0;

	return (1);
}

static void resync_port (int port)
{
	long nt = btime ();
	static int first = 1;
	static long delai[NBPORT];

	if ((port < 0) || (port >= NBPORT))
		return;

	if (first)
	{
		/* Initializes static variables */
		int i;

		for (i = 0; i < NBPORT; i++)
		{
			delai[i] = 0L;
		}
		first = 0;
	}


	if ((p_port[port].synchro < 256) && (delai[port] < nt))
	{
		++p_port[port].synchro;

		/* Retry du dernier paquet */
		hst_send_data (port);
		delai[port] = nt + 20L;
#if defined(__WINDOWS__) || defined(__LINUX__)
		DisplayResync (port, p_port[port].synchro);
#else
		{
			char s[40];

			sprintf (s, "Port %d Resync %-4d", port, p_port[port].synchro);
			aff_chaine (W_DEFL, 1, 1, s);
		}
#endif
	}

	if (p_port[port].synchro == 256)
	{
		char txt[80];

		sprintf (txt, "Port %d was not resynchronized. Stopped. Error =", port);
		warning (4, txt);
		++p_port[port].synchro;
	}
}

static int crc_check (char *ptr, int len)
{
	int i;
	unsigned short crc = 0xffff;

	for (i = 0; i < len; i++)
	{
		crc = updcrc (*ptr, crc);
		++ptr;
	}

	return (crc == 0xf0b8);
}

static int recv_hst (int port, int canal, int *lgbuf, char *buffer)
{
	int crc_ok;
	int r_canal, code = 0, nb, lg;
	int i, pos, c, hst;
	int last = -1;
	char buf[300];
	char *debbuf = buffer;

	/* Implementation polling */
	int valid = 0;
	char *ptr = p_port[port].portbuf;
	int index = p_port[port].portind;

	while ((c = rec_tnc (port)) >= 0)
	{
		if (index == 300)
		{
			/* Erreur : Vider le buffer et resynchroniser ? */
			vide (port, 0);
			index = 0;
			break;
		}
		/* Ajoute les caracteres dans le buffer */
		ptr[index++] = c;
	}
	p_port[port].portind = index;

	/* Tester si la trame est complete */
	if (index >= 6)
	{
		/* La trame doit faire au moins 6 octets : header + canal et code + crc16 */

		/* Verifier que les deux premiers octets sont HST_CHAR */
		if ((ptr[0] != HST_CHAR) || (ptr[1] != HST_CHAR))
		{
			/* Le paquet n'est pas bon. Jeter le 1er octet */
			for (i = 1; i < index; i++)
			{
				ptr[i - 1] = ptr[i];
			}
			--p_port[port].portind;
			return (-1);		/* Modifie de 0 a -1 le 05/04/98 */
		}

		/* Sauter le header - supprimer les 0x00 apres les HST_CHAR et recopie dans buf */
		for (i = 2, pos = 0, hst = 0; i < index; i++)
		{
			if (hst)
			{
				if (ptr[i] == 0)
					hst = 0;
				else
				{
					p_port[port].portind = 0;

					/* Demande la repetition du paquet */
					hst_send_data (port);
					return (-1);
				}
			}
			else
				buf[pos++] = ptr[i];

			if (ptr[i] == HST_CHAR)
			{
				hst = 1;
			}
		}

		/* Verifie que les caracteres HST_CHAR sont bien suivis de leur 0 */
		pos -= hst;

		/* Nouvelle longueur du paquet */
		ptr = buf;
		index = pos;

		crc_ok = crc_check (ptr, index);

		/* On tente le decodage ... */
		index -= 2;				/* enlever le crc */
		r_canal = *ptr++;

		code = *ptr++;
		index -= 2;

		*lgbuf = 0;

		if (canal == 0xff)
			canal = r_canal;

		lg = 0;
		switch (code)
		{
		case 0:
			if (pos >= 4)
			{
				if (!crc_ok)
				{
					/* La trame est erronnee */

					/* Jette la trame recue */
					p_port[port].portind = 0;

					/* Demande la repetition du paquet */
					hst_send_data (port);
					return (-1);
				}
#ifdef DEBUG_HST
				if (r_canal != PACTOR_ST)
				{
					char buf_debug[300];

					buf_debug[0] = 0xaa;
					buf_debug[1] = 0xaa;
					memcpy (buf_debug + 2, buf, pos);
					bin_recv (buf_debug, pos + 2);
				}
#endif
				*buffer = '\0';
				valid = 1;
			}
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			while (index)
			{
				last = *ptr++;
				*buffer++ = last;
				++lg;
				--index;
			}

			/* Le caractere doit etre un NULL pour finir le paquet */
			if (last == '\0')
			{
				if (!crc_ok)
				{
					/* La trame est erronnee */

					/* Jette la trame recue */
					p_port[port].portind = 0;

					/* Demande la repetition du paquet */
					hst_send_data (port);
					return (-1);
				}
#ifdef DEBUG_HST
				if (r_canal != PACTOR_ST)
				{
					char buf_debug[300];

					buf_debug[0] = 0xaa;
					buf_debug[1] = 0xaa;
					memcpy (buf_debug + 2, buf, pos);
					bin_recv (buf_debug, pos + 2);
				}
#endif
				valid = 1;
				*buffer = '\n';
				*++buffer = '\0';
			}
			break;
		case 6:
		case 7:
			nb = lg = 1 + (0xff & *ptr++);
			--index;

			if (nb == index)
			{
				if (!crc_ok)
				{
					/* La trame est erronnee */

					/* Jette la trame recue */
					p_port[port].portind = 0;

					/* Demande la repetition du paquet */
					hst_send_data (port);
					return (-1);
				}
				/* Tous les caracteres sont recus */
#ifdef DEBUG_HST
				if (r_canal != PACTOR_ST)
				{
					char buf_debug[300];

					buf_debug[0] = 0xaa;
					buf_debug[1] = 0xaa;
					memcpy (buf_debug + 2, buf, pos);
					bin_recv (buf_debug, pos + 2);
				}
#endif
				while (nb--)
				{
					*buffer++ = *ptr++;
				}
				valid = 1;
			}
			break;
		default:
			if ((!svoie[CONSOLE]->sta.connect) && (p_port[port].synchro == 0))

			{
				char txt[80];

				if (!crc_ok)
				{
					/* La trame est erronnee */

					/* Jette la trame recue */
					p_port[port].portind = 0;

					/* Demande la repetition du paquet */
					hst_send_data (port);
					return (-1);
				}
				sprintf (txt, "Erreur code trame = %d. Erreur =", code);
				warning (2, txt);
			}
			if (p_port[port].synchro == 0)
			{
				vide (port, 0);
				p_port[port].portind = 0;
			}
			else if ((r_canal == '^') && (code == 'A'))
			{
				/* TNC reinitialise en mode normal ??? */
				valid = 1;
#ifdef __WINDOWS__
				if (operationnel)
				{
					fbb_quit (0);
					*lgbuf = 0;
					return (-1);
				}
#endif
			}
			break;
		}
		*lgbuf = lg;
	}

	if (valid)
	{
		/* La trame a ete acceptee... On libere la donnee */
		m_libere (p_port[port].last, sizeof (PortData));
		p_port[port].last = NULL;

		if ((DebugCh) || (code == 2))
		{
			int len = *lgbuf;
			char buf[300];

			strcpy (buf, debbuf);
			len = *lgbuf;

			if (code == 1)
			{
				--len;
				buf[len] = '\0';
			}

		}

		/* Une trame complete a ete recue */
		if (p_port[port].synchro)
		{
#if defined(__WINDOWS__) || defined(__LINUX__)
			char txt[80];

			sprintf (txt, "Ending resynchronization on port %d - Error =", port);
			warning (5, txt);

			DisplayResync (port, 0);
#else
			++com_error;
			aff_date ();
#endif
		}
		p_port[port].portind = 0;
		p_port[port].synchro = 0;
		p_com[(int)p_port[port].ccom].delai = 0;
	}
	else
	{
		code = -1;

		/* Rien n'a ete recu... Tester le temps (2 secondes d'attente) */
		if (p_com[(int)p_port[port].ccom].delai > 10)
		{
			/* Lancer la procedure de resynchronisation */
			if (p_port[port].synchro == 0)
			{
				char txt[80];

				sprintf (txt, "Starting resynchronization on port %d - Error =", port);
				warning (3, txt);
				/* vide (port, 0); */
			}
			p_port[port].portind = 0;
			resync_port (port);
		}
	}

	return (code);
}

static void hst_get_ui (int port, char *buffer, ui_header * ui)
{
	char *ptr;
	char *scan;

	memset (ui, 0, sizeof (ui_header));

	ui->port = port;

	scan = buffer;

	if ((isdigit (scan[0])) && (scan[1] == ':'))
	{
		/* port HST */
		ui->port = hst_port (port, scan[0] - '0');
		scan += 2;
		if (*scan == ' ')
		{
			/* bug TFPCX ... */
			++scan;
		}
	}

	ptr = strtok (scan, " ");	/* fm */

	if (ptr == NULL)
		return;

	ptr = strtok (NULL, " ");	/* exped */

	if (ptr == NULL)
		return;
	n_cpy (11, ui->from, ptr);

	ptr = strtok (NULL, " ");	/* to */

	if (ptr == NULL)
		return;

	ptr = strtok (NULL, " ");	/* desti */

	if (ptr == NULL)
		return;
	n_cpy (11, ui->to, ptr);

	ptr = strtok (NULL, " ");	/* via ou ctl */

	if (ptr == NULL)
		return;

	if (strcmp (ptr, "via") == 0)
	{
		for (;;)
		{
			if (*ui->via)
				strcat (ui->via, " ");

			ptr = strtok (NULL, " ");	/* digis */

			if (ptr == NULL)
				return;

			if (strcmp (ptr, "ctl") == 0)
				break;

			strncat (ui->via, ptr, 12);
		}
	}

	ptr = strtok (NULL, " ");	/* controle */

	if (ptr == NULL)
		return;
	strn_cpy (11, ui->ctl, ptr);
	ui->ui = (strncmp (ptr, "UI", 2) == 0);

	ptr = strtok (NULL, " ");	/* pid */

	if (ptr == NULL)
		return;

	ptr = strtok (NULL, " ");	/* pid */

	if (ptr == NULL)
		return;
	sscanf (ptr, "%x", &ui->pid);
}

static int nb_waiting (int port)
{
	/* Donne le nombre de trames DATA en attente d'envoi vers le TNC */
	int nb = 0;
	PortData *cmd = p_port[port].cmd;

	while (cmd)
	{
		if (cmd->cmd == DATA)
			++nb;
		cmd = cmd->next;
	}

	return (nb);
}
