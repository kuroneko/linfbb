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

/*
 * Execution de programmes pour l'utilisateur
 *
 */

static void dir_pg (void);
static void pg_commande (void);
static void load_pg (void);

static int nbpg = 0;

#ifdef __LINUX__

#undef open

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/signal.h>

cmdlist *tete_cmd = NULL;

static int pty_open (int *pty, struct winsize *winsize)
{
	int pid;
	int i;
	char line[20];
	int c;
	int tty, devtty;

	for (c = 'p'; c <= 's'; c++)
	{
		for (i = 0; i < 16; i++)
		{
			sprintf (line, "/dev/pty%c%x", c, i);
			*pty = open (line, O_RDWR | O_NOCTTY);
			if (*pty >= 0)
				break;
		}
		if (*pty >= 0)
			break;
	}

	if (*pty < 0)
	{
		fprintf (stderr, "Out of pty\n");
		return -1;
	}

	ioctl (*pty, TIOCEXCL, NULL);

	if ((pid = fork ()) != 0)
		return pid;

	/* child */
	close (*pty);

	signal (SIGHUP, SIG_DFL);

	line[5] = 't';
	tty = open (line, O_RDWR);
	if (tty < 0)
	{
		fprintf (stderr, "Cannot open slave side\n");
		close (*pty);
		return -1;
	}
	(void) chown (line, getuid (), getgid ());
	(void) chmod (line, 0600);

	setsid ();					/* will break terminal affiliation */
	ioctl (tty, TIOCSCTTY, (char *) 0);

	setuid (getuid ());
	setgid (getgid ());

	devtty = open ("/dev/tty", O_RDWR);
	if (devtty < 0)
	{
		perror ("cannot open /dev/tty");
		exit (1);
	}

	ioctl (devtty, TIOCSWINSZ, winsize);
	close (tty);
	dup2 (devtty, 0);
	dup2 (devtty, 1);
	dup2 (devtty, 2);

	return 0;
}

int is_cmd (int voie)
{
	struct timeval to;
	fd_set fds_read;

	if (svoie[voie]->niv1 == N_MBL && svoie[voie]->niv2 == 20 && svoie[voie]->niv3 > 0)
	{
		FD_ZERO (&fds_read);
		FD_SET (svoie[voie]->niv3, &fds_read);

		to.tv_sec = 0;
		to.tv_usec = 0;

		return (select (svoie[voie]->niv3 + 1, &fds_read, NULL, NULL, &to) > 0);
	}
	return 0;
}

void exec_cmd (cmdlist * cptr)
{
	if (pvoie->niv3 == 0)
	{
		int pid, ac;
		struct winsize win =
		{24, 80, 0, 0};
		char *av[20];

		if (cptr == NULL)
			return;

		/* Create the child process */
		pid = pty_open (&pvoie->niv3, &win);
		if (pid == 0)
		{
			/* child */
			struct termios termios;
			char *line;

			memset ((char *) &termios, 0, sizeof (termios));

			ioctl (0, TIOCSCTTY, (char *) 0);

			termios.c_iflag = ICRNL | IXOFF;
			termios.c_oflag = OPOST | ONLCR;
			termios.c_cflag = CS8 | CREAD /*| CLOCAL */ ;
			termios.c_lflag = ISIG | ICANON;
			termios.c_cc[VINTR] = 127;
			termios.c_cc[VQUIT] = 28;
			termios.c_cc[VERASE] = 8;
			termios.c_cc[VKILL] = 24;
			termios.c_cc[VEOF] = 4;
			cfsetispeed (&termios, B38400);
			cfsetospeed (&termios, B38400);
			tcsetattr (0, TCSANOW, &termios);

			/* Transform the command line to list of arguments */
			sup_ln (indd);
			while (*indd && (!isspace (*indd)))
				++indd;

			line = calloc (1, strlen (cptr->action) + strlen (indd) + 2);
			strcpy (line, cptr->action);
			strcat (line, indd);

			ac = 0;
			av[ac] = strtok (line, " \t\n\r");
			while (av[ac])
				av[++ac] = strtok (NULL, " \t\n");

			/* 
			   envc = 0;
			   envp[envc] = (char *) malloc (30);
			   sprintf (envp[envc++], "AXCALL=%s", call);
			   envp[envc] = (char *) malloc (30);
			   sprintf (envp[envc++], "PROTOCOL=%s", protocol);
			   envp[envc] = (char *) malloc (30);
			   sprintf (envp[envc++], "TERM=linux");
			   envp[envc] = NULL;
			 */

			execvp (av[0], av);

			/* should never go there */
			exit (1);
		}

	}


	else
	{
		/* read/write the child process */
		if (*indd)
			write (pvoie->niv3, indd, strlen (indd));

		if (is_cmd (voiecur))
		{
			char buf[250];
			int cnt;

			cnt = read (pvoie->niv3, buf, sizeof (buf));
			if (cnt <= 0)
			{
				/* End of connection */
				close (pvoie->niv3);
				pvoie->niv3 = 0;
				retour_mbl ();
				return;
			}
			out (buf, cnt);
		}
	}
}
#endif

void exec_pg (void)
{
	int n_sauve;
	int ret;
	char s[256];
	char *ptr = NULL;
	FILE *fptr;

	if (pvoie->niv3 == 0)
	{
		/* if ... incindd(); */
		teste_espace ();
		sup_ln (indd);
		if (*indd)
		{
			ptr = pvoie->appendf;
			while (ISGRAPH (*indd))
				*ptr++ = *indd++;
			*ptr = '\0';
			if (!tst_point (pvoie->appendf))
			{
				retour_mbl ();
				return;
			}
			while_space ();
		}
		else
		{
			dir_pg ();
			retour_mbl ();
			return;
		}
	}

	indd[80] = '\0';

	if (pvoie->ncur->coord != 0xffff)
	{
		/* Ecrit le record de l'utilisateur en fichier */
		fptr = ouvre_nomenc ();
		fseek (fptr, pvoie->ncur->coord * ((long) sizeof (info)), 0);
		fwrite ((char *) &(pvoie->finf), (int) sizeof (info), 1, fptr);
		ferme (fptr, 18);
	}

#ifdef __WINDOWS__
#define	PG_BUF	32000
		char *ptr = m_alloue (PG_BUF);

		if (ptr == NULL)
			return;

		memset (ptr, '\0', PG_BUF);

#ifdef __WIN32__
		sprintf (s, "PG32\\%s %s-%d %d %u %d %s",
			pvoie->appendf, pvoie->sta.indicatif.call, pvoie->sta.indicatif.num,
			pvoie->niv3, pvoie->finf.flags, pvoie->ncur->coord, indd);
#else
		sprintf (s, "PG\\%s %s-%d %d %u %d %s",
			pvoie->appendf, pvoie->sta.indicatif.call, pvoie->sta.indicatif.num,
			pvoie->niv3, pvoie->finf.flags, pvoie->ncur->coord, indd);
#endif
		ret = filter (s, ptr, PG_BUF, NULL, NULL);

		ptr[PG_BUF - 1] = '\0';
		if (*ptr)
			out (ptr, strlen (ptr));

		m_libere (ptr, PG_BUF);
#endif
#ifdef __LINUX__
	
		char deroute[80];
		char *pptr = s;

		sprintf (deroute, "%sEXECUTE.xxx", MBINDIR);
		sprintf (s, "./%s %s-%d %d %u %d %s",
				 strlwr (pvoie->appendf), 
				 pvoie->sta.indicatif.call, pvoie->sta.indicatif.num,
				 pvoie->niv3, pvoie->finf.flags, pvoie->ncur->coord, indd);

		ret = call_nbdos (&pptr, 1, REPORT_MODE, deroute, PGDIR, NULL);
		if (ret != -1)
		{
			outfich (deroute);
		}
		unlink (deroute);
#endif
/*
   #ifdef __FBBDOS__

   sprintf (s, "PG\\%s %s-%d %d %u %d %s",
   pvoie->appendf, pvoie->sta.indicatif.call, pvoie->sta.indicatif.num,
   pvoie->niv3, pvoie->finf.flags, pvoie->ncur->coord, indd);

   ret = send_dos (4, s, NULL);

   #endif
 */
	if (pvoie->ncur->coord != 0xffff)
	{
		/* Relit le record de l'utilisateur du fichier */
		fptr = ouvre_nomenc ();
		fseek (fptr, pvoie->ncur->coord * ((long) sizeof (info)), 0);
		fread ((char *) &(pvoie->finf), (int) sizeof (info), 1, fptr);
		ferme (fptr, 18);
	}

	switch (ret)
	{

	case 0:
		init_hold ();
		retour_mbl ();
		break;

	case 1:
		++pvoie->niv3;
		break;

	case 2:
		pvoie->deconnect = 6;
		break;

	case 3:
		pg_commande ();
		break;

	case 4:
		n_sauve = ++pvoie->niv3;
		init_hold ();
		pg_commande ();
		maj_niv (N_MBL, 17, n_sauve);
		break;

	case 5:
		break;

	case -1:
		clear_outbuf (voiecur);
		strcpy (varx[0], pvoie->appendf);
		texte (T_ERR + 1);
		retour_mbl ();
		break;

	default:
		retour_mbl ();
		break;
	}
}

static void dir_pg (void)
{
	int num = 0;
	char str[80];
	char *ptr;

#ifdef __WINDOWS__
#ifdef __WIN32__
	if (findfirst ("PG32\\*.*", &(pvoie->dirblk), FA_DIREC))
#else
	if (findfirst ("PG\\*.*", &(pvoie->dirblk), FA_DIREC))
#endif
	{
		texte (T_DOS + 2);
		return;
	}
#endif
#ifdef __FBBDOS__
	if (findfirst ("PG\\*.*", &(pvoie->dirblk), FA_DIREC))
	{
		texte (T_DOS + 2);
		return;
	}
#endif
#ifdef __LINUX__
	{
		cmdlist *clist = tete_cmd;

		while (clist)
		{
			if (num == 4)
			{
				cr ();
				num = 0;
			}
			sprintf (str, "%-10s", clist->cmd);
			out (str, 10);
			clist = clist->next;
		}
		if (num != 0)
			cr ();
	}
	num = 0;

	sprintf(str, "%s*", PGDIR);
	if (findfirst (str, &(pvoie->dirblk), FA_DIREC))
	{
		texte (T_DOS + 2);
		return;
	}
#endif
	if (*pvoie->dirblk.ff_name == '.')
	{
		findnext (&(pvoie->dirblk));
		if (findnext (&(pvoie->dirblk)))
		{
			texte (T_DOS + 2);
			return;
		}
	}

	while (1)
	{
		if ((pvoie->dirblk.ff_attrib & FA_DIREC) == 0)
		{
			if (num == 4)
			{
				cr ();
				num = 0;
			}
			ptr = strrchr (pvoie->dirblk.ff_name, '.');
			if (ptr)
				*ptr = '\0';
			sprintf (str, "%-10s", pvoie->dirblk.ff_name);
			out (str, 10);
			++num;
		}
		if (findnext (&(pvoie->dirblk)))
			break;
	}
	if (num != 0)
		cr ();
}

static void pg_commande (void)
{
	obuf *optr;
	char *ptr, *tptr;
	int ncars, nbcars, nocars;
	int maxcar = DATABUF;

	if ((optr = pvoie->outptr) != NULL)
	{
		ncars = 0;
		tptr = data;
		nbcars = optr->nb_car;
		nocars = optr->no_car;
		ptr = optr->buffer + nocars;
		while (1)
		{
			if (nbcars == nocars)
			{
				pvoie->outptr = optr->suiv;
				m_libere (optr, sizeof (obuf));
				pvoie->memoc -= 250;
				if ((optr = pvoie->outptr) != NULL)
				{
					nbcars = optr->nb_car;
					nocars = 0;
					ptr = optr->buffer;
				}
				else
				{
					/*            cprintf(" Vide ") ; */
					break;
				}
			}
			*tptr++ = *ptr;
			++nocars;
			if (++ncars == maxcar)
			{
				optr->no_car = nocars;
				break;
			}
			++ptr;
		}
		*tptr = '\0';
		maj_niv (N_MBL, 0, 0);
		in_buf (voiecur, data, strlen (data));
		while (pvoie->inbuf.nblig)
			traite_voie (voiecur);
	}
}

int appel_pg (char *nom)
{
	pglist *lptr = tete_pg;

#ifdef __LINUX__
	cmdlist *cptr = tete_cmd;

	strupr (nom);

	while (cptr)
	{
		if (strcmp (nom, cptr->cmd) == 0)
		{
			maj_niv (N_MBL, 20, 0);
			--indd;
			exec_cmd (cptr);
			return (1);
		}
		cptr = cptr->next;
	}

#else

	strupr (nom);

#endif

	while (lptr)
	{
		if (strcmp (nom, lptr->nom_pg) == 0)
		{
			maj_niv (N_MBL, 17, 0);
			--indd;
			exec_pg ();
			return (1);
		}
		lptr = lptr->suiv;
	}
	return (0);
}

#ifdef __LINUX__
static void load_cmd (void)
{
	FILE *fptr;
	char line[256];
	char cmd[256];
	char action[256];
	cmdlist *cptr;

	fptr = fopen (c_disque ("commands.sys"), "r");
	if (fptr == NULL)
		return;

	while (fgets (line, sizeof (line), fptr))
	{
		int nb = sscanf (line, "%s %[^\n]", cmd, action);

		if (nb < 2 || *cmd == '#')
			continue;
		cptr = malloc (sizeof (cmdlist));
		if (cptr == NULL)
			return;
		strupr (cmd);
		n_cpy (sizeof (cptr->cmd) - 1, cptr->cmd, cmd);
		cptr->action = strdup (action);
		cptr->next = tete_cmd;
		tete_cmd = cptr;

		++nbpg;
		{
			char text[80];

			sprintf (text, "%d: CMD %s", nbpg, cptr->cmd);
			InitText (text);
		}
	}

	fclose (fptr);
}
#endif

static void load_pg (void)
{
	char str[80];
	char *ptr;
	pglist *lptr = tete_pg;
	struct ffblk dirblk;

#if defined(__WINDOWS__) || defined(__FBBDOS__)
#ifdef __WIN32__
	if (findfirst ("PG32\\*.*", &dirblk, FA_DIREC))
#else
	if (findfirst ("PG\\*.*", &dirblk, FA_DIREC))
#endif
	{
		return;
	}
#endif
#if defined(__LINUX__)
	sprintf(str, "%s*", PGDIR);
	if (findfirst (str, &dirblk, FA_DIREC))
	{
		return;
	}
#endif

	if (*pvoie->dirblk.ff_name == '.')
	{
		findnext (&dirblk);
		if (findnext (&dirblk))
		{
			return;
		}
	}
	ptr = strchr (dirblk.ff_name, '.');
	if (ptr)
		*ptr = '\0';
	if ((dirblk.ff_attrib & FA_DIREC) == 0)
	{
		if (lptr)
		{
			lptr->suiv = (pglist *) m_alloue (sizeof (pglist));
			lptr = lptr->suiv;
		}
		else
		{
			tete_pg = lptr = (pglist *) m_alloue (sizeof (pglist));
		}
		lptr->suiv = NULL;
		strn_cpy (8, lptr->nom_pg, dirblk.ff_name);
#if defined(__WINDOWS__) || defined(__LINUX__)
		++nbpg;
		{
			char text[80];

			sprintf (text, "%d: PG  %s", nbpg, lptr->nom_pg);
			InitText (text);
		}
#endif
	}

	while (findnext (&dirblk) == 0)
	{
		ptr = strchr (dirblk.ff_name, '.');
		if (ptr)
			*ptr = '\0';
		if ((dirblk.ff_attrib & FA_DIREC) == 0)
		{
			if (lptr)
			{
				lptr->suiv = (pglist *) m_alloue (sizeof (pglist));
				lptr = lptr->suiv;
			}
			else
			{
				tete_pg = lptr = (pglist *) m_alloue (sizeof (pglist));
			}
			lptr->suiv = NULL;
			strn_cpy (8, lptr->nom_pg, dirblk.ff_name);
#if defined(__WINDOWS__) || defined(__LINUX__)
			++nbpg;
			{
				char text[80];

				sprintf (text, "%d: PG  %s", nbpg, lptr->nom_pg);
				InitText (text);
			}
#endif
		}
	}
#ifdef __LINUX__
	/* Loads the commands of system/commands.sys */
	load_cmd ();
#endif
}

void end_pg (void)
{
	pglist *lptr;

#ifdef __LINUX__
	cmdlist *cptr;
	int voie;

	for (voie = 0; voie < NBVOIES; voie++)
	{
		/* Close open ttys */
		if (svoie[voie]->niv1 == N_MBL && svoie[voie]->niv2 == 20 && svoie[voie]->niv3 > 0)
		{
			printf ("Closing tty %d\n", svoie[voie]->niv3);
			close (svoie[voie]->niv3);
		}
	}

	while (tete_cmd)
	{
		cptr = tete_cmd;
		tete_cmd = cptr->next;
		m_libere (cptr->action, strlen (cptr->action) + 1);
		m_libere (cptr, sizeof (cmdlist));
	}
#endif

	while (tete_pg)
	{
		lptr = tete_pg;
		tete_pg = tete_pg->suiv;
		m_libere (lptr, sizeof (pglist));
	}

}

void affich_pg (int tp)
{
#ifdef __LINUX__

	load_pg ();

#endif
#ifdef __WINDOWS__

	load_pg ();

#endif
#ifdef __FBBDOS__
	fen *fen_ptr;
	pglist *lptr;
	int size, nb_pg = 0;

	load_pg ();

	lptr = tete_pg;
	while (lptr)
	{
		++nb_pg;
		lptr = lptr->suiv;
	}

	if (nb_pg == 0)
		return;

	size = (nb_pg > 19 * 7) ? 19 : 1 + nb_pg / 7;
	lptr = tete_pg;
	deb_io ();

	nb_pg = 0;
	fen_ptr = open_win (10, 2, 76, 4 + size, INIT, "PG");
	while (lptr)
	{
		cprintf ("%-8s ", lptr->nom_pg);
		lptr = lptr->suiv;
	}
	attend_caractere (tp);
	close_win (fen_ptr);

	fin_io ();
#endif
}
