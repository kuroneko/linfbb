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

/* For main module */
#define PUBLIC

#include <serv.h>

#ifdef __ORB__
#include <fbb_orb.h>
#endif

#include <locale.h>	/* Added Satoshi Yasuda for NLS */
#include <sys/vfs.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>

#define NB_INIT_B 11

static int verbose = 0;
static int init_phase = 1;
static int info_canal = -1;

static void sig_fct (int sig)
{
	pid_t pid; 
	int pstatus;

	sig &= 0xff;
	if ((pid = wait (&pstatus)) != -1);
	signal (sig, sig_fct);

	switch (sig)
	{
	case SIGHUP:
		/* reload system files */
		if (verbose)
			fprintf (stderr, "Update system files\n");
		init_buf_fwd ();
		init_buf_swap ();
		init_buf_rej ();
		init_bbs ();
		break;
	case SIGTERM:
		/* end of session */
		if (verbose)
			fprintf (stderr, "Closing connections\n");
		maintenance ();
		fbb_quit (1);
		break;
	case SIGBUS:
		/* end of session */
		fprintf (stderr, "xfbbd : Bus error\n");
		exit (5);
		break;
	case SIGSEGV:
		/* end of session */
		fprintf (stderr, "xfbbd : Segmentation violation\n");
		exit (5);
		break;
	}
	
	/* Other signals are ignored */
}

static void xfbbd_init (void)
{
	while (step_initialisations (init_phase) == 0)
	{
		/* Next step */
		++init_phase;
	};
}

static char *XVersion (int dat)
{
	static char prodVersion[80];
	char sdate[30];

	if (dat)
		sprintf (sdate, " (%s)", date ());
	else
		*sdate = '\0';

	sprintf (prodVersion, "%s%s", version (), sdate);

	return (prodVersion);
}

void banner (void)
{
	fprintf (stderr,
			 "*********************************************************\n"
			 "* XFBB Linux daemon version %s PID=%d\n"
			 "* Copyright F6FBB 1986-1999. All rights reserved.\n"
			 "* Maintainer since 2000 : Bernard, f6bvp@free.fr\n"
			 "*\n"
			 "* This software is in the public domain. It can be copied\n"
			 "* or installed for any use abiding by the laws\n"
			 "* under GNU General Public License\n"
			 "*\n"
			 "* F6FBB (Jean-Paul ROUBELAT) and F6BVP (Bernard PIDOUX)\n"
			 "* decline any responsibilty in the use of XFBB software.\n"
			 "*\n"
			 "*********************************************************\n",
			 XVersion (TRUE), getpid ());
}

static int pactor_status[NBPORT];

void test_pactor(int force)
{
	int port;
	int p_status;
	
	for (port = 1; port < NBPORT; port++)
	{
		if ((p_port[port].pvalid) && (p_port[port].stop == 0))
		{
			if (!IS_PACTOR(port))
					continue;
					
			p_status = 0;
			if (pactor_scan[port])
				p_status |= PACTOR_SCAN;
			/* Connect monitoring */
			if (ISS(port))
				p_status |= PACTOR_ISS;
			if (ONLINE(port))
				p_status |= PACTOR_ONLINE;
			if (force || (p_status != pactor_status[port]))
			{
				pactor_status[port] = p_status;
#ifdef __ORB__
				orb_pactor_status(port, p_status);
#endif
			}
		}
	}
}

void process (void)
{
	FbbMem (0);
	kernel ();
	test_pactor(0);
}

#ifdef __ORB__
extern int fbb_orb (char *service, int port);

#endif

int main (int ac, char **av)
{
	int init_mode = 0;
	int orb = 1;
	int port = 3286;
	char *service = NULL;
	int i, ng;

	daemon_mode = 1;
	all_packets = 0;
	setlocale(LC_CTYPE, "");	/* Added Satoshi Yasuda for NLS */

	for (i = 1; i < NSIG; i++)
	{
		if (i == SIGBUS || i == SIGSEGV ||  i == SIGCHLD || i == SIGALRM)
			continue;
		else if (i == SIGHUP  || i == SIGTERM || i == SIGBUS || i == SIGSEGV)
			signal (i, sig_fct); /* Use sig_fct only for signals that do something */
		else
			signal (i, SIG_IGN); /* Otherwise ignore */
	}

	for (ng = 1; ng < ac; ng++)
	{
		if (strcmp (av[ng], "-h") == 0)
		{
			fprintf (stderr, "usage : xfbbd [-v] [-p port | -s service]\n");
			return (0);
		}
		else if (strcmp (av[ng], "-V") == 0)
		{
			printf("%s\n", version());
			return 0;
		}
		else if (strcmp (av[ng], "-v") == 0)
		{
			verbose = 1;
			fprintf (stderr, "Verbose mode is selected\n");
		}
		else if (strcmp (av[ng], "-a") == 0)
		{
			all_packets = 1;
		}
		else if (strcmp (av[ng], "-i") == 0)
		{
			init_mode = 1;
		}
		else if (strcmp (av[ng], "-p") == 0)
		{
			if ((ng + 1) != ac)
				port = atoi (av[ng + 1]);
		}
		else if (strcmp (av[ng], "-s") == 0)
		{
			if ((ng + 1) != ac)
				service = av[ng + 1];
		}
		else if (strcmp (av[ng], "-n") == 0)
		{
			orb = 0;
		}
	}

	banner ();

	/* Initialisations */
	xfbbd_init ();

	if (init_mode)
	{
		return (2);
	}

#ifdef __ORB__
	if (orb)
		fbb_orb (service, port);
	else
	{
#endif

		fprintf (stderr, "xfbbd ready and running ...\n");

		/* mainloop */
		for (;;)
		{
			process ();
			if (is_idle)
			{
				usleep (50000);
			}
			else
			{
				is_idle = 1;
			}
		}
		
#ifdef __ORB__
	}
#endif

	return 1;
}

char *itoa (int val, char *buffer, int base)
{
	sprintf (buffer, "%d", val);
	return buffer;
}

char *ltoa (long lval, char *buffer, int base)
{
	sprintf (buffer, "%ld", lval);
	return buffer;
}

char *ultoa (unsigned long lval, char *buffer, int base)
{
	sprintf (buffer, "%lu", lval);
	return buffer;
}

void InitText (char *text)
{
	static char *initext[NB_INIT_B] =
	{
#ifdef __linux__
		"Reading fbb.conf",
#else
		"Reading INIT.SRV",
#endif
		"Reading Texts (%s)",
		"Ports configuration (%s)",
		"TNC configuration (%s)",
		"Servers & PG (%s)",
		"Loading BIDs (%s)",
		"Callsigns set-up (%s)",
		"Messages set-up (%s)",
		"WP set-up (%s)",
		"Forward set-up (%s)",
		"BBS set-up (%s)"
	};

	if (!verbose)
		return;

	if (init_phase > NB_INIT_B)
		return;

	printf (initext[init_phase - 1], text);
#ifdef __linux__
	putchar ('\n');
#else
	putchar ('\r');
#endif
	fflush (stdout);

}


void InfoMessage (int temps, char *texte, char *titre)
{
	static char def_titre[80];

	if (!verbose)
		return;

	if (texte == NULL)
		return;

	if (titre == NULL)
		titre = def_titre;
	else
		strcpy (def_titre, titre);

	printf ("%s : %s\n", titre, texte);

}

#include <stdarg.h>
void WinDebug (char *fmt,...)
{
	va_list argptr;
	int cnt;

	if (!verbose)
		return;

	va_start (argptr, fmt);
	cnt = vprintf (fmt, argptr);
	va_end (argptr);
}
/*
   cmd : tableau des commandes a executer
   nb_cmd : nombre de commandes a executer
   mode : REPORT_MODE : attend un fichier log en retour 
   NO_REPORT_MODE : pas de fichier log en retour
   log   : nom du fichier de retour
   xdir  : repertoire dans lequel doivent s'executer les commandes
 */
int call_nbdos (char **cmd, int nb_cmd, int mode, char *log, char *xdir, char *data)
{

	/* Appel DOS */
	int i;
	int ExitCode = 0;

	char *ptr;
	char file[256];
	char buf[256];
	char dir[256];
	char arg[256];

	/* semi-column is forbidden for security reasons */
		
//	if (log)
	if (mode)
		//sprintf (file, " </dev/null >%s 2>&1", back2slash (log));
		sprintf (file, " </dev/null >%s", back2slash (log));
	else
		sprintf (file, " </dev/null");

	if (xdir)
	{
		ptr = strchr(xdir, ';');
		if (ptr)
			*ptr = '\0';
		
		sprintf (dir, "cd %s ; ", back2slash (xdir));
	}
	else
		*dir = '\0';

	*arg = '\0';
	
	if (data)
	{
		ptr = strchr(data, ';');
		if (ptr) {
			*ptr = '\0';
			// wrong, if semi-column not found there aren't arguments passed
			//sprintf (arg, " %s ", data);
		}		
		sprintf (arg, " %s ", data);
	}

	for (i = 0; i < nb_cmd; i++)
	{
		FILE *fp;
		int retour;
		char cmd_buf[1024];
		
		/* semi-column is forbidden for security reasons */
		ptr = strchr(cmd[i], ';');
		if (ptr)
			*ptr = '\0';

		sprintf (buf, "%s%s%s%s", dir, cmd[i], arg, file);

/* Dave van der Locht - 17-12-2020
Replaced system() with popen() to fix issue with filter response text no coming through */
   
		//retour = system (buf);
		
		fp = popen (buf, "r");
		if (fp == NULL)
			printf ("Failed to run command\n" );	
		else
			retour = pclose(fp);
		
		ExitCode = retour >> 8;			
		
		if (verbose) {		
			printf ("Debug: command = {%s}\n", buf);		
			printf ("Debug: exit code = %d\n", ExitCode);
		}

		/* fail-safe bypasses */		
		// if filter executable isn't found ExitCode = 127
		if (ExitCode == 127)
			ExitCode = 0;
		
/*F6BVP :  Why in FBB Linux system() always return -1 ??? */
/*Dave van der Locht: It's because the SIGCHLD signal was ignored, added SIGCHLD to line 190 */
		if (ExitCode == -1)
			ExitCode = 0;		

	}
	return (ExitCode);
}

void CompressPosition (int mode, int val, long numero)
{
	static long last_num = 0;

	if ((verbose) && (numero != last_num))
	{
		last_num = numero;
/*		printf ("%s : msg %ld\n", (mode) ? "Compress" : "Decompress", numero);*/
	}
}

int filter (char *ligne, char *buffer, int len, char *data, char *xdir)
{
	char deroute[80];
	int retour;

	sprintf (deroute, "%sEXECUTE.xxx", MBINDIR);
	retour = call_nbdos (&ligne, 1, REPORT_MODE, deroute, xdir, data);

/* F6BVP was if (retour != -1)
 * as retour is forced to 0 in call_nbos lets change test 
 * to not display deroute file name */
//	if (retour == -1)
	if (retour != -1) 
	{
		outfichs (deroute);
	}
	unlink (deroute);
	return (retour);
}

void sysop_call (char *texte)
{
}

void window_write (int numero, char *data, int len, int color, int header)
{
#ifdef __ORB__
	orb_write (numero, data, len, color, header);
#endif
}

void WinMessage (int temps, char *text)
{
	char str[256];
	fd_set sock_read;
	struct timeval to;

	FD_ZERO (&sock_read);
	FD_SET (0, &sock_read);
	to.tv_sec = temps;
	to.tv_usec = 0;

/*	fprintf (stderr, "Message : %s\n", text);*/
	if (select (1, &sock_read, NULL, NULL, &to) > 0)
		read (0, str, sizeof (str));
}

void win_status (char *txt)
{
}

#ifdef __ORB__
static int old_priv;
static int old_hold;
static int old_total;

void reset_msgs (void)
{
	old_priv = -1;
	old_hold = -1;
	old_total = -1;
}
#endif

void win_msg_cons (int priv, int hold)
{
#ifdef __ORB__
	unsigned num_indic;
	ind_noeud *noeud;

	if (priv == -1)
	{
		noeud = insnoeud (cons_call.call, &num_indic);
		priv = noeud->nbnew;
	}

	if (old_priv != priv || old_hold != nb_hold || old_total != nbmess)
	{
		old_priv = priv;
		old_hold = nb_hold;
		old_total = nbmess;
		orb_nb_msg (priv, nb_hold, nbmess);
	}
#endif
}

int fbb_list (int update)
{
#ifdef __ORB__
	static char lcnx[MAXVOIES][80];
	static int premier = 0;
	static int prec_con = 0;
	int nb_con = 0;
	int i;
	char buffer[80];

	if ((premier) || (update))
	{
		memset (lcnx, 0, NBVOIES * 80);
		premier = 0;
	}

	for (i = 0; i < NBVOIES; i++)
	{
		int ch;

		if (svoie[i]->sta.connect)
		{
			int ok = 0;
			int nobbs;
			struct tm *sdate;
			unsigned t_cnx;
			Forward *pfwd;
			char bbs[10];
			char call[20];
			int choix = 0;
			int fwd = 0;
			char fwd_char;

			++nb_con;

			/* Test du forward */
			ok = 0;
			*bbs = '\0';
			pfwd = p_port[no_port (i)].listfwd;
			while (pfwd)
			{
				if ((svoie[i]->curfwd) && (pfwd->forward == i))
				{
					nobbs = svoie[i]->bbsfwd;
					choix = (int) svoie[i]->cur_choix;
					strn_cpy (6, bbs, bbs_ptr + (nobbs - 1) * 7);
					ok = 1;
					fwd = 1;
				}
				pfwd = pfwd->suite;
			}

			if ((!ok) && (svoie[i]->mode & F_FOR) && (i != CONSOLE))
			{
				fwd = 2;
				strcpy (bbs, svoie[i]->sta.indicatif.call);
			}

			strlwr (bbs);

			/* Affichage */
			sdate = localtime (&(svoie[i]->debut));
			t_cnx = (unsigned) (time (NULL) - svoie[i]->debut);
			if (i == 1)
				ch = 99;
			else
				ch = (i > 0) ? i - 1 : i;
			if (svoie[i]->sta.indicatif.num)
				sprintf (call, "%s-%d", svoie[i]->sta.indicatif.call, svoie[i]->sta.indicatif.num);
			else
				sprintf (call, "%s", svoie[i]->sta.indicatif.call);

			if (fwd == 1)
				fwd_char = '<';
			else if (fwd == 2)
				fwd_char = '>';
			else
				fwd_char = ' ';

			sprintf (buffer, "%c%02d %-9s %02d:%02d %02d:%02d %2d %3d %c%c%s",
					 fwd_char,
					 ch,
					 call,
					 sdate->tm_hour,
					 sdate->tm_min,
					 t_cnx / 3600,
					 (t_cnx / 60) % 60,
					 svoie[i]->sta.ret,
					 svoie[i]->sta.ack,
					 (choix < 2) ? ' ' : '0' + choix,
					 (choix < 2) ? ' ' : '/',
					 bbs
				);
		}
		else
		{
			if (i == 1)
				ch = 99;
			else
				ch = (i > 0) ? i - 1 : i;
			sprintf (buffer, " %02d", ch);
		}

		if (strcmp (buffer, lcnx[i]) != 0)
		{
			orb_con_list (i, buffer);
			strcpy (lcnx[i], buffer);
		}
	}
	if (nb_con != prec_con)
	{
		prec_con = nb_con;
		orb_con_nb (nb_con);
	}
	return (nb_con);
#else
	return (0);
#endif
}

void FbbStatus (char *callsign, char *texte)
{
}

void set_info_channel(int channel)
{
	if (channel == -1)
	{
		user_status(-1);
	}
	else
	{
		info_canal = (channel > 0) ? channel+1 : channel;
		user_status(-1);
		user_status(info_canal);
	}
}

void user_status(int ch)
{
#ifdef __ORB__
	int i;
	char str[256];
	char call[80];
	indicat *ind;
	static struct
	{
		char str[256];
		char prenom[13];
		char home[41];
		stat_ch sta;
		int niv1, niv2, niv3, canal, nbmess, nbnew, paclen, memoc;
		unsigned flags;
	} prev;

	if (ch == -1)
	{
		memset(&prev, 0, sizeof(prev));
		prev.niv1 = prev.niv2 = prev.niv3 = -1;
		prev.canal = prev.nbmess = prev.nbnew = -1;
		prev.paclen = prev.memoc = -1;
		prev.sta.ret = prev.sta.ack = -1;
		*prev.str = 0xff;
		*prev.prenom = 0xff;
		*prev.home = 0xff;
		return;
	}

	if (ch != info_canal)
		return;

	ind = &svoie[info_canal]->sta.indicatif;
	if (memcmp(ind, &prev.sta.indicatif, sizeof(indicat)) != 0)
	{
		prev.sta.indicatif = *ind;
		if (ind->num)
			sprintf(call, "%s-%d", ind->call, ind->num);
		else
			sprintf(call, "%s", ind->call);
		orb_info(ICall, call);

		*str = '\0';
		for (i = 0; i < 2; i++)
		{
			ind = &svoie[info_canal]->sta.relais[i];
			if (*(ind->call))

			{
				if (ind->num)
					sprintf(call, "%s-%d", ind->call, ind->num);
				else
					sprintf(call, "%s", ind->call);
				if (i > 0)
					strcat(str, ",");
				strcat(str, call);
			}
		}
		ind = &svoie[info_canal]->sta.relais[i];
		if (*(ind->call))
			strcat(str, ",...");
		orb_info(IDigis, str);
	}

	if (strcmp(prev.prenom, svoie[info_canal]->finf.prenom) != 0)
	{
		strcpy(prev.prenom, svoie[info_canal]->finf.prenom);
		orb_info(IName, svoie[info_canal]->finf.prenom);
	}

	if (strcmp(prev.home, svoie[info_canal]->finf.home) != 0)
	{
		strcpy(prev.home, svoie[info_canal]->finf.home);
		orb_info(IHome, svoie[info_canal]->finf.home);
	}

	if (info_canal != prev.canal)
	{
		prev.canal = info_canal;
		orb_info(IChan,
				  itoa((info_canal > 0) ? info_canal - 1 : info_canal, str,
					   10));
		orb_info(IPort, itoa(no_port(info_canal), str, 10));
	}

	if ((svoie[info_canal]->niv1 != prev.niv1) ||
		(svoie[info_canal]->niv2 != prev.niv2) ||
		(svoie[info_canal]->niv3 != prev.niv3))
	{
		prev.niv1 = svoie[info_canal]->niv1;
		prev.niv2 = svoie[info_canal]->niv2;
		prev.niv3 = svoie[info_canal]->niv3;
		sprintf(str, "%02d %02d %02d",
				svoie[info_canal]->niv1, svoie[info_canal]->niv2,
				svoie[info_canal]->niv3);
		orb_info(IN1N2N3, str);
	}

	if (svoie[info_canal]->finf.flags != prev.flags)
	{
		prev.flags = svoie[info_canal]->finf.flags;
		orb_info(IFlags, strflags(&svoie[info_canal]->finf));
	}

	if (svoie[info_canal]->paclen != prev.paclen)
	{
		prev.paclen = svoie[info_canal]->paclen;
		orb_info(IPaclen, itoa(svoie[info_canal]->paclen, str, 10));
	}

	if (svoie[info_canal]->sta.stat != prev.sta.stat)
	{
		prev.sta.stat = svoie[info_canal]->sta.stat;
		orb_info(IStatus, stat_voie(info_canal));
	}

	if (svoie[info_canal]->memoc != prev.memoc)
	{
		prev.memoc = svoie[info_canal]->memoc;
		orb_info(IMem, itoa(svoie[info_canal]->memoc, str, 10));
	}

	if (svoie[info_canal]->sta.ack != prev.sta.ack)
	{
		prev.sta.ack = svoie[info_canal]->sta.ack;
		orb_info(IBuf, itoa(svoie[info_canal]->sta.ack, str, 10));
	}

	if (svoie[info_canal]->sta.ret != prev.sta.ret)
	{
		prev.sta.ret = svoie[info_canal]->sta.ret;
		orb_info(IRet, itoa(svoie[info_canal]->sta.ret, str, 10));
	}

	if (svoie[info_canal]->ncur)
	{
		if (svoie[info_canal]->ncur->nbmess != prev.nbmess)
		{
			prev.nbmess = svoie[info_canal]->ncur->nbmess;
			orb_info(IPerso, itoa(svoie[info_canal]->ncur->nbmess, str, 10));
		}
		if (svoie[info_canal]->ncur->nbnew != prev.nbnew)
		{
			prev.nbnew = svoie[info_canal]->ncur->nbnew;
			orb_info(IUnread, itoa(svoie[info_canal]->ncur->nbnew, str, 10));
		}
	}
	else
	{
		orb_info(IPerso, "0");
		orb_info(IUnread, "0");
	}

	if (svoie[info_canal]->niv1 == N_YAPP)
		yapp_str(info_canal, str);
	else if (svoie[info_canal]->niv1 == N_BIN)
		abin_str(info_canal, str);
	else if (svoie[info_canal]->niv1 == N_XFWD)
		xfwd_str(info_canal, str);
	else if (svoie[info_canal]->niv1 == N_FORW)
		ffwd_str(info_canal, str);
	else
		*str = '\0';

	if (strcmp(str, prev.str) != 0)
	{
		strcpy(prev.str, str);
		orb_info(IYapp, str);
	}

	orb_info(0, NULL);

#endif	
}

void maj_menu_options (void)
{
#ifdef __ORB__
	orb_options();
#endif	
}

void CloseFbbWindow (int numero)
{
#ifdef __ORB__
	if (numero == CONSOLE)
	{
		if (v_tell)
		{
			selvoie (v_tell);
			pvoie->seq = v_tell = 0;
			retour_mbl ();

			/*maj_niv (pvoie->sniv1, pvoie->sniv2, pvoie->sniv3);
			   prompt (pvoie->finf.flags, pvoie->niv1); */

			selvoie (CONSOLE);
			maj_niv (0, 0, 0);
			pvoie->sta.connect = FALSE;
		}
		orb_disc ();
	}
#endif
}

int xfbb_edit (void)
{
	return 1;
}

int end_xfbb_edit (void)
{
	return 1;
}

int call_dll (char *cmd, int mode, char *buffer, int len, char *data)
{
	return 1;
}

static FILE *p_fptr = NULL;
static int p_pos = 0;
void RequestPendingForward(char *datafile)
{
	p_pos = 0;
	p_fptr = fopen(datafile, "w");
	if (p_fptr)
	{
		fwd_encours();
		fclose(p_fptr);
	}
}

void AddPendingLine (char *call, int priv, int bull, int kb)
{
	if (p_fptr == NULL)
		return;
	if (*call)
		fprintf(p_fptr, "%02d %s %d %d %d\n", p_pos++, call, priv, bull, kb);
	else
		fprintf(p_fptr, "%02d\n", p_pos++);
}

char *StartForward(int numbbs)
{
	static char str[80];
	char ifwd[NBBBS][7];
	char bbs[8];
	int retour;
	int port_fwd;
	
	ch_bbs (1, ifwd);
	if (numbbs < 0 || numbbs >= NBBBS || *ifwd[numbbs] == '\0')
	{
		sprintf(str, "Unknown BBS nb %d", numbbs);
		return str;
	}
	
	strn_cpy(6, bbs, ifwd[numbbs]);
	retour = val_fwd(bbs, &port_fwd, 1);
	if (retour < 0)
	{
		switch (retour)
		{
	    case -1 :
			sprintf(str, "No forwarding channel on port %d", port_fwd);
			break;
	    case -2:
			sprintf(str, "No port affected to %s", bbs);
			break;
	    case -3 :
			sprintf(str, "Unknown BBS %s", bbs);
			break;
	    case -4 :
			sprintf(str, "BBS %s already connected", bbs);
			break;
	    default :
			sprintf(str, "Unknown error code %d", retour);
			break;
		}
	}
	else
	{
		sprintf(str, "Starting forward to %s on port %d", bbs, port_fwd);
	}
	
	return str;
}

char *StopForward(int numbbs)
{
	static char str[80];
	char ifwd[NBBBS][7];
	char bbs[8];
	int port_fwd;
	
	ch_bbs (1, ifwd);
	if (numbbs < 0 || numbbs >= NBBBS || *ifwd[numbbs] == '\0')
	{
		sprintf(str, "Unknown BBS nb %d", numbbs);
		return str;
	}
	
	strn_cpy(6, bbs, ifwd[numbbs]);
	port_fwd = dec_fwd(bbs);
	if (port_fwd < 0)
	{
		switch (port_fwd)
		{
		case -1 :
			sprintf(str, "BBS %s is not forwarding", bbs);
			break;
		case -2 :
			sprintf(str, "Unknown BBS %s", bbs);
			break;
		default :
			sprintf(str, "Unknown error code %d", port_fwd);
			break;
		}
	}
	else
	{
		sprintf(str, "Stopping forward with %s on port %d", bbs, port_fwd);
	}
	
	return str;
}

void window_connect (int numero)
{
}

void ShowError (char *titre, char *info, int lig)
{
	printf ("%s : %s %d\n", titre, info, lig);
}

void fbb_quit (unsigned retour)
{
	sortie_prg ();
	exit (retour);
}

void WinMSleep (unsigned milliseconds)
{
	usleep (milliseconds * 1000);
}

void WinSleep (unsigned seconds)
{
	sleep (seconds);
}

int sel_option (char *texte, int *val)
{
	char str[256];

	for (;;)
	{
		fprintf (stderr, "%s (Y/N) ? ", texte);
		fflush (stdout);

		read (0, str, sizeof (str));
		if ((*str == 'Y') || (*str == 'y'))
		{
			*val = 'y';
			break;
		}
		if ((*str == 'N') || (*str == 'n'))
			break;
	}

	return (*val == 'y');
}

void aff_traite (int voie, int val)
{
}

void bipper (void)
{
	if (!bip)
		return;
}

void music (int stat)
{
	int i, j;
	int pid;
	static int pid_fils = 0;

	if (stat)
	{
		t_tell = 1000;

		pid = fork ();
		if (pid == 0)
		{
			/* fils */
			for (j = 0; j < 5; j++)
			{
				if (!play ("syscall.wav"))
				{
					int fd;
					char c = '\a';

					fd = open ("/dev/console", O_WRONLY);
					if (fd > 0)
					{
						for (i = 0; i < 10; i++)
						{
							write (fd, &c, 1);
							usleep (200000);
						}
						close (fd);
					}
				}
				sleep (10);
			}
			exit (0);
		}
		else
			pid_fils = pid;
	}
	else
	{
		if (pid_fils)
			kill (pid_fils, SIGKILL);
		pid_fils = 0;
		t_tell = -1;
	}
}

void SpoolLine (int voie, int attr, char *data, int lg)
{
}

void window_init (void)
{
}

void set_win_colors (void)
{
}

void window_disconnect (int numero)
{
}

void disconnect_channel (int channel, int immediate)
{
	int ch = (channel > 0) ? channel+1 : channel;

	if ((svoie[ch]->sta.connect) && (ch) && (ch < NBVOIES))
	{
		if (immediate)
			force_deconnexion (ch, 1);
		else
			deconnexion (ch, 1);
	}
}

void sysop_end (void)
{
}

void FbbMem (int update)
{
	static long old_avail = 0;
	static time_t old_time = 0;
	static int old_getd = 0;
	static long old_nbmess = -1L;
	static long old_temp = -1L;
	static long old_gMem = -1L;
	static long old_us = 0xffffffffL;

	int chg = 0;
	char texte[80];
	int gMem = nb_ems_pages ();
	long us = mem_alloue;

	time_t new_time = time (NULL);

	if (update)
	{
		old_avail = 0;
		old_time = 0;
		old_getd = 0;
		old_nbmess = -1L;
		old_temp = -1L;
		old_gMem = -1;
		old_us = 0xffffffffL;
	}

	if (operationnel == -1)
		return;

	/* Mise a jour toutes les secondes */
	if (old_time == new_time)
		return;

	old_time = new_time;

	if (us < 0L)
		us = 0L;

	if (us != old_us)
	{
		old_us = us;
		chg = 1;
	}

	if (gMem != old_gMem)
	{
		old_gMem = gMem;
		chg = 1;
	}

	/* Test disque toutes les 10 secondes */
	if (old_getd == 0)
	{
		struct statfs dfree;

		if (statfs (DATADIR, &dfree) == 0)
		{
			sys_disk = dfree.f_bavail * (dfree.f_bsize / 1024L);
			if (sys_disk != old_avail)
			{
				old_avail = sys_disk;
				chg = 1;
			}
		}

		if (statfs (MBINDIR, &dfree) == 0)
		{
			tmp_disk = dfree.f_bavail * (dfree.f_bsize / 1024L);
			if (tmp_disk != old_avail && tmp_disk != old_temp)
			{
				old_temp = tmp_disk;
				chg = 1;
			}
		}
		else
		{
			tmp_disk = sys_disk;
		}


		old_getd = 10;
	}
	else
		--old_getd;

#ifdef __ORB__
	if (chg)
	{
		orb_status (old_us, old_gMem, old_avail, old_temp);
	}
#endif

	if (nbmess != old_nbmess)
	{
		sprintf (texte, ": %ld", nbmess);
		old_nbmess = nbmess;
	}

}

void DisplayResync (int port, int nb)
{
	static int tot_resync = 0;

	if (nb)
	{
		if (nb == 1)
		{
			printf ("Resynchro port %d (total = %d)\n", port, ++tot_resync);
		}
	}
	else
	{
		printf ("Port %d OK\n", port);
	}
}

int connect_tell (void)
{
	char s[256];

	if ((!svoie[CONSOLE]->sta.connect) && (svoie[v_tell]->sta.connect))

	{

		console_on ();

#ifdef ENGLISH
		sprintf (s, "*** Talking with %s (%s) ", svoie[v_tell]->sta.indicatif.call, svoie[v_tell]->finf.prenom);

#else
		sprintf (s, "*** Convers. avec %s (%s)", svoie[v_tell]->sta.indicatif.call, svoie[v_tell]->finf.prenom);

#endif
		selvoie (CONSOLE);
		pvoie->sta.connect = 16;
		pvoie->deconnect = FALSE;
		pvoie->ret = 0;
		pvoie->sid = 0;
		pvoie->pack = 0;
		pvoie->read_only = 0;
		pvoie->vdisk = 2;
		pvoie->xferok = 1;
		pvoie->msg_held = 0;
		pvoie->mess_recu = 1;
		pvoie->mbl = 0;
		init_timout (CONSOLE);
		pvoie->temp3 = 0;
		pvoie->nb_err = 0;
		pvoie->finf.lang = langue[0]->numlang;
		init_langue (voiecur);
		maj_niv (N_MBL, 9, 2);
		outln (s, strlen (s));
		selvoie (v_tell);
		init_langue (voiecur);
		maj_niv (N_MBL, 9, 2);
		texte (T_MBL + 15);
		return 1;
	}
	return 0;
}

void RequestMsgsList(char *datafile)
{
	p_pos = 0;
	p_fptr = fopen(datafile, "w");
	if (p_fptr)
	{
		FbbRequestMessageList();
		fclose(p_fptr);
	}
}

void AddMessageList (char *number)
{
	if (p_fptr == NULL)
		return;
	fprintf(p_fptr, "%s\n", number);
}


char *GetMsgInfo(char *number, int *nLen)
{
	static char buf[1024];
	int nNum = 1;
	int nPos = 0;
	bullist rec;
	int i;
	
	if (GetMsgInfos (&rec, atol(number)))
	{
		nPos += sprintf(buf+nPos, "%d %c\n", nNum++, rec.type);
		nPos += sprintf(buf+nPos, "%d %c\n", nNum++, rec.status);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.numero);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.taille);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.date);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.bbsf);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.bbsv);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.exped);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.desti);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.bid);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.titre);
		nPos += sprintf(buf+nPos, "%d %d\n", nNum++, rec.bin);
		nPos += sprintf(buf+nPos, "%d %u\n", nNum++, rec.nblu);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.theme);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.datesd);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.datech);
		nPos += sprintf(buf+nPos, "%d", nNum++);
		for (i = 0 ; i < NBMASK ; i++)
			nPos += sprintf(buf+nPos, " %02x", rec.fbbs[i] & 0xff);
		nPos += sprintf(buf+nPos, "\n");
		nPos += sprintf(buf+nPos, "%d", nNum++);
		for (i = 0 ; i < NBMASK ; i++)
			nPos += sprintf(buf+nPos, " %02x", rec.forw[i] & 0xff);
		nPos += sprintf(buf+nPos, "\n");
	}
	
	*nLen = nPos;
	return buf;
}

int PutMsgInfo(char *number, char *buf, int nLen)
{
	char *ptr;
	int nNum;	
	bullist rec;
	int i;
	int fwd[NBMASK];
	long NumMess = atol(number);
	
	if (!GetMsgInfos (&rec, NumMess))
		return 0;

	ptr = strtok(buf, "\n");
	while (ptr)
	{
		nNum = atoi(ptr);
		while (isdigit(*ptr))
			++ptr;
		while (isspace(*ptr))
			++ptr;
		switch (nNum)
		{
		case 1:
			rec.type = *ptr;
			break;
		case 2:
			rec.status = *ptr;
			break;
		case 3:
			rec.numero = atol(ptr);
			break;
		case 4:
			rec.taille = atol(ptr);
			break;
		case 5:
			rec.date = atol(ptr);
			break;
		case 6:
			strn_cpy(40, rec.bbsf, ptr);
			break;
		case 7:
			strn_cpy(40, rec.bbsv, ptr);
			break;
		case 8:
			strn_cpy(6, rec.exped, ptr);
			break;
		case 9:
			strn_cpy(6, rec.desti, ptr);
			break;
		case 10:
			strn_cpy(12, rec.bid, ptr);
			break;
		case 11:
			n_cpy(60, rec.titre, ptr);
			break;
		case 12:
			rec.bin = (*ptr == '0') ? '\0' : '\1';
			break;
		case 13:
			rec.nblu = atoi(ptr);
			break;
		case 14:
			rec.theme = atol(ptr);
			break;
		case 15:
			rec.datesd = atol(ptr);
			break;
		case 16:
			rec.datech = atol(ptr);
			break;
		case 17:
			sscanf(ptr, "%x %x %x %x %x %x %x %x", 
					&fwd[0], &fwd[1], &fwd[2], &fwd[3],
					&fwd[4], &fwd[5], &fwd[6], &fwd[7]);
			for (i = 0 ; i < NBMASK ; i++)
				rec.fbbs[i] = (char) fwd[i];
			break;
		case 18:
			sscanf(ptr, "%x %x %x %x %x %x %x %x", 
					&fwd[0], &fwd[1], &fwd[2], &fwd[3],
					&fwd[4], &fwd[5], &fwd[6], &fwd[7]);
			for (i = 0 ; i < NBMASK ; i++)
				rec.forw[i] = (char) fwd[i];
			break;
		}
		ptr = strtok(NULL, "\n");
	}
	
	SetMsgInfo(&rec, NumMess);
	
	return 1;
}

void RequestUsersList(char *datafile)
{
	p_pos = 0;
	p_fptr = fopen(datafile, "w");
	if (p_fptr)
	{
		FbbRequestUserList();
		fclose(p_fptr);
	}
}

void AddUserList (char *callsign)
{
	if (p_fptr == NULL)
		return;
	fprintf(p_fptr, "%s\n", callsign);
}

void AddUserLang (char *lang)
{
	if (p_fptr == NULL)
		return;
	fprintf(p_fptr, "%d %s\n", p_pos++, lang);
}


char *GetUserInfo(char *call, int *nLen)
{
	static char buf[1024];
	int nNum = 1;
	int nPos = 0;
	info rec;
	int i;
	
	if (GetUserInfos (call, &rec))
	{
		nPos += sprintf(buf+nPos, "%d %s-%d\n", nNum++, rec.indic.call, rec.indic.num);
		for (i = 0 ; i < 8 ; i++)
			nPos += sprintf(buf+nPos, "%d %s-%d\n", nNum++, rec.relai[i].call, rec.relai[i].num);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.lastmes);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.nbcon);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.hcon);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.lastyap);
		nPos += sprintf(buf+nPos, "%d %u\n", nNum++, rec.flags);
		nPos += sprintf(buf+nPos, "%d %u\n", nNum++, rec.on_base);
		nPos += sprintf(buf+nPos, "%d %u\n", nNum++, rec.nbl);
		nPos += sprintf(buf+nPos, "%d %u\n", nNum++, rec.lang);
		nPos += sprintf(buf+nPos, "%d %ld\n", nNum++, rec.newbanner);
		nPos += sprintf(buf+nPos, "%d %u\n", nNum++, rec.download);
		nPos += sprintf(buf+nPos, "%d %d\n", nNum++, rec.theme);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.nom);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.prenom);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.adres);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.ville);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.teld);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.telp);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.home);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.qra);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.priv);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.filtre);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.pass);
		nPos += sprintf(buf+nPos, "%d %s\n", nNum++, rec.zip);
	}
	
	*nLen = nPos;
	return buf;
}

int PutUserInfo(char *call, char *buf, int nLen)
{
	int nNum;
	info rec;
	char *ptr;
	
	if (!GetUserInfos (call, &rec))
		return 0;
		
	ptr = strtok(buf, "\n");
	while (ptr)
	{
		nNum = atoi(ptr);
		while (isdigit(*ptr))
			++ptr;
		while (isspace(*ptr))
			++ptr;
			
		switch (nNum)
		{
		case 11:
			rec.lastmes = atol(ptr);
			break;
		case 12:
			rec.hcon = atol(ptr);
			break;
		case 13:
			rec.lastyap = atol(ptr);
			break;
		case 14:
			rec.flags = atoi(ptr);
			break;
		case 15:
			rec.on_base = atoi(ptr);
			break;
		case 16:
			rec.nbl = atoi(ptr);
			break;
		case 17:
			rec.lang = atoi(ptr);
			break;
		case 18:
			rec.newbanner = atol(ptr);
			break;
		case 19:
			rec.download = atoi(ptr);
			break;
		case 20:
			rec.theme = atoi(ptr);
			break;
		case 21:
			n_cpy(17, rec.nom, ptr);
			break;
		case 22:
			n_cpy(12, rec.prenom, ptr);
			break;
		case 23:
			n_cpy(60, rec.adres, ptr);
			break;
		case 24:
			n_cpy(30, rec.ville, ptr);
			break;
		case 25:
			n_cpy(12, rec.teld, ptr);
			break;
		case 26:
			n_cpy(12, rec.telp, ptr);
			break;
		case 27:
			n_cpy(40, rec.home, ptr);
			break;
		case 28:
			strn_cpy(6, rec.qra, ptr);
			break;
		case 29:
			n_cpy(12, rec.priv, ptr);
			break;
		case 30:
			n_cpy(6, rec.filtre, ptr);
			break;
		case 31:
			strn_cpy(12, rec.pass, ptr);
			break;
		case 32:
			n_cpy(8, rec.zip, ptr);
			break;
		}
		ptr = strtok(NULL, "\n");
	}
	SetUserInfos(call, &rec);
	
	return 1;
}

int DelUserInfo(char *call)
{
	info rec;
	FILE *fptr;
	ind_noeud *noeud;

	noeud = cher_noeud(call);
	if (noeud == NULL)
		return 0;

	fptr = ouvre_nomenc() ;
	fseek(fptr, (long)noeud->coord * ((long) sizeof(info)), 0) ;
	fread((char *) &rec, (int) sizeof(info), 1, fptr) ;
	
	*(rec.indic.call) = '\0';
	
	fseek(fptr, (long)noeud->coord * ((long) sizeof(info)), 0) ;
	fwrite((char *) &rec, (int) sizeof(info), 1, fptr) ;
	ferme(fptr, 40) ;

	noeud->coord = 0xffff;
	
	return 1;
}

int NewUserInfo(char *call)
{
	info rec;
	indicat callsign;
	unsigned num_indic;
	FILE *fptr;
	ind_noeud *noeud;

	n_cpy(6, callsign.call, call);
	callsign.num = 0;

	noeud = insnoeud(callsign.call, &num_indic);
	if (noeud->coord != 0xffff)
		return 0;

	init_info(&rec, &callsign);

	noeud->coord = rinfo++;
	noeud->val = 1;

	fptr = ouvre_nomenc() ;
	fseek(fptr, (long)noeud->coord * ((long) sizeof(info)), 0) ;
	fwrite((char *) &rec, (int) sizeof(info), 1, fptr) ;
	ferme(fptr, 40) ;
	
	return 1;
}

void CmdCHO(int port, int val)
{
	if (IS_PACTOR(port) && (ONLINE(port)))
	{
		// Tue l'eventuel timer en cours
		del_timer(p_port[port].t_iss);
		p_port[port].t_iss = NULL;
		if (val)
		{
			tor_stop(p_port[port].pr_voie);
		}
		else
		{
			tor_start(p_port[port].pr_voie);
		}
	}
}

void CmdScan(int port, int val)
{
	char cmde[80];

	/* Start/Stop scanning */
	wsprintf(cmde, "PTCTRX SCAN %d", val);
	ptctrx(0, cmde);
}
