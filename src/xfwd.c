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
 * Module XForwarding
 */

static void wrbuf (void);
static int import_xfwd (char *nom_fich);
static int chk_xfwd (XInfo * xinfo);
static int merge_xfwd (XInfo * xinfo);

#if defined(__WINDOWS__) || defined(__LINUX__)
char *xfwd_str (int voie, char *s)
{
#define XMODLEN 44
	static char stdesc[10][11] =
	{
		"RcvSXnb   ",
		"SndBIDList",
		"WaitSS    ",
		"RcvData   ",
		"          ",
		"SndSXnb   ",
		"WaitSYnb  ",
		"RcvBIDList",
		"SendData  ",
		"WaitPrompt"
	};

	char taille[40];
	int niv = svoie[voie]->niv3;

	*s = '\0';
	if ((niv >= 0) && (niv < 10))
	{
		if (svoie[voie]->tailm)
			sprintf (taille, "/%ld", svoie[voie]->tailm);
		else
			*taille = '\0';
		sprintf (s, "XFwd:%s %ld%s",
				 stdesc[niv], svoie[voie]->enrcur, taille);
	}
	return (s);
}

#endif

/* niv2 = 0 -> Appelle pour forwarder */
/* Niv2 = 1 -> Connexion du distant */

void xfwd (void)
{
	int i;
	int res;
	int nb;
	char s[80];
	char asc_file[128];
	char tmp_file[128];
	mess_noeud *lptr;


	/* Les BS ne sont pas autorises en binaire ... */
	set_bs (voiecur, FALSE);

	switch (pvoie->niv3)
	{

	case 0:
		/* Recoit la commande SX */
		if (pvoie->Xfwd == NULL)
			pvoie->Xfwd = (XInfo *) m_alloue (sizeof (XInfo));
		memset (pvoie->Xfwd, 0, sizeof (XInfo));

		sscanf (indd, "%s %d", s, &pvoie->Xfwd->nb_bid);
		if (stricmp (s, "SX") != 0)
		{
			if (toupper (*s) == 'S')
			{
				/* Forward standard */
				maj_niv (N_FORW, 3, 0);
				fwd ();
				return;
			}
			else
			{
				pvoie->deconnect = 6;
				return;
			}
		}

		set_bs (voiecur, TRUE);
		pvoie->Xfwd->r_bid = 0;
		ch_niv3 (1);
		break;

	case 1:
		/* Lit la liste des BIDs */
		n_cpy (12, pvoie->Xfwd->bid[pvoie->Xfwd->r_bid], sup_ln (indd));

		/* Marque le flag d'acceptation pour le BID donne */
		strcpy (ptmes->bid, pvoie->Xfwd->bid[pvoie->Xfwd->r_bid]);

		res = deja_recu (ptmes, 1, &i);
		if ((res == 1) || (res == 4))
			pvoie->Xfwd->ok_bid[pvoie->Xfwd->r_bid] = 1;
		else
			pvoie->Xfwd->ok_bid[pvoie->Xfwd->r_bid] = 0;

		if (++pvoie->Xfwd->r_bid == pvoie->Xfwd->nb_bid)
		{
			/* Les Bid ont ete recus ... On envoie la reponse */
			nb = 0;
			for (i = 0; i < pvoie->Xfwd->nb_bid; i++)
				if (!pvoie->Xfwd->ok_bid[i])
					++nb;
			sprintf (s, "SY %d", nb);
			outsln (s, strlen (s));
			if (nb)
			{
				for (i = 0; i < pvoie->Xfwd->nb_bid; i++)
					if (!pvoie->Xfwd->ok_bid[i])
						outsln (pvoie->Xfwd->bid[i], strlen (pvoie->Xfwd->bid[i]));
				pvoie->Xfwd->r_bid = 0;
				ch_niv3 (2);
			}
			else
			{
				if (pvoie->niv2 == 0)
				{
					texte (T_MBL + 42);
					ch_niv3 (0);
				}
				else
					retour_mbl ();
			}
		}
		break;

	case 2:
		/* Recoit le SS Size */
		if ((*indd++ == 'S') && (*indd++ == 'S'))
		{
			incindd ();
			set_binary (voiecur, N_BIN);
			pvoie->tailm = 0L;
			pvoie->enrcur = 0L;
			pvoie->checksum = 0;
			pvoie->size_trans = 0L;
			pvoie->time_trans = time (NULL);
			pvoie->Xfwd->chck = 0;
			nb = sscanf (indd, "%ld %u", &pvoie->tailm, &pvoie->Xfwd->chck);
			pvoie->Xfwd->ok_chck = (std_header & 2048) ? 0 : (nb == 2);

			/* Supprime un eventuel fichier temporaire */
			temp_name (voiecur, tmp_file);
			unlink (tmp_file);

			ch_niv3 (3);
		}
		break;

	case 3:
		/* Recoit le fichier des messages binaires */
		wrbuf ();
		if (pvoie->enrcur == pvoie->tailm)
		{
			status (voiecur);
			if (std_header & 2048)
				pvoie->Xfwd->ok_chck = 0;
			if (pvoie->Xfwd->ok_chck && (pvoie->Xfwd->chck != pvoie->checksum))
			{
				/* Checksum error... File is not accepted... Disconnect */
				libere (voiecur);
				err_new_fwd (1, 1);
				temp_name (voiecur, tmp_file);
				unlink (tmp_file);
				return;
			}
			/* Le fichier est recu ... Decompression des messages */
			write_mess_temp (O_BINARY, voiecur);
			set_binary (voiecur, 0);

			basic_lzhuf (DECODE, temp_name (voiecur, tmp_file), xfwd_name (voiecur, asc_file));
			import_xfwd (asc_file);
			unlink (tmp_file);
			unlink (asc_file);
			pvoie->tailm = pvoie->enrcur = 0L;
			if (pvoie->niv2 == 0)
			{
				texte (T_MBL + 42);
				ch_niv3 (0);
			}
			else
				retour_mbl ();

			/* Import des messages */
		}
		break;

	case 5:
		/* Envoie la commande SX */
		if (pvoie->Xfwd == NULL)
			pvoie->Xfwd = (XInfo *) m_alloue (sizeof (XInfo));
		memset (pvoie->Xfwd, 0, sizeof (XInfo));

		if (pvoie->niv2 == 1)
			appel_rev_fwd (TRUE);

		/* Cree la liste des messages */
		nb = chk_xfwd (pvoie->Xfwd);
		pvoie->Xfwd->r_bid = 0;

		if (nb)
		{
			sprintf (s, "SX %d", nb);
			outsln (s, strlen (s));
			for (i = 0; i < nb; i++)
			{
				sprintf (s, "%s", pvoie->Xfwd->bid[i]);
				outsln (s, strlen (s));
			}

			ch_niv3 (6);
		}
		else
		{
			if (pvoie->niv2 == 0)
			{
				/* Passe en reverse */
				if (pvoie->rev_mode)
				{
					texte (T_MBL + 42);
					ch_niv3 (0);
				}
				else
					pvoie->deconnect = 6;
			}
			else
			{
				texte (T_MBL + 47);
				pvoie->deconnect = 6;
			}
		}
		break;

	case 6:
		/* Recoit le SY */
		pvoie->Xfwd->ls_bid = 0;
		sscanf (indd, "%s %d", s, &pvoie->Xfwd->ls_bid);
		if (stricmp (s, "SY") != 0)
			pvoie->deconnect = 6;

		if (pvoie->Xfwd->ls_bid)
			ch_niv3 (7);
		else
			ch_niv3 (9);
		break;

	case 7:
		/* Recoit la liste des BIDs */
		sup_ln (indd);
		for (i = 0; i < pvoie->Xfwd->nb_bid; i++)
		{
			if ((!pvoie->Xfwd->ok_bid[i]) && (strcmp (pvoie->Xfwd->bid[i], indd) == 0))
			{
				pvoie->Xfwd->ok_bid[i] = 1;
				break;
			}
		}

		if (++pvoie->Xfwd->r_bid == pvoie->Xfwd->ls_bid)
		{
			long lg;

			/* Cree le Merge et envoie le fichier compresse */
			merge_xfwd (pvoie->Xfwd);
			pvoie->checksum = 0;
			lg = basic_lzhuf (ENCODE,
							  xfwd_name (voiecur, asc_file),
							  temp_name (voiecur, pvoie->sr_fic));
			unlink (asc_file);
			if (std_header & 2048)
				sprintf (s, "SS %ld", lg);
			else
				sprintf (s, "SS %ld %u", lg, pvoie->checksum);
			outsln (s, strlen (s));
			/* Force l'envoi du paquet ... */
			send_buf (voiecur);

			pvoie->tailm = pvoie->enrcur = 0L;
			pvoie->size_trans = 0L;
			pvoie->xferok = 1;
			pvoie->type_yapp = 4;
			pvoie->tailm = file_size (pvoie->sr_fic);
			set_binary (voiecur, 1);
			ch_niv3 (8);
			xfwd ();
		}
		break;

	case 8:
		/* Envoie le fichier compresse */
		if (senddata (1) == 1)
		{
			aff_etat ('E');
			send_buf (voiecur);
			unlink (pvoie->sr_fic);
			set_binary (voiecur, 0);
			ch_niv3 (9);
		}
		break;

	case 9:
		/* Si reception SID ... Attendre le prompt suivant... */
		if (*indd == '[')
		{
			/* SID recu... Attend le prompt ! */
			ch_niv3 (10);
			break;
		}

		/* Attend le prompt */
		if (att_prompt ())
		{
			pvoie->tailm = pvoie->enrcur = 0L;
			/* Marquer les fichiers forwardes */
			if (pvoie->Xfwd)
			{
				for (nb = 0; nb < MAX_X; nb++)
				{
					/* Uniquement les messages envoyes */
					if (pvoie->Xfwd->ok_bid == 0)
						continue;

					/* Recuperer les infos du message */
					if ((lptr = findmess (pvoie->Xfwd->numero[nb])) == NULL)
						continue;

					ouvre_dir ();
					read_dir (lptr->noenr, ptmes);
					ferme_dir ();

					sprintf (s, "F %ld V:%s [%ld]",
					ptmes->numero, pvoie->sta.indicatif.call, ptmes->taille);
					fbb_log (voiecur, 'M', s);
					mark_fwd (voiecur, 0);
				}
			}
			ch_niv3 (5);
			xfwd ();
		}
		break;

	case 10:
		/* Si reception SID ... Attendre le prompt suivant... */
		if (att_prompt ())
			ch_niv3 (9);
		break;

	default:
		/* Erreur */
		break;
	}
}

static void wrbuf (void)
{
	int ncars;
	int nbcar = nb_trait;

	obuf *msgtemp;
	char *ptcur;
	char *ptr;

	pvoie->enrcur += (long) nbcar;
	pvoie->size_trans += (long) nbcar;

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
	ptr = data;
	while (nbcar--)
	{
		++pvoie->memoc;
		pvoie->checksum += *ptr;
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
	if (pvoie->memoc > MAXMEM)
	{
		write_mess_temp (O_BINARY, voiecur);
	}
}

static void rd_champ (char *ligne, char *champ, int len)
{
	/* Saute le mot-cle */
	while ((*ligne) && (!isspace (*ligne)))
		++ligne;

	/* Saute les espaces */
	while ((*ligne) && (isspace (*ligne)))
		++ligne;

	/* Copie la valeur */
	while (len--)
	{
		if (*ligne)
			*champ++ = *ligne++;
		else
			break;
	}
	*champ = '\0';
}

static int read_xfwd_header (FILE * fptr)
{
	int champs = 0;
	char *ptr;
	char ligne[81];

	ini_champs (voiecur);
	for (;;)
	{
		if (fgets (ligne, 80, fptr) == 0)
			return (-1);

		sup_ln (ligne);

		if ((*ligne == '\0') || (*ligne == ' '))
		{
			/* Fin du bloc */
			break;
		}
		else if (strnicmp ("FROM:", ligne, 5) == 0)
		{
			champs |= 1;
			if ((ptr = strchr (ligne, '@')) != NULL)
			{
				int i;

				*ptr++ = '\0';
				for (i = 0; i < 6; i++)
				{
					if (!isalnum (*ptr))
						break;
					ptmes->bbsf[i] = toupper (*ptr);
					ptr++;
				}
				ptmes->bbsf[i] = '\0';
			}
			rd_champ (ligne, ptmes->exped, 6);
		}
		else if (strnicmp ("TO:", ligne, 3) == 0)
		{
			champs |= 2;
			if ((ptr = strchr (ligne, '@')) != NULL)
			{
				*ptr++ = '\0';
				n_cpy (40, ptmes->bbsv, ptr);
			}
			rd_champ (ligne, ptmes->desti, 6);
		}
		else if (strnicmp ("SUBJECT:", ligne, 8) == 0)
		{
			/* champs |= 4; Sujet non obligatoire */
			rd_champ (ligne, ptmes->titre, 50);
		}
		else if (strnicmp ("X-MSGTYPE:", ligne, 10) == 0)
		{
			/* champs |= 8; Type non obligatoire */
			rd_champ (ligne, &ptmes->type, 1);
		}
		else if (strnicmp ("X-BID:", ligne, 6) == 0)
		{
			/* champs |= 0x10; BID non obligatoire... */
			rd_champ (ligne, ptmes->bid, 12);
		}
	}

	if (ptmes->type == ' ')
	{
		if ((find (ptmes->desti)) || (is_serv (ptmes->desti)))
		{
			ptmes->type = 'P';
		}
		else
		{
			ptmes->type = 'B';
		}
	}

	ptmes->status = 'N';
	if (*ptmes->bbsv)
	{
		if ((ptmes->type == 'P') && (!find (ptmes->desti)) && (!find (bbs_via (ptmes->bbsv))))
			ptmes->type = 'B';
		if ((*ptmes->bbsv) && (!find (bbs_via (ptmes->bbsv))))
			ptmes->status = '$';
	}

	swapp_bbs (ptmes);

	if ((*ptmes->bbsv == '\0') && (ptmes->type == 'P'))
	{
		pvoie->m_ack = 1;
	}

	if (!addr_check (ptmes->bbsv))
	{
		return (-2);
	}

	reacheminement ();

	if ((*ptmes->bbsv == '\0') && (ptmes->type == 'A'))
	{
		ptmes->type = 'P';
	}

	entete_saisie ();

	return (champs);
}

static int import_xfwd (char *nom_fich)
{
	int retour;
	int test;
	int recu;
	int msg = 1;
	int res;
	FILE *fptr;
	char ligne[256];
	char bbsf[10];
	char bbsv[50];

	retour = 1;
	aff_etat ('I');


	if ((fptr = fopen (nom_fich, "rt")) != NULL)
	{
		while (1)
		{
			/* Lire header + taches swap, etc... */
			res = read_xfwd_header (fptr);
			if (res == -1)
			{
				/* Fin de fichier */
				break;
			}
			else if (res == 0)
			{
				/* Ligne vide avant les propositions */
				continue;
			}

			*bbsf = '\0';

			if (*ptmes->bbsv)
				sprintf (bbsv, "@%s", ptmes->bbsv);
			else
				*bbsv = '\0';

			sprintf (ligne, "XFWD T:%c Fm:%s%s To:%s%s Id:%s\r",
					 (ptmes->type) ? ptmes->type : '?',
					 ptmes->exped, bbsf,
					 ptmes->desti, bbsv,
					 (*ptmes->bid) ? ptmes->bid : "None");
			aff_bas (voiecur, W_RCVT, ligne, strlen (ligne));

			if (res != 0x3)
			{
				sprintf (ligne, "Error header = %d\r", res);
				aff_bas (voiecur, W_SNDT, ligne, strlen (ligne));

				/* Fin de fichier ou Erreur : Le message n'est pas importe... */
				retour = 0;
				msg = 0;
			}
			else
			{
				/* Test reject */
				recu = deja_recu (ptmes, 1, &test);
				if ((recu == 1) || (recu == 4))
				{
					sprintf (ligne, "Message rejected\r");
					aff_bas (voiecur, W_SNDT, ligne, strlen (ligne));
					msg = 0;
				}
			}

			while (1)
			{
				if (fgets (ligne, 80, fptr) == 0)
				{
					/* Error ... Fichier non fini */
					break;
				}
				lf_to_cr (ligne);

				if (get_mess_fwd ('X', ligne, strlen (ligne), (msg) ? 2 : 3))
				{
					/* Message enregistre */
					msg = 1;
					break;
				}
			}

		}
		if (fptr)
			ferme (fptr, 7);
	}

	return (retour);
}

static int wr_xfwd_mess (FILE * fptr, bullist * pbul)
{
	char last_char;
	char *ptr;
	FILE *fptm;
	char header[256];
	char bbsf[10];
	char bbsv[50];

	fprintf (fptr, "From: %s\n", pbul->exped);
	if (*pbul->bbsv)
		fprintf (fptr, "To: %s@%s\n", pbul->desti, pbul->bbsv);
	else
		fprintf (fptr, "To: %s\n", pbul->desti);
	fprintf (fptr, "Subject: %s\n", pbul->titre);
	fprintf (fptr, "X-msgtype: %c\n", pbul->type);
	fprintf (fptr, "X-BID: %s\n\n", pbul->bid);

	*bbsf = '\0';

	if (*ptmes->bbsv)
		sprintf (bbsv, "@%s", pbul->bbsv);
	else
		*bbsv = '\0';

	sprintf (header, "XFWD #:%ld T:%c Fm:%s%s To:%s%s Id:%s\r",
			 pbul->numero, pbul->type, pbul->exped, bbsf,
			 pbul->desti, bbsv, pbul->bid);
	aff_bas (voiecur, W_SNDT, header, strlen (header));

	/* Header */
	make_header (pbul, header);
	entete_mess_fwd (pbul, header);
	/* Msg_header contient le header complet avec des \r */
	ptr = msg_header;
	while (*ptr)
	{
		if (*ptr == '\r')
			*ptr = '\n';
		++ptr;
	}
	fputs (msg_header, fptr);
	fflush (fptr);

	/* Ajouter le texte du message */
	mess_name (MESSDIR, pbul->numero, pvoie->sr_fic);
	if ((fptm = ouvre_mess (O_TEXT, pbul->numero, '\0')) == NULL)
		return (0);
	fflush (fptr);
	fflush (fptm);
	copy_fic (fileno (fptm), fileno (fptr), &last_char);
	fclose (fptm);

	if (last_char != '\n')
		fputc ('\n', fptr);

	fputs ("/EX\n", fptr);

	return (1);
}

static int merge_xfwd (XInfo * xinfo)
{
	int nb;
	char asc_file[128];
	bullist lbul;
	FILE *fptr;
	mess_noeud *lptr;

	fptr = fopen (xfwd_name (voiecur, asc_file), "wt");
	if (fptr == NULL)
		return (0);

	for (nb = 0; nb < xinfo->nb_bid; nb++)
	{
		if (!xinfo->ok_bid[nb])
			continue;

		/* Recuperer les infos du message */
		if ((lptr = findmess (xinfo->numero[nb])) == NULL)
			return (0);

		ouvre_dir ();
		read_dir (lptr->noenr, &lbul);
		ferme_dir ();

		if (!wr_xfwd_mess (fptr, &lbul))
		{
			fclose (fptr);
			return (0);
		}
	}
	fclose (fptr);
	return (1);
}

int not_in_xfwd_mess (long numero, int voie)
{
	int nb = 0;

	for (nb = 0; nb < svoie[voie]->Xfwd->nb_bid; nb++)
	{
		if (svoie[voie]->Xfwd->numero[nb] == numero)
			return (FALSE);
	}
	return (TRUE);
}

static int chk_xfwd (XInfo * xinfo)
{
	int nb;
	int nb_dupes = 0;
	int chk_dupes;
	int no_more = 0;
	long max_tfwd = 1024L * (long) p_port[no_port (voiecur)].maxbloc;
	long tail_tmess = 0L;

	nb = 0;
	pvoie->Xfwd->nb_bid = 0;
	chk_dupes = ((pvoie->typfwd & FWD_DUPES) == 0);

	while (nb < MAX_X)
	{

		if (tail_tmess > max_tfwd)
			break;

		for (;;)
		{
			if (!mess_suiv (voiecur))
			{
				if ((nb_dupes) && (chk_dupes))
				{
					pvoie->nb_egal -= nb_dupes;
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
			break;
		}

		if (no_more)
			break;

		if (std_header & 16)
		{
			if (*ptmes->bbsv == '\0')
			{
				strcpy (ptmes->bbsv, mypath);
			}
		}

		if (*(ptmes->bbsv) == '\0')
		{
			strcpy (ptmes->bbsv, pvoie->sta.indicatif.call);
		}

		if ((*(ptmes->exped)) && (*(ptmes->bbsv)) &&
			(*(ptmes->desti)) && (*(ptmes->bid)) &&
			(ISGRAPH (ptmes->type)))
		{
			xinfo->numero[nb] = ptmes->numero;
			n_cpy (12, xinfo->bid[nb], ptmes->bid);
			tail_tmess += ptmes->taille;
			++pvoie->Xfwd->nb_bid;
			++nb;
		}
		else
			mark_fwd (voiecur, 0);
	}
	pvoie->nb_egal -= nb_dupes;
	return (pvoie->Xfwd->nb_bid);
}
