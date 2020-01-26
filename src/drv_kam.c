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

typedef struct
{
	char port;
	char stream;
}
KamTries;

typedef struct
{
	char canal;
	char *cmd;
}
KamCmd;

KamTries kam_try[NBPORT];
KamCmd kam_cmd[NBPORT];


static int lit_stat_kam (int port, int canal, stat_ch * ptr);
static int kam_send (int port, int canal, int type, char *chaine, int len);
static int kam_tor (int port, char *chaine, int len);
static int kam_send_dt (int port, int canal, char *buffer, int len);
static int kam_send_ui (int port, char *buffer, int len, Beacon * beacon);
static int kam_inbuf (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui);

/*
 * Fonctions g‚n‚riques du driver
 */

int sta_kam (int port, int canal, int cmd, void *ptr)
{
	char command[256];
	char *cptr = (char *) ptr;

	if ((kam_cmd[port].cmd) && (p_port[port].stop == 0))
	{
		int len = strlen (kam_cmd[port].cmd);

		kam_send (port, kam_cmd[port].canal, 1, kam_cmd[port].cmd, len);
		m_libere (kam_cmd[port].cmd, len + 1);
		kam_cmd[port].cmd = NULL;
	}

	switch (cmd)
	{
	case TNCSTAT:
		return (lit_stat_kam (port, canal, (stat_ch *) ptr));
	case SNDCMD:
		/* Teste la commande de connexion delayee */
		if ((*cptr == 'C') && (*(cptr + 1) == ' ') && (p_port[port].stop))
		{
			kam_cmd[port].cmd = m_alloue (strlen (cptr) + 1);
			strcpy (kam_cmd[port].cmd, cptr);
			kam_cmd[port].canal = canal;
			return (1);
		}
		return (kam_send (port, canal, 1, cptr, strlen (ptr)));
	case ECHOCMD:
		return (1);
	case PORTCMD:
		strcpy (command, (char *) ptr);
		return (kam_send (port, 0, 1, command, strlen (command)));
	}
	return (0);
}

int snd_kam (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	int ret = 0;

	switch (cmd)
	{
	case COMMAND:
		ret = kam_send (port, 0, 1, buffer, len);
		break;

	case TOR:
		ret = kam_tor (port, buffer, len);
		break;

	case DATA:
		ret = kam_send_dt (port, canal, buffer, len);
		break;

	case UNPROTO:
		ret = kam_send_ui (port, buffer, len, ptr);
		break;
	}
	return (ret);
}

int rcv_kam (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ptr)
{
	int val;

	val = kam_inbuf (port, canal, cmd, buffer, len, ptr);

	return (val);
}


/*************** Driver KAM ************/

static int kam_tor (int port, char *chaine, int len)
{
	int c;

	df ("kam_tor", 6);

	while (car_tx (port))
		;						/* attend que le buffer d'emission soit vide */

	send_tnc (port, 0xc0);

	while (len--)
	{
		c = *chaine++;
		if (c == 0xc0)
		{
			send_tnc (port, 0xdb);
			c = 0xdc;
		}
		else if (c == 0xdb)
		{
			send_tnc (port, 0xdb);
			c = 0xdd;
		}
		send_tnc (port, c);
	}
	send_tnc (port, 0xc0);

	ff ();

	return (1);
}

static int kam_read (int port, char *buffer)
{
	int c, nb;
	char *ptr = buffer;


	df ("kam_read", 3);

	nb = 0;

	if (!car_tnc (port))
	{
		ff ();
		return (0);
	}

	if (rec_tnc (port) != 0xc0)
	{
		++com_error;
		ff ();
		return (0);
	}

	for (;;)
	{
		c = rec_tnc (port);
		if (c == -1)
		{
			continue;
		}
		if (c == 0xdb)
		{
			while ((c = rec_tnc (port)) == -1);
			switch (c)
			{
			case 0xdc:
				*ptr++ = 0xc0;
				break;
			case 0xdd:
				*ptr++ = 0xdb;
				break;
			}
			++nb;
		}
		else if (c == 0xc0)
		{
			if (nb)
				break;
			else
				nb = -1;
		}
		else if (nb < 300)
		{
			*ptr++ = c;
			++nb;
		}
	}

	ff ();
	return (nb);
}

static int kam_send (int port, int canal, int type, char *chaine, int len)
{
	int c;
	int c_port = 0;
	int c_voie = 0;
	int c_mode = 0;

	int tor = 0;

	df ("kam_send", 4);

	while (car_tx (port))
		;						/* attend que le buffer d'emission soit vide */

	/* Envoi dans stream 0 en TOR */
	if ((tor) && (type == 2))
		type = 0;

	switch (type)
	{
	case 0:					/* Unproto */
		c_mode = 'D';
		c_port = p_port[port].ccanal + '0';
		c_voie = '0';
		break;

	case 1:					/* Commande */
		c_mode = 'C';

		c_port = p_port[port].ccanal + '0';
		if (canal == 0)
			c_voie = '0';
		else
			c_voie = 'A' + canal - 1;
		break;

	case 2:					/* Data */
		c_mode = 'D';
		c_port = p_port[port].ccanal + '0';
		c_voie = 'A' + canal - 1;
		break;
	}

	send_tnc (port, 0xc0);
	send_tnc (port, c_mode);
	send_tnc (port, c_port);
	send_tnc (port, c_voie);

	while (len--)
	{
		c = *chaine++;
		if (c == 0xc0)
		{
			send_tnc (port, 0xdb);
			c = 0xdc;
		}
		else if (c == 0xdb)
		{
			send_tnc (port, 0xdb);
			c = 0xdd;
		}
		send_tnc (port, c);
	}
	send_tnc (port, 0xc0);

	ff ();

	return (1);
}

static int kam_send_dt (int port, int canal, char *buffer, int len)
{
	int retour = 1;

	if (len <= 0)
		return (0);

	df ("kam_send_dt", 5);

	retour = kam_send (port, canal, 2, buffer, len);

	ff ();
	return (retour);
}


static int kam_send_ui (int port, char *buffer, int len, Beacon * beacon)
{
	char commande[300];
	char s[80];
	int i;
	int via = 1;

	df ("kam_send_ui", 5);

	sprintf (commande, "U %s-%d", mot (beacon->desti.call), beacon->desti.num);

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
					 mot (beacon->digi[i].call), beacon->digi[i].num);
			strcat (commande, s);
		}
	}

	kam_send (port, 0, 1, commande, strlen (commande));
	kam_send (port, 0, 0, buffer, len);

	ff ();
	return (1);
}

static void lit_kam_tries (int port, int canal, char *ptr, stat_ch * sta)
{
	int ret;

	df ("lit_kam_tries", 2);

	sscanf (ptr, "%*s %d", &ret);

	sta->ret = ret;
	sta->stat = -1;
	sta->connect = -1;
	sta->ack = -1;
	sta->mem = -1;

	ff ();

	return;
}

static void lit_kam_status (int port, char *ptr, stat_ch * sta)
{
#define NB_KAM_STAT	13

	static struct
	{
		int lg;
		char *txt;
	}
	kam_stat[NB_KAM_STAT] =

	{
		{
			9, "DISCONNEC"
		}
		,
		{
			9, "CONNECT i"
		}
		,
		{
			9, "FRMR in p"
		}
		,
		{
			9, "DISC in p"
		}
		,
		{
			9, "CONNECTED"
		}
		,
		{
			1, "x"
		}
		,
		{
			9, "Waiting a"
		}
		,
		{
			9, "Device bu"
		}
		,
		{
			9, "Remote de"
		}
		,
		{
			9, "Both devi"
		}
		,
		{
			17, "Waiting ACK and d"
		}
		,
		{
			17, "Waiting ACK and r"
		}
		,
		{
			17, "Waiting ACK and b"
		}
		,
	};

	int i;
	int mod;
	int voie;
	int stat = 0;
	int ack = 0;
	int kam_port = 0;
	int stream = 0;
	int st_kam[MAXVOIES];
	char *scan;

	df ("lit_kam_status", 3);

	sta->ret = -1;
	sta->stat = -1;
	sta->connect = -1;
	sta->ack = -1;
	sta->mem = -1;

	for (i = 0; i < MAXVOIES; i++)
	{
		if ((i < NBVOIES) && (svoie[i]->affport.port == port))
			st_kam[i] = 0;
		else
			st_kam[i] = -1;
	}

	scan = strtok (ptr, "\r");
	if (scan == NULL)
	{
		ff ();
		return;
	}

	sscanf (scan, "%*s %*s %d", &p_port[port].mem);

	while ((scan = strtok (NULL, "\r")) != NULL)
	{

		if (scan[1] != '/')
			continue;

		stream = scan[0] - '@';
		kam_port = (scan[2] == 'V') ? 1 : 2;

		if (kam_port == 0)
			continue;

		mod = 0;

		if (p_port[port].ccanal != kam_port)
		{
			/* Recherche l'autre port du TNC */
			for (i = 1; i < NBPORT; i++)
			{
				if (i == port)
					continue;
				if (p_port[i].ccom == p_port[port].ccom)
				{
					port = i;
					break;
				}
			}
		}
		voie = no_voie (port, stream);
		if (voie == -1)
			continue;


		scan += 4;
		if (*scan == '#')
		{
			while ((*scan) && (*scan != '('))
				++scan;
			++scan;
			ack = 0;
			while (isdigit (*scan))
			{
				ack *= 10;
				ack += (*scan - '0');
				++scan;
			}
			scan += 2;
		}
		else
		{
			ack = 0;
		}

		stat = 20;
		for (i = 0; i < NB_KAM_STAT; i++)
		{
			if (strncmp (kam_stat[i].txt, scan, kam_stat[i].lg) == 0)
			{
				stat = i;
				st_kam[voie] = i;
			}
		}

		if ((P_TOR (voie)) && (p_port[no_port (voie)].transmit))
			stat = 18;

		if (stat == 0)
		{
			ack = 0;
			sta->ret = 0;
		}

		if (sta->ack != ack)
		{
			sta->ack = ack;
			mod = 1;
		}

		if (sta->stat != stat)
		{
			sta->stat = stat;
			mod = 1;
		}

		if (mod)
			status (voie);

	}

	for (voie = 0; voie < MAXVOIES; voie++)
	{
		if (st_kam[voie] == 0)
		{
			mod = 0;

			if (sta->ack != 0)
			{
				sta->ack = 0;
				mod = 1;
			}

			if (sta->ret != 0)
			{
				sta->ret = 0;
				mod = 1;
			}

			if (sta->stat != 0)
			{
				sta->stat = 0;
				mod = 1;
			}

			if (mod)
				status (voie);
		}
	}

	ff ();

	return;
}

static int lit_stat_kam (int port, int canal, stat_ch * ptr)
{
	static long last_call = 0L;
	long temps;

	if (ptr)
	{
		temps = time (NULL);
		if (temps == last_call)
			return (0);

		last_call = temps;
	}

	df ("lit_stat_kam", 1);

	if (kam_try[(int)p_port[port].ccom].port == 0)
	{
		kam_try[(int)p_port[port].ccom].port = (char) port;
		kam_try[(int)p_port[port].ccom].stream = (char) canal;
		kam_send (port, 0, 1, "STATUS", 6);
		kam_send (port, canal, 1, "TRIES", 5);
	}

	ff ();
	return (0);
}

static void kam_get_ui (int port, char *buffer, ui_header * ui)
{
	char *ptr;
	char *sptr;

	memset (ui, 0, sizeof (ui_header));

	ui->port = port;

	ptr = strtok (buffer, ">");	/* exped */

	if (ptr == NULL)
		return;
	n_cpy (11, ui->from, ptr);

	ptr = strtok (NULL, ":,");	/* desti */

	if (ptr == NULL)
		return;
	n_cpy (11, ui->to, ptr);

	ptr = strtok (NULL, ":,");	/* digis */

	if (ptr == NULL)
		return;

	if (*ptr != ' ')
	{
		for (;;)
		{
			strncat (ui->via, ptr, 12);
			strcat (ui->via, " ");

			ptr = strtok (NULL, ":,");	/* digis */

			if (ptr == NULL)
				return;

			if ((*ptr == '\0') || (*ptr == ' '))
				break;

		}
	}

	++ptr;
	sptr = ptr;

	while ((*sptr) && (*sptr != '>'))
		++sptr;
	*sptr = '\0';

	/* controle */
	*ui->ctl = '\0';

	if (ptr[0] == '<')
	{
		int pos = 0;
		int version = 1;
		int reponse = 0;

		++ptr;
		if (ptr[0] == '<')
		{
			version = 2;
			/* AX25 Version 2 */
			++ptr;
		}

		sptr = ptr;
		if (*sptr == 'F')
		{
			pos = 4;
		}
		else if (*sptr == 'U')
		{
			pos = 2;
			if (sptr[1] == 'A')
				reponse = 1;
		}
		else if (*sptr == 'C')
		{
			strcpy (ptr, "SABM");
			pos = 4;
		}
		else if (*sptr == 'D')
		{
			strcpy (ptr, "DISC");
			pos = 4;
		}
		else if (*sptr == 'I')
		{
			pos = 3;
		}
		else
		{
			if (*sptr == 'r')
			{
				strupr (sptr);
				reponse = 1;
			}
			if (sptr[1] == 'R')
				pos = 3;
			else
				pos = 4;
		}

		if (version == 1)
		{
			if (reponse)
				sptr[pos] = '\0';
			else
				sptr[pos] = '!';
		}
		else
		{
			if (reponse)
				sptr[pos] = '-';
			else
				sptr[pos] = '+';
		}
		sptr[pos + 1] = '\0';
		n_cpy (4, ui->ctl, ptr);
	}

	ui->ui = (strncmp (ui->ctl, "UI", 2) == 0);

	ui->pid = 0xf0;
}



static int kam_inbuf (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui)
{
	int nbcar, can;

	int type;
	int i, lport;

	int valid = 0;
	int deja;

	char stemp[80];
	static char buf[600];
	char *ptr;

	df ("lit_txt_kam", 1);
	if ((nbcar = kam_read (*port, buf)) >= 3)
	{
		lport = (int) (*(buf + 1) - '0');
		if (p_port[*port].ccanal != lport)
		{
			/* Recherche l'autre port du TNC */
			for (i = 1; i < NBPORT; i++)
			{
				if (i == *port)
					continue;
				if (p_port[i].ccom == p_port[*port].ccom)
				{
					*port = i;
					break;
				}
			}
		}
		type = (int) *buf;
		can = (int) (*(buf + 2) - 'A') + 1;

		switch (type)
		{

		case 'M':

			ptr = buf + 3;
			nbcar -= 3;

			deja = 0;
			while ((*ptr) && (nbcar))
			{
				--nbcar;
				if (*ptr++ == ':')
				{
					if (deja)
					{
						break;
					}
					deja = 1;
				}
			}

			kam_get_ui (*port, buf + 3, ui);

			if (nbcar > 0)
			{
				++ptr;
				--nbcar;
				memcpy (buffer, ptr, nbcar);
				sprintf (ui->txt, " (%d)", nbcar);
			}
			else
				nbcar = 0;

			valid = 1;
			*cmd = UNPROTO;
			*canal = can;
			*len = nbcar;

			break;

		case 'S':
			nbcar -= 3;
			ptr = buf + 3;

			ptr[nbcar] = '\0';
			*buffer = '\0';
			*len = 0;

			/* TOR modes ... */
			if (strncmp (ptr, "<LINKED", 7) == 0)
			{
				strcpy (stemp, "KAM");
				sscanf (ptr, "%*s %*s %s", stemp);
				if (stemp[strlen (stemp) - 1] == '>')
					stemp[strlen (stemp) - 1] = '\0';
				sprintf (buffer, "(%d) CONNECTED to %s", can, stemp);
				valid = 1;
				*cmd = COMMAND;
				*canal = 1;
				*len = strlen (buffer);
				break;
			}
			else if (strstr (ptr, "STANDBY"))
			{
				sprintf (buffer, "(%d) DISCONNECTED fm KAM", can);
				p_port[*port].transmit = 0;
				valid = 1;
				*cmd = COMMAND;
				*canal = 1;
				*len = strlen (buffer);
				if ((p_port[*port].moport & 0x80) == 0)
				{
					/* Retour en mode packet */
					kam_tor (*port, "X", 1);
				}
				break;
			}

			else if (*ptr != '*')
				break;

			ptr += 4;

			*buffer = '\0';
			switch (*ptr)
			{
			case 'C':
				valid = 1;
				sscanf (ptr, "%*s %*s %s", stemp);
				sprintf (buffer, "(%d) CONNECTED to %s", can, stemp);
				/* Connection */
				break;
			case 'D':
				valid = 1;
				sprintf (buffer, "(%d) DISCONNECTED fm KAM", can);
				/* Deconnection */
				break;
			case 'r':
				valid = 1;
				sprintf (buffer, "(%d) LINK FAILURE with KAM", can);
				break;
			default:
				valid = 0;
				break;
			}

			*cmd = COMMAND;
			*canal = can;
			*len = strlen (buffer);

			break;

		case 'D':
			nbcar -= 3;
			memcpy (buffer, buf + 3, nbcar);

			valid = 1;
			*cmd = DATA;
			if (can < 0)
				can = 1;
			*canal = can;
			*len = nbcar;
			break;

		case 'C':
			nbcar -= 3;
			memcpy (buffer, buf + 3, nbcar);

			*len = 0;
			*cmd = NOCMD;

			if (nbcar > 0)
			{
				if (strncmp (buf + 3, "FREE", 4) == 0)
				{
					stat_ch sta;
					int tnc = p_port[*port].ccom;

					memset (&sta, 0, sizeof (sta));

					*port = kam_try[tnc].port;
					can = kam_try[tnc].stream;

					lit_kam_status (*port, buf + 3, &sta);

					memcpy (buffer, &sta, sizeof (sta));
					valid = 1;
					*cmd = STATS;
					*canal = can;
					*len = 0;
				}
				else if (strncmp (buf + 3, "TRIES", 5) == 0)
				{
					stat_ch sta;
					int tnc = p_port[*port].ccom;

					memset (&sta, 0, sizeof (sta));

					*port = kam_try[tnc].port;
					can = kam_try[tnc].stream;

					lit_kam_tries (*port, can, buf + 3, &sta);

					memcpy (buffer, &sta, sizeof (sta));
					valid = 1;
					*cmd = STATS;
					*canal = can;
					*len = 0;

					kam_try[tnc].port = 0;
				}
				else
				{
					valid = 1;
					*cmd = ECHOCMD;
					*canal = can;
					*len = nbcar;
				}
			}
			break;

		case 'R':
			nbcar -= 3;
			memcpy (buffer, buf + 3, nbcar);

			valid = 1;
			*cmd = UNPROTO;
			*canal = can;
			*len = nbcar;

			break;

		case 'I':
			valid = 0;
			*cmd = TOR;
			p_port[*port].transmit = (can != '0');
			*len = 0;
			break;

		default:
			break;

		}
	}
	ff ();
	return (valid);
}


void env_com_kam (int port, int canal, char *buffer)
{
	if (toupper (*buffer) == 'B')
	{
		paclen_change (port, canal, buffer);
	}
	else
	{
		kam_send (port, 0, 1, buffer, strlen (buffer));
		*buffer = '\0';
	}
}
