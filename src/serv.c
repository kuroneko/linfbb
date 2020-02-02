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
 *
 *      SERV.C : Programme principal
 */

#define FBB_MAIN
#define MAIN

#ifdef __MSDOS__
#define PUBLIC
#endif

#include <serv.h>
#include <yapp.h>

char *os (void)
{
#ifdef __FBBDOS__
	static char *my_os = "MsDos";

#endif
#ifdef __WINDOWS__
	static char *my_os = "Windows";

#endif
#ifdef __linux__
	static char *my_os = "Linux";

#endif
	return (my_os);
}

#if defined(__WINDOWS__) || defined(__linux__)

tp_ems t_ems[NB_EMS] =
{
	{"MSG", EMS_MSG},
	{"BID", EMS_BID},
	{"HIE", EMS_HRT},
	{"FWD", EMS_FWD},
	{"REJ", EMS_REJ},
	{"WPG", EMS_WPG},
	{"SCR", 0},
};

static int out_file (int, char *);

unsigned short xendien ( unsigned short xe1 )
{
	unsigned short xe2;

	xe2 = ( xe1 & 0x00FF ) << 8 ;
	xe2 |= ( xe1 & 0xFF00 ) >> 8 ;

	return xe2 ;
}

unsigned long xendienl ( unsigned long xe1 )
{
	unsigned long xe2;

	xe2 = ( xe1 & 0x000000FF ) << 24 ;
	xe2 |= ( xe1 & 0x0000FF00 ) << 8 ;
	xe2 |= ( xe1 & 0x00FF0000 ) >> 8 ;
	xe2 |= ( xe1 & 0xFF000000 ) >> 24 ;

	return xe2 ;
}

void sortie_prg (void)
{
	static int done = 0;

	if (done)
		return;

	done = 1;

	if (*BBS_DW)
	{
#if defined(__WINDOWS__) || defined(__FBBDOS__)
		send_dos (5, BBS_DW, NULL);
#endif
#ifdef __linux__
		char *pptr = BBS_DW;

		call_nbdos (&pptr, 1, NO_REPORT_MODE, NULL, TOOLDIR, NULL);
#endif
	}

	write_heard ();
/*
   #ifdef __WIN32__
   EndThread();
   #endif
 */
	if (operationnel >= 0)
	{
		operationnel = FALSE;
		flush_wp_cache ();
		dde_wp_serv = 0;
		ferme_log ();
#ifdef TRACE
		close_trace ();
#endif
	}

	/* Dealloue les listes chainees */
	end_pg ();					/* Dealloue la chaine des PG */
	end_watchdog ();
	end_messages ();
	end_admin ();
	end_textes ();
	/*  end_ports (); */
	end_arbre ();
	end_bbs ();
	end_parts ();
	end_hold ();
	end_themes ();
	end_swap ();
	end_beacon ();
	end_wp ();
	end_lzhuf ();
	end_fwd ();
	/*  end_voies (); */
	end_modem ();
	end_exms ();
}

#endif /* __WINDOWS__ */

#ifdef  __FBBDOS__
/********* Test overflow **********
long maxrec;
 ********** Fin du test ***********/

#define BREAK

tp_ems t_ems[NB_EMS] =
{
	{"MSG", EMS_MSG},
	{"BID", EMS_BID},
	{"HIE", EMS_HRT},
	{"FWD", EMS_FWD},
	{"REJ", EMS_REJ},
	{"WPG", EMS_WPG},
	{"SCR", EMS_SCR},
};

/* extern unsigned _stklen = 17408; / 16384 ; / 12000 */
extern unsigned _stklen = 32768;	/* 16384 ; / 12000 */

static char nom_programme[80];

static void init_mem (void);
static int out_file (int, char *);

/* Routine appelee avant le _exit -> directive pragma */

void sortie_prg (void)
{
	static int done = 0;

	if (done)
		return;

	done = 1;

	if (*BBS_DW)
		send_dos (5, BBS_DW, NULL);

	write_heard ();

	if (operationnel >= 0)
	{
		operationnel = FALSE;
		flush_wp_cache ();
		dde_wp_serv = 0;
		ferme_log ();
#ifdef TRACE
		close_trace ();
#endif
		libere_xems ();
		remet_vecteurs ();
	}
	cprintf ("Exiting %d ...", type_sortie);
	sleep (2);

	sleep (2);
	if (!EGA)					/* outportb(0x3d9, 0x0) */
		;
	else
		ega_close ();
	close_win (fen_dos);

	/* Dealloue les listes chainees */
	end_pg ();					/* Dealloue la chaine des PG */

	end_watchdog ();
	end_messages ();
	end_admin ();
	end_textes ();
	end_ports ();
	end_arbre ();
	end_bbs ();
	end_parts ();
	end_hold ();
	end_themes ();
	end_swap ();
	end_beacon ();
	end_wp ();
	end_lzhuf ();
	end_fwd ();
	end_voies ();
	end_modem ();
	end_exms ();
	end_dll ();
	/*  remet_dos(); */

	/* Fortify_OutputAllMemory();
	   Fortify_DumpAllMemory(0);

	   Fortify_LeaveScope(); */
}

#ifdef __FBBDOS__
#pragma exit sortie_prg
#endif

main (int ac, char **av)
{
	int voie;

/*
   ind_noeud null_cur;

   daemon_mode = 0;
   strcpy(null_cur.indic, "NULL");
 */
	accept_connection = FALSE;
	Fortify_EnterScope ();
	Fortify_CheckAllMemory ();

	init_semaine ();

	/* #if FBB_DEBUG */
	init_debug (_CS);
	/* #endif */

	operationnel = -1;

	df ("main", 3);
	/*  debut_fonction("main", 3, MK_FP(_SS, _BP)); */

#ifdef BREAK
	break_stop ();
#endif
	strcpy (nom_programme, av[0]);
	vlang = -1;
	init_mem ();
	if ((ac < 2) || (strcmp (av[1], "-t") != 0))
		boot_prg ();
#ifdef __MSDOS__
	fprintf (stderr, "\nMSDOS defined  main() INITIALISATION\n");
	initialisations ();
#endif
#ifndef BREAK
	dprintf ("Break off !\r\n");
#endif
	/* chmod(nom_programme, S_IREAD); */

	/*cprintf("Voie = %d\r\n", sizeof(Svoie)); */
	/*cprintf("Port = %d\r\n", sizeof(defport)); */
	/*cprintf("Fwd  = %d\r\n", sizeof(Forward)); */
	/*sleep_(5); */

	operationnel = 1;
	display_screen ();
	for (voie = 1; voie < NBVOIES; voie++)
		programm_indic (voie);
	accept_connection = TRUE;
	kernel ();
	ff ();
	return (0);
}

static void init_mem (void)
{
#ifndef __DPMI16__
	FILE *fp;
	int i;
	char ligne[256];
	char *ptr = ligne;
	char *scan;

	if ((fp = fopen ("INIT.SRV", "r")) == NULL)
	{
#ifdef ENGLISH
		cprintf ("Cannot open file INIT.SRV        \r\n\a");
#else
		cprintf ("Erreur ouverture fichier INIT.SRV\r\n\a");
#endif
		curon ();
		exit (0);
	}
	i = 0;

	while (fgets (ligne, 250, fp))
	{
		ptr = sup_ln (ligne);
		if (*ligne == '#')
			continue;
		i++;
		if (i == 36)
		{
			int ems = atoi (ptr);

			/* in_exms = 0; */
			if (ems < 0 || ems > 2)
				ems = 0;
			strtok (ptr, " ,\t");
			if (ems == 2)
			{
				while (scan = strtok (NULL, " ,\t"))
				{
					strupr (scan);
					if (strcmp ("OVR", scan) == 0)
					{
						if (_OvrInitExt (0L, 0L) == 0)
						{
							cprintf ("XMS driver initialized\r\n");
							/* sleep(5); */
						}
						else
						{
							cprintf ("XMS driver not found\r\n");
							curon ();
							exit (1);
						}
					}
				}
			}
			break;
		}
	}
	ferme (fp, 57);
#endif
}

void break_stop (void)
{
	bdos (0x33, 0x00, 0x31);	/* Driver transparent */
	setcbrk (0);				/* breaks inhibes */
}

void break_ok (void)
{
	bdos (0x33, 0x00, 0x30);	/* Driver non transparent */
	setcbrk (1);				/* breaks valides */
}

#endif /* __FBBDOS__ */

void sleep_ (unsigned sec)
{
#ifdef __WINDOWS__
	WinSleep (sec);
#endif

#ifdef __FBBDOS__
	long temps;
	long tempo;

	temps = btime ();

	for (;;)
	{
		tempo = btime ();
		if (tempo < temps)
			break;
		if (tempo > (temps + (long) (sec * 18)))
			break;
	}
/*
   tempo = sec * 18 ;
   while (tempo) ;
 */
	/*  attend_caractere(sec); */
#endif
}


void clear_inbuf (int voie)
{
	lbuf *bptr;

	while (svoie[voie]->inbuf.tete)
	{
		bptr = svoie[voie]->inbuf.tete->suite;
		svoie[voie]->memoc -= svoie[voie]->inbuf.tete->lgbuf;
		m_libere ((char *) svoie[voie]->inbuf.tete->buffer, svoie[voie]->inbuf.tete->lgbuf);
		m_libere ((char *) svoie[voie]->inbuf.tete, sizeof (lbuf));
		svoie[voie]->inbuf.tete = bptr;
	}
	svoie[voie]->inbuf.nblig = svoie[voie]->inbuf.nbcar = svoie[voie]->inbuf.nocar = 0;
	svoie[voie]->inbuf.curr = NULL;
	svoie[voie]->inbuf.ptr = NULL;
}


void init_timout (int voie)
{
	time_att[voie] = svoie[voie]->timout;
}


void init_langue (int voie)
{
	int v, cpt, lutil, luse, lang;

	nlang = svoie[voie]->finf.lang;
	if (nlang >= maxlang)
		nlang = 0;

	vlang = -1;
	for (cpt = 0; cpt < NBLANG; cpt++)
	{
		if (langue[cpt]->numlang == nlang)
		{
			vlang = cpt;
			break;
		}
	}

	if (vlang == -1)
	{
		/* swapp de la langue la moins utilisee */
		lutil = MAXVOIES;
		for (lang = 0; lang < NBLANG; lang++)
		{
			if (langue[lang]->numlang == -1)
			{
				vlang = lang;
				break;
			}
			luse = 0;
			for (v = 0; v < NBVOIES; v++)
			{
				if ((voie != v) && (svoie[v]->sta.connect) && (svoie[v]->finf.lang == lang))
					++luse;
			}
			if (luse < lutil)
			{
				lutil = luse;
				vlang = lang;
			}
		}
		swap_langue (vlang, nlang);
	}
	Oui = *(langue[vlang]->plang[OUI - 1]);
	Non = *(langue[vlang]->plang[NON - 1]);
}


int nbl_page (int voie)
{
	int lig;

	if (voie == CONSOLE)
#ifdef __WINDOWS__
		lig = get_win_lig (CONSOLE) - 1;
#elsif __FBBDOS__
		lig = (doub_fen) ? separe - (M_LIG + 2) : h_screen - console - (M_LIG + 1);
#else
		lig = svoie[voie]->finf.nbl - 1;
#endif
	else
		lig = svoie[voie]->finf.nbl - 1;
	return (lig);
}


void prog_more (int voie)
{
	if (!(PAG (svoie[voie]->finf.flags)) || (FOR (svoie[voie]->mode)) ||
		((voie_forward (voie)) && (voie != CONSOLE)) ||
		((voie == CONSOLE) && (print)) || (svoie[voie]->binary) ||
		(POP ( no_port(voie))))
	{
		svoie[voie]->lignes = -1;
	}
	else
	{
		svoie[voie]->lignes = nbl_page (voie);
	}
	svoie[voie]->stop = 0;
}


/* Substitue les variables : */
/*   mode = 1 : $W = \r */
/*   mode = 2 : $W = \n */
/*   mode = 3 : $W = \r\n */
static char *expand_txt (int mode, char *texte)		/* Fin de ligne = Return */
{
	static char sbuffer[600];
	char *ptr;
	char c;
	int var = 0;
	int nb = 0;

	ptr = NULL;
	while (1)
	{
		if (nb > 590)
			break;
		if (var)
		{
			if (*ptr)
			{
				sbuffer[nb] = *ptr++;
				nb++;
			}
			else
				var = 0;
		}
		else
		{
			if ((c = *texte++) == '\0')
				break;
			if (c == '$')
			{
				if ((c = *texte++) == '\0')
					break;
				if (c == 'W')
				{
					/* Fin de ligne */
					switch (mode)
					{
					case 1:
						sbuffer[nb++] = '\r';
						break;
					case 2:
						sbuffer[nb++] = '\n';
						break;
					case 3:
						sbuffer[nb++] = '\r';
						sbuffer[nb++] = '\n';
						break;
					}
				}
				else
				{
					ptr = variable (c);
					var = 1;
				}
			}
			else if (c == '%')
			{
				if ((c = *texte++) == '\0')
					break;
				ptr = alt_variable (c);
				var = 1;
			}
			else
			{
				sbuffer[nb] = c;
				nb++;
			}
		}
	}
	sbuffer[nb] = '\0';
	return (sbuffer);
}

char *expand (char *texte)		/* Fin de ligne = LF */
{
	return (expand_txt (2, texte));
}

char *var_txt (char *texte)		/* Fin de ligne = Return */
{
	return (expand_txt (1, texte));
}

char *var_crlf (char *texte)	/* Fin de ligne = Return/LineFeed */
{
	return (expand_txt (3, texte));
}

int outfich (char *nomfich)
{
	/* Sortie avec variables */
	return (out_file (1, nomfich));
}


int outfichs (char *nomfich)
{
	/* Sortie sans variables */
	return (out_file (0, nomfich));
}


int out_file (int var, char *nomfich)
{
	int nb;
	char sbuffer[300];
	FILE *fptr;

	if ((fptr = fopen (nomfich, "rt")) != NULL)
	{
		while (fgets (sbuffer, 255, fptr))
		{
			nb = strlen (sbuffer);
			if (var)
				out (sbuffer, nb);
			else
				outs (sbuffer, nb);
		}
		ferme (fptr, 48);
		return (TRUE);
	}
	return (FALSE);
}


char *who (char *ptr)
{
	int i = 0, j;
	char s[80];
	char freq[80];

	for (i = 2; i < NBVOIES; i++)
	{
		if (svoie[i]->sta.connect)
		{
			j = 0;
			sprintf (freq, "(%s)", p_port[no_port (i)].freq);
			sprintf (s, "Ch. %-2d %9s : %6s-%-2d - %s\r",
					 virt_canal (i), freq, svoie[i]->sta.indicatif.call,
			  svoie[i]->sta.indicatif.num, strdate (svoie[i]->debut));
			if (ptr)
			{
				strcpy (ptr, s);
				ptr += strlen (s);
			}
			else
				outs (s, strlen (s));
			if (*(svoie[i]->sta.relais[j].call))
			{
				sprintf (s, "   via : %6s-%-2d",
						 svoie[i]->sta.relais[j].call, svoie[i]->sta.relais[j].num);
				if (ptr)
				{
					strcpy (ptr, s);
					ptr += strlen (s);
				}
				else
					outs (s, strlen (s));
				++j;
				while (*(svoie[i]->sta.relais[j].call))
				{
					if (j == 4)
					{
						if (ptr)
						{
							strcpy (ptr, "\r        ");
							ptr += 9;
						}
						else
							outs ("\n        ", 9);
					}
					sprintf (s, " %6s-%-2d", svoie[i]->sta.relais[j].call, svoie[i]->sta.relais[j].num);
					if (ptr)
					{
						strcpy (ptr, s);
						ptr += strlen (s);
					}
					else
						outs (s, strlen (s));
					++j;
				}
				if (ptr)
				{
					*ptr++ = '\r';
					*ptr = '\0';
				}
				else
					outsln (" ", 1);
			}
		}
	}
	return (ptr);
}

void maj_niv (int nivo1, int nivo2, int nivo3)
{
	long caltemps;

	if (pvoie->niv1 != nivo1)
	{
		stemps[pvoie->niv1] += (time (&caltemps) - pvoie->tstat);
		pvoie->tstat = caltemps;
	}
	pvoie->niv1 = nivo1;
	pvoie->niv2 = nivo2;
	pvoie->niv3 = nivo3;
	status (voiecur);
}


int num_voie (char *indic_recherche)
{
	int i;

	for (i = 0; i < NBVOIES; ++i)
		if (svoie[i]->sta.connect && indcmp (svoie[i]->sta.indicatif.call, indic_recherche))
			return (i);
	return (-1);
}


int texte (int no)
{
	int nl = 0;
	char *ptr;

	if (langue == NULL)
		return (nl);

	ptr = langue[vlang]->plang[no - 1];
	/*  cprintf("Texte : <%s>\r\n", ptr) ; */
	if ((no > 0) && (no <= NBTEXT))
		out (ptr, strlen (ptr));
	while (*ptr)
	{
		if ((*ptr == '$') && (*(ptr + 1) == 'W'))
			++nl;
		++ptr;
	}
	return (nl);
}


int incindd (void)
{
	if (*indd)
	{
		do
		{
			++indd;
		}
		while ((*indd) && (*indd != '\n') && (!ISGRAPH (*indd)));
	}
	return (*indd);
}


void selvoie (int voie)
{
	/* static int    cv = -1; */
	/* static Svoie  curr; */


	df ("selvoie", 1);

	/*
	   if (voie != CONSOLE)
	   selcanal(no_port(voie)) ;
	 */

	if (((voie >= 0) && (voie < NBVOIES)) || (voie == MWARNING))
	{
		pvoie = svoie[voie];
		voiecur = voie;
		ptmes = &(pvoie->entmes);
	}
	else
		fbb_error (ERR_CANAL, "Select channel", voie);
	
	ff ();
}

void deconnexion (int voie, int type)
{
	if (voie == CONSOLE)
	{
		/* dprintf("deconnexion console\n"); */

		if (pvoie->cross_connect != -1)
		{
			int save_voie = voiecur;

			pvoie->ch_mon = -1;
			selvoie (pvoie->cross_connect);
			dec (voiecur, 1);
			selvoie (save_voie);
			fin_tnc ();
		}

		svoie[voie]->sta.connect = 0;
		console_off ();
		maj_niv (0, 0, 0);
		aff_nbsta ();
		curseur ();

		if (svoie[voie]->l_yapp)
		{
			svoie[voie]->finf.lastyap = svoie[voie]->l_yapp;
			svoie[voie]->l_yapp = 0L;
		}

		if (svoie[voie]->curfwd)
		{
			svoie[voie]->curfwd->forward = -1;
			svoie[voie]->curfwd->no_bbs = 0;
			svoie[voie]->curfwd = NULL;
		}

		majinfo (voie, 2);
#ifdef __FBBDOS__
		clear_insert ();
#endif
		libere_zones_allouees (voie);	/* Vide les eventuelles listes */
		aff_forward ();

		del_temp (voie);
		del_copy (voie);
		/* printf("Fin deconnexion console\n"); */
	}

	else if (voie == INEXPORT)
	{
		svoie[voie]->sta.connect = 0;
/*
#ifdef __linux__
  		close(p_com[voie].comfd);
 		p_com[voie].comfd = -1; / * libere le port * /
#endif
*/
		aff_event (voie, 2);
		maj_niv (0, 0, 0);
		aff_nbsta ();

		if (svoie[voie]->curfwd)
		{
			svoie[voie]->curfwd->forward = -1;
			svoie[voie]->curfwd->no_bbs = 0;
			svoie[voie]->curfwd = NULL;
		}

		/*      majinfo(voie, 2); */
		/*      clear_insert(); */
		libere_zones_allouees (voie);	/* Vide les eventuelles listes */
		aff_forward ();

		del_temp (voie);
		del_copy (voie);
	}

	else
	{
		dec (voie, type);
	}
}


void en_navant_toute (void)
{
}

/*
 * Type = 0 : toutes les voies actives
 *        1 : pas la console
 *        2 : toutes les voies sauf la console
 */
int actif (int type)
{
	int i, nb = 0;
	int val = (type == 2) ? 0 : 1;

	for (i = 1; i < NBVOIES; i++)
	{
		if (svoie[i]->sta.connect > val)
			nb++;
	}
	if ((type == 0) && (svoie[CONSOLE]->sta.connect))
		nb++;
	return (nb);
}

char *d_disque (char *chaine)
{
	static char s[256];
	char *ptr;

	strcpy (s, DATADIR);
	ptr = s + strlen (s);
	while (*chaine)
	{
		*ptr++ = tolower (*chaine);
		++chaine;
	}
	*ptr = '\0';
#ifdef __linux__
	strcpy(s, back2slash(s));
#endif
	return (sup_ln (s));
}

char *c_disque (char *chaine)
{
	static char s[256];
	char *ptr;

	strcpy (s, CONFDIR);
	ptr = s + strlen (s);
	while (*chaine)
	{
		*ptr++ = tolower (*chaine);
		++chaine;
	}
	*ptr = '\0';
#ifdef __linux__
	strcpy(s, back2slash(s));
#endif
	return (sup_ln (s));
}

void test_disque (void)
{
#if defined(__WINDOWS__) || defined(__FBBDOS__)
	char texte[300];
	unsigned long mfree;

	mfree = free_disk (0);
#ifdef ENGLISH
	cprintf ("Disk space available : %ld    \r\n\n", mfree);
#else
	cprintf ("Espace disque disponible : %ld\r\n\n", mfree);
#endif
	if (mfree < 1000L)
	{
#ifdef ENGLISH
		sprintf (texte, "Warning ! Disk space available on disk %c: = %ld bytes.        \r", getdisk () + 'A', mfree * 1024UL);
#else
		sprintf (texte, "Attention ! Stockage disponible sur le disque %c: = %ld octets.\r", getdisk () + 'A', mfree * 1024UL);
#endif
		cprintf (texte);

		if (w_mask & W_DISK)
		{
#ifdef ENGLISH
			mess_warning (admin, "*** WARNING : DISK ***  ", texte);
#else
			mess_warning (admin, "*** ATTENTION DISQUE ***", texte);
#endif
		}
	}
#endif
}

int find (char *s)
{
	char *t = s;
	int n = 0;
	int dernier = 0, chiffre = 0, lettre = 0;

	/* Pour valider 50MHz */
	if (isdigit (s[0]) && isdigit (s[1]))
		return (FALSE);

	while (*t)
	{
/*** Test rajoute ***/
		if (!isalnum (*t))
			return (FALSE);
		*t = toupper (*t);

		dernier = (isdigit (*t));

		if (isdigit (*t))
			++chiffre;
		else
			++lettre;

		++t;
		++n;
	}
	/* *t = '\0' ; */

	if (strcmp ("SYSOP", s) == 0)
		return (TRUE);

	if (strcmp ("WP", s) == 0)
		return (TRUE);

	if (std_header & 4096)
	{
		/*
		 * L'indicatif doit avoir entre 3 et 6 caracteres .
		 *             doit contenir au moins un chiffre
		 *                  doit contenir au moins une lettre
		 */
		if ((n < 3) || (n > 6) || (chiffre < 1) || (lettre < 1))
			return (FALSE);
	}
	else
	{
		/*
		 * L'indicatif doit avoir entre 3 et 6 caracteres .
		 *             doit contenir 1 ou 2 chiffres
		 *             et finir par une lettre.
		 */

		if ((n < 3) || (n > 6) || (chiffre < 1) || (chiffre > 2) || dernier)
			return (FALSE);
	}
	return (TRUE);
}
