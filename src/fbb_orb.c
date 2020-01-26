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
 * Serveur de communication socket pour connection console
 *
 */

#include <serv.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>

#include <fbb_orb.h>

#define WAITINFO	0
#define WAITPASS	1
#define CONNECTED	2
#define DISCONNECT	3

extern void process (void);
void orb_services (void);
static void set_opt(char *command, char *value);

/* extern void usleep(unsigned long); */

static int orb_fd = -1;

typedef struct _OrbClient
{
	int fd;
	int state;
	int monitor;
	int channel;
	int options;
	int infos;
	long mask;
	time_t cle;
	char callsign[12];
	struct _OrbClient *next;
}
OrbClient;

static OrbClient *client_head = NULL;

static OrbClient *orb_add_client (void)
{
	OrbClient *sptr;

	sptr = calloc (1, sizeof (OrbClient));
	if (sptr == NULL)
		return (NULL);

	sptr->next = client_head;
	client_head = sptr;

	return (sptr);
}

static void orb_del_client (OrbClient * cptr)
{
	OrbClient *sptr;
	OrbClient *prev;

	prev = NULL;
	sptr = client_head;
	while (sptr)
	{
		if (sptr == cptr)
		{
			if (prev)
				prev->next = sptr->next;
			else
				client_head = sptr->next;
			free (sptr);
			break;
		}
		prev = sptr;
		sptr = sptr->next;
	}
}

static int orb_new_connection (int fd)
{
	unsigned addr_len;
	struct sockaddr_in sock_addr;
	
	memset(&sock_addr, 0x00, sizeof(struct sockaddr_in));
	
	OrbClient *sptr;

	sptr = orb_add_client ();

	addr_len = sizeof (sock_addr);

	sptr->fd = accept (fd, (struct sockaddr *) &sock_addr, &addr_len);
	sptr->state = WAITINFO;

	return (sptr->fd);
}

static int send_data(int sock, char *datafile, char *datarequest, int len, int command)
{
	int fd;
	int nb;
	char buffer[1024];
	
	memset(buffer, 0x00, sizeof(buffer));

	buffer[0] = ORB_DATA;
	buffer[1] = command;

	buffer[2] = len % 256;
	buffer[3] = len >> 8;
	memcpy(buffer+4, datarequest, len);
	write(sock, buffer, len+4);
		
	fd = open(datafile, O_RDONLY);
	if (fd == -1)
	{
		buffer[2] = 0;
		buffer[3] = 0;
		write(sock, buffer, 4);
		return errno;
	}
		
	for (;;)
	{
		nb = read(fd, buffer+4, 1000);
		if (nb < 0)
			nb = 0;
		buffer[2] = nb % 256;
		buffer[3] = nb >> 8;
		write(sock, buffer, nb+4);
		printf("%d data sent\n", nb);
		if (nb == 0)
			break;
	}
	
	close(fd);
	unlink(datafile);
		
	return 0;
}

static long flength(char *filename)
{
	struct stat bstat;
	
	memset(&bstat, 0x00, sizeof(struct stat));
	
	if (stat(filename, &bstat) == -1)
		return 0L;

	return bstat.st_size;
}

static int send_data_buf(OrbClient * sptr, int service, char *command, char *data, int datalen)
{
	char buffer[1024];
	int length;
	
	memset(buffer, 0x00, sizeof(buffer));

	length = strlen(command);
	strcpy(buffer+4, command);
	if ((length + datalen + 4) > 1024)
		return -1;

	if (datalen)
	{	
		memcpy(buffer+4+length, data, datalen);
		length += datalen;
	}
	
	buffer[0] = ORB_DATA;
	buffer[1] = service;
	buffer[2] = length & 0xff;
	buffer[3] = length >> 8;
	
	write(sptr->fd, buffer, 4+length);
	
	return length;
}

static void orb_process_data (OrbClient * sptr)
{
	int i, j;
	int nb;
	int lg;
	static char buffer[1024];
	static char datafile[256];
	static char datarequest[1024];
	static int nbrcv = 0;
	static int rcv_fd;
	static int rcv_state = 0;
	char call[256];
	char pass[256];
	char *ptr;

	memset(buffer, 0x00, sizeof(buffer));
	memset(datafile, 0x00, sizeof(datafile));
	memset(datarequest, 0x00, sizeof(datarequest));
	memset(call, 0x00, sizeof(call));
	memset(pass, 0x00, sizeof(pass));
	
	lg = 0;
	
	nb = read (sptr->fd, buffer + nbrcv, sizeof (buffer) - nbrcv);
	if (nb == -1)
		return;

	if (nb == 0)
	{
		editor = 0;
		/* Client is disconnected */
		if (sptr->mask & ORB_CONSOLE)
		{
			/* Disconnect from console */
			deconnexion (CONSOLE, 0);
		}
		else
		{
			close (sptr->fd);
			orb_del_client (sptr);
		}
		nbrcv = 0;
		return;
	}

	nbrcv += nb;

again:

	if ((nbrcv >= 3) && (buffer[0] == ORB_REQUEST))
	{
		unsigned int len;
		int mask;
		int old_mask = sptr->mask;
		int ack;
		char cmd[300];

		memset(cmd, 0x00, sizeof(cmd));
	
	/*commande */
		switch (buffer[1])
		{
		case 0:
			/* Changement de masque */
			mask = buffer[2] & 0xff;
			ack = 0;
			
			if ((mask & ORB_CONSOLE) != (sptr->mask & ORB_CONSOLE))
			{
				if (mask & ORB_CONSOLE)
				{
					FILE *fptr;

					if ((fptr = fopen (d_disque ("etat.sys"), "r+t")) != NULL)
					{
						fprintf (fptr, "%-6s-%X\n",
								 cons_call.call, cons_call.num);
						ferme (fptr, 74);
					}

					/* Console connection */
					if (v_tell && connect_tell ())
					{
						music (0);
						ack = 1;
					}
					else if (!connect_console ())
					{
						ack = 2;
					}
					else
					{
						ack = 1;
					}
				}
				else
				{
					/* Console deconnection */
					deconnexion (CONSOLE, 0);
					ack = 0;
					nbrcv = 0;
					return;
				}
				buffer[0] = ORB_XFBBX;
				buffer[1] = 0;
				buffer[2] = 1;
				buffer[3] = 0;
				buffer[4] = ack;
				write (sptr->fd, buffer, 5);
			}
			if (ack != 2)	/* Don't update mask if connection refused */
			{
				sptr->mask = mask;
				if ((mask & ORB_MSGS) != (old_mask & ORB_MSGS))
				{
					reset_msgs ();
					aff_msg_cons ();
				}
				if ((mask & ORB_STATUS) != (old_mask & ORB_STATUS))
				{
					FbbMem (1);
					test_pactor(1);
				}
				if ((mask & ORB_NBCNX) != (old_mask & ORB_NBCNX))
					fbb_list (1);
				if ((mask & ORB_LISTCNX) != (old_mask & ORB_LISTCNX))
					fbb_list (1);
			}
			lg = 3;
			break;
		case SVC_DIR:
			/* Dir request */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				int nb;

				buffer[nbrcv] = '\0';
				sprintf(datafile, "%sclientdata.txt", back2slash(DATADIR));
				nb = 1+sprintf(datarequest, "%s%s", back2slash(CONFDIR), buffer+4);
				sprintf(cmd, "ls -oA %s | awk '{ print substr($1,1,1),$4,$8,$5\"-\"$6\"-\"$7 }' > %s 2>&1", datarequest, datafile);
				printf("system %s\n", cmd);
				system(cmd);
				nb += 1+sprintf(datarequest + nb, "%ld", flength(datafile));
				send_data(sptr->fd, datafile, datarequest, nb, SVC_DIR);
			}
			lg = len+4;
			break;
		case SVC_RECV:
			/* File request */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				buffer[nbrcv] = '\0';
				sprintf(datafile, "%sclientdata.txt", back2slash(DATADIR));
				nb = 1+sprintf(datarequest, "%s%s", back2slash(CONFDIR), buffer+4);
				sprintf(cmd, "cp %s %s", datarequest, datafile);
				printf("system %s\n", cmd);
				system(cmd);
				nb += 1+sprintf(datarequest + nb, "%ld", flength(datafile));
				send_data(sptr->fd, datafile, datarequest, nb, SVC_RECV);
			}
			lg = len+4;
			break;
		case SVC_SEND:
			/* File receive */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
				
			if (len <= 1000)
			{
				switch (rcv_state)
				{
				case 0 :
					buffer[nbrcv] = '\0';
					sprintf(datafile, "%sclientdata.txt", back2slash(DATADIR));
					sprintf(datarequest, "%s%s", back2slash(CONFDIR), buffer+4);
					printf("receiving file %s\n", datafile);
					rcv_fd = open(datafile, O_CREAT|O_TRUNC|O_WRONLY, 0666);
					rcv_state = 1;
					break;
				case 1:				
					if (len == 0)
					{
						char header[4];
						
						/* End of file */
						if (rcv_fd > 0)
						{
							close(rcv_fd);
							sprintf(cmd, "mv %s %s", datafile, datarequest);
							printf("system %s\n", cmd);
							system(cmd);
							test_fichiers = 1;
						}
						header[0] = ORB_DATA;
						header[1] = SVC_SEND;
						header[2] = 0;
						header[3] = 0;
						write(sptr->fd, header, 4);
						rcv_state = 0;
						break;
					}
					if (rcv_fd > 0)
					{
						nb = write(rcv_fd, buffer+4, len);
						printf("%d data received\n", len);
						if (nb <= 0)
						{
							close(rcv_fd);
							rcv_fd = -1;
						}
					}
					break;
				}
			}
			lg = len+4;
			break;
		case SVC_FWD:
			/* Pending forward request */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				char command[80];
				char arg[80];
				
				memset(command, 0x00, sizeof(command));
				memset(arg, 0x00, sizeof(arg));
				
				buffer[nbrcv] = '\0';

				sscanf(buffer+4, "%s %s", command, arg);
				if (strcasecmp(command, "LIST") == 0)
				{
					sprintf(datafile, "%sclientdata.txt", back2slash(DATADIR));
					strcpy(datarequest, "pending_forward.txt");
					nb = 1+sprintf(datarequest, "%s%s", back2slash(CONFDIR), buffer+4);
					RequestPendingForward(datafile);
					nb += 1+sprintf(datarequest + nb, "%ld", flength(datafile));
					send_data(sptr->fd, datafile, datarequest, nb, SVC_FWD);
				}
				else if (strcasecmp(command, "START") == 0)
				{
					sprintf(datarequest, "START %s", StartForward(atoi(arg)));
					send_data_buf(sptr, SVC_FWD, datarequest, NULL, 0);
				}
				else if (strcasecmp(command, "STOP") == 0)
				{
					sprintf(datarequest, "STOP %s", StopForward(atoi(arg)));
					send_data_buf(sptr, SVC_FWD, datarequest, NULL, 0);
				}
			}
			lg = len+4;
			break;
		case SVC_DISC:
			/* Disconnect request */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				char callsign[80];
				int nChan;
				int bImm;
				int nb;
				
				memset(callsign, 0x00, sizeof(callsign));

				buffer[nbrcv] = '\0';
				nb = sscanf(buffer+4, "%d %s %d", &nChan, callsign, &bImm);
				if (nb == 3)
					disconnect_channel(nChan, bImm);
			}
			lg = len+4;
			break;
		case SVC_USER:
			/* users management */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				int length;
				char command[80];
				char arg[80];
				char *ptr;
				
				memset(command, 0x00, sizeof(command));
				memset(arg, 0x00, sizeof(arg));
				
				buffer[nbrcv] = '\0';

				sscanf(buffer+4, "%s %s", command, arg);
				if (strcasecmp(command, "LIST") == 0)
				{
					sprintf(datafile, "%sclientdata.txt", back2slash(DATADIR));
					strcpy(datarequest, "service.txt");
					nb = 1+sprintf(datarequest, "%s%s", back2slash(CONFDIR), buffer+4);
					RequestUsersList(datafile);
					nb += 1+sprintf(datarequest + nb, "%ld", flength(datafile));
					send_data(sptr->fd, datafile, datarequest, nb, SVC_USER);
				}
				else if (strcasecmp(command, "GET") == 0)
				{
					ptr = GetUserInfo(arg, &length);
					sprintf(datarequest, "USER %s %d\n", arg, length);
					send_data_buf(sptr, SVC_USER, datarequest, ptr, length);
				}
				else if (strcasecmp(command, "PUT") == 0)
				{
					int datalen = len;
					ptr = buffer+4;
					while (*ptr && (*ptr != '\n'))
					{
						++ptr;
						--datalen;
					}
					if (*ptr == '\n')
					{
						++ptr;
						PutUserInfo(arg, ptr, datalen);
						ptr = GetUserInfo(arg, &length);
						sprintf(datarequest, "USER %s %d\n", arg, length);
						send_data_buf(sptr, SVC_USER, datarequest, ptr, length);
					}
				}
				else if (strcasecmp(command, "NEW") == 0)
				{
					NewUserInfo(arg);
					ptr = GetUserInfo(arg, &length);
					sprintf(datarequest, "USER %s %d\n", arg, length);
					send_data_buf(sptr, SVC_USER, datarequest, ptr, length);
				}
				else if (strcasecmp(command, "DEL") == 0)
				{
					DelUserInfo(arg);
				}
			}
			lg = len+4;
			break;
		case SVC_MSG:
			/* messages management */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				int length;
				char command[80];
				char arg[80];
				char *ptr;
				
				memset(command, 0x00, sizeof(command));
				memset(arg, 0x00, sizeof(arg));
				
				buffer[nbrcv] = '\0';

				sscanf(buffer+4, "%s %s", command, arg);
				if (strcasecmp(command, "LIST") == 0)
				{
					sprintf(datafile, "%sclientdata.txt", back2slash(DATADIR));
					strcpy(datarequest, "service.txt");
					nb = 1+sprintf(datarequest, "%s%s", back2slash(CONFDIR), buffer+4);
					RequestMsgsList(datafile);
					nb += 1+sprintf(datarequest + nb, "%ld", flength(datafile));
					send_data(sptr->fd, datafile, datarequest, nb, SVC_MSG);
				}
				else if (strcasecmp(command, "GET") == 0)
				{
					ptr = GetMsgInfo(arg, &length);
					sprintf(datarequest, "MSG %s %d\n", arg, length);
					send_data_buf(sptr, SVC_MSG, datarequest, ptr, length);
				}
				else if (strcasecmp(command, "PUT") == 0)
				{
					int datalen = len;
					ptr = buffer+4;
					while (*ptr && (*ptr != '\n'))
					{
						++ptr;
						--datalen;
					}
					if (*ptr == '\n')
					{
						++ptr;
						PutMsgInfo(arg, ptr, datalen);
						ptr = GetMsgInfo(arg, &length);
						sprintf(datarequest, "MSG %s %d\n", arg, length);
						send_data_buf(sptr, SVC_MSG, datarequest, ptr, length);
					}
				}
			}
			lg = len+4;
			break;
		case SVC_MREQ:
			/* message request */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				char command[80];
				char arg[80];
				
				memset(command, 0x00, sizeof(command));
				memset(arg, 0x00, sizeof(arg));

				buffer[nbrcv] = '\0';
				sscanf(buffer+4, "%s %s", command, arg);
				if (strcasecmp(command, "MSG") == 0)
				{
					sprintf(datafile, "%smessage.txt", back2slash(DATADIR));
					mess_name(MESSDIR, atol(arg), datarequest);
					nb = 1+strlen(datarequest);
					sprintf(cmd, "cp %s %s", datarequest, datafile);
					printf("system %s\n", cmd);
					system(cmd);
					nb += 1+sprintf(datarequest + nb, "%ld", flength(datafile));
					send_data(sptr->fd, datafile, datarequest, nb, SVC_MREQ);
				}
			}
			lg = len+4;
			break;
		case SVC_OPT:
			/* options management */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				char command[80];
				char arg[80];
				char val[80];
				
				memset(command, 0x00, sizeof(command));
				memset(arg, 0x00, sizeof(arg));
				memset(val, 0x00, sizeof(val));

				sptr->options = 1;
				buffer[nbrcv] = '\0';
				*arg = *val = '\0';
				sscanf(buffer+4, "%s %s %s", command, arg, val);
				if (strcasecmp(command, "GET") == 0)
				{
					orb_options();
				}
				else if (strcasecmp(command, "SET") == 0)
				{
					set_opt(arg, val);
				}
			}
			lg = len+4;
			break;
		case SVC_PACTOR:
			/* pactor management */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				char command[80];
				int val = 1;
				int port = 0;
				
				memset(command, 0x00, sizeof(command));

				buffer[nbrcv] = '\0';
				sscanf(buffer+4, "%s %d %d", command, &port, &val);
				if (strcasecmp(command, "SCAN") == 0)
				{
					CmdScan(port, val);
				}
				else if (strcasecmp(command, "CHO") == 0)
				{
					CmdCHO(port, 1);
				}
				else if (strcasecmp(command, "BRK") == 0)
				{
					CmdCHO(port, 0);
				}
			}
			lg = len+4;
			break;
		case SVC_INFO:
			/* channel information */
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			if (sptr->state != CONNECTED)
				return;
			if (len < 256)
			{
				char command[80];
				int val = -1;
				
				memset(command, 0x00, sizeof(command));

				buffer[nbrcv] = '\0';
				sscanf(buffer+4, "%s %d", command, &val);
				sptr->infos = (val != -1);
				set_info_channel(val);
			}
			lg = len+4;
			break;
		default:
			if (nbrcv < 4)
				return;
			len = ((unsigned int) buffer[3] << 8) + (unsigned int) buffer[2];
			if (nbrcv < len+4)
				return;
			lg = len+4;
			break;
		}
				
	}
	else 
	{
		char *endptr;
		
		/* Check if the whole line is received */
		buffer[nbrcv] = '\0';
		endptr = strrchr (buffer, '\n');
		if (endptr == NULL)
			return;

		editor = 0;
		
		lg = 1 + endptr - buffer;

		switch (sptr->state)
		{
		case WAITINFO:

			/* Mask of services */
			sscanf (buffer, "%ld %d %s", &sptr->mask, &sptr->channel, call);
			call[11] = '\0';
			strcpy (sptr->callsign, call);
			sptr->cle = time (NULL);
			sprintf (call, "%010ld", sptr->cle);
			write (sptr->fd, call, strlen (call));
			sptr->state = WAITPASS;
			break;

		case WAITPASS:

			/* Callsign sans SSID */
			strcpy (call, sptr->callsign);
			if ((ptr = strchr (call, '-')))
				*ptr = '\0';

			/* Password */
			sscanf (buffer, "%s", pass);

			if (comp_passwd (call, pass, sptr->cle))
			{
				orb_services();
				cons_call.num = extind (strupr (sptr->callsign), cons_call.call);
				reset_msgs ();
				FbbMem (1);
				aff_msg_cons ();
				fbb_list (1);
				test_pactor(1);
				if (sptr->mask & ORB_CONSOLE)
				{
					FILE *fptr;

					if ((fptr = fopen (d_disque ("etat.sys"), "r+t")) != NULL)
					{
						fprintf (fptr, "%-6s-%X\n",
								 cons_call.call, cons_call.num);
						ferme (fptr, 74);
					}

					/* Console connection */
					if (v_tell && connect_tell ())
					{
						music (0);
					}
					else if (!connect_console ())
					{
						char *txt = "Console already connected !\n\n";
						int len = strlen (txt) + 3;

						strcpy (buffer + 7, txt);

						/* Header */
						buffer[0] = ORB_CONSOLE;
						buffer[1] = 0;
						buffer[2] = (len) & 0xff;
						buffer[3] = (len) >> 8;

						buffer[4] = CONSOLE;
						buffer[5] = W_CNST;
						buffer[6] = 0;

						write (sptr->fd, buffer, len + 4);
						close (sptr->fd);
						orb_del_client (sptr);
						break;			/* F6BVP */
					}
				}
				sptr->state = CONNECTED;
				rcv_state = 0;
			}
			else
			{
				char *txt = "Callsign/Password error !\n\n";
				int len = strlen (txt) + 3;

				strcpy (buffer + 7, txt);

				/* Header */
				buffer[0] = ORB_CONSOLE;
				buffer[1] = 0;
				buffer[2] = (len) & 0xff;
				buffer[3] = (len) >> 8;

				buffer[4] = CONSOLE;
				buffer[5] = W_CNST;
				buffer[6] = 0;

				write (sptr->fd, buffer, len + 4);
				close (sptr->fd);
				orb_del_client (sptr);
			}
			break;

		case CONNECTED:

			if (sptr->mask & ORB_CONSOLE)
			{
				for (i = 0, j = 0; i < nbrcv; i++)
				{
					if (buffer[i] == '\r')
						continue;

					if (buffer[i] == '\n')
						buffer[j++] = '\r';
					else
						buffer[j++] = buffer[i];
				}

				console_inbuf (buffer, j);
			}

			break;
		}
	}
	
	if (lg < nbrcv)
	{
		nbrcv -= lg;
		memmove(buffer, buffer+lg, nbrcv);
		goto again;
	}
	
	nbrcv = 0;
}

static void orb_process (void)
{
	int nb;
	int maxfd;
	fd_set tcp_read;
	fd_set tcp_write;
	fd_set tcp_excep;
	struct timeval to;
	OrbClient *sptr;

	if (orb_fd == -1)
		return;

	to.tv_sec = to.tv_usec = 0;

	FD_ZERO (&tcp_read);
	FD_ZERO (&tcp_write);
	FD_ZERO (&tcp_excep);

	maxfd = orb_fd;
	FD_SET (orb_fd, &tcp_read);

	sptr = client_head;
	while (sptr)
	{
		FD_SET (sptr->fd, &tcp_read);
		if (sptr->fd > maxfd)
			maxfd = sptr->fd;
		sptr = sptr->next;
	}

	nb = select (maxfd + 1, &tcp_read, NULL, NULL, &to);

	if (nb == -1)
	{
		perror ("orb_select");
		return;
	}

	if (nb == 0)
	{
		/* nothing to do */
		return;
	}

	/* To avoid network locks */
	alarm(60);

	if (FD_ISSET (orb_fd, &tcp_read))
	{
		/* New client connection */
		orb_new_connection (orb_fd);
	}

	sptr = client_head;
	while (sptr)
	{
		/* On sauvegarde le next car le pointeur de client peut etre
		   * libere pendant le traitement en cas de deconnexion */
		OrbClient *ptemp = sptr->next;

		if (FD_ISSET (sptr->fd, &tcp_read))
		{
			orb_process_data (sptr);
			is_idle = 0;
		}
		sptr = ptemp;
	}
	
	alarm(0);

	return;
}

int fbb_orb (char *service, int port)
{
	int val;
	int len;
	struct sockaddr_in sock_addr;
	
	memset(&sock_addr, 0x00, sizeof(struct sockaddr_in));

	if ((orb_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror ("socket_r");
		return (0);
	}

	val = 1;
	len = sizeof (val);
	if (setsockopt (orb_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, len) == -1)
	{
		perror ("opn_tcp : setsockopt SO_REUSEADDR");
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = 0;
	sock_addr.sin_port = htons (port);

	if (bind (orb_fd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) != 0)
	{
		perror ("opn_tcp : bind");
		close (orb_fd);
		return (0);
	}

	if (listen (orb_fd, SOMAXCONN) == -1)
	{
		perror ("listen");
		close (orb_fd);
		return (0);
	}

	fprintf (stderr, "xfbbC/X server running ...\n");
	fprintf (stderr, "xfbbd ready and running ...\n");
	fflush (stderr);

	for (;;)
	{
		orb_process ();
		process ();
		if (is_idle)
		{
 			usleep (1);
		}
		else
		{
			is_idle = 1;
		}
	}

	return 1;
}

static void orb_send_data (char *buffer, int lg, int mask)
{
	OrbClient *sptr = client_head;

	while (sptr)
	{
		if (sptr->mask & mask)
		{
			switch (mask)
			{
			case ORB_MONITOR:
			case ORB_CONSOLE:
			case ORB_CHANNEL:
				if (((mask == ORB_MONITOR) && (buffer[4] == 0xff)) ||
					((mask == ORB_CONSOLE) && (buffer[4] == 0)))
				{
					write (sptr->fd, buffer, lg);
				}
/*				if ((mask == ORB_CHANNEL) && (buffer[4] > 0) && (buffer[4] < 0xff)) */
				if (mask == ORB_CHANNEL)
				{
					if ((sptr->channel == 0) || (sptr->channel == buffer[4]))
						write (sptr->fd, buffer, lg);
				}
				break;
			case ORB_XFBBX:
				if (sptr->mask & ORB_CONSOLE)
					write (sptr->fd, buffer, lg);
				break;
			default:
				write (sptr->fd, buffer, lg);
				break;
			}
		}
		sptr = sptr->next;
	}
}

void orb_write (int channel, char *data, int len, int color, int header)
{
	int i, j;
	char *buffer;

	buffer = calloc ((len + 8), sizeof (char));

	if (buffer == NULL)
		return;

	for (i = 0, j = 7; i < len; i++)
	{
		if (data[i] == '\n')
			continue;

		if (data[i] == '\r')
			buffer[j++] = '\n';
		else
			buffer[j++] = data[i];
	}

	if (channel == CONSOLE)
	{
		/* Editor request ? */
		if (editor_request)
		{
			int len = 1;
			
			char buf[80];

			memset(buf, 0, sizeof(buf));

			editor_request = 0;
			editor = 1;
			buf[0] = ORB_XFBBX;
			buf[1] = 0;
			buf[3] = 0;
			switch (reply)
			{
			case 1 :	/* SR */
				len += sprintf(buf+5, "%ld", svoie[CONSOLE]->enrcur);
				buf[4] = 4;
				break;
			case 3 :	/* Sx */
				buf[4] = 3;
				break;
			default:
				buf[4] = 0;
				break;
			}
			buf[2] = len;
			orb_send_data (buf, 4+len, ORB_XFBBX);
		}

		/* Header */
		buffer[0] = ORB_CONSOLE;
		buffer[1] = 0;
		buffer[2] = (j - 4) & 0xff;
		buffer[3] = (j - 4) >> 8;

		/* Envoie les data de console au client */
		buffer[4] = CONSOLE;
		buffer[5] = color;
		buffer[6] = header;
		orb_send_data (buffer, j, ORB_CONSOLE);
	}
	else if (channel == MMONITOR)
	{
		/* Header */
		buffer[0] = ORB_MONITOR;
		buffer[1] = 0;
		buffer[2] = (j - 4) & 0xff;
		buffer[3] = (j - 4) >> 8;

		/* Envoie les data de monitoring au client */
		buffer[4] = 0xff;
		buffer[5] = color;
		buffer[6] = header;
		orb_send_data (buffer, j, ORB_MONITOR);
	}
	else
	{
		/* Header */
		buffer[0] = ORB_CHANNEL;
		buffer[1] = 0;
		buffer[2] = (j - 4) & 0xff;
		buffer[3] = (j - 4) >> 8;

		/* Envoie les data de monitoring au client */
		buffer[4] = channel - 1;
		buffer[5] = color;
		buffer[6] = header;
		orb_send_data (buffer, j, ORB_CHANNEL);
	}
	free (buffer);
}

void orb_disc (void)
{
	OrbClient *sptr = client_head;

	/* Deconnecte un client */
	while (sptr)
	{
		if (sptr->mask & ORB_CONSOLE)
		{
			if (sptr->mask & ORB_XFBBX)
			{
				char buffer[5];

				/* Envoie la commande de deconnection */
				buffer[0] = ORB_XFBBX;
				buffer[1] = 0;
				buffer[2] = 1;
				buffer[3] = 0;
				buffer[4] = 0;
				write (sptr->fd, buffer, 5);
				sptr->mask &= ~ORB_CONSOLE;
			}
			else
			{
				close (sptr->fd);
				orb_del_client (sptr);
			}
			break;
		}
		sptr = sptr->next;
	}
}

void orb_services (void)
{
	char buffer[260];
	int i;
	
	memset(buffer, 0, sizeof(buffer));
	
	/* Header */
	buffer[0] = ORB_DATA;
	buffer[1] = SVC_LIST;
	buffer[2] = SVC_MAX-1; /* Max 255 services */
	buffer[3] = 0;

	/* One service per byte */
	for (i = 1 ; i < SVC_MAX ; i++)
		buffer[4 + i - 1] = i;
	
	orb_send_data (buffer, 4 + SVC_MAX - 1, 0xffff);
}

void orb_con_list (int channel, char *ligne)
{
	char buffer[256];
	int len = strlen (ligne);

	memset(buffer, 0, sizeof(buffer));
	
	/* Header */
	buffer[0] = ORB_LISTCNX;
	buffer[1] = 0;
	buffer[2] = (len) & 0xff;
	buffer[3] = (len) >> 8;

	/* Envoie la ligne de connection du canal */
	strcpy (buffer + 4, ligne);
	orb_send_data (buffer, len + 4, ORB_LISTCNX);
}

void orb_pactor_status(int port, int p_status)
{
	char buffer[80];
	int len;

	sprintf (buffer + 4, "%d %d", port, p_status);
	len = strlen (buffer + 4);

	/* Header */
	buffer[0] = ORB_DATA;
	buffer[1] = SVC_PACTOR;
	buffer[2] = (len) & 0xff;
	buffer[3] = (len) >> 8;

	/* Send pactor status */
	orb_send_data (buffer, len + 4, ORB_STATUS);
}

void orb_con_nb (int nb)
{
	char buffer[80];
	int len;

	memset(buffer, 0, sizeof(buffer));
	
	sprintf (buffer + 4, "%d", nb);
	len = strlen (buffer + 4);

	/* Header */
	buffer[0] = ORB_NBCNX;
	buffer[1] = 0;
	buffer[2] = (len) & 0xff;
	buffer[3] = (len) >> 8;

	/* Envoie le nombre de connections */
	orb_send_data (buffer, len + 4, ORB_NBCNX);
}

void orb_nb_msg (int priv, int hold, int nbmess)
{
	char buffer[80];
	int len;

	memset(buffer, 0, sizeof(buffer));
	
	sprintf (buffer + 4, "%d %d %d", priv, hold, nbmess);
	len = strlen (buffer + 4);

	/* Header */
	buffer[0] = ORB_MSGS;
	buffer[1] = 0;
	buffer[2] = (len) & 0xff;
	buffer[3] = (len) >> 8;

	/* Envoie le nombre de messages */
	orb_send_data (buffer, len + 4, ORB_MSGS);
}

void orb_status (long lmem, long gmem, long disk1, long disk2)
{
	char buffer[80];
	int len;

	memset(buffer, 0, sizeof(buffer));
	
	sprintf (buffer + 4, "%ld %ld %ld %ld", lmem, gmem, disk1, disk2);
	len = strlen (buffer + 4);

	/* Header */
	buffer[0] = ORB_STATUS;
	buffer[1] = 0;
	buffer[2] = (len) & 0xff;
	buffer[3] = (len) >> 8;

	/* Envoie les infos de status */
	orb_send_data (buffer, len + 4, ORB_STATUS);
}

static void set_opt(char *command, char *value)
{
	int val = atoi(value);
	int ok = 1;
	
	switch(*command)
	{
	case 'B' :
		bip = val;
		break;
	case 'G' :
		gate = val;
		break;
	case 'M' :
		sed = val;
		break;
	case 'T' :
		ok_tell = val;
		break;
	case 'X' :
		aff_inexport = val;
		break;
	case 'P' :
		aff_popsmtp = val;
		break;
	default :
		ok = 0;
		break;
	}

	if (ok)	
		maj_options();
}

void orb_options(void)
{
	OrbClient *sptr = client_head;
	char *command = "OPT\n";
	char buffer[256];
	int nb = 0;
	
	memset(buffer, 0, sizeof(buffer));
	
	nb += sprintf(buffer+nb, "B %d Connection bip\n", (bip) ? 1 : 0);
	nb += sprintf(buffer+nb, "G %d Gateway\n", (gate) ? 1 : 0);
	nb += sprintf(buffer+nb, "M %d Message Editor\n", (sed) ? 1 : 0);
	nb += sprintf(buffer+nb, "T %d Talk\n", (ok_tell) ? 1 : 0);
	nb += sprintf(buffer+nb, "X %d Im/export display\n", (aff_inexport) ? 1 : 0);
	nb += sprintf(buffer+nb, "P %d POP/SMTP display\n", (aff_popsmtp) ? 1 : 0);

	while (sptr)
	{
		/* Send information only to the right clients */
		if (sptr->options)
			send_data_buf(sptr, SVC_OPT, command, buffer, nb);
		sptr = sptr->next;
	}
}

void orb_info(int val, char *str)
{
	static char buffer[512];
	static int nb = 0;

	memset(buffer, 0, sizeof(buffer));
	
	if (str)
	{
		nb += sprintf(buffer+nb, "%d %s\n", val, str);
	}
	else if (nb > 0)
	{
		OrbClient *sptr = client_head;
		char *command = "INFO\n";
		while (sptr)
		{
			/* Send information only to the right clients */
			if (sptr->infos)
				send_data_buf(sptr, SVC_INFO, command, buffer, nb);
			sptr = sptr->next;
		}
		nb = 0;
	}
}
