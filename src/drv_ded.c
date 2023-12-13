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
#include <fbb_drv.h>

static int ded_end_host (int);

static int ded_send_data (int, int, char *, int);
static int ded_send_dt (int, int, char *, int);
static int ded_send_ui (int, char *, int, Beacon *);
static int ded_sndcmd (int, int, char *, int);
static int nb_waiting (int);
static int recv_ded (int, int, int *, char *);

static void ded_get_ui (int, char *, ui_header *);
static void ded_sonde (int, int);
static void send_ded (int, int, char, char *);

/*
 * Fonctions g‚n‚riques du driver
 */

int rcv_ded (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui)
{
	int lgbuf=0;
	int can = p_port[*port].cur_can & 0xff;
	int code;
	int valid;
	int next = 1;

	/* Header provisoire */
	static ui_header loc_ui[NBPORT];

	*cmd = INVCMD;

	/* Test si c'est le port correspondant au MUX ... */
	if ((operationnel) && (*port != p_com[(int)p_port[*port].ccom].mult_sel))
		return (-1);

	if (p_port[*port].polling == 0)
	{
		ded_sonde (*port, next);
		return (0);
	}

	valid = 0;

	usleep(10000);		/* wait 10 msec to allow interrupt and avoid CPU overload */

	deb_io ();
	code = recv_ded (*port, can, &lgbuf, buffer);
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
		if (p_port[*port].wait[can])
			--p_port[*port].wait[can];
		break;
	case 4:
		ded_get_ui (*port, buffer, ui);
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
		ded_get_ui (*port, buffer, ui);
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
		valid = 1;
		*cmd = DATA;
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
		ded_sonde (*port, next);
	}

	return (valid);
}

int opn_ded (int port, int nb)
{
	long bt;
	int reset = 4;
	int nb_can;
	int i;
	int ok;
	char s[80];

	selcanal (port);

	while (reset)
	{
		ok = 0;
		sprintf (s, "Init PORT %d COM%d-%d",
				 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__linux__)
		InitText (s);
#else
		cprintf ("%s\r\n", s);
#endif
		if (p_port[port].moport & 0x80)
		{
			sprintf (s, "Init PORT %d COM%d-%d PTCII",
					 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__linux__)
			InitText (s);
#else
			cprintf ("%s\r\n", s);
#endif
			tncstr (port, "\r", 0);
			sleep_ (10);
			tncstr (port, "PTC 31\r", 0);
		}
		else
		{
			tncstr (port, "\030\033JHOST\r\033MN\r", 0);
		}

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

#ifdef ENGLISH
		cprintf ("Reset sent ... Please wait.     \r\n");
#else
		cprintf ("Reset envoy‚... Patientez S.V.P.\r\n");
#endif
		sprintf (s, "Resync PORT %d COM%d-%d",
				 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__linux__)
		InitText (s);
#else
		cprintf ("%s\r\n", s);
#endif
		ded_end_host (port);
		--reset;
	}
	sprintf (s, "Clear PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__linux__)
	InitText (s);
#else
	cprintf ("%s\r\n", s);
#endif
	tncstr (port, "\033JHOST1\r", 0);

	sprintf (s, "Prog PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
	vide (port, 1);
#if defined(__WINDOWS__) || defined(__linux__)
	InitText (s);
#else
	cprintf ("%s\r\n", s);
#endif
	/* Parametres par defaut */
	sprintf (s, "I %s-%d", mycall, myssid);
	ded_sndcmd (port, 0, (char *) s, 1);
	ded_sndcmd (port, 0, (char *) s, 1);
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
	ded_sndcmd (port, 0, (char *) s, 1);
	if (!DRSI (port))
	{
		sprintf (s, "O %d", p_port[port].frame);
		ded_sndcmd (port, 0, (char *) s, 1);
	}
	return (ok);
}

int cls_ded (int port)
{
	/* Vide la file des commandes en attente */
	selcanal (port);
	while (p_port[port].cmd)
	{
		PortData *cmd = p_port[port].cmd;

		p_port[port].cur_can = cmd->canal;
		switch (cmd->cmd)
		{
		case COMMAND:
			send_ded (port, (int) cmd->canal, 1, cmd->buf);
			break;
		default:
			break;
		}
		p_port[port].cmd = cmd->next;
		m_libere (cmd, sizeof (PortData));
		p_port[port].last_cmde = '\0';
	}

	/* Fin du mode host */
	ded_end_host (port);
	return (1);
}

int sta_ded (int port, int canal, int cmd, void *ptr)
{
	switch (cmd)
	{
	case TOR:
		return (ded_sndcmd (port, canal, (char *) ptr, 0) != -1);
	case SNDCMD:
		return (ded_sndcmd (port, canal, (char *) ptr, 0) != -1);
	case ECHOCMD:
		return (ded_sndcmd (port, canal, (char *) ptr, 1) != -1);
	case PORTCMD:
		return (ded_sndcmd (port, 0, (char *) ptr, 1) != -1);
	case PACLEN:
		*((int *) ptr) = 250;
		return (1);
	}
	return (0);
}

int snd_ded (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	int ret = 0;

	df ("snd_ded", 8);

	if (p_port[port].synchro > 256)
		return (0);

	switch (cmd)
	{
	case COMMAND:
		break;

	case DATA:
		ret = ded_send_dt (port, canal, buffer, len);
		break;

	case UNPROTO:
		ret = ded_send_ui (port, buffer, len, ptr);
		break;
	}
	ff ();
	return (ret);
}

/* Fonctions locales */

static int ded_end_host (int port)
{
	static char *mn = "MN";
	static char *jhost0 = "JHOST0";
	char *ptr;
	int nb;
	long bt;

	int retry = 4;

	while (retry)
	{
		/* Envoie le MN (pas de monitoring) */
		send_tnc (port, 0);
		send_tnc (port, 1);
		send_tnc (port, strlen (mn) - 1);
		ptr = mn;
		while (*ptr)
			send_tnc (port, *ptr++);

		bt = btime () + 20;

		/* Verifie la reception de la reponse */
		do
		{
			if (rec_tnc (port) >= 0)
			{
				/* Envoie le JHOST0 */
				send_tnc (port, 0);
				send_tnc (port, 1);
				send_tnc (port, strlen (jhost0) - 1);
				ptr = jhost0;
				while (*ptr)
					send_tnc (port, *ptr++);
				return (1);
			}
		}
		while (btime () < bt);

		/* Rien recu, on passe en resynchro */
		for (nb = 0; nb < 256; nb++)
		{
			send_tnc (port, 1);
			bt = btime () + 5;
#if defined(__WINDOWS__) || defined(__linux__)
			DisplayResync (port, nb + 1);
#endif
			while (btime () < bt);
			if (rec_tnc (port) >= 0)
			{
#if defined(__WINDOWS__) || defined(__linux__)
				DisplayResync (port, 0);
#else
				++com_error;
				aff_date ();
#endif
				break;
			}
		}

		/* La resynchro n'a rien donne ... On jette l'eponge ! */
		if (nb == 256)
			return (0);

		vide (port, 0);

		--retry;
	}
	return (0);
}

static void warning (unsigned numero, char *texte)
{
}

static void ded_sonde (int pp, int next)
{
	static int cptr[NBPORT];
	static int first = 1;

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

	if (p_port[pp].cur_can == 0xff)
		p_port[pp].cur_can = 0;

	if (p_port[pp].polling)
	{
		return;
	}

	if (next == 0)
	{
		send_ded (pp, p_port[pp].cur_can, 1, "G");
		p_port[pp].last_cmde = 'G';
	}
	else if (p_port[pp].cmd)
	{
		PortData *cmd = p_port[pp].cmd;

		p_port[pp].cur_can = cmd->canal;
		switch (cmd->cmd)
		{
		case COMMAND:
			send_ded (pp, (int) cmd->canal, 1, cmd->buf);
			break;
		case DATA:
		case UNPROTO:
			ded_send_data (pp, cmd->canal, cmd->buf, cmd->len);
			cptr[pp] = 0;
			break;
		default:
			break;
		}
		p_port[pp].cmd = cmd->next;
		m_libere (cmd, sizeof (PortData));
		p_port[pp].last_cmde = '\0';
	}
	else if (operationnel == 1)
	{
		if (cptr[pp] == 0)
		{
			send_ded (pp, p_port[pp].cur_can, 1, "@B");
			p_port[pp].last_cmde = 'B';
			cptr[pp] = 50;
		}
		else
		{
			/* Passe au canal suivant */
			if (p_port[pp].idem == 0)
			{
				++(p_port[pp].cur_can);
				if (p_port[pp].cur_can > p_port[pp].tt_can)
				{
					int mux_ch;
					int i;
					int com;
					int first_mux;
					int last_mux;

					/* Passe eventuellement au PORT/MUX suivant */
					mux_ch = p_port[pp].ccanal;
					com = p_port[pp].ccom;

					first_mux = (DRSI (pp)) ? 0 : 1;
					last_mux = (DRSI (pp)) ? 7 : 4;

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

					if (DRSI (pp))
					{
						/* Fait la somme des canaux deja balayes (1 seul TNC !!) */
						for (i = 1; i < pp; i++)
						{
							if (DRSI (i))
								p_port[pp].cur_can += p_port[i].nb_voies;
						}
					}

					selcanal (pp);
				}
			}
			else
				p_port[pp].idem = 0;
			send_ded (pp, p_port[pp].cur_can, 1, "L");
			p_port[pp].last_cmde = 'L';
			if (cptr[pp])
				--cptr[pp];
		}
	}
	p_port[pp].polling = 1;
	p_com[(int)p_port[pp].ccom].delai = 0;
}


/*
   int rcv_ded(int *port, int *canal, int *cmd, char *buffer, int *len, ui_header *ptr)
   {
   int val;

   df("snd_ded", 12);

   val = ded_inbuf(port, canal, cmd, buffer, len, ptr);

   ff();
   return(val);
   }
 */


/*
 * Fonctions locales
 */

static int ded_sndcmd (int port, int canal, char *buffer, int retour)
{

	PortData *cmd = p_port[port].cmd;

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
	cmd->cmd = COMMAND;
	strcpy (cmd->buf, buffer);
	return (1);
}

static int ded_send_dt (int port, int canal, char *buffer, int len)
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

static int ded_send_ui (int port, char *buffer, int len, Beacon * beacon)
{
	char commande[300];
	char s[80];
	int i;
	int via = 1;
	PortData *cmd = p_port[port].cmd;

	if (DRSI (port))
	{
		sprintf (commande, "C %d:%s-%d", p_port[port].ccanal,
				 beacon->desti.call, beacon->desti.num);
	}
	else
	{
		sprintf (commande, "C %s-%d",
				 beacon->desti.call, beacon->desti.num);
	}
	for (i = 0; i < 8; i++)
	{
		if (*beacon->digi[i].call)
		{
			if (via)
			{
				strcat (commande, " VIA ");
				via = 0;
			}
			else
			{
				strcat (commande, ",");
			}
			sprintf (s, "%s-%d",
					 beacon->digi[i].call, beacon->digi[i].num);
			strcat (commande, s);
		}
	}

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

	cmd->canal = 0;
	cmd->cmd = COMMAND;
	cmd->len = 0;
	strcpy (cmd->buf, commande);

	cmd->next = (PortData *) m_alloue (sizeof (PortData));

	cmd = cmd->next;
	cmd->next = NULL;
	cmd->canal = 0;
	cmd->cmd = UNPROTO;
	cmd->len = len > 256 ? 256 : len;
	memcpy (cmd->buf, buffer, cmd->len);

	return (1);
}

static int ded_send_data (int port, int canal, char *buffer, int len)
{
	int i;

	if (p_port[port].synchro > 256)
		return (1);

	selcanal (port);
	send_tnc (port, canal);
	send_tnc (port, 0);
	send_tnc (port, len - 1);
	for (i = 0; i < len; i++)
		send_tnc (port, *buffer++);

	return (1);
}

static void send_ded (int port, int canal, char type, char *chaine)
{
	if (p_port[port].synchro > 256)
		return;

	if (strcmp (chaine, "%O") == 0)
		sleep_ (1);

	/* type = 0 : texte. type = 1 : commande */

	selcanal (port);
	send_tnc (port, canal);
	send_tnc (port, type);
	send_tnc (port, strlen (chaine) - 1);
	while (*chaine)
		send_tnc (port, *chaine++);
	ff ();
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
		p_port[port].cur_can = 1;
		send_tnc (port, '\001');
		delai[port] = nt + 4L;
#if defined(__WINDOWS__) || defined(__linux__)
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

static int recv_ded (int port, int canal, int *lgbuf, char *buffer)
{
	int r_canal, code = 0, nb, lg;
	int c;

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
	if (index >= 2)
	{
		/* La trame doit faire au moins 2 octets : canal et code */
		/* On tente le decodage ... */
		r_canal = *ptr++;
		code = *ptr++;
		index -= 2;

		*lgbuf = 0;

		if (canal == 0xff)
			canal = r_canal;

		if ((r_canal != canal) && (p_port[port].synchro == 0))
		{
			if ((!svoie[CONSOLE]->sta.connect) && (p_port[port].synchro == 0))
			{
				char txt[80];

#ifdef ENGLISH
				sprintf (txt, "Receive : Error channel number: sent %d - received %d -  Error =", canal, r_canal);
#else
				sprintf (txt, "Recoit : Erreur Numero de canal: envoye %d - recu %d  - Erreur =", canal, r_canal);
#endif
				warning (1, txt);
			}
			if (nb_error++ == 10)
				fbb_error (ERR_TNC, "DED RCV DATA", port);
			vide (port, 0);
			return (-1);
		}

		lg = 0;
		switch (code)
		{
		case 0:
			*buffer = '\0';
			valid = 1;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:

			while (index)
			{
				*buffer = *ptr++;
				++lg;
				--index;
				/* Le caractere doit etre un NULL pour finir le paquet */
				if ((index == 0) && (*buffer == '\0'))
				{
					valid = 1;
					*buffer = '\n';
					*++buffer = '\0';
				}
				++buffer;
			}
			break;
		case 6:
		case 7:
			nb = lg = 1 + (0xff & *ptr++);
			--index;

			if (nb == index)
			{
				/* Tous les caracteres sont recus */
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
		/* Une trame complete a ete recue */
		if (p_port[port].synchro)
		{
#if defined(__WINDOWS__) || defined(__linux__)
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
		code = -1;

	if (!valid)
	{
		/* Rien n'a ete recu... Tester le temps (15 secondes d'attente) */
		if (p_com[(int)p_port[port].ccom].delai > 15)
		{
			/* Lancer la procedure de resynchronisation */
			if (p_port[port].synchro == 0)
			{
				char txt[80];

				sprintf (txt, "Starting resynchronization on port %d - Error =", port);
				warning (3, txt);
				vide (port, 0);
			}
			resync_port (port);
		}
	}

	return (code);
}

static void ded_get_ui (int port, char *buffer, ui_header * ui)
{
	char *ptr;
	char *scan;

	memset (ui, 0, sizeof (ui_header));

	ui->port = port;

	scan = buffer;

	if ((isdigit (scan[0])) && (scan[1] == ':'))
	{
		/* port DRSI */
		ui->port = drsi_port (port, scan[0] - '0');
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
