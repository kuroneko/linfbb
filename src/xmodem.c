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
 *    MODULE X, Y, ZMODEM
 */

#include <serv.h>
#include <modem.h>

#ifdef __linux__
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

#ifdef __WINDOWS__
char *xmodem_str (int voie, char *s)
{
#define XMODLEN 44
	static char stdesc[10][11] =
	{
		"SendInit  ",
		"SendData  ",
		"SendData  ",
		"SendEof   ",
		"RcvInit   ",
		"RcvLabel  ",
		"RcvData   ",
		"RcvEnd    ",
		"XmodmAbort",
		"ZsInit    "
	};

	static char xmod_name[4][5] =
	{"Xmod", "Xm1K", "Ymod", "Zmod"};
	char taille[40];
	int n;
	int niv = svoie[voie]->niv3;
	long offset = svoie[voie]->enrcur;

	if (offset < 0L)
		offset = 0L;

	*s = '\0';
	if ((niv >= 0) && (niv < 10))
	{
		if (svoie[voie]->tailm)
		{
			sprintf (taille, "/%ld", svoie[voie]->tailm);
		}
		else
		{
			*taille = '\0';
		}
		sprintf (s, "%s:%s %s %ld%s",
				 xmod_name[svoie[voie]->type_yapp],
				 stdesc[niv], svoie[voie]->appendf,
				 offset, taille);
	}
	return (s);
}
#else
static void aff_xmodem (int ok)
{
#define XMODLEN 44
	static char stdesc[10][11] =
	{
		"SendInit  ",
		"SendData  ",
		"SendData  ",
		"SendEof   ",
		"RcvInit   ",
		"RcvLabel  ",
		"RcvData   ",
		"RcvEnd    ",
		"XmodmAbort",
		"ZsInit    "
	};

	static char xmod_name[4][5] =
	{"Xmod", "Xm1K", "Ymod", "Zmod"};
	char s[80];
	char taille[40];
	int n;
	int niv = pvoie->niv3;
	long offset = pvoie->enrcur;

	if (offset < 0L)
		offset = 0L;

	*s = '\0';
	if (ok)
	{
		if ((niv >= 0) && (niv < 10))
		{
			if (pvoie->tailm)
			{
				sprintf (taille, "/%ld", pvoie->tailm);
			}
			else
			{
				*taille = '\0';
			}
			sprintf (s, "%s:%s %s %ld%s",
					 xmod_name[pvoie->type_yapp],
					 stdesc[niv], pvoie->appendf,
					 offset, taille);
		}
	}
	for (n = strlen (s); n < XMODLEN; n++)
		s[n] = ' ';
	s[XMODLEN] = '\0';
	aff_chaine (W_DEFL, 17, 3, s);
}
#endif

void display_dsz_perf (int voie)
{
	int save_voie;
	FILE *fptr;
	long lg_file;
	Ylist *temp;
	Ylist *yptr;
	char ligne[256];

	selvoie (voie);

	cr ();

	pvoie->size_trans = 0L;

	fptr = fopen (svoie[voie]->ch_temp, "rt");
	if (fptr == NULL)
		return;

	yptr = svoie[voie]->ytete;
	while (yptr)
	{
		char tmp1[80];
		char tmp2[80];
		char tmp3[80];

		temp = yptr;
		strcpy (svoie[voie]->appendf, yptr->filename);
		fgets (ligne, sizeof (ligne), fptr);
		WinDebug ("Res -> %s\r\n", ligne);
		sscanf (ligne, "%s %s %s %s %s",
				tmp1, varx[0], tmp2, tmp3, varx[1]);
		lg_file = atol (varx[0]);
		pvoie->size_trans += lg_file;

		if (pvoie->kiss == -2)
		{
			save_voie = voiecur;
			selvoie (CONSOLE);
			texte (T_DOS + 5);
			selvoie (save_voie);
		}
		else
			texte (T_DOS + 5);

		yptr = yptr->next;
		m_libere (temp, sizeof (Ylist));
	}
	svoie[voie]->ytete = NULL;

	fclose (fptr);
	unlink (svoie[voie]->ch_temp);
	sprintf (ligne, "%sDSZFIL.%d", MBINDIR, voiecur);
	unlink (ligne);
}

void xmodem_mode (int mode, int voie)
{
#ifdef __FBBDOS__
	RSEGS rsegs;
	int port = no_port (voie);

	memset (&rsegs, 0, sizeof (RSEGS));
	rsegs.DX = (int) p_port[port].ccom - 1;
	rsegs.AX = 0x0e00 | (mode & 0xff);
	int14real (&rsegs);
#endif
}


static void zmodem_tx_on (int voie, int type, int lg)
{
#ifdef __FBBDOS__
	RSEGS rsegs;
	int port = no_port (voie);

	memset (&rsegs, 0, sizeof (RSEGS));
	rsegs.DX = (int) p_port[port].ccom - 1;
	rsegs.AX = 0x0e00 | ((type) ? 0x9 : 0x8);
	rsegs.CX = lg;
	int14real (&rsegs);
#endif
}


void get_zrpos (int voie)
{
#ifdef __FBBDOS__
	RSEGS rsegs;
	int port = no_port (voie);
	char str[80];

	selvoie (voie);
	memset (&rsegs, 0, sizeof (RSEGS));
	rsegs.DX = (int) p_port[port].ccom - 1;
	rsegs.AX = 0x0e0a;
	rsegs.BX = 0x00;
	int14real (&rsegs);

	svoie[voie]->enrcur = (((long) rsegs.BX) << 16) | (((long) rsegs.AX) & 0xffff);

	if (svoie[voie]->enrcur)
	{
		sprintf (str, "ZRPOS to %ld\r", svoie[voie]->enrcur);
		aff_bas (voie, W_RCVT, str, strlen (str));
	}

	clear_outbuf (voie);
	svoie[voie]->stop = 0;
	ch_niv3 (XS_SEND);
	xmodem ();
#endif
}


static char *ymodem_header (int voie, long date, int *lg)
{
	static char ybuf[128];
	char *ptr;
	int len = 0;

	memset (ybuf, 0, 128);

	strcpy (ybuf, svoie[voie]->appendf);

	ptr = ybuf;
	while (*ptr)
	{
		++ptr;
		++len;
	}
	++ptr;
	++len;
	sprintf (ptr, "%ld %lo ", svoie[voie]->tailm, date);
	*lg = strlen (ptr) + len + 1;

	return (ybuf);
}

void libere_ymodem (int voie, int display)
{
	Ylist *yptr, *temp;

	yptr = svoie[voie]->ytete;

	while (yptr)
	{
		temp = yptr;
		if ((display) && (yptr->ok == 1))
		{
			svoie[voie]->size_trans = temp->size_trans;
			svoie[voie]->time_trans = temp->time_trans;
			strcpy (svoie[voie]->appendf, temp->filename);
			display_perf (voie);
		}
		yptr = yptr->next;
		m_libere (temp, sizeof (Ylist));
	}

	svoie[voie]->ytete = NULL;
}

static int ymodem_files (void)
{
	int nb = 0;
	Ylist *yptr;
	struct stat bufstat;

	libere_ymodem (voiecur, 0);

	yptr = pvoie->ytete;

	strtok (indd, " \r");

	indd[40] = '\0';

	if ((indd = strtok (NULL, " \r")) == NULL)
	{
		texte (T_ERR + 20);
		retour_appel ();
		return (0);
	}

	do
	{

		if ((tst_point (indd)) &&
			(stat (nom_yapp (), &bufstat) != -1) &&
			((bufstat.st_mode & S_IFREG) != 0))
		{
			if (yptr)
			{
				yptr->next = (Ylist *) m_alloue (sizeof (Ylist));
				yptr = yptr->next;
			}
			else
			{
				pvoie->ytete = yptr = (Ylist *) m_alloue (sizeof (Ylist));
			}
			n_cpy (255, yptr->filename, pvoie->appendf);
			++nb;
		}
		else
			texte (T_ERR + 11);

	}
	while ((indd = strtok (NULL, " \r")) != NULL);

	yptr = pvoie->ytete;
	if (yptr)
	{
		*pvoie->appendf = '\0';
		while (yptr)
		{
			strcat (pvoie->appendf, yptr->filename);
			yptr = yptr->next;
			if (yptr)
				strcat (pvoie->appendf, " ");
		}
		texte (T_YAP + 0);
		aff_etat ('E');
		send_buf (voiecur);
		sleep_ (1);
	}
	else
	{
		retour_appel ();
	}
	return (nb);
}

static void zmodem_init (void)
{
	static char zrqinit[25] = "rz\r**\030B00000000000000\r\n\021";

	md_no_echo (voiecur);
	outs (zrqinit, strlen (zrqinit));
	ch_niv3 (ZS_FILE);
}

static int zmodem_tx (int voie)
{
	int lg;
	int retour = 0;
	struct stat bufstat;
	Ylist *yptr;
	char *ptr;


	yptr = svoie[voie]->ytete;

	while ((yptr) && (yptr->ok))
		yptr = yptr->next;

	while ((retour == 0) && (yptr))
	{

		indd = yptr->filename;

		if ((tst_point (indd)) && (stat (nom_yapp (), &bufstat) != -1) &&
			((bufstat.st_mode & S_IFREG) != 0))
		{
			while (!modem_vide (voie))
				;
			sleep_ (1);
			pvoie->enrcur = -1L;
			pvoie->tailm = file_size (pvoie->sr_fic);
			set_binary (voiecur, 1);
			pvoie->xferok = 1;
			pvoie->size_trans = 0L;
			pvoie->time_trans = 0L;
			prog_more (voie);
			ptr = ymodem_header (voie, bufstat.st_ctime, &lg);
			zmodem_tx_on (voie, (yptr->next != NULL), lg);
			outs (ptr, lg);
			svoie[voie]->stop = 0;
			ch_niv3 (XS_WAIT);
			retour = 1;
		}
		else
		{
			yptr->ok = -1;
			yptr = yptr->next;
		}
	}
	return (retour);
}


static int ymodem_tx (int voie)
{
	int lg;
	int retour = 0;
	struct stat bufstat;
	Ylist *yptr;
	char *ptr;


	yptr = svoie[voie]->ytete;

	while ((yptr) && (yptr->ok))
		yptr = yptr->next;

	while ((retour == 0) && (yptr))
	{

		indd = yptr->filename;

		if ((tst_point (indd)) && (stat (nom_yapp (), &bufstat) != -1) &&
			((bufstat.st_mode & S_IFREG) != 0))
		{
			while (!modem_vide (voie))
				;
			sleep_ (1);
			pvoie->enrcur = 0L;
			pvoie->tailm = file_size (pvoie->sr_fic);
			set_binary (voiecur, 1);
			pvoie->xferok = 1;
			pvoie->size_trans = 0L;
			pvoie->time_trans = 0L;
			prog_more (voie);
			ptr = ymodem_header (voie, bufstat.st_ctime, &lg);
			ymodem_tx_on (voie, (yptr->next != NULL));
			outs (ptr, 128);
			if (senddata (1))
			{
				ch_niv3 (XS_QUEUE);
			}
			else
				ch_niv3 (XS_SEND);
			retour = 1;
		}
		else
		{
			yptr->ok = -1;
			yptr = yptr->next;
		}
	}
	return (retour);
}


#ifdef __FBBDOS__
static int xmodem_tx (int voie)
{
	int retour = 0;
	struct stat bufstat;
	char *ptr;

	strtok (indd, " \r");
	if ((indd = strtok (NULL, " \r")) == NULL)
	{
		texte (T_ERR + 20);
		retour_appel ();
	}
	else if ((tst_point (indd)) && (stat (nom_yapp (), &bufstat) != -1) &&
			 ((bufstat.st_mode & S_IFREG) != 0))
	{
		texte (T_YAP + 0);
		aff_etat ('E');
		send_buf (voie);
		while (!modem_vide (voie))
			;
		sleep_ (1);
		pvoie->enrcur = 0L;
		if ((ptr = strtok (NULL, " \r")) != NULL)
			pvoie->enrcur = atol (ptr);
		pvoie->tailm = file_size (pvoie->sr_fic);
		set_binary (voiecur, 1);
		pvoie->xferok = 1;
		pvoie->size_trans = 0L;
		pvoie->time_trans = 0L;
		prog_more (voie);
		switch (pvoie->type_yapp)
		{
		case 0:
			xmodem_tx_on (voie);
			break;
		case 1:
			xmodem_tx_1k (voie);
			break;
		}
		if (senddata (1))
		{
			ch_niv3 (XS_QUEUE);
		}
		else
			ch_niv3 (XS_SEND);
		retour = 1;
	}
	else
	{
		texte (T_ERR + 11);
		retour_appel ();
	}
	return (retour);
}
#endif


static int xmodem_rx (int voie)
{
	int fd;
	int retour = 0;
	struct stat bufstat;

	strtok (indd, " \r");

	if (read_only ())
		retour_appel ();
	else if ((indd = strtok (NULL, " \r")) == NULL)
	{
		texte (T_ERR + 20);
		retour_appel ();
	}
	else if ((tst_point (indd)) && (stat (nom_yapp (), &bufstat) == -1))
	{
		fd = open (svoie[voie]->sr_fic, O_CREAT | O_WRONLY | O_TRUNC, S_IREAD | S_IWRITE);
		if (fd > 0)
		{
			close (fd);
			unlink (svoie[voie]->sr_fic);
			pvoie->ytete = (Ylist *) m_alloue (sizeof (Ylist));
			n_cpy (255, pvoie->ytete->filename, pvoie->appendf);

			texte (T_YAP + 3);
			ch_niv3 (XR_LABL);
			del_temp (voie);
			pvoie->enrcur = 0L;
			pvoie->tailm = 0L;
			pvoie->size_trans = 0L;
			pvoie->time_trans = time (NULL);
			retour = 1;
		}
		else
		{
			texte (T_ERR + 30);
			retour_appel ();
		}
	}
	else
	{
		texte (T_ERR + 23);
		retour_appel ();
	}
	return (retour);
}

#define hex(d) ((d > '9') ? d-55 : d-48)

static int gethex (void)
{
	int val = 0;

	val = (int) (hex (*indd)) << 4;
	++indd;
	val += (int) (hex (*indd));
	++indd;

	return (val);
}

static int ztyp_header (long *f)
{
	long fl = 0L;
	int type = ZFERR;
	int i;

	while ((*indd) && (*indd != '*'))
		++indd;

	if (*indd)
	{
		while (*indd == '*')
			++indd;
		if (*indd == 24)
			++indd;
		if (*indd == 'B')
		{
			++indd;
			type = gethex ();
			for (i = 0; i < 4; i++)
			{
				fl <<= 8;
				fl += gethex ();
			}
		}
	}
	*f = fl;
	return (type);
}

#ifdef __linux__

void m_flush (int fd)
{
	ioctl (fd, TCFLSH, TCOFLUSH);
}

void kill_rzsz (int voie)
{
	if (svoie[voiecur]->rzsz_pid != -1)
	{
		kill (svoie[voie]->rzsz_pid, SIGTERM);
	}
}

static void sig_child (int sig)
{
	int ret;
	int pid;
	int voie;
	int status;

	pid = wait (&status);
	ret = WEXITSTATUS (status);

	for (voie = 0; voie < NBVOIES; voie++)
	{
		if (svoie[voie]->rzsz_pid == pid)
		{
			svoie[voie]->rzsz_pid = -1;
			svoie[voie]->ask = 1;
			break;
		}
	}
	signal (SIGCHLD, sig_child);
}

int run_rzsz (int voie, char *cmde)
{
	char *args[128];
	struct termios tty;
	defcom *ptrcom = &p_com[(int)p_port[no_port (voie)].ccom];
	int n;

	printf ("Commande = <%s>\n", cmde);

	set_binary (voie, 2);
	signal (SIGPIPE, SIG_IGN);
	signal (SIGCHLD, sig_child);

	/* Serial port in binary raw mode */
	tcgetattr (ptrcom->comfd, &tty);
	tty.c_iflag &= ~(IGNBRK | IGNCR | INLCR | ICRNL | IUCLC |
					 IXANY | IXON | IXOFF | INPCK | ISTRIP);
	tty.c_iflag |= (BRKINT | IGNPAR);
	tty.c_oflag &= ~OPOST;
	tty.c_lflag = ~(ICANON | ISIG | ECHO | ECHONL | ECHOE | ECHOK);
	tcsetattr (ptrcom->comfd, TCSANOW, &tty);

	/* pipe(svoie[voie]->to_xfbb); */
	/* pipe(svoie[voie]->to_rzsz); */

	switch (svoie[voie]->rzsz_pid = fork ())
	{
	case -1:
		return (0);
	case 0:					/* Child */
		dup2 (ptrcom->comfd, 0);
		dup2 (ptrcom->comfd, 1);
		for (n = 1; n < NSIG; n++)
			signal (n, SIG_DFL);

		n = 0;
		args[n] = strtok (cmde, " ");
		while (args[n])
		{
			if (n == 127)
			{
				args[n] = NULL;
				break;
			}
			args[++n] = strtok (NULL, " ");
		}

		(void) execvp (args[0], args);
		printf ("Exec %s failed\n", args[0]);
		exit (1);

	default:					/* Parent */
		break;
	}
	return (1);
}
#endif

void xmodem (void)
{
	long f;
	int ok = 1;
	int ncars;
	obuf *msgtemp;
	char *ptcur;
	char *ptr;
	Ylist *yptr;

	switch (pvoie->type_yapp)
	{
	case 0:
		var_cpy (0, "XMODEM");
		break;
	case 1:
		var_cpy (0, "1K-XMODEM");
		break;
	case 2:
		var_cpy (0, "YMODEM");
		break;
	case 3:
		var_cpy (0, "ZMODEM");
		break;
	}

	switch (pvoie->niv3)
	{
	case XS_EXTERN:

		/* Infos pour statistiques */
		pvoie->time_trans = time (NULL) - pvoie->time_trans;
		pvoie->finf.download += (int) (pvoie->size_trans / 1024L);

#ifdef __linux__
		/* Attend le vidage de la queue */
		set_binary (voiecur, 0);

		ioctl (p_com[(int)p_port[no_port (voiecur)].ccom].comfd, TCFLSH, 2);

		/* Reinitialise le port */
		init_modem (no_port (voiecur));

		clear_inbuf (voiecur);

#else
		/* Reeinitialisation du modem */
#ifdef __WIN32__
		{
			int val = 0;

			sta_drv (voiecur, SUSPCMD, &val);
		}
#else
		init_modem (no_port (voiecur));
#endif
#endif

		aff_header (voiecur);
		display_dsz_perf (voiecur);

		pvoie->finf.download += (int) (pvoie->size_trans / 1024L);

		retour_appel ();
		break;

	case XS_INIT:
		switch (pvoie->type_yapp)
		{
		case 0:
		case 1:
#ifdef __linux__
			if (BIOS (no_port (voiecur)) == P_LINUX)
			{
				if (ymodem_files ())
				{
					/* Runs DSZ or equivalent */
					char cmde[1024];
					int ok_send = 0;

					if (pvoie->type_yapp == 0)
						sprintf (cmde, "fbb_zm sx ");
					else
						sprintf (cmde, "fbb_zm sx -k ");

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, back2slash (nom_yapp ()));
						yptr = yptr->next;
						ok_send = 1;
						break;
					}

					/* Execute la commande en tache de fond */
					ok = 0;
					if (ok_send)
					{
						pvoie->sta.ack = 0;

						/* Attend le vidage de la queue */
						while (!modem_vide (voiecur));
						sleep (1);

						if (run_rzsz (voiecur, cmde))
						{
							pvoie->xferok = 1;
							pvoie->size_trans = 0L;
							pvoie->time_trans = 0L;
							prog_more (voiecur);
							ch_niv3 (XS_EXTERN);
							ok = 1;
						}
					}
				}
			}
#endif
#ifdef __WINDOWS__
			if (BIOS (no_port (voiecur)) == P_WINDOWS)
			{
#ifdef __WIN32__
				if (ymodem_files ())
				{
					/* Runs DSZ or equivalent */
					char cmde[256];
					int ok_send = 0;

					sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBIN, voiecur);

					if (pvoie->type_yapp == 0)
						sprintf (cmde, "sx -g %s", pvoie->ch_temp);
					else
						sprintf (cmde, "sx -k -g %s", pvoie->ch_temp);

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, nom_yapp ());
						yptr = yptr->next;
						ok_send = 1;
						break;	/* Only 1 file with XModem */

					}

					/* Execute la commande en tache de fond */
					ok = 0;
					if (ok_send)
					{
						int val = 1;

						pvoie->sta.ack = 0;

						/* Attend le vidage de la queue */
						while (!modem_vide (voiecur));

						// sta_drv(voiecur, SUSPCMD, &val);
						// sleep(1);

						if (run_rzsz (voiecur, cmde) == 0)
						{
							pvoie->xferok = 1;
							pvoie->size_trans = 0L;
							pvoie->time_trans = 0L;
							prog_more (voiecur);
							/* pvoie->seq = 1; */
							ch_niv3 (XS_EXTERN);
							ok = 1;
						}
					}
				}
				else
					ok = 0;
#else
				if (ymodem_files ())
				{
					/* Runs DSZ or equivalent */
					char cmde[256];
					int ok_send = 0;

					sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBIN, voiecur);
					if (pvoie->type_yapp == 0)
						wsprintf (cmde, "DSZ.PIF %s %x %d speed %ld handshake on sx",
								  pvoie->ch_temp, p_com[com].cbase, p_com[com].irq, p_com[com].baud);
					else
						wsprintf (cmde, "DSZ.PIF %s %x %d speed %ld handshake on sx",
								  pvoie->ch_temp, p_com[com].cbase, p_com[com].irq, p_com[com].baud);

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, nom_yapp ());
						yptr = yptr->next;
						ok_send = 1;
						break;	/* Only 1 file with XModem */

					}

					/* Execute la commande en tache de fond */
					if (ok_send)
					{
						closecom_windows (p_port[no_port (voiecur)].ccom);
						pvoie->sta.ack = 0;
						win_execute (cmde);
						ch_niv3 (XS_EXTERN);
						ok = 1;
					}
					else
						ok = 0;
				}
				else
					ok = 0;
#endif
			}
#endif
#ifdef __FBBDOS__
			ok = xmodem_tx (voiecur);
#endif

			break;
		case 2:
			if (ymodem_files ())
			{
#ifdef __linux__
				if (BIOS (no_port (voiecur)) == P_LINUX)
				{
					/* Runs DSZ or equivalent */
					char cmde[1024];
					int ok_send = 0;

					sprintf (cmde, "fbb_zm sb -k ");

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, back2slash (nom_yapp ()));
						yptr = yptr->next;
						ok_send = 1;
					}

					/* Execute la commande en tache de fond */
					ok = 0;
					if (ok_send)
					{
						pvoie->sta.ack = 0;

						/* Attend le vidage de la queue */
						while (!modem_vide (voiecur));
						sleep (1);

						if (run_rzsz (voiecur, cmde))
						{
							pvoie->xferok = 1;
							pvoie->size_trans = 0L;
							pvoie->time_trans = 0L;
							prog_more (voiecur);
							/* pvoie->seq = 1; */
							ch_niv3 (XS_EXTERN);
							ok = 1;
						}
					}
				}
#endif
#ifdef __WINDOWS__
				if (BIOS (no_port (voiecur)) == P_WINDOWS)
				{
#ifdef __WIN32__
					/* Runs DSZ or equivalent */
					char cmde[256];
					int ok_send = 0;

					sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBIN, voiecur);
					sprintf (cmde, "sb -k -g %s", pvoie->ch_temp);

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, nom_yapp ());
						yptr = yptr->next;
						ok_send = 1;
						break;	/* 1 file with YModem */

					}

					/* Execute la commande en tache de fond */
					ok = 0;
					if (ok_send)
					{
						int val = 1;

						pvoie->sta.ack = 0;

						/* Attend le vidage de la queue */
						while (!modem_vide (voiecur));

						sta_drv (voiecur, SUSPCMD, &val);
						sleep (1);

						if (run_rzsz (voiecur, cmde) == 0)
						{
							pvoie->xferok = 1;
							pvoie->size_trans = 0L;
							pvoie->time_trans = 0L;
							prog_more (voiecur);
							/* pvoie->seq = 1; */
							ch_niv3 (XS_EXTERN);
							ok = 1;
						}
					}
#else
					/* Runs DSZ or equivalent */
					char cmde[256];
					int ok_send = 0;

					wsprintf (cmde, "DSZ.PIF %s %x %d speed %ld handshake on sb -k",
							  pvoie->ch_temp, p_com[com].cbase, p_com[com].irq, p_com[com].baud);

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, nom_yapp ());
						yptr = yptr->next;
						ok_send = 1;
						break;	/* 1 file with YModem */

					}

					/* Execute la commande en tache de fond */
					if (ok_send)
					{
						char env[256];

						closecom_windows (p_port[no_port (voiecur)].ccom);
						pvoie->sta.ack = 0;
						win_execute (cmde);

						ch_niv3 (XS_EXTERN);
						ok = 1;
					}
					else
						ok = 0;
#endif
				}
#endif
#ifdef __FBBDOS__
				ok = ymodem_tx (voiecur);
#endif
			}
			else
				ok = 0;
			break;
		case 3:				/* ZModem */
			if (ymodem_files ())
			{
#ifdef __linux__
				if (BIOS (no_port (voiecur)) == P_LINUX)
				{
					/* Runs DSZ or equivalent */
					char cmde[1024];
					int ok_send = 0;

					sprintf (cmde, "fbb_zm sz ");

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, back2slash (nom_yapp ()));
						yptr = yptr->next;
						ok_send = 1;
					}

					/* Execute la commande en tache de fond */
					ok = 0;
					if (ok_send)
					{
						pvoie->sta.ack = 0;

						/* Attend le vidage de la queue */
						while (!modem_vide (voiecur));
						sleep (1);

						if (run_rzsz (voiecur, cmde))
						{
							pvoie->xferok = 1;
							pvoie->size_trans = 0L;
							pvoie->time_trans = 0L;
							prog_more (voiecur);
							/* pvoie->seq = 1; */
							ch_niv3 (XS_EXTERN);
							ok = 1;
						}
					}
				}
#endif
#ifdef __WINDOWS__
				if (BIOS (no_port (voiecur)) == P_WINDOWS)
				{
#ifdef __WIN32__
					/* Runs DSZ or equivalent */
					char cmde[1024];
					int val;
					int ok_send = 0;

					sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBIN, voiecur);
					sprintf (cmde, "sz -g %s", pvoie->ch_temp);

					yptr = svoie[voiecur]->ytete;
					while (yptr)
					{
						strcat (cmde, " ");
						indd = yptr->filename;
						strcat (cmde, nom_yapp ());
						yptr = yptr->next;
						ok_send = 1;
					}

					/* Execute la commande en tache de fond */
					ok = 0;
					if (ok_send)
					{
						pvoie->sta.ack = 0;

						/* Attend le vidage de la queue */
						while (!modem_vide (voiecur));
						val = 1;
						sta_drv (voiecur, SUSPCMD, &val);
						sleep (1);

						if (run_rzsz (voiecur, cmde) == 0)
						{
							pvoie->xferok = 1;
							pvoie->size_trans = 0L;
							pvoie->time_trans = 0L;
							prog_more (voiecur);
							/* pvoie->seq = 1; */
							ch_niv3 (XS_EXTERN);
							ok = 1;
						}
					}
#else
					/* Runs DSZ or equivalent */
					char cmde[256];
					char env[128];
					char fil[128];
					int ok_send = 0;
					FILE *fptr;

					sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBIN, voiecur);
					sprintf (env, "%sDSZFIL.%d", MBIN, voiecur);
					wsprintf (cmde, "DSZ.PIF %s %x %d speed %ld handshake on sz -b -m -rr @%s",
							  pvoie->ch_temp,
						  p_com[com].cbase, p_com[com].irq, p_com[com].baud,
							  env);

					yptr = svoie[voiecur]->ytete;
					fptr = fopen (env, "wt");
					if (fptr == NULL)
					{
						ok = 0;
						break;
					}
					while (yptr)
					{
						/*
						   strcat(cmde, " ");
						   indd = yptr->filename;
						   strcat(cmde, nom_yapp());
						 */
						indd = yptr->filename;
						fputs (nom_yapp (), fptr);
						fputc ('\n', fptr);
						yptr = yptr->next,
							ok_send = 1;
					}
					fclose (fptr);

					/* Execute la commande en tache de fond */
					if (ok_send)
					{
						closecom_windows (p_port[no_port (voiecur)].ccom);
						pvoie->sta.ack = 0;
						win_execute (cmde);

						ch_niv3 (XS_EXTERN);
						ok = 1;
					}
					else
						ok = 0;
#endif
				}
#endif
#ifdef __FBBDOS__
				{
					zmodem_init ();
					ok = 1;
				}
#endif
			}
			else
				ok = 0;
			break;
		}
		break;

	case ZS_FILE:
		switch (ztyp_header (&f))
		{
		case ZRINIT:
			zmodem_tx (voiecur);
			break;
		case ZCOMM:
			zmodem_init ();
			break;
		case ZRQINIT:
			break;
		default:
			ch_niv3 (XM_ABORT);
			xmodem ();
			ok = 0;
			break;
		}
		break;

	case XS_SEND:
#ifdef __FBBDOS__
		if (senddata (1))
		{
			ch_niv3 (XS_QUEUE);
		}
#endif
		break;

	case XS_WAIT:
		break;

	case XS_QUEUE:
		break;

	case XS_END:
		xmodem_off (voiecur);
		set_binary (voiecur, 0);
		aff_header (voiecur);
		pvoie->time_trans = time (NULL) - pvoie->time_trans;
		pvoie->finf.download += (int) (pvoie->size_trans / 1024L);

		if ((pvoie->type_yapp == 2) || (pvoie->type_yapp == 3))
		{
			yptr = svoie[voiecur]->ytete;
			while ((yptr) && (yptr->ok))
				yptr = yptr->next;
			if (pvoie->type_yapp == 3)
				pvoie->size_trans = pvoie->enrcur;
			yptr->time_trans = pvoie->time_trans;
			yptr->size_trans = pvoie->size_trans;
			if (yptr)
			{
				yptr->ok = 1;
				yptr = yptr->next;
			}
			if (yptr)
			{
				switch (pvoie->type_yapp)
				{
				case 2:
					ch_niv3 (XS_INIT);
					ok = ymodem_tx (voiecur);
					break;
				case 3:
					ok = zmodem_tx (voiecur);
					break;
				}
			}
			else
			{
				libere_ymodem (voiecur, 1);
				retour_appel ();
				ok = 0;
			}
		}
		else
		{
			display_perf (voiecur);
			retour_appel ();
			ok = 0;
		}
		break;

	case XR_EXTERN:

		/* Infos pour statistiques */
		pvoie->time_trans = time (NULL) - pvoie->time_trans;

#ifdef __linux__
		/* Attend le vidage de la queue */
		set_binary (voiecur, 0);

		ioctl (p_com[(int)p_port[no_port (voiecur)].ccom].comfd, TCFLSH, 2);

		/* Reinitialise le port */
		init_modem (no_port (voiecur));

		clear_inbuf (voiecur);

#else
		/* Reeinitialisation du modem */
#ifdef __WIN32__
		{
			int val = 0;

			sta_drv (voiecur, SUSPCMD, &val);
		}
#else
		init_modem (no_port (voiecur));
#endif
#endif

		/* Appel du filtre... */
		if (test_temp (voiecur))
		{
			rename_temp (voiecur, pvoie->sr_fic);
			wr_dir (pvoie->sr_fic, pvoie->sta.indicatif.call);
			pvoie->time_trans = time (NULL) - pvoie->time_trans;
		}
		aff_header (voiecur);

		display_dsz_perf (voiecur);

		retour_appel ();
		break;

	case XR_INIT:
		ok = xmodem_rx (voiecur);
		break;

	case XR_LABL:
		while ((*indd) && (!ISPRINT (*indd)))
			++indd;

		w_label (pvoie->sr_fic, sup_ln (indd));
		n_cpy (LABEL_NOM - 1, pvoie->label, sup_ln (indd));

		texte (T_YAP + 1);
		aff_etat ('E');
		send_buf (voiecur);
		while (!modem_vide (voiecur))
			;
#ifdef __linux__
		if (BIOS (no_port (voiecur)) == P_LINUX)
		{
			char cmde[256];
			char temp[256];

			temp_name (voiecur, temp);
			sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBINDIR, voiecur);

			switch (pvoie->type_yapp)
			{
			case 0:
			case 1:
				sprintf (cmde, "fbb_zm rx %s", temp);
				break;
			case 2:
				sprintf (cmde, "fbb_zm rb %s", temp);
				break;
			case 3:
				sprintf (cmde, "fbb_zm rz %s", temp);
				break;
			}

			pvoie->sta.ack = 0;

			/* Attend le vidage de la queue */
			while (!modem_vide (voiecur));
			sleep (1);

			if (run_rzsz (voiecur, cmde))
			{
				pvoie->xferok = 1;
				pvoie->size_trans = 0L;
				pvoie->time_trans = 0L;
				prog_more (voiecur);
				ch_niv3 (XR_EXTERN);
				ok = 1;
			}
			else
				retour_dos ();
		}
#endif
#ifdef __WINDOWS__
		if (BIOS (no_port (voiecur)) == P_WINDOWS)
		{
#ifdef __WIN32__
			char cmde[256];
			char temp[256];
			int ok_send = 0;

			temp_name (voiecur, temp);
			sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBIN, voiecur);

			switch (pvoie->type_yapp)
			{
			case 0:
			case 1:
				wsprintf (cmde, "rx %s -g %s", temp, pvoie->ch_temp);
				break;
			case 2:
				wsprintf (cmde, "rb %s -g %s", temp, pvoie->ch_temp);
				break;
			case 3:
				wsprintf (cmde, "rz %s -g %s", temp, pvoie->ch_temp);
				break;
			}

			/* Execute la commande en tache de fond */
			pvoie->sta.ack = 0;

			/* Attend le vidage de la queue */
			while (!modem_vide (voiecur));

			{
				int val = 1;

				sta_drv (voiecur, SUSPCMD, &val);
				sleep (1);
				if (run_rzsz (voiecur, cmde) == 0)
				{
					/*
					   pvoie->xferok = 1;
					   pvoie->size_trans = 0L;
					   pvoie->time_trans = 0L;
					   prog_more (voiecur);
					 */
					/* pvoie->seq = 1; */
					ch_niv3 (XR_EXTERN);
				}
			}
#else
			char cmde[256];
			char temp[256];
			int ok_send = 0;

			temp_name (voiecur, temp);
			sprintf (pvoie->ch_temp, "%sDSZLOG.%d", MBIN, voiecur);

			switch (pvoie->type_yapp)
			{
			case 0:
			case 1:
				wsprintf (cmde, "DSZ.PIF %s %x %d speed %ld handshake on rx %s",
						  pvoie->ch_temp, p_com[com].cbase, p_com[com].irq, p_com[com].baud, temp);
				break;
			case 2:
				wsprintf (cmde, "DSZ.PIF %s %x %d speed %ld handshake on rb %s",
						  pvoie->ch_temp, p_com[com].cbase, p_com[com].irq, p_com[com].baud, temp);
				break;
			case 3:
				wsprintf (cmde, "DSZ.PIF %s %x %d speed %ld handshake on rz %s",
						  pvoie->ch_temp, p_com[com].cbase, p_com[com].irq, p_com[com].baud, temp);
				break;
			}

			/* Execute la commande en tache de fond */
			closecom_windows (p_port[no_port (voiecur)].ccom);
			pvoie->sta.ack = 0;
			win_execute (cmde);
			ch_niv3 (XR_EXTERN);
#endif
		}
#endif
#ifdef __FBBDOS__
		{
			sleep_ (1);
			set_binary (voiecur, 1);
			pvoie->xferok = 0;
			prog_more (voiecur);
			xmodem_rx_on (voiecur);
			ch_niv3 (XR_RECV);
		}
#endif
		break;

	case XR_RECV:
		pvoie->size_trans += (long) nb_trait;
		pvoie->enrcur += (long) nb_trait;
		if ((msgtemp = pvoie->msgtete) != NULL)
		{
			while (msgtemp->suiv)
				msgtemp = msgtemp->suiv;
		}
		else
		{
			msgtemp = (obuf *) m_alloue (sizeof (obuf));
			pvoie->msgtete = msgtemp;
			msgtemp->nb_car = msgtemp->no_car = 0;
			msgtemp->suiv = NULL;
		}
		ncars = msgtemp->nb_car;
		ptcur = msgtemp->buffer + ncars;
		ptr = data;
		while (nb_trait--)
		{
			++pvoie->memoc;
			*ptcur++ = *ptr++;
			if (++ncars == 250)
			{
				msgtemp->nb_car = ncars;
				msgtemp->suiv = (obuf *) m_alloue (sizeof (obuf));
				msgtemp = msgtemp->suiv;
				msgtemp->nb_car = msgtemp->no_car = ncars = 0;
				msgtemp->suiv = NULL;
				ptcur = msgtemp->buffer;
			}
		}
		msgtemp->nb_car = ncars;
		if (pvoie->memoc > MAXMEM)
		{
			ok = write_mess_temp (O_BINARY, voiecur);
		}
		if (ok)
		{
			break;
		}
		/* Si erreur continue sur XR_END ! */

	case XR_END:
		write_mess_temp (O_BINARY, voiecur);
		xmodem_off (voiecur);
		set_binary (voiecur, 0);
		if (test_temp (voiecur))
		{
			rename_temp (voiecur, pvoie->sr_fic);
			wr_dir (pvoie->sr_fic, pvoie->sta.indicatif.call);
			pvoie->time_trans = time (NULL) - pvoie->time_trans;
			display_perf (voiecur);
		}
		else
			del_temp (voiecur);
		aff_header (voiecur);
		retour_appel ();
		ok = 0;
		break;

	case XM_ABORT:
		clear_outbuf (voiecur);
		xmodem_off (voiecur);
		set_binary (voiecur, 0);
		if (!pvoie->xferok)
			del_temp (voiecur);
		aff_header (voiecur);
		pvoie->finf.download += (int) (pvoie->size_trans / 1024L);
		retour_appel ();
		ok = 0;
		break;

	}
#ifndef __WINDOWS__
	aff_xmodem (ok);
#endif
}


void modem (void)
{
	switch (pvoie->niv2)
	{
	case 0:
		accueil_modem ();
		break;
	case 1:
		indic_modem ();
		break;
	case 2:
		passwd_modem ();
		break;
	case 3:
		passwd_change ();
		break;
	case 4:
		xmodem ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "MODEM", pvoie->niv2);
		break;
	}
}
