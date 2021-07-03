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
 * Taches auxilliaires demandees par le noyau
 *
 */

#include <serv.h>
#include <yapp.h>
#include <modem.h>

/* Timer head */
static FbbTimer *timer_head = NULL;

static int inbuf_bin (int);
static int inbuf_xmodem (int);
static int inbuf_yapp (int);
static int nb_buf (int);
static int trait_buf (int);

/* void mail_in (void); */
static void tst_buf (int);
static int tst_minfwd (int, int);

/* Horloge ticker */
long btime (void)
{
	static long oldtime = 0L;
	static long offset = 0L;
	long bt;

#ifdef __WINDOWS__
	struct time dt;

	gettime (&dt);
	bt = (((long) dt.ti_hour * 3600L + (long) dt.ti_min * 60L + (long) dt.ti_sec) << 14) / 900L;
	bt += (((long) dt.ti_hund * 10) << 16) / 3600000L;

	if (bt > 0x17ffffL)
		bt = 0x17ffffL;
	bt += offset;
	while (bt < oldtime)
	{
		/* Le jour a change... */
		offset += 0x180000L;
		bt += 0x180000L;
	}
	// WinDebug("bt = %ld\r\n", bt);
#endif

#ifdef __FBBDOS__
	bt = get_ticker ();
#endif

#ifdef __linux__
	struct timeval tv;
	struct timezone tz;

	gettimeofday (&tv, &tz);

	bt = ((tv.tv_sec % 86400L) << 16) / 3600L;
	bt += ((tv.tv_usec / 1000) << 16) / 3600000L;

	if (bt > 0x17ffffL)
		bt = 0x17ffffL;
	bt += offset;
	while (bt < oldtime)
	{
		/* Le jour a change... */
		offset += 0x180000L;
		bt += 0x180000L;
	}

#endif
	oldtime = bt;
	return (bt);
}

void k_tasks (void)
{
	int port, hour, min;
	int min_arret;
	int i;
	long caltemps;
	Forward *pfwd;
	static long balprec[NBPORT];

	df ("k_tasks", 0);

#ifdef __FBBDOS__
	if (video_off == 3)
		blank_screen ();
#endif

	if (dde_wp_serv)
	{
		wp_server ();
	}

#ifdef __FBBDOS__
	test_kb ();
#endif

#ifdef __WIN32__
	CheckTasks ();
#endif

	if (aff_use == 0)
	{
		free_use ();
		aff_use = -1;
	}

	if (throute)
	{
		hupdate ();
	}

	if ((p_forward) && (!svoie[CONSOLE]->sta.connect))
	{
		maj_fwd ();
	}

	/* Horloge minutes */
	time (&caltemps);
	min = minute (caltemps);
	hour = heure (caltemps);

	if (time_bcl == 0)
	{
		aff_etat ('Y');
		broadcast_list ();
		time_bcl = def_time_bcl;
	}

	/* On force l'arret a H+15 */
	min_arret = min - stop_min;
	if (min_arret < 0)
		min_arret += 60;

	if ((save_fic) && ((!actif (0)) || (min_arret >= 15)))
	{
		if (min != stop_min)
		{
			aff_etat ('Y');
			maintenance ();		/* reboot serveur */
#ifdef __WINDOWS__
			fbb_quit (type_sortie);
			return;
#else
			exit (type_sortie);
#endif
		}
	}

	for (port = 0; port < NBPORT; port++)
	{
		if (save_fic)
			break;
		if (p_port[port].pvalid)
		{
			pfwd = p_port[port].listfwd;
			while (pfwd)
			{
				if (pfwd->forward == -1)
				{
					aff_etat ('F');
					appel_fwd (pfwd, port);
				}
				pfwd = pfwd->suite;
			}
		}
	}

	if (test_fichiers)
	{
		aff_etat ('Y');
		test_fichiers = 0;
		/* Test des fichiers systeme */
		init_bbs ();
		test_buf_fwd ();
		init_buf_swap ();
		init_buf_rej ();
		load_themes ();
		through_mfilter = 1;
	}

	if ((caltemps / 60) != timeprec)
	{

		/* Toutes les minutes */

		aff_etat ('Y');
		timeprec = caltemps / 60;
		aff_nbsta ();
		ferme_log ();			/* Flush du log toutes les minutes */
		if ((!editor) && (blank > 0))
		{
			if (--blank == 0)
				video_off = 3;
		}

		nb_error = 0;
		/* fflush (log_ptr); */
#ifdef __FBBDOS__

		{
			/* Test des espaces disque */
			struct dfree free;
			int sdisk = 0;
			long k_cluster;

			if (DISK[1] == ':')
			{
				sdisk = DISK[0] - '@';
				getdfree (sdisk, &free);
			}
			else
			{
				sdisk = getdisk () + 1;
				getdfree (sdisk, &free);
			}

			k_cluster = ((long) free.df_sclus * (long) free.df_bsec) / 1024L;
			sys_disk = free.df_avail * k_cluster;

			if ((sdisk) && (MBIN[1] == ':') && (MBIN[0] != DISK[0]))
			{
				k_cluster = ((long) free.df_sclus * (long) free.df_bsec) / 1024L;
				sdisk = MBIN[0] - '@';
				getdfree (sdisk, &free);
				tmp_disk = free.df_avail * k_cluster;
			}
			else
				tmp_disk = sys_disk;
		}

		if (ch_fen)
		{
			maj_fen ();
		}
#endif
		aff_date ();
		env_date ();
		init_bbs ();
		if (hour != hour_time)
		{
			hour_time = hour;
			cron (caltemps);
			send_wp_mess ();
		}
		if ((hour == h_maint) && (min == 0))
		{
			house_keeping ();
		}

#ifdef __WINDOWS__
		if (!WindowService ())
#endif
			mail_in ();

		/*          if (p_forward == 0) test_buf_fwd();           */
		init_hold ();
		init_buf_swap ();
		init_buf_rej ();
		load_themes ();

		aff_msg_cons ();

		for (port = 0; port < NBPORT; port++)
		{
			if (save_fic)
				break;

			if (!p_port[port].pvalid)
				continue;

			if (tst_minfwd (min, port))
			{
				for (i = 0; i < NBMASK; i++)
					p_port[port].fwd[i] = '\0';
				pfwd = p_port[port].listfwd;
				while (pfwd)
				{
					if ((pfwd->forward == 0) && (ch_voie (port, 0) != -1))
					{
						*pfwd->fwdbbs = '\0';
						pfwd->forward = -1;
						pfwd->fwdpos = 0;
						pfwd->fin_fwd = pfwd->cptif = pfwd->fwdlig = 0;
						pfwd->reverse = 0;
					}
					if (pfwd->forward == -1)
					{
						if (pfwd->fwdpos == 0xffff)
							pfwd->forward = 0;
						else
						{
							aff_etat ('F');
							appel_fwd (pfwd, port);
						}
					}
					pfwd = pfwd->suite;
				}
			}
			if (t_balise[port] == 0)
				continue;
			if (caltemps / t_balise[port] != balprec[port])
			{
				balprec[port] = caltemps / t_balise[port];
				if (!arret)
				{
					send_balise (port);
				}
			}
		}
	}
	ff ();
}

static int extern_task (int voie)
{
	if ((svoie[voie]->niv1 == N_MOD) &&
	 ((svoie[voie]->niv3 == XS_EXTERN) || (svoie[voie]->niv3 == XR_EXTERN)))
		return 1;
#ifdef __WINDOWS__
	if (editor_on ())
		return (1);
#endif
	return (0);
}

static int off_time (void)
{
	static long prec_time = 0L;
	long diff_time;

	df ("off_time", 0);

	if (prec_time == 0L)
		prec_time = time (NULL);

	diff_time = time (NULL) - prec_time;
	prec_time += diff_time;

	ff ();
	return ((int) diff_time);
}

int del_timer (FbbTimer * timer_id)
{
	FbbTimer *tt = timer_head;
	FbbTimer *pr = NULL;

	while (tt)
	{
		if (tt == timer_id)
		{
			if (pr)
			{
				pr->next = tt->next;
				m_libere (tt, sizeof (FbbTimer));
			}
			else
			{
				timer_head = tt->next;
				m_libere (tt, sizeof (FbbTimer));
			}
			return (1);
		}
		pr = tt;
		tt = tt->next;
	}
	return (0);
}

FbbTimer *add_timer (int delay, int port, void FAR * fct, void *userdata)
{
	FbbTimer *tt = timer_head;

	if (tt)
	{
		while (tt->next)
			tt = tt->next;
		tt->next = (FbbTimer *) m_alloue (sizeof (FbbTimer));
		tt = tt->next;
	}
	else
		tt = timer_head = (FbbTimer *) m_alloue (sizeof (FbbTimer));

	tt->next = NULL;
	tt->userdata = userdata;
	tt->port = port;
	tt->temps = (delay * 182) / 10;
	tt->fct = fct;

	return (tt);
}

static void fbb_timer (void)
{
	static long bprec = -1;
	long bt;
	FbbTimer *tt;
	FbbTimer *pr;
	long delta;

	bt = btime ();
	delta = bt - bprec;

	if (delta == 0)
		return;

	bprec = bt;

	tt = timer_head;
	pr = NULL;

	while (tt)
	{
		if (tt->temps >= (time_t) delta)
			tt->temps -= (time_t) delta;
		else
			tt->temps = 0L;

		if (tt->temps == 0L)
		{
			if (tt->fct)
				(*tt->fct) (tt->port, tt->userdata);

			/* Supprimer le timer */
			del_timer (tt);

			break;

			/*if (pr)
			   tt = pr->next;
			   else
			   tt = timer_head; */
		}
		else
		{
			pr = tt;
			tt = tt->next;
		}
	}
}

void user_time_out (void)
{
	int voie;
	int port;
	int com;
	int dt;

	dt = off_time ();

	fbb_timer ();

	if (dt == 0)
		return;

	df ("user_time_out", 0);

	/* System timers */
	if (t_tell > 0)
	{
		t_tell -= (18 * dt);
		if (t_tell < 0)
			t_tell = 0;
	}

	if (time_bcl > 0)
	{
		time_bcl -= (18 * dt);
		if (time_bcl < 0)
			time_bcl = 0;
	}

	for (com = 0; com < NBPORT; com++)
	{
		if (p_com[com].baud)
			p_com[com].delai += dt;
	}

	for (port = 0; port < NBPORT; port++)
	{
		if ((p_port[port].pvalid) && (p_port[port].stop > 0))
		{
			if (p_port[port].stop > dt)
				p_port[port].stop -= dt;
			else
				p_port[port].stop = 0;
		}
	}

	for (voie = 0; voie < NBVOIES; voie++)
	{
/* disconnect test added  - F6BVP 		
		if (svoie[voie]->sta.connect == 0) 	
			time_att[voie] = 0;  */

		if ((!extern_task (voie)) && (svoie[voie]->sta.connect !=0) && ((voie != CONSOLE) || (!editor)))
		{
			/* Was if (time_att[voie] > 0)  F6BVP */
			if (time_att[voie] >= 0)
			{
				time_att[voie] -= dt;
				if (time_att[voie] <= 0)
				{
/*					fprintf(stderr, "Timeout voie %d depasse\n", voie);*/
					time_att[voie] = 0;
					if (svoie[voie]->nb_err == 10)
					{
						/* Voie bloquee -> Liberation d'office */
/*					fprintf(stderr, "Voie %d bloquee. Liberation d'office\n", voie);*/
						svoie[voie]->sta.connect = 0;
						svoie[voie]->deconnect = 0;
						svoie[voie]->niv3 = 0;
						svoie[voie]->niv2 = 0;
						svoie[voie]->niv1 = 0;
					}
					else
					{
						svoie[voie]->deconnect = 2;
						svoie[voie]->nb_err = 10;
						traite_voie (voie);
					}
				}
			}
			if (time_yapp[voie] > 0)
			{
				time_yapp[voie] -= dt;
				if (time_yapp[voie] < 0)
					time_yapp[voie] = 0;
			}
		}
	}
	ff ();
}

void mail_in (void)
{
	struct stat buf;

	if ((!is_room ()) || (svoie[INEXPORT]->sta.connect) || (inexport))
		return;

	df ("mail_in", 0);

	mail_ch = INEXPORT;

	/* if (access (MAILIN, 0) == 0) */
	if (stat (MAILIN, &buf) == 0)
	{
		init_etat ();
		selvoie (mail_ch);
		strcpy (pvoie->sta.indicatif.call, mycall);
		pvoie->sta.indicatif.num = 0;
		pvoie->enrcur = 0L;
		pvoie->mode = F_FOR;
		pvoie->fbb = 0;
		pvoie->mbl_ext = 0;
		pvoie->prot_fwd = FWD_MBL;
		pvoie->finf.lang = langue[0]->numlang;
		pvoie->ncur = &def_cur;
		pvoie->debut = time (NULL);
		pvoie->tstat = pvoie->debut;
		pvoie->tmach = 0L;
		strcpy (io_fich, MAILIN);
		inexport = 4;	/* Apres console_on ! */
		pvoie->sta.connect = inexport;	/* Apres console_on ! */
		aff_event (mail_ch, 1);
		maj_niv (N_MBL, 99, 0);
#ifdef __WINDOWS__
		window_connect (voiecur);
#endif
		aff_nbsta ();
	}
	ff ();
}


int mail_out (char *fichier)
{
	if ((svoie[INEXPORT]->sta.connect) || (inexport))
		return (0);

	df ("mail_out", 2);

	mail_ch = INEXPORT;

	selvoie (mail_ch);
	n_cpy (256, io_fich, strlwr(fichier));
	strcpy (pvoie->sta.indicatif.call, "MAIL");
	pvoie->sta.indicatif.num = 0;
	aff_event (mail_ch, 1);
	inexport = 4;
	pvoie->sta.connect = inexport;
	pvoie->enrcur = 0L;
	pvoie->debut = time (NULL);
	pvoie->tstat = pvoie->debut;
	pvoie->tmach = 0L;
	pvoie->mode = F_FOR | F_HIE | F_BID | F_MID;
	pvoie->finf.lang = langue[0]->numlang;
	pvoie->ncur = &def_cur;
	maj_niv (N_MBL, 98, 0);
#ifdef __WINDOWS__
	window_connect (voiecur);
#endif
	aff_nbsta ();
	ff ();
	return (1);
}


int voie_forward (int voie)
{
	int port;
	Forward *pfwd;

	df ("voie_forward", 1);

	if (voie != CONSOLE)
	{
		for (port = 1; port < NBPORT; port++)
		{
			if (p_port[port].pvalid)
			{
				pfwd = p_port[port].listfwd;
				while (pfwd)
				{
					if (pfwd->forward == voie)
					{
						ff ();
						return (1);
					}
					pfwd = pfwd->suite;
				}
			}
		}
	}
	ff ();
	return (0);
}


int tst_minfwd (int min, int noport)
{
	int test = p_port[noport].min_fwd;
	int inc = 0;

	df ("tst_minfwd", 2);

	while (inc < 60)
	{
		if (test == min)
		{
			ff ();
			return (1);
		}
		inc += p_port[noport].per_fwd;
		test += p_port[noport].per_fwd;
		if (test >= 60)
			test -= 60;
	}
	ff ();
	return (0);
}


indicat *get_indic (char *chaine)
{
	int c = 0;
	int nb = 0;
	char *ptr;
	static indicat indic;

	df ("get_indic", 2);

	ptr = indic.call;

	while (isalnum (*chaine))
	{
		*ptr++ = toupper (*chaine);
		++chaine;
		if (++nb == 6)
			break;
	}
	*ptr = '\0';

	if (*chaine++ == '-')
		c = atoi (chaine);
	indic.num = c;

	ff ();
	return (&indic);
}


void add_heard (int port, indicat * indic)
{
	int i;
	long date = 0x7fffffffL;
	Heard *pobs = NULL;
	Heard *pheard = p_port[port].heard;

	df ("add_heard", 3);

	for (i = 0; i < NBHEARD; i++)
	{
		if ((pheard->last) && (strcmp (pheard->indic.call, indic->call) == 0) && (pheard->indic.num == indic->num))
		{
			pheard->last = time (NULL);
			++pheard->nb;
			ff ();
			return;
		}
		if (date > pheard->last)
		{
			date = pheard->last;
			pobs = pheard;
		}
		++pheard;
	}
	pobs->indic = *indic;
	pobs->first = pobs->last = time (NULL);
	pobs->nb = 1;
	ff ();
}


static void tst_buf (int voie)
{
	df ("tst_buf", 0);
	if ((svoie[voie]->memoc <= MAXMEM / 2) && (svoie[voie]->sr_mem))
	{
		ptype = SN;
		if (debug_on)
		{
			fprintf (debug_fptr, "Tst_buf : %d\n", svoie[voie]->memoc);
		}
		traite_voie (voie);
	}
	ff ();
}

static int nb_buf (int voie)
{
	int port = no_port (voie);
	int nb = (int) p_port[port].frame * 2;

	df ("nb_buf", 1);
	switch (p_port[port].typort)
	{
	case TYP_PK:
	case TYP_KAM:
		if (nb > 6)
			nb = 6;
		break;
	case TYP_DED:
		if (svoie[voie]->sta.mem < 100)
			nb /= 2;
		break;
	case TYP_FLX:
		/* if (svoie[voie]->sta.mem < 8)
		   nb /= 2;
		   if (nb > 7) */
		nb = 7;
		break;
	case TYP_BPQ:
		if (svoie[voie]->sta.mem < 60)
			nb /= 2;
		break;
#ifdef __WIN32__
	case TYP_MOD:
		nb = 50;
		break;
#endif
	}

	ff ();
	return (nb);
}


int ack_suiv (int voie)
{
	int nblig, n_buf, vide = TRUE, retour = 0;
	int ch_status;
	int port = no_port (voie);
	stat_ch sta;				/* Etat de la voie */
	stat_ch sta_new;			/* Etat de la voie */

	df ("ack_suiv", 1);
	ch_status = svoie[voie]->ch_status;

	sta = svoie[voie]->sta;
	sta_new = sta;

#ifdef __WINDOWS__
	if (ETHER (no_port (voie)))
	{
		sta_new.ack = tcp_busy (voie);
		svoie[voie]->sta = sta_new;

		if (memcmp (&sta, &sta_new, sizeof (stat_ch)) != 0)
			ch_status = 1;
	}
	else
#endif
	if (sta_drv (voie, TNCSTAT, &sta_new))
	{
		if (svoie[voie]->sta.ack != sta_new.ack)
			ch_status = 1;

		if (svoie[voie]->sta.ret != sta_new.ret)
			ch_status = 1;

		svoie[voie]->sta = sta_new;

		/*if (memcmp(&sta, &sta_new, sizeof(stat_ch)) != 0)
		   ch_status = 1; */
	}

	if (svoie[voie]->outptr == NULL)
		svoie[voie]->aut_nc = 0;

	if (svoie[voie]->dde_int)
		interruption (voie);

	svoie[voie]->maxbuf = nb_buf (voie);

	if (voie == CONSOLE)
		svoie[voie]->sta.ack = 0;

	if (svoie[voie]->ask)
	{
		svoie[voie]->ask = 0L;
		traite_voie (voie);
	}

#ifdef  __linux__
	/* dprintf("bin = %d\n", svoie[voie]->binary); */
	if (svoie[voie]->binary == 2)
	{
#if 0
		/* Process en FORK */
		char buffer[300];
		int fd = svoie[voie]->to_xfbb[0];
		int nb;

		int old = fcntl (fd, F_GETFL, 0);

		(void) fcntl (fd, F_SETFL, old | O_NDELAY);
		do
		{
			nb = read (fd, buffer, 300);
			if (nb < 0)
			{
				perror ("read");
				nb = 0;
			}
			if (nb)
			{
				int i;

				dprintf ("Recu %d carac de rszs (fd = %d)\n", nb, fd);
				for (i = 0; i < nb; i++)
					dprintf ("%02x ", buffer[i] & 0xff);
				dprintf ("\n");
				selvoie (voie);
				outs (buffer, nb);
			}
			if (svoie[voie]->memoc >= MAXMEM)
			{
				/* pvoie->sr_mem = TRUE; */
				break;
			}
		}
		while (nb > 0);
		(void) fcntl (fd, F_SETFL, old);
#endif
	}
#endif
	if ((svoie[voie]->sta.ack) || (svoie[voie]->outptr))
	{
		if (!svoie[voie]->sta.connect)
		{
			clear_outbuf (voie);
			svoie[voie]->sta.ack = 0;
		}
		else
		{
			if ((svoie[voie]->stop) && ((nblig = lig_bufi (voie)) != 0))
			{
				trait_buf (voie);
				if (nblig != lig_bufi (voie))
					prog_more (voie);
			}
			if (!svoie[voie]->stop)
			{
				int aff_status = 1;

				/* status(voie) ; */
				if ((voie == CONSOLE) || (DEBUG) || (!p_port[port].pvalid))
				{
					n_buf = svoie[voie]->maxbuf;
					while ((!svoie[voie]->stop) && (svoie[voie]->outptr) && (n_buf--))
					{
						if (aff_status)
						{
							aff_etat ('K');
							aff_status = 0;
							init_timout (voie);
							ch_status = 1;
							vide = FALSE;
						}
						aff_etat ('E');
						send_buf (voie);
						tst_buf (voie);
					}
				}
				else
				{

					int lettre = 0;

					while ((!svoie[voie]->stop) &&
						   (svoie[voie]->outptr) &&
						   (svoie[voie]->sta.ack < svoie[voie]->maxbuf) &&
						   (min_ok (voie)))
					{
						if (lettre == 0)
						{
							lettre = aff_etat ('E');
						}
						if (aff_status)
						{
							aff_etat ('K');
							aff_status = 0;
							init_timout (voie);
							ch_status = 1;
							vide = FALSE;
						}

						if (send_buf (voie) == 0)
							break;

						tst_buf (voie);

						aff_etat (lettre);
					}

					/* Repasse le TNC en reception */
					if ((svoie[voie]->seq == 0) && (svoie[voie]->pack) && (svoie[voie]->t_tr == 0) && (svoie[voie]->outptr == NULL))
					{
						tor_stop (voie);
						svoie[voie]->pack = 0;
					}

				}
			}
			retour = 1;
		}
	}

	if (ch_status)
		status (voie);

	if (vide)
		aff_etat ('A');

	ff ();
	return (retour);
}


int inbuf_ok (int voie)
{
	df ("inbuf_ok", 1);

	nb_trait = 0;
	if (svoie[voie]->binary)
	{
		switch (svoie[voie]->niv1)
		{
		case N_YAPP:
		case N_FORW:
			ff ();
			return (inbuf_yapp (voie));
		case N_MOD:
			ff ();
			return (inbuf_xmodem (voie));
		case N_BIN:
		case N_XFWD:
			ff ();
			return (inbuf_bin (voie));
		}
	}
	else
	{
		if (((v_tell) && (v_tell == voie) && (t_tell == 0)) ||
			((svoie[voie]->inbuf.nblig) && (!svoie[voie]->outptr)) ||
			(svoie[voie]->deconnect > 0))
		{
			if ((svoie[voie]->sta.connect == 0) && (svoie[voie]->deconnect == 0))
				clear_inbuf (voie);
			ff ();
			return (1);
		}
	}
	ff ();
	return (0);
}


int inbuf_bin (int voie)
{
	int nb;

	df ("inbuf_xmodem", 1);

	nb = svoie[voie]->inbuf.nbcar;

	if (svoie[voie]->enrcur + (long) nb > svoie[voie]->tailm)
		nb_trait = (int) (svoie[voie]->tailm - svoie[voie]->enrcur);
	else
		nb_trait = nb;

	if (nb_trait == 0)
	{
		ff ();
		return (0);
	}

	ff ();
	return (1);
}


int inbuf_xmodem (int voie)
{
	df ("inbuf_xmodem", 1);

	if ((nb_trait = svoie[voie]->inbuf.nbcar) == 0)
	{
		ff ();
		return (0);
	}

	ff ();
	return (1);
}


int inbuf_yapp (int voie)
{
	char *ptr;
	lbuf *bptr;
	int nb, fct, ext;

	df ("inbuf_yapp", 1);

	if ((nb = svoie[voie]->inbuf.nbcar) < 2)
	{
		if (time_yapp[voie] == 0)
		{
			nb_trait = 0;
			ptype = TM;
			time_yapp[voie] = -1;
			ff ();
			return (1);
		}
		else
		{
			ff ();
			return (0);
		}
	}
	ptr = svoie[voie]->inbuf.tete->buffer + svoie[voie]->inbuf.nocar;
	fct = *ptr++ & 0xff;
	if ((svoie[voie]->inbuf.tete->lgbuf - svoie[voie]->inbuf.nocar) == 1)
	{
		if (svoie[voie]->inbuf.tete->suite == NULL)
		{
			cprintf ("suite nul - nocar %d - lgbuf %d - nbcar %d c1 %d c2 %d %s\r\n\n\n",
					 svoie[voie]->inbuf.nocar,
					 svoie[voie]->inbuf.tete->lgbuf,
					 svoie[voie]->inbuf.nbcar,
					 *(ptr - 2) & 0xff, *(ptr - 1) & 0xff,
					 svoie[voie]->sta.indicatif.call);
		}
		if (svoie[voie]->inbuf.tete->suite->buffer == NULL)
		{
			cprintf ("buffer nul - nocar %d - lgbuf %d - nbcar %d c1 %d c2 %d %s\r\n\n\n",
					 svoie[voie]->inbuf.nocar,
					 svoie[voie]->inbuf.tete->lgbuf,
					 svoie[voie]->inbuf.nbcar,
					 *(ptr - 2) & 0xff, *(ptr - 1) & 0xff,
					 svoie[voie]->sta.indicatif.call);
		}
		ext = *(svoie[voie]->inbuf.tete->suite->buffer) & 0xff;
	}
	else
		ext = *ptr & 0xff;
	ptype = UK;
	switch (fct)
	{
	case ACK:
		nb_trait = 2;
		switch (ext)
		{
		case 1:
			ptype = RR;
			svoie[voie]->time_trans = time (NULL);
			break;
		case 2:
			ptype = RF;
			svoie[voie]->time_trans = time (NULL);
			svoie[voie]->type_yapp = 0;
			break;
		case 3:
			ptype = AF;
			break;
		case 4:
			ptype = AT;
			break;
		case 5:
			ptype = CA;
			break;
		case ACK:
			ptype = RF;
			svoie[voie]->type_yapp = 1;
			break;
		}
		break;
	case ENQ:
		nb_trait = 2;
		switch (ext)
		{
		case 1:
			ptype = SI;
			break;
		case 2:
			ptype = RI;
			break;
		}
		break;
	case SOH:
		nb_trait = (int) ext + 2;
		ptype = HD;
		break;
	case STX:
		nb_trait = (ext) ? (int) ext + 2 : 258;
		if (svoie[voie]->type_yapp)
			++nb_trait;
		ptype = DT;
		break;
	case ETX:
		nb_trait = 2;
		if (ext == 1)
			ptype = EF;
		break;
	case EOT:
		nb_trait = 2;
		/*if (ext == 1) */ ptype = ET;
		break;
	case NAK:
		nb_trait = ext + 2;
		ptype = NR;
		break;
	case CAN:
		nb_trait = ext + 2;
		ptype = CN;
		break;
	case DLE:
		nb_trait = ext + 2;
		ptype = TX;
		break;
	default:
		--svoie[voie]->inbuf.nbcar;
		if (++svoie[voie]->inbuf.nocar == svoie[voie]->inbuf.tete->lgbuf)
		{
			bptr = svoie[voie]->inbuf.tete->suite;
			/* svoie[voie]->memoc -= svoie[voie]->inbuf.tete->lgbuf ; */
			m_libere ((char *) svoie[voie]->inbuf.tete->buffer, svoie[voie]->inbuf.tete->lgbuf);
			m_libere ((char *) svoie[voie]->inbuf.tete, sizeof (lbuf));
			svoie[voie]->inbuf.tete = bptr;
			svoie[voie]->inbuf.nocar = 0;
		}
		nb_trait = 0;
		ff ();
		return (0);
	}

	if (nb < nb_trait)
	{
		if (time_yapp[voie] == 0)
		{
			nb_trait = 0;
			ptype = TM;
			time_yapp[voie] = -1;
			ff ();
			return (1);
		}
		ff ();
		return (0);
	}

	if (svoie[voie]->kiss == -2)
		init_timout (CONSOLE);

	if (ptype == TX)
	{
		trait_buf (voie);
		out_txt ();
		ff ();
		return (0);
	}

	ff ();
	return (1);
}


/*
 * Recopie dans data une ligne d'un max de 300 caracteres a traiter et
 * retourne le nb de caracteres a traiter
 */

int trait_buf (int voie)
{
	char *ptri;
	char *ptro = data;
	lbuf *bptr;
	int c, nb, nbtot, fin;
	FScreen *screen = &conbuf;

	df ("trait_buf", 1);

	fin = nbtot = 0;

	if ((svoie[voie]->binary) && (nb_trait == 0))
	{
		ff ();
		return (0);
	}

	while ((!fin) && (svoie[voie]->inbuf.tete))
	{
		ptri = svoie[voie]->inbuf.tete->buffer + svoie[voie]->inbuf.nocar;
		/* recopie buffer dans buffer de datas */
		nb = svoie[voie]->inbuf.tete->lgbuf - svoie[voie]->inbuf.nocar;
		while (nb)
		{
			c = *ptri++;
			--svoie[voie]->inbuf.nbcar;
			++svoie[voie]->inbuf.nocar;
			nb--;
			if ((svoie[voie]->binary) || (c != '\n'))
			{
				*ptro++ = c;
				++nbtot;
				if (svoie[voie]->binary)
				{
					if (c == '\r')
						--svoie[voie]->inbuf.nblig;
					if (--nb_trait == 0)
					{
						fin = 1;
						break;
					}
				}
				else
				{
					if (c == '\r')
					{			/* Sortie si fin de ligne */
						--svoie[voie]->inbuf.nblig;
						fin = 1;
						if ((voie == CONSOLE) && (screen->totlig))
						{
							data[nbtot] = '\0';
#ifdef __FBBDOS__
							inputs (CONSOLE, W_CNST, data);
#endif
						}
						break;
					}
				}
				if (nbtot == DATABUF)
				{
					fin = 1;
					break;
				}
			}
		}
		if (nb == 0)
		{
			bptr = svoie[voie]->inbuf.tete->suite;
			/* svoie[voie]->memoc -= svoie[voie]->inbuf.tete->lgbuf ;      */
			m_libere ((char *) svoie[voie]->inbuf.tete->buffer, svoie[voie]->inbuf.tete->lgbuf);
			m_libere ((char *) svoie[voie]->inbuf.tete, sizeof (lbuf));
			svoie[voie]->inbuf.tete = bptr;
			svoie[voie]->inbuf.nocar = 0;
		}
	}
	*ptro = '\0';

	ff ();
	return (nbtot);
}

void traite_voie (int voie)
{
	/* char *ptr = s ; */
	int nbuf, mode;
	long debtrait;

	df ("traite_voie", 1);
	aff_etat ('T');
	selvoie (voie);

	
	if (pvoie->sta.connect != 0) {
		init_timout (voie);
/*		fprintf (stderr, "Initialisation timeout voie %d a %d mn\n", voie, svoie[voie]->timout / 60);  */
	}

	if (pvoie->sta.connect)
		init_langue (voie);
	status (voie);
	if ((voie == INEXPORT) && (inexport))
	{
		aff_traite (voiecur, TRUE);
		time (&debtrait);
		premier_niveau ();
		pvoie->tmach += time (NULL) - debtrait;
		aff_traite (voiecur, FALSE);
		vlang = 0;
		free_mem ();
		ff ();
		return; 
	}
	if (pvoie->deconnect)
		clear_inbuf (voie);
	if (tot_mem > 5000L)
	{
		if ((!pvoie->binary) && (!svoie[CONSOLE]->sta.connect))
		{
			aff_header (voie);
		}
		if ((!pvoie->deconnect) && (pvoie->sta.connect != 0) && (voie >= 0) && (voie < NBVOIES))
		{
			if ((!pvoie->seq) && (!pvoie->sr_mem))
				prog_more (voie);
			if (!pvoie->stop)
			{
				if ((!pvoie->sr_mem) && (!pvoie->seq))
				{
					nb_trait = trait_buf (voie);
				}
				else
					nb_trait = 0;
				if (aff_ok(voiecur))
				{
					deb_io ();
					aff_bas (voiecur, W_RCVT, data, nb_trait);
					fin_io ();
				}
				aff_traite (voiecur, TRUE);
				time (&debtrait);
				if ((voie == CONSOLE) && (print))
				{
#if defined(__WINDOWS__) || defined(__linux__)
					SpoolLine (voie, W_RCVT, data, strlen (data));
#else
					fputs (data, file_prn);
					if (data[nb_trait - 1] == '\r')
						fputc ('\n', file_prn);
#endif
				}
				if (pvoie->dde_marche)
					en_navant_toute ();
				else
				{
					indd = data;
					pvoie->t_tr = 1;
#ifdef COMPUTE
					deb_compute ();
#endif
					premier_niveau ();
#ifdef COMPUTE
					end_compute ();
#endif
					pvoie->t_tr = 0;
					data[0] = '\0';
					nbuf = pvoie->maxbuf;
					while ((nbuf-- > 0) &&
						   (pvoie->sta.ack < 4) &&
						   (pvoie->outptr) &&
						   (min_ok (voie)))
					{
						send_buf (voie);
					}

				}
				pvoie->tmach += time (NULL) - debtrait;
				aff_traite (voiecur, FALSE);
			}

			/* Repasse le TNC en reception */
			if ((pvoie->seq == 0) && (pvoie->pack) && (pvoie->t_tr == 0) && (pvoie->outptr == NULL))
			{
				tor_stop (voiecur);
				pvoie->pack = 0;
			}

		}
	}
	if (pvoie->deconnect > 0)
	{
		switch (pvoie->deconnect)
		{
		case 1:
			if ((!voie_forward (voie)) &&
				(voie != CONSOLE) &&
				(!P_GUEST (voie)) &&
			/*                  (EXP(pvoie->finf.flags) == 0) && */
				(BBS (pvoie->finf.flags) == 0) &&
				(FOR (pvoie->mode) == 0)
				)
			{
				texte (T_MES + 12);
				aff_etat ('E');
				send_buf (voie);
			}
			fbb_log (voiecur, 'X', "B");
			pvoie->log = 0;
			if (pvoie->deconnect == 1)
				mode = bpq_deconnect;
			else
				mode = 1;
			deconnexion (voie, mode);
			break;
		case 2:
			texte (T_ERR + 31);
			aff_etat ('E');
			send_buf (voie);
			fbb_log (voiecur, 'X', "T");
			pvoie->log = 0;
			force_deconnexion (voie, 1);
			break;
		case 3:
			fbb_log (voiecur, 'X', "F");
			pvoie->log = 0;
			force_deconnexion (voie, 1);
			break;
		case 4:
			fbb_log (voiecur, 'X', "M");
			pvoie->log = 0;
			force_deconnexion (voie, 1);
			break;
		case 5:
			fbb_log (voiecur, 'X', "P");
			pvoie->log = 0;
			force_deconnexion (voie, 1);
			break;
		case 6:
			fbb_log (voiecur, 'X', "B");
			pvoie->log = 0;
			if (pvoie->deconnect == 1)
				mode = bpq_deconnect;
			else
				mode = 1;
			deconnexion (voie, mode);
			break;
		case 7:
			outln ("Sorry, no more channels available", 33);
			aff_etat ('E');
			send_buf (voie);
			pvoie->log = 0;
			force_deconnexion (voie, 1);
			break;
		default:
			fbb_log (voiecur, 'X', "?");
			pvoie->log = 0;
			force_deconnexion (voie, 1);
			break;
		}
		pvoie->deconnect = -1;
	}
	vlang = 0;
	free_mem ();
	aff_etat ('A');
	ff ();
}


void premier_niveau (void)
{
	df ("premier_niveau", 0);

	/* Task identifier */
	++tid;

	switch (pvoie->niv1)
	{
	case N_MENU:
		menu_principal ();
		break;
#ifndef MINISERV
	case N_QRA:
		qraloc ();
		break;
	case N_STAT:
		statistiques ();
		break;
	case N_INFO:
		documentations ();
		break;
	case N_NOMC:
		nomenclature ();
		break;
	case N_TRAJ:
		trajec ();
		break;
#endif
	case N_DOS:
		dos ();
		break;
	case N_MOD:
		modem ();
		break;
	case N_BIN:
		bin_transfer ();
		break;
	case N_MBL:
		mbl_emul ();
		break;
	case N_FORW:
		fwd ();
		break;
	case N_TELL:
		duplex_tnc ();
		break;
	case N_YAPP:
		yapp ();
		break;
	case N_CONF:
		conference ();
		break;
	case N_RBIN:
		send_bin_message ();
		break;
	case N_THEMES:
		themes ();
		break;
	case N_XFWD:
		xfwd ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "KERNEL", pvoie->niv1);
		break;
	}
	ff ();
}
