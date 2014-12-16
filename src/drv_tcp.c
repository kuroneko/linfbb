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


 /******************************************************
 *                                                     *
 *         FBB Driver for TCP/IP domain sockets       *
 *                                                     *
 *         F6FBB - 1996                                *
 *                                                     *
 ******************************************************/

#include <serv.h>
#include <fbb_drv.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#undef open
#undef read
#undef write
#undef close

#define AX25_CALLSID 10
#define READ_EVENT 1
#define WRITE_EVENT 2
#define EXCEPT_EVENT 4
#define QUEUE_EVENT 8

#define DISCONNECT  0
#define CPROGRESS   1
#define CONNECTED   2
#define WAITINGCALL 3
#define WAITINGPASS 4
#define READONLY    5
#define SENDCALL	6

#define DISC_EVENT  1
#define CONN_EVENT  2
#define RETR_EVENT  4
#define BUSY_EVENT  8
#define TIME_EVENT  16

typedef struct
{
	int ncan;
	int sock;
	int sock_ui;
	int port;
	int state;
	int paclen;
	int maxframe;
	int netrom;
	int event;
	int queue;
	int nego;
	int cesc;
	int lgial;
	int lpos;
	int lqueue;
	int nb_try;
	int nb_ret;
	int bs;
	int binary;
	int lgcall;
	long timeout;
	char ial[3];
	char call[80];
	indicat callsign;
	char *lbuf;
}
tcan_t;

typedef struct
{
	int sock_ui;
	int rem_port;
	int curcan;
	int nbcan;
	char rem_addr[80];
	tcan_t *tcan;
}
tport_t;

static tport_t tport[NBPORT];

/* static int last_can = 1; */

static int stop_cnx (int port);
static int tcp_getline (int port, int can, char *buffer);
static int tcp_snd_dt (int port, int, char *, int);
static int tcp_cmd (int, int, char *);
static int tcp_connect (int, char *, int);
static int tcp_stat (int, int, stat_ch *);
static int tcp_paclen (int port, int);
static int s_free (tcan_t *);
static int s_status (tcan_t *);
static int tcp_check_call (int port, int can, char *callsign, char *address);
static int tcp_check_pass (int port, int can, char *callsign);
static int tcp_trame (int port, int canal, char *data, int len);
static int snd_tcp_ui(int port, char *buffer, int len, Beacon *ptr);
static int rcv_tcp_ui(int port, char *buffer, int *len, ui_header * ui);

static void clear_can (int port, int canal);
static void read_only_alert (int port, int);

/*
 * Generic functions of the driver
 */

/* Check or change status of a port/channel */
int sta_tcp (int port, int canal, int cmd, void *ptr)
{
	char *scan;
	char commande;

	switch (cmd)
	{
	case TNCSTAT:
		return (tcp_stat (port, canal, (stat_ch *) ptr));
	case PACLEN:
		*((int *) ptr) = tcp_paclen (port, canal);
		return (1);
	case BSCMD:
		tport[port].tcan[canal].bs = *((int *) ptr);
		return (1);
	case SNDCMD:
		return (tcp_cmd (port, canal, (char *) ptr));
	case SETBUSY:
		return stop_cnx (port);
	case PORTCMD:
		scan = (char *)ptr;
		commande = toupper(*scan);
		++scan;

		while (isspace(*scan))
			++scan;

		switch (commande)
		{
		case 'A' :
			n_cpy(39, tport[port].rem_addr, scan);
			break;
		case 'P' :
			tport[port].rem_port = atoi(scan);
			break;
		}
		return(1);
	}
	return (0);
}

/* Sends data */
int snd_tcp (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	int ret = 0;

	switch (cmd)
	{
	case COMMAND:
		break;

	case UNPROTO:
		if (p_port[port].typort == TYP_ETH)
		{
			snd_tcp_ui(port, buffer, len, ptr);
			return 1;
		}
		break;

	case DATA:
		if (len == 0) {
/*			fprintf (stderr, "FBB snd_tcp() DATA len = 0 !\n");*/
			break;
		}
		else
			ret = tcp_snd_dt (port, canal, buffer, len);
			break;
	}
	return (ret);
}

/* receives data */
int rcv_tcp (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui)
{
#define LGBUF 252
	char buf[LGBUF + 2];
	int can;
	int valid;
	int res;
	int i;

	*cmd = INVCMD;

	valid = 0;

	/* Teste les UIs */
	if (rcv_tcp_ui(*port, buffer, len, ui))
	{
		*canal = 0;
		*cmd = UNPROTO;
		return 1;
	}
	
	usleep(50000);		/* wait 50 msec to allow interrupt and avoid CPU overload */

	/* Teste s'il y a une connection */
	tport[*port].tcan[0].sock = p_port[*port].fd;
	res = s_status (&tport[*port].tcan[0]);

	if (res & READ_EVENT)
	{
		/* static char *TelnetInit = "\377\375\001\377\375\042"; */
		/* static char *TelnetInit = "\377\376\001\377\375\042"; */

		static char *TelnetInit =
		"\377\374\001\000Telnet Initialisation string                                   ";

		int new;
		unsigned addr_len;
		struct sockaddr_in sock_addr;
		addr_len = sizeof (sock_addr);

		new = accept (p_port[*port].fd, (struct sockaddr *) &sock_addr, &addr_len);
		if (new == -1)
		{
			perror ("rcv_tcp() accept");
			return (FALSE);
		}


		/* Affecter le nouveau socket a un canal vide */
		for (i = 1; i <= tport[*port].nbcan; i++)
		{
			if (tport[*port].tcan[i].state == DISCONNECT)
			{
				break;
			}
		}

		if (i > tport[*port].nbcan)
		{
			write (new, TelnetInit, strlen (TelnetInit));

			/* Impossible d'affecter le canal -> deconnexion */
			sprintf (buf, "\r\nSorry, no more channels available\r\n\r\n");
			write (new, buf, strlen (buf));
			close (new);
		}
		else
		{
			int val = 0;
			FILE *fptr;

			tport[*port].tcan[i].state = WAITINGCALL;
			tport[*port].tcan[i].sock = new;
			tport[*port].tcan[i].paclen = (val == 0) ? 250 : val;
			tport[*port].tcan[i].queue = s_free (&tport[*port].tcan[i]);
			tport[*port].tcan[i].timeout = time (NULL) + 120L;

			if (p_port[*port].typort == TYP_ETH)
			{
				sprintf (buf, "\r\n%s BBS data access\r\n\r\n", my_call);
				write (new, buf, strlen (buf));
			}
			else
			{
				write (new, TelnetInit, strlen (TelnetInit));

				if ((fptr = fopen (c_disque ("LANG\\TELNET.ENT"), "rt")) == NULL)
				{
					sprintf (buf, "\r\n%s BBS. TELNET Access\r\n\r\n", my_call);
					write (new, buf, strlen (buf));
/*					fprintf (stderr, "\n%s BBS. TELNET Access\n\n", my_call);*/

				}
				else
				{
					int nb;
					char *ptr;

					while (fgets (buf, LGBUF, fptr))
					{
						nb = strlen (buf);
						if (nb)
						{
							buf[nb - 1] = '\r';
							buf[nb] = '\n';
							buf[nb + 1] = '\0';
						}
						ptr = var_txt (buf);
						write (new, ptr, strlen (ptr));
					}
					fclose (fptr);
				}
			}
			sprintf (buf, "Callsign : ");
/*			fprintf (stderr, "Callsign : ");*/

			write (new, buf, strlen (buf));

			val = p_port[*port].pk_t;

			return (FALSE);
		}
	}

	for (i = 1 ; i <= tport[*port].nbcan ; i++)
	{
		/* Passe au canal suivant pour le polling */
		++tport[*port].curcan;
		if (tport[*port].curcan > tport[*port].nbcan)
			tport[*port].curcan = 1;

		can = tport[*port].curcan;

		if ((tport[*port].tcan[can].sock == -1) && (tport[*port].tcan[can].state != DISCONNECT))
		{
			sprintf (buffer, "(%d) DISCONNECTED fm TCP", can);
			
/*			fprintf (stderr, "(%d) DISCONNECTED fm TCP\n", can);*/

			tport[*port].tcan[can].state = DISCONNECT;
			clear_can (*port, can);
			*len = strlen (buffer);
			*cmd = COMMAND;
			*canal = can;
			return (TRUE);
		}

		/* Canal de communication */
		res = s_status (&tport[*port].tcan[can]);

		if (res & TIME_EVENT)
		{
			sprintf (buf, "Timeout, disconnected !\r\n");
/*			fprintf (stderr, "Timeout, disconnected !\n");*/
	
			write (tport[*port].tcan[can].sock, buf, strlen (buf));
			close (tport[*port].tcan[can].sock);
			clear_can (*port, can);
			return (FALSE);
		}

		if (res & WRITE_EVENT)
		{
			/* Can write to the socket... Unused */
		}

		if (res & EXCEPT_EVENT)
		{
			if (tport[*port].tcan[can].event == CONN_EVENT)
			{
				/* Appel sortant connecte */

				/* Le host distant a ete connecte.
				   sprintf(buffer, "*** Connected to %s-%d\r", 
				   tcan[can].callsign.call, tcan[can].callsign.num);
				   in_buf(voie, buffer, strlen(buffer)); */

				/* if (tcan[can].lgcall)
					write (tcan[can].sock, tcan[can].call, tcan[can].lgcall); */

				/* Connexion a la BBS... */
				sprintf (buffer, "(%d) CONNECTED to %s-%d",
						 can, tport[*port].tcan[can].callsign.call, tport[*port].tcan[can].callsign.num);
				
/*				fprintf (stderr, "(%d) CONNECTED fm tcp to %s-%d\n",
						 can, tport[*port].tcan[can].callsign.call, tport[*port].tcan[can].callsign.num);*/

				if (p_port[*port].typort == TYP_ETH)
					tport[*port].tcan[can].state = SENDCALL;
				else
					tport[*port].tcan[can].state = CONNECTED;

				tport[*port].tcan[can].nb_try = 0;
				tport[*port].tcan[can].timeout = 0L;
				*len = strlen (buffer);
				*cmd = COMMAND;
				*canal = can;
				return (TRUE);
			}
		}

#define LGTCP 1100

		if ((res & QUEUE_EVENT) || (res & READ_EVENT))
		{
			int nb = 0;
			int i;

			if (tport[*port].tcan[can].sock == -1)
			{
				printf ("read on invalid socket\n");
				return (FALSE);
			}

			/* Alloue le buffer si necessaire */
			if (tport[*port].tcan[can].lbuf == NULL)
			{
				tport[*port].tcan[can].lbuf = calloc (LGTCP, 1);
				tport[*port].tcan[can].lpos = 0;
				tport[*port].tcan[can].lqueue = 0;
				tport[*port].tcan[can].nb_ret = 0;
			}

			if (res & READ_EVENT)
			{
				/* Reste de la place ds le buffer ? */
				nb = ((LGTCP - tport[*port].tcan[can].lqueue) > 256) ? 256 : LGTCP - tport[*port].tcan[can].lqueue;
				if (nb)
				{
					nb = read (tport[*port].tcan[can].sock, buffer, nb);
					if ((nb == 0) || ((nb == -1) && (errno == ENOTCONN)))
					{
						/* Deconnection */
						sprintf (buffer, "(%d) DISCONNECTED fm TCP", can);
/*						fprintf (stderr, "(%d) DISCONNECTED fm TCP\n", can);*/
						
						close (tport[*port].tcan[can].sock);
						clear_can (*port, can);
						*len = strlen (buffer);
						*cmd = COMMAND;
						*canal = can;
						return (TRUE);
					}
					else if (nb == -1)
					{
						printf ("errno = %d\n", errno);
						perror ("read");
						return (FALSE);
					}
				}
			}

			{
				int process;
				int pos;
				int lg;
				char *ptr;
				char *address;
				unsigned addr_len;
				struct sockaddr_in sock_addr;

				if (nb)
				{
					nb = tcp_trame (*port, can, buffer, nb);

					if (nb == 0)
						return (FALSE);
				}

				pos = tport[*port].tcan[can].lpos + tport[*port].tcan[can].lqueue;
				if (pos > LGTCP)
					pos -= LGTCP;

				for (i = 0; i < nb; i++)
				{
					if (tport[*port].tcan[can].lqueue > (LGTCP - 10))
					{
						++tport[*port].tcan[can].nb_ret;
						break;
					}

					tport[*port].tcan[can].lbuf[pos] = buffer[i];
					if (++pos == LGTCP)
						pos = 0;

					++tport[*port].tcan[can].lqueue;

					if (buffer[i] == '\r')
					{
						++tport[*port].tcan[can].nb_ret;
					}
				}

				nb = tport[*port].tcan[can].lqueue;

				tport[*port].tcan[can].timeout = time (NULL) + 120L;
				process = (tport[*port].tcan[can].nb_ret > 0);

				switch (tport[*port].tcan[can].state)
				{
				case SENDCALL:
					lg = tcp_getline (*port, can, buffer);
					if (lg == 0)
						return 0;

					buffer[lg] = '\0';
					if (strstr(buffer, "allsig"))
					{
						write(tport[*port].tcan[can].sock, tport[*port].tcan[can].call, tport[*port].tcan[can].lgcall);
						tport[*port].tcan[can].state = CONNECTED;
						tport[*port].tcan[can].lgcall = 0;
						return (TRUE);
					}
					break;
				case WAITINGCALL:
					if (!process)
						return (FALSE);

					if (!tcp_getline (*port, can, buffer))
						return 0;

					sup_ln (strupr (buffer));

					addr_len = sizeof (sock_addr);
					if ((getpeername(tport[*port].tcan[can].sock, (struct sockaddr *)&sock_addr, &addr_len) == 0) && (sock_addr.sin_family == AF_INET))
						address = inet_ntoa(sock_addr.sin_addr);
					else
						address = NULL;

					switch (tcp_check_call (*port, can, buffer, address))
					{
					case 0:
						if (++tport[*port].tcan[can].nb_try > 3)
						{
							sprintf (buf, "Callsign error, disconnected !\r\n");
/*							fprintf (stderr, "Callsign error, disconnected !\n");*/
							write (tport[*port].tcan[can].sock, buf, strlen (buf));
							close (tport[*port].tcan[can].sock);
							tport[*port].tcan[can].sock = -1;
						}
						else
						{
							char buf[80];

							buffer[20] = '\0';
							sprintf (buf, "Invalid callsign \"%s\" !\r\n", buffer);
/*							fprintf (stderr, "Invalid callsign \"%s\" !\n", buffer);*/
							write (tport[*port].tcan[can].sock, buf, strlen (buf));
							sprintf (buf, "Callsign : ");
							
/*							fprintf (stderr, "Callsign : ");*/
							
							write (tport[*port].tcan[can].sock, buf, strlen (buf));
						}
						break;
					case 1:
						{
							info buf_info;
							FILE *fptr;
							unsigned coord;

							if ((coord = chercoord (tport[*port].tcan[can].callsign.call)) == 0xffff)
							{
								close (tport[*port].tcan[can].sock);
								tport[*port].tcan[can].sock = -1;
								break;
							}

							fptr = ouvre_nomenc ();
							fseek (fptr, (long) coord * sizeof (info), 0);
							fread ((char *) &buf_info, sizeof (info), 1, fptr);
							ferme (fptr, 11);

							if (!MOD (buf_info.flags))
							{
								read_only_alert (*port, can);
								tport[*port].tcan[can].state = READONLY;
							}
							else
							{
								sprintf (buf, "Password : ");
								write (tport[*port].tcan[can].sock, buf, strlen (buf));
								tport[*port].tcan[can].state = WAITINGPASS;
								tport[*port].tcan[can].nb_try = 0;
							}
						}
						break;
					case 2:
						if (++tport[*port].tcan[can].nb_try > 3)
						{
							sprintf (buf, "Callsign error, disconnected !\r\n");
/*							fprintf (stderr, "Callsign error, disconnected !\n");*/
							write (tport[*port].tcan[can].sock, buf, strlen (buf));
							close (tport[*port].tcan[can].sock);
							tport[*port].tcan[can].sock = -1;
						}
						else
						{
							if (p_port[*port].moport & 0x40)
							{
								read_only_alert (*port, can);
								tport[*port].tcan[can].state = READONLY;
							}
							else
							{
								char buf[80];

								sprintf (buf, "Unregistered callsign \"%s\" !\r\n", buffer);
								write (tport[*port].tcan[can].sock, buf, strlen (buf));
								sprintf (buf, "For registration send message to SYSOP.\r\n\n", buffer);
								write (tport[*port].tcan[can].sock, buf, strlen (buf));
								sprintf (buf, "Callsign : ");
								write (tport[*port].tcan[can].sock, buf, strlen (buf));
							}
						}
						break;
					case 5:
						sprintf (buffer, "(%d) CONNECTED to %s-%d",
								 can, tport[*port].tcan[can].callsign.call,
								 tport[*port].tcan[can].callsign.num);
/*						fprintf (stderr, "(%d) CONNECTED to %s-%d\n",
								 can, tport[*port].tcan[can].callsign.call,
								 tport[*port].tcan[can].callsign.num);*/
						tport[*port].tcan[can].state = CONNECTED;
						tport[*port].tcan[can].nb_try = 0;
						tport[*port].tcan[can].timeout = 0L;
						*len = strlen (buffer);
						*cmd = COMMAND;
						*canal = can;
						return (TRUE);
					}
					break;

				case WAITINGPASS:
					if (!process)
						return (FALSE);

					if (!tcp_getline (*port, can, buffer))
						return 0;

					sup_ln (buffer);
					if (tcp_check_pass (*port, can, buffer))
					{
						sprintf (buf, "\r\nLogon Ok. Type NP to change password.\r\n\r\n");
						write (tport[*port].tcan[can].sock, buf, strlen (buf));
						sprintf (buffer, "(%d) CONNECTED to %s-%d",
								 can, tport[*port].tcan[can].callsign.call,
								 tport[*port].tcan[can].callsign.num);
						tport[*port].tcan[can].state = CONNECTED;
						tport[*port].tcan[can].nb_try = 0;
						tport[*port].tcan[can].timeout = 0L;
						*len = strlen (buffer);
						*cmd = COMMAND;
						*canal = can;
						return (TRUE);
					}
					else
					{
						if (++tport[*port].tcan[can].nb_try > 3)
						{
							if (p_port[*port].moport & 0x40)
							{
								read_only_alert (*port, can);
								tport[*port].tcan[can].state = READONLY;
							}
							else
							{
								sprintf (buf, "Password error, disconnected !\r\n");
								write (tport[*port].tcan[can].sock, buf, strlen (buf));
								close (tport[*port].tcan[can].sock);
								tport[*port].tcan[can].sock = -1;
							}
						}
						else
						{
							char buf[80];

							sprintf (buf, "Password error !\r\nPassword : ");
							write (tport[*port].tcan[can].sock, buf, strlen (buf));
						}
					}
					return (FALSE);

				case READONLY:
					if (!process)
						return (FALSE);

					if (!tcp_getline (*port, can, buffer))
						return 0;

					if (toupper (*buffer) == 'Y')
					{
						/* Read-Only mode accepted */
						sprintf (buf, "\r\nLogon Ok. You have a read-only access.\r\n\r\n");
						write (tport[*port].tcan[can].sock, buf, strlen (buf));
						sprintf (buffer, "(%d) READONLY to %s-%d",
								 can, tport[*port].tcan[can].callsign.call,
								 tport[*port].tcan[can].callsign.num);
						tport[*port].tcan[can].state = CONNECTED;
						tport[*port].tcan[can].nb_try = 0;
						tport[*port].tcan[can].timeout = 0L;
						*len = strlen (buffer);
						*cmd = COMMAND;
						*canal = can;
						return (TRUE);
					}
					else
					{
						sprintf (buf, "Callsign : ");
						write (tport[*port].tcan[can].sock, buf, strlen (buf));
						tport[*port].tcan[can].state = WAITINGCALL;
					}
					break;

				default:
					pos = tport[*port].tcan[can].lpos;
					ptr = tport[*port].tcan[can].lbuf;
					for (i = 0; i < tport[*port].tcan[can].lqueue; i++)
					{
						buffer[i] = ptr[pos];
						if (++pos == LGTCP)
							pos = 0;
					}
					*len = tport[*port].tcan[can].lqueue;
					*cmd = DATA;
					*canal = can;
					tport[*port].tcan[can].nb_ret = 0;
					tport[*port].tcan[can].lpos = 0;
					tport[*port].tcan[can].lqueue = 0;
					tport[*port].tcan[can].timeout = 0L;
					return (TRUE);
				}
			}
		}
	}
	return (FALSE);
}

/* Open port */
int opn_tcp (int port, int nb)
{
	int i;
	int val;
	int len;
	int ok = TRUE;
	int tcp_port;
	char s[80];
	struct sockaddr_in sock_addr;
	char *ptr;

	tport[port].sock_ui = -1;
	
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = 0;

	/* Test if portname is hex number */
	ptr = p_com[(int) p_port[port].ccom].name;

	if (strcmp (ptr, "0") == 0)
	{
		tcp_port = p_com[(int) p_port[port].ccom].port;
	}
	else if (strspn (ptr, "0123456789abcdefABCDEF") != strlen (ptr))
	{
		/* It may be tcp address. Port number is in port */
		if (inet_aton (ptr, &sock_addr.sin_addr))
			tcp_port = p_com[(int) p_port[port].ccom].port;
		else
			tcp_port = p_com[(int) p_port[port].ccom].cbase;
	}
	else
	{
		/* for up compatibility */
		tcp_port = p_com[(int) p_port[port].ccom].cbase;
	}

	sock_addr.sin_port = htons (tcp_port);

	sprintf (s, "Init PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
	InitText (s);

	/*
	old_can = last_can;
	last_can += nb;

	if (last_can > TCP_MAXCAN)
		last_can = TCP_MAXCAN;
	
	for (i = old_can; i < last_can; i++)
	{
		clear_can (i);
	}
	*/
	
	tport[port].tcan = (tcan_t *)calloc(nb+1, sizeof(tcan_t));
	if (tport[port].tcan == NULL)
		return 0;
		
	tport[port].curcan = 1;
	tport[port].nbcan = nb;
	for (i = 0 ; i <= nb ; i++)
		clear_can(port, i);

	/* Socket reception d'appels */
	if (p_port[port].fd == 0)
	{

		sprintf (s, "Open PORT %d COM%d-%d",
				 port, p_port[port].ccom, p_port[port].ccanal);
		InitText (s);
		sleep (1);

		if ((p_port[port].fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror ("socket_r");
			return (0);
		}

		val = 1;
		len = sizeof (val);
		if (setsockopt (p_port[port].fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, len) == -1)
		{
			perror ("opn_tcp : setsockopt SO_REUSEADDR");
		}

		if (bind (p_port[port].fd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) != 0)
		{
			perror ("opn_tcp : bind");
			close (p_port[port].fd);
			p_port[port].fd = -1;
			return (0);
		}

		if (listen (p_port[port].fd, SOMAXCONN) == -1)
		{
			perror ("listen");
			close (p_port[port].fd);
			p_port[port].fd = -1;
			return (0);
		}

		memset (&tport[port].tcan[0], 0, sizeof (tcan_t));

	}

	/* Socket reception UIs */
	if (p_port[port].typort == TYP_ETH)
	{
		if ((tport[port].sock_ui = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			perror ("socket_ui");
			return (0);
		}

		if (bind (tport[port].sock_ui, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) != 0)
		{
			perror ("opn_tcp : bind");
			close (tport[port].sock_ui);
			tport[port].sock_ui = -1;
			return (0);
		}

	}
	
	sprintf (s, "Prog PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
	InitText (s);

	return (ok);
}

/* Close port */
int cls_tcp (int port)
{
	int i;

	for (i = 1; i <= tport[port].nbcan; i++)
	{
		if (tport[port].tcan[i].sock != -1)
		{
			close (tport[port].tcan[i].sock);
		}
		tport[port].tcan[i].state = DISCONNECT;
	}

	if ((p_port[port].typort == TYP_SCK) && (p_port[port].fd))
	{
		close (p_port[port].fd);
		p_port[port].fd = 0;
	}

	if (tport[port].sock_ui != -1)
	{
		close (tport[port].sock_ui);
		tport[port].sock_ui = -1;
	}
	
	free(tport[port].tcan);
	
	return (1);
}


/* 

 * Static functions
 *
 */

#define LF	0x0a
#define CR	0x0d
#define SE      0xf0
#define NOP     0xf1
#define DM      0xf2
#define BRK     0xf3
#define IP      0xf4
#define AO      0xf5
#define AYT     0xf6
#define EC      0xf7
#define EL      0xf8
#define GA      0xf9
#define SB      0xfa
#define WILL	0xfb
#define WONT	0xfc
#define DO	0xfd
#define DONT	0xfe
#define IAC     0xff

/* Telnet options */
#define TN_TRANSMIT_BINARY	0
#define TN_ECHO			1
#define TN_SUPPRESS_GA		3
#define TN_STATUS		5
#define TN_TIMING_MARK		6

/* Linemode options */
#define TN_LINEMODE		34
#define TN_LINEMODE_MODE	1
#define TN_LINEMODE_MODE_EDIT	1

static int stop_cnx (int port)
{
	if (p_port[port].fd) /* Prevent closing of 0 */
	{
		close(p_port[port].fd);
		p_port[port].fd = 0;
	}
	return 1;
}

static int tcp_trame (int port, int canal, char *data, int len)
{
	int i;
	int carac;
	int olg = 0;

	char *optr = data;

	int pial = tport[port].tcan[canal].lgial;

	if (tport[port].tcan[canal].binary)
	{
		return (len);
	}

	for (i = 0; i < len; i++)
	{
		carac = data[i] & 0xff;

		switch (pial)
		{
		case 0:
			if (carac == IAC)
				tport[port].tcan[canal].ial[pial++] = carac;
			else
			{
				if (tport[port].tcan[canal].nego == 0)
				{
					optr[olg++] = carac;
					if ((carac == CR) && (tport[port].tcan[canal].bs))
						pial = 10;
				}
				else
				{
					printf ("%02x ", carac);
				}
			}
			break;
		case 1:
			if (carac == IAC)
			{
				optr[olg++] = carac;
				pial = 0;
			}
			else if (tport[port].tcan[canal].nego)
			{
				printf ("%02x...\n", carac);
			}
			else
			{
				switch (carac)
				{

				case WILL:
					printf ("WILL ");
					break;
				case WONT:
					printf ("WONT ");
					break;
				case DO:
					printf ("DO ");
					break;
				case DONT:
					printf ("DONT ");
					break;

				case SB:
					printf ("SUBN ");
					tport[port].tcan[canal].nego = 1;
					pial = 0;
					break;
				case GA:
					printf ("GA\n");
					pial = 0;
					break;
				case EL:
					printf ("EL\n");
					pial = 0;
					break;
				case EC:
					printf ("EC\n");
					pial = 0;
					break;
				case AYT:
					printf ("AYT\n");
					pial = 0;
					break;
				case AO:
					printf ("AO\n");
					pial = 0;
					break;
				case IP:
					printf ("IP\n");
					pial = 0;
					break;
				case BRK:
					printf ("BRK\n");
					pial = 0;
					break;
				case DM:
					printf ("DM\n");
					pial = 0;
					break;
				case NOP:
					printf ("NOP\n");
					pial = 0;
					break;
				case SE:
					printf ("SE\n");
					/* l = 0; */
					tport[port].tcan[canal].nego = 0;
					pial = 0;
					break;
				default:
					printf ("%02x...\n", carac);
					break;
				}
				if (pial)
					tport[port].tcan[canal].ial[pial++] = carac;
			}
			break;
		case 2:
			{
				char buf[80];

/*				printf ("%02x...\n", carac);*/
				switch (tport[port].tcan[canal].ial[1])
				{
				case WILL:
					if (carac == TN_LINEMODE)
					{
						/* write(tport[port].tcan[canal].sock, buffer, strlen(buffer)); */
					}
					printf (" -> DONT %02x\n", carac);
					sprintf (buf, "%c%c%c", IAC, DONT, carac);
					write (tport[port].tcan[canal].sock, buf, strlen (buf));
					break;
				case DO:
					if (carac != TN_ECHO)
					{
						printf (" -> WONT %02x\n", carac);
						sprintf (buf, "%c%c%c", IAC, WONT, carac);
						write (tport[port].tcan[canal].sock, buf, strlen (buf));
					}
					break;
				case WONT:
				case DONT:
					break;

				case SB:
					break;
				}
				pial = 0;
			}
			break;
		case 10:
			{
				/* Teste le CRLF -> CR */
				if (carac != LF)
					optr[olg++] = carac;
				pial = 0;
			}
			break;
		}
	}

	tport[port].tcan[canal].lgial = pial;

	return (olg);
}

static int tcp_check_call (int port, int can, char *callsign, char *address)
{
	int res = 0;
	
	if (*callsign == '.')
	{
		tport[port].tcan[can].binary = 1;
		++callsign;
	}

	tport[port].tcan[can].callsign.num = extind (callsign, tport[port].tcan[can].callsign.call);
	if (find (tport[port].tcan[can].callsign.call))
	{
		if (chercoord (tport[port].tcan[can].callsign.call) != 0xffff)
			res = 1;
		else
			res = 2;
	}

	// Adresse autorisee sans password ?
	if (address)
	{
		FILE *fptr;
		char str[256];
		char ip[80];
		char pass[256];

		fptr = fopen(c_disque("passwd.sys"), "rt");
		if (fptr)
		{
			while (fgets(str, sizeof(str), fptr))
			{
				if ((*str == '\0') || (*str == '#'))
					continue;
				*ip = *pass = '\0';
				sscanf(str, "%s %s", ip, pass);
				if (strcmp("+", ip) == 0)
				{
					res = 5;
					break;
				}
				if (strcmp(address, ip) == 0)
				{
					if (strcmp("+", pass) == 0)
						res = 5;
					else
						res = 2;
					break;
				}
			}
			fclose(fptr);
		}
	}

	return (res);
}

static int tcp_check_pass (int port, int can, char *passwd)
{
	unsigned record;

	record = chercoord (tport[port].tcan[can].callsign.call);

	if (record != 0xffff)
	{
		FILE *fptr;
		info frec;

		fptr = ouvre_nomenc ();
		fseek (fptr, ((long) record * sizeof (info)), 0);
		fread (&frec, sizeof (info), 1, fptr);
		ferme (fptr, 92);

		if (strcasecmp (passwd, frec.pass) == 0)
		{
			return (TRUE);
		}
	}

	return (FALSE);
}

static int tcp_snd_dt (int port, int canal, char *buffer, int len)
{
	int i;
	int nb;
	int lg;
	char *ptr;

	char buf[600];

	if (tport[port].tcan[canal].sock == -1)
		return (FALSE);

	for (ptr = buf, nb = 0, i = 0; i < len; i++)
	{

		*ptr++ = buffer[i];
		++nb;
		if (!tport[port].tcan[canal].binary)
		{
			if ((buffer[i] == '\r') && (tport[port].tcan[canal].bs))
			{
				*ptr++ = '\n';
				++nb;
			}
			else if (buffer[i] == IAC)
			{
				*ptr++ = IAC;
				++nb;
			}
		}
	}

	lg = write (tport[port].tcan[canal].sock, buf, nb);
	if (lg == -1)
	{
/*		printf ("tcp_snd_dt() Error %d on socket %d\n", errno, tport[port].tcan[canal].sock); */
/*		perror ("tcp_snd_dt() write on socket");*/
		return (FALSE);
	}
	else if (lg < nb)
	{
/*		printf ("tcp_snd_dt() Cannot write %d bytes on socket %d\n", nb - lg, tport[port].tcan[canal].sock); */
		return (FALSE);
	}
	return (TRUE);
}

static int tcp_cmd (int port, int canal, char *cmd)
{
	switch (*cmd++)
	{
	case 'I':
		/* source callsign */
		while (isspace (*cmd))
			++cmd;
		tport[port].tcan[canal].callsign.num = extind (cmd, tport[port].tcan[canal].callsign.call);
		break;
	case 'D':
		/* Deconnection */
		if (tport[port].tcan[canal].sock != -1)
		{
			close (tport[port].tcan[canal].sock);
			tport[port].tcan[canal].sock = -1;
		}
		break;
	case 'C':
		tcp_connect (port, cmd, canal);
		break;
	default:
		return (0);
	}
	return (1);
}

static int tcp_connect (int port, char *commande, int can)
{
	int fd;
	int ioc;
	int tcp_port;
	char tcp_add[80];
	char indic[80];

	struct sockaddr_in sock_addr;
	struct hostent *host;
		
	/* Connection */
	while (isspace (*commande))
		++commande;

	tport[port].tcan[can].paclen = p_port[port].pk_t;
	tport[port].tcan[can].maxframe = p_port[port].frame;
	tport[port].tcan[can].port = port;

	if (tport[port].tcan[can].state != DISCONNECT)
		return (0);

	tcp_add[0] = '\0';
	tcp_port = 23;

	commande[79] = '\0';
	
	if (p_port[port].typort == TYP_ETH)
	{
		char *ptr;

		strcpy(tcp_add, tport[port].rem_addr);
		tcp_port = tport[port].rem_port;
		
		sprintf(tport[port].tcan[can].call, ".%s-%d^", tport[port].tcan[can].callsign.call, tport[port].tcan[can].callsign.num);

		/* Remote call */
		ptr = strtok(commande, " ,\t");
		if (!ptr)
			return(0);

		strcat(tport[port].tcan[can].call, strupr(ptr));

		/* Digis */
		ptr = strtok(NULL, " ,\t");
		if (ptr)
			strcat(tport[port].tcan[can].call, " via");

		while (ptr)
		{
			if ((stricmp(ptr, "via") != 0) && (stricmp(ptr, "v") != 0))
			{
				strcat(tport[port].tcan[can].call, " ");
				strcat(tport[port].tcan[can].call, strupr(ptr));
			}
			ptr = strtok(NULL, " ,\t");
		}

		strcat(tport[port].tcan[can].call, "\r");
		tport[port].tcan[can].lgcall = strlen(tport[port].tcan[can].call);

		tport[port].tcan[can].binary = 1;
		/* Canal->phase = SENDCALL; */
	}
	else
	{
		sscanf (commande, "%s %s %i", indic, tcp_add, &tcp_port);

		tport[port].tcan[can].callsign.num = extind (indic, tport[port].tcan[can].callsign.call);
		tport[port].tcan[can].lgcall = 0;

		if (!find (tport[port].tcan[can].callsign.call))
		{
/*			fprintf (stderr, "connect : invalid callsign %s\n", indic);*/
			return (0);
		}

		if (tcp_add[0] == '\0')
		{
/*			fprintf (stderr, "connect : tcp address missing\n");*/
			return (0);
		}
		
	}
	
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror ("socket_ax");
		clear_can (port, can);
		return (0);
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons (tcp_port);

	host = gethostbyname(tcp_add);
	if (host)
		sock_addr.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;
	else
		sock_addr.sin_addr.s_addr = inet_addr (tcp_add);

	ioc = 1;
	ioctl (fd, FIONBIO, &ioc);
	if (connect (fd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) == -1)
	{
		if (errno != EINPROGRESS)
		{
			printf ("\n");
	/*		perror ("connect");*/
/*	printf ("*** Cannot connect %s canal %d\n", commande, can);*/
			clear_can (port, can);
			close (fd);
			return (0);
		}
	}

/*	printf ("*** Connect in progress %s canal %d\n", commande, can);*/

	tport[port].tcan[can].sock = fd;
	tport[port].tcan[can].state = CPROGRESS;
	tport[port].tcan[can].queue = s_free (&tport[port].tcan[can]);

	return (TRUE);
}

static int tcp_stat (int port, int canal, stat_ch * ptr)
{
	int val;

	if ((canal == 0) || (tport[port].tcan[canal].sock == -1))
		return (0);

	ptr->mem = 100;

	val = s_free (&tport[port].tcan[canal]);

	if (tport[port].tcan[canal].state != CONNECTED)
		ptr->ack = 0;
	else
	{
		ptr->ack = (tport[port].tcan[canal].queue - val) / tport[port].tcan[canal].paclen;
		if ((tport[port].tcan[canal].queue - val) && (ptr->ack == 0))
			ptr->ack = 1;
	}

	return (1);
}

static int s_status (tcan_t * can)
{
	int nb;
	int res = 0;
	fd_set tcp_read;
	fd_set tcp_write;
	fd_set tcp_excep;
	struct timeval to;

	if (can->sock <= 0) /* Was -1.  Sock=0 during housekeeping.  Cause of select errors */
		return (0);

	if ((can->timeout) && (can->timeout < time (NULL)))
	{
		res |= TIME_EVENT;
		can->timeout = 0L;
		return (res);
	}

	if (can->lqueue)
	{
		res |= QUEUE_EVENT;
	}

	to.tv_sec = to.tv_usec = 0;
	can->event = 0;

	FD_ZERO (&tcp_read);
	FD_ZERO (&tcp_write);
	FD_ZERO (&tcp_excep);

	FD_SET (can->sock, &tcp_read);
	FD_SET (can->sock, &tcp_write);
	FD_SET (can->sock, &tcp_excep);

	nb = select (can->sock + 1, &tcp_read, &tcp_write, &tcp_excep, &to);
	if (nb == -1)
	{
		perror ("select");
		return (res);
	}
	else if (nb == 0)
	{
		return (res);
	}
	else
	{
		if (FD_ISSET (can->sock, &tcp_read))
		{
			res |= READ_EVENT;
		}
		if (FD_ISSET (can->sock, &tcp_write))
		{
			if (can->state == CPROGRESS)
			{
				if (p_port[port].typort == TYP_ETH)
					can->state = SENDCALL;
				else
					can->state = CONNECTED;
				write (can->sock, &res, 0);
				res |= EXCEPT_EVENT;
				can->event = CONN_EVENT;
			}
			else
				res |= WRITE_EVENT;
		}
		if (FD_ISSET (can->sock, &tcp_excep))
		{
			res |= EXCEPT_EVENT;
		}
	}
	return (res);
}

static int tcp_paclen (int port, int canal)
{
	if (tport[port].tcan[canal].sock == -1)
		return (0);

	return (tport[port].tcan[canal].paclen);
}

static int s_free (tcan_t * can)
{
	int queue_free;

	if (ioctl (can->sock, TIOCOUTQ, &queue_free) == -1)
	{
		perror ("ioctl : TIOCOUTQ");
		return (0);
	}
	return (queue_free);
}

static void clear_can (int port, int canal)
{
/*	fprintf (stderr, "drv_tcp : clear_can() port %d canal %d\n", port, canal);*/

	if (tport[port].tcan[canal].lbuf)
		free (tport[port].tcan[canal].lbuf);
	memset (&tport[port].tcan[canal], 0, sizeof (tcan_t));
	tport[port].tcan[canal].sock = -1;
	tport[port].tcan[canal].bs = 1;
	tport[port].tcan[canal].binary = 0;
	tport[port].tcan[canal].state = DISCONNECT;
}

#define READ_ONLY "\r\nLogin in read-only mode.\r\nYou may leave a message to SYSOP.\r\n\r\nGo on anyway (Y/N) ? "

static void read_only_alert (int port, int can)
{
	char buf[256];

	sprintf (buf, READ_ONLY);
	write (tport[port].tcan[can].sock, buf, strlen (buf));
}

/* Copie une ligne dans le buffer */
static int tcp_getline (int port, int can, char *buffer)
{
	int i = 0;
	int c;
	int pos;
	char *ptr;

	pos = tport[port].tcan[can].lpos;
	ptr = tport[port].tcan[can].lbuf;

	while (tport[port].tcan[can].lqueue)
	{
		c = ptr[pos];
		if (++pos == LGTCP)
			pos = 0;

		--tport[port].tcan[can].lqueue;

		if ((c == '\n') && (tport[port].tcan[can].bs))
			continue;

		buffer[i++] = c;

		if (c == '\r')
		{
			--tport[port].tcan[can].nb_ret;
			break;
		}
	}

	buffer[i] = '\0';

	tport[port].tcan[can].lpos = pos;

	return (i);
}

static int snd_tcp_ui(int port, char *buffer, int len, Beacon *ptr)
{
	int i;
	int lg;
	char buf[1024];
	char call[10];
	char desti[120];
	int sock_ui;
	struct sockaddr_in sock_addr;

	sock_addr.sin_family      = AF_INET;
	sock_addr.sin_addr.s_addr = INADDR_ANY;
	sock_addr.sin_port        = 0;

	sock_ui = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_ui < 0)
	{
		perror ("socket_ui");
		return (0);
	}
	
	if (bind (sock_ui, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) != 0)
	{
		perror ("snd_tcp_ui : bind");
		close (sock_ui);
		return (0);
	}

	sprintf(desti, "%s-%d", ptr->desti.call, ptr->desti.num);
	for (i = 0; i < ptr->nb_digi; i++)
	{
		strcat(desti, " ");
		sprintf(call, "%s-%d", ptr->digi[i].call, ptr->digi[i].num);
		strcat(desti, call);
	}

	sprintf(buf, "%d^%s-%d^%s^UI^F0^%d^", port, mycall, myssid, desti, len);
	lg = strlen(buf);

	memcpy(buf+lg, buffer, len);

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(tport[port].rem_addr);
	sock_addr.sin_port = htons(tport[port].rem_port);
	lg = sendto(sock_ui, buf, lg+len, 0, (const struct sockaddr *)&sock_addr, sizeof (sock_addr));
	
	close(sock_ui);

	if (lg < 0)
	{
		perror ("snd_tcp_ui:sendto");
		return (0);
	}

	return(1);
}

static int rcv_tcp_ui(int port, char *buffer, int *len, ui_header * ui)
{
	int nb;
	int lg;
	fd_set tcp_read;
	struct timeval to;
	struct sockaddr_in sock_addr;
	unsigned addr_len = sizeof (sock_addr);
	char buf[1024];
	char *ptr;
	char *p;

	if (tport[port].sock_ui == -1)
		return (0);

	to.tv_sec = to.tv_usec = 0;

	FD_ZERO (&tcp_read);

	FD_SET (tport[port].sock_ui, &tcp_read);

	nb = select (tport[port].sock_ui + 1, &tcp_read, NULL, NULL, &to);
	if (nb == -1)
	{
		perror ("select_ui");
		return (0);
	}
	else if (nb == 0)
	{
		return (0);
	}
	
	nb = recvfrom(tport[port].sock_ui, buf, sizeof(buf), 0, (struct sockaddr *) &sock_addr, &addr_len);
	
	ptr = buf;
	
	/* remote port ignored */
	lg = 0;
	p = ptr;
	while (*ptr != '^')
	{
		++ptr;
		++lg;
		--nb;
	}
	++ptr;
	--nb;
	ui->port = port;

	/* from */
	lg = 0;
	p = ptr;
	while (*ptr != '^')
	{
		++ptr;
		++lg;
		--nb;
	}
	n_cpy(lg, ui->from, p);
	++ptr;
	--nb;

	/* to */
	lg = 0;
	p = ptr;
	while (*ptr != '^')
	{
		if (*ptr == ' ')
		{
			/* Digis ... */
			break;
		}
		++ptr;
		++lg;
		--nb;
	}
	n_cpy(lg, ui->to, p);

	ui->via[0] = '\0';
	if (*ptr == ' ')
	{
		/* Digis ... */
		++ptr;
		--nb;

		lg = 0;
		p = ptr;
		while (*ptr != '^')
		{
			++ptr;
			++lg;
			--nb;
		}
		n_cpy(lg, ui->via, p);
		strcat(ui->via, " ");
	}

	++ptr;
	--nb;

	/* ctrl */
	lg = 0;
	p = ptr;
	while (*ptr != '^')
	{
		++ptr;
		++lg;
		--nb;
	}
	n_cpy(lg, ui->ctl, p);
	ui->ui = (strncmp("UI", p, 2) == 0);
	++ptr;
	--nb;

	/* pid */
	lg = 0;
	p = ptr;
	while (*ptr != '^')
	{
		++ptr;
		++lg;
		--nb;
	}
	sscanf(p, "%x", &ui->pid);
	++ptr;
	--nb;

	/* len */
	lg = 0;
	p = ptr;
	while (*ptr != '^')
	{
		++ptr;
		++lg;
		--nb;
	}
	
	lg = atoi(p);
	++ptr;
	--nb;

	sprintf (ui->txt, " (%d)", lg);

	if (lg != nb)
		return 0;

	if (lg > 0)
		memcpy(buffer, ptr, lg);

	*len = lg;
	return lg;
}

