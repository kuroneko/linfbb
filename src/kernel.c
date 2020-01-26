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

 * Routines de gestion des voies (Noyau).
 *
 */

#include <serv.h>

static void choix_suivant (int);
static void lit_port (int);
static void teste_console (void);
static void teste_voies (int);

/* Boucle principale du noyau */

void kernel (void)
{
	static int port = 1;
	static int loc_voie = 1;
	int i;

#ifdef __FBBDOS__
  kernel_boucle:
#endif

	if (!operationnel)
		return;

	aff_etat ('A');

	user_time_out ();

	is_idle = 1;

#ifdef __linux__
	pbsv ();
#endif

	watchdog ();
	teste_console ();
	deb_io ();
	fin_io ();

#ifdef __FBBDOS__

	if (backscroll)
	{
		if (t_scroll)
			goto kernel_boucle;
		else
			fin_backscroll ();
	}

#endif

#ifdef __linux__
	for (i = 1; i < NBPORT; i++)
	{
		if (++port == NBPORT)
			port = 1;

		if ((p_port[port].pvalid) && (p_port[port].stop == 0))
		{
			lit_port (port);
			if (!operationnel)
				return;

			teste_voies (0);

			for (i = 1; i < NBVOIES; i++)
			{
				if (p_port[no_port (i)].stop == 0)
					teste_voies (i);
			}

			break;
		}
	}
#elif defined(__WIN32__)
	for (i = 1; i < NBPORT; i++)
	{
		if (++port == NBPORT)
			port = 1;

		if ((p_port[port].pvalid) && (p_port[port].stop == 0))
		{
			lit_port (port);
			if (!operationnel)
				return;

			teste_voies (0);

			if (++loc_voie == NBVOIES)
				loc_voie = 1;

			if (p_port[no_port (loc_voie)].stop == 0)
				teste_voies (loc_voie);
		}
	}
#elif defined(__MSDOS__) || defined(__WINDOWS__)
	for (i = 1; i < NBPORT; i++)
	{
		if (++port == NBPORT)
			port = 1;

		if ((p_port[port].pvalid) && (p_port[port].stop == 0))
		{
			lit_port (port);
			if (!operationnel)
				return;

			teste_voies (0);

			if (++loc_voie == NBVOIES)
				loc_voie = 1;

			if (p_port[no_port (loc_voie)].stop == 0)
				teste_voies (loc_voie);

			break;
		}
	}
#endif

	if (loc_voie == 1)
	{
		k_tasks ();
	}

	if (!operationnel)
		return;

#ifdef __FBBDOS__
	goto kernel_boucle;
#endif
}

static void teste_console (void)
{
	df ("teste_console", 0);

	trait_time = 0;				/* remet le compteur du watch-dog a 0 */

/***** A SUPPRIMER *****/
	selvoie (CONSOLE);

	switch (traite_console ())
	{
	case 0:
		break;
	case 1:
		if (pvoie->stop == 0)
		{
			traite_voie (CONSOLE);
		}
		break;
	case 2:					/* Demande de forward */
		break;
	}

	ff ();
}


static void tst_ddebcl (int port, uchar * mon, char *exped, char *indic)
{
	int i;
	int ok = 0;
	long ltemp;
	char stemp[80];
	char call[80];
	uchar checksum;
	unsigned ck;
	int ssid = 0;

	df ("tst_ddebcl", 7);

	if (*mon++ == '?')
	{

		if (*mon++ == ' ')
		{

			checksum = 0;
			for (i = 0; i < 10; i += 2)
			{
				if ((!ok) && ((mon[i] != '0') || (mon[i + 1] != '0')))
					ok = 1;
				if ((!isxdigit (mon[i])) && (!isxdigit (mon[i] + 1)))
					break;
				sscanf (mon + i, "%2x", &ck);
				ck &= 0xff;
				if (i < 8)
					checksum += (uchar) ck;
			}

			if ((!ok) || (i != 10) || (checksum != ck))
			{
				ff ();
				return;
			}

			sscanf (indic, "%[0-9A-Z]-%d", stemp, &ssid);
			sscanf (exped, "%[0-9A-Z]", call);
			stemp[6] = '\0';
			call[6] = '\0';

			if ((ssid != myssid) || (strcmp (stemp, mycall) != 0))
			{
				ff ();
				return;
			}

			strn_cpy (8, stemp, mon);
			sscanf (stemp, "%lX", &ltemp);

			dde_synchro (call, ltemp, port);
		}
	}
	ff ();
}

static void choix_suivant (int voie)
{
	int choix;

	df ("choix_suivant", 1);

	if ((svoie[voie]->curfwd) && (voie == svoie[voie]->curfwd->forward))
	{
		choix = get_link (svoie[voie]->bbsfwd);
		if (choix < (int) svoie[voie]->nb_choix)
		{
			++choix;
			set_link (svoie[voie]->bbsfwd, choix);
			clr_bit_fwd (p_port[no_port (voie)].fwd, svoie[voie]->bbsfwd);
			svoie[voie]->curfwd->fwdpos = svoie[voie]->curfwd->lastpos;
		}
		else
			set_link (svoie[voie]->bbsfwd, 1);
	}
	ff ();
	return;
}


static int message (int voie, char *buffer)
{
	char *ptr;
	int val;

	df ("message", 3);

	buffer[2] = '*';
	if ((ptr = strtok (buffer, " ")) == NULL)
	{
		ff ();
		return (0);
	}

	ptr = strtok (NULL, " ");
	if (ptr == NULL)
		return (0);

	switch (*ptr)
	{
	case 'F':
	case 'D':
		if (strtok (NULL, " "))
		{
			choix_suivant (voie);
			val = dec_voie (voie);
			ff ();
			return (val);
		}
		break;
	case 'R':
		if (strlen (ptr) == 8)
		{
			/* Readonly */
			val = con_voie (voie, ptr);
			svoie[voie]->read_only = 1;
			ff ();
			return (val);
		}
		else if (strlen (ptr) == 9)
		{
			/* Reconnect */
			dec_voie (voie);
			val = con_voie (voie, ptr);
			ff ();
			return (val);
		}
		break;
	case 'C':
		if (strlen (ptr) == 9)
		{
			/* Connected */
			val = con_voie (voie, ptr);
			ff ();
			return (val);
		}
		break;
	case 'L':
		ptr = strtok (NULL, " ");	/* indicatif */
		if (strcmp (ptr, "FAILURE") == 0)
		{
			if (svoie[voie]->conf)
			{
				svoie[voie]->conf = 0;
				text_conf (T_CNF + 8);
			}
			choix_suivant (voie);
			val = dec_voie (voie);
			ff ();
			return (val);
		}
		break;
	case 'B':
		if (svoie[voie]->conf)
		{
			svoie[voie]->conf = 0;
			text_conf (T_CNF + 9);
		}
		val = dec_voie (voie);
		ff ();
		return (val);
	default:
		if (svoie[voie]->conf)
		{
			svoie[voie]->conf = 0;
		}
		break;
	}
	ff ();
	return (0);
}

void traite_commande (int voie, char *buffer, int len)
{
	if (svoie[voie]->kiss != -1)
	{
		if (!svoie[CONSOLE]->sta.connect)
			aff_header (voie);
		init_timout (voie);
		if (svoie[voie]->kiss < 0)
		{
			svoie[voie]->kiss = CONSOLE;
		}
		selvoie (svoie[voie]->kiss);
		out ("\n$O: ", 5);
		if (len >= 4)
		{
			outsln (buffer + 4, len - 4);
			selvoie (voie);
			message (voie, buffer);
		}
	}
	else
	{
		nb_trait = 0;
		if (message (voie, buffer))
		{
			traite_voie (voie);	/* Il y a des actions */
			status (voie);
		}
	}
}

void traite_data (int voie, char *buffer, int len)
{
	aff_etat ('R');
	if (svoie[voie]->kiss >= 0)
	{
		if (!svoie[CONSOLE]->sta.connect)
			aff_header (voie);
		init_timout (voie);
		selvoie (svoie[voie]->kiss);
		outs (buffer, len);
		write_capture (buffer, len);

		selvoie (voie);
	}
	else
	{
		in_buf (voie, buffer, len);
		status (voie);
	}
}

/*
 * Ressort un caractere du buffer d'entree
 * retourne le nb de caracteres (1 ou 0)
 */
int get_data (int voie)
{
	if (svoie[voie]->kiss >= 0)
	{
		/* Rien a faire, la data est deja partie */
		return (0);
	}
	else
	{
		return (get_inbuf (voie));
	}
}

static void lit_port (int port)
{
	char buffer[RCV_BUFFER_SIZE];
	int voie;
	int len = 0;
	int max;
	int cmd;
	ui_header ui;
	char header[200];
	char pid[20];
	char to[20];
	char ctl[20];
	char via[10];

	df ("lit_port", 1);

	memset (buffer, 0, sizeof (buffer));

#ifdef __WINDOWS__
	if (p_port[port].typort == TYP_TCP)
	{
		ff ();
		return;
	}
#endif

	if (p_port[port].typort == TYP_MOD)
	{
		lit_port_modem (port);
		aff_etat ('A');
		ff ();
		return;
	}

	max = 30;
	voie = 0xff;
	while (rcv_drv (&port, &voie, &cmd, buffer, &len, &ui))
	{
		switch (cmd)
		{

		case DISPLAY:
			break;

		case NBCHR:
			p_port[port].cur = *((long *) buffer);
			break;

		case NBBUF:
			p_port[port].mem = *((int *) buffer) << 5;
			break;

		case STATS:
			{
				int ch_status = 0;
				int mem = p_port[port].mem >> 5;

				stat_ch *sta_new = (stat_ch *) buffer;
				stat_ch *sta = &svoie[voie]->sta;

				if ((sta_new->stat != -1) && (sta->stat != sta_new->stat))
				{
					sta->stat = sta_new->stat;
					ch_status = 1;
				}
				if ((sta_new->connect != -1) && (sta->connect != sta_new->connect))
				{
					if (sta_new->connect == 1)
					{
						sta->connect = 1;
					}
					ch_status = 1;
				}
				if ((sta_new->ret != -1) && (sta_new->ret != sta->ret))
				{
					sta->ret = sta_new->ret;
					ch_status = 1;
				}
				if ((sta_new->ack != -1) && (sta_new->ack != sta->ack))
				{
					sta->ack = sta_new->ack;
					ch_status = 1;
				}
				if ((sta_new->mem != -1) && (sta->mem != mem))
				{
					sta->mem = mem;
					ch_status = 1;
				}
				svoie[voie]->ch_status = ch_status;
			}
			break;

		case DATA:
			traite_data (voie, buffer, len);
			break;

		case COMMAND:
			traite_commande (voie, buffer, len);
			break;

		case UNPROTO:
			aff_etat ('M');
			if (ui.pid)
				sprintf (pid, " pid %02X", ui.pid);
			else
				*pid = '\0';

			if (*ui.ctl)
				sprintf (ctl, "ctl %s", ui.ctl);
			else
				*ctl = '\0';

			if (*ui.to)
				sprintf (to, " to %s ", ui.to);
			else
				*to = '\0';

			if (*ui.via)
				strcpy (via, "via ");
			else
				*via = '\0';

			sprintf (header, "[%d] fm %s%s%s%s%s%s%s",
					 ui.port, ui.from, to, via, ui.via, ctl, pid, ui.txt);
			add_heard (ui.port, get_indic (ui.from));
			monitor (ui.port, header, strlen (header));
			put_ui (header, W_MONH, strlen (header));

			if (len == 0)
				break;

			switch (ui.pid)
			{
			case 0x01:
				put_rose (buffer, W_MOND, len);
				break;
			case 0xcf:
				put_nr (buffer, W_MOND, len);
				break;
			case 0xf0:
				if (ui.ui)
					tst_ddebcl (ui.port, buffer, ui.from, ui.to);
				monitor (ui.port, buffer, len);
				put_ui (buffer, W_MOND, len);
				break;
			case 0x0:
				/* Pactor */
				monitor (ui.port, buffer, len);
				put_ui (buffer, W_MOND, len);
				break;
			}
			break;

		}
		if (--max == 0)
			break;
	}
	aff_etat ('A');
	ff ();
}

void teste_voies (int voie)
{
	int ch_stat = 0;
	long caltemps;
	static long lock_time = 0L;
	struct stat statbuf;

	df ("teste_voies", 0);

	trait_time = 0;				/* remet le compteur du watch-dog a 0 */

#ifdef __linux__
	if (is_cmd (voie))
	{
		/* A command is being processed */
		traite_voie (voie);
	}
	else
#endif
	if (voie == INEXPORT)
	{
		time (&caltemps);

		if ((inexport) && (!backscroll))
		{
			if (stat (LOCK_IN, &statbuf) == -1)
			{
				lock_time = 0L;
				aff_etat ('Y');
				selvoie (INEXPORT);
				traite_voie (INEXPORT);
			}
			else
			{
				if (lock_time == 0L)
					lock_time = caltemps;
				else if ((caltemps - lock_time) > 300L)
				{
					lock_time = 0L;
					unlink (LOCK_IN);
				}
			}
		}
		return;
	}

	if (svoie[voie]->affport.port == 0xff)
	{
		ff ();
		return;
	}

	if (svoie[voie]->sta.connect)
	{
		ack_suiv (voie);
	}

	if ((!svoie[voie]->stop) && (svoie[voie]->seq))
	{							/* Recharge les bufs de sortie */
		if (tot_mem > 10000L)
		{
			traite_voie (voie);
			ch_stat = 1;
		}
		ff ();
		return;
	}

	if ((v_tell == 0) || (v_tell != voie) || ((v_tell) && (v_tell == voie)))
	{
		while ((!svoie[voie]->seq) && (inbuf_ok (voie)))
		{
			ch_stat = 1;
			traite_voie (voie);
		}
	}

	if (ch_stat)
		status (voie);

	ff ();
}


int min_ok (int voie)
{
	int port;
	int mini;

	df ("min_ok", 1);

	port = no_port (voie);
	mini = 300;

	switch (p_port[port].typort)
	{
	case TYP_BPQ:
		mini = (40 * 64);
		break;
	case TYP_DED:
		mini = (70 * 32);
	case TYP_FLX:
		p_port[port].mem = svoie[voie]->sta.mem * 250;
		mini = (250);
		break;
	}

	ff ();
	return (p_port[port].mem > mini);
}
