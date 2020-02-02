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
/* pbdir.c 1993.8.6 */

/*#ifdef __linux__
#define __a1__ __attribute__ ((packed, aligned(1)))
#else */
#define __a1__
/*#endif*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <serv.h>
#include <pbsv.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netax25/axconfig.h>

#define PFH_MSG 1
#define PFH_FILE 2

static BOOL f_beacon = OFF;
static struct stqueue actuser;
static struct stqueue freeuser;
static struct stqueue freehole;
static char rkss[FRMSIZE];

static ushort chck;
static ushort crc;

static struct stuser user[MAXUSER];
static struct sthole hole[MAXHOLE];

static long tim_0;
static long tim_1;
static long tim_2;
static long tim_3;
static int baud;

typedef struct full_sockaddr_ax25 fs_ax25;

static BOOL f_rkss;		/* pbsv.c */
static int lnrkss;
static struct stpfhdir pfhdir[MAXPFHDIR];
static char pac_call[12];
static char pac_port[12];
static int npfhdir;
static int msock = -1;			/* Rx monitoring socket */
static int pacsat;

static long to_empty;
static long to_user;

#ifdef PACDBG
static void dispdir (void);
#endif

static int add_buf (char *buf, void *data, int nb);
static uchar send_pfh (int dirent, int block_size);
static int req_dir (char *callsign);
static int pfhstat ( char *name, struct stpfh *pfh);
static void initq (struct stqueue *q);
static void putq (struct stqueue *q, struct stqcell *d);
static struct stqcell *getq (struct stqueue *q);
static void delq (struct stqueue *q, struct stqcell *d);
static void timer (void);
static void pblist (int ctl);
static void qst (void);
static void qst_dir (struct stuser *user);
static void qst_bul (struct stuser *user);
static void rcvreq (void);
static void res_msg (char *callsign, int errorcode);
static int findfile (int id);
static int compare (struct stpfhdir *d1, struct stpfhdir *d2);
static int req_bul (char *callsign);
static void set_user (struct stuser *user, char *callsign, long file_id, ushort block_size);
static void add_hole (struct stuser *user, long offset, ushort length);
static struct stuser *sch_user (char *callsign);
static void del_user (struct stuser *user);
static int add_struct (char *buf, int id, void *data, int lg_data);
static int pfh_msg (FILE * fpth, long numero);
static int mk_pfh (char *buf, bullist * pbul, long numero);
static int mk_pfh_file (long numero, int type);
static char *dlname (long numero);
static void rm_pfh_file (long numero);
static int add_crc (char *buf, int nb);
static void snd_pac (char *desti, char *buf, int len, int pid);
static int rcvkss (void);
static char *ax25_ntoaii (char *call);

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

void pbsv (void)
{
	if (pacsat == 0)
		return;

	timer ();
	while (rcvkss ())
		rcvreq ();
	qst ();
	pblist (0);
}

#define TIME(x) ((((long)(x)) << 16) / 3600L)
void init_pac (void)
{
	FILE *fptr;
	char buf[256];
	char *av1, *av2;

	if ((fptr = fopen ("init.pac", "r")) == NULL)
	{
		pacsat = 0;
/* removed warning until this protocol is used by future satellites ??? 
 * printf ("No PACSAT satellit protocol configuration file 'init.pac'\n"); */
		return;
	}

	printf ("Reading PACSAT satellite protocol configuration file 'init.pac'\n");

	/* Default values */
	strcpy (pac_call, mycall);
	if (myssid)
	{
		sprintf (pac_call + strlen (pac_call), "-%d", myssid);
	}

	n_cpy (10, pac_port, p_port[1].name);

	to_empty = TIME (2);
	to_user = TIME (10);
	baud = 1200;


	while (fgets (buf, 128, fptr) != NULL)
	{
		av1 = strtok (buf, " \t\r\n");
		strupr (av1);
		if (av1 == NULL)
			continue;

		av2 = strtok (NULL, " \t\r\n");
		if (av2 == NULL)
			continue;

		if (strcmp (av1, "CALLSIGN") == 0)
		{
			strupr (av2);
			n_cpy (10, pac_call, av2);
		}
		else if (strcmp (av1, "PORT") == 0)
		{
			n_cpy (10, pac_port, av2);

		}
		else if (strcmp (av1, "USER") == 0)
		{
			to_user = TIME (atoi (av2));
			printf ("User time-out = %s sec\n", av2);

		}
		else if (strcmp (av1, "EMPTY") == 0)
		{
			to_empty = TIME (atoi (av2));
			printf ("PB empty time-out = %s sec\n", av2);
		}
		else if (strcmp (av1, "BAUD") == 0)
		{
			baud = atoi (av2);
			printf ("Baud rate = %d\n", baud);
		}
	}

	fclose (fptr);

	printf ("Broadcast callsign [%s]\n", pac_call);
	printf ("Broadcast port     [%s]\n", pac_port);
	pacsat = 1;
}

/*
 * < initdir > initialize directory entry
 * Opens dirmes.sys and initialises the PFH list
 */
int init_pfh (void)
{
	bloc_mess *bptr = tete_dir;
	mess_noeud *mptr;
	int offset;
	bullist bul;
	int i;
	FILE *fptr;

	init_pac ();
	if (pacsat == 0)
		return 0;

	initq (&actuser);
	initq (&freeuser);
	for (i = 0; i < MAXUSER; i++)
		putq (&freeuser, (struct stqcell *)&user[i]);
	initq (&freehole);
	for (i = 0; i < MAXHOLE; i++)
		putq (&freehole, (struct stqcell *)&hole[i]);

	npfhdir = 0;

	ouvre_dir ();
	offset = 0;
	while (bptr)
	{
		mptr = &(bptr->st_mess[offset]);
		if (mptr->noenr)
		{
			read_dir (mptr->noenr, &bul);
			if ((bul.status != 'K') && (bul.status != 'A') && (npfhdir < MAXPFHDIR))
			{
				pfhdir[npfhdir].t_new = bul.date;
				pfhdir[npfhdir].file_id = bul.numero;
				pfhdir[npfhdir].pfh_type = PFH_MSG;
				npfhdir++;
			}
		}
		if (++offset == T_BLOC_MESS)
		{
			bptr = bptr->suiv;
			offset = 0;
		}
	}
	ferme_dir ();

	/* Idem for YAPPLBL.DAT */
	fptr = fopen (d_disque ("YAPPLBL.DAT"), "rb");
	if (fptr)
	{
		Rlabel rlabel;

		while (fread (&rlabel, sizeof (Rlabel), 1, fptr))
		{
			if ((*rlabel.nomfic) && (npfhdir < MAXPFHDIR))
			{
				pfhdir[npfhdir].t_new = rlabel.date_creation;
				pfhdir[npfhdir].file_id = rlabel.index;
				pfhdir[npfhdir].pfh_type = PFH_FILE;
				npfhdir++;
			}
		}
		fclose (fptr);
	}

	qsort (&pfhdir[0], npfhdir, sizeof (struct stpfhdir), (int (*)(const void *, const void *))compare);

	pfhdir[0].t_old = 0;
	for (i = 1; i < npfhdir; i++)
	{
		pfhdir[i].t_old = pfhdir[i - 1].t_new + 1;
	}
#ifdef PACDBG
	dispdir ();
#endif

	if (fbb_ax25_config_load_ports () == 0)
		fprintf (stderr, "no AX.25 port data configured\n");

	/* Creates RX monitoring socket */
	if ((msock = socket (AF_PACKET, SOCK_PACKET, htons (ETH_P_AX25))) == -1)
	{
		perror ("pacsat socket_monitoring");
	}
	else
	{
		/* Non blocking socket */
		int ioc = 1;

		ioctl (msock, FIONBIO, &ioc);
	}


	return (npfhdir);
}

void add_pfh (bullist * pbul)
{
	if (pacsat == 0)
		return;

	if ((pbul->status != 'K') && (pbul->status != 'A') && (npfhdir < MAXPFHDIR))
	{
		if (npfhdir)
			pfhdir[npfhdir].t_old = pfhdir[npfhdir - 1].t_new + 1;
		else
			pfhdir[npfhdir].t_old = 0;
		pfhdir[npfhdir].t_new = pbul->date;
		pfhdir[npfhdir].file_id = pbul->numero;

		send_pfh (npfhdir, MAXBLKSIZE);

		npfhdir++;
	}

}

static int compare (struct stpfhdir *d1, struct stpfhdir *d2)
{
	if (d1->t_new == d2->t_new)
	{
		return (0);
	}
	else if ((u_long) d1->t_new > (u_long) d2->t_new)
	{
		return (1);
	}
	else
	{
		return (-1);
	}
}

#ifdef PACDBG
/*
 * < dispdir > display directory
 */
static void dispdir (void)
{
	int i;

	for (i = 0; i < npfhdir; i++)
	{
		printf (" %4d %lu,%lu %lX %d\n", i,
				pfhdir[i].t_old, pfhdir[i].t_new, pfhdir[i].file_id,
				pfhdir[i].pfh_size);
	}
}
#endif

static int add_buf (char *buf, void *data, int nb)
{
	memcpy (buf, data, nb);
	return (nb);
}

/*
 * < send_pfh > send PFH
 */
static uchar send_pfh (int dirent, int block_size)
{
	char buf[256];
	char out[256];
	char *pbuf;
	uchar flags;
	int psize, size, nb;
	long file_id, offset;

	file_id = pfhdir[dirent].file_id;
	psize = mk_pfh (buf, NULL, file_id);

	flags = 0x00;
	if ((dirent + 1) == npfhdir)
		flags |= 0x40;
	offset = 0;
	for (;;)
	{
		size = psize - offset;
		if (size <= block_size)
		{
			flags |= 0x20;
		}
		else
		{
			size = block_size;
		}

		pbuf = buf + offset;
		nb = 0;
		nb += add_buf (&out[nb], &flags, 1);
		nb += add_buf (&out[nb], &file_id, 4);
		nb += add_buf (&out[nb], &offset, 4);
		nb += add_buf (&out[nb], &pfhdir[dirent].t_old, 4);
		nb += add_buf (&out[nb], &pfhdir[dirent].t_new, 4);
		nb += add_buf (&out[nb], pbuf, size);
		nb = add_crc (out, nb);
		snd_pac ("QST-1", out, nb, 0xbd);
		if (flags & 0x20)
		{
			break;
		}
		offset += size;
	}
	return (flags);
}

/*
 * < req_dir > request directory
 */
static int req_dir (char *callsign)
{
	u_char flags;
	ushort block_size;
	struct stuser *user;
	struct sthole *hole;
	time_t start, end;
	int i;

	i = HDRSIZE;
	flags = rkss[i];
	i += 1;
	memcpy (&block_size, &rkss[i], 2);
	i += 2;

	user = (struct stuser *) getq (&freeuser);
	if (user == NULL)
	{
		return (-1);			/* queue full */
	}
	user->entry_t = time (NULL);
	user->flags = F_DIR;
	strcpy (user->call, callsign);
	user->block_size = block_size;
	initq (&user->hole);
	putq (&actuser, (struct stqcell *)user);

	for (; i < lnrkss; i += (4 + 4))
	{
		memcpy (&start, &rkss[i], 4);
		memcpy (&end, &rkss[i + 4], 4);
		hole = (struct sthole *) getq (&freehole);
		if (hole == NULL)
			break;
		hole->start = start;
		hole->end = end;
		putq (&user->hole, (struct stqcell *)hole);
	}
	return (0);					/* OK */
}

/*
 * < pfhstat > PFH stat
 */
static int pfhstat ( char *name, struct stpfh *pfh)
{
	FILE *fp;
	int c, i, s, length;
	char data[256];
	ushort id;

	s = 0;

	if ((fp = fopen (name, "rb")) == NULL)
	{
		return (-1);
	}
	c = fgetc (fp);
	s++;
	id = (c << 8);
	c = fgetc (fp);
	s++;
	id |= c;
	if (id != 0xaa55)
	{							/* PFH check ? */
		fclose (fp);
		return (-1);
	}
	for (;;)
	{
		c = fgetc (fp);
		if (c == EOF)
			break;
		id = c;
		c = fgetc (fp);
		id |= (c << 8);
		c = fgetc (fp);
		length = c;
		s += 3;
		if (id == 0 && length == 0)		/* termination ? */
			break;
		for (i = 0; i < length; i++)
		{
			data[i] = fgetc (fp);
			s++;
		}
		switch (id)
		{
		case 0x04:
			memcpy (&pfh->file_size, data, 4);
			break;
		case 0x08:
			pfh->file_type = data[0];
			break;
		case 0x0b:
			memcpy (&pfh->body_offset, data, 2);
			break;
		default:
			break;
		}
	}
	fclose (fp);
	pfh->pfh_size = s;
	return (0);
}

/*
 * < initq > initialize queue
 */
static void initq (struct stqueue *q)
{
	q->head = q->tail = NULL;
}

/*
 * < putq > put queue
 */
static void putq (struct stqueue *q, struct stqcell *d)
{
	if (q == NULL || d == NULL)
	{
		assert (0);
	}
	d->next = NULL;
	if (q->head == NULL)
	{
		q->head = q->tail = d;
	}
	else
	{
		q->tail->next = d;
		q->tail = d;
	}
}

/*
 * < getq > get queue
 */
static struct stqcell *getq (struct stqueue *q)
{
	struct stqcell *r;

	if (q == NULL)
	{
		assert (0);
	}
	r = q->head;
	if (r != NULL)
	{
		q->head = r->next;
	}
	if (q->head == NULL)
	{
		q->tail = NULL;
	}
	return (r);
}

/*
 * < delq > delete queue
 */
static void delq (struct stqueue *q, struct stqcell *d)
{
	struct stqcell *d2;
	struct stqueue temp;

	initq (&temp);
	while ((d2 = getq (q)) != NULL)
	{
		if (d2 != d)
		{
			putq (&temp, d2);
		}
	}
	*q = temp;
}

/*
 * < timer > timer
 */
static void timer (void)
{
	static long t1;
	long clk, t;

	clk = btime ();
	t = clk - t1;
	t1 = clk;

	if ((u_long) tim_0 > (u_long) t)
	{
		tim_0 -= t;
	}
	else
		tim_0 = 0;

	if ((u_long) tim_1 > (u_long) t)
	{
		tim_1 -= t;
	}
	else
		tim_1 = 0;

	if ((u_long) tim_2 > (u_long) t)
	{
		tim_2 -= t;
	}
	else
		tim_2 = 0;

	t *= 100;
	if ((u_long) tim_3 > (u_long) t)
	{
		tim_3 -= t;
	}
	else
		tim_3 = 0;
}

/*
 * < pblist > PB list
 */
static void pblist (int ctl)
{
	VOID add_kss (char *, int);

	char buf[256];
	int n;
	struct stuser *user;

	if (f_beacon)
	{
		f_beacon = OFF;
		tim_0 = 0;
	}
	if (ctl == 1)
		tim_0 = 0;
	else
	{
		if (tim_0 != 0 || actuser.head != NULL)
			return;
	}

	if (tim_0 != 0)
		return;

	tim_0 = to_empty;

	strcpy (buf, "PB:");
	user = (struct stuser *) actuser.head;
	n = 0;
	if (user == NULL)
	{
		strcat (buf, " Empty.");
	}
	else
	{
		while (user != NULL)
		{
			strcat (buf, " ");
			strcat (buf, user->call);
			if (user->flags & F_DIR)
				strcat (buf, "\\D");
			user = user->next;
			n++;
		}
	}
	strcat (buf, "\r");
	snd_pac ("PBLIST", buf, strlen (buf), 0xf0);
}

/*
 * < qst >
 */
static void qst (void)
{
	struct stuser *user;

	if (actuser.head == NULL)
		return;

	if (tim_3)
	{
		return;
	}

	pblist (1);
	user = (struct stuser *) getq (&actuser);

	/* check time over ? */
	if ((time (NULL) - user->entry_t) > to_user)
	{
		del_user (user);
		return;
	}

	if (user->flags & F_DIR)
	{
		qst_dir (user);
	}
	else
	{
		qst_bul (user);
	}

	if (user->hole.head == NULL)
	{
		putq (&freeuser, (struct stqcell *)user);
		/* Termine... Supprime le fichier si pas utilise */
		rm_pfh_file (user->file_id);
	}
	else
	{
		putq (&actuser, (struct stqcell *)user);
	}

	if (actuser.head == NULL)
	{
		pblist (1);
	}
}

/*
 * < qst_dir > directory
 */
static void qst_dir (struct stuser *user)
{
	struct sthole *hole;
	u_char flags;
	int i, j, dirent;
	int nb_blk = (baud / 600);

	if (nb_blk > 10)
		nb_blk = 10;

	for (i = 0; i < nb_blk; i++)
	{
		hole = (struct sthole *) user->hole.head;
		if (hole == NULL)
			break;

		dirent = npfhdir - 1;
		for (j = 0; j < npfhdir; j++)
		{
			if ((u_long) hole->start >= (u_long) pfhdir[j].t_old &&
				(u_long) hole->start <= (u_long) pfhdir[j].t_new)
			{
				dirent = j;
				break;
			}
		}
		flags = send_pfh (dirent, user->block_size);
		hole->start = pfhdir[dirent].t_new + 1;

		if ((u_long) hole->start > (u_long) hole->end || (flags & 0x40))
		{						/* hole delete ? */
			hole = (struct sthole *) getq (&user->hole);
			putq (&freehole, (struct stqcell *)hole);
		}
	}
}

/*
 * < qst_bul >
 */
static void qst_bul (struct stuser *user)
{
	struct sthole *hole;
	char name[128];
	char buf[256];
	char out[256];
	FILE *fp;
	char flags, file_type;
	long file_id, offset;
	int i;
	int nb;
	int size;
	int nb_blk = (baud / 600);

	if (nb_blk > 10)
		nb_blk = 10;

	file_id = user->file_id;
	file_type = user->file_type;

	strcpy (name, dlname (file_id));
	if ((fp = fopen (name, "rb")) == NULL)
	{
		printf ("QST-1 file[%s] open error\n", name);
		exit (1);
	}
	flags = 0x02;
	for (i = 0; i < nb_blk; i++)
	{
		hole = (struct sthole *) user->hole.head;
		if (hole == NULL)
		{
			break;
		}
		offset = hole->offset;
		if (user->block_size > hole->length)
		{
			size = hole->length;
		}
		else
		{
			size = user->block_size;
		}
		hole->offset += (long) size;
		hole->length -= size;
		if (hole->length == 0)
		{
			hole = (struct sthole *) getq (&user->hole);
			putq (&freehole, (struct stqcell *)hole);
		}

		fseek (fp, offset, SEEK_SET);
		fread (buf, size, 1, fp);

		nb = 0;
		nb += add_buf (&out[nb], &flags, 1);
		nb += add_buf (&out[nb], &file_id, 4);
		nb += add_buf (&out[nb], &file_type, 1);
		nb += add_buf (&out[nb], &offset, 3);
		nb += add_buf (&out[nb], buf, size);
		nb = add_crc (out, nb);
		snd_pac ("QST-1", out, nb, 0xbb);
	}
	fclose (fp);
}

/*
 * < rcvreq > receive request
 */
static void rcvreq (void)
{
	char callsign[12];
	int r;
	u_char pid;

	if (!f_rkss)
		return;

	f_rkss = OFF;

	if (lnrkss <= HDRSIZE)
		return;

	pid = rkss[HDRSIZE - 1] & 0xff;
	/* Verify if the request is for me */
	if (strcmp (pac_call, ax25_ntoaii (rkss + 1)) != 0)
	{
		return;
	}
	strcpy (callsign, ax25_ntoaii (rkss + 1 + ADRSIZE));
	del_user (sch_user (callsign));
	switch (pid)
	{
	case 0xbb:					/* pid = bb */
		r = req_bul (callsign);
		break;
	case 0xbd:					/* pid = bd */
		r = req_dir (callsign);
		break;
	default:
		return;
	}
	res_msg (callsign, r);
}


/*
 * < res_msg > response messages
 */
static void res_msg (char *callsign, int errorcode)
{
	char buf[64];

	if (errorcode == 0)
	{
		sprintf (buf, "OK %s\r", callsign);
	}
	else
	{
		sprintf (buf, "NO %d %s\r", errorcode, callsign);
	}
	snd_pac (callsign, buf, strlen (buf), 0xf0);
}

static int findfile (int id)
{
	FILE *fptr;
	Rlabel rlabel;
	int ret = -1;
	int record = 0;

	/* Seach file in the existing labels */

	if ((fptr = fopen (d_disque ("YAPPLBL.DAT"), "rb")) == NULL)
		return (ret);

	while (fread (&rlabel, sizeof (Rlabel), 1, fptr))
	{
		if ((*rlabel.nomfic) && (rlabel.index == id))
		{
			ret = record;
			break;
		}
		++record;
	}

	fclose (fptr);

	return (ret);
}

/*
 * < req_bul > request
 */
static int req_bul (char *callsign)
{
	uchar flags;
	long file_id, offset;
	ushort block_size, length;
	int i;
	int file_type;
	struct stuser *user;

	i = HDRSIZE;
	flags = rkss[i];
	i += 1;
	memcpy (&file_id, &rkss[i], 4);
	i += 4;
	memcpy (&block_size, &rkss[i], 2);
	i += 2;

	if (findmess (file_id) == NULL)
	{
		/* Not a msg... Is it a file ? */
		if (findfile (file_id) != -1)
		{
			file_type = PFH_FILE;
		}
		else
		{
			/* None exist */
			return (-2);
		}
	}
	else
	{
		file_type = PFH_MSG;
	}

	user = (struct stuser *) getq (&freeuser);
	if (user == NULL)
	{
		return (-1);			/* queue full */
	}

	/* Create the DL file */
	if (mk_pfh_file (file_id, file_type) == 0)
		return (-2);

	if (access (dlname (file_id), 00) != 0)
	{							/* DL file exist ? */
		return (-2);			/* file does not exist */
	}

	switch (flags & 0x03)
	{
	case 0x00:					/* start */
		set_user (user, callsign, file_id, block_size);
		offset = 0;
		length = block_size * 10;
		for (i = 0; i < 10; i++)
		{
			add_hole (user, offset, length);
			offset += length;
		}
		break;

	case 0x01:					/* stop */
		break;

	case 0x02:					/* hole list */
		set_user (user, callsign, file_id, block_size);
		for (; i < lnrkss; i += (3 + 2))
		{
			offset = 0;
			memcpy (&offset, &rkss[i], 3);
			memcpy (&length, &rkss[i + 3], 2);
			add_hole (user, offset, length);
		}
		break;

	default:
		return (-4);			/* debug */
		break;
	}
	return (0);					/* OK */
}

/*
 * < set_user > set user list
 */
static void set_user (struct stuser *user, char *callsign, long file_id, ushort block_size)
{
	struct stat sbuf;
	struct stpfh pfh;
	char pathname[128];

	strcpy (pathname, dlname (file_id));
	stat (pathname, &sbuf);
	pfhstat (pathname, &pfh);

	user->entry_t = time (NULL);
	user->flags = 0x0000;
	strcpy (user->call, callsign);
	user->file_id = file_id;
	user->file_type = pfh.file_type;
	user->file_size = sbuf.st_size;
	if (block_size > MAXBLKSIZE)
	{							/* block size adjust */
		user->block_size = MAXBLKSIZE;
	}
	else
	{
		user->block_size = block_size;
	}
	initq (&user->hole);
	putq (&actuser, (struct stqcell *)user);
}

/*
 * < add_hole >
 */
static void add_hole (struct stuser *user, long offset, ushort length)
{
	struct sthole *hole;

	if (offset >= user->file_size)
		return;

	if (offset + length > user->file_size)	/* adjust */
		length = user->file_size - offset;

	hole = (struct sthole *) getq (&freehole);
	if (hole == NULL)
	{
		assert (0);
	}
	hole->offset = offset;
	hole->length = length;
	putq (&user->hole, (struct stqcell *)hole);
}

/*
 * < sch_user > search user
 */
static struct stuser *sch_user (char *callsign)
{
	struct stuser *user;

	user = (struct stuser *) actuser.head;
	while (user)
	{
		if (strcmp (user->call, callsign) == 0)
		{
			break;
		}
		user = (struct stuser *) user->next;
	}
	return (user);
}

/*
 * < del_user > delete user
 */
static void del_user (struct stuser *user)
{
	struct sthole *hole;

	if (user == NULL)
		return;

	while ((hole = (struct sthole *) getq (&user->hole)) != NULL)
	{
		putq (&freehole, (struct stqcell *)hole);
	}
	delq (&actuser, (struct stqcell *)user);
	putq (&freeuser, (struct stqcell *)user);
}

static int add_struct (char *buf, int id, void *data, int lg_data)
{
	*buf++ = id & 0xff;
	*buf++ = id >> 8;
	*buf++ = lg_data;
	memcpy (buf, data, lg_data);
	lg_data += 3;

	return (lg_data);
}

static int pfh_msg (FILE * fpth, long numero)
{
	/* int verbose = 1; */
	FILE *fptm;
	int fd_msg;
	int fd_pfh;
	int i, nb, wr, tot, lgbuf;
	uchar buf[4096];

	if (fpth)
	{
		fflush (fpth);
		fd_pfh = fileno (fpth);
	}
	else
		fd_pfh = 0;

	if ((fptm = ouvre_mess (O_TEXT, numero, '\0')) != NULL)
	{
		fd_msg = fileno (fptm);
	}
	else
		return 0;

	crc = 0;
	tot = 0;
	wr = 0;
	lgbuf = sizeof (buf);

	for (;;)
	{
		nb = read (fd_msg, buf, lgbuf);
		if (nb <= 0)
			break;
		if (fd_pfh)
			wr = write (fd_pfh, buf, nb);
		for (i = 0; i < nb; i++)
		{
			crc += buf[i];
		}
		tot += nb;
	}

	fclose (fptm);

	return tot;
}

struct pfh1
{
	unsigned short id __a1__;
	uchar len __a1__;
	uchar val __a1__;
};

struct pfh2
{
	unsigned short id __a1__;
	uchar len __a1__;
	unsigned short val __a1__;
};

struct pfh3
{
	unsigned short id __a1__;
	uchar len __a1__;
	uchar val[3] __a1__;
};

struct pfh4
{
	unsigned short id __a1__;
	uchar len __a1__;
	unsigned long val __a1__;
};

struct pfh8
{
	unsigned short id __a1__;
	uchar len __a1__;
	uchar val[8] __a1__;
};

struct pfh
{
	unsigned short magic_number __a1__;
	struct pfh4 file_number __a1__;
	struct pfh8 file_name __a1__;
	struct pfh3 file_ext __a1__;
	struct pfh4 file_size __a1__;
	struct pfh4 create_time __a1__;
	struct pfh4 last_modif_time __a1__;
	struct pfh1 seu_flag __a1__;
	struct pfh1 file_type __a1__;
	struct pfh2 body_checksum __a1__;
	struct pfh2 header_checksum __a1__;
	struct pfh2 body_offset __a1__;
};

static int mk_pfh (char *buf, bullist * pbul, long numero)
{
	bullist bul;
	mess_noeud *lptr;
	int lg;
	int i;
	int nb;
	int zero = 0;
	struct pfh *pfh = (struct pfh *) buf;
	uchar *ptr;
	char lbuf[80];

	if (pbul == NULL)
	{
		lptr = findmess (numero);
		if (lptr == NULL)
			return (0);

		ouvre_dir ();
		read_dir (lptr->noenr, &bul);
		ferme_dir ();

		pbul = &bul;
	}

	lg = pfh_msg (NULL, pbul->numero);

	pfh->magic_number = 0x55aa;

	pfh->file_number.id = 1;
	pfh->file_number.len = 4;
	pfh->file_number.val = pbul->numero;

	pfh->file_name.id = 2;
	pfh->file_name.len = 8;
	sprintf (pfh->file_name.val, "        ");

	pfh->file_ext.id = 3;
	pfh->file_ext.len = 3;
	sprintf (pfh->file_ext.val, "   ");

	pfh->create_time.id = 5;
	pfh->create_time.len = 4;
	pfh->create_time.val = pbul->date;

	pfh->last_modif_time.id = 6;
	pfh->last_modif_time.len = 4;
	pfh->last_modif_time.val = 0;

	pfh->seu_flag.id = 7;
	pfh->seu_flag.len = 1;
	pfh->seu_flag.val = 0;

	pfh->file_type.id = 8;
	pfh->file_type.len = 1;
	pfh->file_type.val = 0;

	pfh->body_checksum.id = 9;
	pfh->body_checksum.len = 2;
	pfh->body_checksum.val = crc;

	pfh->header_checksum.id = 0xa;
	pfh->header_checksum.len = 2;
	pfh->header_checksum.val = 0;

	nb = sizeof (struct pfh);
	nb += add_struct (&buf[nb], 0x10, pbul->exped, strlen (pbul->exped));

	sprintf (lbuf, "%-6s", pbul->exped);
	nb += add_struct (&buf[nb], 0x11, lbuf, 6);
	nb += add_struct (&buf[nb], 0x12, &pbul->date, 4);
	nb += add_struct (&buf[nb], 0x13, &pbul->nblu, 1);

	if (*pbul->bbsv)
		sprintf (lbuf, "%s@%s", pbul->desti, pbul->bbsv);
	else
		sprintf (lbuf, "%s", pbul->desti);
	nb += add_struct (&buf[nb], 0x14, lbuf, strlen (lbuf));
	nb += add_struct (&buf[nb], 0x15, "      ", 6);
	nb += add_struct (&buf[nb], 0x16, &zero, 4);
	nb += add_struct (&buf[nb], 0x17, &zero, 4);
	nb += add_struct (&buf[nb], 0x18, &zero, 1);
	nb += add_struct (&buf[nb], 0x19, &zero, 1);
	nb += add_struct (&buf[nb], 0x20, &pbul->type, 1);
	nb += add_struct (&buf[nb], 0x21, pbul->bid, strlen (pbul->bid));
	nb += add_struct (&buf[nb], 0x22, pbul->titre, strlen (pbul->titre));

	buf[nb++] = '\0';
	buf[nb++] = '\0';
	buf[nb++] = '\0';

	pfh->file_size.id = 4;
	pfh->file_size.len = 4;
	pfh->file_size.val = lg + nb;

	pfh->body_offset.id = 0xb;
	pfh->body_offset.len = 2;
	pfh->body_offset.val = (unsigned short) nb;

	/* Computing header checksum */
	ptr = (char *) pfh;
	for (chck = 0, i = 0; i < nb; ptr++, i++)
	{
		chck += 0xff & (unsigned short) *ptr;
	}
	pfh->header_checksum.val = chck;

	return (nb);
}

static int mk_pfh_file (long numero, int type)
{
	int nb = 0;
	FILE *fptr;
	char header[256];

	if (type == PFH_MSG)
	{
		nb = mk_pfh (header, NULL, numero);
		if (nb == 0)
			return (0);

		if (access (dlname (numero), 00) == 0)
			return (1);

		fptr = fopen (dlname (numero), "wb");
		if (fptr == NULL)
			return (0);

		fwrite (header, nb, 1, fptr);

		nb = pfh_msg (fptr, numero);

		fclose (fptr);
	}

	if (type == PFH_FILE)
	{
		nb = 0;
	}

	return nb;
}

/*
 * < dlname > pathname DL file
 */
static char *dlname (long numero)
{
	static char pathname[128];

	sprintf (pathname, "%s%lx.dl", MBINDIR, numero);
	return (pathname);
}

static void rm_pfh_file (long numero)
{
	struct stuser *user;

	user = (struct stuser *) actuser.head;

	while (user)
	{
		if (((user->flags & F_DIR) == 0) && (user->file_id == numero))
		{
			/* used by another user */
			return;
		}
		user = user->next;
	}

	/* unlink the DL file */
	unlink (dlname (numero));
}

static int add_crc (char *buf, int nb)
{
	int j;
	short crc;

	for (j = 0, crc = 0; j < nb; j++)
	{
		crc = calc_crc (buf[j], crc);
	}
	buf[j++] = (crc >> 8) & 0xff;
	buf[j++] = crc & 0xff;
	return (j);
}

static void snd_pac (char *desti, char *buf, int len, int pid)
{
	static int s_f0 = -1;
	static int s_bb = -1;
	static int s_bd = -1;

	int slen;
	int dlen;
	fs_ax25 dst;
	fs_ax25 src;
	char *call = NULL;
	int s = -1;
	long temps;

	switch (pid)
	{
	case 0xbb:
		s = s_bb;
		break;
	case 0xbd:
		s = s_bd;
		break;
	case 0xf0:
		s = s_f0;
		break;
	}

	if (s == -1)
	{
		slen = ax25_aton (pac_call, &src);
		call = ax25_config_get_addr (pac_port);
		if (call == NULL)
		{
			printf ("Cannot find a callsign for port %s\n",
					pac_port);
			return;
		}
		ax25_aton_entry (call, src.fsa_digipeater[0].ax25_call);
		src.fsa_ax25.sax25_ndigis = 1;

		if ((s = socket (AF_AX25, SOCK_DGRAM, pid)) == -1)
		{
			perror ("beacon:socket");
			return;

		}
		if (bind (s, (struct sockaddr *) &src, slen) == -1)
		{
			perror ("beacon:bind");
			return;
		}

		switch (pid)
		{
		case 0xbb:
			s_bb = s;
			break;
		case 0xbd:
			s_bd = s;
			break;
		case 0xf0:
			s_f0 = s;
			break;
		}

	}
	if ((dlen = ax25_aton (desti, &dst)) == -1)
	{
		fprintf (stderr, "beacon: unable to convert '%s'\n", call);
		return;
	}

	if (sendto (s, buf, len, 0, (struct sockaddr *) &dst, dlen) == -1)
	{
		perror ("beacon: sendto");
		return;
	}

	temps = (TIME ((len + HDRSIZE))) * 1000 / baud;

	tim_3 += temps;
}

static int rcvkss (void)
{
	struct sockaddr sa;
	unsigned asize = sizeof (sa);
	int lg;
	char *ptr;

	lnrkss = 0;

	if (msock == -1)
		return (0);

	lg = recvfrom (msock, rkss, FRMSIZE, 0, &sa, &asize);
	if (lg <= 0)
		return (0);

	if ((ptr = ax25_config_get_name (sa.sa_data)) != NULL)
	{
		/* printf("Received UI on port %s, lg=%d\n", ptr, lg); */
	}

	lnrkss = lg;
	f_rkss = ON;
	return (lg);
}
static char *ax25_ntoaii (char *call)
{
	char *scan;
	char *ptr = _ax25_ntoa ((ax25_address *)call);
	scan = strrchr (ptr, '-');
	if ((scan) && (*(scan + 1) == '0'))
		*scan = '\0';
	return (ptr);
}
