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

/******************************
 *
 *  DRIVER pour POP IP access
 *
 ******************************/

#include <serv.h>

#include <fbb_drv.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

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

#define POP_USER 	10
#define POP_PASS 	11
#define POP_TRANS   12
#define POP_MSG		13

#define SMTP_START 	20
#define SMTP_USER 	21
#define SMTP_MD5 	22
#define SMTP_PASS 	23
#define SMTP_TRANS	24
#define SMTP_MSG	25

#define NNTP_USER 	30
#define NNTP_TRANS	31
#define NNTP_MSG	32
#define NNTP_LINE	33
#define NNTP_POSTMSG	34

#define DISC_EVENT  1
#define CONN_EVENT  2
#define RETR_EVENT  4
#define BUSY_EVENT  8
#define TIME_EVENT  16

#define SMTP_AUTH_NO	1
#define SMTP_AUTH_LOGIN	2
#define SMTP_AUTH_PLAIN	4
#define SMTP_AUTH_CRMD5	8

#define POP_AUTH_USER	1
#define POP_AUTH_APOP	2

typedef struct
{
	long mess_num;
	long mess_size;
	char mess_stat;
	char mess_del;
}
tmess_t;

typedef struct taddr
{
	char *address;
	struct taddr *next;
}
taddr_t;

typedef struct dbuf
{
	char *data;
	int len;
	struct dbuf *next;
}
dbuf_t;

typedef struct
{
	int cr;
	int head;
	int ncan;
	int sock;
	int port;
	int state;
	int paclen;
	int maxframe;
	int event;
	int queue;
	int lpos;
	int lqueue;
	int nb_try;
	int nb_ret;
	int nb_lines;
	int lgcall;
	int mess_nb;
	int mess_cur;
	int disc_request;
	int quit_request;
	int auth_ok;
	int extra;
	long mess_tot;
	long timeout;
	char call[80];
	char md5string[80];
	taddr_t *mail_from;
	taddr_t *rcpt_to;
	taddr_t *content;
	indicat callsign;
	char *lbuf;
	dbuf_t *lsend;
	dbuf_t *msgbuf;
	tmess_t *mess;
}
tcan_t;

typedef struct
{
	int pop_fd;
	int smtp_fd;
	int nntp_fd;
	int rem_port;
	int curcan;
	int nbcan;
	int pop_auth;
	int smtp_auth;
	char rem_addr[80];
	tcan_t *tcan;
}
tport_t;

static tport_t tport[NBPORT];

static int stop_cnx (int port);
static int s_free (tcan_t *);
static int s_status (tcan_t *);
static void clear_can (int port, int canal);
static int pop_paclen (int port, int);
static int pop_getline (int port, int can, char *buffer);
static int pop_snd_dt (int port, int can, char *, int);
static int pop_cmd (int port, int canal, char *cmd);
static int pop_ini (int port, int canal, char *cmd);
static int pop_stat (int port, int can, stat_ch *);
static int pop_check_call (int port, int can, char *callsign, struct sockaddr_in *address);
static int pop_check_pass (int port, int can, char *callsign);
static int pop_send(int port, int canal, char *fmt, ...);
static int pop_delete(int port, int canal);
static int pop_process_read(int port, int canal, int *cmd, char *buffer, int nb);
static int pop_to_bbs(int port, int canal, char *buffer, int clean);
static int smtp_rcv_dt(int port, int can, char *buffer, int len);
static int nntp_rcv_dt(int port, int can, char *buffer, int len);
static char *base64_to_str(char *str);
static char *str_to_base64(char *str);
static int pop_quit(int port, int can); 

/*** pop commands ***/
static int pop_cmd_apop (int port, int can, char *buffer);
static int pop_cmd_dele (int port, int can, char *buffer);
static int pop_cmd_last (int port, int can, char *buffer);
static int pop_cmd_list (int port, int can, char *buffer);
static int pop_cmd_noop (int port, int can, char *buffer);
static int pop_cmd_pass (int port, int can, char *buffer);
static int pop_cmd_quit (int port, int can, char *buffer);
static int pop_cmd_retr (int port, int can, char *buffer);
static int pop_cmd_rset (int port, int can, char *buffer);
static int pop_cmd_stat (int port, int can, char *buffer);
static int pop_cmd_top  (int port, int can, char *buffer);
static int pop_cmd_uidl (int port, int can, char *buffer);
static int pop_cmd_user (int port, int can, char *buffer);

/*** smtp commands ***/
static int smtp_cmd_md5 (int port, int can, char *buffer);
static int smtp_cmd_auth(int port, int can, char *buffer);
static int smtp_cmd_data(int port, int can, char *buffer);
static int smtp_cmd_ehlo(int port, int can, char *buffer);
static int smtp_cmd_helo(int port, int can, char *buffer);
static int smtp_cmd_mail(int port, int can, char *buffer);
static int smtp_cmd_noop(int port, int can, char *buffer);
static int smtp_cmd_pass(int port, int can, char *buffer, int base64);
static int smtp_cmd_quit(int port, int can, char *buffer);
static int smtp_cmd_rcpt(int port, int can, char *buffer);
static int smtp_cmd_rset(int port, int can, char *buffer);
static int smtp_cmd_user(int port, int can, char *buffer);
static int smtp_cmd_vrfy(int port, int can, char *buffer);

/*** nntp commands ***/
static int nntp_cmd_article(int port, int can, char *buffer);
static int nntp_cmd_body(int port, int can, char *buffer);
static int nntp_cmd_group(int port, int can, char *buffer);
static int nntp_cmd_head(int port, int can, char *buffer);
static int nntp_cmd_help(int port, int can, char *buffer);
static int nntp_cmd_ihave(int port, int can, char *buffer);
static int nntp_cmd_last(int port, int can, char *buffer);
static int nntp_cmd_list(int port, int can, char *buffer);
static int nntp_cmd_mode(int port, int can, char *buffer);
static int nntp_cmd_newgroups(int port, int can, char *buffer);
static int nntp_cmd_newnews(int port, int can, char *buffer);
static int nntp_cmd_next(int port, int can, char *buffer);
static int nntp_cmd_post(int port, int can, char *buffer);
static int nntp_cmd_quit(int port, int can, char *buffer);
static int nntp_cmd_slave(int port, int can, char *buffer);
static int nntp_cmd_stat(int port, int can, char *buffer);
static int nntp_cmd_xhdr(int port, int can, char *buffer);
static int nntp_cmd_xover(int port, int can, char *buffer);
static int nntp_cmd_authinfo(int port, int can, char *buffer);

static char *INVALID_CMD = "-ERR Invalid command; valid commands:";

/*
 * Driver's generic functions.
 */


int snd_pop (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	switch (cmd)
	{
	case DATA:
		return pop_snd_dt (port, canal, buffer, len);
	}
	return 0;
}

/* receives data */
int rcv_pop (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui)
{
#define LGBUF 252
	char buf[LGBUF + 2];
	int can;
/*	int valid;*/
	int res;
	int i;

	*cmd = INVCMD;

/*	valid = 0;*/

	/* Checks if there is a POP connection */
	res = 0;
	if (tport[*port].pop_fd)
	{
		tport[*port].tcan[0].sock = tport[*port].pop_fd;
		res = s_status (&tport[*port].tcan[0]);
	}

	if (res & READ_EVENT)
	{
		int new;
		int i;
		unsigned addr_len;
		struct sockaddr_in sock_addr;
		addr_len = sizeof (sock_addr);

		new = accept (tport[*port].pop_fd, (struct sockaddr *) &sock_addr, &addr_len);

		/* Assigns the new socket to an empty channel. */
		for (i = 1; i <= tport[*port].nbcan; i++)
		{
			if (tport[*port].tcan[i].state == DISCONNECT)
			{
				break;
			}
		}

		if (i > tport[*port].nbcan)
		{
			/* If cannot assign the channel, then disconnect. */
			sprintf (buf, "-ERR FBB POP3 server at %s - No free channel!\r\n", mycall);
			if (write (new, buf, strlen (buf)) != strlen (buf))
				perror("rcv_pop() socket write error");
			close (new);
		}
		else
		{
			int val = 0;

			tport[*port].tcan[i].state = POP_USER;
			tport[*port].tcan[i].sock = new;
			tport[*port].tcan[i].paclen = (val == 0) ? 250 : val;
			tport[*port].tcan[i].queue = s_free (&tport[*port].tcan[i]);
			tport[*port].tcan[i].timeout = time (NULL) + 120L;

			sprintf (tport[*port].tcan[i].md5string, "<%d.%ld@%s>",
				getpid(), time(NULL), mypath);
			sprintf (buf, "+OK FBB POP3 server ready %s\r\n", 
				tport[*port].tcan[i].md5string);
			if (write (new, buf, strlen (buf)) != strlen (buf))
				perror("rcv_pop() socket write error");
			
			val = p_port[*port].pk_t;

			return (FALSE);
		}
	}

	/* Tests if there is a SMTP connection. */
	res = 0;
	if (tport[*port].smtp_fd)
	{
		tport[*port].tcan[0].sock = tport[*port].smtp_fd;
		res = s_status (&tport[*port].tcan[0]);
	}

	if (res & READ_EVENT)
	{
		int new;
		int i;
		unsigned addr_len;
		struct sockaddr_in sock_addr;
		addr_len = sizeof (sock_addr);

		new = accept (tport[*port].smtp_fd, (struct sockaddr *) &sock_addr, &addr_len);

		/* Assigns the new socket to an empty channel. */
		for (i = 1; i <= tport[*port].nbcan; i++)
		{
			if (tport[*port].tcan[i].state == DISCONNECT)
			{
				break;
			}
		}

		if (i > tport[*port].nbcan)
		{
			/* If cannot assign the channel, then disconnects. */
			sprintf (buf, "421 FBB SMTP server at %s - No free channel!\r\n", mypath);
			if (write (new, buf, strlen (buf)) != strlen(buf))
				perror("rcv_pop() socket write error");
			close (new);
		}
		else
		{
			int val = 0;
			
			tport[*port].tcan[i].state = SMTP_START;
			tport[*port].tcan[i].sock = new;
			tport[*port].tcan[i].paclen = (val == 0) ? 250 : val;
			tport[*port].tcan[i].queue = s_free (&tport[*port].tcan[i]);
			tport[*port].tcan[i].timeout = time (NULL) + 120L;


/* 			sprintf (buf, "220 FBB SMTP server ready at %s\r\n", mypath); */
			sprintf (buf, "220 FBB ESMTP server ready at %s\r\n", mypath);
			if (write (new, buf, strlen (buf)) != strlen(buf))
				perror("rcv_pop() socket write error");

			val = p_port[*port].pk_t;

			return (FALSE);
		}
	}

	/* Checks if there is a NNTP connection. */
	res = 0;
	if (tport[*port].nntp_fd)
	{
		tport[*port].tcan[0].sock = tport[*port].nntp_fd;
		res = s_status (&tport[*port].tcan[0]);
	}
	
	if (res & READ_EVENT)
	{
		int new;
		unsigned addr_len;
		struct sockaddr_in sock_addr;
		addr_len = sizeof (sock_addr);

		new = accept (tport[*port].nntp_fd, (struct sockaddr *) &sock_addr, &addr_len);

		/* Assign the new socket to an empty channel. */
		for (i = 1; i <= tport[*port].nbcan; i++)
		{
			if (tport[*port].tcan[i].state == DISCONNECT)
			{
				break;
			}
		}

		if (i > tport[*port].nbcan)
		{
			/* If cannot assign the channel, then disconnect. */
			sprintf (buf, "400 FBB NNTP server at %s - No free channel!\r\n", mypath);
			if (write (new, buf, strlen (buf)) != strlen(buf))
				perror("rcv_pop() socket write error");
			close (new);
		}
		else
		{
			int val = 0;
			
			tport[*port].tcan[i].state = NNTP_USER;
			tport[*port].tcan[i].sock = new;
			tport[*port].tcan[i].paclen = (val == 0) ? 250 : val;
			tport[*port].tcan[i].queue = s_free (&tport[*port].tcan[i]);
			tport[*port].tcan[i].timeout = time (NULL) + 120L;


			sprintf (buf, "201 FBB NNTP server ready at %s\r\n", mypath);
			if (write (new, buf, strlen (buf)) != strlen(buf))
				perror("rcv_pop() socket write error");

			val = p_port[*port].pk_t;

			if (tport[*port].tcan[i].state == NNTP_USER)
			{
				int nb;
								
				/* Connection */		
				nb = sprintf (buffer, "(%d) CONNECTED to %s", i, mycall);
				tport[*port].tcan[i].state = NNTP_TRANS;
				*len = nb;
				*cmd = COMMAND;
				*canal = i;
				return (TRUE);
			}
	
			return (FALSE);
		}
	}

	for (i = 1 ; i <= tport[*port].nbcan ; i++)
	{
		/* Next channel for polling */
		++tport[*port].curcan;
		if (tport[*port].curcan > tport[*port].nbcan)
			tport[*port].curcan = 1;

		can = tport[*port].curcan;

		if (tport[*port].tcan[can].lsend)
		{
			int nb = 0;
			dbuf_t *buf = tport[*port].tcan[can].lsend;

/*			for (;;) */
			{
				memcpy(buffer+nb, buf->data, buf->len);
				nb += buf->len;

				tport[*port].tcan[can].lsend = buf->next;

				free (buf->data);
				free (buf);
				/*
				buf = tport[*port].tcan[can].lsend;
				if (buf == NULL)
					break;

				if ((nb + buf->len) > RCV_BUFFER_SIZE)
					break;
				*/
			}
			*len = nb;
			*cmd = DATA;
			*canal = can;
			return (TRUE);
		}

		if ((tport[*port].tcan[can].sock == -1) && (tport[*port].tcan[can].state != DISCONNECT))
		{
			tport[*port].tcan[can].disc_request = 1;
		}

		if (tport[*port].tcan[can].disc_request)
		{
			sprintf (buffer, "(%d) DISCONNECTED fm server", can);
			tport[*port].tcan[can].state = DISCONNECT;
			clear_can (*port, can);
			*len = strlen (buffer);
			*cmd = COMMAND;
			*canal = can;
			return (TRUE);
		}
		
		/* Communication channel */
		res = s_status (&tport[*port].tcan[can]);

		if (res & TIME_EVENT)
		{
			int nb;

			/*
			pop_send(*port, can, "-ERR time-out %s POP3 Server shutdown.\r\n", mycall);
			close (tport[*port].tcan[can].sock);
			clear_can (*port, can);
			*/
			strcpy(buffer, "QUIT\r");
//			nb = pop_process_read(*port, can, cmd, buffer, strlen(buffer));
			nb = pop_quit(*port, can);
			if (nb > 0)
			{
				*len = nb;
				*cmd = COMMAND;
				*canal = can;
				return TRUE;
			}
			return (FALSE);
		}

		if (res & WRITE_EVENT)
		{
			/* Can write to the socket... Unused */
		}

		if (res & EXCEPT_EVENT)
		{
		}

#define LGTCP 1100

		if ((res & QUEUE_EVENT) || (res & READ_EVENT))
		{
			int nb = 0;

			if (tport[*port].tcan[can].sock == -1)
			{
				printf ("read on invalid socket\n");
				return (FALSE);
			}

			/* Allocates buffer if necessary */
			if (tport[*port].tcan[can].lbuf == NULL)
			{
				tport[*port].tcan[can].lbuf = calloc (LGTCP, 1);
				tport[*port].tcan[can].lpos = 0;
				tport[*port].tcan[can].lqueue = 0;
				tport[*port].tcan[can].nb_ret = 0;
			}

			if (res & READ_EVENT)
			{
				int qlen = tport[*port].tcan[can].lqueue;
				
				/* Some room left in buffer ? */
				nb = ((LGTCP - qlen) > 256) ? 256 : LGTCP - qlen;
				if (nb)
				{
					nb = read (tport[*port].tcan[can].sock, buffer, nb);
					if ((nb == 0) || ((nb == -1) && (errno == ENOTCONN)))
					{
/*						tport[*port].tcan[can].disc_request = 1; */
						int nb;
						
						close(tport[*port].tcan[can].sock);
						tport[*port].tcan[can].sock = -1;

						strcpy(buffer, "QUIT\r");
						// nb = pop_process_read(*port, can, cmd, buffer, strlen(buffer));
						nb = pop_quit(*port, can);
			
						if (nb > 0)
						{
							*len = nb;
							*cmd = COMMAND;
							*canal = can;
							return TRUE;
						}
						return (FALSE);
					}
					else if (nb == -1)
					{
						printf ("errno = %d\n", errno);
						perror ("read");
						return (FALSE);
					}
				}
				if (nb > 0)
				{
					nb = pop_process_read(*port, can, cmd, buffer, nb);
					if (nb > 0)
					{
						*len = nb;
						*cmd = COMMAND;
						*canal = can;
						return TRUE;
					}
				}
			}
		}
	}
	return (FALSE);
}

/* Open port */
int opn_pop (int port, int nb)
{
	int i;
	int val;
	int len;
	int ok = TRUE;
	char s[80];
	struct sockaddr_in sock_addr;
	char *ptr;
	int pop_port = 0;
	int smtp_port = 0;
	int nntp_port = 0;

	sprintf (s, "Init PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
	InitText (s);

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = 0;

	/* Test if portname is hex number */
	ptr = p_com[(int) p_port[port].ccom].name;

	if (strcmp (ptr, "0") == 0)
	{
		pop_port = p_com[(int) p_port[port].ccom].port;
	}
	else if (strspn (ptr, ":0123456789abcdefABCDEF") != strlen (ptr))
	{
		/* It may be tcp address. Port number is in port */
		if (inet_aton (ptr, &sock_addr.sin_addr))
			pop_port = p_com[(int) p_port[port].ccom].port;
		else
			pop_port = p_com[(int) p_port[port].ccom].cbase;
	}
	else
	{
		sscanf (p_com[(int) p_port[port].ccom].name, "%x:%x:%x", &pop_port, &smtp_port, &nntp_port);
	}

	sprintf (s, "Init PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
	InitText (s);

	tport[port].tcan = (tcan_t *)calloc(nb+1, sizeof(tcan_t));
	if (tport[port].tcan == NULL)
		return 0;
		
	tport[port].pop_auth = POP_AUTH_USER;
	tport[port].smtp_auth = SMTP_AUTH_NO; // SMTP_AUTH_LOGIN|SMTP_AUTH_PLAIN; // auth_login does seem to work okay.  It would prevent Spammers from sending messages.  NOTE: The code currently does work, and  you can specify a command in inittnc*.sys.  Just the default is now set to no password.
	tport[port].curcan = 1;
	tport[port].nbcan = nb;
	for (i = 0 ; i <= nb ; i++)
		clear_can(port, i);

	/* Socket for receiving calls. */
	if (tport[port].pop_fd == 0)
	{

		sprintf (s, "Open PORT %d COM%d-%d",
				 port, p_port[port].ccom, p_port[port].ccanal);
		InitText (s);
		sleep (1);

		if (pop_port)
		{
			/* POP socket */

			sock_addr.sin_port = htons (pop_port);

			if ((tport[port].pop_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
			{
				perror ("socket_r");
				return (0);
			}

			val = 1;
			len = sizeof (val);
			if (setsockopt (tport[port].pop_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, len) == -1)
			{
				perror ("opn_pop : setsockopt SO_REUSEADDR");
			}

			if (bind (tport[port].pop_fd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) != 0)
			{
				perror ("opn_pop : bind");
				close (tport[port].pop_fd);
				tport[port].pop_fd = -1;
				return (0);
			}

			if (listen (tport[port].pop_fd, SOMAXCONN) == -1)
			{
				perror ("listen");
				close (tport[port].pop_fd);
				tport[port].pop_fd = -1;
				return (0);
			}
		}

		if (nntp_port)
		{
			/* NNTP socket */

			sock_addr.sin_port = htons (nntp_port);

			if ((tport[port].nntp_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
			{
				perror ("socket_r");
				return (0);
			}

			val = 1;
			len = sizeof (val);
			if (setsockopt (tport[port].nntp_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, len) == -1)
			{
				perror ("opn_pop : setsockopt SO_REUSEADDR");
			}

			if (bind (tport[port].nntp_fd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) != 0)
			{
				perror ("opn_pop : bind");
				close (tport[port].nntp_fd);
				tport[port].nntp_fd = -1;
				return (0);
			}

			if (listen (tport[port].nntp_fd, SOMAXCONN) == -1)
			{
				perror ("listen");
				close (tport[port].nntp_fd);
				tport[port].nntp_fd = -1;
				return (0);
			}
		}
		
		if (smtp_port)
		{
			/* SMTP socket */

			sock_addr.sin_port = htons (smtp_port);

			if ((tport[port].smtp_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
			{
				perror ("socket_r");
				return (0);
			}

			val = 1;
			len = sizeof (val);
			if (setsockopt (tport[port].smtp_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, len) == -1)
			{
				perror ("opn_pop : setsockopt SO_REUSEADDR");
			}

			if (bind (tport[port].smtp_fd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) != 0)
			{
				perror ("opn_pop : bind");
				close (tport[port].smtp_fd);
				tport[port].smtp_fd = -1;
				return (0);
			}

			if (listen (tport[port].smtp_fd, SOMAXCONN) == -1)
			{
				perror ("listen");
				close (tport[port].smtp_fd);
				tport[port].smtp_fd = -1;
				return (0);
			}
		}
		
		memset (&tport[port].tcan[0], 0, sizeof (tcan_t));
	}
	
	sprintf (s, "Prog PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
	InitText (s);

	return (ok);
}

/* Close port */
int cls_pop (int port)
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

	if (tport[port].pop_fd)
	{
		close (tport[port].pop_fd);
		tport[port].pop_fd = 0;
	}

	if (tport[port].smtp_fd)
	{
		close (tport[port].smtp_fd);
		tport[port].smtp_fd = 0;
	}

	free(tport[port].tcan);
	
	return (1);
}

int sta_pop (int port, int canal, int cmd, void *ptr)
{
	switch (cmd)
	{
	case TNCSTAT:
		return (pop_stat (port, canal, (stat_ch *) ptr));
	case PACLEN:
		*((int *) ptr) = pop_paclen (port, canal);
		return (1);
	case PORTCMD:
		return (pop_ini (port, canal, (char *) ptr));
	case SNDCMD:
		return (pop_cmd (port, canal, (char *) ptr));
	case SETBUSY:
		return stop_cnx (port);
	}
	return 0;
}

/********************************************************************/

static void free_address(int port, int can)
{
	taddr_t *head;
	taddr_t *curr;

	head = tport[port].tcan[can].mail_from;
	while (head)
	{
		curr = head;
		head = head->next;
		if (curr->address)
			free(curr->address);
		free(curr);
	}

	head = tport[port].tcan[can].rcpt_to;
	while (head)
	{
		curr = head;
		head = head->next;
		if (curr->address)
			free(curr->address);
		free(curr);
	}
	
	head = tport[port].tcan[can].content;
	while (head)
	{
		curr = head;
		head = head->next;
		if (curr->address)
			free(curr->address);
		free(curr);
	}
	
	tport[port].tcan[can].rcpt_to = NULL;
	tport[port].tcan[can].mail_from = NULL;
	tport[port].tcan[can].content = NULL;
}

static int pop_quit(int port, int can) // The old code of sending QUIT caused crashes, and seemed unnecessarily complicated.  Let's just force the quit routine directly
{
	char buffer[6];
	strcpy(buffer, "QUIT\r");
	switch (tport[port].tcan[can].state)
	{
		case NNTP_USER:
		case NNTP_TRANS:
		case NNTP_POSTMSG:
			nntp_cmd_quit(port, can, buffer);
			break;
			
		case SMTP_START:
		case SMTP_USER:
		case SMTP_MD5:
		case SMTP_PASS:
		case SMTP_TRANS:
		case SMTP_MSG:
			smtp_cmd_quit(port, can, buffer);
			break;

		case POP_USER:
		case POP_PASS:
		case POP_TRANS:
			pop_cmd_quit(port, can, buffer);
			break;

		default:
			break;
	}
	return 0;
}


static int pop_process_read(int port, int can, int *cmd, char *buffer, int nb)
{
	int i;
	int pos;
	char *ptr;

	buffer[nb] = '\0';

	pos = tport[port].tcan[can].lpos + tport[port].tcan[can].lqueue;
	if (pos >= LGTCP)
		pos -= LGTCP;

	ptr = tport[port].tcan[can].lbuf;
	
	for (i = 0; i < nb; i++)
	{
		if (tport[port].tcan[can].lqueue > (LGTCP - 10))
		{
			++tport[port].tcan[can].nb_ret;
			break;
		}

		ptr[pos] = buffer[i];
		if (++pos == LGTCP)
			pos = 0;

		++tport[port].tcan[can].lqueue;

		if (buffer[i] == '\r')
		{
			++tport[port].tcan[can].nb_ret;
		}
	}

	while (tport[port].tcan[can].nb_ret > 0)
	{
		/*		
		nb = tport[port].tcan[can].lqueue;
		if (tport[port].tcan[can].nb_ret <= 0)
			break;
		*/

		tport[port].tcan[can].timeout = time (NULL) + 120L;

		switch (tport[port].tcan[can].state)
		{
		case NNTP_USER:
		case NNTP_TRANS:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			if (strncmpi(buffer, "QUIT", 4) == 0)
			{
				nntp_cmd_quit(port, can, buffer);
			}
			else if (strncmpi(buffer, "ARTICLE", 7) == 0)
			{
				nntp_cmd_article(port, can, buffer);
			}
			else if (strncmpi(buffer, "BODY", 4) == 0)
			{
				nntp_cmd_body(port, can, buffer);
			}
			else if (strncmpi(buffer, "GROUP", 5) == 0)
			{
				nntp_cmd_group(port, can, buffer);
			}
			else if (strncmpi(buffer, "HEAD", 4) == 0)
			{
				nntp_cmd_head(port, can, buffer);
			}
			else if (strncmpi(buffer, "HELP", 4) == 0)
			{
				nntp_cmd_help(port, can, buffer);
			}
			else if (strncmpi(buffer, "IHAVE", 5) == 0)
			{
				nntp_cmd_ihave(port, can, buffer);
			}
			else if (strncmpi(buffer, "LAST", 4) == 0)
			{
				nntp_cmd_last(port, can, buffer);
			}
			else if (strncmpi(buffer, "LIST", 4) == 0)
			{
				nntp_cmd_list(port, can, buffer);
			}
			else if (strncmpi(buffer, "MODE", 4) == 0)
			{
				nntp_cmd_mode(port, can, buffer);
			}
			else if (strncmpi(buffer, "NEWGROUPS", 9) == 0)
			{
				nntp_cmd_newgroups(port, can, buffer);
			}
			else if (strncmpi(buffer, "NEWNEWS", 7) == 0)
			{
				nntp_cmd_newnews(port, can, buffer);
			}
			else if (strncmpi(buffer, "NEXT", 4) == 0)
			{
				nntp_cmd_next(port, can, buffer);
			}
			else if (strncmpi(buffer, "POST", 4) == 0)
			{
				nntp_cmd_post(port, can, buffer);
			}
			else if (strncmpi(buffer, "SLAVE", 5) == 0)
			{
				nntp_cmd_slave(port, can, buffer);
			}
			else if (strncmpi(buffer, "STAT", 4) == 0)
			{
				nntp_cmd_stat(port, can, buffer);
			}
			else if (strncmpi(buffer, "XHDR", 4) == 0)
			{
				nntp_cmd_xhdr(port, can, buffer);
			}
			else if (strncmpi(buffer, "XOVER", 5) == 0)
			{
				nntp_cmd_xover(port, can, buffer);
			}
			else if (strncmpi(buffer, "AUTHINFO", 8) == 0)
			{
				nntp_cmd_authinfo(port, can, buffer);
			}
				else if (*buffer != '\0')
			{
				pop_send(port, can, "500 Unknown command\r\n");
			}
			break;
		case NNTP_POSTMSG:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			nntp_rcv_dt(port, can, buffer, strlen(buffer));
			break;

			
		case SMTP_START:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			if (strncmpi(buffer, "QUIT", 4) == 0)
			{
				smtp_cmd_quit(port, can, buffer);
			}
			else if (strncmpi(buffer, "EHLO", 4) == 0)
			{
				return smtp_cmd_ehlo(port, can, buffer);
			}
			else if (strncmpi(buffer, "HELO", 4) == 0)
			{
				return smtp_cmd_helo(port, can, buffer);
			}
			else if (*buffer != '\0')
			{
				pop_send(port, can, "502 Unknown command\r\n",INVALID_CMD);
			}
			break;
			
		case SMTP_USER:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			smtp_cmd_user(port, can, buffer);
			break;
			
		case SMTP_MD5:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);
			smtp_cmd_md5(port, can, buffer);
			break;
			
		case SMTP_PASS:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);
			smtp_cmd_pass(port, can, buffer, 1);
			break;
			
		case SMTP_TRANS:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			if (strncmpi(buffer, "QUIT", 4) == 0)
			{
				smtp_cmd_quit(port, can, buffer);
			}
			else if (strncmpi(buffer, "AUTH", 4) == 0)
			{
				return smtp_cmd_auth(port, can, buffer);
			}
			else if (strncmpi(buffer, "MAIL FROM:", 10) == 0)
			{
				smtp_cmd_mail(port, can, buffer);
			}
			else if (strncmpi(buffer, "RCPT TO:", 8) == 0)
			{
				smtp_cmd_rcpt(port, can, buffer);
			}
			else if (strncmpi(buffer, "NOOP", 4) == 0)
			{
				smtp_cmd_noop(port, can, buffer);
			}
			else if (strncmpi(buffer, "RSET", 4) == 0)
			{
				smtp_cmd_rset(port, can, buffer);
			}
			else if (strncmpi(buffer, "DATA", 4) == 0)
			{
				smtp_cmd_data(port, can, buffer);
			}
			else if (strncmpi(buffer, "VRFY", 4) == 0)
			{
				smtp_cmd_vrfy(port, can, buffer);
			}
			else if (*buffer != '\0')
			{
				pop_send(port, can, "502 Unknown command\r\n",INVALID_CMD);
			}
			break;

		case SMTP_MSG:
			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			smtp_rcv_dt(port, can, buffer, strlen(buffer));
			break;

		case POP_USER:

			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			if (strncmpi(buffer, "QUIT", 4) == 0)
			{
				pop_cmd_quit(port, can, buffer);
			}
			else if ((tport[port].pop_auth & POP_AUTH_USER) && (strncmpi(buffer, "USER", 4) == 0))
			{
				pop_cmd_user(port, can, buffer);
			}
			else if ((tport[port].pop_auth & POP_AUTH_APOP) && (strncmpi(buffer, "APOP", 4) == 0))
			{
				return pop_cmd_apop(port, can, buffer);
			}
			else if (*buffer != '\0')
			{
				pop_send(port, can, "%s  USER, APOP  or  QUIT\r\n",INVALID_CMD);
			}
			break;

		case POP_PASS:

			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);
			if (strncmpi(buffer, "QUIT", 4) == 0)
			{
				pop_cmd_quit(port, can, buffer);
			}
			else if (strncmpi(buffer, "PASS", 4) == 0)
			{
				return pop_cmd_pass(port, can, buffer);
			}
			else if (*buffer != '\0')
			{
				pop_send(port, can, "%s  PASS,  QUIT\r\n",INVALID_CMD);
			}
			break;

		case POP_TRANS:

			if (!pop_getline (port, can, buffer))
				break;

			sup_ln (buffer);

			if (strncmpi(buffer, "QUIT", 4) == 0)
			{
				pop_cmd_quit(port, can, buffer);
			}
			else if (strncmpi(buffer, "LIST", 4) == 0)
			{
				pop_cmd_list(port, can, buffer);
			}
			else if (strncmpi(buffer, "LAST", 4) == 0)
			{
				pop_cmd_last(port, can, buffer);
			}
			else if (strncmpi(buffer, "DELE", 4) == 0)
			{
				pop_cmd_dele(port, can, buffer);
			}
			else if (strncmpi(buffer, "NOOP", 4) == 0)
			{
				pop_cmd_noop(port, can, buffer);
			}
			else if (strncmpi(buffer, "RETR", 4) == 0)
			{
				pop_cmd_retr(port, can, buffer);
			}
			else if (strncmpi(buffer, "RSET", 4) == 0)
			{
				pop_cmd_rset(port, can, buffer);
			}
			else if (strncmpi(buffer, "STAT", 4) == 0)
			{
				pop_cmd_stat(port, can, buffer);
			}
			else if (strncmpi(buffer, "TOP", 3) == 0)
			{
				pop_cmd_top(port, can, buffer);
			}
			else if (strncmpi(buffer, "UIDL", 4) == 0)
			{
				pop_cmd_uidl(port, can, buffer);
			}
			else if (*buffer != '\0')
			{
				pop_send(port, can, "%s  DELE, LAST, LIST, NOOP, RETR, RSET, STAT, TOP, UIDL  or  QUIT\r\n",INVALID_CMD);
			}
			break;

		default:
			close (tport[port].tcan[can].sock);
			tport[port].tcan[can].sock = -1;
			tport[port].tcan[can].lqueue = 0;
			tport[port].tcan[can].nb_ret = 0;
			break;
		}
	}
	return (0);
}

static int stop_cnx (int port)
{
	if (tport[port].pop_fd)
	{
		close(tport[port].pop_fd);
		tport[port].pop_fd = 0;
	}
	if (tport[port].smtp_fd)
	{
		close(tport[port].smtp_fd);
		tport[port].smtp_fd = 0;
	}
	return 1;
}

static int pop_check_call (int port, int can, char *callsign, struct sockaddr_in *address)
{
	int res = 0;

	tport[port].tcan[can].callsign.num = extind (callsign, tport[port].tcan[can].callsign.call);
	if (find (tport[port].tcan[can].callsign.call))
	{
		if (chercoord (tport[port].tcan[can].callsign.call) != 0xffff)
			res = 1;
		else
			res = 2;
	}

	/* Authorized address ? - To be written... */
	if (address)
	{
	}

	return (res);
}

static int pop_check_pass (int port, int can, char *passwd)
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

		if (strcmpi (passwd, frec.pass) == 0)
		{
			return (TRUE);
		}
	}

	return (FALSE);
}

static int pop_check_md5 (int port, int can, char *passwd)
{
	unsigned record;

	record = chercoord (tport[port].tcan[can].callsign.call);

	if (record != 0xffff)
	{
		uchar source[300];
		uchar dest[80];
		uchar pass[80];
		FILE *fptr;
		info frec;

		fptr = ouvre_nomenc ();
		fseek (fptr, ((long) record * sizeof (info)), 0);
		fread (&frec, sizeof (info), 1, fptr);
		ferme (fptr, 92);
		
		strcpy(pass, frec.pass);

		/* try with recorded password */
		strcpy(source, tport[port].tcan[can].md5string);
		strcat(source, pass);
		MD5String (dest, source);
		if (strcmpi (passwd, dest) == 0)
		{
			return (TRUE);
		}

		/* try with password in lower case */
		strcpy(source, tport[port].tcan[can].md5string);
		strlwr(pass);
		strcat(source, pass);
		MD5String (dest, source);
		if (strcmpi (passwd, dest) == 0)
		{
			return (TRUE);
		}
	}

	return (FALSE);
}

static int pop_snd_dt (int port, int canal, char *buffer, int len)
{
	int i;
	int cr;
	int head;
	int nb_lines;
/*	int num; */
	char *ptr;
	char buf[600];

	if (tport[port].tcan[canal].sock == -1)
		return (FALSE);

	if (tport[port].tcan[canal].state == NNTP_LINE)
	{
		ptr = strchr(buffer, '\r');
		if (ptr)
			*ptr = '\0';
		pop_send(port, canal, "%s\r\n", buffer);
		tport[port].tcan[canal].state = NNTP_TRANS;
	}
	else if (tport[port].tcan[canal].state == NNTP_MSG)
	{
		cr = tport[port].tcan[canal].cr;
		head = tport[port].tcan[canal].head;
		nb_lines = tport[port].tcan[canal].nb_lines;

		for (ptr = buf, i = 0; i < len; i++)
		{
			if (buffer[i] == '\033')
			{
				pop_send(port, canal, ".\r\n");
				tport[port].tcan[canal].state = NNTP_TRANS;
				return 1;
			}

			if (buffer[i] == '\032')
			{
				tport[port].tcan[canal].state = NNTP_TRANS;
				return 1;
			}

			if (cr && buffer[i] == '.')
				*ptr++ = '.';

			*ptr++ = buffer[i];
			if (buffer[i] == '\r')
			{
				*ptr++ = '\n';
				*ptr = '\0';

				if (head == 0 && nb_lines != 1)
				{
					/* Mode = 2 : No headers */
					pop_send(port, canal, "%s", buf);
				}

				if (head == 1 && nb_lines != 2)
				{
					/* Mode = 1 : Only headers */
					pop_send(port, canal, "%s", buf);
				}

				if (cr)
				{
					/* Empty line -> End of headers */
					head = 0;
				}

				ptr = buf;
				cr = 1;			

			}
			else
			{
				cr = 0;
			}
		}

		*ptr = '\0';
		if (*buf)
			pop_send(port, canal, "%s", buf);

		tport[port].tcan[canal].cr = cr;
		tport[port].tcan[canal].head = head;
	}
	else if (tport[port].tcan[canal].state == POP_MSG)
	{
		cr = tport[port].tcan[canal].cr;
		head = tport[port].tcan[canal].head;
/*		num = tport[port].tcan[canal].mess_cur;*/
		nb_lines = tport[port].tcan[canal].nb_lines;

		for (ptr = buf, i = 0; i < len; i++)
		{
			if (buffer[i] == '\033')
			{
				pop_send(port, canal, ".\r\n");
				tport[port].tcan[canal].state = POP_TRANS;
				return 1;
			}

			if (cr && buffer[i] == '.')
				*ptr++ = '.';

			*ptr++ = buffer[i];
			if (buffer[i] == '\r')
			{
				if (cr)
				{
					/* Empty line -> End of headers */
					head = 0;
				}

				*ptr++ = '\n';
				*ptr = '\0';
				pop_send(port, canal, "%s", buf);
				ptr = buf;
				cr = 1;			

				if (head == 0 && nb_lines-- == 0)
				{
					pop_send(port, canal, ".\r\n");
					tport[port].tcan[canal].state = POP_TRANS;
					return 1;
				}
			}
			else
			{
				cr = 0;
			}
		}

		*ptr = '\0';
		if (*buf)
			pop_send(port, canal, "%s", buf);

		tport[port].tcan[canal].cr = cr;
		tport[port].tcan[canal].head = head;
		tport[port].tcan[canal].nb_lines = nb_lines;
	}
	
	return 1;
}

static int pop_send(int port, int canal, char *fmt, ...)
{
	char *buf; //  Size was 1200. This was crashing.  
	va_list argptr;

	if (tport[port].tcan[canal].sock != -1)
	{
		int len;
		va_start (argptr, fmt);
		len = vsnprintf(0, 0, fmt, argptr); // Return the length of the string
		va_end (argptr);
		if (len < 0)return 0; 
		buf = malloc(len+1);
		va_start (argptr, fmt);
		vsnprintf (buf,len+1, fmt, argptr);
		va_end (argptr);
		//if (1) I think it's best not to touch encoding.  Firefox can read and send 850 if needed.  This conversion just causes things to break
		//		ibm_to_ansi(buf, strlen(buf));
				
		if (write (tport[port].tcan[canal].sock, buf, strlen (buf)) != strlen (buf))
			perror("pop_send() socket write error");
		free(buf);	
		return 1;
	}
	return 0;
}

static int pop_ini (int port, int canal, char *cmd)
{
	char *ptr;
	
	ptr = strtok(cmd, " \t");
	if (ptr == NULL)
		return 0;
	
	switch (*ptr)
	{
	case 'S':
		/* SMTP configuration */
		ptr = strtok(NULL, " \t");
		if (strcmpi(ptr, "AUTH") == 0)
		{
			tport[port].smtp_auth = 0;
			while ((ptr = strtok(NULL, " \t")) != NULL)
			{
				if (strcmpi(ptr, "NO") == 0)
					tport[port].smtp_auth |= SMTP_AUTH_NO;
				else if (strcmpi(ptr, "LOGIN") == 0)
					tport[port].smtp_auth |= SMTP_AUTH_LOGIN;
				else if (strcmpi(ptr, "PLAIN") == 0)
					tport[port].smtp_auth |= SMTP_AUTH_PLAIN;
				else if (strcmpi(ptr, "CRAM-MD5") == 0)
					tport[port].smtp_auth |= SMTP_AUTH_CRMD5;
			}
			if (tport[port].smtp_auth == 0)
				tport[port].smtp_auth = SMTP_AUTH_NO;
		}
		break;
	case 'P':
		/* POP configuration */
		ptr = strtok(NULL, " \t");
		if (strcmpi(ptr, "AUTH") == 0)
		{
			tport[port].pop_auth = 0;
			while ((ptr = strtok(NULL, " \t")) != NULL)
			{
				if (strcmpi(ptr, "USER") == 0)
					tport[port].pop_auth |= POP_AUTH_USER;
				else if (strcmpi(ptr, "APOP") == 0)
					tport[port].pop_auth |= POP_AUTH_APOP;
			}
			if (tport[port].pop_auth == 0)
				tport[port].pop_auth |= POP_AUTH_USER;
		}
		break;
	}
	return (0);
}

static int pop_cmd (int port, int canal, char *cmd)
{
	int i;
	char status;
	long nb, size;
	
	switch (*cmd++)
	{
	case 'D':
		close (tport[port].tcan[canal].sock);
		tport[port].tcan[canal].sock = -1;
		break;
	case 'S':
		/* Message information */
		sscanf(cmd, "%ld", &nb);
		tport[port].tcan[canal].mess_nb = nb;
		tport[port].tcan[canal].mess_cur = 0;
		tport[port].tcan[canal].mess_tot = 0L;
		tport[port].tcan[canal].mess = malloc(sizeof(tmess_t) * nb);
		break;
	case 'M':
		if (tport[port].tcan[canal].state != POP_TRANS)
			break;
			
		/* Message list */
		nb = 0;
		sscanf(cmd, "%ld %ld %c", &nb, &size, &status);
		
		if (nb == 0)
		{
			/* End of list */
			pop_send(port, canal, "+OK connected to %s BBS %d messages (%ld bytes)\r\n", 
					tport[port].tcan[canal].callsign.call,
					tport[port].tcan[canal].mess_nb,
					tport[port].tcan[canal].mess_tot);
		}
		
		i = tport[port].tcan[canal].mess_cur;

		if (i < tport[port].tcan[canal].mess_nb)
		{
			tport[port].tcan[canal].mess_tot += size;
			tport[port].tcan[canal].mess[i].mess_num = nb;
			tport[port].tcan[canal].mess[i].mess_size = size;
			tport[port].tcan[canal].mess[i].mess_stat = status;
			tport[port].tcan[canal].mess[i].mess_del = 0;
			tport[port].tcan[canal].mess_cur = i+1;
		}
		break;
	}
	return (0);
}

static int pop_stat (int port, int canal, stat_ch * ptr)
{
	int val;

	if ((canal == 0) || (tport[port].tcan[canal].sock == -1))
		return (0);

	ptr->mem = 100;

	val = s_free (&tport[port].tcan[canal]);

	ptr->ack = 0;
	
	if (tport[port].tcan[canal].state == POP_MSG
		|| tport[port].tcan[canal].state == SMTP_MSG
		|| tport[port].tcan[canal].state == NNTP_MSG
		|| tport[port].tcan[canal].state == NNTP_POSTMSG)
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

	if (can->sock == -1)
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
			res |= WRITE_EVENT;
		}
		if (FD_ISSET (can->sock, &tcp_excep))
		{
			res |= EXCEPT_EVENT;
		}
	}
	return (res);
}

/* Copies a line in the buffer. */
static int pop_getline (int port, int can, char *buffer)
{
	int i = 0;
	int c;
	int pos;
	char *ptr;

	pos = tport[port].tcan[can].lpos;
	ptr = tport[port].tcan[can].lbuf;

	while (tport[port].tcan[can].lqueue > 0)
	{
		c = ptr[pos];
		if (++pos == LGTCP)
			pos = 0;

		--tport[port].tcan[can].lqueue;

		if (c != '\n')
			buffer[i++] = c;

		if (c == '\r')
		{
			--tport[port].tcan[can].nb_ret;
			break;
		}
	}

	buffer[i] = '\0';

	tport[port].tcan[can].lpos = pos;
	
	if (tport[port].tcan[can].lqueue == 0)
		tport[port].tcan[can].nb_ret = 0;

	return (i);
}

static void free_lsend(int port, int canal)
{
	dbuf_t *buf;
	
	while ((buf = tport[port].tcan[canal].lsend) != NULL)
	{
		tport[port].tcan[canal].lsend = buf->next;
		free(buf->data);
		free(buf);
	}
}

static void free_msgbuf(int port, int canal)
{
	dbuf_t *buf;
	
	while ((buf = tport[port].tcan[canal].msgbuf) != NULL)
	{
		tport[port].tcan[canal].msgbuf = buf->next;
		free(buf->data);
		free(buf);
	}
}

static int pop_to_bbs(int port, int canal, char *buf, int clean)
{
	dbuf_t *cur;
	dbuf_t *sbuf;
	
	if (clean)
		free_lsend(port, canal);
	
	cur = malloc(sizeof(dbuf_t));
	if (cur == NULL)
		return 0;
		
	cur->len = strlen(buf);
	cur->data = strdup(buf);
	cur->next = NULL;
	
	sbuf = tport[port].tcan[canal].lsend;

	if (sbuf)
	{
		/* Append to last buffer */
		while (sbuf->next != NULL)
			sbuf = sbuf->next;

		sbuf->next = cur;		
	}
	else
	{
		tport[port].tcan[canal].lsend = cur;
	}
	return 1;
}

static int pop_delete(int port, int canal)
{
	int i;
	int nb = 0;
	char num[80];
	char buf[80];
	
	if (tport[port].tcan[canal].disc_request)
		return 0;
		
	strcpy(buf, "K");
	for (i = 0 ; i < tport[port].tcan[canal].mess_nb ; i++)
	{
		if (tport[port].tcan[canal].mess[i].mess_del)
		{
			sprintf(num, " %ld", tport[port].tcan[canal].mess[i].mess_num);
			strcat(buf, num);
			if ((++nb % 4) == 0)
			{
				strcat(buf, "\r");
				pop_to_bbs(port, canal, buf, 0);
				strcpy(buf, "K");
				nb = 0;
			}
		}
	}

	if (nb > 0)
	{
		strcat(buf, "\r");
		pop_to_bbs(port, canal, buf, 0);
	}

	return 1;
}

static int pop_paclen (int port, int canal)
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
	free_address(port, canal);
	if (tport[port].tcan[canal].lbuf)
		free (tport[port].tcan[canal].lbuf);
	if (tport[port].tcan[canal].lsend)
		free_lsend(port, canal);
	if (tport[port].tcan[canal].msgbuf)
		free_msgbuf(port, canal);
	if (tport[port].tcan[canal].mess)
		free(tport[port].tcan[canal].mess);
	memset (&tport[port].tcan[canal], 0, sizeof (tcan_t));
	tport[port].tcan[canal].sock = -1;
	tport[port].tcan[canal].state = DISCONNECT;
}

/*
 * POP commands processing **
 */

/* Check the given # msg in a pop command */
static int check_num (int port, int can, int num)
{
	if (num < 1 || num > tport[port].tcan[can].mess_nb)
	{
		pop_send(port, can, "-ERR invalid message; number out of range.\r\n");
		return 0;
	}
	
	if (tport[port].tcan[can].mess[num-1].mess_del)
	{
		pop_send(port, can, "-ERR message %d has been marked for deletion.\r\n", num);
		return 0;
	}
	
	return 1;
}

static int pop_cmd_quit(int port, int can, char *buffer)
{
	if (tport[port].tcan[can].quit_request)
		return 0;
		
	tport[port].tcan[can].quit_request = 1;
	
	/* Received "QUIT" : disconnect */
	if (tport[port].tcan[can].state != POP_USER)
	{
		pop_delete(port, can);
		pop_to_bbs(port, can, "B\r", 0);
	}
	else
	{
		close (tport[port].tcan[can].sock);
		tport[port].tcan[can].sock = -1;
	}
	pop_send(port, can, "+OK %s POP3 Server shutdown.\r\n", mycall);
	return 0;
}

static int pop_cmd_list (int port, int can, char *buffer)
{
	int i;
	int nb;
	char *ptr = buffer + 4;

	while (isspace(*ptr))
		++ptr;

	if (*ptr)
	{
		nb = atoi(ptr);
		if (check_num(port, can, nb))
			pop_send(port, can, "+OK %d %ld\r\n", nb, tport[port].tcan[can].mess[nb-1].mess_size);
		return 1;
	}
	
	pop_send(port, can, "+OK %d messages (%ld bytes).\r\n",
				tport[port].tcan[can].mess_nb,
				tport[port].tcan[can].mess_tot);
	for (i = 0 ; i < tport[port].tcan[can].mess_nb ; i++)
	{
		if (tport[port].tcan[can].mess[i].mess_del == 0)
			pop_send(port, can, "%ld %ld \r\n", i+1, tport[port].tcan[can].mess[i].mess_size);
	}
	pop_send(port, can, ".\r\n");
	return 0;
}

static int pop_cmd_uidl (int port, int can, char *buffer)
{
	int i;
	int nb;
	char *ptr = buffer + 4;

	while (isspace(*ptr))
		++ptr;

	if (*ptr)
	{
		nb = atoi(ptr);
		if (check_num(port, can, nb))
			pop_send(port, can, "+OK %d %s\r\n", nb, xuidl(tport[port].tcan[can].mess[nb-1].mess_num, mycall));
		return 1;
	}
	
	pop_send(port, can, "+OK uidl command accepted.\r\n");
	for (i = 0 ; i < tport[port].tcan[can].mess_nb ; i++)
	{
		if (tport[port].tcan[can].mess[i].mess_del == 0)
			pop_send(port, can, "%ld %s\r\n", i+1, xuidl(tport[port].tcan[can].mess[i].mess_num, mycall));
	}
	pop_send(port, can, ".\r\n");
	return 0;
}

static int pop_cmd_user (int port, int can, char *buffer)
{
	char *ptr = buffer + 4;

	while (isspace(*ptr))
		++ptr;

	strn_cpy(6, tport[port].tcan[can].callsign.call, ptr);
	tport[port].tcan[can].state = POP_PASS;

	pop_send(port, can, "+OK please send PASS command\r\n");
	return 0;
}

static int pop_cmd_pass (int port, int can, char *buffer)
{
	int ok = 0;
	char *ptr = buffer + 4;
	char *callsign = tport[port].tcan[can].callsign.call;
	unsigned addr_len;
	struct sockaddr_in *address;
	struct sockaddr_in sock_addr;

	while (isspace(*ptr))
		++ptr;

	addr_len = sizeof (sock_addr);
	if ((getpeername(tport[port].tcan[can].sock, (struct sockaddr *)&sock_addr, &addr_len) == 0) && (sock_addr.sin_family == AF_INET))
		address = &sock_addr;
	else
		address = NULL;

	// Check login
	switch (pop_check_call (port, can, callsign, address))
	{
	case 1:				
		// Check password
		if (pop_check_pass (port, can, ptr))
		{
			sprintf (buffer, "(%d) CONNECTED to %s-%d",
					 can, tport[port].tcan[can].callsign.call,
					 tport[port].tcan[can].callsign.num);
			tport[port].tcan[can].state = POP_TRANS;
			tport[port].tcan[can].nb_try = 0;
			tport[port].tcan[can].timeout = time (NULL) + 120L;
			return (strlen (buffer));
		}
		break;
	default:
		break;
	}
	if (ok == 0)
	{
		pop_send(port, can, "-ERR invalid usercode or password, please try later\r\n");
		close (tport[port].tcan[can].sock);
		tport[port].tcan[can].sock = -1;
	}
	return 0;
}

static int pop_cmd_apop (int port, int can, char *buffer)
{
	int ok = 0;
	char *ptr = buffer + 4;
	char *callsign;
	unsigned addr_len;
	struct sockaddr_in *address;
	struct sockaddr_in sock_addr;

	while (isspace(*ptr))
		++ptr;
		
	callsign = ptr;

	while (*ptr && !isspace(*ptr))
		++ptr;
	
	if (*ptr == '\0')
	{
		pop_send(port, can, "-ERR password missing\r\n");
		close (tport[port].tcan[can].sock);
		tport[port].tcan[can].sock = -1;
		return 0;
	}

	*ptr++ = '\0';
	while (isspace(*ptr))
		++ptr;	

	addr_len = sizeof (sock_addr);
	if ((getpeername(tport[port].tcan[can].sock, (struct sockaddr *)&sock_addr, &addr_len) == 0) && (sock_addr.sin_family == AF_INET))
		address = &sock_addr;
	else
		address = NULL;

	// Check login
	switch (pop_check_call (port, can, callsign, address))
	{
	case 1:				
		// Check password
		if (pop_check_md5 (port, can, ptr))
		{
			sprintf (buffer, "(%d) CONNECTED to %s-%d",
					 can, tport[port].tcan[can].callsign.call,
					 tport[port].tcan[can].callsign.num);
			tport[port].tcan[can].state = POP_TRANS;
			tport[port].tcan[can].nb_try = 0;
			tport[port].tcan[can].timeout = time (NULL) + 120L;
			return (strlen (buffer));
		}
		break;
	}
	if (ok == 0)
	{
		pop_send(port, can, "-ERR invalid usercode or password, please try later\r\n");
		close (tport[port].tcan[can].sock);
		tport[port].tcan[can].sock = -1;
	}
	return 0;
}

static int pop_cmd_last (int port, int can, char *buffer)
{
	int i, max = 0;

	for (i = 0 ; i < tport[port].tcan[can].mess_nb ; i++)
		if (tport[port].tcan[can].mess[i].mess_del == 0)
			if (tport[port].tcan[can].mess[i].mess_stat != 'N')
				max = i+1;
	pop_send(port, can, "+OK %d\r\n", max);
	return 0;
}

static int pop_cmd_dele (int port, int can, char *buffer)
{
	char *ptr = buffer + 4;

	while (isspace(*ptr))
		++ptr;

	if (*ptr == '\0')
	{
		pop_send(port, can, "-ERR message number required (e.g.  DELE 1)\r\n");
	}
	else
	{
		int nb = atoi(ptr);
		if (nb < 1 || nb > tport[port].tcan[can].mess_nb)
		{
			pop_send(port, can, "-ERR invalid message; number out of range\r\n");
		}
		else
		{
			tport[port].tcan[can].mess[nb-1].mess_del = 1;
			pop_send(port, can, "+OK message %d marked for deletion\r\n", nb);
		}
	}
	return 0;
}

static int pop_cmd_retr (int port, int can, char *buffer)
{
	char *ptr = buffer + 4;

	while (isspace(*ptr))
		++ptr;

	if (*ptr == '\0')
	{
		pop_send(port, can, "-ERR message number required (e.g.  TOP 1 7)\r\n");
	}
	else
	{
		int nb = atoi(ptr);

		if (check_num (port, can, nb))
		{
			char buf[80];

			tport[port].tcan[can].nb_lines = -1;
			tport[port].tcan[can].mess_cur = nb-1;
			tport[port].tcan[can].cr = 1;
			tport[port].tcan[can].head = 1;

			pop_send(port, can, "+OK message %d (%ld bytes)\r\n", nb, tport[port].tcan[can].mess[nb-1].mess_size);
			tport[port].tcan[can].mess[nb-1].mess_stat = 'Y';
			tport[port].tcan[can].state = POP_MSG;

			sprintf(buf, "R %ld\r", tport[port].tcan[can].mess[nb-1].mess_num);
			pop_to_bbs(port, can, buf, 1);
		}
	}
	return 0;
}

static int pop_cmd_rset (int port, int can, char *buffer)
{
	int i ;
	for (i = 0 ; i < tport[port].tcan[can].mess_nb ; i++)
		tport[port].tcan[can].mess[i].mess_del = 0;
	pop_send(port, can, "+OK %d messages %ld bytes\r\n", tport[port].tcan[can].mess_nb, tport[port].tcan[can].mess_tot);
	return 0;
}

static int pop_cmd_top (int port, int can, char *buffer)
{
	char *ptr = buffer + 3;

	while (isspace(*ptr))
		++ptr;

	if (*ptr == '\0')
	{
		pop_send(port, can, "-ERR message number and line count required (e.g.  TOP 1 7)\r\n");
	}
	else
	{
		int nb = atoi(ptr);
		while (!isspace(*ptr))
			++ptr;
		while (isspace(*ptr))
			++ptr;
		if (*ptr == '\0')
		{
			pop_send(port, can, "-ERR line count required (e.g.  TOP 1 7)\r\n");
		}
		else
		{
			int lines = atoi(ptr);

			if (check_num (port, can, nb))
			{
				char buf[80];

				tport[port].tcan[can].nb_lines = lines;
				tport[port].tcan[can].mess_cur = nb-1;
				tport[port].tcan[can].cr = 1;
				tport[port].tcan[can].head = 1;

				pop_send(port, can, "+OK message %d (%ld bytes)\r\n", nb, tport[port].tcan[can].mess[nb-1].mess_size);
				tport[port].tcan[can].mess[nb-1].mess_stat = 'Y';
				tport[port].tcan[can].state = POP_MSG;

				sprintf(buf, "R %ld\r", tport[port].tcan[can].mess[nb-1].mess_num);
				pop_to_bbs(port, can, buf, 1);
			}
		}
	}
	return 0;
}

static int pop_cmd_stat (int port, int can, char *buffer)
{
	pop_send(port, can, "+OK %d %ld\r\n", tport[port].tcan[can].mess_nb, tport[port].tcan[can].mess_tot);
	return 0;
}

static int pop_cmd_noop (int port, int can, char *buffer)
{
	pop_send(port, can, "+OK\r\n");
	return 0;
}

/*
 * SMTP commands processing **
 */

static int smtp_reply(int port, int can, int next, int num, char *fmt, ...)
{
	int nb;
	char buf[1024];
	va_list argptr;

	if (tport[port].tcan[can].sock != -1)
	{
		va_start (argptr, fmt);
		nb = sprintf(buf, "%d%c", num, (next) ? '-' : ' ');
		vsprintf (buf+nb, fmt, argptr);
		va_end (argptr);

		strcat(buf, "\r\n");
		if (write (tport[port].tcan[can].sock, buf, strlen (buf)) != strlen(buf))
			perror("rcv_pop() socket write error");

		return 1;
	}
	return 0;
}

static int smtp_message(int port, int canal)
{
	taddr_t *dest = tport[port].tcan[canal].rcpt_to;
	char *ptr;
	char str[256];
	char *exped = tport[port].tcan[canal].mail_from->address;
	int ret = 1;
		
	ptr = strchr(exped, '@');
	if (ptr)
		*ptr = '\0';
	
	/* Send the message to all recipients of the list */
	while (dest)
	{
		dbuf_t *buf;
		
		snprintf(str, sizeof(str)-1, "SP %s < %s\r", dest->address, exped);
		str[sizeof(str)-1] = 0;
		if (!pop_to_bbs(port, canal, str, 0))
		{
			ret = 0;
			break;
		}

		buf = tport[port].tcan[canal].msgbuf;
		while (buf)
		{
			if (!pop_to_bbs(port, canal, buf->data, 0))
			{
				ret = 0;
				break;
			}
			
			buf = buf->next;
		}

		if (ret == 0)
			break;

		dest = dest->next;
	}

	free_address(port, canal);
	free_msgbuf(port, canal);

	return ret;
}

static int nntp_message(int port, int canal)
{
	taddr_t *dest = tport[port].tcan[canal].rcpt_to;
	char *ptr;
	char str[256];
	char *exped = tport[port].tcan[canal].mail_from->address;
	int ret = 1;
		
	ptr = strchr(exped, '@');
	if (ptr)
		*ptr = '\0';
	
	/* Send the message to all recipients of the list */
	while (dest)
	{
		dbuf_t *buf;
		
		snprintf(str, sizeof(str)-1, "SB %s < %s\r", dest->address, exped);
		str[sizeof(str)-1] = 0;
		if (!pop_to_bbs(port, canal, str, 0))
		{
			ret = 0;
			break;
		}

		buf = tport[port].tcan[canal].msgbuf;
		while (buf)
		{
			if (!pop_to_bbs(port, canal, buf->data, 0))
			{
				ret = 0;
				break;
			}
			
			buf = buf->next;
		}

		if (ret == 0)
			break;

		dest = dest->next;
	}

	free_address(port, canal);
	free_msgbuf(port, canal);

	return ret;
}


static int add_to_msg(int port, int can, char *buffer)
{
	int lg;
	dbuf_t *cur;
	
	cur = malloc(sizeof(dbuf_t));
	if (cur == NULL)
		return 0;
		
	lg = strlen(buffer);
	cur->len = lg;
	cur->data = strdup(buffer);
	cur->next = NULL;
	
	//if (1) // Remove the encoding change.  Leaving the text untouched allows other encodings to pass through the nntp server intact.  The user can always choose the IBM850 encoding from Thunderbird, if they need to.
	//	ansi_to_ibm(cur->data, lg);

	if (tport[port].tcan[can].msgbuf)
	{
		dbuf_t *buf = tport[port].tcan[can].msgbuf;
		
		/* Append to last buffer */
		while (buf->next)
			buf = buf->next;

		buf->next = cur;		
	}
	else
	{
		tport[port].tcan[can].msgbuf = cur;
	}
	return 1;
}

static int smtp_rcv_dt(int port, int can, char *buffer, int len)
{
	char *ptr = buffer;

	if (tport[port].tcan[can].sock == -1)
		return (FALSE);

	if (tport[port].tcan[can].state != SMTP_MSG)
		return TRUE;

	/* Main message */
	if (*ptr == '.')
	{
		if (len == 1)
		{
			/* End of the message */
			add_to_msg(port, can, "/EX\r");
			tport[port].tcan[can].state = SMTP_TRANS;
			if (smtp_message(port, can))
				smtp_reply(port, can, 0, 250, "Message accepted");
			else
				smtp_reply(port, can, 0, 552, "Message failed");
			return 1;	
		}
		if (ptr[1] == '.')
			++ptr;
	}
	
	if (tport[port].tcan[can].head)
	{
		/* Read the headers */
		
		if (len == 0)
		{
			char str[256];
			taddr_t *dest, *extra;

			extra = tport[port].tcan[can].content;
			if (extra)
			{
				while (extra)
				{
					snprintf(str, sizeof(str)-1, "%s\r", extra->address);
					str[sizeof(str)-1] = 0;
					add_to_msg(port, can, str);
					extra = extra->next;
				}
			}
			else
			{
				snprintf(str, sizeof(str)-1, "SMTP mail received, may be 3rd party mail.\rHeaders:\rFrom: %s\r", tport[port].tcan[can].mail_from->address);
				str[sizeof(str)-1] = 0;
				add_to_msg(port, can, str);
			
				dest = tport[port].tcan[can].rcpt_to;
				while (dest)
				{
					sprintf(str, "To:   %s\r", dest->address);
					add_to_msg(port, can, str);
					dest = dest->next;
				}

			}

			/* End of headers */
			tport[port].tcan[can].head = 0;
			add_to_msg(port, can, "\r");
		}
		else if (tport[port].tcan[can].extra)
		{
				taddr_t *cur = tport[port].tcan[can].content;
				taddr_t *cont = malloc(sizeof(taddr_t));
				cont->address = strdup(ptr);
				cont->next = NULL;

				if (cur)
				{
					/* Add the extra header to the end of list */
					while (cur->next)
						cur = cur->next;
					cur->next = cont;
				}
				tport[port].tcan[can].extra = (ptr[strlen(ptr)-1] == ';');
		}
		else if (strncmpi("content-type:", ptr, 13) == 0)
		{
			if (strstr(ptr, "multipart"))
			{
				taddr_t *cur = tport[port].tcan[can].content;
				taddr_t *cont = malloc(sizeof(taddr_t));
				cont->address = strdup(ptr);
				cont->next = NULL;

				if (cur)
				{
					/* Add the extra header to the end of list */
					while (cur->next)
						cur = cur->next;
					cur->next = cont;
				}
				else
				{
					tport[port].tcan[can].content = cont;
				}
				tport[port].tcan[can].extra = (ptr[strlen(ptr)-1] == ';');
			}
			
		}
		else if (strncmpi("subject:", ptr, 8) == 0)
		{
			/* Add the title */			
			ptr += 8;
			while (isspace (*ptr))
				++ptr;
			
			strcat(ptr, "\r");
			add_to_msg(port, can, ptr);
		}
		
		return 1;
	}
	
	strcat(ptr, "\r");
	add_to_msg(port, can, ptr);
	return 1;	
}

static char *get_address(char *ptr)
{
	static char add[41];
	char *end, *ptr_tmp;
	
	ptr_tmp = strchr(ptr, '<');
	if (ptr_tmp)
	{
		ptr = ptr_tmp+1;
		end = strchr(ptr, '>');
		if (end)
			*end = '\0';
		else 
			return NULL;
	}
	/* If address does not match '"nickname" <user@host>', takes all chars*/
	else
	{
		/* Consistency check */
		end = strchr(ptr, '>');
		if (end) return NULL ; 
	}
	
	/**** SHOULD BE CONFIGURABLE (BEGIN) ****/
	
	/* Translate address like f6fbb%f6fbb.fmlr.fra.eu@f6fbb.ampr.org */
	end = strchr(ptr, '%');
	if (end)
	{
		*end = '@';
		end = strchr(end+1, '@');
		if (end)
			*end = '\0';
	}
	
	/**** SHOULD BE CONFIGURABLE (END) ****/

	/* copy and upcase */
	strn_cpy(sizeof(add)-1, add, ptr);
	
	return add;
}

static int smtp_cmd_ehlo(int port, int can, char *buffer)
{
	char line[80];
	
	smtp_reply(port, can, 1, 250, "%s", mypath);
	
	strcpy(line, "AUTH");
	if (tport[port].smtp_auth & SMTP_AUTH_LOGIN)
		strcat(line, " LOGIN");
	if (tport[port].smtp_auth & SMTP_AUTH_PLAIN)
		strcat(line, " PLAIN");
	if (tport[port].smtp_auth & SMTP_AUTH_CRMD5)
		strcat(line, " CRAM-MD5");
	smtp_reply(port, can, 0, 250, line);

	sprintf (buffer, "(%d) CONNECTED to %s", can, mycall);
	tport[port].tcan[can].state = SMTP_TRANS;
	tport[port].tcan[can].nb_try = 0;
	tport[port].tcan[can].timeout = time (NULL) + 120L;
	if (tport[port].smtp_auth & SMTP_AUTH_NO)
	{
		/* No authentication needed */
		tport[port].tcan[can].auth_ok = 1;
	}
	return (strlen (buffer));
}

static int smtp_cmd_auth(int port, int can, char *buffer)
{
	char *pstr;
	char *ptr = buffer + 4;

	if (tport[port].tcan[can].auth_ok == 2)
	{
		smtp_reply(port, can, 0, 503, "Authentication already done !");
		return 0;
	}
	
	while (isspace(*ptr))
		++ptr;

	if ((tport[port].smtp_auth & SMTP_AUTH_LOGIN) && (strncmpi(ptr, "LOGIN", 5) == 0))
	{
		ptr += 5;

		while (isspace(*ptr))
			++ptr;

		if (*ptr)
		{
			/* Login name on the same line */
			strn_cpy(6, tport[port].tcan[can].callsign.call, base64_to_str(ptr));
			smtp_reply(port, can, 0, 334, "%s", str_to_base64("Password:"));
			tport[port].tcan[can].state = SMTP_PASS;
		}
		else
		{
			/* Ask for login name */
			smtp_reply(port, can, 0, 334, "%s", str_to_base64("Username:"));
			tport[port].tcan[can].state = SMTP_USER;
		}
		tport[port].tcan[can].nb_try = 0;
		tport[port].tcan[can].timeout = time (NULL) + 120L;
	}
	else if ((tport[port].smtp_auth & SMTP_AUTH_PLAIN) && (strncmpi(ptr, "PLAIN", 5) == 0))
	{
		ptr += 5;

		while (isspace(*ptr))
			++ptr;

		/* Login name and password on the same line */
		pstr = base64_to_str(ptr);
		
		if (*pstr == '\0')
			++pstr;
		strn_cpy(6, tport[port].tcan[can].callsign.call, pstr);
		pstr += strlen(pstr)+1;
		smtp_cmd_pass(port, can, pstr, 0);
	}
	else if ((tport[port].smtp_auth & SMTP_AUTH_CRMD5) && (strncmpi(ptr, "CRAM-MD5", 5) == 0))
	{
		sprintf (tport[port].tcan[can].md5string, "<%d.%ld@%s>",
				getpid(), time(NULL), mypath);
		smtp_reply(port, can, 0, 334, "%s", str_to_base64(tport[port].tcan[can].md5string));
		tport[port].tcan[can].state = SMTP_MD5;
	}
	else
	{
		smtp_reply(port, can, 0, 502, "Unrecognized authentication type.");
		return 0;
	}		
	return 0;
}

static int smtp_cmd_user(int port, int can, char *buffer)
{
	char *ptr = buffer;
	
	while (isspace(*ptr))
		++ptr;

	strn_cpy(6, tport[port].tcan[can].callsign.call, base64_to_str(ptr));
	smtp_reply(port, can, 0, 334, "%s", str_to_base64("Password:"));
	tport[port].tcan[can].state = SMTP_PASS;
	return 0;
}

static int smtp_cmd_md5(int port, int can, char *buffer)
{
	char *callsign;
	char *ptr = buffer;
	unsigned addr_len;
	struct sockaddr_in *address;
	struct sockaddr_in sock_addr;

	while (isspace(*ptr))
		++ptr;

	addr_len = sizeof (sock_addr);
	if ((getpeername(tport[port].tcan[can].sock, (struct sockaddr *)&sock_addr, &addr_len) == 0) && (sock_addr.sin_family == AF_INET))
		address = &sock_addr;
	else
		address = NULL;
		
	ptr = base64_to_str(ptr);

	callsign = ptr;

	while (*ptr && !isspace(*ptr))
		++ptr;
	
	if (*ptr == '\0')
	{
		smtp_reply(port, can, 0, 501, "Authentication error.");
		close (tport[port].tcan[can].sock);
		tport[port].tcan[can].sock = -1;
		return 0;
	}

	*ptr++ = '\0';
	while (isspace(*ptr))
		++ptr;	

	strn_cpy(6, tport[port].tcan[can].callsign.call, callsign);

	switch (pop_check_call (port, can, tport[port].tcan[can].callsign.call, address))
	{
	case 1:				
		// Check password
		if (pop_check_md5 (port, can, ptr))
		{
			tport[port].tcan[can].state = SMTP_TRANS;
			smtp_reply(port, can, 0, 235, "Authentication successful.");
			tport[port].tcan[can].nb_try = 0;
			tport[port].tcan[can].timeout = time (NULL) + 120L;
			tport[port].tcan[can].auth_ok = 2;
			return 0;
		}
		break;
	}

	smtp_reply(port, can, 0, 501, "Authentication error.");
	close (tport[port].tcan[can].sock);
	tport[port].tcan[can].sock = -1;
	return 0;
}

static int smtp_cmd_pass(int port, int can, char *buffer, int base64)
{
	char *ptr = buffer;
	unsigned addr_len;
	struct sockaddr_in *address;
	struct sockaddr_in sock_addr;

	while (isspace(*ptr))
		++ptr;

	addr_len = sizeof (sock_addr);
	if ((getpeername(tport[port].tcan[can].sock, (struct sockaddr *)&sock_addr, &addr_len) == 0) && (sock_addr.sin_family == AF_INET))
		address = &sock_addr;
	else
		address = NULL;
		
	if (base64)
		ptr = base64_to_str(ptr);

	switch (pop_check_call (port, can, tport[port].tcan[can].callsign.call, address))
	{
	case 1:				
		// Check password
		if (pop_check_pass (port, can, ptr))
		{
			tport[port].tcan[can].state = SMTP_TRANS;
			smtp_reply(port, can, 0, 235, "Authentication successful.");
			tport[port].tcan[can].nb_try = 0;
			tport[port].tcan[can].timeout = time (NULL) + 120L;
			tport[port].tcan[can].auth_ok = 2;
			return 0;
		}
		break;
	}
	smtp_reply(port, can, 0, 501, "Authentication error.");
	close (tport[port].tcan[can].sock);
	tport[port].tcan[can].sock = -1;
	return 0;
}

static int smtp_cmd_helo(int port, int can, char *buffer)
{
	smtp_reply(port, can, 0, 250, "%s", mypath);
	sprintf (buffer, "(%d) CONNECTED to %s", can, mycall);
	tport[port].tcan[can].state = SMTP_TRANS;
	tport[port].tcan[can].nb_try = 0;
	tport[port].tcan[can].timeout = time (NULL) + 120L;
	if (tport[port].smtp_auth & SMTP_AUTH_NO)
	{
		/* No authentication needed */
		tport[port].tcan[can].auth_ok = 1;
	}
	return (strlen (buffer));
}

static int smtp_cmd_quit(int port, int can, char *buffer)
{
	smtp_reply(port, can, 0, 221, "Bye!");
	if (tport[port].tcan[can].state != SMTP_START)
	{
		pop_to_bbs(port, can, "B\r", 0);
	}
	else
	{
		close (tport[port].tcan[can].sock);
		tport[port].tcan[can].sock = -1;
	}
	return 0;
}

static int smtp_cmd_mail(int port, int can, char *buffer)
{
	char *ptr = buffer + 10;

	if (!tport[port].tcan[can].auth_ok)
	{
		smtp_reply(port, can, 0, 503, "Authentication required !");
		return 0;
	}
	
	if (tport[port].tcan[can].mail_from)
	{
		smtp_reply(port, can, 0, 503, "Duplicate MAIL FROM:");
		return 0;
	}

	while (isspace(*ptr))
		++ptr;
		
	ptr = get_address(ptr);
	if (ptr)
	{
		taddr_t *mailfrom = calloc(sizeof(taddr_t), 1);
		mailfrom->address = strdup(ptr);
		mailfrom->next = tport[port].tcan[can].mail_from;
		tport[port].tcan[can].mail_from = mailfrom;
		smtp_reply(port, can, 0, 250, "OK");
	}
	else
	{
		smtp_reply(port, can, 0, 501, "Error : MAIL FROM: <address>");
	}

	return 0;
}

static int smtp_cmd_rcpt(int port, int can, char *buffer)
{
	char *ptr = buffer + 8;

	if (!tport[port].tcan[can].auth_ok)
	{
		smtp_reply(port, can, 0, 503, "Authentication required !");
		return 0;
	}
	
	if (!tport[port].tcan[can].mail_from)
	{
		smtp_reply(port, can, 0, 513, "Missing MAIL FROM:");
		return 0;
	}

	while (isspace(*ptr))
		++ptr;
		
	ptr = get_address(ptr);
	if (ptr)
	{
		taddr_t *cur = tport[port].tcan[can].rcpt_to;

		taddr_t *rcptto = malloc(sizeof(taddr_t));
		rcptto->address = strdup(ptr);
		rcptto->next = NULL;

		if (cur)
		{
			/* Add the recipient to the end of list */
			while (cur->next)
				cur = cur->next;
			cur->next = rcptto;
		}
		else
		{
			tport[port].tcan[can].rcpt_to = rcptto;
		}

		smtp_reply(port, can, 0, 250, "OK");
	}
	else
	{
		smtp_reply(port, can, 0, 501, "Error : RCPT TP: <address>");
	}

	return 0;
}

static int smtp_cmd_noop(int port, int can, char *buffer)
{
	smtp_reply(port, can, 0, 250, "OK");
	return 0;
}

static int smtp_cmd_rset(int port, int can, char *buffer)
{
	free_address(port, can);
	smtp_reply(port, can, 0, 250, "OK");
	return 0;
}

static int smtp_cmd_data(int port, int can, char *buffer)
{
	if (!tport[port].tcan[can].auth_ok)
	{
		smtp_reply(port, can, 0, 503, "Authentication required !");
		return 0;
	}
	
	if (!tport[port].tcan[can].rcpt_to)
		smtp_reply(port, can, 0, 513, "Missing RCPT TO:");
	else
	{
		tport[port].tcan[can].head = 1;
		tport[port].tcan[can].state = SMTP_MSG;
		smtp_reply(port, can, 0, 354, "End with <CRLF>.<CRLF>");	
	}
	return 0;
}

static int smtp_cmd_vrfy(int port, int can, char *buffer)
{
	char *ptr = buffer + 4;

	if (!tport[port].tcan[can].auth_ok)
	{
		smtp_reply(port, can, 0, 503, "Authentication required !");
		return 0;
	}
	
	while (isspace(*ptr))
		++ptr;

	smtp_reply(port, can, 0, 252, "Cannot VRFY %s", ptr);
	return 0;
}


/*
 * NNTP commands processing **
 */

/* Get a bulletin */
static int nntp_bull(int port, int can, int mode, char *str)
{
	char buf[80];
	int bid;

	tport[port].tcan[can].nb_lines = mode;
	tport[port].tcan[can].cr = 1;
	tport[port].tcan[can].head = 1;
	if (mode == 3)
		tport[port].tcan[can].state = NNTP_LINE;
	else
		tport[port].tcan[can].state = NNTP_MSG;
	bid = search_bid(str); // Use BID's as the Message ID. 
	if (bid > 0)
	{
		sprintf(buf, "TH R %d %d\r", mode, bid);
	}
	else // But fallback to the old code, in case old messages are still around
	{
		snprintf(buf, sizeof(buf)-1, "TH R %d %s\r", mode, str);
		buf[sizeof(buf)-1]= 0;
	}
	pop_to_bbs(port, can, buf, 1);

	return 0;
}

static int nntp_cmd_quit(int port, int can, char *buffer)
{
	/* Received "QUIT" : disconnect */
	pop_send(port, can, "205 NNTP Server shutdown.\r\n", mycall);
	if (tport[port].tcan[can].state != NNTP_USER)
	{
		pop_to_bbs(port, can, "B\r", 0);
	}
	else
	{
		close (tport[port].tcan[can].sock);
		tport[port].tcan[can].sock = -1;
	}
	return 0;
}

static int nntp_cmd_group(int port, int can, char *buffer)
{
	char buf[80];
	char *ptr = buffer + 5;
	
	/*
	smtp_reply(port, can, 0, 480, "Authentivation required");
	return 0;
	*/
	
	while (isspace(*ptr))
		++ptr;

	if (*ptr == '\0')
	{
		pop_send(port, can, "411 Group name missing\r\n");
		return 0;
	}
	
	snprintf(buf, sizeof(buf)-1, "TH G %s\r", ptr);
	buf[sizeof(buf)-1] = 0;
	pop_to_bbs(port, can, buf, 1);

	tport[port].tcan[can].state = NNTP_LINE;
	tport[port].tcan[can].nb_try = 0;
	tport[port].tcan[can].timeout = time (NULL) + 120L;

	return 0;
}

static int nntp_cmd_list(int port, int can, char *buffer)
{
	char *ptr = buffer + 4;
	
	while (isspace(*ptr))
		++ptr;

	if (strcmpi(ptr, "NEWSGROUP") == 0)
	{
		char name[80];
		
		smtp_reply(port, can, 0, 215, "List of groups information");
		ptr = first_group();
		while (ptr)
		{
			sscanf(ptr, "%s", name);
			pop_send(port, can, "%s %s\r\n", name, name);
			ptr = next_group();
		}
	}
	else
	{
		smtp_reply(port, can, 0, 215, "List of groups");
		ptr = first_group();
		while (ptr)
		{
			pop_send(port, can, "%s\r\n", ptr);
			ptr = next_group();
		}
	}			
	pop_send(port, can, ".\r\n");
	return 0;
}

static int nntp_cmd_stat(int port, int can, char *buffer)
{
	char *ptr = buffer + 4;
	
	while (isspace(*ptr))
		++ptr;
		
	nntp_bull(port, can, 3, ptr);

	return 0;
}

static int nntp_cmd_head(int port, int can, char *buffer)
{
	char *ptr = buffer + 4;
	
	while (isspace(*ptr))
		++ptr;
		
	nntp_bull(port, can, 1, ptr);
	
	return 0;
}

static int nntp_cmd_body(int port, int can, char *buffer)
{
	char *ptr = buffer + 4;
	
	while (isspace(*ptr))
		++ptr;
		
	nntp_bull(port, can, 2, ptr);
	
	return 0;
}

static int nntp_cmd_next(int port, int can, char *buffer)
{
	char buf[80];

	sprintf(buf, "TH N\r");
	pop_to_bbs(port, can, buf, 1);

	tport[port].tcan[can].state = NNTP_LINE;
	tport[port].tcan[can].nb_try = 0;
	tport[port].tcan[can].timeout = time (NULL) + 120L;

	return 0;
}

static int nntp_cmd_post(int port, int can, char *buffer)
{
	if (!tport[port].tcan[can].auth_ok)
	{
		pop_send(port, can, "480 Authentication required\r\n");
		return 0;
	}
	tport[port].tcan[can].head = 1;
	tport[port].tcan[can].state = NNTP_POSTMSG;
	pop_send(port, can, "340 Ok\r\n");
	return 0;
}

static int nntp_cmd_article(int port, int can, char *buffer)
{
	char *ptr = buffer + 7;
	
	while (isspace(*ptr))
		++ptr;
		
	nntp_bull(port, can, 0, ptr);
	
	return 0;
}

static int nntp_cmd_help(int port, int can, char *buffer)
{
	pop_send(port, can, "100 Help text follows\r\n");
	pop_send(port, can, "Commands : \r\n");
	pop_send(port, can, "ARTICLE   BODY      GROUP     HEAD\r\n");
	pop_send(port, can, "HELP      IHAVE     LAST      LIST\r\n");
	pop_send(port, can, "NEWGROUPS NEWNEWS   NEXT      POST\r\n");
	pop_send(port, can, "QUIT      SLAVE     STAT\r\n");
	pop_send(port, can, ".\r\n");
	return 0;
}

static int nntp_cmd_ihave(int port, int can, char *buffer)
{
	pop_send(port, can, "437 Posting not allowed\r\n");
	return 0;
}

static int nntp_cmd_last(int port, int can, char *buffer)
{
	char buf[80];

	sprintf(buf, "TH P\r");
	pop_to_bbs(port, can, buf, 1);

	tport[port].tcan[can].state = NNTP_LINE;
	tport[port].tcan[can].nb_try = 0;
	tport[port].tcan[can].timeout = time (NULL) + 120L;

	return 0;
}

static int nntp_cmd_xhdr(int port, int can, char *buffer)
{
	char buf[80];

	char *ptr = buffer + 4;
	
	while (isspace(*ptr))
		++ptr;
		
	snprintf(buf, sizeof(buf)-1, "TH H %s\r", ptr);
	buf[sizeof(buf)-1] = 0;
	pop_to_bbs(port, can, buf, 1);

	tport[port].tcan[can].state = NNTP_MSG;
	tport[port].tcan[can].nb_try = 0;
	tport[port].tcan[can].timeout = time (NULL) + 120L;

	return 0;
}

static int nntp_cmd_xover(int port, int can, char *buffer)
{
	char buf[80];
	char *ptr = buffer + 5;
	
	while (isspace(*ptr))
		++ptr;
		
	snprintf(buf,sizeof(buf)-1, "TH O %s\r", ptr);
	buf[sizeof(buf)-1] = 0;
	pop_to_bbs(port, can, buf, 1);

	tport[port].tcan[can].state = NNTP_MSG;
	tport[port].tcan[can].nb_try = 0;
	tport[port].tcan[can].timeout = time (NULL) + 120L;

	return 0;
}

static int nntp_cmd_authinfo(int port, int can, char *buffer)
{
	char *ptr = buffer + 8;
 	struct sockaddr_in *address;
	struct sockaddr_in sock_addr;
	char *callsign = tport[port].tcan[can].callsign.call;
	
	while (isspace(*ptr))
		++ptr;
	if (strncmpi(ptr, "USER", 4) == 0)	
	{
		ptr += 4;
		while (isspace(*ptr))
			++ptr;
		strn_cpy(6, tport[port].tcan[can].callsign.call, ptr);
		pop_send(port, can, "381 PASS required\r\n");
		
	}
	else if (strncmpi(ptr, "PASS", 4) == 0)	
	{
		unsigned addr_len;
		ptr += 4;
		while (isspace(*ptr))
			++ptr;
		addr_len = sizeof (sock_addr);
		if ((getpeername(tport[port].tcan[can].sock, (struct sockaddr *)&sock_addr, &addr_len) == 0) && (sock_addr.sin_family == AF_INET))
			address = &sock_addr;
		else
			address = NULL;

		// Check login
		switch (pop_check_call (port, can, callsign, address))
		{
		case 1:				
			// Check password
			if (pop_check_pass (port, can, ptr)){
				taddr_t *mailfrom;
				pop_send(port, can, "281 Ok\r\n");
				tport[port].tcan[can].nb_try = 0;
				tport[port].tcan[can].timeout = time (NULL) + 120L;
				tport[port].tcan[can].auth_ok = 2;
				// Set From callsign to be CALLSIGN (from local bbs)
				mailfrom = calloc(sizeof(taddr_t), 1);
				mailfrom->address = strdup(callsign);
				mailfrom->next = tport[port].tcan[can].mail_from;
				tport[port].tcan[can].mail_from = mailfrom;
				return 0;
			}
			break;
		default:
			break;
		}
		pop_send(port, can, "281 Ok\r\n");// According to Spec.  always return 281 Ok
		
	}
	else 
	{
		pop_send(port, can, "501 user NAME|pass Password\r\n");
	}
				

	return 0;
}


static int twodig(char *ptr)
{
	int val;
	
	val  = (ptr[0] -'0') * 10;
	val += (ptr[1] -'0');

	return val;
}

static int nntp_cmd_newgroups(int port, int can, char *buffer)
{
	int val;
	struct tm t;
	time_t temps;
	char *ptr = buffer + 9;
	
	while (isspace(*ptr))
		++ptr;
		
	temps = time(NULL);
	if (strlen(ptr) >= 13)
	{
		/* Get time information */
		t = *(localtime(&temps));
			
		/* Get year */
		val = twodig(ptr); ptr += 2;
		if (val < 90)
			val += 100;
		t.tm_year = val;

		/* Get month */
		val = twodig(ptr); ptr += 2;
		t.tm_mon = val - 1;

		/* Get day */
		val = twodig(ptr); ptr += 2;
		t.tm_mday = val;

		while (isspace(*ptr))
			++ptr;

		/* Get hour */
		val = twodig(ptr); ptr += 2;
		t.tm_hour = val;

		/* Get min */
		val = twodig(ptr); ptr += 2;
		t.tm_min = val;

		/* Get sec */
		val = twodig(ptr); ptr += 2;
		t.tm_sec = val;

		/* Set time */
		temps = mktime(&t);
	}
	
	pop_send(port, can, "231 New groups follow\r\n");
	pop_send(port, can, "%s", check_dates(temps));

	return 0;
}

static int nntp_cmd_newnews(int port, int can, char *buffer)
{
	pop_send(port, can, "500 Unknown command\r\n");
	return 0;
}

static int nntp_cmd_slave(int port, int can, char *buffer)
{
	pop_send(port, can, "202 Slave status ignored\r\n");
	return 0;
}

static int nntp_cmd_mode(int port, int can, char *buffer)
{
	char *ptr = buffer + 4;
	
	while (isspace(*ptr))
		++ptr;

	if (strcmpi(ptr, "READER") == 0)		
		pop_send(port, can, "201 Posting not allowed\r\n");
	else
		pop_send(port, can, "500 Unknown command\r\n");
	return 0;
}

static int nntp_rcv_dt(int port, int can, char *buffer, int len)
{
	char *ptr = buffer;

	if (tport[port].tcan[can].sock == -1)
		return (FALSE);

	if (tport[port].tcan[can].state != NNTP_POSTMSG)
		return TRUE;

	/* Main message */
	if (*ptr == '.')
	{
		if (len == 1)
		{
			/* End of the message */
			add_to_msg(port, can, "/EX\r");
			tport[port].tcan[can].state = NNTP_TRANS;
			if (nntp_message(port, can))
				pop_send(port, can, "240 Article posted\r\n");
			else
				pop_send(port, can, "441 Posting failed\r\n");
			return 1;	
		}
		if (ptr[1] == '.')
			++ptr;
	}
	
	if (tport[port].tcan[can].head)
	{
		/* Read the headers */
		
		if (len == 0)
		{
			char str[256];
			taddr_t *extra;

			extra = tport[port].tcan[can].content;
			if (extra)
			{
				while (extra)
				{
					snprintf(str, sizeof(str)-1, "%s\r", extra->address);
					str[sizeof(str)-1] = 0;
					add_to_msg(port, can, str);
					extra = extra->next;
				}
			}

			/* End of headers */
			tport[port].tcan[can].head = 0;
			add_to_msg(port, can, "\r");
		}
		else if (tport[port].tcan[can].extra)
		{
				taddr_t *cur = tport[port].tcan[can].content;
				taddr_t *cont = malloc(sizeof(taddr_t));
				cont->address = strdup(ptr);
				cont->next = NULL;

				if (cur)
				{
					/* Add the extra header to the end of list */
					while (cur->next)
						cur = cur->next;
					cur->next = cont;
				}
				tport[port].tcan[can].extra = (ptr[strlen(ptr)-1] == ';');
		}
		else if (strncmpi("content-type:", ptr, 13) == 0)
		{
			if (strstr(ptr, "multipart"))
			{
				taddr_t *cur = tport[port].tcan[can].content;
				taddr_t *cont = malloc(sizeof(taddr_t));
				cont->address = strdup(ptr);
				cont->next = NULL;

				if (cur)
				{
					/* Add the extra header to the end of list */
					while (cur->next)
						cur = cur->next;
					cur->next = cont;
				}
				else
				{
					tport[port].tcan[can].content = cont;
				}
				tport[port].tcan[can].extra = (ptr[strlen(ptr)-1] == ';');
			}
			
		}
		else if (strncmpi("subject:", ptr, 8) == 0)
		{
			/* Add the title */			
			ptr += 8;
			while (isspace (*ptr))
				++ptr;
			
			strcat(ptr, "\r");
			add_to_msg(port, can, ptr);
		}
		else if (strncmpi("newsgroups:", ptr, 11) == 0)
		{
			ptr += 11;
			while (isspace (*ptr))
				++ptr;
			do
			{
				char *start;int len;
				if (*ptr == ',')ptr++;
				start = ptr;
				for (len = 0;*ptr && (*ptr != ',') ;len++,ptr++);
				if (len)
				{
					taddr_t *cur = tport[port].tcan[can].rcpt_to;

					taddr_t *rcptto = malloc(sizeof(taddr_t));
					rcptto->address = strndup(start, len);
					rcptto->next = NULL;
					if (cur)
					{
						/* Add the recipient to the end of list */
						while (cur->next)
							cur = cur->next;
						cur->next = rcptto;
					}
					else
					{
						tport[port].tcan[can].rcpt_to = rcptto;
					}
				}	
			}while (*ptr);
			
		}
#ifdef HEADERTEST
		else
		{
			taddr_t *cur = tport[port].tcan[can].content;
			taddr_t *cont = malloc(sizeof(taddr_t));
			cont->address = strdup(ptr);
			cont->next = NULL;
			if (cur)
			{
				/* Add the extra header to the end of list */
				while (cur->next)
					cur = cur->next;
				cur->next = cont;
			}
			else
			{
				tport[port].tcan[can].content = cont;
			}
			tport[port].tcan[can].extra = (ptr[strlen(ptr)-1] == ';');
		}
#endif		
	
		return 1;
	}
	
	strcat(ptr, "\r");
	add_to_msg(port, can, ptr);
	return 1;	
}


/* Base 64 routines */

/*
 * Table for encoding base64
static char to_64[64] = {
    33,34,35,36, 37,38,39,40, 41,42,43,44, 45,46,47,48,
    49,50,51,52, 53,54,55,56, 57,58,65,66, 67,68,69,70,
    71,72,73,74, 75,76,77,78, 79,80,81,82, 83,84,85,86,
    87,88,89,90, 48,49,50,51, 52,53,54,55, 56,57,43,47,
};
 */
static char to_64[64] = {
     65, 66, 67, 68,  69, 70, 71, 72,  73, 74, 75, 76,  77, 78, 79, 80,
     81, 82, 83, 84,  85, 86, 87, 88,  89, 90, 97, 98,  99,100,101,102,
    103,104,105,106, 107,108,109,110, 111,112,113,114, 115,116,117,118,
    119,120,121,122,  48, 49, 50, 51,  52, 53, 54, 55,  56, 57, 43, 47,
};
#define B64(ch)  (to_64[(int)(uchar)((ch) & 0x3f)])

/*
 * Table for decoding base64
 */
static char fm_64[256] = {
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255, 62, 255,255,255, 63,
     52, 53, 54, 55,  56, 57, 58, 59,  60, 61,255,255, 255,255,255,255,
    255,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14,
     15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25,255, 255,255,255,255,
    255, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40,
     41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
};
#define C64(ch)  (fm_64[(int)(uchar)(ch)])

static char *str_to_base64(char *str)
{
	int ch1, ch2, ch3;
	static char buf[256];
	char *ptr = buf;
	
	while (*str)
	{
		ch1 = *str++;
		*ptr++ = B64(ch1 >> 2);
		
		ch2 = *str++;
		*ptr++ = B64(ch1 << 4 | ch2 >> 4);

		if (ch2 == '\0')
			break;

		ch3 = *str++;
		*ptr++ = B64(ch2 << 2 | ch3 >> 6);
		*ptr++ = B64(ch3 & 0x7f);

		if (ch3 == '\0')
			break;
	}
	*ptr++ = '=';
	*ptr++ = '\0';
	
	return buf;
}

static char *base64_to_str(char *str)
{
	int ch1, ch2, ch3, ch4;
	char *ptr = str;
	char *buf = str;
	
	while (*str)
	{
		ch1 = *str++;
		
		if (C64(ch1) == 255)
			continue;
		
        do {
			if (*str)
	            ch2 = *str++;
			else
				ch2 = 255;
        } while (ch2 != 255 && ch2 != '=' && C64(ch2) == 255);
		
        do {
			if (*str)
	            ch3 = *str++;
			else
				ch3 = 255;
        } while (ch3 != 255 && ch3 != '=' && C64(ch3) == 255);

        do {
			if (*str)
	            ch4 = *str++;
			else
				ch4 = 255;
        } while (ch4 != 255 && ch4 != '=' && C64(ch4) == 255);

		if (ch1 == '=' || ch2 == '=')
			break;

        ch1 = C64(ch1);
        ch2 = C64(ch2);
		*ptr++ = (((ch1 & 0xff) << 2) | ((ch2 & 0x30) >> 4 ));

		if (ch3 == '=')
			break;

        ch3 = C64(ch3);
	    *ptr++ = (((ch2 & 0x0f) << 4) | ((ch3 & 0x3c) >> 2));

		if (ch4 == '=')
			break;

        ch4 = C64(ch4);
		*ptr++ = (((ch3 & 0x03) << 6) | (ch4 & 0xff));	
	}

	*ptr = '\0';
		
	return buf;
}


