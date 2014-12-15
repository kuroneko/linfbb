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
 *    MODULE FWDUTIL.C
 */

#include <serv.h>

static long hold_nb = 0L;
static void append_message (void);
static int data_sysop (void);
static int hold_wp (void);

/*
 *    RECEPTION D'UN MESSAGE
 */

void libere (int voie)
{
	obuf *msgtemp;

	while ((msgtemp = svoie[voie]->msgtete) != NULL)
	{
		svoie[voie]->memoc -= msgtemp->nb_car;
		svoie[voie]->msgtete = msgtemp->suiv;
		m_libere ((char *) msgtemp, sizeof (obuf));
		msgtemp = NULL;
	}
	svoie[voie]->entmes.numero = 0L;
}


void supp_mess (long nmess)
{
	char temp[128];

	if (nmess)
		unlink (mess_name ((pvoie->binary) ? MBINDIR : MESSDIR, nmess, temp));
}


int write_mess_temp (unsigned mode, int voie)
{
	int fd;
	obuf *msgtemp;
	char temp[128];

	temp_name (voie, temp);

	fd = open (temp, O_WRONLY | O_APPEND | O_CREAT | mode, S_IREAD | S_IWRITE);

	if (fd != -1)
	{
		while ((msgtemp = pvoie->msgtete) != NULL)
		{
			if (write (fd, msgtemp->buffer, msgtemp->nb_car) != msgtemp->nb_car)
			{
				write_error (temp);
			}
			pvoie->memoc -= msgtemp->nb_car;
			pvoie->msgtete = msgtemp->suiv;
			m_libere ((char *) msgtemp, sizeof (*msgtemp));
		}
		close (fd);
		return (1);
	}
	else
	{
		fbb_error (ERR_OPEN, temp, 0);
		return (0);
	}
}


#if 0
int write_mess (unsigned mode, long nmess)
{
	int fd;
	obuf *msgtemp;

	if (nmess == 0L)
		nmess = next_num ();

	fd = open (nom_mess ((mode == O_BINARY) ? MBIN : MESS, nmess), O_WRONLY | O_APPEND | O_CREAT | mode, S_IREAD | S_IWRITE);
	if (fd != -1)
	{
		while (msgtemp = pvoie->msgtete)
		{
			if (write (fd, msgtemp->buffer, msgtemp->nb_car) != msgtemp->nb_car)
			{
				write_error ();
			}
			pvoie->memoc -= msgtemp->nb_car;
			pvoie->msgtete = msgtemp->suiv;
			m_libere ((char *) msgtemp, sizeof (*msgtemp));
		}
		close (fd);
		return (1);
	}
	else
	{
		write_error ();
		return (0);
	}
}
#endif


static void append_message (void)
{
	int nb;
	FILE *fd, *fich_ptr;
	char buffer[250];

	if ((fich_ptr = fopen (pvoie->appendf, "rb")) == NULL)
	{
		texte (T_ERR + 11);
	}
	else
	{
		next_num ();
		if ((fd = fopen (mess_name (MESSDIR, ptmes->numero, buffer), "ab")) != NULL)
		{
			while (1)
			{
				fflush (fich_ptr);
				nb = read (fileno (fich_ptr), buffer, 250);
				ptmes->taille += nb;
				if (fwrite (buffer, nb, 1, fd) < 1)
					write_error (mess_name (MESSDIR, ptmes->numero, buffer));
				if (nb < 250)
					break;
			}
			ferme (fd, 45);
		}
		ferme (fich_ptr, 44);
		if (voiecur == CONSOLE)
			texte (T_MBL + 20);
	}
}


void ins_fwd (bullist * fwd_rec)
{
	int pos;
	lfwd *ptr_fwd;
	char *ptr = NULL;

	ptr_fwd = tete_fwd;
	pos = 0;

	if ((!fast_fwd) || (fwd_rec->numero == 0L))
		return;

	if (ptr_fwd == NULL)
	{
		cprintf ("ptr_fwd = NULL !!!\n");
		*ptr = 0;
	}

	while (1)
	{
		if (pos == NBFWD)
		{
			if (ptr_fwd->suite == NULL)
				ptr_fwd = cree_bloc_fwd (ptr_fwd);
			else
				ptr_fwd = ptr_fwd->suite;
			pos = 0;
		}
		if (ptr_fwd->fwd[pos].type == '\0')
		{
			fwd_cpy (&ptr_fwd->fwd[pos], fwd_rec);
			break;
		}
		++pos;
	}
}

static void test_ping_pong (bullist * lbul)
{
	int i;

	if ((lbul->type != 'P') || (*lbul->bbsv == '\0'))
		return;

	if (pvoie->warning & W_ROU)
		return;

	for (i = 0; i < NBMASK; i++)
		if (lbul->fbbs[i])
			return;

	/* Ya une bbs, pas de warning et pas de route -> ping-pong ! */
	dde_warning (W_PPG);
}

static int data_sysop (void)
{
	if ((std_header & 32) &&
		(strcmp (ptmes->desti, "SYSOP") == 0) &&
		(ptmes->type == 'P') &&
		(ptmes->bin))
	{
		return (1);
	}
	else
		return (0);
}

static int hold_wp (void)
{
	if ((std_header & 256) && (strcmp (ptmes->desti, "WP") == 0))
		return (0);
	return (1);
}

int is_held (char *ascfile)
{
	long temps;
	char holdname[130];
	int i;
	int nobbs;
	char status = '\0';
	char s[256];

	long numess = ptmes->numero;

	if (test_message)
	{
		int ret = -1;

		/* Ecrire les infos ptmes dans le record 0 de DIRMES.SYS */
		ptmes->numero = nomess;

		ouvre_dir ();
		write_dir (0, ptmes);
		ferme_dir ();

		ptmes->numero = numess;

#ifdef __LINUX__
		sprintf (s, "./m_filter %s %c %s %s %d",
				 ascfile, ptmes->type, ptmes->exped, ptmes->desti, 0);
		ret = filter (s, NULL, 0, NULL, FILTDIR);
#else
		sprintf (s, "m_filter %s %c %s %s %d",
				 ascfile, ptmes->type, ptmes->exped, ptmes->desti, 0);
		ret = filter (s, NULL, 0, NULL, NULL);
#endif

		switch (ret)
		{
		case -1:
			if (test_message == 2)
			{
				/* M-FILTER is not found ... Will not be called again */
				test_message = 0;
				break;
			}
		case 1:				/* Kill */
			for (i = 0; i < NBMASK; i++)
				ptmes->fbbs[i] = ptmes->forw[i] = '\0';
			status = 'K';
			break;
		case 2:				/* Archive */
			for (i = 0; i < NBMASK; i++)
				ptmes->fbbs[i] = ptmes->forw[i] = '\0';
			status = 'A';
			break;
		case 3:				/* Hold */
			for (i = 0; i < NBMASK; i++)
				ptmes->fbbs[i] = ptmes->forw[i] = '\0';
			if ((FOR (pvoie->mode)) && ((nobbs = n_bbs (pvoie->sta.indicatif.call)) != 0))
				set_bit_fwd (ptmes->forw, nobbs);
			status = 'H';
			break;
		default:
			break;
		}

		if (status)
		{
			ptmes->status = status;
		}
	}

	if (test_message == 2)
		test_message = 1;

	if ((hold_wp ()) && ((pvoie->msg_held) || (status == 'H') || (hold (ptmes))))
	{
		/* Le message est HOLD. Il n'est pas gere, mais mis dans le repertoire HOLD */
		ptmes->status = 'H';
		++nb_hold;
		if (*ptmes->bid)
		{
			w_bid ();
		}
		if ((FOR (pvoie->mode)) && ((nobbs = n_bbs (pvoie->sta.indicatif.call)) != 0))
			set_bit_fwd (ptmes->forw, nobbs);

		/* Cree un index du nom de fichier */
		temps = time (NULL);
		if (hold_nb >= temps)
			++hold_nb;
		else
			hold_nb = temps;

		for (;;)
		{
			hold_name (hold_nb, holdname);
			if (access (holdname, 0) != 0)
			{
				hold_temp (voiecur, ascfile, holdname, TRUE);
				break;
			}
			++hold_nb;
		}
		if (!FOR (pvoie->mode))
		{
			texte (T_MBL + 59);
		}
		aff_msg_cons ();		/* Affiche cons + held */

		return (1);
	}
	return (0);
}

void put_mess_fwd (char clog)
{
	int i;

	char s[256];
	long numess = ptmes->numero;

	if (*(pvoie->appendf))
		append_message ();

	if ((strncmp (ptmes->titre, "ACK", 3) != 0) &&
		(strcmp (ptmes->exped, "WP") != 0) &&
		(find (ptmes->exped)) &&
		(!is_serv (ptmes->exped)) &&
		(pvoie->niv2 != 99))
	{
		exped_wp (ptmes->exped, pvoie->mess_home);
	}

	if ((ptmes->status != 'K') && (ptmes->status != 'A'))
	{
		if (ptmes->type == 'B')
			valide_themes (0, 1, ptmes);
			
		if (test_forward (1))
			ins_fwd (ptmes);

		test_ping_pong (ptmes);

		if (!(FOR (pvoie->mode)))
			texte (T_MBL + 48);
	}

	if (clog)
	{
		sprintf (s, "%c %ld%c F:%s T:%s@%s [%ld] S:%s",
				 clog, ptmes->numero, ptmes->type, ptmes->exped,
				 ptmes->desti, ptmes->bbsv, ptmes->taille, ptmes->titre);
		fbb_log (voiecur, 'M', s);
	}

	libere (voiecur);

	flush_wp_cache ();

	pvoie->mess_recu = 1;
	pvoie->header = 0;
	ptmes->numero = numess;

	/* Supprime le message si datas en "SP SYSOP" */
	if (data_sysop ())
	{
		for (i = 0; i < NBMASK; i++)
			ptmes->fbbs[i] = ptmes->forw[i] = '\0';
		ptmes->status = 'K';
		clear_fwd (numess);
	}

	valmess (ptmes);
	++nbmess;
	ins_iliste (ptmes);
#ifdef __LINUX__
	add_pfh (ptmes);
#endif

	aff_msg_cons ();
}


void tst_ack (bullist * pbul)
{
	int nb;
	int numlang = vlang;
	char temp;
	bullist psauve;
	char titre[90];
	char texte[80];
	char suite[80];

	int mode = pvoie->binary;

	psauve = *pbul;

	if (pvoie->m_ack != 2)
		return;

	pvoie->m_ack = 0;
	set_binary (voiecur, 0);
	vlang = 0;
	if (*psauve.bbsv)
	{
		sprintf (suite, " - routed to %s", psauve.bbsv);
	}
	else
		*suite = '\0';
	sprintf (titre, "ACK:%s", psauve.titre);
	sprintf (texte, "Msg %s@%s - %sz%s\032",
	   psauve.desti, mycall, datheure_mbl (time (NULL) + _timezone), suite);
	vlang = numlang;
	ini_champs (voiecur);
	*ptmes->bbsf = '\0';
	temp = *(pvoie->sta.indicatif.call);
	*(pvoie->sta.indicatif.call) = '@';
	strn_cpy (6, ptmes->desti, psauve.exped);
	strn_cpy (6, ptmes->exped, mycall);
	n_cpy (60, ptmes->titre, titre);
	if (routage (psauve.numero))
	{
		swapp_bbs (ptmes);
		ptmes->type = 'A';
		ptmes->status = 'N';
		reacheminement ();
		nb = strlen (texte);
		/*      texte[nb] = '\032' ; */
		get_mess_fwd ('\0', texte, nb, 2);
	}
	*(pvoie->sta.indicatif.call) = temp;
	*pbul = psauve;
	set_binary (voiecur, mode);
}


void tst_sysop (char *desti, long numero)
{
	char buffer[80];
	char sauv_ind[7];
	bullist ptsauv = *ptmes;
	int mode = pvoie->binary;
	Msysop *sptr = mess_sysop;

	if (data_sysop ())
		return;

	if ((sptr) && (strcmp (desti, "SYSOP") == 0))
	{
		if (!find (ptmes->bbsv))
		{
			set_binary (voiecur, 0);
			strn_cpy (6, sauv_ind, pvoie->sta.indicatif.call);
			strn_cpy (6, pvoie->sta.indicatif.call, "SYSOP");
			while (sptr)
			{
				sprintf (buffer, "P %s", sptr->call);
				copy_mess (numero, buffer, '\032');
				sptr = sptr->next;
			}
			strn_cpy (6, pvoie->sta.indicatif.call, sauv_ind);
		}
	}
	*ptmes = ptsauv;
	set_binary (voiecur, mode);
}

int get_mess_fwd (char clog, char *ptr, int nbcar, int type)
{
	int ncars, debut = 1;
	int held = 0;
	long numess = 0L;
	obuf *msgtemp;
	char fin[2];
	char *ptcur;
	char temp[128];

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
		if (debut)
		{
			if ((pvoie->entete) && (*ptr == 'R') && (*(ptr + 1) == ':'))
			{
				analyse_header (voiecur, ptr);
			}
			else
			{
				pvoie->entete = 0;
			}
			if ((*ptr == '/') && (toupper (*(ptr + 1)) == 'A') && (toupper (*(ptr + 2)) == 'C'))
			{
				if (pvoie->m_ack == 1)
				{
					pvoie->m_ack = 2;
				}
			}
			check_bin (ptmes, ptr);
			if ((*ptr == '/') && (toupper (*(ptr + 1)) == 'A') && (toupper (*(ptr + 2)) == 'B'))
			{
				/* Message aborted */
				texte (T_MBL + 21);
				type = 4;
				*ptr = '\32';
			}
			if ((*ptr == '\32') || ((*ptr == '/') && (toupper (*(ptr + 1)) == 'E') && (toupper (*(ptr + 2)) == 'X')))
			{
				int test;

				msgtemp->nb_car = ncars;

				if (type != 4)
				{
					/* Le BID existe ? -> message rejete ! */
					if (((*ptmes->bid == '\0') || (*ptmes->bid == ' ')) &&
						(((pvoie->mess_num != -1) && (*pvoie->mess_home)) ||
						 (*pvoie->mess_bid)))
					{
						make_bid ();
						if (deja_recu (ptmes, 1, &test) == 1)
						{
							type = 4;
							if (!(FOR (pvoie->mode)))
								texte (T_MBL + 45);
						}
					}

					if (pvoie->header)
						strn_cpy (6, ptmes->bbsf, pvoie->sta.indicatif.call);
					else
						*(ptmes->bbsf) = '\0';
					ptmes->datesd = pvoie->messdate;
					ptmes->date = ptmes->datech = time (NULL);	/* Mettre localtime ? */

					if (type < 3)
					{
						/* Flush des buffers */
						/* pdebug("(%d) create message file %s", voiecur, temp_name (voiecur, temp)); */
						write_mess_temp (O_TEXT, voiecur);

						/* tester is hold ? */
						if (is_held (temp_name (voiecur, temp)))
							held = 1;
						else
						{
							numess = ptmes->numero = next_num ();
							rename_temp (voiecur, mess_name (MESSDIR, numess, temp));
						}
					}
				}

				switch (type)
				{
				case 0:
					if (!held)
					{
						put_mess_fwd (clog);
						tst_sysop (ptmes->desti, numess);
						tst_serveur (ptmes);
						tst_ack (ptmes);
						tst_warning (ptmes);
					}
					retour_menu ((pvoie->mbl) ? N_MBL : N_MENU);
					/* deconnexion si fin de message */
					if (save_fic)
						pvoie->deconnect = 6;
					break;
				case 1:
					if (!held)
					{
						put_mess_fwd (clog);
						tst_sysop (ptmes->desti, numess);
						tst_serveur (ptmes);
						tst_ack (ptmes);
						tst_warning (ptmes);
					}
					maj_niv (pvoie->niv1, pvoie->niv2, 0);
					texte (T_MBL + 42);
					/* deconnexion si fin de message */
					if (save_fic)
						pvoie->deconnect = 6;
					break;
				case 2:
					if (!held)
					{
						put_mess_fwd (clog);
						tst_sysop (ptmes->desti, numess);
						tst_serveur (ptmes);
						tst_ack (ptmes);
						tst_warning (ptmes);
					}
					/* deconnexion si fin de message */
					if (save_fic)
						pvoie->deconnect = 6;
					break;
				case 3:
					del_temp (voiecur);
					libere (voiecur);
					break;
				case 4:
					del_temp (voiecur);
					libere (voiecur);
					if (voiecur != INEXPORT)
						retour_menu ((pvoie->mbl) ? N_MBL : N_MENU);
					break;
				}
				return (1);
			}
		}
		else
		{
			if (*ptr == '\32')
			{
				fin[0] = '\r';
				fin[1] = '\32';
				ptr = fin;
				nbcar = 1;
				debut = 1;
			}
		}
		if (*ptr == '\n')
			++ptr;
		else
		{
			if (*ptr == '\r')
			{
				*ptr = '\n';
				if ((nbcar > 0) && (*(ptr + 1) == '\n'))
				{
					--nbcar;
					++ptr;
				}
				debut = 1;
			}
			else
				debut = 0;
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
	}
	msgtemp->nb_car = ncars;
	if (pvoie->memoc > MAXMEM)
	{
		if (type != 3)
			write_mess_temp (O_TEXT, voiecur);
		else
			libere (voiecur);
	}
	return (0);
}

int mbl_hold (void)
{
	long no;
	long temps;
	char holdname[130];
	char filename[130];

	if ((toupper (*indd) == 'O') && (droits (COSYSOP)))
	{
		++indd;
		if (!teste_espace ())
		{
			texte (T_ERR + 2);
			retour_mbl ();
			return (0);
		}

		if (((no = lit_chiffre (1)) != 0L) && (ch_record (NULL, no, ' ') != NULL) && (ptmes->status != 'A'))
		{
			mess_name (MESSDIR, no, filename);

			/* Cree un index du nom de fichier */
			temps = time (NULL);
			if (hold_nb >= temps)
				++hold_nb;
			else
				hold_nb = temps;

			for (;;)
			{
				hold_name (hold_nb, holdname);
				if (access (holdname, 0) != 0)
				{
					hold_temp (voiecur, filename, holdname, FALSE);
					break;
				}
				++hold_nb;
			}

			++nb_hold;
			texte (T_MBL + 59);

			/* Passe le message en status ARCHIVE caché ... */
			ch_record (NULL, no, 'Z');

			/* Supprime le message de la liste des fwd */
			clear_fwd (no);
			aff_msg_cons ();	/* Affiche cons + held */
		}
		else
			texte (T_ERR + 10);
		retour_mbl ();
		return (0);
	}
	return (1);
}
