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
 *    MODULE FORWARDING OVERLAY 3
 */

#include <serv.h>


static void seek_fwd (unsigned pos)
{
	if (EMS_FWD_OK ())
		seek_exms_string (FORWARD, (long) pos);
	else
		fwd_scan = fwd_file + pos;
}


void rewind_fwd (void)
{
	seek_fwd (0);
}


static unsigned tell_fwd (void)
{
	if (EMS_FWD_OK ())
		return (unsigned) (tell_exms_string (FORWARD));
	else
		return (unsigned) (fwd_scan - fwd_file);
}


void param_tnc (int type, typ_pfwd ** ptnc, char *cmde)
{
	typ_pfwd *p;

	if (*ptnc == NULL)
	{
		*ptnc = (typ_pfwd *) m_alloue (sizeof (typ_pfwd));
		p = *ptnc;
	}
	else
	{
		p = *ptnc;
		while (p->suiv)
			p = p->suiv;
		p->suiv = (typ_pfwd *) m_alloue (sizeof (typ_pfwd));
		p = p->suiv;
	}
	p->suiv = NULL;
	n_cpy (78, p->chaine, cmde);
	p->type = type;
}


void libere_tnc (typ_pfwd ** ptnc)
{
	typ_pfwd *pcurr, *ptemp;

	pcurr = *ptnc;
	while (pcurr)
	{
		ptemp = pcurr;
		pcurr = pcurr->suiv;
		m_libere (ptemp, sizeof (typ_pfwd));
	}
	*ptnc = NULL;
}



int n_bbs (char *bbs)
{
	char *ptr = bbs_ptr;
	int nb = 1;

	while (nb <= NBBBS)
	{
		if (strncmp (ptr, bbs, 6) == 0)
			return (nb);
		++nb;
		ptr += 7;
	}

	return (0);

}

int num_bbs (char *bbs)
{
	int nb = n_bbs (strupr (bbs));
	char bbs_nom[10];
	char s[80];

	if (nb == 0 && !svoie[CONSOLE]->sta.connect)
	{
		strn_cpy (6, bbs_nom, bbs);

#ifdef ENGLISH
		sprintf (s, "Unknown BBS %s in BBS.SYS file ", bbs_nom);
#else
		sprintf (s, " BBS %s inconnue dans BBS.SYS  ", bbs_nom);
#endif
		win_message (2, s);
	}
	return (nb);
}


int get_link (int no_bbs)
{
	if (no_bbs)
		return ((int) *(bbs_ptr + 6 + 7 * (no_bbs - 1)));
	else
		return (1);
}


void set_link (int no_bbs, int choix)
{
	if (no_bbs)
		*(bbs_ptr + 6 + 7 * (no_bbs - 1)) = (char) choix;
}


static int fwd_libre (int nobbs, int noport)
{
	int test;
	Forward *pfwd = p_port[noport].listfwd;

	test = already_forw (p_port[noport].fwd, nobbs);

	if (test == 0)
	{
		while (pfwd)
		{
			if (nobbs == pfwd->no_bbs)
				return (0);
			pfwd = pfwd->suite;
		}
		return (1);
	}
	else
		return (0);
}


int what_port (int n_bbs)
{
	int temp;
	int trouve = 0;
	int fin = 0;
	int nobbs = 0;
	int cptif = 0;
	int fwdlig = 0;
	int dans_bloc = 0;
	int nbchoix = 1;
	long h_time = time (NULL);
	int port = -1;
	char combuf[80];
	char *pcom;

	/*  init_bbs() ; */

	rewind_fwd ();

	while (!fin)
	{
		if (fwd_get (combuf) == 0)
		{
			fin = TRUE;
			break;
		}
		pcom = combuf;
		++fwdlig;
		switch (*pcom++)
		{
		case 'E':				/* ENDIF */
			--cptif;
			break;
		case 'I':
			++cptif;
			if (tst_fwd (pcom, nobbs, h_time, 0, &nbchoix, 0, -1) == FALSE)
			{
				temp = cptif - 1;
				while (cptif != temp)
				{
					if (fwd_get (combuf) == 0)
					{
						fin = TRUE;
						break;
					}
					pcom = combuf;
					++fwdlig;
					switch (*pcom++)
					{
					case 'I':
						++cptif;
						break;
					case 'E':
						--cptif;
						break;
					case '@':
						if (cptif == (temp + 1))
							++temp;		/* else */
						break;
					default:
						break;
					}
				}
			}
			break;
		case '@':				/* ELSE */
			temp = cptif - 1;
			while (cptif != temp)
			{
				if (fwd_get (combuf) == 0)
				{
					fin = TRUE;
					break;
				}
				pcom = combuf;
				++fwdlig;
				switch (*pcom++)
				{
				case 'I':
					++cptif;
					break;
				case 'E':
					--cptif;
					break;
				default:
					break;
				}
			}
			break;
		case 'P':				/* Indication du port de forward */
			if (dans_bloc)
			{
				*pcom = toupper (*pcom);
				port = *pcom - '@';
			}
			break;
		case 'A':
			dans_bloc = port = 1;
			nobbs = num_bbs (pcom);
			trouve = (nobbs == n_bbs);
			break;
		case '-':
			nobbs = dans_bloc = 0;
			if (trouve)
				fin = 1;
			break;
		}
	}

	return (port);
}


void appel_fwd (Forward * pfwd, int noport)
{
	uchar max, old, typ;
	char nombbs[80];
	char combuf[80];
	char *ptr;
	char *pcom;
	char mesnoeud[4][3][20];
	int mesnum;
	int i, nobbs, port, voie = 0, temp, choix, canal;
	int reverse, fin, retour, con_fwd, dde_clk;
	int trouve, dans_bloc, fbb_mode, t_o, nbchoix;
	int reverse_mode, val, dans_test_port;
	int data_mode = 1;
	int nb_prompt = 0;
	char s[80];

#ifdef __FBBDOS__
	fen *fen_ptr;

#endif

	typ_pfwd *ptnc;
	typ_pfwd *ctnc;
	atfwd *nbmess;
	long h_time = time (NULL);

	char p_route[NB_P_ROUTE][7];

	df ("appel_fwd", 3);

	ctnc = NULL;

	pfwd->lastpos = pfwd->fwdpos;
	do
	{
		for (i = 0; i < NB_P_ROUTE; i++)
			*p_route[i] = '\0';
		seek_fwd (pfwd->fwdpos);
		/* fwd_scan = fwd_file + pfwd->fwdpos; */
		ptnc = NULL;
		nobbs = 0;
		dans_test_port = 0;
		port = 1;
		reverse = pfwd->reverse;
		fin = FALSE;
		retour = 0;
		con_fwd = FALSE;
		trouve = 0;
		dans_bloc = 0;
		fbb_mode = -1;
		dde_clk = 0; 
/*		t_o = 0; */
		t_o = time_b;
		nbchoix = 1;
		while (!trouve)
		{
			if (fwd_get (combuf) == 0)
			{
				fin = TRUE;
				retour = 0;
				pfwd->reverse = 0;
				break;
			}
			pcom = combuf;
			++pfwd->fwdlig;
			switch (*pcom++)
			{
			case 'E':			/* ENDIF */
				if (dans_test_port == pfwd->cptif)
					dans_test_port = 0;
				--pfwd->cptif;
				break;
			case '@':			/* ELSE */
				temp = pfwd->cptif - 1;
				while (pfwd->cptif != temp)
				{
					if (fwd_get (combuf) == 0)
					{
						fin = TRUE;
						break;
					}
					pcom = combuf;
					++pfwd->fwdlig;
					switch (*pcom++)
					{
					case 'I':
						++pfwd->cptif;
						break;
					case 'E':
						--pfwd->cptif;
						break;
					default:
						break;
					}
				}
				break;
			case 'I':
				++pfwd->cptif;
				if ((val = tst_fwd (pcom, nobbs, h_time, port, NULL, pfwd->reverse, noport)) == FALSE)
				{
					temp = pfwd->cptif - 1;
					while (pfwd->cptif != temp)
					{
						if (fwd_get (combuf) == 0)
						{
							fin = TRUE;
							break;
						}
						pcom = combuf;
						++pfwd->fwdlig;
						switch (*pcom++)
						{
						case 'I':
							++pfwd->cptif;
							break;
						case 'E':
							--pfwd->cptif;
							break;
						case '@':
							if (pfwd->cptif == (temp + 1))
								++temp;
							break;
						default:
							break;
						}
					}
				}
				else if (val == 'P')
				{
					dans_test_port = pfwd->cptif;
				}
				break;
			case 'D':
				if ((!dans_bloc) && ((port == noport) || (dans_test_port)))
				{
					if (strncmpi (pcom, "PTCTRX", 6) == 0)
					{
						ptctrx (port, pcom);
					}
					else
					{
#ifdef __WINDOWS__

						if (call_dll (pcom, NO_REPORT_MODE, NULL, 0, NULL) == -1)
							call_nbdos (&pcom, 1, NO_REPORT_MODE, NULL, NULL, NULL);
#endif
#ifdef __FBBDOS__
						send_dos (1, pcom, NULL);
#endif
#ifdef __LINUX__
						call_nbdos (&pcom, 1, NO_REPORT_MODE, NULL, TOOLDIR, NULL);
#endif
					}
				}
				break;
			case 'L':
				if ((!dans_bloc) && ((port == noport) || (dans_test_port)))
					program_tnc (no_voie (noport, 1), pcom);
				break;
			case 'X':
				if ((!dans_bloc) && ((port == noport) || (dans_test_port)))
				{
					if (strncmpi (pcom, "PTCTRX", 6) == 0)
					{
						ptctrx (port, pcom);
					}
					else
					{
#ifdef __WINDOWS__

						if (call_dll (pcom, NO_REPORT_MODE, NULL, 0, NULL) == -1)
							call_nbdos (&pcom, 1, NO_REPORT_MODE, NULL, NULL, NULL);
#endif
#ifdef __FBBDOS__
						send_dos (2, pcom, NULL);
#endif
#ifdef __LINUX__
						call_nbdos (&pcom, 1, NO_REPORT_MODE, NULL, TOOLDIR, NULL);
#endif
					}
				}
				break;
			case 'P':			/* Indication du port de forward */
				*pcom = toupper (*pcom);
				port = *pcom - '@';
				break;
			case 'A':
				dans_bloc = 1;
				strcpy (nombbs, pcom);
				nobbs = num_bbs (nombbs);
				if ((chercher_voie (nombbs) == -1) &&
					(!pfwd->fin_fwd) &&
					(fwd_libre (nobbs, noport)) &&
					((*pfwd->fwdbbs == '\0') || (strcmp (pfwd->fwdbbs, nombbs) == 0)))
				{
					trouve = 1;
				}
				break;
			case '-':
				nobbs = dans_bloc = 0;
				break;
			}
		}

		pfwd->con_lig[0][0] = '\0';
		pfwd->txt_con[0] = '\0';
		for (mesnum = 0; mesnum < 4; mesnum++)
			for (i = 0; i < 3; i++)
				*mesnoeud[mesnum][i] = '\0';
		mesnum = 0;
		max = 0xff;
		old = 0;
		typ = '\0';
		port = 1;
		canal = 0;
		pfwd->no_con = 0;
		for (i = 0; i < 8; i++)
			*pfwd->con_lig[i] = '\0';

		if (!fin)
		{
			/* Valide le flag forward de la bbs */
			set_bit_fwd (p_port[noport].fwd, nobbs);
			reverse_mode = 1;
			while ((!fin) && (fwd_get (combuf)))
			{
				pcom = combuf;
				++pfwd->fwdlig;
				switch (*pcom++)
				{
				case '!':		/* Not */
					break;
				case 'B':		/* Nom de la BBS destinataire */
					break;
				case 'C':		/* sauvegarde du texte de connnexion */
					if (pfwd->no_con < 8)
					{
						n_cpy (79, pfwd->con_lig[pfwd->no_con], pcom);
						pfwd->no_con++;
					}
					break;
				case 'd':		/* commande DOS */
					param_tnc (1, &ctnc, pcom);
					break;
				case 'D':
					param_tnc (1, &ptnc, pcom);
					break;
				case 'E':		/* ENDIF */
					if (dans_test_port == pfwd->cptif)
						dans_test_port = 0;
					--pfwd->cptif;
					break;
				case 'F':		/* OM associe a la BBS */
					break;
				case 'G':		/* indicateur de groupe */
					break;
				case 'H':		/* indicateur de hierarchie */
					break;
				case 'I':		/* IF */
					++pfwd->cptif;
					if ((val = tst_fwd (pcom, nobbs, h_time, port, &nbchoix, pfwd->reverse, noport)) == FALSE)
					{
						temp = pfwd->cptif - 1;
						while (pfwd->cptif != temp)
						{
							if (fwd_get (combuf) == 0)
							{
								fin = TRUE;
								break;
							}
							pcom = combuf;
							++pfwd->fwdlig;
							switch (*pcom++)
							{
							case 'I':
								++pfwd->cptif;
								break;
							case 'E':
								--pfwd->cptif;
								break;
							case '@':
								if (pfwd->cptif == (temp + 1))
/* code folded from here */
									++temp;
/* unfolding */
								break;
							default:
								break;
							}
						}
					}
					else if (val == 'P')
					{
						dans_test_port = pfwd->cptif;
					}
					break;
				case 'J':		/* Selection des messages "data" */
					data_mode = atoi (pcom);
					break;
				case 'K':		/* Canal preselectionne */
					canal = atoi (pcom);
					break;
				case 'l':		/* Ligne de commande TNC (connection) */
					param_tnc (0, &ctnc, pcom);
					break;
				case 'L':		/* Ligne de commande TNC */
					param_tnc (0, &ptnc, pcom);
					break;
				case 'M':		/* Pas de reverse */
					reverse_mode = 0;
					break;
				case 'N':		/* Pas de forward FBB */
					/* 1 = FBB */
					/* 2 = FBB+BIN */
					/* 4 = FBB+BIN+RESUME */
					/* 8 = XFWD */
					fbb_mode = (atoi (pcom) << 1) + 1;
					if (fbb_mode & 8)
						fbb_mode |= 7;
					else if (fbb_mode & 4)
						fbb_mode |= 3;
					break;
				case 'O':		/* Time-out en forward */
					t_o = atoi (pcom) * 60;
					break;
				case 'P':		/* Indication du port de forward */
					*pcom = toupper (*pcom);
					port = *pcom - '@';
					break;
				case 'Q':		/* Interdiction Reverse */
					break;
				case 'R':		/* reverse forwarding */
					reverse = TRUE;
					break;
				case 'S':		/* critere de selection des messages Noeuds */
					if (mesnum == 4)
						break;
					sscanf (pcom, "%s %s %s",
							mesnoeud[mesnum][0],
							mesnoeud[mesnum][1],
							mesnoeud[mesnum][2]);
					++mesnum;
					break;
				case 'T':		/* limite la taille du fichier forwarde */
					while (*pcom)
					{
						if (toupper (*pcom) == 'M')
						{
							++pcom;
							max = (uchar) atoi (pcom);
							while (isdigit (*pcom))
								++pcom;
						}
						else if (toupper (*pcom) == 'O')
						{
							++pcom;
							old = (uchar) atoi (pcom);
							while (isdigit (*pcom))
								++pcom;
						}
						else if (toupper (*pcom) == 'P')
						{
							typ |= FWD_PRIV;
							++pcom;
						}
						else if (toupper (*pcom) == 'S')
						{
							typ |= FWD_SMALL;
							++pcom;
						}
						else if (toupper (*pcom) == 'D')
						{
							typ |= FWD_DUPES;
							++pcom;
						}
						else if (isdigit (*pcom))
						{
							max = (uchar) atoi (pcom);
							while (isdigit (*pcom))
								++pcom;
						}
						else
							++pcom;
					}
					break;
				case 'U':		/* Routage prioritaire */
					if ((ptr = strtok (pcom, " \t")) != NULL)
					{
						strn_cpy (6, p_route[0], ptr);
						i = 1;
						while ((ptr = strtok (NULL, " \t")) != NULL)
						{
							strn_cpy (6, p_route[i], ptr);
							++i;
							if (i == NB_P_ROUTE)
								break;
						}
					}
					break;
				case 'V':		/* texte envoye en connexion */
					strcpy (pfwd->txt_con, pcom);
					break;
				case 'W':
					if (isdigit (*pcom))
						nb_prompt = atoi (pcom);
					else
						nb_prompt = 1;
					break;
				case 'x':		/* commande DOS */
					param_tnc (2, &ctnc, pcom);
					break;
				case 'X':		/* commande DOS */
					param_tnc (2, &ptnc, pcom);
					break;
				case 'Y':		/* Mise a l'heure PMS */
					dde_clk = 1;
					break;
				case 'Z':		/* NTS */
					break;
				case '@':		/* ELSE */
					temp = pfwd->cptif - 1;
					while (pfwd->cptif != temp)
					{
						if (fwd_get (combuf) == 0)
						{
							fin = TRUE;
							break;
						}
						pcom = combuf;
						++pfwd->fwdlig;
						switch (*pcom++)
						{
						case 'I':
							++pfwd->cptif;
							break;
						case 'E':
							--pfwd->cptif;
							break;
						default:
							break;
						}
					}
					break;
				case '*':
				case '#':		/*   ligne commentaire */
					break;
				case '-':		/* fin de bloc - passage BBS a connecter suivante */
/************ ???????????? ***************/
					/* pfwd->fwdpos = (unsigned) (fwd_scan - fwd_file) ; */
					pfwd->fwdpos = tell_fwd ();
					if ((noport == port) &&
						((voie = ch_voie (port, canal)) >= 0) &&
						((((nbmess = attend_fwd (nobbs, max, old, typ, data_mode)) != 0L) ||
						  (reverse))))
					{
						deb_io ();
						sprintf (s, "Forward %d %s", nobbs, nombbs);
						
/*						fprintf (stderr, "\nForward %d %s\n", nobbs, nombbs);*/
#ifdef __FBBDOS__
						fen_ptr = open_win (45, 5, 68, 18, INIT, s);
#endif
#ifdef __LINUX__
#ifdef ENGLISH
						if (max < 0xff)
							cprintf ("Max size : %u Kb\n", max);
#else
/*						if (max < 0xff)
							printf ("Taille max %u Kb\n", max);*/
#endif

#ifdef ENGLISH
						if (typ & FWD_SMALL)
							cprintf ("Smallest first  \n");
#else
						if (typ & FWD_SMALL)
							cprintf ("Envoi par taille\n");
#endif
#endif
/*						if (nbmess)
						{
#ifdef __FBBDOS__
#ifdef ENGLISH
							cprintf ("\nPrivate   : %d\r\n", nbmess->nbpriv);
							cprintf ("Bulletins : %d\r\n", nbmess->nbbul);
#else
							cprintf ("\nPriv‚s    : %d\r\n", nbmess->nbpriv);
							cprintf ("Bulletins : %d\r\n", nbmess->nbbul);
#endif
#endif
#ifdef __LINUX__
#ifdef ENGLISH
							cprintf ("\nPrivate   : %d\n", nbmess->nbpriv);
							cprintf ("Bulletins : %d\n", nbmess->nbbul);
#else
							cprintf ("\nPriv‚s    : %d\n", nbmess->nbpriv);
							cprintf ("Bulletins : %d\n", nbmess->nbbul);
#endif
#endif
						}
						else
#ifdef __FBBDOS__
							cprintf ("\nReverse\r\n");
#endif
#ifdef __LINUX__
							cprintf ("\nReverse\n");
#endif
*/						/* il y a des messages a forwarder - connexion */
						if (*pfwd->con_lig[0])
						{
#ifdef __FBBDOS__
/*							cprintf ("Link BBS #%d\r\n", get_link (nobbs));*/
#endif
#ifdef __LINUX__
/*							cprintf ("Link BBS #%d\n", get_link (nobbs));*/
#endif
							program_fwd (1, 1, &ptnc, voie);
							svoie[voie]->ctnc = ctnc;
							ctnc = NULL;
							svoie[voie]->nb_prompt = nb_prompt;
							svoie[voie]->nb_choix = nbchoix;
							svoie[voie]->cur_choix = (uchar) get_link (nobbs);
							svoie[voie]->curfwd = pfwd;
							svoie[voie]->maxfwd = max;
							svoie[voie]->oldfwd = old;
							svoie[voie]->typfwd = typ;
							svoie[voie]->bbsfwd = (uchar) nobbs;
							svoie[voie]->rev_mode = reverse_mode;
							svoie[voie]->data_mode = data_mode;
							if (dde_clk)
								svoie[voie]->clock = 1;
							for (mesnum = 0; mesnum < 4; mesnum++)
								for (i = 0; i < 3; i++)
									n_cpy (19, pfwd->mesnode[mesnum][i],
										   mesnoeud[mesnum][i]);

							pvoie->sid = 0;

							if (fbb_mode == -1)
							{
								svoie[voie]->fbb = bin_fwd;
								svoie[voie]->prot_fwd = prot_fwd;
							}
							else
							{
								if (fbb_mode & FWD_BIN)
									svoie[voie]->fbb = bin_fwd;
								svoie[voie]->prot_fwd = prot_fwd & fbb_mode;
							}

						/*	if (t_o == time_n) */
						/*	if (t_o == time_b)
								++t_o; */
						/*	if (t_o) { */
								svoie[voie]->timout = t_o;
								/* init_timout (voie);  * ? F6BVP */

#ifdef __FBBDOS__
/*								cprintf ("Time out %d mn\r\n", t_o / 60);*/
#endif
#ifdef __LINUX__
/*								cprintf ("Time out %d mn\n", t_o / 60);*/
#endif
								con_fwd = TRUE;
								retour = voie;
						}
						else
						{
							choix = get_link (nobbs);
							if (choix < (int) nbchoix)
							{
								++choix;
								set_link (nobbs, choix);
								clr_bit_fwd (p_port[noport].fwd, nobbs);
								pfwd->fwdpos = pfwd->lastpos;
							}
							else
								set_link (nobbs, 1);
							retour = -1;
						}
						for (i = 0; i < NB_P_ROUTE; i++)
						{
							if (*p_route[i])
#ifdef __FBBDOS__
								cprintf ("Prio : %s\r\n", p_route[i]);
#endif
#ifdef __LINUX__
								cprintf ("Prio : %s\n", p_route[i]);
#endif
							strcpy (svoie[voie]->p_route[i], p_route[i]);
						}
#ifdef __FBBDOS__
						if (pfwd->reverse)
						{
							sleep_ (1);
						}
						close_win (fen_ptr);
#endif
						fin_io ();
					}
					else
					{
						retour = -1;
					}
					fin = TRUE;
					break;
				}
			}
		}
		if (con_fwd)
		{
			pfwd->no_bbs = nobbs;
			if (noport)
			{
				if (connect_fwd (voie, pfwd) != 0)
				{
					dec_voie (voie);
					retour = -1;
				}
				else
				{
					pfwd->no_con = 1;
				}
			}
			else
			{
				if (!mail_out (pfwd->con_lig[0]))
					retour = -1;
			}
		}
		libere_tnc (&ptnc);
		if (ctnc)
			libere_tnc (&ctnc);
	}
	while (retour == -1);
	if (!retour)
	{
		*pfwd->fwdbbs = '\0';
	}
	pfwd->forward = retour;
	aff_forward ();
	ff ();
}


int appel_rev_fwd (int affiche)
{
	int i;
	int temp, nobbs, retour = FALSE, fin = FALSE, ligne = 0, cptif = 0;
	int reverse = 1;
	char nombbs[80];
	char combuf[80];
	char *pcom;
	char *ptr;

#ifdef __FBBDOS__
	fen *fen_ptr;

#endif
	typ_pfwd *ptnc;
	atfwd *nbmess;
	long h_time = time (NULL);

	df ("appel_rev_fwd", 1);
	ptnc = NULL;
	pvoie->data_mode = 1;

	/*  init_bbs() ; */

	pvoie->bbsfwd = 0;

	rewind_fwd ();

	while (TRUE)
	{
		if (fwd_get (combuf) == 0)
		{
			ff ();
			return (FALSE);
		}
		pcom = combuf;
		++ligne;
		if (*pcom++ == 'A')
		{
			port = 1;
			strcpy (nombbs, pcom);
			nobbs = num_bbs (nombbs);
			if (indcmp (nombbs, pvoie->sta.indicatif.call))
			{
				break;
			}
		}
	}
	for (i = 0; i < NB_P_ROUTE; i++)
		*(pvoie->p_route[i]) = '\0';
	pvoie->maxfwd = 0xff;
	pvoie->oldfwd = 0;
	pvoie->typfwd = 0;
	pvoie->bbsfwd = (uchar) nobbs;

	deb_io ();
#ifdef __FBBDOS__
	if (affiche == 1)
	{
		sprintf (s, "Reverse %d %s", nobbs, nombbs);
		fen_ptr = open_win (45, 5, 68, 18, INIT, s);
	}
#endif

	while ((!fin) && (fwd_get (combuf)))
	{
		pcom = combuf;
		++ligne;
		switch (*pcom++)
		{
		case '!':				/* Not */
			break;
		case 'B':				/* recuperer le nom de la BBS destinataire */
			break;
		case 'C':				/* sauvegarde du texte de connexion */
			break;
		case 'l':				/* Ligne de commande TNC */
			break;
		case 'O':
			pvoie->timout = atoi (pcom) * 60;
/*			if (affiche == 1)
				cprintf ("Time out voie %d = %d mn\r\n", i, pvoie->timout / 60);*/
			break;
		case 'd':				/* commande DOS */
		case 'x':				/* commande DOS */
		case 'P':				/* Indication du port de forward */
		case 'D':				/* Commande DOS */
		case 'K':				/* Canal preselectionne */
		case 'M':				/* Pas de reverse */
		case 'N':				/* Mode FBB inactif */
		case 'R':				/* Reverse */
		case 'S':				/* Chaines identification */
		case 'V':				/* Texte connexion */
		case 'X':				/* Commande DOS */
		case 'Y':				/* Mise a l'heure */
		case 'Z':				/* NTS */
			break;
		case 'Q':				/* Interdit le reverse */
			reverse = 0;
			break;
		case 'E':				/* ENDIF */
			--cptif;
			break;
		case 'F':				/* chercher les messages de l'OM associe a la BBS */
			break;
		case 'G':				/* Indicateur de groupe */
			break;
		case 'H':				/* indicateur de hierarchie */
			break;
		case 'I':				/* IF */
			++cptif;
			if (tst_fwd (pcom, nobbs, h_time, 0, NULL, 1, -1) == FALSE)
			{
				temp = cptif - 1;
				while (cptif != temp)
				{
					if (fwd_get (combuf) == 0)
					{
						fin = TRUE;
						break;
					}
					pcom = combuf;
					switch (*pcom++)
					{
					case 'I':
						++cptif;
						break;
					case 'E':
						--cptif;
						break;
					case '@':
						if (cptif == (temp + 1))
							++temp;
						break;
					
					default:
						break;
					}
				}
			}
			break;
		case 'J':				/* Selection des messages "data" */
			pvoie->data_mode = atoi (pcom);
			break;
		case 'T':				/* limite la taille du fichier forwarde */
			while (*pcom)
			{
				if (toupper (*pcom) == 'M')
				{
					++pcom;
					pvoie->maxfwd = (uchar) atoi (pcom);
					while (isdigit (*pcom))
						++pcom;
				}
				else if (toupper (*pcom) == 'O')
				{
					++pcom;
					pvoie->oldfwd = (uchar) atoi (pcom);
					while (isdigit (*pcom))
						++pcom;
				}
				else if (toupper (*pcom) == 'P')
				{
					pvoie->typfwd |= FWD_PRIV;
					++pcom;
				}
				else if (toupper (*pcom) == 'S')
				{
					pvoie->typfwd |= FWD_SMALL;
					++pcom;
				}
				else if (toupper (*pcom) == 'D')
				{
					pvoie->typfwd |= FWD_DUPES;
					++pcom;
				}
				else if (isdigit (*pcom))
				{
					pvoie->maxfwd = (uchar) atoi (pcom);
					while (isdigit (*pcom))
						++pcom;
				}
				else
					++pcom;
			}
			break;
		case 'U':				/* Routage prioritaire */
			if ((ptr = strtok (pcom, " \t")) != NULL)
			{
				strn_cpy (6, pvoie->p_route[0], ptr);
				i = 1;
				while ((ptr = strtok (NULL, " \t")) != NULL)
				{
					strn_cpy (6, pvoie->p_route[i], ptr);
					++i;
					if (i == NB_P_ROUTE)
						break;
				}
			}
			break;
		case '@':				/* ELSE */
			temp = cptif - 1;
			while (cptif != temp)
			{
				if (fwd_get (combuf) == 0)
				{
					fin = TRUE;
					break;
				}
				pcom = combuf;
				++ligne;
				switch (*pcom++)
				{
				case 'I':
					++cptif;
					break;
				case 'E':
					--cptif;
					break;
				default:
					break;
				}
			}
			break;
		case '*':
		case '#':				/*   ligne commentaire */
			break;
		case '-':				/* fin de bloc - envoi du reverse forwarding */
			if ((reverse) &&
				((nbmess = attend_fwd (nobbs, pvoie->maxfwd, pvoie->oldfwd, pvoie->typfwd, pvoie->data_mode)) != 0L))
			{
/*				if (affiche == 1)
				{
#ifdef ENGLISH
					cprintf ("\nPrivate   : %d\r\n", nbmess->nbpriv);
					cprintf ("Bulletins : %d\r\n", nbmess->nbbul);
#else
					cprintf ("\nPriv‚s    : %d\r\n", nbmess->nbpriv);
					cprintf ("Bulletins : %d\r\n", nbmess->nbbul);
#endif
					for (i = 0; i < NB_P_ROUTE; i++)
					{
						if (*pvoie->p_route[i])
							cprintf ("Prio : %s\r\n", pvoie->p_route[i]);
					}
				}
*/				retour = 1;
			}
			else
				retour = 0;
			fin = TRUE;
			break;
		}
	}
	libere_tnc (&ptnc);
#ifdef __FBBDOS__
	if (affiche)
	{
		if (pvoie->reverse)
		{
			sleep_ (1);
		}
		close_win (fen_ptr);
	}
#endif
	fin_io ();
	ff ();
	return (retour);
}

void prog_rev_tnc (int voie)
{
	int i;
	int temp, nobbs, fin = FALSE, ligne = 0, cptif = 0;
	int port = 1;
	char nombbs[80];
	char combuf[80];
	char *pcom;
	typ_pfwd *ptnc;
	long h_time = time (NULL);

	ptnc = NULL;
	nobbs = 0;

	rewind_fwd ();

	while (TRUE)
	{
		if (fwd_get (combuf) == 0)
		{
			return;
		}
		pcom = combuf;
		++ligne;
		if (*pcom++ == 'A')
		{
			port = 1;
			strcpy (nombbs, pcom);
			nobbs = num_bbs (nombbs);
			if (indcmp (nombbs, svoie[voie]->sta.indicatif.call))
			{
				break;
			}
		}
	}
	for (i = 0; i < NB_P_ROUTE; i++)
		*(svoie[voie]->p_route[i]) = '\0';
	svoie[voie]->maxfwd = 0xff;
	svoie[voie]->oldfwd = 0;
	svoie[voie]->typfwd = 0;

	deb_io ();
	while ((!fin) && (fwd_get (combuf)))
	{
		pcom = combuf;
		++ligne;
		switch (*pcom++)
		{
		case '!':				/* Not */
			break;
		case 'B':				/* recuperer le nom de la BBS destinataire */
			break;
		case 'C':				/* sauvegarde du texte de connexion */
			break;
		case 'l':				/* Ligne de commande TNC */
			param_tnc (0, &ptnc, pcom);
			break;
		case 'O':
			break;
		case 'd':				/* commande DOS */
			param_tnc (1, &ptnc, pcom);
			break;
		case 'x':				/* commande DOS */
			param_tnc (2, &ptnc, pcom);
			break;
		case 'P':				/* Indication du port de forward */
			*pcom = toupper (*pcom);
			port = *pcom - '@';
			break;
		case 'K':				/* Canal preselectionne */
		case 'M':				/* Pas de reverse */
		case 'N':				/* Mode FBB inactif */
		case 'R':				/* Reverse */
		case 'S':				/* Chaines identification */
		case 'V':				/* Texte connexion */
		case 'Y':				/* Mise a l'heure */
		case 'Z':				/* NTS */
			break;
		case 'Q':				/* Interdit le reverse */
			break;
		case 'E':				/* ENDIF */
			--cptif;
			break;
		case 'F':				/* chercher les messages de l'OM associe a la BBS */
			break;
		case 'G':				/* Indicateur de groupe */
			break;
		case 'H':				/* indicateur de hierarchie */
			break;
		case 'I':				/* IF */
			++cptif;
			if (tst_fwd (pcom, nobbs, h_time, 0, NULL, 1, -1) == FALSE)
			{
				temp = cptif - 1;
				while (cptif != temp)
				{
					if (fwd_get (combuf) == 0)
					{
						fin = TRUE;
						break;
					}
					pcom = combuf;
					switch (*pcom++)
					{
					case 'I':
						++cptif;
						break;
					case 'E':
						--cptif;
						break;
					case '@':
						if (cptif == (temp + 1))
							++temp;
						break;
					default:
						break;
					}
				}
			}
			break;
		case 'T':				/* limite la taille du fichier forwarde */
			break;
		case 'U':				/* Routage prioritaire */
			break;
		case '@':				/* ELSE */
			temp = cptif - 1;
			while (cptif != temp)
			{
				if (fwd_get (combuf) == 0)
				{
					fin = TRUE;
					break;
				}
				pcom = combuf;
				++ligne;
				switch (*pcom++)
				{
				case 'I':
					++cptif;
					break;
				case 'E':
					--cptif;
					break;
				default:
					break;
				}
			}
			break;
		case '*':
		case '#':				/*   ligne commentaire */
			break;
		case '-':				/* fin de bloc - envoi du reverse forwarding */
			if ((port == no_port (voie)) && (ptnc))
			{
				program_fwd (1, 1, &ptnc, voie);
			}
			fin = 1;
			break;
		}
	}
	libere_tnc (&ptnc);
	fin_io ();
}
