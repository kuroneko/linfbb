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

#include <serv.h>

static void pk_check (int port);
static int pk_inbuf (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ptr);
static int pk_read (int port, char *buffer);
static int pk_send (int port, int canal, int type, char *chaine, int len);
static int pk_send_ok (int port, int canal, int type, char *buffer, int len);
static int pk_send_ui (int port, char *buffer, int len, Beacon * beacon);
static int pk_stat (int port, int canal, stat_ch * ptr);

static void pk_get_ui (int port, char *buffer, ui_header * ui);

int sta_aea (int port, int canal, int cmd, void *ptr)
{
	switch (cmd)
	{
	case TNCSTAT:
		return (pk_stat (port, canal, (stat_ch *) ptr));
	case SNDCMD:
	case ECHOCMD:
		return (pk_send (port, canal, 1, (char *) ptr, strlen (ptr)));
	case PORTCMD:
		return (pk_send (port, 0xf, 1, ptr, strlen (ptr)));
	}
	return (0);
}

int rcv_aea (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ptr)
{
	int val;

	val = pk_inbuf (port, canal, cmd, buffer, len, ptr);

	return (val);
}

int snd_aea (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	int ret = 0;

	switch (cmd)
	{
	case COMMAND:
		ret = pk_send (port, canal, 1, buffer, len);
		break;

	case DATA:
		ret = pk_send (port, canal, 2, buffer, len);
		break;

	case UNPROTO:
		ret = pk_send_ui (port, buffer, len, ptr);
		break;
	}

	return (ret);
}

static int pk_stat (int port, int canal, stat_ch * ptr)
{
	static long last_call = 0L;
	char buf[80];
	long temps;

	/* Demande les stats courantes */

	if (ptr)
	{
		temps = time (NULL);
		if (temps == last_call)
			return 1;

		last_call = temps;
	}

	/* Refait une demande de stats sur le canal */
	if (canal != 0xf)
		canal = canal - 1;

	sprintf (buf, "\001%cCO\027", 0x40 + canal);
	tncstr (port, buf, 0);

	return 1;
}

static int pk_send (int port, int canal, int type, char *buffer, int len)
{
	if (p_port[port].polling == 0)
	{
		/* Rien en cours, On envoie directement... */
		p_port[port].polling = 1;
		return (pk_send_ok (port, canal, type, buffer, len));
	}
	else
	{
		/* Bufferise la requete */

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
		cmd->cmd = type;
		cmd->len = len > 256 ? 256 : len;
		memcpy (cmd->buf, buffer, cmd->len);
	}
	return 1;
}

static void pk_check (int port)
{
	if (p_port[port].cmd)
	{
		PortData *cmd = p_port[port].cmd;

		p_port[port].cur_can = cmd->canal;
		pk_send_ok (port, cmd->canal, cmd->cmd, cmd->buf, cmd->len);
		p_port[port].cmd = cmd->next;
		m_libere (cmd, sizeof (PortData));
		p_port[port].last_cmde = '\0';
	}
	else
		p_port[port].polling = 0;
}

static int pk_send_ok (int port, int canal, int type, char *buffer, int len)
{
	/* Le premier canal est 1 dans FBB, 0 sur PK232 */
	if (canal != 0xf)
		--canal;

	switch (type)
	{
	case 0:					/* Unproto */
		send_tnc (port, 0x1);
		send_tnc (port, 0x29);
		break;
	case 1:					/* Commande */
		send_tnc (port, 0x1);
		send_tnc (port, 0x40 + canal);
		break;
	case 2:					/* Data */
		send_tnc (port, 0x1);
		send_tnc (port, 0x20 + canal);
		break;
	default:
		return (0);
	}

	while (len)
	{
		if ((*buffer == '\027') || (*buffer == '\020') || (*buffer == '\001'))
			send_tnc (port, '\020');
		send_tnc (port, *buffer++);
		--len;
	}
	send_tnc (port, '\027');
	sleep_ (1);

	if (type == 2)
		pk_stat (port, (canal == 0xf) ? canal : canal + 1, NULL);
	sleep_ (1);

	return (1);
}

static int pk_inbuf (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui)
{
	int commande, nbcar, can, nb, nback = 0;
	char *ptr;
	static char buf[600];
	int valid = 0;
	int offset = 1;

	if ((nbcar = pk_read (*port, buf)) < 2)
		return (0);

	*buffer = '\0';
	commande = *buf & 0xf0;
	can = *buf & 0x0f;

	switch (commande)
	{
	case 0x30:
		if ((can == 0xf) || (can == 0xd))
		{
			ptr = buf;
			nb = 0;

			if (buf[1] == 'p')
			{
				*port = (int) (*(buf + 2) - '0');
				offset += 3;
			}

			while (nbcar)
			{
				if ((*ptr == ':') || ((*ptr == '\r') && (isdigit (*(ptr - 1)))))
				{
					*ptr++ = '\0';
					--nbcar;
					break;
				}
				++ptr;
				--nbcar;
			}

			pk_get_ui (*port, buf + offset, ui);

			if (nbcar > 0)
			{
				sprintf (ui->txt, " (%d)", nbcar);
				memcpy (buffer, ptr, nbcar);
			}
			else
				nbcar = 0;

			valid = 1;
			*cmd = UNPROTO;
			*canal = can;
			*len = nbcar;
		}
		else
		{
			/* First channel of TNC must be 1 */
			can = can + 1;
			memcpy (buffer, buf + 1, nbcar - 1);

			valid = 1;
			*cmd = DATA;
			*canal = can;
			*len = nbcar - 1;
		}
		break;

	case 0x40:
		if (can == 0xf)
		{
			memcpy (buffer, buf + 1, 3);
			switch (buffer[2])
			{
			case 0:
				strcpy (buffer + 3, "OK\r");
				break;
			case 1:
				strcpy (buffer + 3, "bad\r");
				break;
			case 2:
				strcpy (buffer + 3, "too many\r");
				break;
			case 3:
				strcpy (buffer + 3, "not enough\r");
				break;
			case 4:
				strcpy (buffer + 3, "too long\r");
				break;
			case 5:
				strcpy (buffer + 3, "range\r");
				break;
			case 6:
				strcpy (buffer + 3, "callsign\r");
				break;
			case 7:
				strcpy (buffer + 3, "unknown command\r");
				break;
			case 8:
				strcpy (buffer + 3, "need VIA\r");
				break;
			case 9:
				strcpy (buffer + 3, "not while connected\r");
				break;
			case 10:
				strcpy (buffer + 3, "need MYCALL\r");
				break;
			case 11:
				strcpy (buffer + 3, "need MYSELCAL\r");
				break;
			case 12:
				strcpy (buffer + 3, "already connected\r");
				break;
			case 13:
				strcpy (buffer + 3, "not while disconnected\r");
				break;
			case 14:
				strcpy (buffer + 3, "different connectees\r");
				break;
			case 15:
				strcpy (buffer + 3, "too many packets outstanding\r");
				break;
			case 16:
				strcpy (buffer + 3, "clock not set\r");
				break;
			case 17:
				strcpy (buffer + 3, "need ALL/NONE/YES/NO\r");
				break;
			case 21:
				strcpy (buffer + 3, "not in this mode\r");
				break;
			default:
				memcpy (buffer + 3, buf + 3, nbcar - 3);
				buffer[nbcar] = '\r';
				buffer[nbcar + 1] = '\0';
				break;
			}
			buffer[2] = ':';

			commande = 0;
			valid = 1;
			*cmd = ECHOCMD;
			*canal = 0;
			*len = strlen (buffer);
		}
		else
		{
			int voie = no_voie (*port, can + 1);

			buf[nbcar] = '\0';
			if ((buf[1] == 'C') && (buf[2] == 'O'))
			{
				stat_ch sta;

				memset (&sta, 0, sizeof (sta));

				sta.ack = nback = buf[5] - '0';
				sta.ret = buf[6] - '0';
				sta.stat = buf[3] - '0';

				if (sta.stat == 0)
				{
					sta.ack = sta.ret = 0;
				}

				memcpy (buffer, &sta, sizeof (sta));

				valid = 1;
				*cmd = STATS;
				*canal = can + 1;
				*len = 0;
			}
			if ((nback > 14) && (svoie[voie]->ch_mon >= 0))
				svoie[voie]->ch_mon = -1;
		}
		break;
	case 0x50:
		if (can == 0xf)
		{
		}
		else
		{
			char stemp[80];
			char *ptr = buf + 1;

			buf[nbcar] = '\0';

			/* First channel of TNC must be 1 */
			can = can + 1;
			switch (*ptr)
			{
			case 'C':
				if (ptr[1] == 'O')
				{
					valid = 1;
					sscanf (ptr, "%*s %*s %s", stemp);
					sprintf (buffer, "(%d) CONNECTED to %s", can, stemp);
				}
				break;
			case 'D':
				valid = 1;
				sprintf (buffer, "(%d) DISCONNECTED fm PK", can);
				break;
			case 'R':
				valid = 1;
				sprintf (buffer, "(%d) LINK FAILURE with PK", can);
				break;
			default:
				valid = 0;
				break;
			}

			*cmd = COMMAND;
			*canal = can;
			*len = strlen (buffer);
		}
		break;
	default:
		break;
	}

	pk_check (*port);

	ff ();
	return (valid);
}


static int pk_read (int port, char *buffer)
{
	int c;
	int nb = 0;

	if ((!car_tnc (port)) || ((c = rcv_tnc (port)) != 1))
		return (0);

	for (;;)
	{
		c = rec_tnc (port);
		if (c == -1)
			continue;

		if (c == '\027')
			break;
		if (c == '\020')
			c = rcv_tnc (port);
		*buffer++ = c;
		++nb;
	}
	return (nb);
}

static int pk_send_ui (int port, char *buffer, int len, Beacon * beacon)
{
	char buf[600];
	char s[80];
	int i;
	int via = 1;

	sprintf (buf, "UN%s-%d", mot (beacon->desti.call), beacon->desti.num);

	for (i = 0; i < 8; i++)
	{
		if (*beacon->digi[i].call)
		{
			if (via)
			{
				strcat (buf, " VIA ");
				via = 0;
			}
			else
			{
				strcat (buf, ",");
			}
			sprintf (s, "%s-%d",
					 mot (beacon->digi[i].call), beacon->digi[i].num);
			strcat (buf, s);
		}
	}

	pk_send (port, 0xf, 1, buf, strlen (buf));
	pk_send (port, 0xf, 0, buffer, len);

	return (1);
}



static char *ccopy (char *dst, char *src)
{
	int nb = 11;

	while ((isalnum (*src)) || (*src == '-') || (*src == '*'))
	{
		*dst++ = *src++;
		if (--nb == 0)
			break;
	}
	*dst = '\0';

	return (src);
}

static void pk_get_ui (int port, char *buffer, ui_header * ui)
{
	char *ptr;
	char *sptr;

	memset (ui, 0, sizeof (ui_header));

	ui->port = port;
	ui->pid = 0xf0;
	ui->ui = 1;
	strcpy (ui->ctl, "??");

	ptr = buffer;
	ptr = ccopy (ui->from, ptr);
	sptr = strchr (ui->from, '*');
	if (sptr)
		*sptr = '\0';

	if (*ptr++ != '>')
		return;
	ptr = ccopy (ui->to, ptr);

	while (*ptr == '>')
	{
		++ptr;
		strcat (ui->via, ui->to);
		strcat (ui->via, " ");
		ptr = ccopy (ui->to, ptr);
	}

	if (*ptr == ' ')
		++ptr;

	sptr = ptr;

	while ((*sptr) && (*sptr != ')') && (*sptr != '>') && (*ptr != ']'))
		++sptr;
	*sptr = '\0';

	/* controle */
	*ui->ctl = '\0';
	ui->pid = 0;

	if ((*ptr == '(') || (*ptr == '<') || (*ptr == '['))
	{
		int pos = 0;
		int reponse = 0;

		++ptr;
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
			else
				ui->pid = 0xf0;
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
			sptr[1] = sptr[2];
			sptr[2] = sptr[4];
			pos = 3;
			ui->pid = 0xf0;
		}
		else
		{
			if (sptr[1] == 'R')
			{
				if (sptr[3] == 'P')
				{
					sptr[2] = sptr[5];
					pos = 3;
					reponse = 0;
				}
				else if (sptr[3] == 'F')
				{
					sptr[2] = sptr[5];
					pos = 3;
					reponse = 1;
				}
				else
				{
					sptr[2] = sptr[3];
					pos = 3;
				}
			}
		}

		if (reponse)
			sptr[pos] = '-';
		else
			sptr[pos] = '+';

		sptr[pos + 1] = '\0';
		n_cpy (4, ui->ctl, ptr);
	}

	ui->ui = (strncmp (ui->ctl, "UI", 2) == 0);
}
