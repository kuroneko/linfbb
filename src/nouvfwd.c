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

/*
 *  MODULE NOUVFWD.C
 */

#include <serv.h>
#include <yapp.h>

#define FDEBUG 0

static char checksum (char *);

static void bin_hdr (char *, long);
static void env_proposition (void);
static void mode_binaire (int);
static void mode_ascii (int);
static void pas_de_message (void);
static void recoit_titre (void);
static void r_n_fwd (void);
static void teste_suite (void);

static int bloc_message (int);
static int get_nouv_fwd (void);
static int rcv_fb (int);
static int rcv_fs (int);
static int s_n_fwd (void);
static int snd_fb (int);
static int snd_fs (int);

/* pvoie->temp2 = TRUE si je suis appelant */

#if defined(__WINDOWS__) || defined(__LINUX__)
char *ffwd_str (int voie, char *s)
{
#define XMODLEN 44
	static char stdesc[10][11] =
	{
		"Snd Prop  ",
		"Wait FS/FF",
		"Wait Prop ",
		"Wait Msg  ",
		"Rcv Msg   ",
		"Snd Title ",
		"Snd Msg   "
	};

	char taille[40];
	char nummes[40];
	char offset[40];
	int niv2 = svoie[voie]->niv2;
	int niv3 = svoie[voie]->niv3;

	*s = '\0';
	if (niv2 == 5 && niv3 >= 0 && niv3 < 7)
	{
		if (svoie[voie]->tailm)
			sprintf (taille, "/%ld", svoie[voie]->tailm);
		else
			*taille = '\0';
		if ((niv3 == 3) || (niv3 == 4))
			sprintf (nummes, "#%d ", svoie[voie]->ind_mess + 1);
		else
			*nummes = '\0';
		if ((niv3 == 4) || (niv3 == 6))
			sprintf (offset, "%ld", svoie[voie]->enrcur);
		else
			*offset = '\0';

		sprintf (s, "FbbFwd:%s %s%s%s",
				 stdesc[niv3], nummes, offset, taille);
	}
	return (s);
}
#endif

void new_fwd (void)
{
	df ("new_fwd", 0);

	switch (pvoie->niv3)
	{

	case 0:
		env_proposition ();
		break;

	case 1:
		if (s_n_fwd ())
			pvoie->ind_mess = -1;
		break;

	case 2:
		r_n_fwd ();
		break;

	case 3:
		while (pvoie->fb_mess[pvoie->ind_mess].type == '\0')
		{
			if (++pvoie->ind_mess >= 5)
			{
				teste_suite ();
				return;
			}
		}
		recoit_titre ();
		ch_niv3 (4);
		break;

	case 4:
		if (get_nouv_fwd ())
			teste_suite ();
		break;

	case 5:
		env_message ();
		break;

	case 6:
		if (bin_message (pvoie->t_read) == 0)
		{
			ch_niv3 (5);
			tst_warning (ptmes);
			env_message ();
		}
		break;
	}


	ff ();
}

static void pactor_ident (void)
{
	char buf[80];
	char *noflag = "*N*";

	if (!P_TOR (voiecur))
		return;

	sprintf (buf, "; %d:%s de %s", actif (1), pvoie->sta.indicatif.call, mycall);
	if (noflag[1] != 'N')
		outln (buf, strlen (buf));
}

static void env_proposition (void)
{
	int nb;

	df ("env_proposition", 0);
	pvoie->enrcur = 0L;
	pvoie->tailm = 0L;
	if ((nb = snd_fb (voiecur)) != 0)
	{
		pvoie->ind_mess = nb;
		ch_niv3 (1);
	}
	else
	{
		if (pvoie->ind_mess == -2)
		{
			outln ("FQ", 2);
			pvoie->deconnect = 6;
		}
		else
		{
			pas_de_message ();
		}
	}
	ff ();
}

static void mess_cancel (char *texte)
{
	int nb = strlen (texte);
	uchar buf[257];

	buf[0] = CAN;
	buf[1] = nb;
	strcpy (buf + 2, texte);
	outs (buf, nb + 2);
}


void env_message (void)
{
	char temp[128];

/*  long    mess_size; */
	long nmess;
	int ratio;
	struct stat st;

	pvoie->seq = 0;

	for (;;)
	{
		if (pvoie->t_read)
		{
			nmess = pvoie->t_read->nmess;
			if (pvoie->binary)
			{					/* Binaire */
				if (pvoie->t_read->verb)
				{
					if (compress_mess (ch_record (NULL, nmess, '\0')))
					{
						deb_io ();
						aff_header (voiecur);
						stat (mess_name (MBINDIR, nmess, temp), &st);
						ratio = 100 - (int) (((st.st_size - 6) * 100) / (ptmes->taille + 75));
						if (ratio < 0)
							ratio = 0;
#ifdef ENGLISH
						sprintf (temp, "Send compressed #%ld (%d%%) \r\n", nmess, ratio);
#else
						sprintf (temp, "Envoie #%ld compress‚ (%d%%)\r\n", nmess, ratio);
#endif
						winputs (voiecur, W_SNDT, temp);
						fin_io ();

						pvoie->enrcur = 0L;
						pvoie->tailm = st.st_size - 6;

						if (bin_message (pvoie->t_read))
						{
							ch_niv3 (6);
							return;
						}
						else
						{
							/* Le message a ete envoye ... On attend */
							pvoie->seq = 1;
							return;
						}
					}
					else
					{
						ch_niv3 (6);
						return;
					}
				}
				else
				{				/* erreur ! */
					sprintf (temp, "Msg #%ld does not exist !\r", nmess);
					mess_cancel (temp);
					pvoie->t_read = pvoie->t_read->suite;
				}
			}
			else
			{					/* Ascii */
				if (pvoie->t_read->verb)
				{
					if (bin_message (pvoie->t_read))
					{
						ch_niv3 (6);
						return;
					}
				}
				else
				{
					sprintf (temp, "Msg #%ld does not exist !\r", nmess);
					outln (temp, strlen (temp));
					pvoie->t_read = pvoie->t_read->suite;
				}
			}
		}
		else
		{
			pvoie->sr_mem = 0;
			mode_ascii (voiecur);
			ch_niv3 (2);
			return;
		}
	}
}


void mode_binaire (int voie)
{
	df ("mode_binaire", 1);
	if (svoie[voie]->binary == 0)
	{
		time_yapp[voie] = -1;
		set_binary (voie, 1);

		/* Les BS ne sont pas autorises en binaire ... */
		set_bs (voiecur, FALSE);
	}
	ff ();
}


void mode_ascii (int voie)
{
	df ("mode_ascii", 1);
	if (svoie[voie]->binary)
	{
		set_binary (voie, 0);
	}
	ff ();
}


void recoit_titre (void)
{
	df ("recoit_titre", 0);
#if FDEBUG
	if (!svoie[CONSOLE]->connect)
		cprintf ("Temp1 : %d\n", pvoie->ind_mess);
#endif
	*(ptmes) = pvoie->fb_mess[pvoie->ind_mess];
	*(pvoie->appendf) = '\0';
	*(pvoie->mess_bid) = '\0';

	ptmes->theme = 0;
	ptmes->numero = 0L;
	ptmes->bin = 0;
	pvoie->chck = 0;
	pvoie->m_ack = 0;
	pvoie->messdate = time (NULL);
	pvoie->mess_num = -1;
	pvoie->enrcur = 0L;
	pvoie->tailm = 0L;

	swapp_bbs (ptmes);

	if ((*ptmes->bbsv == '\0') && (ptmes->type == 'P'))
		pvoie->m_ack = 1;

	if ((*ptmes->bbsv == '\0') && (ptmes->type == 'A'))
		ptmes->type = 'P';

	ptmes->status = 'N';
	if (*ptmes->bbsv)
	{
		if (((ptmes->type != 'P') && (ptmes->type != 'T') && (ptmes->type != 'A')) || (strcmp (ptmes->desti, "SYSOP") == 0))
		{
			ptmes->status = '$';
		}
	}
	reacheminement ();

	if (pvoie->binary)
	{
		char *ptr;

		if (ptype != HD)
		{
			err_new_fwd (0, 0);
			ff ();
			return;
		}

		indd = data + 2;
		ptr = indd;
		while (*ptr++);
		sscanf (ptr, "%ld", &pvoie->noenr_menu);

		if (pvoie->noenr_menu)
			old_part (pvoie->sta.indicatif.call, ptmes->bid);
		else
			part_file (pvoie->sta.indicatif.call, ptmes->bid);
	}
	else
		pvoie->noenr_menu = 0L;

	rcv_titre ();

	ptmes->taille = pvoie->noenr_menu;

	ff ();
}


int write_temp_bin (int voie, int all)
{
	int fd;
	obuf *msgtemp;
	bullist *pbul = &svoie[voie]->entmes;

	df ("write_temp_bin", 1);
	if (svoie[voie]->msgtete == NULL)
	{
		ff ();
		return (1);
	}

	fd = open (svoie[voie]->sr_fic, O_WRONLY | O_APPEND | O_BINARY);

	if (fd != -1)
	{
		while ((msgtemp = svoie[voie]->msgtete) != NULL)
		{
			if ((!all) && (msgtemp->suiv == NULL))
				break;
			if (write (fd, msgtemp->buffer, msgtemp->nb_car) != msgtemp->nb_car)
			{
				char buffer[128];

				sprintf (buffer, "write_temp_bin : write %s", svoie[voie]->sr_fic);
				write_error (buffer);
			}
			svoie[voie]->memoc -= msgtemp->nb_car;
			svoie[voie]->msgtete = msgtemp->suiv;
			m_libere ((char *) msgtemp, sizeof (*msgtemp));
		}
		close (fd);
	}
	else
	{
		char buffer[128];

		sprintf (buffer, "write_temp_bin : open %s", svoie[voie]->sr_fic);
		write_error (buffer);
	}
	mod_part (svoie[voie]->sta.indicatif.call, pbul->taille, pbul->bid);
	ff ();
	return (1);
}

static int get_mess_bin (char clog, char *ptr, int nbcar)
{
	int ncars, nb;
	obuf *msgtemp;
	char *ptcur;

	df ("get_mess_bin", 5);
	for (nb = 0; nb < nbcar; nb++)
		pvoie->chck += ptr[nb];

	pvoie->enrcur += nbcar;

	nb = 0;
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
	while (nbcar--)
	{
		++pvoie->memoc;
		++(ptmes->taille);
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
	if (pvoie->memoc >= MAXMEM)
	{
		write_temp_bin (voiecur, FALSE);
	}
	nb = clog;					/* Bidon ! */
	ff ();
	return (0);
}

static int check_phase (ushort crc, long lg)
{
	FILE *fptr;
	ushort tcrc;
	long tlg;

	if ((fptr = fopen (pvoie->sr_fic, "rb")) != NULL)
	{
		fseek (fptr, 20L, 0);
		if ((fread (&tcrc, sizeof (tcrc), 1, fptr)) &&
			(fread (&tlg, sizeof (lg), 1, fptr)))
		{
			if (moto)
			{
				tcrc = xendien (tcrc);
				tlg = xendienl (tlg);
			}
			if ((tlg == lg) && (tcrc == crc))
			{
				fclose (fptr);
				return (1);
			}
		}
		fclose (fptr);
	}
	return (0);
}

int get_nouv_fwd (void)
{
	int nb;
	int nbcar;
	ushort crc;
	long lg;
	long numess;

	if (pvoie->deconnect)
		return (0);

	if (pvoie->binary)
	{
		switch (ptype)
		{
		case DT:
			nbcar = (data[1]) ? data[1] & 0xff : 256;

			if ((pvoie->noenr_menu) && (nbcar >= 6))
			{
				for (nb = 2; nb < 8; nb++)
					pvoie->chck += data[nb];
				memcpy (&crc, data + 2, sizeof (crc));
				memcpy (&lg, data + 4, sizeof (lg));
				if (moto)
				{
					crc = xendien (crc);
					lg = xendienl (lg);
				}
				if (!check_phase (crc, lg))
				{
					del_part (voiecur, ptmes->bid);
					libere (voiecur);
					err_new_fwd (1, 14);
					return (0);
				}
				else
				{
					df ("DT_1", 0);
					get_mess_bin ('W', data + 8, nbcar - 6);
					ff ();
				}
				pvoie->noenr_menu = 0L;
			}
			else
			{
				df ("DT_2", 0);
				get_mess_bin ('W', data + 2, nbcar);
				ff ();
			}
			return (0);
		case ET:				/* Fin du message */
			df ("ET", 0);
			if (!write_temp_bin (voiecur, TRUE))
			{
				ff ();
				return (0);
			}
			ff ();
			pvoie->chck += data[1];
			if (pvoie->chck)
			{
				del_part (voiecur, ptmes->bid);
				libere (voiecur);
				err_new_fwd (1, 1);
				return (0);
			}
			else
			{
				/* On prevoie le numero !! */
				numess = nomess + 1;
				if ((numess % 0x10000L) == 0)
					numess = nomess + 2;
				ptmes->numero = numess;
				/* pdebug("  (%d) decompress %ld", voiecur, numess); */
				dde_huf (voiecur, ptmes, DECODE);
#ifdef HUFF_TASK
				while (pvoie->ask == 0L)
				{
					deb_io ();
					zero_tic ();
					fin_io ();
				}
#endif
				del_part (voiecur, ptmes->bid);
				ptmes->taille = pvoie->ask;
				pvoie->ask = 0L;
				pvoie->enrcur = 0L;
				if (ptmes->taille == -1L)
				{
					/* Erreur dans le decodage */
					return (err_new_fwd (0, 2));
				}
				else
				{
					char asc_file[130];
					int test;

					if (pvoie->header)
						strn_cpy (6, ptmes->bbsf, pvoie->sta.indicatif.call);
					else
						*(ptmes->bbsf) = '\0';

					ptmes->datesd = pvoie->messdate;
					ptmes->date = ptmes->datech = time (NULL);

					if (deja_recu (ptmes, 1, &test) == 1)
					{
						char temp[256];

						/* Already received on another channel */
						sprintf (temp, "Bid %s already received. Message discarded\r\n", ptmes->bid);
						winputs (voiecur, W_RCVT, temp);
					}
					else
					{
						if (!is_held (mess_name (MESSDIR, ptmes->numero, asc_file)))
						{
							/* On affecte le numero officiellement ! */
							ptmes->numero = 0L;
							numess = ptmes->numero = next_num ();

							/* On valide le message */
							put_mess_fwd ('W');
							tst_sysop (ptmes->desti, numess);
							tst_serveur (ptmes);
							tst_ack (ptmes);
							tst_warning (ptmes);
						}
					}
					return (1);
				}
			}
		default:
			err_new_fwd (0, 3);
			return (0);
		}
	}
	return (get_mess_fwd ('W', indd, nb_trait, 2));
}


void teste_suite (void)
{
	df ("teste_suite", 0);
#if FDEBUG
	if (!svoie[CONSOLE]->connect)
		cprintf ("Dans teste suite : ind_mess %d temp2 %d\n", pvoie->ind_mess, pvoie->temp2);
#endif

	if (save_fic)
	{
		++pvoie->sta.ack;
		pvoie->deconnect = 6;
		ff ();
		return;
	}

	while (++pvoie->ind_mess < 5)
	{
		if (pvoie->fb_mess[pvoie->ind_mess].type)
		{
			ch_niv3 (3);
			ff ();
			return;
		}
	}
	mode_ascii (voiecur);
	if (!pvoie->temp2)
	{
#if FDEBUG
		if (!svoie[CONSOLE]->connect)
			cprintf ("Appel_rev 1\n");
#endif
		if (appel_rev_fwd (0))
		{
			ch_niv3 (0);
			env_proposition ();
		}
		else
		{
			pas_de_message ();
		}
	}
	else
	{
		if ((pvoie->ind_mess = snd_fb (voiecur)) != 0)
		{
			ch_niv3 (1);
		}
		else
		{
			pas_de_message ();
		}
	}
	ff ();
}


void pas_de_message (void)
{
	pvoie->ind_mess = -2;
	init_rec_fwd (voiecur);
	outln ("FF", 2);
	ch_niv3 (2);
#if FDEBUG
	if (!svoie[CONSOLE]->connect)
		cprintf ("Pas de message voie %d\n", voiecur);
#endif
}


/* reception forward */

void r_n_fwd (void)
{
	int c, nb;
	long noprec = 0L;
	int chck;
	char *ligne = indd;
	char s[256];

	if (*indd == ';')
		return;

	if (*indd != 'F')
	{
		if ((*indd != '\r') && (*indd != '*'))
		{
			err_new_fwd (0, 13);
		}
		else
		{
			mode_ascii (voiecur);
			pvoie->deconnect = 3;
		}
		return;
	}
	++indd;

	c = *indd;
	incindd ();

	if (pvoie->ind_mess < 0)
	{
		if (pvoie->ind_mess == -1)
		{						/* Marque les messages envoyes */
			/* Eviter la proposition de messages "=" a chaque bloc */
			noprec = pvoie->fb_mess[0].numero;
			for (nb = 0; nb < MAX_FB; nb++)
			{
				if (pvoie->fb_mess[nb].type == '\0')
					continue;
				*(ptmes) = pvoie->fb_mess[nb];
				sprintf (s, "F %ld V:%s [%ld]",
				   ptmes->numero, pvoie->sta.indicatif.call, ptmes->taille);
				fbb_log (voiecur, 'M', s);
				mark_fwd (voiecur, 0);
				noprec = 0L;
			}
		}
		init_rec_fwd (voiecur);
		pvoie->ind_mess = 0;
	}

	switch (c)
	{

	case 'A':
	case 'B':					/*  Recoit FB  */
		pvoie->chck += checksum (ligne);
		rcv_fb (voiecur);
		break;

	case '>':					/*  Recoit F>  */
		if (!is_room ())
		{
			err_new_fwd (1, 15);
			break;
		}
		if (isxdigit (*indd))
		{
			sscanf (indd, "%x", &chck);
			pvoie->chck += chck;
			if (pvoie->chck)
			{
				err_new_fwd (1, 4);
				break;
			}
		}
		nb = snd_fs (voiecur);
		ch_niv3 (3);
		pvoie->ind_mess = 0;
#if FDEBUG
		if (!svoie[CONSOLE]->connect)
			cprintf ("Accepte %d messages\n", nb);
#endif
		if (nb == 0)
			new_fwd ();
		break;

	case 'F':					/*  Recoit FF */
		if (pvoie->temp2 == TRUE)
		{
			pvoie->ind_mess = -2;
			if (noprec == mess_suiv (voiecur))
			{
				outln ("FQ", 2);
				pvoie->deconnect = 6;
			}
			else
			{
				ch_niv3 (0);
				env_proposition ();
			}
		}
		else
		{
#if FDEBUG
			if (!svoie[CONSOLE]->connect)
				cprintf ("Appel_rev 2\n");
#endif
			if ((appel_rev_fwd (0)) && (noprec != mess_suiv (voiecur)))
			{
				ch_niv3 (0);
				env_proposition ();
			}
			else
			{
				outln ("FQ", 2);
				pvoie->deconnect = 6;
			}
		}
		break;

	case 'Q':					/*  recoit FQ  Demande deconnection ... */
		/* pvoie->deconnect = 6; Wait for remote disconnection (Winlink2000) */
		break;

	default:
		err_new_fwd (0, 12);
		break;
	}
}


/* envoi forward */

int s_n_fwd (void)
{
	int c;

	if (*indd == ';')
		return (0);

	if ((*indd == '\r') || (*indd == '*'))
	{
		mode_ascii (voiecur);
		pvoie->deconnect = 3;
		return (0);
	}

	if (*indd++ != 'F')
	{
		err_new_fwd (0, 5);
		return (0);
	}

	sup_ln (indd);
	c = *indd;
	incindd ();

	switch (c)
	{
	case 'F':
		ch_niv3 (0);
		env_proposition ();
		break;
	case 'S':
		if (rcv_fs (voiecur))
		{
			ch_niv3 (5);
			env_message ();
		}
		else
		{
			ch_niv3 (2);
		}
		break;
	default:
		err_new_fwd (0, 6);
		break;
	}
	return (1);
}


/* Proposition de messages  FB  */

int not_in_fb_mess (long numero, int voie)
{
	int nb = 0;

	while ((nb < MAX_FB) && (svoie[voie]->fb_mess[nb].type))
	{
		if (svoie[voie]->fb_mess[nb].numero == numero)
			return (FALSE);
		nb++;
	}
	return (TRUE);
}


void init_fb_mess (int voie)
{
	int nb;

	for (nb = 0; nb < MAX_FB; nb++)
		svoie[voie]->fb_mess[nb].type = '\0';
}


static char checksum (char *s)
{
	char chck = 0;

	while (*s)
	{
		chck += *s;
		++s;
	}
	return (chck);
}

int check_dupes (int voie, int nb, char *exped, int tst_priv)
{
	int i;

	for (i = 0; i < nb; i++)
	{
		if (strcmp (svoie[voie]->fb_mess[i].exped, exped) == 0)
		{
			if ((tst_priv) || (!PRIVATE (svoie[voie]->fb_mess[i].type)))
				return 1;
		}
	}
	return (0);
}

int snd_fb (int voie)
{
	int ok = 0, nb;
	int nb_dupes = 0;
	int chk_dupes;
	int no_more = 0;
	int tst_priv = 1;
	long max_tfwd = 1024L * (long) p_port[no_port (voie)].maxbloc;
	char chck = 0;
	long tail_tmess = 0L;
	char mode;
	char s[80];
	char *bbs_v;

	nb = 0;
	chk_dupes = ((svoie[voie]->typfwd & FWD_DUPES) == 0);

	init_fb_mess (voie);

	pactor_ident ();

	while (nb < MAX_FB)
	{

		if (tail_tmess > max_tfwd)
			break;

		for (;;)
		{
			if (!mess_suiv (voie))
			{
				if ((nb_dupes) && (chk_dupes))
				{
					svoie[voie]->nb_egal -= nb_dupes;
					nb_dupes = 0;
					chk_dupes = 0;
					continue;
				}
				else
				{
					no_more = 1;
					break;
				}
			}

			if (chk_dupes)
			{
				if ((tst_priv) && (!PRIVATE (svoie[voie]->entmes.type)))
				{
					svoie[voie]->nb_egal -= nb_dupes;
					nb_dupes = 0;
					tst_priv = 0;
					continue;
				}

				if (svoie[voie]->nb_egal == (NB_DEL - 1))
				{
					svoie[voie]->nb_egal -= nb_dupes;
					nb_dupes = 0;
					chk_dupes = 0;
					continue;
				}

				if (!check_dupes (voie, nb, svoie[voie]->entmes.exped, tst_priv))
					break;

				svoie[voie]->mess_egal[(int)svoie[voie]->nb_egal] = svoie[voie]->entmes.numero;
				++(svoie[voie]->nb_egal);
				++nb_dupes;
			}
			else
				break;

		}

		if (no_more)
			break;

		if (std_header & 16)
		{
			if (*svoie[voie]->entmes.bbsv == '\0')
			{
				strcpy (svoie[voie]->entmes.bbsv, mypath);
			}
		}

		if (*(svoie[voie]->entmes.bbsv) == '\0')
		{
			strcpy (svoie[voie]->entmes.bbsv, svoie[voie]->sta.indicatif.call);
		}

		if ((*(svoie[voie]->entmes.exped)) && (*(svoie[voie]->entmes.bbsv)) &&
			(*(svoie[voie]->entmes.desti)) && (*(svoie[voie]->entmes.bid)) &&
			(ISGRAPH (svoie[voie]->entmes.type)))
		{
			svoie[voie]->fb_mess[nb] = svoie[voie]->entmes;
			if (HIE (svoie[voie]->mode))
				bbs_v = svoie[voie]->entmes.bbsv;
			else
				bbs_v = bbs_via (svoie[voie]->entmes.bbsv);
			if (svoie[voie]->prot_fwd & FWD_BIN)
				mode = 'A';
			else
				mode = 'B';
			if ((svoie[voie]->entmes.type == 'A') && (!ACQ (svoie[voie]->mode)))
				svoie[voie]->entmes.type = 'P';
			sprintf (s, "F%c %c %s %s %s %s %ld\r",
					 mode, svoie[voie]->entmes.type,
				svoie[voie]->entmes.exped, bbs_v, svoie[voie]->entmes.desti,
					 svoie[voie]->entmes.bid, svoie[voie]->entmes.taille);
			outs (s, strlen (s));
			chck += checksum (s);
			tail_tmess += svoie[voie]->entmes.taille;
			++ok;
			++nb;
		}
		else
			mark_fwd (voie, 0);
	}
	if (ok)
	{
		sprintf (s, "F> %02X\r", (-chck) & 0xff);
		outs (s, strlen (s));
	}
	svoie[voie]->nb_egal -= nb_dupes;
	return (ok);
}


/* message a envoyer ou a marquer FS */

int rcv_fs (int voie)
{
	long pos;
	int c, no = 0, nb = 0;
	rd_list *ptemp = NULL;
	char wtexte[256];

	sup_ln (indd);
	libere_tread (voie);
	while ((c = *indd++) != 0)
	{
		if (no >= 5)
			return (err_new_fwd (0, 7));
		svoie[voie]->fb_mess[no].taille = 0L;
		svoie[voie]->entmes = svoie[voie]->fb_mess[no];
		switch (c)
		{
		case 'E':
#ifdef ENGLISH
			sprintf (wtexte,
					 "Msg #%ld from %s to %s@%s Bid:%s\rSubject: %s\rFormat error  \r",
					 svoie[voie]->entmes.numero,
					 svoie[voie]->entmes.exped,
					 svoie[voie]->entmes.desti,
					 svoie[voie]->entmes.bbsv,
					 svoie[voie]->entmes.bid,
					 svoie[voie]->entmes.titre);
			if (w_mask & W_ERROR)
				mess_warning (admin, "*** FORMAT ERROR *** ", wtexte);
#else
			sprintf (wtexte,
					 "Msg #%ld de %s pour %s@%s Bid:%s\rSujet: %s\rErreur de format\r",
					 svoie[voie]->entmes.numero,
					 svoie[voie]->entmes.exped,
					 svoie[voie]->entmes.desti,
					 svoie[voie]->entmes.bbsv,
					 svoie[voie]->entmes.bid,
					 svoie[voie]->entmes.titre);
			if (w_mask & W_ERROR)
				mess_warning (admin, "*** ERREUR FORMAT ***", wtexte);
#endif
			mark_fwd (voie, 0);
			svoie[voie]->fb_mess[no].type = '\0';
			break;
		case 'R':
#ifdef ENGLISH
			sprintf (wtexte,
					 "Msg #%ld from %s to %s@%s Bid:%s\rSubject: %s\rRejected by %s\r",
					 svoie[voie]->entmes.numero,
					 svoie[voie]->entmes.exped,
					 svoie[voie]->entmes.desti,
					 svoie[voie]->entmes.bbsv,
					 svoie[voie]->entmes.bid,
					 svoie[voie]->entmes.titre,
					 svoie[voie]->sta.indicatif.call);
			if (w_mask & W_REJECT)
				mess_warning (admin, "*** REJECTED MAIL ***  ", wtexte);
#else
			sprintf (wtexte,
					 "Msg #%ld de %s pour %s@%s Bid:%s\rSujet: %s\rRejete par %s   \r",
					 svoie[voie]->entmes.numero,
					 svoie[voie]->entmes.exped,
					 svoie[voie]->entmes.desti,
					 svoie[voie]->entmes.bbsv,
					 svoie[voie]->entmes.bid,
					 svoie[voie]->entmes.titre,
					 svoie[voie]->sta.indicatif.call);
			if (w_mask & W_REJECT)
				mess_warning (admin, "*** COURRIER REJETE ***", wtexte);
#endif
			mark_fwd (voie, 'R');
			svoie[voie]->fb_mess[no].type = '\0';
			break;
		case 'H':
#ifdef ENGLISH
			sprintf (wtexte,
			 "Msg #%ld from %s to %s@%s Bid:%s\rSubject: %s\rHeld by %s \r",
					 svoie[voie]->entmes.numero,
					 svoie[voie]->entmes.exped,
					 svoie[voie]->entmes.desti,
					 svoie[voie]->entmes.bbsv,
					 svoie[voie]->entmes.bid,
					 svoie[voie]->entmes.titre,
					 svoie[voie]->sta.indicatif.call);
			if (w_mask & W_HOLD)
				mess_warning (admin, "*** HELD MAIL ***      ", wtexte);
#else
			sprintf (wtexte,
			 "Msg #%ld de %s pour %s@%s Bid:%s\rSujet: %s\rRetenu par %s\r",
					 svoie[voie]->entmes.numero,
					 svoie[voie]->entmes.exped,
					 svoie[voie]->entmes.desti,
					 svoie[voie]->entmes.bbsv,
					 svoie[voie]->entmes.bid,
					 svoie[voie]->entmes.titre,
					 svoie[voie]->sta.indicatif.call);
			if (w_mask & W_HOLD)
				mess_warning (admin, "*** COURRIER RETENU ***", wtexte);
#endif
			if (ptemp)
			{
				ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
				ptemp = ptemp->suite;
			}
			else
			{
				ptemp = (rd_list *) m_alloue (sizeof (rd_list));
				svoie[voie]->t_read = ptemp;
			}
			ptemp->suite = NULL;
			ptemp->nmess = svoie[voie]->fb_mess[no].numero;
			ptemp->pmess = &(svoie[voie]->fb_mess[no]);
			ptemp->verb = 1;
			++nb;
			break;
		case 'N':
		case '-':
			mark_fwd (voie, 0);
			svoie[voie]->fb_mess[no].type = '\0';
			break;
		case 'L':
		case '=':
			svoie[voie]->fb_mess[no].type = '\0';
			svoie[voie]->mess_egal[(int)svoie[voie]->nb_egal] = svoie[voie]->fb_mess[no].numero;
			++(svoie[voie]->nb_egal);
			break;
		case 'A':
		case '!':
			pos = 0;
			while (isdigit (*indd))
			{
				pos *= 10;
				pos += *indd - '0';
				++indd;
			}
			svoie[voie]->fb_mess[no].taille = pos;
		case 'Y':
		case '+':
			if (ptemp)
			{
				ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
				ptemp = ptemp->suite;
			}
			else
			{
				ptemp = (rd_list *) m_alloue (sizeof (rd_list));
				svoie[voie]->t_read = ptemp;
			}
			ptemp->suite = NULL;
			ptemp->nmess = svoie[voie]->fb_mess[no].numero;
			ptemp->pmess = &(svoie[voie]->fb_mess[no]);
			ptemp->verb = 1;
			++nb;
			break;
		default:
			return (err_new_fwd (0, 8));
		}
		++no;
	}
	if (no != pvoie->ind_mess)
	{
		return (err_new_fwd (0, 9));
	}
	if (nb)
	{
		svoie[voie]->enrcur = 0L;
		if (svoie[voie]->prot_fwd & FWD_BIN)
			mode_binaire (voie);
		return (1);
		/* return(snd_mess(voie, 1)) ;  */
	}
	return (0);
}


/* marquage d'un message par son id */

void mark_fwd (int voie, char mode)
{
/**************** ESSAI ******************/
	switch (mode)
	{
	case 'R':
		sup_fwd (svoie[voie]->entmes.numero, FALSE, svoie[voie]->bbsfwd);
		break;
	case '\0':
		if ((svoie[voie]->entmes.status == '$') || (multi_prive))
			sup_fwd (svoie[voie]->entmes.numero, TRUE, svoie[voie]->bbsfwd);
		else
			sup_fwd (svoie[voie]->entmes.numero, TRUE, 0);
		break;
	}
	svoie[voie]->entmes.type = '\0';
}


int addr_check (char *s)
{
	char *str = s;
	int nb = 0;

	while (*s)
	{
		if (*s == '.')
		{
			if (std_header & 2)
				nb = -10;
			else
				nb = 0;
		}
		else
		{
			if (nb == 6)
			{
				if (!(FOR (pvoie->mode)))
				{
					n_cpy (40, varx[0], str);
					texte (T_ERR + 17);
				}
				return (0);
			}
			++nb;
		}
		++s;
	}
	return (1);
}

int is_bid (char *bid)
{
	char *ptr = bid;

	strupr (bid);

	if ((strlen (bid)) > 12)
		return (0);

	if (*bid == ' ')
		return (1);

	while (*bid)
	{
		if (ISGRAPH (*bid))
		{
			*ptr++ = *bid;
		}
		++bid;
	}
	*ptr = '\0';
	return (1);
}

/* Marque un BID propose comme forwarde en retour */

void mark_reverse_bid (bullist * fb_mess, int nb, int *t_res)
{
	int i;
	int pos;
	FILE *fptr;
	int nobbs = 0;
	bidfwd fwbid;
	bullist bul;
	mess_noeud *lptr;

	nobbs = svoie[voiecur]->bbsfwd;

	for (i = 0; i < nb; i++)
	{
		if (t_res[i] != 1)
			continue;

		pos = search_bid (fb_mess[i].bid);

		/* TESTER SI LE MESSAGE EST A FORWARDER POUR LA DIRECTION !! */
		if (pos)
		{
			fptr = fopen (d_disque ("WFBID.SYS"), "r+b");
			if (fptr == NULL)
				return;

			fseek (fptr, (long) pos * sizeof (bidfwd), 0);
			fread (&fwbid, sizeof (fwbid), 1, fptr);

			fclose (fptr);

			if ((lptr = findmess (fwbid.numero)) != NULL)
			{
				ouvre_dir ();
				read_dir (lptr->noenr, &bul);

				/* Marque le message F s'il etait a forwarder vers nobbs */
				if (already_forw (bul.fbbs, nobbs))
					sup_fwd (fwbid.numero, 1, nobbs);
				ferme_dir ();
			}
		}
	}
}

static int is_invalid (char *champ, int nb, int mode)
{
	int error = 0;
	char *str = champ;
	char last = '\0';

	while (*champ)
	{
		last = *champ;
		if (--nb >= 0)
		{
			if (!ISGRAPH (*champ))
			{
				error = 1;
				break;
			}

			switch (mode)
			{
			case 1:
				if ((*champ == '@') || (*champ == '.'))
					error = 1;
				break;

			case 2:
				if (*champ == '@')
					error = 1;
				break;
			}

			if (error)
				break;

		}
		champ++;
	}

	if ((mode == 2) && ((std_header & 2) == 0) && (last == '.'))
		error = 1;

	if (nb < 0)
		error = 2;

	if (error)
	{
		char s[80];

		sprintf (s, "Error in field (%s)\r", str);
		aff_bas (voiecur, W_CNST, s, strlen (s));
	}
	return (error);
}

int rcv_fb (int voie)
{
	int nb, i;
	char exped[80];
	char desti[80];
	char bbsv[80];
	char bid[80];
	bullist *mptr = &(svoie[voie]->fb_mess[svoie[voie]->ind_mess]);

	if (svoie[voie]->ind_mess >= 5)
		err_new_fwd (0, 10);

	for (i = 0; i < NBMASK; i++)
		mptr->fbbs[i] = mptr->forw[i] = '\0';

	indd[80] = '\0';
	nb = sscanf (indd, "%c %79s %79s %79s %79s %ld",
				 &(mptr->type), exped, bbsv, desti, bid, &(mptr->taille));

	if ((is_invalid (exped, 6, 1)) ||
		(is_invalid (bbsv, 31, 2)) ||
		(is_invalid (desti, 6, 1)) ||
		(is_invalid (bid, 12, 0)) ||
		(!is_bid (bid)) ||
		(nb < 6))
	{
		mptr->type = '\0';
	}
	else
	{
		strcpy (mptr->exped, exped);
		strcpy (mptr->desti, desti);
		strcpy (mptr->bbsv, bbsv);
		strcpy (mptr->bid, bid);
	}


	++svoie[voie]->ind_mess;

	if (!addr_check (mptr->bbsv))
	{
		mptr->type = '\0';
		return (0);
	}

	return (1);
}


int snd_fs (int voie)
{
	int nb, total = 0;
	char chaine[80];
	char s[80];
	int t_res[MAX_FB];
	char *ptr = chaine;
	char *tptr;

	pvoie->tailm = 0L;

	*ptr++ = 'F';
	*ptr++ = 'S';
	*ptr++ = ' ';

	for (nb = 0; nb < MAX_FB; nb++)
		t_res[nb] = 0;

	deja_recu (svoie[voie]->fb_mess, svoie[voie]->ind_mess, t_res);


	if (pvoie->fbb == 2)
		part_recu (svoie[voie]->fb_mess, svoie[voie]->ind_mess, t_res);


	for (nb = 0; nb < svoie[voie]->ind_mess; nb++)
	{
		niveau = nb;
		svoie[voie]->entmes = svoie[voie]->fb_mess[nb];

		if (svoie[voie]->entmes.type == '\0')
		{
			if (pvoie->fbb == 2)
				t_res[nb] = 6;
			else
				t_res[nb] = 2;
		}

		if ((svoie[voie]->rev_mode == 0) && (t_res[nb] == 0))
			t_res[nb] = 2;

		switch (t_res[nb])
		{
		case 0:
			if (svoie[voie]->fbb >= 2)
				*ptr++ = 'Y';		/* Pas encore recu */
			else
				*ptr++ = '+';		/* Pas encore recu */
			++total;
			break;
		case 1:
			if (svoie[voie]->fbb >= 2)
				*ptr++ = 'N';		/* Pas encore recu */
			else
				*ptr++ = '-';		/* Deja recu */
			sprintf (s, "N B:%s V:%s",
					 svoie[voie]->entmes.bid,
					 svoie[voie]->sta.indicatif.call);
			fbb_log (voie, 'M', s);
			svoie[voie]->fb_mess[nb].type = '\0';
			break;
		case 2:
			if (svoie[voie]->fbb >= 2)
				*ptr++ = 'L';		/* Pas encore recu */
			else
				*ptr++ = '=';		/* En cours de reception -> differe */
			svoie[voie]->fb_mess[nb].type = '\0';
			break;
		case 3:
			if (svoie[voie]->fbb >= 2)
				*ptr++ = 'A';		/* Pas encore recu */
			else
				*ptr++ = '!';		/* En partie recu. Demande la fin */
			sprintf (s, "%ld", svoie[voie]->fb_mess[nb].taille);
			tptr = s;
			while (*tptr)
				*ptr++ = *tptr++;
			++total;
			break;
		case 4:
			*ptr++ = 'R';		/* Rejete */
			sprintf (s, "J B:%s V:%s",
					 svoie[voie]->entmes.bid,
					 svoie[voie]->sta.indicatif.call);
			fbb_log (voie, 'M', s);
			svoie[voie]->fb_mess[nb].type = '\0';
			break;
		case 5:
			*ptr++ = 'H';		/* Retenu */
			sprintf (s, "H B:%s V:%s",
					 svoie[voie]->entmes.bid,
					 svoie[voie]->sta.indicatif.call);
			fbb_log (voie, 'M', s);
			++total;
			break;
		case 6:
			*ptr++ = 'E';		/* Erreur */
			svoie[voie]->fb_mess[nb].type = '\0';
			break;
		}
	}
	for (nb = svoie[voie]->ind_mess; nb < MAX_FB; nb++)
		svoie[voie]->fb_mess[nb].type = '\0';
	*ptr = '\0';

	outln (chaine, strlen (chaine));
	send_buf (voie);

	mark_reverse_bid (svoie[voie]->fb_mess, svoie[voie]->ind_mess, t_res);

	if ((total) && (svoie[voie]->prot_fwd & FWD_BIN))
		mode_binaire (voie);
	return (total);
}


int err_new_fwd (int no, int code)
{
	static char cause[][80] =
	{
		"Binary packet is not of header (HD) type in title.",
		"Checsum of message is wrong.",
		"Message could not be uncompressed (CRC Error).",
		"Received binary frame is not DATA (DT), not EOT (ET).",
		"Checsum of proposals is wrong.",
		"Answer to proposals must start with \"F\" or \"**\".",
		"Answer to proposals must be either \"FF\" not \"FS\".",
		"More than 5 answers (with \"+\", \"-\" or \"=\") to proposals.",
		"Answer to proposal is not \"+\", \"-\" or \"=\".",
		"The number of answers does not match the number of proposals.",
		"More than 5 proposals have been received.",
		"The number of fields in a proposal is wrong (6 fields).",
		"Command must be \"FA\", \"FB\", \"F>\", \"FF\" or \"FQ\".",
		"Line starting with a letter which is not \"F\" or \"*\".",
		"Binary file has been changed.",
		"Disk full.",
		"Unknown protocol error."
	};
	static char chaine[2][24] =
	{
		"*** Protocol error: %s",
		"*** Checksum error (%s)"
	};

	char temp[250];

	df ("err_new_fwd", 2);
	mode_ascii (voiecur);
	if (code > 16)
		code = 16;
	sprintf (temp, chaine[no], cause[code]);
	outln (temp, strlen (temp));
	aff_etat ('E');
	send_buf (voiecur);
	pvoie->deconnect = 3;
	ff ();
	return (0);
}


void init_rec_fwd (int voie)
{
	int nb;

	pvoie->chck = 0;
	for (nb = 0; nb < MAX_FB; nb++)
		svoie[voie]->fb_mess[nb].type = '\0';
}


static int bloc_message (int voie)
{
	int i, nb = 0;
	FILE *fptr;
	char chaine[258];
	unsigned mode;
	char *ptr;
	rd_list *ptemp = svoie[voie]->t_read;

	mode = (svoie[voie]->binary) ? O_BINARY : O_TEXT;

	if ((fptr = ouvre_mess (mode, ptemp->nmess, '\0')) != NULL)
	{
		fseek (fptr, svoie[voie]->enrcur, 0);
		fflush (fptr);
		while ((nb = read (fileno (fptr), chaine + 2, 250)) > 0)
//		while ((nb = read (fileno (fptr), chaine + 2, 256)) > 0)
		{
			if (nb)
			{
				if (svoie[voie]->binary)
				{
					chaine[0] = STX;
					chaine[1] = (nb == 256) ? 0 : nb;
					ptr = chaine + 2;
					for (i = 0; i < nb; i++)
						svoie[voie]->chck += *ptr++;
					outs (chaine, nb + 2);
					ptemp->pmess->taille += (long) (nb + 2);
				}
				else
				{
					outs (chaine + 2, nb);
				}
			}
			if (pvoie->memoc >= MAXMEM)
			{
				svoie[voie]->enrcur = ftell (fptr);
				break;
			}
		}
		ferme (fptr, 45);
	}
	else
	{
		/* Incident :
		   Le message a ete supprime sur un autre voie. deconnection !! */
		pvoie->deconnect = 6;
		return (1);
	}
	if (nb == 0)
	{
		if (svoie[voie]->binary)
			sendeot (-svoie[voie]->chck);
		else
			ctrl_z ();
		svoie[voie]->t_read = ptemp->suite;
		m_libere (ptemp, sizeof (rd_list));
		svoie[voie]->enrcur = 0L;
		svoie[voie]->tailm = 0L;
	}
	return (nb);
}


static void bin_hdr (char *header, long pos)
{
	int nb = 2;
	uchar buf[257];
	uchar *ptr = buf;

	*ptr++ = SOH;
	++ptr;
	strcpy (ptr, header);
	while (*ptr++)
		++nb;
	sprintf (ptr, "%6ld", pos);
	while (*ptr++)
		++nb;
	buf[1] = nb;
	outs (buf, nb + 2);
	pvoie->chck = 0;
}


int bin_message (rd_list * ptemp)	/* Envoie un message */
{
	int i, nb = 0;
	char chaine[258];
	FILE *fptr;
	char *ptr;

	pvoie->seq = 0;
	pvoie->sr_mem = 1;

	if (pvoie->enrcur == 0L)
	{
		pvoie->entmes = *(ptemp->pmess);
		if (pvoie->binary)
		{
			if (pvoie->fbb == 2)
			{
				if (pvoie->entmes.taille)
				{
					bin_hdr (pvoie->entmes.titre, pvoie->entmes.taille);
					if ((fptr = ouvre_mess (O_BINARY, pvoie->entmes.numero, '\0')) != NULL)
					{
						fflush (fptr);
						nb = read (fileno (fptr), chaine + 2, 6);
						ferme (fptr, 45);
						pvoie->enrcur = pvoie->entmes.taille;
						if (nb)
						{
							if (pvoie->binary)
							{
								chaine[0] = STX;
								chaine[1] = nb;
								ptr = chaine + 2;
								for (i = 0; i < nb; i++)
									pvoie->chck += *ptr++;
								outs (chaine, nb + 2);
							}
							/* Remet le compteur d'octets envoyes a 0 */
							pvoie->entmes.taille = 0L;
						}
						else
							return (err_new_fwd (0, 14));
					}
					else
						return (err_new_fwd (0, 14));
				}
				else
				{
					pvoie->enrcur = 0L;
					bin_hdr (pvoie->entmes.titre, pvoie->entmes.taille);
				}
			}
			else
			{
				pvoie->enrcur = 2L;
				bin_hdr (pvoie->entmes.titre, pvoie->entmes.taille);
			}
		}
		else
		{
			entete_fwd (voiecur);
		}
	}

	if (bloc_message (voiecur))
		return (1);

	pvoie->sr_mem = 0;
	return (0);
}

/* Envoie les messages ASCII de la liste */
int snd_mess (int voie, int type)
{
	char header[160];
	rd_list *ptemp;

	svoie[voie]->seq = 0;
	svoie[voie]->sr_mem = 1;
	while ((ptemp = svoie[voie]->t_read) != NULL)
	{
		if (trait_time > MAXTACHE)
		{
			svoie[voie]->seq = 1;
			return (1);
		}
		if (svoie[voie]->enrcur == 0L)
		{
			svoie[voie]->entmes = *(ptemp->pmess);
			if (svoie[voie]->binary)
			{
				bin_hdr (svoie[voie]->entmes.titre, 0L);
				make_header (&(svoie[voie]->entmes), header);
				entete_mess_fwd (&(svoie[voie]->entmes), header);
			}
			else
			{
				if (type == 2)
					entete_envoi_fwd (voie);
				entete_fwd (voie);
			}
		}
		if (bloc_message (voie))
			return (1);
		tst_warning (ptmes);
	}
	svoie[voie]->sr_mem = 0;
	return (0);
}
