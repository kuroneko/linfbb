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
 * INIT.C
 *
 */

#include "serv.h"

static void aff_heure (void);
static void cree_tpstat (void);
static int init_dir (void);
static void init_tpstat (void);
static void initarbre (void);
static void initmessage (void);
static void start_bbs (void);

#ifdef __MSDOS__
static void init_bufscreen (Screen *);
static void val_screen (void);

#endif

extern char _video[];

#ifdef __MSDOS__
void initialisations (void)
{
	int bbs_init = 1;
	fprintf (stderr, "\nMSDOS defined  init.c INITIALISATIONS\n");

	while (!step_initialisations (bbs_init))
		++bbs_init;
}
#endif

int step_initialisations (int niveau)
{
	int i;
#ifdef __WINDOWS__
	unsigned long dMem;
#endif

	InitText ("");

/*	fprintf (stderr, "FBB step_initialisations niveau:%d\n",niveau);*/
		
	switch (niveau)
	{

	case 1:
		{
			/* Check endian */
			long val = 1;
			moto = (*((char *)(&val)) == '\0');
		}
		debug_ptr = NULL;
		operationnel = -1;

		h_ok = 1;
		MAXTACHE = MAXTASK;

		init_semaine ();

#ifdef __WINDOWS__
		/* Allocation du buffer partage ... */
		dMem = GlobalDosAlloc (400ul);
		if (dMem == 0ul)
		{
			cprintf ("Can't allocate shared buffer\r\n");
			exit (1);
		}
		BufSeg = HIWORD (dMem);
		BufSel = LOWORD (dMem);
		BufReel = (char far *) ((unsigned long) BufSel << 16);

#endif

		sed = 1;

		EditorOff = TRUE;
		editor_request = 0;
		errno = 0;
		fast_fwd = 1;
		lastaff = -1;
		reply = 0;
		snd_io = 0;
		test_message = 2;
		h_screen = 25;
		bid_ptr = NULL;
		log_ptr = NULL;
		throute = NULL;
		tbroute = NULL;
		p_port = NULL;
		def_cur.nbmess = def_cur.nbnew = 0;
		def_cur.coord = 0xffff;
		memcpy (def_cur.indic, "NULL", 5);
		test_fichiers = 0;
#ifdef __MSDOS__
		ton_bip = 0;
		kam_timer = 0;
#endif
		vlang = -1;
		time_bcl = 0;
		blank = 0;
		nb_hold = 0;
		ok_aff = 1;
		aut_ui = 1;
		doubl = 0;
		v_aff = -1;
		console = 0;
		editor = 0;
		cmd_fct = 0;
		print = 0;
		def_mask = 0;
		t_tell = -1;
		v_tell = 0;
		ch_fen = 0;
		inexport = 0;
		nb_error = 0;
		temp_sec = SECONDE;
		d_blanc = 0;
		include_size = 0;
		p_forward = 0;
		time_include = NULL;
		mem_alloue = 0L;
		t_bbs = 0L;
		t_appel = 0L;
		hour_time = -1;
		t_iliste.suiv = NULL;
		tete_fwd = NULL;
		tete_serv = NULL;
		tete_pg = NULL;
		bbs_ptr = NULL;
		p_port = NULL;
		*t_iliste.indic = '\0';
		p_iliste = &t_iliste;
		NBVOIES = 1;
		for (i = 0; i < NBLIG; i++)
			af_voie[i] = -1;
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("Reading fbb.conf file\n");
#else
		cprintf ("Lecture du fichier fbb.conf\n");
#endif
#else
#ifdef ENGLISH
		cprintf ("Reading INIT.SRV\r\n");
#else
		cprintf ("Lecture de INIT.SRV\r\n");
#endif
#endif
		init_voie (CONSOLE);	/* Init console */
		voiecur = CONSOLE;
		pvoie = svoie[voiecur];
		if (!init_admin ())
			return (0);
		if (!init_dir ())
			return (0);
		port_log (0, 0, 'S', "I *** BBS Initialize");
		deb_io ();
		operationnel = 0;
		window_connect (ALLCHAN);
		window_connect (CONSOLE);
		window_connect (MMONITOR);
		free_mem ();
		return 0;

	case 2:
		init_exms ();
		initexte ();
		aff_date ();
		free_mem ();
		ferme(ouvre_sat(), 0);
		return 0;

	case 3:
		init_tpstat ();
		free_mem ();

#ifdef __FBBDOS__
#ifdef ENGLISH
		cprintf ("RS232 ports set-up            \r\n");
#else
		cprintf ("Initialisation des ports RS232\r\n");
#endif
#endif
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("RS232 ports set-up            \n");
#else
		cprintf ("Initialisation des ports RS232\n");
#endif
#endif
		init_watchdog (watchport);
		initport ();
		return 0;

	case 4:
		initcom ();
#ifdef __FBBDOS__
#ifdef ENGLISH
		cprintf ("TNC set-up            \r\n");
#else
		cprintf ("Initialisation des TNC\r\n");
#endif
#endif
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("TNC set-up            \n");
#else
		cprintf ("Initialisation des TNC\n");
#endif
#endif
		inittnc ();
		return 0;

	case 5:
		affich_serveurs (2);
		affich_pg (2);
		t_label();
		free_mem ();
		return 0;

	case 6:
#ifdef __FBBDOS__
#ifdef ENGLISH
		cprintf ("BID set-up            \r\n");
#else
		cprintf ("Initialisation des BID\r\n");
#endif
#endif
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("BID set-up            \n");
#else
		cprintf ("Initialisation des BID\n");
#endif
#endif
		cree_bid ();
		free_mem ();
		return 0;

	case 7:
		initarbre ();
		free_mem ();
		return 0;

	case 8:
		/*  epure_messages() ; */
		load_dirmes ();
		initmessage ();
		return 0;

	case 9:
		if (!init_white_pages ())
			cree_routes ();
		else
			h_ok = 0;
		free_mem ();
#ifdef __FBBDOS__
#ifdef ENGLISH
		cprintf ("Files set-up complete           \r\n");
#else
		cprintf ("Fin des initialisations Fichiers\r\n");
#endif
#endif
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("Files set-up complete           \n");
#else
		cprintf ("Fin des initialisations Fichiers\n");
#endif
#endif
		return 0;

	case 10:
		fwd_file = NULL;
		fwd_size = 0;

#ifdef __FBBDOS__
#ifdef ENGLISH
		cprintf ("FORWARD set-up           \r\n");
#else
		cprintf ("Initialisation du forward\r\n");
#endif
#endif
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("FORWARD set-up           \n");
#else
		cprintf ("Initialisation du forward\n");
#endif
#endif
		init_buf_fwd ();
		init_buf_swap ();
		init_buf_rej ();
		return 0;

	case 11:
		init_bbs ();
		init_part ();
		free_mem ();
		read_heard ();
		init_etat ();
		tst_appel ();
		return 0;

	case 12:
		init_hold ();
		init_pfh();
		load_themes ();
#ifdef __FBBDOS__
#ifdef ENGLISH
		cprintf ("Set-up complete        \r\n");
#else
		cprintf ("Fin des initialisations\r\n");
#endif
#endif
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("Set-up complete        \n");
#else
		cprintf ("Fin des initialisations\n");
#endif
#endif
		vlang = 0;
		com_error = old_com_error = 0;
		aff_ind_console ();
		free_mem ();
		aff_date ();
		aff_heure ();
		fin_io ();
		canaff = v_aff;
		winlig = h_screen - 1 - M_LIG;
#ifdef TRACE
		open_trace ();
#endif
		if (*BBS_UP)
		{
			char *pptr = BBS_UP;
#ifdef __linux__
			call_nbdos (&pptr, 1, NO_REPORT_MODE, NULL, TOOLDIR, NULL);
#else
			call_nbdos (&pptr, 1, NO_REPORT_MODE, NULL, NULL, NULL);
#endif
		}
		start_bbs ();
		return 1;
	}

	return 1;
}

static void start_bbs (void)
{
	int port = 1;
	char buffer[300];
	int nb;


	df ("io", 0);

	deb_io ();
	voiecur = 1;
	type_sortie = 1;
	save_fic = 0;
	aff_date ();

	operationnel = 1;
	env_date ();
	for (port = 1; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			switch (p_port[port].typort)
			{
			case TYP_DED:
				sprintf (buffer, "H 18");
				tnc_commande (port, buffer, PORTCMD);
				sprintf (buffer, "H 1");
				tnc_commande (port, buffer, PORTCMD);
				sprintf (buffer, "H 2");
				tnc_commande (port, buffer, PORTCMD);
				/* Ajouter la validation des ports */
				break;
			case TYP_PK:
				break;
			}
		}
	}
	for (nb = 1; nb < NBVOIES; nb++)
	{
		programm_indic (nb);
		set_bs(nb, TRUE);
	}

	test_disque ();
 	sprintf (buffer, "A *** BBS Online (%s)", version());
 	port_log (0, 0, 'S', buffer);
/*	port_log (0, 0, 'S', "A *** BBS Online");*/
	fin_io ();

	start_tasks ();
	mem_alloue = 0L;
	free_mem ();
	dde_wp_serv = 1;


	/* Amorce les TNCs en mode DED */
	for (nb = 0; nb < NBPORT; nb++)
	{
		int i;

		/* positionnel le mult_sel au 1er multi dans chaque COM */

		for (i = 0; i < 8; i++)
		{
			if (p_com[nb].multi[i])
			{
				int port = p_com[nb].multi[i];

				p_com[nb].mult_sel = port;
				break;
			}
		}
	}
	ff ();
}

static int dir_found (char *dir)
{
	int nb;
	char path[256];
	char str[256];

	strcpy (path, dir);
	nb = strlen (path);
	if ((nb > 1) && (path[1] == ':'))
	{
		if ((nb > 3) && (path[nb - 1] == '\\'))
			path[nb - 1] = '\0';
	}
	else
	{
		if ((nb > 1) && (path[nb - 1] == '\\'))
			path[nb - 1] = '\0';
	}

	if (!is_dir (path))
	{
		sprintf (str, "Directory %s not found", back2slash (path));
		ShowError ("FBB TREE", str, 0);
		fbb_quit (0);
		WinSleep (10);
		return (0);
	}
	return (1);
}

static int init_dir (void)
{
  /* Cree l'arbre des repertoires */
	int i;
	char dir[128];

	/* Arborescence MAIL */
	if (!dir_found (MESSDIR))
		return (0);
	for (i = 0; i < 10; i++)
	{
		sprintf (dir, "%smail%d", MESSDIR, i);
		if (!dir_found (dir))
			return (0);
	}

	/* Arborescence BINMAIL */
	if (!dir_found (MBINDIR))
		return (0);
	for (i = 0; i < 10; i++)
	{
		sprintf (dir, "%smail%d", MBINDIR, i);
		if (!dir_found (dir))
			return (0);
	}

	/* Repertoire WP */
	sprintf (dir, "%swp", DATADIR);
	if (!dir_found (dir))
		return (0);

	/* Repertoire DOCS */
	if (!dir_found (DOCSDIR))
		return (0);

	/* Repertoires DOS */
	for (i = 1; i < 8; i++)
	{
		if (*PATH[i])
			if (!dir_found (PATH[i]))
				return (0);
	}

	/* Repertoire YAPP */
	if (!dir_found (YAPPDIR))
		return (0);

	/* Repertoire PG */
	/* if (!dir_found (PGDIR))
		return (0); */

	return (1);
}

void init_semaine (void)
{
	long temps = time (NULL);
	struct tm *sdate = localtime (&temps);

	int ny = sdate->tm_yday;	/* Numero du jour dans l'annee */
	int nw = sdate->tm_wday;	/* Numero du jour dans la semaine */
/*
	if (nw == 0)
		nw = 6;
	else
		--nw;			*/		/* 0 = dimanche -> 0 = lundi */

	if (ny < nw)				/* Premiere semaine de l'annee ? */
	{
		temps -= (3600L * 24L * (ny + 1));
		sdate = localtime (&temps);
		ny = sdate->tm_yday;	/* Numero du jour de l'annee precedente */
		nw = sdate->tm_wday;	/* Numero du jour de la semaine avant */

/*		if (nw == 0)
			nw = 6;
		else
			--nw;		*/		/* 0 = dimanche -> 0 = lundi */
	}
	num_semaine = (7 - nw + ny) / 7;
}

void start_tasks (void)
{
	trait_time = 0;
#ifdef ENGLISH
	cprintf ("Starting multitasking ...");
#else
	cprintf ("D‚marre le multitƒches...");
#endif

#ifdef __MSDOS__
	init_keyboard ();
#endif
#ifdef __linux__
	cprintf (" ok\n");
#else
	cprintf (" ok\r\n");
#endif

}

void end_voies (void)
{
	int i;

	/* Inclut la voie Warnings (NBVOIES+1) */
	for (i = 0; i < NBVOIES + 1; i++)
	{
		if (svoie[i])
			m_libere (svoie[i], sizeof (Svoie));
	}
}

void init_voie (int voie)
{
	int i;
	Svoie *vptr;

	svoie[voie] = vptr = (Svoie *) m_alloue (sizeof (Svoie));
	time_yapp[voie] = -1;
	vptr->timout = time_n;
	init_timout (voie);
	vptr->msg_held = 0;
	vptr->rzsz_pid = -1;
	vptr->temp1 = vptr->mess_recu = 1;
	vptr->fbb = bin_fwd;
	/* vptr->wp = */ vptr->clock = '\0';
	vptr->ret = 0;	
	vptr->sid = vptr->aut_nc = vptr->ind_mess = vptr->warning = 0;
	vptr->ask = vptr->seq = vptr->dde_int = vptr->stop = 0;
	vptr->mbl = vptr->sta.stat = vptr->sta.connect = vptr->deconnect = 0;
	vptr->sta.mem = 32000;
	vptr->sr_mem = vptr->memoc = vptr->nb_err = 0;
	vptr->niv1 = vptr->niv2 = vptr->niv3 = 0;
	vptr->maj_ok = vptr->conf = vptr->dde_marche = 0;
	vptr->groupe = vptr->kiss = vptr->ch_mon = vptr->cross_connect = -1;
	vptr->cur_bull = -1L;
	vptr->localmode = vptr->binary = vptr->sta.ret = vptr->sta.ack = 0;
	vptr->type_yapp = 0;
	vptr->maxbuf = 8;
	vptr->emis = NULL;
	vptr->ctnc = NULL;
	vptr->outptr = NULL;
	vptr->msgtete = NULL;
	vptr->l_mess = vptr->l_yapp = vptr->entmes.numero = 0L;
	vptr->inbuf.nblig = vptr->inbuf.nbcar = vptr->inbuf.nocar = 0;
	vptr->inbuf.tete = NULL;
	vptr->inbuf.curr = NULL;
	vptr->inbuf.ptr = NULL;
	vptr->curfwd = NULL;
	vptr->t_read = NULL;
	vptr->t_list = NULL;
	vptr->llabel = NULL;
	vptr->r_tete = NULL;
	vptr->ncur = &def_cur;
	vptr->mbl_ext = 1;
	vptr->nb_egal = 0;
	vptr->nb_prompt = 0;
	vptr->prot_fwd = prot_fwd;
	vptr->tete_edit.liste = NULL;
	vptr->entmes.bid[0] = '\0';
	vptr->sta.callsign.call[0] = '\0';
	vptr->sta.callsign.num = 0;
	vptr->sta.indicatif.call[0] = '\0';
	vptr->sta.indicatif.num = 0;
	for (i = 0; i < 8; i++)
	{
		vptr->sta.relais[i].call[0] = '\0';
		vptr->sta.relais[i].num = 0;
	}
	init_fb_mess (voie);
}


static void aff_heure (void)
{
	struct tm *sdate;
	long temps = time (NULL);

	sdate = gmtime (&temps);
	cprintf ("GMT %02d:%02d", sdate->tm_hour, sdate->tm_min);
	sdate = localtime (&temps);
#ifdef __linux__
	cprintf (" - LOCAL %02d:%02d\n", sdate->tm_hour, sdate->tm_min);
#else	
	cprintf (" - LOCAL %02d:%02d\r\n", sdate->tm_hour, sdate->tm_min);
#endif
}

void lit_appel (void)
{
	FILE *fptr;

	sed = 1;
	fptr = fopen (d_disque ("OPTIONS.SYS"), "rb");
	if (fptr)
	{
		fread (&bip, sizeof (short), 1, fptr);
		fread (&ok_tell, sizeof (short), 1, fptr);
		fread (&ok_aff, sizeof (short), 1, fptr);
		fread (&separe, sizeof (short), 1, fptr);
		fread (&doub_fen, sizeof (short), 1, fptr);
		fread (&gate, sizeof (short), 1, fptr);
		fread (&just, sizeof (short), 1, fptr);
		fread (&p_forward, sizeof (short), 1, fptr);
		fread (&sed, sizeof (short), 1, fptr);
		fread (&aff_inexport, sizeof (short), 1, fptr);
		fread (&aff_popsmtp, sizeof (short), 1, fptr);

		fclose (fptr);
	}

	if (separe < M_LIG + 5)
		separe = M_LIG + 5;
	if (separe > h_screen - 4)
		separe = h_screen - 4;
	/*  printf("%d %d %d %d %d\n", bip, ok_tell, ok_aff, separe, doub_fen) ; */
#if defined(__WINDOWS__) || defined(__linux__)
	maj_menu_options ();
#endif
}

void cree_dir (int erreur)
{
	bullist lbul;
	FILE *fptr;

	if ((fptr = fopen (d_disque ("DIRMES.SYS"), "wb")) == NULL)
	{
		fbb_error (ERR_CREATE, d_disque ("DIRMES.SYS"), erreur);
	}
	lbul.type = '\0';
	lbul.numero = 100L;
	fwrite (&lbul, sizeof (bullist), 1, fptr);
	fclose (fptr);
}


void cree_info (void)
{
	FILE *fptr;

	if ((fptr = fopen (d_disque ("INF.SYS"), "wb")) == NULL)
	{
		fbb_error (ERR_CREATE, d_disque ("INF.SYS"), 0);
	}
	fclose (fptr);
}


void cree_stat (void)
{
	FILE *fptr;

	if ((fptr = fopen (d_disque ("statis.dat"), "wb")) == NULL)
	{
		fbb_error (ERR_CREATE, d_disque ("statis.dat"), 0);
	}
	fclose (fptr);
}


void cree_sat (void)
{
	FILE *fptr;

	if ((fptr = fopen (d_disque ("SAT\\SATEL.DAT"), "wb")) == NULL)
	{
		fbb_error (ERR_CREATE, d_disque ("SAT\\SATEL.DAT"), 0);
	}
	fclose (fptr);
}


static void cree_tpstat (void)
{
	int i;
	FILE *fichier;

	if ((fichier = fopen (d_disque ("TPSTAT.SYS"), "wb")) == NULL)
	{
		fbb_error (ERR_CREATE, d_disque ("TPSTAT.SYS"), 0);
	}
	for (i = 0; i < NBRUB; i++)
		stemps[i] = 0L;
	fwrite ((char *) stemps, sizeof (*stemps) * NBRUB, 1, fichier);
	fclose (fichier);
}


int err_ouvert (char *nomfic)
{
	int c;
	char s[80];

	deb_io ();
#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Cannot open %s     \n", nomfic);
#else
	cprintf ("Erreur ouverture %s\n", nomfic);
#endif
#else
#ifdef ENGLISH
	cprintf ("Cannot open %s     \r\n", nomfic);
#else
	cprintf ("Erreur ouverture %s\r\n", nomfic);
#endif
#endif
	c = 0;

#ifdef ENGLISH
	sprintf (s, "Creating file %s      ", nomfic);
#else
	sprintf (s, "Creation du fichier %s", nomfic);
#endif
	if (sel_option (s, &c))
	{
#ifdef __linux__
#ifdef ENGLISH
		cprintf ("\rCreating file %s      \n", nomfic);
#else
		cprintf ("\rCreation du fichier %s\n", nomfic);
#endif
#else
#ifdef ENGLISH
		cprintf ("\rCreating file %s      \r\n", nomfic);
#else
		cprintf ("\rCreation du fichier %s\r\n", nomfic);
#endif
#endif
	}
	fin_io ();
	return (c);
}

static void init_tpstat (void)
{
	FILE *fichier;

	fichier = ouvre_stats ();
	fclose (fichier);

	while (TRUE)
	{
		if ((fichier = fopen (d_disque ("TPSTAT.SYS"), "r+b")) == NULL)
		{
			if (err_ouvert ("TPSTAT.SYS"))
				cree_tpstat ();
			else
				fbb_error (ERR_CREATE, d_disque ("TPSTAT.SYS"), 0);
		}
		else
			break;
	}
	fread ((char *) stemps, sizeof (*stemps) * NBRUB, 1, fichier);
	fclose (fichier);
}

static void initarbre (void)
{
	int offset;
	int i = 0;
	info buf2;
	char buf[40];
	FILE *fptr;
	bloc_indic *bptr;

#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Callsign set-up                  \n");
#else
	cprintf ("Initialisation de la nomenclature\n");
#endif
#else
#ifdef ENGLISH
	cprintf ("Callsign set-up                  \r\n");
#else
	cprintf ("Initialisation de la nomenclature\r\n");
#endif
#endif
	deb_io ();
#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Callsigns set-up             \n");
#else
	cprintf ("Initialisation des indicatifs\n");
#endif
#else
#ifdef ENGLISH
	cprintf ("Callsigns set-up             \r\n");
#else
	cprintf ("Initialisation des indicatifs\r\n");
#endif
#endif
	rinfo = 0;

	racine = bptr = new_bloc_info ();
	offset = 0;

	strcpy (bptr->st_ind[0].indic, "TOUS");
	bptr->st_ind[0].nbmess = (short) 0;
	bptr->st_ind[0].nbnew = (short) 0;
	bptr->st_ind[0].coord = 0xffff;
	++offset;

	fptr = ouvre_nomenc ();
	*buf2.indic.call = '\0';

	while (fread ((char *) &buf2, (int) sizeof (buf2), 1, fptr))
	{
		if (find (buf2.indic.call))
		{
			if (offset == T_BLOC_INFO)
			{
				bptr->suiv = new_bloc_info ();
				bptr = bptr->suiv;
				offset = 0;
			}
#ifdef __linux__
			if ((i++ % 50) == 0)
			{
				InitText (itoa (i, buf, 10));
			}
#endif
#ifdef __WINDOWS__
			if ((i++ % 50) == 0)
			{
				InitText (itoa (i, buf, 10));
			}
#endif
#ifdef __MSDOS__
			if (tempo == 0)
			{
				cprintf ("%-6s\r\n", buf2.indic.call);
				tempo = CADENCE;
			}
#endif
			inscoord (rinfo, &buf2, &(bptr->st_ind[offset]));
			++offset;
		}
		++rinfo;
	}
#ifdef __WINDOWS__
	InitText (itoa (i, buf, 10));
#else
#ifdef __linux__
	cprintf ("%-6s\n", buf2.indic.call);
#else
	cprintf ("%-6s\r\n", buf2.indic.call);
#endif
#endif
	ferme (fptr, 4);
	fin_io ();
}

void fwd_cpy (recfwd * dest, bullist * orig)
{
	int i;

	dest->type = orig->type;
	dest->bin = orig->bin;
	dest->kb = (unsigned char) ((orig->taille + 500) / 1000);
	dest->date = orig->date;
	dest->nomess = orig->numero;
	for (i = 0; i < NBMASK; i++)
		dest->fbbs[i] = orig->fbbs[i];
	strncpy (dest->bbsv, bbs_via (orig->bbsv), 6);
}

lfwd *cree_bloc_fwd (lfwd * ptr)
{
	int i;

	if (ptr)
	{
		ptr->suite = (lfwd *) m_alloue (sizeof (lfwd));
		ptr = ptr->suite;
	}
	else
		ptr = (lfwd *) m_alloue (sizeof (lfwd));
	ptr->suite = NULL;
	for (i = 0; i < NBFWD; i++)
		ptr->fwd[i].type = '\0';
	return (ptr);
}

int fwd_mask (char *masque)
{
	int i;

	for (i = 0; i < NBMASK; i++)
		if (*masque++)
			return (1);
	return (0);
}

bloc_mess *new_bloc_mess (void)
{
	int i;

	bloc_mess *bptr = (bloc_mess *) m_alloue (sizeof (bloc_mess));

	bptr->suiv = NULL;
	for (i = 0; i < T_BLOC_MESS; i++)
	{
		bptr->st_mess[i].noenr = 0;
		bptr->st_mess[i].nmess = 0L;
		bptr->st_mess[i].no_indic = 0;
	}
	return (bptr);
}

void end_messages (void)
{
	bloc_mess *bptr;
	lfwd *ptr_fwd;

	/* Libere la liste des forwards */
	while (tete_fwd)
	{
		ptr_fwd = tete_fwd;
		tete_fwd = tete_fwd->suite;
		m_libere (ptr_fwd, sizeof (lfwd));
	}

	/* Libere la liste des messages */
	while (tete_dir)
	{
		bptr = tete_dir;
		tete_dir = tete_dir->suiv;
		m_libere (bptr, sizeof (bloc_mess));
	}
}

static void initmessage (void)
{
	char buf[40];
	bloc_mess *bptr;
	unsigned no_indic;
	unsigned offset = 0;
	bullist bufdir;
	unsigned rmess = 0;
	char stat;
	int pos, nbfwd;
	int i = 0;
	lfwd *ptr_fwd;

	pos = nbfwd = 0;
	ptr_fwd = tete_fwd = cree_bloc_fwd (NULL);

#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Message set-up             \n");
#else
	cprintf ("Initialisation des messages\n");
#endif
#else
#ifdef ENGLISH
	cprintf ("Message set-up             \r\n");
#else
	cprintf ("Initialisation des messages\r\n");
#endif
#endif
	deb_io ();
	nbmess = 0;

	ouvre_dir ();
	read_dir (rmess++, &bufdir);
	nomess = bufdir.numero;
#ifdef __linux__
#ifdef ENGLISH
	cprintf ("Next message %ld  \n", nomess + 1);
#else
	cprintf ("Prochain message %ld\n", nomess + 1);
#endif
#else
#ifdef ENGLISH
	cprintf ("Next message %ld   \r\n", nomess + 1,);
#else
	cprintf ("Prochain message %ld\r\n", nomess + 1);
#endif
#endif

	bptr = tete_dir = new_bloc_mess ();

	while (read_dir (rmess, &bufdir))
	{
		if (bufdir.type)
		{
#ifdef __linux__
			if ((i++ % 50) == 0)
			{
				InitText (itoa (i, buf, 10));
			}
#endif
#ifdef __WINDOWS__
			if ((i++ % 50) == 0)
			{
				InitText (itoa (i, buf, 10));
			}
#endif
#ifdef __MSDOS__
			if (tempo == 0)
			{
				cprintf ("%ld %-6s %d\r\n", bufdir.numero, bufdir.exped, nbfwd);
				tempo = CADENCE;
			}
#endif
			if (offset == T_BLOC_MESS)
			{
				bptr->suiv = new_bloc_mess ();
				bptr = bptr->suiv;
				offset = 0;
			}
			no_indic = insarbre (&bufdir);
			bptr->st_mess[offset].nmess = bufdir.numero;
			bptr->st_mess[offset].noenr = rmess;
			bptr->st_mess[offset].no_indic = no_indic;
			stat = bufdir.status;
			if ((stat != 'A') && (stat != 'K'))
			{
				++nbmess;
				if ((stat != 'F') && (stat != 'X') && (stat != 'H'))
				{
					if ((bufdir.numero) && (fwd_mask (bufdir.fbbs)))
					{
						if (pos == NBFWD)
						{
							pos = 0;
							ptr_fwd = cree_bloc_fwd (ptr_fwd);
						}
						fwd_cpy (&ptr_fwd->fwd[pos], &bufdir);
						++pos;
						++nbfwd;
					}
				}
				else if (stat == 'H')
					++nb_hold;
			}
			ins_iliste (&bufdir);
			++offset;
		}
		++rmess;
	}

#ifdef __WINDOWS__
	InitText (itoa (i, buf, 10));
#else
#ifdef __linux__
	if (bufdir.type)
	{
		cprintf ("%ld %-6s %d\n", bufdir.numero, bufdir.exped, nbfwd);
	}
#else
	if (bufdir.type)
	{
		cprintf ("%ld %-6s %d\r\n", bufdir.numero, bufdir.exped, nbfwd);
	}
#endif
#endif
	ferme_dir ();
	fin_io ();
#ifdef __linux__
#ifdef ENGLISH
	cprintf ("End - %d forward(s)\n", nbfwd);
#else
	cprintf ("Fin - %d forward(s)\n", nbfwd);
#endif
#else
#ifdef ENGLISH
	cprintf ("End - %d forward(s)\r\n", nbfwd);
#else
	cprintf ("Fin - %d forward(s)\r\n", nbfwd);
#endif
#endif
}
