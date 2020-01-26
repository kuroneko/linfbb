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

#include <serv.h>
#include <fbb_conf.h>
#include <config.h>

static int no_init_error = 1;

void err_init (int lig)
{
	no_init_error = 0;
#if defined(__WINDOWS__)
	ShowError ("INIT.SRV", "Error line", lig);
	fbb_quit (0);
#endif
#if defined(__linux__)
	ShowError ("FBB.CONF", "Error line", lig);
	fbb_quit (0);
#endif
#ifdef __FBBDOS__

#ifdef ENGLISH
	cprintf ("Error file INIT.SRV line %d     \r\n\a", lig);
#else
	cprintf ("Erreur fichier INIT.SRV ligne %d\r\n\a", lig);
#endif
	curon ();
	sleep (10);
	exit (0);
#endif
}


static char *test_back_slash (char *chaine, int nolig)
{
	static char temp[256];
	
	strcpy(temp, chaine);
	
#ifdef __linux__
	if (temp[strlen (temp) - 1] != '/')
		strcat(temp, "/");
/*		err_init (nolig); */
#else
	if (temp[strlen (temp) - 1] != '\\')
		err_init (nolig);
#endif
	return (temp);
}

void end_admin (void)
{
	Msysop *sptr;

	while (mess_sysop)
	{
		sptr = mess_sysop;
		mess_sysop = mess_sysop->next;
		m_libere (sptr, sizeof (Msysop));
	}
	libere_serveurs ();
}

#define FIELDS 0x301f	/* Mask of the mandatory fields */
static char *inistr[] = {
	"vers",	/* 0 */
	"call",	/* 1 */
	"ssid",	/* 2 */
	"qral",	/* 3 */
	"city",	/* 4 */
	"conf",	/* 5 */
	"data",	/* 6 */
	"mess",	/* 7 */
	"comp",	/* 8 */
	"fbbd",	/* 9 */
	"yapp",	/* 10 */
	"docs",	/* 11 */
	"name",	/* 12 */
	"syso",	/* 13 */
	"sysm", /* 14 */
	"impo",	/* 15 */
	"logs",	/* 16 */
	"test",	/* 17 */
	"fbbf",	/* 18 */
	"fbbc",	/* 19 */
	"aski",	/* 20 */
	"mask",	/* 21 */
	"secu",	/* 22 */
	"warn",	/* 23 */
	"hous",	/* 24 */
	"time",	/* 25 */
	"maxd",	/* 26 */
	"loca",	/* 27 */
	"beac", /* 28 */
	"scro",	/* 29 */
	"fwdh",	/* 30 */
	"maxb",	/* 31 */
	"life",	/* 32 */
	"wpca",	/* 33 */
	"zipc",	/* 34 */
	"unpr",	/* 35 */
	"upba",	/* 36 */
	"dwba",	/* 37 */
	"pg",	/* 38 */
	"fdir",	/* 39 */
	"sdir",	/* 40 */
	"tdir",	/* 41 */
	"poph",	/* 42 */
	NULL
};

char *value(char *line, int *key)
{
	static char keystr[5];
	char *ptr;
	int i;
	
	while (isspace(*line))
		++line;

	if (*line == '#' || *line == '\0')
	{
		*key = -3;
		return line;
	}
			
	i = 0;	
	ptr = keystr;
	while (isgraph(*line))
	{
		if (i++ < 4)
		{
			*ptr++ = *line;
		}
		++line;
		if (*line == '=')
			break;
	}
	*ptr = '\0';

	*key = -1;
	for (i = 0 ; inistr[i] ; i++)
	{
		if (strcasecmp(keystr, inistr[i]) == 0)
		{
			*key = i;
			break;
		}
	}

	line = strchr(line, '=');
	if (line)
	{
		++line;
		
		while (isspace(*line))
			++line;
	}
	else
	{
		*key = -2;
	}
	
	return line;	
}

int init_admin (void)
{
	static int first = 1;
	int ntemp;
	char ligne[256];
	char stemp[256];
	char *ptr = ligne;
	char *start;
	char *scan;
	Msysop *sptr = NULL;
	int i, j;
	int key;
	int val;
	int ok_init = 0;
	int fond_haut = 0, fond_milieu = 0, fond_bas = 0, fond_menu = 0;

	balbul = 0;
	nb_ovr = 0;
	
	/* Default values */
	BLK_TO = 0;
	h_ok = 1;
	internal_int = 0xff;
	ems_aut = 1;
	in_exms = 0;
	for (j = 0; j < NB_EMS; j++)
		in_exms |= t_ems[j].flag;
	FOND_VOIE = 0;
	DEF = (fond_haut << 4) + 15;
	STA = (fond_milieu << 4) + 15;
	INIT = (fond_menu << 4) + 15;
	SEND = (fond_bas << 4) + 15;
	RECV = (fond_bas << 4) + 15;
	HEADER = (fond_bas << 4) + 15;
	UI = (fond_bas << 4) + 15;
	CONS = (fond_bas << 4) + 15;
	INDIC = (fond_bas << 4) + 15;
	VOIE = (fond_bas << 4) + 15;

	*mypath = '\0';
	myssid = 0;
	*qra_locator = '\0';
	*my_city = '\0';

	/* Ne pas reinitialiser */
	if (first)
	{
		int i;
		for (i = 0; i < 10; i++)
			varx[i] = m_alloue (81);

		for (i = 1; i < NBPORT; i++)
			t_balise[i] = 900;
	}

	if (read_fbb_conf(NULL) > 0)
	{	
			no_init_error = 0;
			ShowError ("fbb.conf", "Cannot open file", 0);
			window_init ();
			fbb_quit (0);
			return (0);
	}

#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Parameters set-up            \n");
#else
	cprintf ("Initialisation des parametres\n");
#endif
#else
#ifdef ENGLISH
	cprintf ("Parameters set-up            \r\n");
#else
	cprintf ("Initialisation des parametres\r\n");
#endif
#endif


	libere_serveurs ();

	for (key = 0 ; inistr[key] ; key++)
	{
		ptr = find_fbb_conf(inistr[key], 0);
		if (ptr == NULL)
			ptr = def_fbb_conf(inistr[key]);
			if (ptr == NULL)
				continue;
			
		fprintf(stderr, "%4s : %s\n", inistr[key], ptr);
		
		switch (key)
		{
		case 0:
			/*	Accept all versions !!
			sprintf (stemp, "FBB%s", VERSION);
			if (strncasecmp (stemp, ptr, strlen (stemp)) != 0)
			{
#ifdef ENGLISH
				cprintf ("*** Error : Waiting for fbb.conf version %s \r\n", stemp);
#else
				cprintf ("*** Erreur : Version de fbb.conf attendue %s\r\n", stemp);
#endif
				err_init (0);
			}
			*/
			ok_init |= (1 << key);
			break;
		case 1:
			strn_cpy (39, mypath, ptr);
			if ((j = strlen (mypath)) && (mypath[j - 1] == '.'))
				mypath[j - 1] = '\0';
			ok_init |= (1 << key);
			break;
		case 2:
			myssid = (char) atoi (ptr);
			ok_init |= (1 << key);
			break;
		case 3:
			strn_cpy (10, qra_locator, ptr);
			ok_init |= (1 << key);
			break;
		case 4:
			n_cpy (19, my_city, ptr);
			ok_init |= (1 << key);
			break;
		case 5:
			n_cpy (80, CONFDIR, test_back_slash (ptr, 0));
			break;
		case 6:
			n_cpy (80, DATADIR, test_back_slash (ptr, 0));
			break;
		case 7:
			n_cpy (80, MESSDIR, test_back_slash (ptr, 0));
			break;
		case 8:
			n_cpy (80, MBINDIR, test_back_slash (ptr, 0));
			break;
		case 9:
			for (j = 0; j < 8; j++)
			{
				*PATH[j] = '\0';
			}
			printf("fbbd : <%s>\n", ptr);
			scan = strtok (ptr, " ,\t");
			for (j = 0; j < 8; j++)
			{
				printf("%d : {%s}\n", j, scan);
				if (scan == NULL)
					break;

				if (*scan != '*')
				{
					n_cpy (80, PATH[j] + 2, slash2back (test_back_slash (scan, 0)));
					PATH[j][0] = getdisk () + 'A';
					PATH[j][1] = ':';
				}
				scan = strtok (NULL, " ,\t");
				
				printf("%d : {%s}\n", j, PATH[j]);
			}
			break;
		case 10:
			n_cpy (80, YAPPDIR+2, slash2back (test_back_slash (ptr, 0)));
			YAPPDIR[0] = getdisk () + 'A';
			YAPPDIR[1] = ':';
			break;
		case 11:
			n_cpy (80, DOCSDIR, test_back_slash (ptr, 0));
			ptr = DOCSDIR + strlen (DOCSDIR) - 1;
			if (*ptr == '\\')
				*ptr = '\0';
			break;
		case 12:
			n_cpy (12, my_name, ptr);
			ok_init |= (1 << key);
			break;
		case 13:
			strn_cpy (6, admin, ptr);
			ok_init |= (1 << key);
			break;
		case 14:
			scan = strtok (ptr, " ,\t");
			/* Delete la liste existante eventuellement */
			while (mess_sysop)
			{
				sptr = mess_sysop;
				mess_sysop = mess_sysop->next;
				m_libere (sptr, sizeof (Msysop));
			}

			if ((scan) && (isalpha (*scan)))
			{
				sptr = mess_sysop = (Msysop *) m_alloue (sizeof (Msysop));
				strn_cpy (20, sptr->call, scan);
			}
			while ((scan = strtok (NULL, " ,\t")) != NULL)
			{
				sptr->next = (Msysop *) m_alloue (sizeof (Msysop));
				sptr = sptr->next;
				strn_cpy (20, sptr->call, scan);
			}
			break;
		case 15:
			n_cpy (80, MAILIN, ptr);
			n_cpy (76, LOCK_IN, ptr);
			start = strrchr (LOCK_IN, '/');
			if (start == NULL)
				start = LOCK_IN;
			scan = strrchr (start, '.');
			if (scan)
				*scan = '\0';
			strcat(LOCK_IN, ".lck");
			break;
		case 16:
			comlog = (toupper (*ptr) == 'O');
			break;
		case 17:
			DEBUG = (toupper (*ptr) == 'O');
			miniserv = 0xffff;
			watchport = 0;
			sscanf (ptr, "%*s %d", &watchport);
			break;
		case 18:
			fbb_fwd = (toupper (*ptr) == 'O');
			/*
			   1  : Espace obligatoire avant le @ dans l'adresse
			   2  : Ne teste pas la limite a 6 des champs de l'adresse
			   4  : Ne tronque pas le header a l'espace < 79 caracteres
			   8  : Header MBL/RLI
			   16 : Si pas de champ BBS envoie l'indicatif de ma BBS (->PMS)
			   32 : Supprime les messages data au SYSOP
			   64 : N'utilise pas le BID cree a partir des headers si pas de BID
			   128: N'accepte le forward que des BBS declarees.
			   256: Les messages WP ne sont pas HOLD
			   512: XWFD has priority on FBB
			   1024: Alternate BID generation
			   2048: XFWD with checksum
			   4096: Simple check for callsigns (3 to 6 chars, 1 digit, 1 alpha).
			 */
			std_header = 0;
			sscanf (ptr, "%*s %d", &std_header);
			break;
		case 19:
			/*
			   1  : Protocole de niveau 1
			 */
			bin_fwd = (toupper (*ptr) == 'O');
			/*
			   1  : Binaire FBB version 1
			   2  : Binaire RLI
			 */
			ntemp = -1;
			sscanf (ptr, "%*s %d", &ntemp);
			if ((bin_fwd) & (ntemp != -1) && (ntemp & 1))
			{
				bin_fwd = 2;
			}
			prot_fwd = FWD_MBL;
			if (fbb_fwd)
			{
				prot_fwd |= FWD_FBB;
				if (bin_fwd)
				{
					prot_fwd |= FWD_BIN;
					if (bin_fwd == 2)
						prot_fwd |= FWD_BIN1;
				}
			}
			if ((ntemp == -1) || (ntemp & 2))
				prot_fwd |= FWD_XPRO;

			break;
		case 20:
			info_ok = (toupper (*ptr) == 'O');
			break;
		case 21:
			def_mask = (unsigned) atoi (ptr);
			break;
		case 22:
			if (sscanf (ptr, "%u %u %u", &d_droits, &ds_droits, &dss_droits) != 3)
				err_init (0);
			break;
		case 23:
			w_mask = (unsigned) atoi (ptr);
			break;
		case 24:
			h_maint = atoi (ptr);
			break;
		case 25:
			if (sscanf (ptr, "%d %d", &time_n, &time_b) != 2)
				err_init (0);
			time_n *= 60;
			time_b *= 60;
			break;
		case 26:
			if (sscanf (ptr, "%d %d", &max_yapp, &max_mod) != 2)
				err_init (0);
			break;
		case 27:			/* tzone = 3600 * -atol(ptr) ; */
			if (getenv ("TZ") == NULL)
			{
				/*
				   memset(_tzname[1], 0, 4);
				   strcpy(_tzname[0],"GMT");
				   _timezone = 3600L * -atol(ptr) ;
				   _daylight = 0;       stime
				 */
				/* Ne pas liberer... Sinon ca plante a l'appel suivant. */
				char *tzl = malloc (20);

				sprintf (tzl, "TZ=GMT%d", -atoi (ptr));
				j = putenv (tzl);
			}
			tzset ();
			break;
		case 28:
			if (toupper (*ptr) == 'B')
			{
				balbul = 1;
				do
				{
					++ptr;
				}
				while (!ISGRAPH (*ptr));
			}
			max_indic = atoi (ptr);
			break;
		case 29:
			winbuf.totlig = conbuf.totlig = monbuf.totlig = 1500;
			sscanf (ptr, "%d %d %d",
					&winbuf.totlig, &conbuf.totlig, &monbuf.totlig);
			break;
		case 30:
			txtfwd[0] = '\0';
			n_cpy (51, txtfwd + 1, ptr);
			if (txtfwd[1])
				txtfwd[0] = ' ';
			break;
		case 31:
			maxbbid = atoi (ptr);
			if (maxbbid < 0)
				maxbbid = 2000;
			break;
		case 32:
			multi_prive = 0;
			sscanf (ptr, "%ld %d", &nb_jour_val, &multi_prive);
			if (nb_jour_val < 1L)
				nb_jour_val = 1L;
			break;
		case 33:
			strn_cpy (256, wp_line, ptr);
			break;
		case 34:
			n_cpy (8, my_zip, ptr);
			break;
		case 35:
			nb_unproto = 300L;
			val = 6;
			mute_unproto = ack_unproto = via_unproto = priv_unproto = 0;
			sscanf (ptr, "%ld %d %s", &nb_unproto, &val, stemp);

			scan = strupr (stemp);
			while (*scan)
			{
				switch (*scan)
				{
				case 'V':
					via_unproto = 1;
					break;

				case 'P':
					priv_unproto = 1;
					break;

				case 'A':
					ack_unproto = 1;
					break;

				case 'M':
					mute_unproto = 1;
					break;
				}
				++scan;
			}

			def_time_bcl = (val * 18);
			break;
		case 36:
			n_cpy (79, BBS_UP, ptr);
			break;
		case 37:
			n_cpy (79, BBS_DW, ptr);
			break;
		case 38:
			n_cpy (80, PGDIR, test_back_slash (ptr, 0));
			break;
		case 39:
			n_cpy (80, FILTDIR, test_back_slash (ptr, 0));
			break;
		case 40:
			n_cpy (80, SERVDIR, test_back_slash (ptr, 0));
			break;
		case 41:
			n_cpy (80, TOOLDIR, test_back_slash (ptr, 0));
			break;
		case 42:
			n_cpy (40, pop_host, ptr);
			break;
		}
	}

	/* Read services list */
	ptr = find_fbb_conf("serv", 0);
	while (ptr)
	{
		init_serveur (ptr, 0);
		ptr = find_fbb_conf("serv", 1);
	}

	free_fbb_conf();

	init_serveur ("WP     * Request White pages info", 0);
	init_serveur ("REQCFG * Request configuration   ", 0);
	init_serveur ("REDIST * Bulletin redistribution ", 0);

	i = 0;

	ptr = mypath;
	while (isalnum (*ptr))
	{
		mycall[i] = *ptr++;
		if (++i == 6)
			break;
	}
	mycall[i] = '\0';

	ind_console (1, mycall);
	strcpy (my_call, cons_call.call);

	if (first)
		window_init ();
	set_win_colors ();

	first = 0;

	if (ok_init != FIELDS)
	{
		int mask = 1;
		
		for (i = 0 ; i < 32 ; i++)
		{
			if (((mask << i) & FIELDS) && (((mask << i) & ok_init) == 0))
			{
#ifdef ENGLISH
				cprintf ("*** Error : Mandatory field \"%s\" missing in fbb.conf     \r\n", inistr[i]);
#else
				cprintf ("*** Erreur : Le champ \"%s\" n'est pas defini dans fbb.conf\r\n", inistr[i]);
#endif
			}
		}
		err_init (0);
		no_init_error = 0;
	}

	return (no_init_error);
}

static void cree_etat (void)
{
	FILE *fichier;

	if ((fichier = fopen (d_disque ("ETAT.SYS"), "wt")) == NULL)
	{
		fbb_error (ERR_CREATE, d_disque ("ETAT.SYS"), 0);
	}
	fprintf (fichier, "%-6s-%d\n", mycall, myssid);
	ind_console (1, mycall);
	fprintf (fichier, "Mise en service par %s-%d le %s\n",
			 cons_call.call, cons_call.num, strdate (time (NULL)));
	fclose (fichier);
}


void init_etat (void)
{
	FILE *fichier;
	char s[81];

	if ((fichier = fopen (d_disque ("ETAT.SYS"), "r+t")) == NULL)
	{
		if (err_ouvert ("ETAT.SYS"))
		{
			cree_etat ();
			arret = FALSE;
		}
		else
			fbb_error (ERR_OPEN, d_disque ("ETAT.SYS"), 0);
	}
	else
	{
		fgets (s, 80, fichier);
		ind_console (1, sup_ln (s));
		if (fgetc (fichier) == 'A')
			arret = TRUE;
		else
			arret = FALSE;
		fclose (fichier);
	}
}
