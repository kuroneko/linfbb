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


 /**********************************************
 *                                             *
 * xfbbC : Client for xfbbd BBS daemon version *
 *                                             *
 **********************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <malloc.h>
#include <sys/signal.h>
#include <config.h>

#include <fbb_orb.h>
#include "terminal.h"


#define CONSOLE 0
#define MONITOR 1
/*#define ALLCHAN 2*/
#define BUFFSIZE 1024

#define uchar char

static char *usage =
"Usage: xfbbC [-c | -m channel] [-d] [-f] [-h hostname] [-p port] [-i mycall] [-w password]\n"
"-c         : console connection\n"
"-d file    : download remote configuration file\n"
"-f         : data filtering. Control characters are discarded\n"
"-h address : hostname address of the xfbbd server\n"
"-i mycall  : callsign used for connection\n"
"-m channel : displays a connected channel (0 = all channels)\n"
"-p port    : IP port number (default 3286)\n"
"-r         : don't use curses\n"
"-l         : display remote directory\n"
"-s svc_nb  : request service number\n"
"-u file    : upload configuration file\n"
"-w password: password of the callsign as defined in passwd.sys\n"
"\nEnvironment variables :\n"
"\tXFBBC_HOST : hostname    (default localhost)\n"
"\tXFBBC_PORT : socket port (default 3286)\n"
"\tXFBBC_CALL : my callsign\n"
"\tXFBBC_PASS : password string of passwd.sys in xfbbd\n\n";

void makekey (char *cle, char *pass, char *buffer);

static int open_connection (char *tcp_addr, int tcp_port, int mask)
{
	int sock;
	struct sockaddr_in sock_addr;
	struct hostent *phe;

	if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror ("socket_r");
		return (0);
	}

	if ((phe = gethostbyname (tcp_addr)) == NULL)
	{
		perror ("gethostbyname");
		return (-1);
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons (tcp_port);
	memcpy ((char *) &sock_addr.sin_addr, phe->h_addr, phe->h_length);

	if (connect (sock, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) == -1)
	{
		perror ("connect");
		close (sock);
		return (-1);
	}

	return (sock);
}

static int do_filter (char *ptr, int len)
{
	char *scan = ptr;
	int lg = 0;

	while (len)
	{
		if ((*ptr == '\n') || isprint (*ptr))
			scan[lg++] = *ptr;
		++ptr;
		--len;
	}
	return (lg);
}

static int console_send_file(int sock, char *filename)
{
	int fd;
	int nb;
	char *buffer;

	buffer = (char *) (calloc(BUFFSIZE , sizeof(char)));
	
	buffer[0] = ORB_REQUEST;
	buffer[1] = 3;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		buffer[2] = 0;
		buffer[3] = 0;
		if (write(sock, buffer, 4) != 4)
			perror ("console_send_file() socket write error");
		return errno;
	}
		
	for (;;)
	{
		nb = read(fd, buffer+4, BUFFSIZE-24);
		if (nb < 0)
			nb = 0;
		buffer[2] = nb % 256;
		buffer[3] = nb >> 8;
		//write(sock, buffer, nb+4);
		if (write(sock, buffer, nb+4) != nb+4) {
			perror ("console_send_file() socket write error");
			printf ("%d data NOT sent\n", nb);
		} else {
			printf ("%d data sent\n", nb);
		}
		if (nb == 0)
			break;
	}
	
	close(fd);
	return 0;
}

int main (int ac, char *av[])
{
	char host[80];
	char pass[256];
	char key[256];
	char mycall[80];
	char filename[256];
	char arg[256];
	char *buffer;
	char *ptr;
	int len;
	int s;
	int nb;
	int service;
	int transfer;
	int filelen = 0;
	int filetotal;
	int channel;
	int sock;
	int mask = 0;
	int debug = 0;
	int filter = 0;
	int use_curses = 1;
	int port;
	int mode;

	buffer = (char *) (calloc(BUFFSIZE , sizeof(char)));
	
	signal(SIGINT,SIG_IGN);  /* disable ctrl-C */
	signal(SIGQUIT,SIG_IGN); /* disable ctrl-\ */
	signal(SIGTSTP,SIG_IGN); /* disable ctrl-Z */
	
	fprintf (stderr, "\nClient application for xfbbd V%s (%s) ( help : xfbbC -? )\n\n", VERSION, __DATE__);

	if ((ptr = getenv ("XFBBC_HOST")))
		strcpy (host, ptr);
	else
		strcpy (host, "localhost");

	if ((ptr = getenv ("XFBBC_PORT")))
		port = atoi (ptr);
	else
		port = 3286;

	if ((ptr = getenv ("XFBBC_PASS")))
		strcpy (pass, ptr);
	else
		strcpy (pass, "password");

	if ((ptr = getenv ("XFBBC_CALL")))
		strcpy (mycall, ptr);
	else
		strcpy (mycall, "nocall");

	channel = 0;

	mask = ORB_MONITOR;
	transfer = 0;
	
	service = 0;

	while ((s = getopt (ac, av, "cfd:h:i:l:m:p:rs:u:w:?")) != -1)
	{
		switch (s)
		{
		case 'h':
			strcpy (host, optarg);
			break;
		case 'd':
			strcpy (filename, optarg);
			transfer = SVC_RECV;
			mask = 0;
			break;
		case 'u':
			strcpy (filename, optarg);
			transfer = SVC_SEND;
			mask = 0;
			break;
		case 'l':
			strcpy (filename, optarg);
			transfer = SVC_DIR;
			mask = 0;
			break;
		case 'f':
			filter = 1;;
			break;
		case 'p':
			port = atoi (optarg);
			break;
		case 'w':
			strcpy (pass, optarg);
			break;
		case 'i':
			strcpy (mycall, optarg);
			break;
		case 'c':
			mask = ORB_CONSOLE;
			break;
		case 'm':
			channel = atoi (optarg);
			mask = ORB_CHANNEL;
			break;
		case 'r':
			use_curses = 0;
			mask = ORB_CONSOLE;
			break;
		case 's':
			service = atoi (optarg);
			break;
		case ':':
			fprintf (stderr, "xfbbC: option needs an argument\n");
			return 1;
		case '?':
			fprintf (stderr, "%s",  usage);
			return 1;
		}
	}

	if (debug && !transfer)
	{
		/* if -d option, ask all services */
		mask |= (ORB_MSGS | ORB_STATUS | ORB_NBCNX | ORB_LISTCNX);
	}

	fprintf(stderr, "Connecting %s ... ", host); fflush(stdout);
	sock = open_connection (host, port, mask);

	if (sock == -1)
	{
		fprintf (stderr, "Cannot connect to xfbbd, bbs may be offline. Sysop should check if fbb is running.\n");
		return 0;
	}

	fprintf (stderr, " Ok\n");

	sprintf (buffer, "%d %d %s\n", mask, channel, mycall);
	if (write (sock, buffer, strlen (buffer)) != strlen (buffer))
		perror ("main() socket write error");

	fprintf (stderr, "Authentication in progress ... ");
	fflush (stderr);

	nb = read (sock, buffer, 20);
	if (nb <= 0)
	{
		fprintf (stderr, "Connection closed. Terminating\n");
		return (0);
	}

	buffer[nb] = '\0';
	sscanf (buffer, "%s", key);

	fprintf (stderr, " Ok\n");

	makekey (key, pass, buffer);
	strcat (buffer, "\n");
	if (write (sock, buffer, strlen (buffer)) != strlen (buffer))
		perror ("main() socket write error");

	mode = 0;
	if (mask & ORB_CONSOLE)
	{
		mode = 1;
		fprintf (stderr, "Console connection ...\n\n");
		
	}
	if (mask & ORB_CHANNEL)
		fprintf (stderr, "Monitoring channel %d ... Enter <ESC> character to Quit\n\n", channel);
	if (mask & ORB_MONITOR)
		fprintf (stderr, "Monitoring all ports ... Enter <ESC> character to Quit\n\n");

	if (transfer)
	{
		len = strlen(filename);
		
		switch(transfer)
		{
		case SVC_DIR:
			fprintf (stderr, "Requesting directory %s ...\n\n", filename);
			buffer[0] = ORB_REQUEST;
			buffer[1] = SVC_DIR;
			buffer[2] = len;
			buffer[3] = 0;
			strcpy(buffer+4, filename);
			if (write (sock, buffer, len+4) != len+4)
				perror ("main() socket write error");
			break;
		case SVC_RECV:
			fprintf (stderr, "Requesting file %s ...\n\n", filename);
			buffer[0] = ORB_REQUEST;
			buffer[1] = SVC_RECV;
			buffer[2] = len;
			buffer[3] = 0;
			strcpy(buffer+4, filename);
			if (write (sock, buffer, len+4) != len+4)
				perror ("main() socket write error");
			break;
		case SVC_SEND:
			fprintf (stderr, "Sending file %s ...\n\n", filename);
			buffer[0] = ORB_REQUEST;
			buffer[1] = SVC_SEND;
			buffer[2] = len;
			buffer[3] = 0;
			strcpy(buffer+4, filename);
			if (write (sock, buffer, len+4) != len+4)
				perror ("main() socket write error");
			console_send_file(sock, filename);
			break;
		}
		
		filename[0] = '\0';
		filelen = 0;
	}
	else if (service)
	{
		switch(service)
		{
		case SVC_FWD:
			strcpy(buffer+4, "LIST");
			len = 4;
			
			fprintf (stderr, "Requesting Pending Forward ...\n\n");
			buffer[0] = ORB_REQUEST;
			buffer[1] = SVC_FWD;
			buffer[2] = len;
			buffer[3] = 0;
			if (write (sock, buffer, len+4) != len+4)
				perror ("main() socket write error");
			break;
		case SVC_DISC:
			sprintf(buffer+4, "01 F6FBB-1 0");
			len = strlen(buffer+4);
			
			fprintf (stderr, "Requesting Disconnection ...\n\n");
			buffer[0] = ORB_REQUEST;
			buffer[1] = SVC_DISC;
			buffer[2] = len;
			buffer[3] = 0;
			if (write (sock, buffer, len+4) != len+4)
				perror ("main() socket write error");
			break;
		}
	}
	else
	{
		sprintf(buffer, " xfbbC V%s (%s) -  Callsign : %s  -  Remote host : %s", VERSION, __DATE__, mycall, host);
#ifdef HAVE_NCURSES
	if (use_curses)
		init_terminal(mode, buffer);
	else
#endif
		fprintf (stderr, "%s\n", buffer);
	}
	
	for (;;)
	{
		fd_set sock_read;

		FD_ZERO (&sock_read);
		FD_SET (STDIN_FILENO, &sock_read);
		FD_SET (sock, &sock_read);

		/* Wait for I/O event */
		if (select (sock + 1, &sock_read, NULL, NULL, NULL) == -1)
		{
			perror ("select");
			break;
		}

		if (FD_ISSET (STDIN_FILENO, &sock_read))
		{
#ifdef HAVE_NCURSES
			if (use_curses)
				nb = read_terminal(buffer, BUFFSIZE);
			else
			{
#endif
				nb = read (STDIN_FILENO, buffer, BUFFSIZE);
				if (nb == -1)
				{
					perror ("read");
					break;
				}
#ifdef HAVE_NCURSES
			}
#endif
			nb = write (sock, buffer, nb);
			if (nb == -1)
			{
				perror ("write");
				break;
			}

			if (*buffer == 0x1B)
			{
#ifdef HAVE_NCURSES
				if (use_curses)
					end_terminal();
#endif
				close (sock);
				free (buffer);
				return (0);
			}

		}

		if (FD_ISSET (sock, &sock_read))
		{
			uchar header[4];
			unsigned int service;
			unsigned int command;
			unsigned int len;
			unsigned int total;
			char *ptr = '\0';

			/* Read header first. Be sure the 4 bytes are read */
			for (total = 0; total < 4;)
			{
				nb = read (sock, header + total, 4 - total);
				if (nb == -1)
				{
					perror ("read");
					break;
				}

				if (nb == 0)
				{
					printf ("Connection closed. Terminating\n");
#ifdef HAVE_NCURSES
					if (use_curses)
						end_terminal();
#endif
					return (0);
				}

				total += nb;
			}

			service = (unsigned int) header[0];
			command = (unsigned int) header[1];
			len = ((unsigned int) header[3] << 8) + (unsigned int) header[2];

			/* printf("\nservice=%d command=%d len=%d : ", service, command, len); fflush(stdout);*/
				
			/* Read the data following the header. Be sure all bytes are read */
			for (total = 0; total < len;)
			{
				nb = read (sock, buffer + total, len - total);
				if (nb == -1)
				{
					perror ("read");
					break;
				}
				if (nb == 0)
				{
					printf ("Connection closed. Terminating\n");
#ifdef HAVE_NCURSES
					if (use_curses)
						end_terminal();
#endif
					return (0);
				}
				total += nb;
			}

			if (total == 0)
			{
				return 0;	/* end of transfer */
			}
			else
			{
				/* decodes and displays the services */
				switch (service)
				{
				case ORB_CONSOLE:
				case ORB_MONITOR:
				case ORB_CHANNEL:
					if (total > 3)
					{
						/* skip color and header information */
						total -= 3;
						ptr = buffer + 3;
						if (filter)
							total = do_filter (ptr, total);
#ifdef HAVE_NCURSES
						if (use_curses)
							write_terminal(ptr, total);
						else
#endif
							if (write (1, ptr, total) != total)
								perror ("main() socket write error");
					}
					break;
				case ORB_MSGS:
					{
						int nbPriv, nbHeld, nbTotal;

						buffer[total] = '\0';
						sscanf (buffer, "%d %d %d",
								&nbPriv, &nbHeld, &nbTotal);
						if (debug)
							printf ("Messages : Priv %d  Held %d  Total %d\n",
								nbPriv, nbHeld, nbTotal);
					}
					break;
				case ORB_STATUS:
					{
						int MemUsed, MemAvail, Disk1, Disk2;

						buffer[total] = '\0';
						sscanf (buffer, "%d %d %d %d",
								&MemUsed, &MemAvail, &Disk1, &Disk2);
						if (debug)
							printf ("Status   : LMemUsed %d  GMemUsed %dk  Disk1 %dk  Disk2 %dk\n",
								MemUsed, MemAvail, Disk1, Disk2);
					}
					break;
				case ORB_NBCNX:
					buffer[total] = '\0';
					if (debug)
						printf ("Nb Conn  : %s\n", buffer);
					break;
				case ORB_LISTCNX:
					buffer[total] = '\0';
					if (debug)
						printf ("ConnLine :%s\n", buffer);
					break;
				case ORB_DATA:
					switch (command)
					{
					case SVC_LIST:
						/*
						int i;
						printf("Implemented services :\n");
						for (i = 0 ; i < total ; i++)
							printf("   %d : %s\n", buffer[i] & 0xff, service_name(buffer[i]));
						*/
						break;
					case SVC_DIR:
						buffer[total] = '\0';
						if (filename[0] == '\0')
						{
							filelen = filetotal = 0;
							strcpy(filename, buffer);
							sscanf(buffer+strlen(filename)+1, "%d", &filetotal);
						}
						else
						{
							filelen += total;
							if (write(STDOUT_FILENO, buffer, total) != total)
								perror ("main() socket write error");
						}
						fprintf(stderr, "receiving directory %s %d/%d bytes\n", filename, filelen, filetotal);
						break;
					case SVC_RECV:
						buffer[total] = '\0';
						if (filename[0] == '\0')
						{
							filelen = filetotal = 0;
							strcpy(filename, buffer);
							sscanf(buffer+strlen(filename)+1, "%d", &filetotal);
						}
						else
						{
							filelen += total;
							if (write(STDOUT_FILENO, buffer, total) != total)
								perror ("main() socket write error");
						}
						fprintf(stderr, "receiving file %s %d/%d bytes\n", filename, filelen, filetotal);
						break;
					case SVC_FWD:
						buffer[total] = '\0';
						if (filename[0] == '\0')
						{
							filelen = filetotal = 0;
							*arg = '\0';
							sscanf(buffer, "%s %s", filename, arg);
							sscanf(buffer+strlen(buffer)+1, "%d", &filetotal);
							if (*arg)
								fprintf(stderr, "%s : %s\n", filename, arg);
						}
						else
						{
							filelen += total;
							if (write(STDOUT_FILENO, buffer, total) != total)
								perror ("main() socket write error");
						}
						if (filetotal)
							fprintf(stderr, "receiving fwd list %s %d/%d bytes\n", filename, filelen, filetotal);
						break;
					}
					break;
				}
			}
		}
	}

	close (sock);
	free (buffer);
#ifdef HAVE_NCURSES
	if (use_curses)
		end_terminal();
#endif
	printf ("Abnormal termination\n");
	return (1);
}

#define PROTOTYPES 1
#include "global.h"
#include "md5.h"


void MD5String (uchar *dest, uchar *source)
{
	int i;
	MD5_CTX context;
	uchar digest[16];
	unsigned int len = strlen (source);

	MD5Init (&context);
	MD5Update (&context, source, len);
	MD5Final (digest, &context);

	*dest = '\0';

	for (i = 0; i < 16; i++)
	{
		char tmp[5];

		sprintf (tmp, "%02X", digest[i]);
		strcat (dest, tmp);
	}
}

void makekey (char *cle, char *pass, char *buffer)
{
	char *source;

	source = (char *) (calloc(BUFFSIZE , sizeof(char)));
	
	strcpy (source, cle);
	strcat (source, pass);
	MD5String (buffer, source);
	free (source);
}
