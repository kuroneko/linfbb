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
 *    MODULE FORWARD.C
 */

#include <serv.h>


static int rcv_fwd (void);

static void snd_fwd (void);
static void snd_rev_fwd (void);

static char *check_call (char *ptr)
{
	char *scan = ptr;

	while (*scan > ' ')
		++scan;
	if (*scan == '\0')
		scan = ptr;
	else
	{
		while ((*scan) && (isspace (*scan)))
			++scan;
		if (isgraph(*scan) && !isalnum(*scan)) /* Pactor - ! ou % */
			++scan;
		if ((isdigit (*scan)) && (!isalnum (*(scan + 1))))
		{
			++scan;
			while ((*scan) && (!isalnum (*scan)))
				++scan;
		}
	}
	return (scan);
}


static int connect_ok (char *ptr)
{
	int i;
	int j;
	char *str;
	Forward *pfwd = pvoie->curfwd;
	static char *test[3] =
	{"Connected", "Failure", "Busy"};

	if (pfwd->no_con == 1)
		return (1);

	for (j = 0; j < 5; j++)
	{
		for (i = 0; i < 3; i++)
		{
			if (j == 4)
				str = test[i];
			else
			{
				str = pfwd->mesnode[j][i];
			}

			if ((*str) && (strstr (ptr, str)))
			{
				return (i + 1);
			}
		}
	}
	return (0);
}


static int snd_mess_fwd_rli (void)
{
	/*
	   * Retour =   0 : pas de suite
	   *                =   1 : il y a une suite
	   *                = -1 : Plus de message
	 */

	if (mess_suiv (voiecur))
	{
		libere_tread (voiecur);
		pvoie->enrcur = 0L;
		pvoie->t_read = (rd_list *) m_alloue (sizeof (rd_list));
		pvoie->t_read->suite = NULL;
		pvoie->t_read->nmess = ptmes->numero;
		pvoie->t_read->pmess = ptmes;
		return (snd_mess (voiecur, 2));
	}
	return (-1);
}


static void snd_fwd (void)
{
	char *ptr = indd;
	char *scan, *st;
	char s[256];
	int i, nobbs;
	long temps;
	Forward *pfwd = pvoie->curfwd;

	nobbs = pvoie->bbsfwd;

	switch (pvoie->niv3)
	{
	case 0:
		if (pfwd->no_con < 8)
		{
			switch (connect_ok (ptr))
			{
			case 0:
				break;
			case 1:
				scan = check_call (pfwd->con_lig[pfwd->no_con - 1]);
				pvoie->sta.indicatif.num = extind (scan, pvoie->sta.indicatif.call);
				if (*pfwd->con_lig[pfwd->no_con])
				{
					outln (pfwd->con_lig[pfwd->no_con], strlen (pfwd->con_lig[pfwd->no_con]));
					pfwd->no_con++;
				}
				else
				{
					ptr = s;
					pfwd->no_con = 8;
					strcpy (pvoie->dos_path, "\\");
					connexion (voiecur);	/* positionne ncur */
					nouveau (voiecur);
					if (pvoie->ctnc)
					{

						program_fwd (1, 1, &(pvoie->ctnc), voiecur);

					}
					if (*pfwd->con_lig[1])
					{
						*ptr++ = ' ';
						*ptr++ = '{';
						i = 0;
						while (1)
						{
							scan = pfwd->con_lig[i] + 2;
							while (ISGRAPH (*scan))
								*ptr++ = *scan++;
							if ((++i == 7) || (*pfwd->con_lig[i + 1] == '\0'))
							{
								*ptr++ = '}';
								*ptr++ = '\0';
								break;
							}
							else
								*ptr++ = ',';
						}
					}
					*ptr = '\0';
					status (voiecur);
					connect_log (voiecur, s);
#ifdef __FBBDOS__
					trait (0, "");
#endif
					if (*pfwd->txt_con)
						out (pfwd->txt_con, strlen (pfwd->txt_con));
				}
				libere_tread (voiecur);
				break;
			case 2:			/* Failure */
				pvoie->mode |= F_FOR;
				pvoie->deconnect = 6;
				break;
			case 3:			/* Busy */
				set_link (nobbs, 1);
				pvoie->nb_choix = 1;
				pvoie->mode |= F_FOR;
				pvoie->deconnect = 6;
				break;
			}
			break;
		}
		if (att_prompt ())
		{
			set_link (nobbs, 1);
			pvoie->nb_choix = 1;
			pvoie->maj_ok = 1;
			st = idnt_fwd ();
			if (pvoie->prot_fwd & FWD_FBB)
			{
				outs (st, strlen (st));
				pvoie->temp1 = -1;
				pvoie->temp2 = TRUE;
				maj_niv (pvoie->niv1, 5, 0);
				new_fwd ();
			}
			else if (pvoie->prot_fwd & FWD_XPRO)
			{
				outs (st, strlen (st));
				maj_niv (N_XFWD, 0, 9);
				xfwd ();
			}
			else if (FOR (pvoie->mode))
			{
				outs (st, strlen (st));
				if (pvoie->clock == 2)
					maj_niv (pvoie->niv1, pvoie->niv2, 4);
				else
					maj_niv (pvoie->niv1, pvoie->niv2, 5);
			}
			else
			{
				switch (snd_mess_fwd_rli ())
				{
				case 0:
					ch_niv3 (2);	/* Pas de suite */
					break;
				case 1:
					ch_niv3 (1);	/* Ya une suite */
					break;
				case -1:
					pvoie->deconnect = 6;	/* Plus de message */
					break;
				}
			}
		}
		break;
	case 1:
		if (snd_mess (voiecur, 2) == 0)
			ch_niv3 (2);
		break;
	case 2:
		if (att_prompt ())
		{
			/* Supression du message eventuel */
			sprintf (s, "F %ld V:%s [%ld]",
				   ptmes->numero, pvoie->sta.indicatif.call, ptmes->taille);
			fbb_log (voiecur, 'M', s);
			if (fin_envoi_fwd (voiecur))
				++stat_fwd;
			switch (snd_mess_fwd_rli ())
			{
			case 0:
				break;
			case 1:
				ch_niv3 (1);
			case -1:
				pvoie->deconnect = 6;
				break;
			}
		}
		break;
	case 4:
		if (att_prompt ())
		{
			strcpy (s, "C ");
			strcat (s, date_mbl (temps = time (NULL)));
			strcat (s, " ");
			strcat (s, heure_mbl (temps));
			outsln (s, strlen (s));
			maj_niv (pvoie->niv1, pvoie->niv2, 5);
		}
		break;
	case 5:
		if (att_prompt ())
		{
			/* Supression du message eventuel */
			if (fin_envoi_fwd (voiecur))
			{
				++stat_fwd;
				sprintf (s, "F %ld V:%s [%ld]",
				   ptmes->numero, pvoie->sta.indicatif.call, ptmes->taille);
				fbb_log (voiecur, 'M', s);
			}
			if (mess_suiv (voiecur))
			{
				entete_envoi_fwd (voiecur);
				maj_niv (pvoie->niv1, pvoie->niv2, 6);
			}
			else
			{
				if (pvoie->rev_mode)
				{
					texte (T_MBL + 42);
					maj_niv (pvoie->niv1, 3, 0);
				}
				else
					pvoie->deconnect = 6;
			}
		}
		break;
	case 6:
		switch (toupper (*indd))
		{
		case 'O':
			pvoie->t_read = (rd_list *) m_alloue (sizeof (rd_list));
			pvoie->t_read->suite = NULL;
			pvoie->t_read->nmess = ptmes->numero;
			pvoie->t_read->pmess = ptmes;
			pvoie->enrcur = 0L;
			if (snd_mess (voiecur, 1) == 0)
				ch_niv3 (5);
			else
				ch_niv3 (7);
			break;
		case 'N':
			ch_niv3 (5);
			sprintf (s, "N B:%s V:%s", ptmes->bid, pvoie->sta.indicatif.call);
			fbb_log (voiecur, 'M', s);
			break;
		case 'R':
			ch_niv3 (5);
			sprintf (s, "R B:%s V:%s", ptmes->bid, pvoie->sta.indicatif.call);
			fbb_log (voiecur, 'M', s);
			break;
		case 'L':
			ch_niv3 (8);
			pvoie->mess_egal[(int)pvoie->nb_egal] = ptmes->numero;
			++pvoie->nb_egal;
			sprintf (s, "L B:%s V:%s", ptmes->bid, pvoie->sta.indicatif.call);
			fbb_log (voiecur, 'M', s);
			break;
		default:
			err_new_fwd (0, 9);
			/* pvoie->deconnect = 3; */
			pvoie->deconnect = 6;
			break;
		}
		break;
	case 7:
		if (snd_mess (voiecur, 1) == 0)
			ch_niv3 (5);
		break;
	case 8:
		if (att_prompt ())
		{
			/* message retarde */
			if (mess_suiv (voiecur))
			{
				entete_envoi_fwd (voiecur);
				maj_niv (pvoie->niv1, pvoie->niv2, 6);
			}
			else
			{
				if (pvoie->rev_mode)
				{
					texte (T_MBL + 42);
					maj_niv (pvoie->niv1, 3, 0);
				}
				else
					pvoie->deconnect = 6;
			}
		}
		break;
	}
}


/*
 * REVERSE - FORWARDING
 */

static void snd_rev_fwd (void)
{
	int test;
	int res;

	switch (pvoie->niv3)
	{
	case 0:
		if (toupper (*indd) == 'S')
		{
			if (toupper (indd[1]) == 'X')
			{
				/* Forward xfwd */
				maj_niv (N_XFWD, 1, 0);
				xfwd ();
				return;
			}

			++indd;
			if (lit_com_fwd () == 0)
			{
				texte (T_MBL + 44);
				texte (T_MBL + 42);
				return;
			}

			res = deja_recu (ptmes, 1, &test);
			mark_reverse_bid (ptmes, 1, &test);

			switch (res)
			{
			case 0:
				texte (T_MBL + 46);
				maj_niv (pvoie->niv1, pvoie->niv2, 1);
				break;
			case 4:
				if (pvoie->mbl_ext)
				{
					outln ("REJECT", 6);
					texte (T_MBL + 42);
					return;
				}
			case 2:
				if (pvoie->mbl_ext)
				{
					outln ("LATER", 5);
					texte (T_MBL + 42);
					return;
				}
			case 1:
				texte (T_MBL + 45);
				texte (T_MBL + 42);
				/* supprimer de la liste des propositions */
				return;
			}
		}
		else					/* if (att_prompt()) */
			pvoie->deconnect = 6;
		break;
	case 1:
		if (*indd)
		{
			rcv_titre ();
			maj_niv (pvoie->niv1, pvoie->niv2, 2);
			if (*indd)
				get_mess_fwd ('<', indd, nb_trait, 1);
		}
		break;
	case 2:
		get_mess_fwd ('<', data, nb_trait, 1);
		break;
	default:
		fbb_error (ERR_NIVEAU, "SND-REV-FWD", pvoie->niv3);
		break;
	}
}


void rcv_rev_fwd (void)
{
	char s[256];

	switch (pvoie->niv3)
	{
	case 0:
		appel_rev_fwd (TRUE);
		if (mess_suiv (voiecur))
		{
			libere_tread (voiecur);
			entete_envoi_fwd (voiecur);
			ch_niv3 (1);
		}
		else
		{
			texte (T_MBL + 47);
			retour_menu (N_MBL);
			deconnexion (voiecur, 1);
		}
		break;
	case 1:
		switch (toupper (*indd))
		{
		case 'O':
			pvoie->t_read = (rd_list *) m_alloue (sizeof (rd_list));
			pvoie->t_read->suite = NULL;
			pvoie->t_read->nmess = ptmes->numero;
			pvoie->t_read->pmess = ptmes;
			pvoie->enrcur = 0L;
			if (snd_mess (voiecur, 1) == 0)
				ch_niv3 (2);
			else
				ch_niv3 (3);
			break;
		case 'N':
			ch_niv3 (2);
			sprintf (s, "N B:%s V:%s", ptmes->bid, pvoie->sta.indicatif.call);
			fbb_log (voiecur, 'M', s);
			break;
		case 'L':
			ch_niv3 (4);
			pvoie->mess_egal[(int)pvoie->nb_egal] = ptmes->numero;
			++pvoie->nb_egal;
			sprintf (s, "L B:%s V:%s", ptmes->bid, pvoie->sta.indicatif.call);
			fbb_log (voiecur, 'M', s);
			break;
		case 'R':
			ch_niv3 (2);
			sprintf (s, "R B:%s V:%s", ptmes->bid, pvoie->sta.indicatif.call);
			fbb_log (voiecur, 'M', s);
			break;
		default:
			err_new_fwd (0, 9);
			pvoie->deconnect = 6;
			/* retour_menu(N_MBL) ; */
			break;
		}
		break;
	case 2:
		if (att_prompt ())
		{						/* Le message a ete envoye */
			fin_envoi_fwd (voiecur);
			sprintf (s, "> %ld V:%s [%ld]",
				   ptmes->numero, pvoie->sta.indicatif.call, ptmes->taille);
			fbb_log (voiecur, 'M', s);
			++stat_fwd;
			if (mess_suiv (voiecur))
			{
				entete_envoi_fwd (voiecur);
				ch_niv3 (1);
			}
			else
			{
				texte (T_MBL + 47);
				retour_menu (N_MBL);
				deconnexion (voiecur, 1);
			}
		}
		break;
	case 3:
		if (snd_mess (voiecur, 1) == 0)
			ch_niv3 (2);
		break;
	case 4:
		if (att_prompt ())
		{						/* Le message a ete retarde */
			if (mess_suiv (voiecur))
			{
				entete_envoi_fwd (voiecur);
				ch_niv3 (1);
			}
			else
			{
				texte (T_MBL + 47);
				retour_menu (N_MBL);
				deconnexion (voiecur, 1);
			}
		}
		break;
	}
}


static int sysop_mess (void)
{
	return ((*ptmes->bbsv == '\0') &&
			((strcmp (ptmes->desti, "SYSOP") == 0) || (strcmp (ptmes->desti, admin) == 0)));
}


static int rcv_fwd (void)
{
	int test;
	int len;
	int res;
	char *ptr;
	char clog = (pvoie->mode & F_FOR) ? 'W' : 'S';

	switch (pvoie->niv3)
	{
	case 0:
		if (FOR (pvoie->mode))
		{
			if (lit_com_fwd () == 0)
			{
				libere (voiecur);
				texte (T_MBL + 44);
				retour_menu (N_MBL);
				return (0);
			}
			res = deja_recu (ptmes, 1, &test);
			mark_reverse_bid (ptmes, 1, &test);

			switch (res)
			{
			case 0:
				texte (T_MBL + 46);
				maj_niv (pvoie->niv1, pvoie->niv2, 3);
				break;
			case 4:
				if (pvoie->mbl_ext)
				{
					outln ("REJECT", 6);
					libere (voiecur);
					retour_menu (N_MBL);
					return (0);
				}
			case 2:
				if (pvoie->mbl_ext)
				{
					outln ("LATER", 5);
					libere (voiecur);
					retour_menu (N_MBL);
					return (0);
				}
			case 1:
				texte (T_MBL + 45);
				libere (voiecur);
				retour_menu (N_MBL);
				/* supprimer de la liste des propositions */
				return (0);
			}
		}
		else
		{
			len = 0;
			ptr = indd;
			while (ISGRAPH (*ptr))
			{
				++len;
				++ptr;
			}
			if (len > 1)
				return (1);
			if (lit_com_fwd () == 0)
			{
				retour_mbl ();
				return (0);
			}
			if (sysop_mess ())
			{
				if (pvoie->read_only)
					strcpy (ptmes->desti, admin);
			}
			else if (read_only ())
			{
				outln ("Message to SYSOP only !", 23);
				retour_mbl ();
				return (0);
			}
			if (deja_recu (ptmes, 1, &test) == 1)
			{
				texte (T_MBL + 21);
				retour_mbl ();
				return (0);
			}
			maj_niv (15, 0, 1);
			texte (T_MBL + 5);
		}
		break;
	case 1:
		if (!rcv_titre ())
		{
			retour_mbl ();
		}
		else
		{
			maj_niv (15, 0, 2);
			texte (T_MBL + 6);
			if ((sed) && (EditorOff) && (voiecur == CONSOLE))
			{
				aff_etat ('E');
#ifdef __LINUX__
				if (daemon_mode)
					editor_request = 1;
#endif
				reply = 3;
				send_buf (voiecur);
				pvoie->enrcur = 0L;
#ifdef __WINDOWS__
				maj_niv (N_MBL, 3, 0);
				if (win_edit () == 5)
					end_win_edit ();
#endif
#ifdef __LINUX__
				if (!daemon_mode)
				{
					maj_niv (N_MBL, 3, 0);
					if (xfbb_edit () == 5)
						end_xfbb_edit ();
				}
#endif
#ifdef __FBBDOS__
				maj_niv (N_MBL, 3, 0);
				if (mini_edit () == 5)
					end_mini_edit ();
#endif
			}
		}
		break;
	case 2:
		get_mess_fwd (clog, indd, nb_trait, 0);
		break;
	case 3:
		if (!rcv_titre ())
		{
			retour_mbl ();
		}
		else
		{
			maj_niv (pvoie->niv1, pvoie->niv2, 4);
			if (*indd)
			{
				get_mess_fwd (clog, indd, nb_trait, 0);
			}
		}
		break;
	case 4:
		get_mess_fwd (clog, indd, nb_trait, 0);
		break;
	default:
		fbb_error (ERR_NIVEAU, "RCV-FWD", pvoie->niv3);
		break;
	}
	return (0);
}


int fwd (void)
{
	int error = 0;

	df ("fwd", 0);

	switch (pvoie->niv2)
	{
	case 0:
		error = rcv_fwd ();
		break;
	case 1:
		rcv_rev_fwd ();
		break;
	case 2:
		snd_fwd ();
		break;
	case 3:
		snd_rev_fwd ();
		break;
	case 5:
		new_fwd ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "FWD", pvoie->niv2);
		break;
	}
	ff ();
	return (error);
}
