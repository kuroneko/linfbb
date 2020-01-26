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


 /******************************************************
 *                                                     *
 *         FBB Driver for AF_AX25 domain sockets       *
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
#include <netax25/axconfig.h>
#include <netax25/nrconfig.h>
#include <netax25/rsconfig.h>

union sockaddr_ham
{
	struct full_sockaddr_ax25 axaddr;
	struct sockaddr_rose rsaddr;
};

#define AX25_CALLSID 11			/* Big enough for ROSE addresses */
#define READ_EVENT 1
#define WRITE_EVENT 2
#define EXCEPT_EVENT 4

#define DISCONNECT 0
#define CPROGRESS 1
#define CONNECTED 2

#define NO_EVENT   0
#define DISC_EVENT 1
#define CONN_EVENT 2
#define RETR_EVENT 3
#define BUSY_EVENT 4

#define SOCK_MAXCAN  (MAXVOIES) /* was 50 -- this was causing bad problems. 
			This table was being accessed beyond the array */

#define	CAN_AX25	0
#define	CAN_NETROM	1
#define	CAN_ROSE	2

#ifndef SOL_AX25
#define SOL_AX25 257
#endif

#undef open
#undef read
#undef write
#undef close

typedef struct
{
	int ncan;
	int sock;
	int port;
	int state;
	int paclen;
	int maxframe;
	int type;					/* CAN_AX25, CAN_NETROM or CAN_ROSE */
	int event;
	int queue;
	char source[AX25_CALLSID];
}
scan_t;

scan_t scan[SOCK_MAXCAN] = {{0}};  /* I think this is necessary (Though not 100% sure.  */

static int last_can = 1;
static int msocket = -1;

static int is_rsaddr (char *);
static int stop_cnx (int port);
static int sock_rcv_ui (int *, char *, int *, ui_header *);
static int sock_snd_ui (int, char *, int, Beacon *);
static int sock_snd_dt (int, char *, int);
static int sock_cmd (int, int, char *);
static int sock_connect (char *, int);
static int sock_connexion (int, int, union sockaddr_ham *, int *, char *);
static int sock_stat (int, stat_ch *);
static int sock_paclen (int);
static int s_free (scan_t *);
static int s_status (scan_t *);
static int name_to_port (char *);
static char *ax25_ntoaddr (char *, const ax25_address *);
static char *rose_ntoaddr (char *, const rose_address *);
static char *rose_ntodnic (char *, const rose_address *);
static void clear_can (int canal);

/*
 *	ax25 -> ascii conversion
 */
static char *_ax25_ntoa(const ax25_address *a)
{
	static char buf[11];
	char c, *s;
	int n;

	for (n = 0, s = buf; n < 6; n++) {
		c = (a->ax25_call[n] >> 1) & 0x7F;

		if (c != ' ')
			*s++ = c;
	}
	
	*s++ = '-';

	n = (a->ax25_call[6] >> 1) & 0x0F;
	if (n > 9) {
		*s++ = '1';
		n -= 10;
	}
	
	*s++ = n + '0';
	*s++ = '\0';

	return buf;
}

/*
 * Generic functions of the driver
 */

/* Check or change status of a port/channel */
int sta_sck (int port, int canal, int cmd, void *ptr)
{

	switch (cmd)
	{
	case TNCSTAT:
		return (sock_stat (canal, (stat_ch *) ptr));
	case PACLEN:
		*((int *) ptr) = sock_paclen (canal);
		return (1);
	case SNDCMD:
		return (sock_cmd (port, canal, (char *) ptr));
	case SETBUSY:
		return stop_cnx (port);
	}
	return (0);
}

/* Sends data */
int snd_sck (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	int ret = 0;

	switch (cmd)
	{
	case COMMAND:
		break;

	case DATA:
		if (len == 0) {
/*			fprintf (stderr, "FBB snd_sck() DATA len == 0 !\n");*/
			break;
		}
		else
			ret = sock_snd_dt (canal, buffer, len);
			break;

	case UNPROTO:
		ret = sock_snd_ui (port, buffer, len, ptr);
		break;
	}
	return (ret);
}

/* receives data */
int rcv_sck (int *port, int *canal, int *cmd, char *buffer, int *len, ui_header * ui)
{
	static int can = 0;
/*	int valid;*/
	int res;

	*cmd = INVCMD;

/*	valid = 0;*/

	/* Test monitoring */

	if (sock_rcv_ui (port, buffer, len, ui))
	{
		*canal = 0;
		*cmd = UNPROTO;
		return (1);
	}

	usleep(50000);		/* wait 50 msec to allow interrupt and avoid CPU overload */
	
	/* Test connection */
	scan[0].sock = p_port[*port].fd;
	res = s_status (&scan[0]);

	if (res & READ_EVENT)
	{
		int new;
		int i;
		unsigned addr_len;
		union sockaddr_ham addr;

		memset(&addr, 0x00, sizeof(struct full_sockaddr_ax25));

/*		addr_len = sizeof (union sockaddr_ham);*/
		addr_len = sizeof (struct full_sockaddr_ax25);

		new = accept (p_port[*port].fd, (struct sockaddr *) &addr, &addr_len);
		if (new == -1)
		{
			perror ("rcv_sck() accept");
			return (FALSE);
		}

		/* Affect the new socket to an empty channel */
		for (i = 1; i < last_can; i++)
		{
			if (scan[i].state == DISCONNECT)
			{
				break;
			}
		}

		if (i == last_can)
		{
			/* Impossible to affect the channel -> disconnection */
			printf ("no channel available\n");
			close (new);
		}
		else
		{
			int ret;

			*canal = i;
			ret = sock_connexion (new, i, &addr, port, buffer);
			if (ret)
			{
				*len = strlen (buffer);
				*cmd = COMMAND;
				*canal = i;
				return (TRUE);
			}
			return (FALSE);
		}
	}

	/* Next channel for the polling */
	++can;
	if (can == last_can)
		can = 1;

	if ((scan[can].sock <= 0) && (scan[can].state != DISCONNECT))
	{
		switch (scan[can].type)
		{
		case CAN_AX25:
			sprintf (buffer, "(%d) DISCONNECTED fm AX25", can);
/*			fprintf (stderr, "(%d) DISCONNECTED fm AX25\n", can);*/
			break;
		case CAN_NETROM:
			sprintf (buffer, "(%d) DISCONNECTED fm NETROM", can);
/*			fprintf (stderr, "(%d) DISCONNECTED fm NETROM\n", can);*/
			break;
		case CAN_ROSE:
			sprintf (buffer, "(%d) DISCONNECTED fm ROSE", can);
/*			fprintf (stderr, "(%d) DISCONNECTED fm ROSE\n", can);*/
			break;
		}
		clear_can (can);
		/* scan[can].state = DISCONNECT; */
		*len = strlen (buffer);
		*cmd = COMMAND;
		*canal = can;
		return (TRUE);
	}

	/* Canal de communication */
	res = s_status (&scan[can]);

	if (res & EXCEPT_EVENT)
	{
		if (scan[can].event == CONN_EVENT)
		{
			/* Appel sortant connecte */
			unsigned addr_len;
			union sockaddr_ham addr;
			int ret;

			memset(&addr, 0x00, sizeof(struct full_sockaddr_ax25));
			
/*			addr_len = sizeof (union sockaddr_ham);*/
			addr_len = sizeof (struct full_sockaddr_ax25);

			if (getpeername (scan[can].sock, (struct sockaddr *) &addr, &addr_len) == 0)
			{
				ret = sock_connexion (scan[can].sock, can, &addr, port, buffer);
				if (ret)
				{
					*len = strlen (buffer);
					*cmd = COMMAND;
					*canal = can;
					return (TRUE);
				}
			}
			else
/*				perror ("getpeername");*/
			return (FALSE);
		}
		else
		{
			switch (scan[can].type)
			{
			case CAN_AX25:
				sprintf (buffer, "(%d) DISCONNECTED fm AX25", can);
/*				fprintf (stderr, "(%d) DISCONNECTED fm AX25\n", can);*/
				break;
			case CAN_NETROM:
				sprintf (buffer, "(%d) DISCONNECTED fm NETROM", can);
/*				fprintf (stderr, "(%d) DISCONNECTED fm NETROM\n", can);*/
				break;
			case CAN_ROSE:
				sprintf (buffer, "(%d) DISCONNECTED fm ROSE", can);
/*				fprintf (stderr, "(%d) DISCONNECTED fm ROSE\n", can);*/
				break;
			}
/*			printf ("Event -> deconnection %s\n", buffer);*/
			close (scan[can].sock);
			clear_can (can);
			*len = strlen (buffer);
			*cmd = COMMAND;
			*canal = can;
			return (TRUE);
		}
	}

	if (res & READ_EVENT)
	{
		int nb;

		if (scan[can].sock <= 0)
		{
/*			printf ("read on invalid socket\n");*/
			return (FALSE);
		}
		nb = read (scan[can].sock, buffer, 256);
		if ((nb == 0) || ((nb == -1) && (errno == ENOTCONN)))
		{
			/* Deconnection */
			switch (scan[can].type)
			{
			case CAN_AX25:
				sprintf (buffer, "(%d) DISCONNECTED fm AX25", can);
/*				fprintf (stderr, "(%d) DISCONNECTED fm AX25\n", can);*/
				break;
			case CAN_NETROM:
				sprintf (buffer, "(%d) DISCONNECTED fm NETROM", can);
/*				fprintf (stderr, "(%d) DISCONNECTED fm NETROM\n", can);*/
				break;
			case CAN_ROSE:
				sprintf (buffer, "(%d) DISCONNECTED fm ROSE", can);
/*				fprintf (stderr, "(%d) DISCONNECTED fm ROSE\n", can);*/
				break;
			}
			close (scan[can].sock);
			clear_can (can);
			*len = strlen (buffer);
			*cmd = COMMAND;
			*canal = can;
			return (TRUE);
		}
		else if (nb == -1)
		{
			printf ("errno = %d\n", errno);
			perror ("read");
		}
		else
		{
			*len = nb;
			*cmd = DATA;
			*canal = can;
			return (TRUE);
		}
	}
	if (res & WRITE_EVENT)
	{
		/* Can write to the socket... Unused */
	}

	return (FALSE);
}

/* Open port */
int opn_sck (int port, int nb)
{
	static int nb_init = 0;
	int old_can;
	int i;
	int ioc;
	int backoff;
	int ok = TRUE;
	char s[80];
	char *p_name;

	sprintf (s, "Init PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);
	InitText (s);

	old_can = last_can;
	last_can += nb;

	if (last_can > SOCK_MAXCAN)
		last_can = SOCK_MAXCAN;

	for (i = old_can; i < last_can; i++)
	{
		clear_can (i);
	}

	if (nb_init == 0)
	{

		nb_init = 1;

		fbb_ax25_config_load_ports ();

		fbb_nr_config_load_ports ();

		fbb_rs_config_load_ports ();

		/* Socket reception monitoring */
		if (msocket <= 0)
		{
			int proto = (all_packets) ? ETH_P_ALL : ETH_P_AX25;

			if ((msocket = socket (AF_PACKET, SOCK_PACKET, htons (proto))) == -1)
			{
				perror ("socket_monitoring");
			}

			else
			{
				/* Socket non bloquant */
				ioc = 1;
				ioctl (msocket, FIONBIO, &ioc);
			}
		}
	}

	/* Receive connections socket */
	if (p_port[port].fd == 0)
	{
		char call[20];
		int addrlen = 0;
		union sockaddr_ham addr;
		
		memset(&addr, 0x00, sizeof(struct full_sockaddr_ax25));

		sprintf (s, "Open PORT %d COM%d-%d",
				 port, p_port[port].ccom, p_port[port].ccanal);
/*		fprintf (stderr, "Open PORT %d COM%d-%d\n",
				 port, p_port[port].ccom, p_port[port].ccanal);
*/		InitText (s);
		sleep (1);

		if (ax25_config_get_addr (p_port[port].name) != NULL)
		{
			p_port[port].type = CAN_AX25;
		}
		else if (nr_config_get_addr (p_port[port].name) != NULL)
		{
			p_port[port].type = CAN_NETROM;
		}
		else if (is_rsaddr (p_port[port].name) || (rs_config_get_addr (p_port[port].name) != NULL))
		{
			p_port[port].type = CAN_ROSE;
		}
		else
		{
/*			fprintf (stderr, "invalid port name %s\n", p_port[port].name);*/
			return (0);
		}

		switch (p_port[port].type)
		{
		case CAN_AX25:
			printf ("CAN_AX25\n");
			if ((p_port[port].fd = socket (AF_AX25, SOCK_SEQPACKET, 0)) < 0)
			{
				perror ("socket_r");
				clear_can (port);
				return (0);
			}

			/* Socket non bloquant */
			ioc = 1;
			ioctl (p_port[port].fd, FIONBIO, &ioc);

			backoff = 0;
			if (setsockopt (p_port[port].fd, SOL_AX25, AX25_BACKOFF, &backoff, sizeof (backoff)) == -1)
			{
				perror ("setsockopt : AX25_BACKOFF");
				close (p_port[port].fd);
				clear_can (port);
				return (0);
			}

			addr.axaddr.fsa_ax25.sax25_family = AF_AX25;
			addr.axaddr.fsa_ax25.sax25_ndigis = 1;
			sprintf (call, "%s-%d", mycall, myssid);

			fprintf (stderr, "CAN_AX25 myscall %s myssid %d\n", mycall, myssid);

			ax25_aton_entry (call, addr.axaddr.fsa_ax25.sax25_call.ax25_call);
			p_name = ax25_config_get_addr (p_port[port].name);
			ax25_aton_entry (p_name, addr.axaddr.fsa_digipeater[0].ax25_call);
			addrlen = sizeof (struct full_sockaddr_ax25);

			break;

		case CAN_NETROM:
			printf ("CAN_NETROM\n");
			if ((p_port[port].fd = socket (AF_NETROM, SOCK_SEQPACKET, 0)) < 0)
			{
				perror ("socket_r");
				clear_can (port);
				return (0);
			}

			/* Socket non bloquant */
			ioc = 1;
			ioctl (p_port[port].fd, FIONBIO, &ioc);

			addr.axaddr.fsa_ax25.sax25_family = AF_NETROM;
			addr.axaddr.fsa_ax25.sax25_ndigis = 0;

			p_name = nr_config_get_addr (p_port[port].name);
			fprintf (stderr, "CAN_NETROM mycall %s myssid %d\n", mycall, myssid);
			ax25_aton_entry (p_name, addr.axaddr.fsa_ax25.sax25_call.ax25_call);
			addrlen = sizeof (struct full_sockaddr_ax25);

			break;

		case CAN_ROSE:
			printf ("CAN_ROSE\n");
			if ((p_port[port].fd = socket (AF_ROSE, SOCK_SEQPACKET, 0)) < 0)
			{
				perror ("socket_r");
				clear_can (port);
				return (0);
			}

			/* Socket non bloquant */
			ioc = 1;
			ioctl (p_port[port].fd, FIONBIO, &ioc);

			addr.rsaddr.srose_family = AF_ROSE;
			addr.rsaddr.srose_ndigis = 0;

			sprintf (call, "%s-%d", mycall, myssid);
			fprintf (stderr, "CAN_ROSE mycall %s myssid %d\n", mycall, myssid);
			ax25_aton_entry (call, addr.rsaddr.srose_call.ax25_call);
			if (is_rsaddr (p_port[port].name))
				p_name = p_port[port].name;
			else
				p_name = rs_config_get_addr (p_port[port].name);
			rose_aton (p_name, addr.rsaddr.srose_addr.rose_addr);
			addrlen = sizeof (struct sockaddr_rose);

			break;
		}

		if (bind (p_port[port].fd, (struct sockaddr *) &addr, addrlen) == -1)
		{
			perror ("bind");
			close (p_port[port].fd);
			clear_can (port);
			return (0);
		}

		if (listen (p_port[port].fd, SOMAXCONN) == -1)
		{
			perror ("listen");
			close (p_port[port].fd);
			clear_can (port);
			return (0);
		}

		memset (&scan[0], 0, sizeof (scan_t));
	}

	sprintf (s, "Prog PORT %d COM%d-%d",
			 port, p_port[port].ccom, p_port[port].ccanal);

/*	fprintf (stderr, "Prog PORT %d COM%d-%d\n",
			 port, p_port[port].ccom, p_port[port].ccanal);
*/
	InitText (s);

	return (ok);
}

/* Close port */
int cls_sck (int port)
{
	int i;

	for (i = 0; i < last_can; i++)
	{
		if (scan[i].sock > 0)
		{
/*			printf ("cls_sck : disconnect stream %d\n", i);
			fprintf (stderr, "cls_sck : disconnect stream %d\n", i);
*/			
			close (scan[i].sock);
		}
		clear_can (i);
		scan[i].state = DISCONNECT;
	}

	if (msocket > 0)
		close (msocket);
	msocket = -1;

	if ((p_port[port].typort == TYP_SCK) && (p_port[port].fd))
	{
		close (p_port[port].fd);
		clear_can (port);
	}

	return (1);
}


/* 

 * Static functions
 *
 */

static int get_call (char *trame, char *buffer)
{
	int c, i, ssid;

	for (i = 0; i < 6; i++)
	{
		c = (*trame++) >> 1;
		if (isalnum (c))
			*buffer++ = c;
	}
	ssid = *trame;

	if ((c = ((ssid >> 1) & 0xf)) != 0)
	{
		*buffer++ = '-';
		if (c >= 10)
		{
			c -= 10;
			*buffer++ = '1';
		}
		*buffer++ = c + '0';
	}
	*buffer = '\0';

	return (ssid);
}

static int stop_cnx (int port)
{
	if (p_port[port].fd) /* Prevent closing of 0 */
	{
		close(p_port[port].fd);
		p_port[port].fd = 0;
	}
	return 1;
}

static int sock_rcv_ui (int *port, char *buffer, int *plen, ui_header * phead)
{
	struct sockaddr sa;
	struct ifreq ifr;
	unsigned asize;

	/*   struct ifreq ifr; */
	char buf[1500];
	char temp[11];
	char *via;
	int lg, ssid, ssid_d, v2;
	char ctrl;
	char *ptr;
	char *pctl;
	int b_port;
	int family;
/*	int prevp;*/
	char name[80];
	
	memset(&sa, 0x00, sizeof(struct sockaddr));
	memset(&ifr, 0x00, sizeof(struct ifreq));
	memset(buf, 0, sizeof(buf));
	memset(temp, 0, sizeof(temp));
	memset(name, 0, sizeof(name));

	do
	{
		b_port = *port;

		asize = sizeof (sa);
		lg = recvfrom (msocket, buf, 1500, 0, &sa, &asize);

		if ((lg <= 0) || ((buf[0] & 0xf) != 0))
		{
			return (0);
		}

		strcpy (ifr.ifr_name, sa.sa_data);
		if (ioctl (msocket, SIOCGIFHWADDR, &ifr) < 0)
		{
			perror ("GIFADDR");
			return (0);
		}

		family = ifr.ifr_hwaddr.sa_family;
	}
	while ((family != AF_AX25) && (family != AF_NETROM) && (family != AF_ROSE));


	if ((ptr = ax25_config_get_name (sa.sa_data)) != NULL)
	{
		strcpy(name, ptr);
		b_port = name_to_port (ptr);
	}

	ptr = buf + 1;
	--lg;

	if (lg < 15)
		return (FALSE);

	memset (phead, 0, sizeof (ui_header));

	via = phead->via;

	if (lg > 300)
		lg = 300;

	ssid_d = get_call (ptr, phead->to);
	ptr += 7;
	ssid = get_call (ptr, phead->from);
	ptr += 7;
	v2 = ((ssid_d & 0x80) != (ssid & 0x80));
	lg -= 14;

	*via = '\0';

	if ((ssid & 1) == 0)
	{
		do
		{
			lg -= 7;
			if (lg < 1)
			{
				ff ();
				return (0);
			}
			ssid = get_call (ptr, temp);
			ptr += 7;
			if (*temp)
			{
				strcat (via, temp);
				if (ssid & 0x80)
				{
					if ((ssid & 1) || (((ssid & 1) == 0) && ((ptr[6] & 0x80) == 0)))
						strcat (via, "*");
				}
				strcat (via, " ");
			}
		}
		while ((ssid & 1) == 0);
	}

	ctrl = *ptr++;
	lg--;
	pctl = phead->ctl;

	if ((ctrl & 0x1) == 0)
	{
		/* I frame */
		*pctl++ = 'I';
		*pctl++ = (ctrl >> 5) + '0';
		*pctl++ = ((ctrl >> 1) & 0x7) + '0';

	}
	else if ((ctrl & 0x3) == 1)
	{
		/* S frame */
		switch (ctrl & 0xf)
		{
		case 0x1:
			*pctl++ = 'R';
			*pctl++ = 'R';
			break;
		case 0x5:
			*pctl++ = 'R';
			*pctl++ = 'N';
			*pctl++ = 'R';
			break;
		case 0x9:
			*pctl++ = 'R';
			*pctl++ = 'E';
			*pctl++ = 'J';
			break;
		}
		*pctl++ = (ctrl >> 5) + '0';

	}
	else
	{
		/* U frame */
		switch (ctrl & 0xec)
		{
		case 0x2c:
			*pctl++ = 'S';
			*pctl++ = 'A';
			*pctl++ = 'B';
			*pctl++ = 'M';
			break;
		case 0x40:
			*pctl++ = 'D';
			*pctl++ = 'I';
			*pctl++ = 'S';
			*pctl++ = 'C';
			break;
		case 0x0c:
			*pctl++ = 'D';
			*pctl++ = 'M';
			break;
		case 0x60:
			*pctl++ = 'U';
			*pctl++ = 'A';
			break;
		case 0x84:
			*pctl++ = 'F';
			*pctl++ = 'R';
			*pctl++ = 'M';
			*pctl++ = 'R';
			break;
		case 0x00:
			*pctl++ = 'U';
			*pctl++ = 'I';
			phead->ui = 1;
			break;
		}
	}

	if (v2)
	{
		if (ctrl & 0x10)
		{
			if (ssid_d & 0x80)
				*pctl++ = '+';
			else
				*pctl++ = '-';
		}
		else
		{
			if (ssid_d & 0x80)
				*pctl++ = '^';
			else
				*pctl++ = 'v';
		}
	}
	else if (ctrl & 0x10)
		*pctl++ = '!';

	*pctl = '\0';

	if (((ctrl & 1) == 0) || ((ctrl & 0xef) == 0x3))
	{
		lg--;
		if (lg < 0)
		{
			return (0);
		}
		phead->pid = *ptr++ & 0xff;
	}
	else
		phead->pid = 0;

/*	prevp = b_port;*/
	if (b_port == 0)
	{
		int bp;

		switch (phead->pid)
		{
		case 0xcf:
			/* Look for the port listening to this packet */
			for (bp = 1; bp < NBPORT; bp++)
			{
				if ((p_port[bp].pvalid) && (p_port[bp].type == CAN_NETROM))
					b_port = bp;
			}
			break;
		case 0x01:
			/* Look for the port listening to this packet */
			for (bp = 1; bp < NBPORT; bp++)
			{
				if ((p_port[bp].pvalid) && (p_port[bp].type == CAN_ROSE))
					b_port = bp;
			}
			break;
		}
		if (b_port == 0)
			return (0);
	}

	/* Par securite ? */
	if (lg < 0)
		return (0);

	if (lg > 256)
		lg = 256;

	*plen = lg;
	if (lg)
		sprintf (phead->txt, " (%d)", lg);
	phead->port = *port = b_port;
	memcpy (buffer, ptr, lg);
	buffer[lg] = '\0';
	return (1);
}

static int sock_snd_ui (int port, char *chaine, int len, Beacon * beacon)
{
	char *calls[10];
	char callsign[9];
	char *ptr;
	int dlen;
	int slen;
	struct full_sockaddr_ax25 dest;
	struct full_sockaddr_ax25 src;
	int i;
	int nb;
	int s;
	
	memset(&src, 0x00, sizeof(struct full_sockaddr_ax25));
	memset(&dest, 0x00, sizeof(struct full_sockaddr_ax25));

	nb = 0;
	calls[nb] = malloc (AX25_CALLSID);
	sprintf (calls[nb++], "%s-%d", beacon->desti.call, beacon->desti.num);
	for (i = 0; i < beacon->nb_digi; i++)
	{
		calls[nb] = malloc (AX25_CALLSID);
		sprintf (calls[nb++], "%s-%d", beacon->digi[i].call, beacon->digi[i].num);
	}
	calls[nb] = NULL;

	if ((dlen = ax25_aton_arglist ((const char **)calls, &dest)) == -1)
	{
		fprintf (stderr, "beacon: unable to convert callsign '%s'\n", calls[0]);
		return 0;
	}
	for (i = 0; calls[i]; i++)
		free (calls[i]);
	sprintf (callsign, "%s-%d", mycall, myssid);
	slen = ax25_aton (callsign, &src);
	ptr = ax25_config_get_addr (p_port[port].name);
	if (ptr == NULL)
		return (0);
	ax25_aton_entry (ptr, src.fsa_digipeater[0].ax25_call);
	src.fsa_ax25.sax25_ndigis = 1;

	if ((s = socket (AF_AX25, SOCK_DGRAM, 0)) == -1)
	{
		perror ("beacon: socket");
		return 0;
	}

	if (bind (s, (struct sockaddr *) &src, slen) == -1)
	{
		perror ("beacon: bind");
		return 0;
	}

	if (sendto (s, chaine, len, 0, (struct sockaddr *) &dest, dlen) == -1)
	{
		perror ("beacon: sendto");
		return 0;
	}

	close (s);

	return (1);
}

static int sock_snd_dt (int canal, char *buffer, int len)
{
	int nb;

	if (scan[canal].sock <= 0)
		return (FALSE);

	nb = write (scan[canal].sock, buffer, len);
	if (nb == -1)
	{
/*		fprintf (stderr, "sock_snd_dt() Error %d on socket %d\n", errno, scan[canal].sock);*/

/*		perror ("sock_snd_dt() : write on socket");*/
		return (FALSE);
	}
	else if (nb < len)
	{
/*		fprintf (stderr, "Cannot write %d bytes on socket %d\n", len - nb, scan[canal].sock);*/

		perror ("Cannot write ?? bytes on socket\n");
		return (FALSE);
	}
	return (TRUE);
}

static int sock_cmd (int port, int canal, char *cmd)
{
	strcpy (scan[canal].source, "\0") ;

	switch (*cmd++)
	{
	case 'I':
		/* source callsign */
		while (isspace (*cmd))
			++cmd;
		strn_cpy (AX25_CALLSID, scan[canal].source, cmd);
		break;
	case 'D':
		/* Deconnection */
		if (scan[canal].sock > 0)
		{
			close (scan[canal].sock);
			/* clear_can (canal); */
			scan[canal].sock = -1;
		}
		break;
	case 'C':
		/* Connection */
		while (isspace (*cmd))
			++cmd;

		scan[canal].paclen = p_port[port].pk_t;
		scan[canal].maxframe = p_port[port].frame;
		scan[canal].port = port;
		scan[canal].type = p_port[port].type;

		sock_connect (cmd, canal);
		break;
	default:
		return (0);
	}
	return (1);
}
static int nr_ax25_aton (char *address, struct full_sockaddr_ax25 *addr)
{
	char buffer[100], *call, *alias;
	FILE *fp;
	int addrlen;

	memset(buffer, 0, sizeof(buffer));

	call = address;

	while (*call)
	{
		if (isspace (*call))
		{
			*call = '\0';
			break;
		}
		*call = toupper (*call);
		++call;
	}

	if ((fp = fopen ("/proc/net/nr_nodes", "r")) == NULL)
	{
		fprintf (stderr, "call: NET/ROM not included in the kernel\n");
		return -1;
	}

	while (fgets (buffer, 100, fp) != NULL)
	{
		call = strtok (buffer, " \t\n\r");
		alias = strtok (NULL, " \t\n\r");

		if (strcmp (address, call) == 0 || strcmp (address, alias) == 0)
		{
			addrlen = ax25_aton (call, addr);
			addr->fsa_ax25.sax25_family = AF_NETROM;
			fclose (fp);
			return (addrlen == -1) ? -1 : sizeof (struct sockaddr_ax25);
		}
	}

	fclose (fp);

/*	fprintf (stderr, "call: NET/ROM callsign or alias not found\n");*/

	return -1;
}
static int rs_ax25_aton (char *address, struct sockaddr_rose *addr)
{
	char *command, *call, *rsaddr, *digi;

	command = strdup (address);
	if (command)
	{
		char roseaddr[12];

		call = strtok (command, " ,\t\n\r");

		addr->srose_family = AF_ROSE;
		addr->srose_ndigis = 0;
		if (ax25_aton_entry (call, addr->srose_call.ax25_call) == -1)
		{
			free (command);
			return -1;
		}

		rsaddr = strtok (NULL, " \t\r\n");
		if (rsaddr == NULL)
		{
			free (command);
			return -1;
		}

		/* DNIC / Address */
		if (strlen (rsaddr) == 10)
			strcpy (roseaddr, rsaddr);
		else if (strlen (rsaddr) == 4)
		{
			strcpy (roseaddr, rsaddr);
			rsaddr = strtok (NULL, " \t\r\n");
			if ((rsaddr == NULL) || (strlen (rsaddr) != 6))
			{
				free (command);
				return -1;
			}
			strcpy (roseaddr + 4, rsaddr);
		}
		if (rose_aton (roseaddr, addr->srose_addr.rose_addr) == -1)
		{
			free (command);
			return -1;
		}

		/* Digis */
		while ((digi = strtok (NULL, " \t\r\n")) != NULL)
		{
#if 0
			if (ax25_aton_entry (digi, addr->srose_digis[addr->srose_ndigis].ax25_call) == -1)
			{
				free (command);
				return -1;
			}
			if (++addr->srose_ndigis == 6)
				break;
#else
			if (ax25_aton_entry (digi, addr->srose_digi.ax25_call) == -1)
			{
				free (command);
				return -1;
			}
			if (++addr->srose_ndigis == 1)
				break;
#endif
		}

		free (command);
	}
	return sizeof (struct sockaddr_rose);
}

static int sock_connect (char *commande, int can)
{
	int fd;
	int addrlen;
	int t1 = 50;

	/* int retry = 3; */

	/*  int one = debug; */
	int ioc;
	union sockaddr_ham addr;
	char *p_name;
	char mycallsign[AX25_CALLSID];
	int backoff = 0;

	memset(&addr, 0x00, sizeof(struct full_sockaddr_ax25));
	memset(mycallsign, 0, sizeof(mycallsign));

	if (strcmp(scan[can].source,"") != 0 )
	{
		strcpy (mycallsign, scan[can].source);
/*		fprintf (stderr, "sock_connect() scan[can].source '%s'\n", scan[can].source);*/
	}
	else
	{
		sprintf (mycallsign, "%s-%d", mycall, myssid);
/*		fprintf (stderr, "sock_connect() mycall '%s' myssid '%d'\n", mycall, myssid);*/
	}

/*		fprintf (stderr, "sock_connect() mycallsign '%s'\n", mycallsign);*/

	switch (scan[can].type)
	{
	case CAN_AX25:
		if ((fd = socket (AF_AX25, SOCK_SEQPACKET, 0)) < 0)
		{
			perror ("socket_ax");
			clear_can (can);
			return (0);
		}

		addr.axaddr.fsa_ax25.sax25_family = AF_AX25;
		addr.axaddr.fsa_ax25.sax25_ndigis = 1;
		p_name = ax25_config_get_addr (p_port[scan[can].port].name);
		ax25_aton_entry (mycallsign, addr.axaddr.fsa_ax25.sax25_call.ax25_call);
		ax25_aton_entry (p_name, addr.axaddr.fsa_digipeater[0].ax25_call);
		addrlen = sizeof (struct full_sockaddr_ax25);

		if (setsockopt (fd, SOL_AX25, AX25_WINDOW, &scan[can].maxframe, sizeof (scan[can].maxframe)) == -1)
		{
			perror ("AX25_WINDOW");
			close (fd);
			clear_can (can);
			return (0);
		}

		if (setsockopt (fd, SOL_AX25, AX25_PACLEN, &scan[can].paclen, sizeof (scan[can].paclen)) == -1)
		{
			perror ("AX25_PACLEN");
			close (fd);
			clear_can (can);
			return (0);
		}

		t1 = 5;
		if (setsockopt (fd, SOL_AX25, AX25_T1, &t1, sizeof (t1)) == -1)
		{
			perror ("AX25_T1");
			close (fd);
			clear_can (can);
			return (0);
		}

		/*
		   if (setsockopt (fd, SOL_AX25, AX25_N2, &retry, sizeof (retry)) == -1)
		   {
		   perror ("AX25_N2");
		   close (fd);
		   clear_can (can);
		   return (0);
		   }
		 */

		if (backoff != -1)
		{
			if (setsockopt (fd, SOL_AX25, AX25_BACKOFF, &backoff, sizeof (backoff)) == -1)
			{
				perror ("AX25_BACKOFF");
				close (fd);
				clear_can (can);
				return (0);
			}
		}

		/*
		   if (ax25mode != -1)
		   {
		   if (setsockopt (fd, SOL_AX25, AX25_EXTSEQ, &ax25mode, sizeof (ax25mode)) == -1)
		   {
		   perror ("AX25_EXTSEQ");
		   close (fd);
		   clear_can (can);
		   return (0);
		   }
		   }
		 */
		break;

	case CAN_NETROM:
		if ((fd = socket (AF_NETROM, SOCK_SEQPACKET, 0)) < 0)
		{
			perror ("socket_nr");
			clear_can (can);
			return (0);
		}

		addr.axaddr.fsa_ax25.sax25_family = AF_NETROM;
		addr.axaddr.fsa_ax25.sax25_ndigis = 1;
		p_name = nr_config_get_addr (p_port[scan[can].port].name);
		ax25_aton_entry (p_name, addr.axaddr.fsa_ax25.sax25_call.ax25_call);
		ax25_aton_entry (mycallsign, addr.axaddr.fsa_digipeater[0].ax25_call);
		addrlen = sizeof (struct full_sockaddr_ax25);

		/* if (setsockopt (fd, SOL_NETROM, NETROM_PACLEN, &scan[can].paclen, sizeof (scan[can].paclen)) == -1)
		   {
		   perror ("NETROM_PACLEN");
		   close (fd);
		   clear_can (can);
		   return (0);
		   } */
		break;

	case CAN_ROSE:
		if ((fd = socket (AF_ROSE, SOCK_SEQPACKET, 0)) < 0)
		{
			perror ("socket_rs");
			clear_can (can);
			return (0);
		}

		addr.rsaddr.srose_family = AF_ROSE;
		addr.rsaddr.srose_ndigis = 0;
		if (is_rsaddr (p_port[scan[can].port].name))
			p_name = p_port[scan[can].port].name;
		else
			p_name = rs_config_get_addr (p_port[scan[can].port].name);
		rose_aton (p_name, addr.rsaddr.srose_addr.rose_addr);
		ax25_aton_entry (mycallsign, addr.rsaddr.srose_call.ax25_call);
		addrlen = sizeof (struct sockaddr_rose);

		break;

	default:
		return FALSE;
	}

	/*
	   if (debug && setsockopt (fd, SOL_SOCKET, SO_DEBUG, &one, sizeof (one)) == -1)
	   {
	   perror ("SO_DEBUG");
	   close (fd);
	   clear_can (can);
	   return (0);
	   }
	 */


	scan[can].state = CPROGRESS;

	if (bind (fd, (struct sockaddr *) &addr, addrlen) == -1)
	{
		perror ("sock_connect : bind");
		close (fd);

		/* clear_can (can); */
		scan[can].sock = -1;
		return (TRUE);
	}

	switch (scan[can].type)
	{
	case CAN_AX25:
		if ((addrlen = ax25_aton (commande, &addr.axaddr)) == -1)
		{
			close (fd);
			/* clear_can (can); */
			scan[can].sock = -1;
			return (0);
		}
		break;

	case CAN_NETROM:
		if ((addrlen = nr_ax25_aton (commande, &addr.axaddr)) == -1)
		{
			close (fd);
			/* clear_can (can); */
			scan[can].sock = -1;
			return (TRUE);
		}
		break;

	case CAN_ROSE:
		if ((addrlen = rs_ax25_aton (commande, &addr.rsaddr)) == -1)
		{
			close (fd);
			/* clear_can (can); */
			scan[can].sock = -1;
			return (TRUE);
		}
		break;
	}


	ioc = 1;
	ioctl (fd, FIONBIO, &ioc);
	if (connect (fd, (struct sockaddr *) &addr, addrlen) == -1)
	{
		if (errno != EINPROGRESS)
		{
			perror ("connect");
			close (fd);
			/* clear_can (can); */
			fd = -1;
		}
	}

	scan[can].sock = fd;

	return (TRUE);
}

static int sock_stat (int canal, stat_ch * ptr)
{
	unsigned lg;
	int val;
	int tries = 0;
/*
 * valid when ROSE is patched for 
 *	ax25_info_new structure
 */
 	struct ax25_info_struct ax25info; 
 
	memset(&ax25info, 0x00, sizeof(struct ax25_info_struct));

/*	struct ax25_info_struct_new ax25info;
*/
	if (scan[canal].sock <= 0)
	{
		ptr->ack = 0;
		ptr->ret = 0;
		return (0);
	}

	ptr->mem = 100;

	val = s_free (&scan[canal]);

	if (scan[canal].state == DISCONNECT)
	{
		ptr->ack = 0;
		ptr->ret = 0;
	}
	else if (scan[canal].state != CONNECTED)
		ptr->ack = 0;
	else
	{
		ptr->ack = (scan[canal].queue - val) / scan[canal].paclen;
		if ((scan[canal].queue - val) && (ptr->ack == 0))
			ptr->ack = 1;
	}


	if (scan[canal].type == CAN_AX25)
	{
/*
 * valid when ROSE will be patched for 
 *	ax25_info_new structure
 *
 * if (ioctl (scan[canal].sock, SIOCAX25GETINFONEW, &ax25info) == 0) 
 *
 */
		if (ioctl (scan[canal].sock, SIOCAX25GETINFO, &ax25info) == 0)
		{
			ptr->ret = ax25info.n2count;
				
/*			fprintf(stderr, "n2=%d n2t=%d send=%d  recv=%d\n", 
				ax25info.n2, ax25info.n2count,
			   	ax25info.snd_q, ax25info.rcv_q); */
		}
		else
		{
			lg = sizeof (int);

#ifdef AX25_N2COUNT
			if (getsockopt (scan[canal].sock, SOL_AX25, AX25_N2COUNT, &tries, &lg) <= 0)
			{
#endif
				if (getsockopt (scan[canal].sock, SOL_AX25, AX25_N2, &tries, &lg) <= 0)
				{
					/* perror("getsockopt : AX25_N2"); */
				}
#ifdef AX25_N2COUNT
			}
#endif

			ptr->ret = tries;
		}
	}
	else
	{
		ptr->ret = 0;			/* XXX */
	}

	return (1);
}

static int s_status (scan_t * can)
{
	int nb;
	int res = 0;
	fd_set sock_read;
	fd_set sock_write;
	fd_set sock_excep;
	struct timeval to;

	if (can->sock <= 0)
		return (0);

	to.tv_sec = to.tv_usec = 0;
	can->event = 0;

	FD_ZERO (&sock_read);
	FD_ZERO (&sock_write);
	FD_ZERO (&sock_excep);

	FD_SET (can->sock, &sock_read);
	FD_SET (can->sock, &sock_write);
	FD_SET (can->sock, &sock_excep);

	nb = select (can->sock + 1, &sock_read, &sock_write, &sock_excep, &to);
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
		if (FD_ISSET (can->sock, &sock_read))
		{
			res |= READ_EVENT;
		}
		if (FD_ISSET (can->sock, &sock_write))
		{
			if (can->state == CPROGRESS)
			{
/*F6BVP 			nb = write (can->sock, &res, 0);
				printf ("nb = %d\n", nb);
				if (nb != -1)
				{ */
					can->state = CONNECTED;
					can->event = CONN_EVENT;
/*				} */
				res |= EXCEPT_EVENT;
			}
			else
				res |= WRITE_EVENT;
		}
		if (FD_ISSET (can->sock, &sock_excep))
		{
			printf ("Exception sur %d\n", can->sock);
			can->event = NO_EVENT;
			res |= EXCEPT_EVENT;
		}
	}
	return (res);
}

static int sock_paclen (int canal)
{
	if (scan[canal].sock <= 0)
		return (0);

	return (scan[canal].paclen);
}

static int s_free (scan_t * can)
{
	int queue_free;

	if (ioctl (can->sock, TIOCOUTQ, &queue_free) == -1)
	{
		perror ("ioctl : TIOCOUTQ");
		return (0);
	}
	return (queue_free);
}

static char *ax25_ntoaddr (char *peer, const ax25_address * axa)
{
	strcpy(peer, _ax25_ntoa(axa));
	return peer;
}

static char *rose_ntoaddr (char *peer, const rose_address * rsa)
{
	strcpy(peer, rose_ntoa(rsa)+4);
	return peer;
}

static char *rose_ntodnic (char *peer, const rose_address * rsa)
{
	strncpy(peer, rose_ntoa(rsa), 4);
	peer[4] = '\0';
	return peer;
}

static int name_to_port (char *port)
{
	int i;

	for (i = 1; i < NBPORT; i++)
	{
		if (strcmp (port, p_port[i].name) == 0)
			return (i);
	}
	/*  printf("Port <%s> not found in port.sys\n", port); */
	return (0);
}

static int sock_connexion (int new, int can, union sockaddr_ham *addr, int *port, char *buffer)
{
	int p;
	int val;
	unsigned plen;
	int backoff;
	char *pn = NULL;
	char User[80];
	char Node[80];
	char Dnic[80];
	union sockaddr_ham addrham;

	memset(&addrham, 0x00, sizeof(struct full_sockaddr_ax25));
	
	scan[can].state = CONNECTED;
	scan[can].sock = new;

	if (scan[can].port == 0)
	{
		plen = sizeof (addrham);
		if (getsockname (scan[can].sock, (struct sockaddr *) &addrham, &plen) == 0)
		{
			switch (addrham.axaddr.fsa_ax25.sax25_family)
			{
			case AF_AX25:
				scan[can].type = CAN_AX25;
				pn = ax25_config_get_port (&addrham.axaddr.fsa_digipeater[0]);
				break;
			case AF_NETROM:
				scan[can].type = CAN_NETROM;
				pn = nr_config_get_port (&addrham.axaddr.fsa_ax25.sax25_call);
				break;
			case AF_ROSE:
				scan[can].type = CAN_ROSE;
				pn = rs_config_get_port (&addrham.rsaddr.srose_addr);
				break;
			}
			scan[can].port = (pn) ? name_to_port (pn) : *port;
/*			fprintf (stderr, "Connection received on port %s = %d\n", pn, scan[can].port);*/
			if (scan[can].port == 0)
			{
				close (new);
				clear_can (can);
				return (0);
			}
		}
		else
			scan[can].port = *port;
	}

	scan[can].queue = s_free (&scan[can]);

	*port = p = scan[can].port;
	val = p_port[p].pk_t;
	if (val == 0)
		val = 128;
	scan[can].paclen = val;

	switch (scan[can].type)
	{
	case CAN_AX25:
		if (setsockopt (new, SOL_AX25, AX25_PACLEN, &val, sizeof (val)) == -1)
		{
			perror ("setsockopt : AX25_PACLEN");
			close (new);
			clear_can (can);
			return (0);
		}

		backoff = 0;
		if (setsockopt (new, SOL_AX25, AX25_BACKOFF, &backoff, sizeof (backoff)) == -1)
		{
			perror ("setsockopt : AX25_BACKOFF");
			close (new);
			clear_can (can);
			return (0);
		}

		/* Look for informations on the connection */
		ax25_ntoaddr (User, &addr->axaddr.fsa_ax25.sax25_call);
		sprintf (buffer, "(%d) CONNECTED to %d:%s", can, p, User);

/*		fprintf (stderr, "(%d) CONNECTED to %d:%s\n", can, p, User);*/

		if (addr->axaddr.fsa_ax25.sax25_ndigis)
		{
			int i;
			char c_digi[AX25_CALLSID + 1];

			strcat (buffer, " via");
			for (i = 0; i < addr->axaddr.fsa_ax25.sax25_ndigis; i++)
			{
				c_digi[0] = ' ';
				ax25_ntoaddr (c_digi + 1, &addr->axaddr.fsa_digipeater[i]);
				strcat (buffer, c_digi);
			}
		}
		break;

	case CAN_NETROM:
		/* if (setsockopt(new, SOL_NETROM, NETROM_PACLEN, &val, sizeof(val)) == -1)
		   {
		   perror("setsockopt : NETROM_PACLEN");
		   close (new);
		   clear_can (can);
		   return(0);
		   } */

		/* Look for informations on the connection */
		ax25_ntoaddr (User, &addr->axaddr.fsa_ax25.sax25_call);
		ax25_ntoaddr (Node, &addr->axaddr.fsa_digipeater[0]);
		sprintf (buffer, "(%d) CONNECTED to %d:%s via %s", can, p, User, Node);
		
/*		fprintf (stderr, "(%d) CONNECTED to %d:%s via %s\n", can, p, User, Node);
*/
		break;

	case CAN_ROSE:
		/* Look for informations on the connection */
		rose_ntodnic (Dnic, &addr->rsaddr.srose_addr);
		rose_ntoaddr (Node, &addr->rsaddr.srose_addr);
		ax25_ntoaddr (User, &addr->rsaddr.srose_call);
		sprintf (buffer, "(%d) CONNECTED to %d:%s via %s %s", can, p, User, Dnic, Node);
		
/*		fprintf (stderr, "(%d) CONNECTED to %d:%s via %s %s\n", can, p, User, Dnic, Node);
*/		break;
	}

	return (TRUE);
}

static void clear_can (int canal)
{
/*	fprintf (stderr, "(%d) drv_sock : clear_can()\n", canal);*/

	memset (&scan[canal], 0, sizeof (scan_t));
	scan[canal].sock = -1;
	scan[canal].state = DISCONNECT;
}

static int is_rsaddr (char *addr)
{
	int n = 0;

	while (addr[n])
	{
		if (!isdigit (addr[n]))
			return (0);
		++n;
	}
	return (n == 10);
}
