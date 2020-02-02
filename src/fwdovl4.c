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
 *    MODULE FORWARDING OVERLAY 4
 */

#include <serv.h>

static long t_msg_fwd_suiv (int, uchar, uchar, uchar, int, char *);

#ifdef __FBBDOS__
static char *expand_path (char *cmde)
{
	struct stat statbuf;
	char str[256];
	char *path;

	/* Cherche en direct */
	if (stat (cmde, &statbuf) == 0)
	{
		path = cmde;
		return (path);
	}
	sprintf (str, "%s.EXE", cmde);
	if (stat (str, &statbuf) == 0)
	{
		path = str;
		return (path);
	}
	sprintf (str, "%s.COM", cmde);
	if (stat (str, &statbuf) == 0)
	{
		path = str;
		return (path);
	}
	sprintf (str, "%s.BAT", cmde);
	if (stat (str, &statbuf) == 0)
	{
		path = str;
		return (path);
	}

	/* Cherche avec le path */
	path = searchpath (cmde);
	if (path == NULL)
	{
		sprintf (str, "%s.EXE", cmde);
		path = searchpath (str);
	}
	if (path == NULL)
	{
		sprintf (str, "%s.COM", cmde);
		path = searchpath (str);
	}
	if (path == NULL)
	{
		sprintf (str, "%s.BAT", cmde);
		path = searchpath (str);
	}
	/*
	   if (path == NULL)
	   {
	   dprintf("Pas trouve {%s}\n", str);
	   }
	 */
	return (path);
}
#endif

/*
 * send_dos : envoie une commande MsDos
 *
 * Type = 0 :
 *        1 : Sauvegarde la fenetre BBS et affiche le message d'erreur
 *        2 : Affiche le message d'erreur
 *        3 : Pas d'affichage, de sauvegarde et de retour.
 *        4 : Redirige les sorties sur EXECUTE.$$$ et retourne le contenu
 *        5 : Sauvegarde de la fenetre, pas d'affichage et de retour.
 */

int send_dos (int type, char *commande, char *data)
{
#ifdef __linux__
	return (-1);
#endif
#ifdef __WINDOWS__
	int err;
	char log[256];

	strcpy (log, commande);
	if (data)
	{
		strcat (log, " ");
		strcat (log, data);
	}

	err = fbb_exec (log);

	return (err);

#endif
#ifdef __FBBDOS__
	static char option[] = "/C";
	char *arg[40];
	char log[256];
	char deroute[80];

	char *path;
	int duplic, oldstdout, oldstderr;
	int err, i = 0;
	fen *sav_fen;
	int errtemp;
	int disk;
	int ofst;
	char cur_dir[MAXPATH];

	int n;

	if (!*commande)
		return (0);

	log[0] = (type == 1) ? 'D' : 'X';
	log[1] = ' ';
	strn_cpy (70, log + 2, commande);
	fbb_log (voiecur, 'D', log);

	arg[i++] = getenv ("COMSPEC");
	arg[i++] = option;
	arg[i++] = strtok (commande, " ");
	while ((arg[i] = strtok (NULL, " ")) != NULL)
		++i;

	if (data)
	{
		arg[i++] = data;
		arg[i] = NULL;
	}

	path = expand_path (arg[2]);
	if (path == NULL)
		return (-1);

	arg[2] = path;

	ofst = 2;
	if (strstr (arg[2], ".BAT"))
	{
		ofst = 0;
	}

	deb_io ();

	disk = getdisk ();
	strcpy (cur_dir, "X:\\");
	cur_dir[0] = 'A' + disk;
	getcurdir (0, cur_dir + 3);

	operationnel = FALSE;
	if ((type == 1) || (type == 5))
	{
		sav_fen = create_win (1, 1, 80, h_screen);
		if (EGA)
			ega_close ();
		puttxt (fen_dos);
		puttext_info (&(fen_dos->sav_mod));
		break_ok ();
		err = spawnv (P_WAIT, arg[ofst], arg + ofst);
		if (err == -1)
		{
			ofst = 0;
			err = spawnv (P_WAIT, arg[ofst], arg + ofst);
		}
		break_stop ();
		errtemp = errno;
		gettext_info (&(fen_dos->sav_mod));
		gettxt (1, 1, 80, 25, fen_dos->ptr);
		if (EGA)
			ega_open ();
		close_win (sav_fen);
	}
	else
	{

		if (type == 4)
		{
			/* redirige stdout et stderr sur le fichier EXECUTE.$$$ */
			sprintf (deroute, "%s\\EXECUTE.$$$", getcwd (log, 80));
			duplic = open (deroute, O_CREAT | O_RDWR, S_IWRITE | S_IREAD);
		}
		else
		{
			duplic = open ("NUL", O_WRONLY, S_IWRITE | S_IREAD);
		}

		oldstdout = dup (1);
		oldstderr = dup (2);
		dup2 (duplic, 1);
		dup2 (duplic, 2);
		close (duplic);

		break_ok ();
		err = spawnv (P_WAIT, arg[ofst], arg + ofst);
		if (err == -1)
		{
			ofst = 0;
			err = spawnv (P_WAIT, arg[ofst], arg + ofst);
		}
		break_stop ();
		errtemp = errno;

		dup2 (oldstdout, 1);
		dup2 (oldstderr, 2);
		close (oldstdout);
		close (oldstderr);

		if (type == 4)
		{
			if (err != -1)
			{
				outfich (deroute);
			}
			unlink (deroute);
		}
	}
	operationnel = TRUE;

	setdisk (disk);
	chdir (cur_dir);

	if ((type < 3) && (err == -1))
	{
		strcpy (log, "DOS Error : ");
		for (n = ofst; n < i; n++)
		{
			strcat (log, arg[n]);
			strcat (log, " ");
		}
		errno = errtemp;
		perror (log);
	}

	fin_io ();
	return (err);
#endif
}

/* ok_fwd   = 0 -> Le message n'est pas marque 'F' */
/* Si nobbs = 0 -> raz flags fwd */
void sup_fwd (long num, int ok_fwd, uchar nobbs)
{
	int i, pos, mode;
	int del = 0;
	unsigned num_indic;
	bullist bul;
	ind_noeud *noeud;
	mess_noeud *mptr;

	lfwd *ptr_fwd = tete_fwd;
	recfwd *prec = NULL;

	pos = 0;

	deb_io ();

	if (fast_fwd)
	{
		while (1)
		{
			if (pos == NBFWD)
			{
				ptr_fwd = ptr_fwd->suite;
				if (ptr_fwd == NULL)
					break;
				pos = 0;
			}
			prec = &ptr_fwd->fwd[pos];
			if (prec->nomess == num)
				break;
			pos++;
		}
	}

	if ((mptr = findmess (num)) != NULL)
	{
		ouvre_dir ();
		read_dir (mptr->noenr, &bul);
		if (nobbs <= NBBBS)
		{
			if (nobbs)
			{
				/* Bulletin : Marque la BBS */
				if (fast_fwd)
					clr_bit_fwd (prec->fbbs, nobbs);
				clr_bit_fwd (bul.fbbs, nobbs);
				set_bit_fwd (bul.forw, nobbs);
			}
			else
			{
				/* Prive : Marque et supprime les autres routes */
				for (i = 0; i < NBMASK; i++)
				{
					bul.forw[i] = bul.fbbs[i];
					bul.fbbs[i] = '\0';
				}
				/* Supprime le MID */
				if (del)
					delete_bid (bul.bid);
			}
		}

		/* Supprime le message binaire si forwarde */
		if (!fwd_mask (bul.fbbs))
		{
			if (ok_fwd)
			{
				noeud = insnoeud (bul.desti, &num_indic);
				if (bul.status == 'N')
					--(noeud->nbnew);
				bul.status = 'F';
			}
			if (fast_fwd)
			{
				prec->type = '\0';
				prec->bin = '\0';
			}
			mode = pvoie->binary;
			set_binary (voiecur, 1);
/************** Verifier si pas en cours d'envoi ! *********************/
			del = 1;
			for (i = 1; i < NBVOIES; i++)
			{
				if ((svoie[i]->sta.connect) &&
					(i != voiecur) &&
					(FOR (svoie[i]->mode)) &&
					(svoie[i]->entmes.numero == num))
				{
					del = 0;
				}
			}
			if (del)
				supp_mess (num);	/* supprime le fichier binaire eventuellement */
			set_binary (voiecur, mode);
			aff_msg_cons ();
		}

		write_dir (mptr->noenr, &bul);
		ferme_dir ();
	}
	else if (fast_fwd)
	{
		for (i = 0; i < NBMASK; i++)
			prec->fbbs[i] = '\0';
	}
	fin_io ();
}

/* Cherche si le destinataire existe dans la base et route sur son homebbs */
void entete_envoi_fwd (int voie)
{
	/* envoie la ligne de commande acheminant message vers bbs */
	char s[80];
	char type;
	char *bbs_v;

	if (HIE (svoie[voie]->mode))
		bbs_v = svoie[voie]->entmes.bbsv;
	else
		bbs_v = bbs_via (svoie[voie]->entmes.bbsv);

	type = svoie[voie]->entmes.type;
	if ((type == 'A') && (!ACQ (svoie[voie]->mode)))
		type = 'P';

	sprintf (s, "S%c %s", type, svoie[voie]->entmes.desti);
	printf ("S%c %s\n", type, svoie[voie]->entmes.desti);
	outs (s, strlen (s));
	if (*bbs_v)
	{
		sprintf (s, " @ %s", bbs_v);
		outs (s, strlen (s));
	}
	if (*svoie[voie]->entmes.exped)
	{
		sprintf (s, " < %s", svoie[voie]->entmes.exped);
		outs (s, strlen (s));
	}
	if (*(svoie[voie]->entmes.bid))
	{
		if (((svoie[voie]->entmes.status == '$') && (BID (svoie[voie]->mode)))
			|| (MID (svoie[voie]->mode)) || (strcmp (svoie[voie]->entmes.desti, "SYSOP") == 0)
			)
		{
			sprintf (s, " $%s", svoie[voie]->entmes.bid);
			printf (" $%s\n", svoie[voie]->entmes.bid);
			outs (s, strlen (s));
		}
	}
	cr ();
}


void entete_fwd (int voie)
{
	char *s;
	char header[160];

	s = svoie[voie]->entmes.titre;
	outsln (s, strlen (s));

	deb_io ();

	make_header (&(svoie[voie]->entmes), header);
	entete_mess_fwd (&(svoie[voie]->entmes), header);
	outs (msg_header, strlen (msg_header));
	aff_etat ('E');
	send_buf (voiecur);

	fin_io ();
}


int fin_envoi_fwd (int voie)
{
	/* Si Nø de message nul retour */
	if (svoie[voie]->entmes.numero == 0L)
		return (FALSE);

	/* teste si le message est encore a forwarder et enleve de la liste */
	if ((svoie[voie]->entmes.status == '$') || (multi_prive))
		sup_fwd (svoie[voie]->entmes.numero, TRUE, svoie[voie]->bbsfwd);
	else
		sup_fwd (svoie[voie]->entmes.numero, TRUE, 0);

	return (TRUE);
}

atfwd *attend_fwd (int nobbs, uchar max, uchar old, uchar typ, int data_mode)
{
	int pos, noctet, ok = 0;
	char cmpmsk;
	static atfwd nbmess;
	recfwd *prec;
	lfwd *ptr_fwd = tete_fwd;
	time_t date = time(NULL) - 3600L * (long)old;

	unsigned offset = 0;
	bloc_mess *bptr = tete_dir;
	bullist ligne;

	pos = 0;
	noctet = (nobbs - 1) / 8;
	cmpmsk = 1 << ((nobbs - 1) % 8);

	nbmess.nbpriv = nbmess.nbbul = nbmess.nbkb = 0;

	if (fast_fwd)
	{
		while (1)
		{
			if (pos == NBFWD)
			{
				ptr_fwd = ptr_fwd->suite;
				if (ptr_fwd == NULL)
					break;
				pos = 0;
			}
			prec = &ptr_fwd->fwd[pos];
			pos++;
			if (prec->type)
			{
				if ((data_mode == 0) && (prec->bin))
					continue;

				if ((data_mode == 3) && (!prec->bin))
					continue;

				if ((data_mode == 2) && (prec->bin) && (!PRIVATE (prec->type)))
					continue;

				if ((prec->fbbs[noctet] & cmpmsk) && (prec->date <= date) && (prec->kb <= max))
				{
					if (PRIVATE (prec->type))
					{
						++nbmess.nbpriv;
						ok = 1;
						nbmess.nbkb += prec->kb;
					}
					else if (!(typ & FWD_PRIV))
					{
						++nbmess.nbbul;
						ok = 1;
						nbmess.nbkb += prec->kb;
					}
				}
			}
		}
	}
	else
	{
		ouvre_dir ();

		while (bptr)
		{
			if (bptr->st_mess[offset].noenr)
			{
				read_dir (bptr->st_mess[offset].noenr, &ligne);

				if (ligne.type)
				{
					int kb = (int) (ligne.taille >> 10);

					if ((data_mode == 0) && (ligne.bin))
						;

					else if ((data_mode == 3) && (!ligne.bin))
						;

					else if ((data_mode == 2) && (ligne.bin) && (!PRIVATE (ligne.type)))
						;

					else if ((ligne.fbbs[noctet] & cmpmsk) && (ligne.date <= date) && (kb <= max))
					{
						if (PRIVATE (ligne.type))
						{
							++nbmess.nbpriv;
							ok = 1;
							nbmess.nbkb += kb;
						}
						else if (!(typ & FWD_PRIV))
						{
							++nbmess.nbbul;
							ok = 1;
							nbmess.nbkb += kb;
						}
					}
				}

			}
			if (++offset == T_BLOC_MESS)
			{
				bptr = bptr->suiv;
				offset = 0;
			}
		}
	}
	return ((ok) ? &nbmess : NULL);
}


static int is_egal (long numero)
{
	int i;
	int nb = pvoie->nb_egal;

	if (nb > NB_DEL)
		nb = NB_DEL;

	for (i = 0; i < nb; i++)
	{
		if (pvoie->mess_egal[i] == numero)
			return (1);
	}
	return (0);
}

int not_in_bloc (long numero, int voie)
{
	if (pvoie->niv1 == N_XFWD)
		return (not_in_xfwd_mess (numero, voie));
	else
		return (not_in_fb_mess (numero, voie));
}

static long t_msg_fwd_suiv (int nobbs, uchar max, uchar old, uchar typ, int voie, char *test)
{

	int pos, noctet;
	int data_mode = svoie[voie]->data_mode;
	char cmpmsk;
	recfwd *prec;
	recfwd *psel;
	lfwd *ptr_fwd;
	time_t date = time(NULL) - 3600L * (long)old;

	unsigned offset = 0;
	bloc_mess *bptr = tete_dir;
	bullist bul;
	bullist sel;
	int fsel;
	int skb;

	noctet = (nobbs - 1) / 8;
	cmpmsk = 1 << ((nobbs - 1) % 8);

	if (fast_fwd)
	{
		ptr_fwd = tete_fwd;
		psel = NULL;
		pos = 0;
		while (1)
		{
			if (pos == NBFWD)
			{
				ptr_fwd = ptr_fwd->suite;
				if (ptr_fwd == NULL)
					break;
				pos = 0;
			}
			prec = &ptr_fwd->fwd[pos];

			pos++;

			/* Message en cours d'edition */
			if ((reply == 4) && (svoie[CONSOLE]->enrcur == prec->nomess))
				continue;

			if ((data_mode == 0) && (prec->bin))
				continue;

			if ((data_mode == 3) && (!prec->bin))
				continue;

			if ((data_mode == 2) && (prec->bin) && (!PRIVATE (prec->type)))
				continue;

			if (is_egal (prec->nomess))
			{
				continue;
			}

			if ((prec->type) && (prec->fbbs[noctet] & cmpmsk) &&
				(prec->date <= date) && (prec->kb <= max) && (not_in_bloc (prec->nomess, voie)))
			{

				if (typ & FWD_SMALL)
				{
					if (PRIVATE (prec->type))
					{
						if ((!psel) || (!PRIVATE (psel->type)) || (psel->kb > prec->kb))
							psel = prec;
					}
					else if ((!(typ & FWD_PRIV)) && ((!psel) || (!PRIVATE (psel->type))))
					{
						if ((test == NULL) || (strncmp (test, prec->bbsv, 6) == 0))
						{
							if ((!psel) || (psel->kb > prec->kb))
								psel = prec;
						}
					}
				}
				else
				{
					if (PRIVATE (prec->type))
					{
						if ((!psel) || (!PRIVATE (psel->type)) || (psel->kb > prec->kb))
							/* if ((!psel) || (prec->nomess < psel->nomess)) */
							psel = prec;
					}
					else if ((!(typ & FWD_PRIV)) && ((!psel) || (!PRIVATE (psel->type))))
					{
						if ((test == NULL) || (strncmp (test, prec->bbsv, 6) == 0))
						{
							if ((!psel) || (prec->nomess < psel->nomess))
							{
								psel = prec;
							}
						}
					}
				}
			}
		}

		if ((psel) && (typ & FWD_PRIV) && (!PRIVATE (psel->type)))
			psel = NULL;
		if ((psel) && (!PRIVATE (psel->type)) && (test) && (strncmp (test, psel->bbsv, 6) != 0))
			psel = NULL;
		return ((psel) ? psel->nomess : 0L);
	}
	else
	{
		fsel = 0;
		skb = 0;
		memset (&sel, 0, sizeof (bullist));

		ouvre_dir ();

		while (bptr)
		{
			if (bptr->st_mess[offset].noenr)
			{

				read_dir (bptr->st_mess[offset].noenr, &bul);


				if (bul.type)
				{

					int kb = (int) (bul.taille >> 10);

					if ((reply == 4) && (svoie[CONSOLE]->enrcur == bul.numero))
						continue;

					if ((data_mode == 0) && (bul.bin))
						continue;

					if ((data_mode == 3) && (!bul.bin))
						continue;

					if ((data_mode == 2) && (bul.bin) && (!PRIVATE (bul.type)))
						continue;

					if (!is_egal (bul.numero))
					{

						if ((bul.type) && (bul.fbbs[noctet] & cmpmsk) &&
							(bul.date <= date) && (kb <= max) && (not_in_bloc (bul.numero, voie)))
						{

							if (typ & FWD_SMALL)
							{
								if (PRIVATE (bul.type))
								{
									if ((!fsel) || (!PRIVATE (sel.type)) || (skb > kb))
									{
										sel = bul;
										fsel = 1;
										skb = kb;
									}
								}
								else if ((!(typ & FWD_PRIV)) && ((!fsel) || (!PRIVATE (sel.type))))
								{
									if ((test == NULL) || (strncmp (test, bul.bbsv, 6) == 0))
									{
										if ((!fsel) || (skb > kb))
										{
											sel = bul;
											fsel = 1;
											skb = kb;
										}
									}
								}
							}
							else
							{
								if (PRIVATE (bul.type))
								{
									if ((!fsel) || (!PRIVATE (sel.type)) || (skb > kb))
										/* if ((!psel) || (bul.nomess < psel->nomess)) */
									{
										sel = bul;
										fsel = 1;
										skb = kb;
									}
								}
								else if ((!(typ & FWD_PRIV)) && ((!fsel) || (!PRIVATE (sel.type))))
								{
									if ((test == NULL) || (strncmp (test, bul.bbsv, 6) == 0))
									{
										if ((!fsel) || (bul.numero < sel.numero))
										{
											sel = bul;
											fsel = 1;
											skb = kb;
										}
									}
								}
							}
						}
					}
				}
			}
			if (++offset == T_BLOC_MESS)
			{
				bptr = bptr->suiv;
				offset = 0;
			}
		}


		if ((typ & FWD_PRIV) && (!PRIVATE (sel.type)))
			fsel = 0;
		if ((fsel) && (!PRIVATE (sel.type)) && (test) && (strncmp (test, sel.bbsv, 6) != 0))
			fsel = 0;
		return ((fsel) ? sel.numero : 0L);
	}
}

long msg_fwd_suiv (int nobbs, uchar max, uchar old, uchar typ, int voie)
{
	int i;
	long numero;

	for (i = 0; i < NB_P_ROUTE; i++)
	{
		if (*pvoie->p_route[i])
		{
			numero = t_msg_fwd_suiv (nobbs, max, old, typ, voie, pvoie->p_route[i]);
			if (numero)
			{
				return (numero);
			}
		}
		else
			break;
	}
	numero = t_msg_fwd_suiv (nobbs, max, old, typ, voie, NULL);
	return (numero);
}
