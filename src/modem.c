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
 *    MODULE MODEM
 */

#include <serv.h>
#include <modem.h>

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#endif


static int ok_pass_modem (void);

static void init_md (void)
{
	buf_md.ptr_r = buf_md.ptr_l = buf_md.ptr_a = 0;
	buf_md.buf_vide = 1;
	buf_md.nblig = 0;
	buf_md.nbcar = 0;
	buf_md.flush = 0;
}


int md_send (int port, char *chaine)
{
	int ctrl = 0;
	char c;

	df ("md_send", 2);

	while ((c = *chaine++) != 0)
	{

		switch (c)
		{
		case '~':
			deb_io ();
#if defined(__WINDOWS__) || defined(__linux__)
			WinMSleep (500);
#endif
#ifdef __FBBDOS__
			tempo = 10;
			/* #pragma warn -eff */
			while (tempo);
			/* #pragma warn .eff */
#endif
			fin_io ();
			break;

		case '^':
			ctrl = 1;
			break;

		default:
			if (ctrl)
			{
				c -= '@';
				ctrl = 0;
			}

#ifdef __FBBDOS__
			{
				RSEGS rsegs;

				BufReel[0] = c;
				memset (&rsegs, 0, sizeof (RSEGS));
				rsegs.CX = 1;
				rsegs.DX = (int) p_port[port].ccom - 1;
				rsegs.DI = 0;
				rsegs.ES = BufSeg;
				rsegs.DS = BufSeg;

				while (rsegs.CX)
				{
					rsegs.AX = 0x0a00;
					int14real (&rsegs);
				}
			}
#else
			send_tnc (port, c);
#endif
			break;
		}

		ff ();
	}
	return (1);
}


static void md_etoiles (int voie)
{
	int port = no_port (voie);

#ifdef __linux__
	struct termios tty;
	defcom *ptrcom = &p_com[(int)p_port[port].ccom];

	tcgetattr (ptrcom->comfd, &tty);
	tty.c_lflag &= ~ECHO;
	tty.c_lflag |= ECHONL;
	tcsetattr (ptrcom->comfd, TCSANOW, &tty);
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
		defcom *ptrcom = &p_com[p_port[port].ccom];

#ifdef __WIN32__
		int val = 2;

		sta_drv (voie, ECHOCMD, (void *) &val);
#else
		if (EscapeCommFunction (ptrcom->comfd, SETSTAR) < 0)
			ShowError ("Driver error", "Unknown command :", SETSTAR);
#endif
	}
#endif
#ifdef __FBBDOS__
	{
		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0c01;
		int14real (&rsegs);
	}
#endif
}

void md_echo (int voie)
{
#ifdef __linux__
	int port = no_port (voie);
	struct termios tty;
	defcom *ptrcom = &p_com[(int)p_port[port].ccom];

	tcgetattr (ptrcom->comfd, &tty);
	tty.c_lflag |= ECHO;
	tcsetattr (ptrcom->comfd, TCSANOW, &tty);
#endif
}

void md_no_echo (int voie)
{
	modem_no_echo (no_port (voie));
}


void modem_no_echo (int port)
{
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
		defcom *ptrcom = &p_com[p_port[port].ccom];

#ifdef __WIN32__
		int val = 0;

		sta_drv (port, ECHOCMD, (void *) &val);
#else
		if (EscapeCommFunction (ptrcom->comfd, CLRECHO) < 0)
			ShowError ("Driver error", "Unknown command :", CLRECHO);
#endif
	}
#endif
#ifdef __FBBDOS__
	{
		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0c02;
		int14real (&rsegs);
	}
#endif
}

void init_modem (int port)
{
#ifdef __linux__
	if (BIOS (port) == P_LINUX)
	{
		struct termios tty;
		int comfd;

		initcom_linux (p_port[port].ccom);

		default_tty (p_port[port].ccom);
		comfd = p_com[(int)p_port[port].ccom].comfd;

		tcflow (comfd, TCOON);

		tcgetattr (comfd, &tty);
		tty.c_iflag = (IGNBRK | IXON | IXOFF | ICRNL);
		tty.c_oflag = (ONLCR | OPOST);
		tty.c_lflag = (ICANON | ECHO);
		tty.c_cflag |= (CREAD | CRTSCTS);
		tty.c_cc[2] = '\b';
		tcsetattr (comfd, TCSANOW, &tty);


	}
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
#ifdef __WIN32__
		SetModemLine (port, 1);
#else
		defcom *ptrcom = &p_com[p_port[port].ccom];

		initcom_windows (p_port[port].ccom,
						 INQUE, OUTQUE, XON | CTS);
		if (EscapeCommFunction (ptrcom->comfd, SETDTR) < 0)
			ShowError ("Driver error", "Unknown command :", SETDTR);
		if (EscapeCommFunction (ptrcom->comfd, SETRTS) < 0)
			ShowError ("Driver error", "Unknown command :", SETRTS);
#endif
	}
#endif
#ifdef __FBBDOS__
	{
		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0e00;
		int14real (&rsegs);

		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0900 | (p_com[p_port[port].ccom].options & 0xff);
		int14real (&rsegs);
	}
#endif
}


void modem_stop (int port)
{
#ifdef __linux__
	{
		int comfd = p_com[(int)p_port[port].ccom].comfd;

		tcflow (comfd, TCOOFF);
	}
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
#ifdef __WIN32__
		SetModemLine (port, 0);
#else
		defcom *ptrcom = &p_com[p_port[port].ccom];

		if (EscapeCommFunction (ptrcom->comfd, CLRDTR) < 0)
			ShowError ("Driver error", "Unknown command :", CLRDTR);
		if (EscapeCommFunction (ptrcom->comfd, CLRRTS) < 0)
			ShowError ("Driver error", "Unknown command :", CLRRTS);
#endif
	}
#endif
#ifdef __FBBDOS__
	{

		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0500;
		int14real (&rsegs);
	}
#endif
}


void modem_start (int port)
{
	char s[80];
	FILE *fpinit;

#ifdef __FBBDOS__
	RSEGS rsegs;
	fen *fen_ptr;

#endif

	if (save_fic)
		return;

#ifdef ENGLISH
	sprintf (s, "Modem Port %d Set-up", port);
#else
	sprintf (s, " Init modem port %d ", port);
#endif

#ifdef __FBBDOS__
	fen_ptr = open_win (4, 2, 50, 8, INIT, s);
#endif

#ifdef __linux__
	if (BIOS (port) == P_LINUX)
	{
		struct termios tty;
		int comfd;

		initcom_linux (p_port[port].ccom);

		default_tty (p_port[port].ccom);
		comfd = p_com[(int)p_port[port].ccom].comfd;

		tcgetattr (comfd, &tty);
		tty.c_iflag = (IGNBRK | IXON | IXOFF | ICRNL);
		tty.c_oflag = (ONLCR | OPOST);
		tty.c_lflag = (ICANON | ECHO);
		tty.c_cflag |= (CREAD | CRTSCTS);
		tty.c_cc[2] = '\b';
		tcsetattr (comfd, TCSANOW, &tty);
	}
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
#ifdef __WIN32__
		SetModemLine (port, 1);
#else
		/* Initialisation du modem */
		initcom_windows (p_port[port].ccom, INQUE, OUTQUE, XON | DSR);
#endif
	}
#endif
#ifdef __FBBDOS__
	{
		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0e00;
		int14real (&rsegs);

		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0900 | (p_com[p_port[port].ccom].options & 0xff);
		int14real (&rsegs);

		modem_no_echo (port);

		rsegs.DX = port - 1;
		rsegs.AX = 0x0600;
		int14real (&rsegs);

		/* Initialisation du modem */
		initcom_combios (p_port[port].ccom);
	}
#endif

	sprintf (s, "INITTNC%d.SYS", port);
	if ((fpinit = fopen (c_disque (s), "rb")) != NULL)
	{
		while (fgets (s, 80, fpinit))
		{
			sup_ln (s);
			if ((*s) && (*s != '#'))
			{
				tnc_commande (port, s, PORTCMD);
			}
		}
		ferme (fpinit, 2);
	}
#ifdef __FBBDOS__
	close_win (fen_ptr);
#endif
}


void md_reset (int voie)
{
#ifdef __WINDOWS__
	int port = no_port (voie);

	xmodem_off (voie);

	if (BIOS (port) == P_WINDOWS)
	{
		/* Initialisation du modem */
		/* initcom_windows(p_port[port].ccom); */
	}
#endif
#ifdef __FBBDOS__
	int port = no_port (voie);

	xmodem_off (voie);

	{
		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0900 | (p_com[p_port[port].ccom].options & 0xff);
		int14real (&rsegs);
	}
#endif
}

void md_defaut (int voie)
{
	int port = no_port (voie);

#ifdef __linux__
	if (BIOS (port) == P_LINUX)
	{
		struct termios tty;
		defcom *ptrcom = &p_com[(int)p_port[port].ccom];

		/* Revalide l'echo ... */
		tcgetattr (ptrcom->comfd, &tty);
		tty.c_lflag = ICANON | ECHO;
		tcsetattr (ptrcom->comfd, TCSANOW, &tty);
	}
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
		defcom *ptrcom = &p_com[p_port[port].ccom];

#ifdef __WIN32__
		int val = 1;

		sta_drv (voie, ECHOCMD, (void *) &val);
#else
		if (EscapeCommFunction (ptrcom->comfd, SETECHO) < 0)
			ShowError ("Driver error", "Unknown command :", SETECHO);
#endif
	}
#endif
#ifdef __FBBDOS__
	{
		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.AX = 0x0d03;
		int14real (&rsegs);
	}
#endif
}

int modem_vide (int voie)
{
	int nbtot;
	int port = no_port (voie);

#ifdef __linux__
	if (BIOS (port) == P_LINUX)
	{
		defcom *ptrcom = &p_com[(int)p_port[port].ccom];

		ioctl (ptrcom->comfd, TIOCOUTQ, &nbtot);
	}
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
#ifdef __WIN32__
		int total;

		GetModemStat (no_port (voie), &total, &nbtot);
#else
		COMSTAT cstat;
		defcom *ptrcom = &p_com[p_port[port].ccom];

		GetCommError (ptrcom->comfd, &cstat);
		nbtot = cstat.cbOutQue;
#endif
	}
#endif
#ifdef __FBBDOS__
	{
		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.CX = 0;
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.DI = 0;
		rsegs.ES = BufSeg;
		rsegs.DS = BufSeg;
		rsegs.AX = 0x0a00;
		int14real (&rsegs);
		nbtot = (int) ((rsegs.BX >> 8) - (rsegs.BX & 0xff));
	}
#endif
	return (nbtot == 0);
}


void md_inbuf (int voie, char *ptr, int nb)
/* Entree de caracteres dans le buffer modem */
{
	in_buf (voie, ptr, nb);

#ifdef __linux__
	{
		defcom *ptrcom = &p_com[(int)p_port[port].ccom];
		struct termios tty;

		/* Revalide l'echo ... */
		tcgetattr (ptrcom->comfd, &tty);
		if ((tty.c_lflag & ECHO) == 0)
		{
			tty.c_lflag = ECHO;
			tcsetattr (ptrcom->comfd, TCSANOW, &tty);
		}
	}
#endif
}


int md_busy (char *ptr, int nb)
/* test de lignes recues */
{
	ptr[nb] = '\0';
	while ((*ptr) && (!ISGRAPH (*ptr)))
		++ptr;
	ptr[250] = '\0';
	if ((strstr (ptr, "NO CARRIER")) || (strstr (ptr, "BUSY")) ||
		(strstr (ptr, "VOICE")) || (strstr (ptr, "ERROR")) ||
		(strstr (ptr, "NO ANSWER")) || (strstr (ptr, "NO DIALTONE")))
	{
		return (1);
	}
	return (0);
}




void connect_modem (int voie)
{
	df ("connect_modem", 1);

	init_timout (voie);
	init_md ();
	connect_fen ();
	svoie[voie]->ncur = NULL;
	svoie[voie]->aut_linked = 1;
	svoie[voie]->log = 1;
	svoie[voie]->sta.stat = svoie[voie]->sta.connect = 4;
	svoie[voie]->deconnect = FALSE;
	svoie[voie]->private_dir = 0;
	svoie[voie]->ret = 0;
	svoie[voie]->sid = 0;
	svoie[voie]->pack = 0;
	svoie[voie]->read_only = 0;
	svoie[voie]->vdisk = 2;
	svoie[voie]->tstat = svoie[voie]->debut = time (NULL);
	svoie[voie]->nb_err = svoie[voie]->seq = svoie[voie]->stop = svoie[voie]->sr_mem = 0;
	svoie[voie]->tmach = 0L;
	svoie[voie]->l_mess = 0L;
	svoie[voie]->l_yapp = 0L;
	svoie[voie]->sta.ack = 0;
	svoie[voie]->maj_ok = 0;
	svoie[voie]->ch_mon = svoie[voie]->cross_connect = -1;
	svoie[voie]->conf = 0;
	set_binary (voie, 0);
	svoie[voie]->msg_held = 0;
	svoie[voie]->mess_recu = svoie[voie]->xferok = 1;
	svoie[voie]->mbl = 1;
	svoie[voie]->entmes.numero = 0L;
	svoie[voie]->entmes.theme = 0;
	svoie[voie]->finf.lang = langue[0]->numlang;
	svoie[voie]->r_tete = NULL;
	svoie[voie]->mode = 0;
	svoie[voie]->rev_mode = 1;
	svoie[voie]->mbl_ext = 1;
	svoie[voie]->nb_egal = 0;
	svoie[voie]->paclen = p_port[no_port (voie)].pk_t;
	if (fbb_fwd)
		svoie[voie]->mode |= F_NFW;
	curseur ();
	if (svoie[voie]->niv1 == 0)
	{
		strcpy (svoie[voie]->sta.indicatif.call, "MODEM");
		svoie[voie]->sta.indicatif.num = '\0';
		svoie[voie]->niv1 = N_MOD;
		svoie[voie]->niv2 = 0;
		svoie[voie]->niv3 = 0;
	}
	else
	{
		if (svoie[voie]->curfwd)
			svoie[voie]->curfwd->no_con = 8;
		connect_log (voie, " {MODEM}");
		if (cher_noeud (svoie[voie]->sta.indicatif.call))
		{
			connexion (voie);
			new_om = nouveau (voie);
			/* getvoie(voie)->mode = getvoie(voie)->finf.flags ; */
			svoie[voie]->mode = 0;
			if (fbb_fwd)
			{
				svoie[voie]->mode |= F_NFW;
			}
		}
	}
	aff_nbsta ();
#ifdef __WINDOWS__
	window_connect (voie);
#endif
	change_droits (voie);
	strcpy (svoie[voie]->dos_path, "\\");
	aff_event (voie, 1);

	ff ();
	return;
}


void deconnect_modem (int voie)
{
	int port;
	long temps;

	if (DEBUG)
		return;

	port = no_port (voie);

	/* Attend que le buffer soit vide */

#ifdef __linux__
	{
		int tempo = 100;		/* attend 10s le vidage du modem */

		while (!modem_vide (voie))
		{
			WinMSleep (50);
			if (tempo > 0)
				--tempo;
			if (tempo == 0)
				break;
		}
	}
#endif
#ifdef __WINDOWS__
	{
		int tempo = 100;		/* attend 10s le vidage du modem */

		while (!modem_vide (voie))
		{
			WinMSleep (50);
			if (tempo > 0)
				--tempo;
			if (tempo == 0)
				break;
		}
		WinMSleep (500);
	}
#endif
#ifdef __FBBDOS__
	tempo = 180;				/* attend 10s le vidage du modem */

	while (!modem_vide (voie))
	{
		if (tempo == 0)
			break;
	}
	sleep_ (1);
#endif

	modem_stop (port);
	modem_no_echo (port);

	temps = time (NULL) + 2;

#ifdef __linux__
	if (BIOS (port) == P_LINUX)
	{
		defcom *ptrcom = &p_com[(int)p_port[port].ccom];

		if (ptrcom->comfd < 0)
			return;

		/* Attend la descente du DCD */
		for (;;)
		{
			int mcs;

			ioctl (ptrcom->comfd, TIOCMGET, &mcs);
			if (mcs & TIOCM_CAR)
			{
				if (time (NULL) > temps)
				{
					md_send (port, "~~~+++~~~ATH0^M");
					break;
				}
			}
			else
				break;
		}
	}
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)
	{
		defcom *ptrcom = &p_com[p_port[port].ccom];

		if (ptrcom->comfd < 0)
			return;

#ifdef __WIN32__
		while (GetModemDCD (port))
		{
			if (time (NULL) > temps)
			{
				md_send (port, "~~~+++~~~ATH0^M");
				break;
			}
		}
#else
		while (GetCommStatus (ptrcom->comfd) & 0x80)
		{
			if (time (NULL) > temps)
			{
				md_send (port, "~~~+++~~~ATH0^M");
				break;
			}
		}
#endif
	}
#endif
#ifdef __FBBDOS__
	{
		RSEGS rsegs;

		memset (&rsegs, 0, sizeof (RSEGS));
		do
		{
			rsegs.AX = 0x300;
			rsegs.DX = (int) p_port[port].ccom - 1;
			int14real (&rsegs);
			if ((time (NULL) > temps) && (rsegs.AX & 0x80))
			{
				if (!getvoie (CONSOLE)->sta.connect)
					cprintf ("\r\nModem time-out\r\n");
				md_send (port, "~~~+++~~~ATH0^M");
				break;
			}
		}
		while (rsegs.AX & 0x80);

	}
#endif
	svoie[voie]->niv1 = svoie[voie]->niv2 = svoie[voie]->niv3 = 0;
	svoie[voie]->timout = time_n;

	ff ();

}


void re_init_modem (int voie)
{
	int port;

	if (DEBUG)
		return;

	df ("re_init_modem", 1);

	port = no_port (voie);

	modem_start (port);
	ff ();
}


void accueil_modem (void)
{
	bipper ();
	md_defaut (voiecur);
	cr ();
	pvoie->lignes = -1;
	p_port[no_port (voiecur)].echo = TRUE;
	if (!outfich (c_disque ("LANG\\MODEM.ENT")))
		out ("$W$O BBS. Phone access$W$W", 25);

	fprintf (stderr, "%s", (c_disque ("LANG\\MODEM.ENT")));

	out ("Callsign :", 10);
	pvoie->temp1 = pvoie->temp2 = 0;
	pvoie->maj_ok = 0;
	pvoie->l_mess = 0L;
	pvoie->l_yapp = 0L;
	pvoie->read_only = 0;
	ch_niv2 (1);
}

#define READ_ONLY_0 "\rThe callsign \"%s\" is not registered.\rYou have a read-only access.\rYou may leave a message to SYSOP.\r\rGo on anyway (Y/N) "
#define READ_ONLY_1 "\rThere is no password for \"%s\".\rYou have a read-only access.\rYou may leave a message to SYSOP.\r\r"

static void read_only_alert (int mode, char *callsign)
{
	char chaine[256];

	pvoie->read_only = 1;
	switch (mode)
	{
	case 0:
		sprintf (chaine, READ_ONLY_0, callsign);
		break;
	case 1:
		sprintf (chaine, READ_ONLY_1, callsign);
		break;
	}
	out (chaine, strlen (chaine));
	aff_etat ('E');
	send_buf (voiecur);
}

void indic_modem (void)
{
	int i, rejet = 0, dde_call = 0;
	char chaine[128];
	char *st;

	indd[40] = '\0';

	while ((*indd) && (!isgraph (*indd)))
		++indd;

	if (*indd == '.')
	{
		pvoie->temp2 = 1;
		p_port[no_port (voiecur)].echo = FALSE;
		++indd;
	}

	if (pvoie->read_only)
	{
		if (toupper (*indd) == 'Y')
		{
			if (cher_noeud (pvoie->sta.indicatif.call))
			{
				*pvoie->passwd = '\0';
				pvoie->mode = 0;
				if (fbb_fwd)
				{
					pvoie->mode |= F_NFW;
				}
				connexion (voiecur);
#ifdef __WINDOWS__
				window_connect (voiecur);
#endif
				change_droits (voiecur);
				new_om = nouveau (voiecur);
			}
			else
			{
				init_info (&pvoie->finf, &pvoie->sta.indicatif);
				strn_cpy (6, pvoie->sta.indicatif.call, "MODEM");
				pvoie->read_only = 1;
				*pvoie->passwd = '\0';
				connexion (voiecur);
				strcpy (pvoie->finf.prenom, "...");
				pvoie->droits = d_droits;
			}
			cr ();
			init_langue (voiecur);
			st = idnt_fwd ();
			outs (st, strlen (st));
			texte (T_MES + 2);
			finentete ();
			retour_mbl ();
		}
		else
		{
			pvoie->temp1 = pvoie->temp2 = 0;
			p_port[no_port (voiecur)].echo = TRUE;
			pvoie->read_only = 0;
			md_echo (voiecur);
			out ("\rCallsign :", 11);
		}
		strcpy (pvoie->dos_path, "\\");
		return;
	}

	if (*indd == '\0')
	{
		md_echo (voiecur);
		out ("Callsign :", 10);
		return;
	}

	if ((strncmp (indd, "CONNECT", 7) == 0) ||
		(strncmp (indd, "CARRIER", 7) == 0) ||
		(strncmp (indd, "PROTOCO", 7) == 0) ||
		(strncmp (indd, "COMPRES", 7) == 0))
	{
		return;
	}

	++pvoie->temp1;

	sup_ln (indd);

	if (!find (indd))
	{
		sprintf (chaine, "Invalid callsign \"%s\" !", indd);
		outln (chaine, strlen (chaine));
		md_echo (voiecur);
		if (pvoie->temp1 == 10)
			rejet = 1;
		else
			dde_call = 1;
		pvoie->temp2 = 0;
		p_port[no_port (voiecur)].echo = TRUE;
	}
	else
	{
		strn_cpy (6, pvoie->sta.indicatif.call, indd);
		pvoie->sta.indicatif.num = 0;
		for (i = 0; i < 8; i++)
			*(pvoie->sta.relais[i].call) = '\0';
		connect_log (voiecur, " {MODEM}");
		if (cher_noeud (pvoie->sta.indicatif.call))
		{
			*pvoie->passwd = '\0';
			/* pvoie->mode = pvoie->finf.flags ; */
			pvoie->mode = 0;
			if (fbb_fwd)
			{
				pvoie->mode |= F_NFW;
				/*              if (bin_fwd) pvoie->mode |= F_BIN ; */
			}
			connexion (voiecur);
			new_om = nouveau (voiecur);
#ifdef __WINDOWS__
			window_connect (voiecur);
#endif
			change_droits (voiecur);
			strcpy (pvoie->dos_path, "\\");
			if (MOD (pvoie->finf.flags))
			{
				if (*pvoie->finf.pass)
				{
					ch_niv2 (2);
					out ("Password :", 10);
					pvoie->temp1 = 0;
					md_etoiles (voiecur);
				}
				else
				{
					read_only_alert (1, indd);
					md_echo (voiecur);
					maj_niv (0, 0, 0);
					premier_niveau ();
				}
			}
			else if (pvoie->temp1 == 5)
				rejet = 1;
			else
			{
				if (P_READ (voiecur))
				{
					read_only_alert (0, indd);
					return;
				}
				else
				{
					sprintf (chaine, "Invalid callsign \"%s\" !", indd);
					outln (chaine, strlen (chaine));
					md_echo (voiecur);
					dde_call = 1;
				}
			}
		}
		else if (pvoie->temp1 == 5)
			rejet = 1;
		else
		{
			if (P_READ (voiecur))
			{
				if (pvoie->temp1 == 10)
					rejet = 1;
				else
				{
					read_only_alert (0, indd);
					return;
				}
			}
			else
			{
				sprintf (chaine, "Unregistered callsign \"%s\" !", indd);
				outln (chaine, strlen (chaine));
				md_echo (voiecur);
				dde_call = 1;
			}
		}
	}

	if (rejet)
	{
		outln ("Sorry, you are not a registered user !    ", 42);
		outln ("Disconnected", 12);
		md_echo (voiecur);
		pvoie->deconnect = 4;
		pvoie->maj_ok = 0;
	}

	if (dde_call)
	{
		pvoie->read_only = 0;
		md_echo (voiecur);
		out ("Callsign :", 10);
	}
}


void passwd_modem (void)
{
	while ((*indd) && (!isgraph (*indd)))
		++indd;

	indd[20] = '\0';
	if (++pvoie->temp1 == 3)
	{
		outln ("Password error !", 16);
		outln ("Disconnected", 12);
		md_echo (voiecur);
		pvoie->deconnect = 5;
		pvoie->maj_ok = 0;
	}
	else if (ok_pass_modem ())
	{
		if (pvoie->temp2)
			md_no_echo (voiecur);
		/*
		   else
		   md_echo(voiecur); */
		else
			md_echo (voiecur);
		if (!pvoie->read_only)
			outln ("$WLogon Ok. Type NP to change password.$W", 41);
		maj_niv (0, 0, 0);
		premier_niveau ();
	}
	else
	{
		out ("Password :", 10);
		md_etoiles (voiecur);
	}
}


int ok_pass_modem (void)
{
	if (pvoie->read_only)
		return (1);
	return (strncmp (pvoie->finf.pass, strupr (sup_ln (indd)), 12) == 0);
}


void passwd_change (void)
{
	while_space ();
	switch (pvoie->niv3)
	{
	case 0:
		if (ok_pass_modem ())
		{
			out ("Enter new password :", 20);
			ch_niv3 (1);
		}
		else
		{
			outln ("Password error !", 16);
			retour_menu (N_MBL);
		}
		break;
	case 1:
		strn_cpy (12, pvoie->ch_temp, sup_ln (indd));
		out ("Once more :", 12);
		ch_niv3 (2);
		break;
	case 2:
		strupr (sup_ln (indd));
		if (strcmp (pvoie->ch_temp, indd) == 0)
		{
			strn_cpy (12, pvoie->finf.pass, indd);
			majinfo (voiecur, 2);
			md_echo (voiecur);
		}
		else
			outln ("Password error !", 16);
		md_echo (voiecur);
		retour_menu (N_MBL);
		break;
	}
}
