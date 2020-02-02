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
 *    MODULE TNC.C : INTERFACE AVEC LE TNC - MODE DUPLEX
 */

#include <serv.h>

static int capture = 0;

static int ch_canal (int);
static int open_capture (void);
static int pactor_cq (void);

static void disp_freq (int);
static void close_capture (void);
static void gw_help (void);
static void prog_indic (int);
static void tnc_cmd (int, int);

#define TNC_START 1
#define TNC_END 2

int connect_tnc (void)
{
	df ("connect_tnc", 0);

	if (svoie[CONSOLE]->sta.connect)
		return (0);
	selvoie (CONSOLE);
	strn_cpy (6, pvoie->sta.indicatif.call, cons_call.call);
	pvoie->sta.indicatif.num = cons_call.num;
	if (voiecur == CONSOLE)
	{
		pvoie->timout = time_n;
		capture = 0;
	}
	else
		pvoie->timout = time_n;
	pvoie->tstat = pvoie->debut = time (NULL);
	pvoie->l_mess = 0L;
	pvoie->l_yapp = 0L;
	pvoie->ret = 0;
	pvoie->sid = 0;
	pvoie->pack = 0;
	pvoie->read_only = 0;
	pvoie->vdisk = 2;
	pvoie->msg_held = 0;
	pvoie->xferok = pvoie->mess_recu = 1;
	pvoie->mbl = 0;
	init_timout (voiecur);
	pvoie->temp3 = 0;
	pvoie->nb_err = 0;
	console = 1;
	maj_niv (N_TELL, 0, 0);
	pvoie->deconnect = FALSE;
	console_on ();
	aff_event (CONSOLE, 1);
	connexion (voiecur);
	nouveau (voiecur);
	pvoie->sta.connect = 16;
	change_droits (voiecur);
	aff_nbsta ();
	curseur ();
	ff ();
	return (TRUE);
}

static int open_capture (void)
{
	capture = open (pvoie->appendf, O_CREAT | O_APPEND | O_WRONLY | O_TEXT, S_IREAD | S_IWRITE);
	if (capture < 0)
	{
		texte (T_ERR + 11);
		return (0);
	}
	return (capture);
}

static void close_capture (void)
{
	close (capture);
}

void write_capture (char *text, int len)
{
	int lg;
	char *ptr = text;

	if ((voiecur != CONSOLE) || (capture == 0))
		return;

	for (lg = 0; lg < len; lg++)
	{
		if (*ptr == '\r')
			*ptr = '\n';
		++ptr;
	}

	if (open_capture ())
	{
		write (capture, text, len);
		close_capture ();
	}

	ptr = text;
	for (lg = 0; lg < len; lg++)
	{
		if (*ptr == '\n')
			*ptr = '\r';
		++ptr;
	}
}

void duplex_tnc (void)
{
	char *ptr;
	char s[81];
	char buffer[300];
	struct stat bufstat;
	int nb, novoie, fin, save_voie, noport, prompt = 1;
	int command;


	switch (pvoie->niv3)
	{
	case 0:
		if (read_only ())
		{
			fin_tnc ();
			break;
		}
		pvoie->lignes = -1;
		texte (T_GAT + 0);
		aff_freq ();
		texte (T_GAT + 1);
		ch_niv3 (1);
		break;
	case 1:
		sup_ln (indd);
		strupr (indd);
		if ((*indd == 'E') || (*indd == 'Q'))
			fin_tnc ();
		else if (*indd == 'H')
		{
			gw_help ();
			texte (T_GAT + 1);
		}
		else
		{				
			//noport = *indd - '0';					
			noport = atoi(indd);		
			
			if ((voiecur != CONSOLE) && (noport == no_port (voiecur)))
			{
				texte (T_GAT + 6);
				texte (T_GAT + 1);
			}
			else
			{
				if ((noport > 0) && (noport < NBPORT) &&
					(p_port[noport].pvalid) &&
					(((p_port[noport].moport & 0x8) == 0) || (voiecur == CONSOLE)) &&
					((p_port[noport].moport & 0x10) || (droits (ACCGATE))))
				{
					if ((novoie = ch_canal (noport)) > 0)
					{
						/* *getvoie(novoie] = *pvoie; */
						svoie[novoie]->debut = time (NULL);
						svoie[novoie]->kiss = voiecur;
						init_timout (novoie);
						svoie[novoie]->sta.connect = 17;
						svoie[novoie]->lignes = -1;
						svoie[novoie]->xferok = 1;
						svoie[novoie]->msg_held = 0;
						svoie[novoie]->mess_recu = 1;
						set_binary (novoie, 0);
						svoie[novoie]->pack = 0;
						svoie[novoie]->read_only = 0;
						svoie[novoie]->vdisk = 2;
						svoie[novoie]->mbl = 0;
						svoie[novoie]->finf.lang = pvoie->finf.lang;
						svoie[novoie]->l_mess = 0L;
						svoie[novoie]->l_yapp = 0L;
						svoie[novoie]->temp3 = 0;
						svoie[novoie]->nb_err = 0;
						svoie[novoie]->niv1 = 0;
						svoie[novoie]->niv2 = 0;
						svoie[novoie]->niv3 = 0;
						if (voiecur == CONSOLE)
						{
							*svoie[novoie]->passwd = 'O';
							svoie[novoie]->finf.flags |= F_SYS;
						}
						pvoie->cross_connect = novoie;
						itoa ((novoie > 0) ? novoie - 1 : 0, varx[0], 10);
						var_cpy (1, p_port[noport].freq);
						texte (T_GAT + 2);
						if (p_port[no_port (novoie)].typort == TYP_BPQ)
						{
							command = 1;
							sta_drv (novoie, CMDE, (void *) &command);
							texte (T_GAT + 5);
							ch_niv3 (3);
						}
						else
						{
							texte (T_GAT + 4);
							ch_niv3 (2);
						}
						prog_indic (novoie);
						aff_nbsta ();
						tnc_cmd (TNC_START, novoie);
					}
					else
					{
						texte (T_GAT + 3);
						texte (T_GAT + 1);
					}
				}
				else
				{
					texte (T_GAT + 7);
					texte (T_GAT + 1);
				}
			}
		}
		break;
	case 2:
		sup_ln (indd);
		strupr (indd);
		pvoie->lignes = -1;
		if (strlen (indd))
		{
			*buffer = '\0';
			*indd = toupper (*indd);
#ifndef __linux__
			if (p_port[no_port (pvoie->cross_connect)].typort == TYP_MOD)
			{
				switch (*indd)
				{
				case 'Q':
				case 'E':
					save_voie = voiecur;
					pvoie->ch_mon = -1;
					selvoie (pvoie->cross_connect);
					dec (voiecur, 1);
					selvoie (save_voie);
					fin_tnc ();
					return;
				case 'D':
					save_voie = voiecur;
					selvoie (pvoie->cross_connect);
					md_no_echo (voiecur);
					dec (voiecur, 1);
					selvoie (save_voie);
					break;
				case 'K':
					texte (T_GAT + 5);
					ch_niv3 (3);
					return;
				case 'P':					
					save_voie = voiecur;
					pvoie->ch_mon = -1;
					selvoie (pvoie->cross_connect);
					dec (voiecur, 1);
					tnc_cmd (TNC_END, voiecur);
					if (p_port[no_port (voiecur)].typort == TYP_MOD)
						re_init_modem (voiecur);
					pvoie->kiss = -1;
					pvoie->sta.connect = FALSE;
					selvoie (save_voie);
					pvoie->cross_connect = -1;
					incindd ();
					if (*indd)
						ch_niv3 (1);
					else
						ch_niv3 (0);
					duplex_tnc ();
					return;
				case 'S':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						incindd ();
						strcpy (pvoie->appendf, indd);
						if ((stat (pvoie->appendf, &bufstat) == -1) ||
							((bufstat.st_mode & S_IFREG) == 0))
						{
							texte (T_ERR + 11);
							break;
						}
						texte (T_GAT + 10);
						save_voie = voiecur;
						pvoie->ch_mon = -1;
						selvoie (pvoie->cross_connect);
						status (voiecur);
						strcpy (pvoie->sr_fic, indd);
						pvoie->enrcur = 0L;
						fin = senddata (0);
						if (!fin)
						{
							maj_niv (N_TELL, 0, 4);
							prompt = 0;
						}
						selvoie (save_voie);
						if (!fin)
							ch_niv3 (5);
						else
						{
							texte (T_GAT + 5);
							ch_niv3 (3);
						}
					}
					else
						texte (T_GAT + 3);
					break;
				case 'W':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						incindd ();
						if (*indd)
						{
							strcpy (pvoie->appendf, indd);
							if (open_capture ())
							{
								texte (T_GAT + 11);
								close (capture);
							}
						}
						else
						{
							if (capture)
							{
								texte (T_GAT + 12);
								capture = 0;
							}
							else
								texte (T_ERR + 0);
						}
					}
					else
						texte (T_GAT + 3);
					break;
				default:
					save_voie = voiecur;
					selvoie (pvoie->cross_connect);
					outln (indd, strlen (indd));
					selvoie (save_voie);
					break;
				}
			}
			else if (p_port[no_port (pvoie->cross_connect)].typort != TYP_BPQ)
#endif
			{
				/* DED && KAM && FLEX && LINUX */
				switch (*indd)
				{
				case 'Q':
				case 'E':
					save_voie = voiecur;
					pvoie->ch_mon = -1;
					selvoie (pvoie->cross_connect);
					dec (voiecur, 1);
					dec (voiecur, 1);
					selvoie (save_voie);
					fin_tnc ();
					return;
				case 'K':
					prog_indic (pvoie->cross_connect);
					texte (T_GAT + 5);
					ch_niv3 (3);
					return;
				case 'C':
					pvoie->ch_mon = -1;
					if (*(indd + 1) == 'Q')
					{
						if (!pactor_cq ())
							texte (T_ERR + 2);
					}
					else if ((*(indd + 1) != ' ') && (p_port[no_port (voiecur)].typort != TYP_BPQ))
					{
						texte (T_ERR + 2);
					}
					else
					{
						save_voie = voiecur;
						selvoie (pvoie->cross_connect);
						*buffer = '\0';
						if (p_port[no_port (voiecur)].typort == TYP_PK)
							*(indd + 1) = 'O';
						nb = 0;
						deb_io ();
						switch (p_port[no_port (voiecur)].typort)
						{
						case TYP_DED:
						case TYP_HST:
							prog_indic (voiecur);
							if (!IS_PACTOR (no_port (voiecur)) && (DRSI (no_port (voiecur)) || HST (no_port (voiecur))))
							{
								while (ISGRAPH (*indd))
									++indd;
								while (isspace (*indd))
									++indd;
								sprintf (buffer, "C %d:%s", p_port[no_port (voiecur)].ccanal, indd);
							}
							else
								strcpy (buffer, indd);
							tnc_commande (voiecur, buffer, SNDCMD);
							break;
						case TYP_PK:
							tnc_commande (voiecur, indd, SNDCMD);
							break;
						case TYP_KAM:
							kam_commande (voiecur, indd);
							break;
						case TYP_BPQ:
							texte (T_GAT + 5);
							ch_niv3 (3);
							break;
#ifdef __WINDOWS__
						case TYP_AGW:
#endif
						case TYP_SCK:
						case TYP_TCP:
						case TYP_ETH:
						case TYP_FLX:
							prog_indic (voiecur);
							tnc_commande (voiecur, indd, SNDCMD);
							break;
						}
						fin_io ();
						selvoie (save_voie);
						outs (buffer, nb);
						aff_nbsta ();
					}
					break;
				case 'D':
					save_voie = voiecur;
					selvoie (pvoie->cross_connect);
					*buffer = '\0';
					nb = 0;
					deb_io ();
					switch (p_port[no_port (voiecur)].typort)
					{
					case TYP_DED:
					case TYP_HST:
					case TYP_SCK:
					case TYP_TCP:
					case TYP_ETH:
					case TYP_FLX:
#ifdef __WINDOWS__
					case TYP_AGW:
#endif
						tnc_commande (voiecur, "D", SNDCMD);
						break;
					case TYP_PK:
						tnc_commande (voiecur, "DI", SNDCMD);
						break;
					case TYP_KAM:
						kam_commande (voiecur, "D");
						break;
					}
					fin_io ();
					selvoie (save_voie);
					outs (buffer, nb);
					break;
				case 'J':
					j_list ((char) no_port (pvoie->cross_connect) + '0');
					break;
				case 'M':
					if (pvoie->ch_mon == -1)
					{
						pvoie->ch_mon = no_port (pvoie->cross_connect);
						sprintf (s, "%s: MONITOR ON", my_call);
					}
					else
					{
						pvoie->ch_mon = -1;
						sprintf (s, "%s: MONITOR OFF", my_call);
					}
					outln (s, strlen (s));
					break;
				case 'P':					
					save_voie = voiecur;
					pvoie->ch_mon = -1;
					selvoie (pvoie->cross_connect);
					dec (voiecur, 1);
					dec (voiecur, 1);
					tnc_cmd (TNC_END, voiecur);
#ifndef __linux__
					if (p_port[no_port (voiecur)].typort == TYP_MOD)
						re_init_modem (voiecur);
#endif
					pvoie->kiss = -1;
					pvoie->sta.connect = FALSE;
					selvoie (save_voie);
					pvoie->cross_connect = -1;
					incindd ();
					if (*indd)
						ch_niv3 (1);
					else
						ch_niv3 (0);
					duplex_tnc ();
					return;
				case 'S':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						incindd ();
						strcpy (pvoie->appendf, indd);
						if ((stat (pvoie->appendf, &bufstat) == -1) ||
							((bufstat.st_mode & S_IFREG) == 0))
						{
							texte (T_ERR + 11);
							break;
						}
						texte (T_GAT + 10);
						save_voie = voiecur;
						pvoie->ch_mon = -1;
						selvoie (pvoie->cross_connect);
						status (voiecur);
						strcpy (pvoie->sr_fic, indd);
						pvoie->enrcur = 0L;
						fin = senddata (0);
						if (!fin)
						{
							maj_niv (N_TELL, 0, 4);
							prompt = 0;
						}
						selvoie (save_voie);
						if (!fin)
							ch_niv3 (5);
						else
						{
							texte (T_GAT + 5);
							ch_niv3 (3);
						}
					}
					else
						texte (T_GAT + 3);
					break;
				case 'W':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						incindd ();
						if (*indd)
						{
							strcpy (pvoie->appendf, indd);
							if (open_capture ())
							{
								texte (T_GAT + 11);
								close (capture);
							}
						}
						else
						{
							if (capture)
							{
								texte (T_GAT + 12);
								capture = 0;
							}
							else
								texte (T_ERR + 0);
						}
					}
					else
						texte (T_GAT + 3);
					break;
				case 'Y':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						strcpy (pvoie->dos_path, "\\");
						ptr = ++indd;
						if (toupper (*ptr) == 'U')
							*ptr = 'D';
						else if (toupper (*ptr) == 'D')
							*ptr = 'U';
						incindd ();
						strcpy (pvoie->appendf, indd);
						if (*ptr == 'D')
						{
#ifdef ENGLISH
							sprintf (buffer, "Yapp sending %s ...", indd);
#else
							sprintf (buffer, "Yapp envoie %s ... ", indd);
#endif
#ifdef __FBBDOS__
							trait (0, buffer);
#endif
						}
						else
						{
#ifdef ENGLISH
							sprintf (buffer, "Yapp receiving %s ...", indd);
#else
							sprintf (buffer, "Yapp re‡oit %s ...   ", indd);
#endif
#ifdef __FBBDOS__
							trait (0, buffer);
#endif
						}
						indd = ptr;
						save_voie = voiecur;
						pvoie->ch_mon = -1;
						selvoie (pvoie->cross_connect);
						status (voiecur);

						maj_niv (N_YAPP, 0, 0);
						pvoie->temp1 = pvoie->niv1;
						pvoie->kiss = -2;
						menu_yapp ();
						if (pvoie->binary)
						{
							selvoie (save_voie);
							ch_niv3 (6);
						}
						else
						{
							selvoie (save_voie);
						}
						prompt = 0;
					}
					else
					{
						var_cpy (0, " ");
						texte (T_TRT + 2);
					}
					break;
				case 'H':
				case '?':
					gw_help ();
					break;
				case 'F':
					if (p_port[no_port (pvoie->cross_connect)].lfreq)
					{
						incindd ();
						if (*indd == '\0')
						{
							disp_freq (no_port (pvoie->cross_connect));
							break;
						}
					}
					else
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
				default:		/* if (!defaut()) { */
					if ((p_port[no_port (pvoie->cross_connect)].lfreq) && (isdigit (*indd)))
					{
						/* Selection d'une frequence */
						char *ptr;
						int port = no_port (pvoie->cross_connect);
						int val;
						int ok = 0;
						ListFreq *plf = p_port[port].lfreq;
						double freq;

						/* Transforme les ',' en '.' */
						ptr = indd;
						while (*ptr)
						{
							if (*ptr == ',')
								*ptr = '.';
							++ptr;
						}

						freq = atof (indd);
						val = (int) freq;

						if ((double) val == freq)
						{
							/* rechercher d'abord par val */

							while (plf)
							{
								if (plf->val == val)
								{
									ok = 1;
									break;
								}
								plf = plf->next;
							}
						}

						if (!ok)
						{
							/* C'est peut-etre la frequence */

							plf = p_port[port].lfreq;
							while (plf)
							{
								if (plf->freq == freq)
								{
									ok = 1;
									break;
								}
								plf = plf->next;
							}
						}

						if (ok)
						{
							char buf[80];

							sprintf (buf, "-> %g", plf->freq);
							outln (buf, strlen (buf));

							/* envoyer la commande */
							if (strncmpi (plf->cmde, "PTCTRX", 6) == 0)
							{
								ptctrx (port, plf->cmde);
							}
							else
							{
								char *cmde = plf->cmde;
#ifdef __WINDOWS__
								if (call_dll (cmde, NO_REPORT_MODE, NULL, 0, NULL) == -1)
									call_nbdos (&cmde, 1, NO_REPORT_MODE, NULL, NULL, NULL);
#endif
#ifdef __FBBDOS__
								send_dos (2, cmde, NULL);
#endif
#ifdef __linux__							
								call_nbdos (&cmde, 1, NO_REPORT_MODE, NULL, TOOLDIR, NULL);
#endif
							}
						}
						else
						{
							texte (T_ERR + 0);
							disp_freq (port);
						}
					}
					else
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
					}
					/* } */
					break;
				}
			}
#ifndef __linux__
			else
			{					/* BPQ */
				/* prog_indic(pvoie->cross_connect) ; */
				switch (*indd)
				{
				case 'Q':
				case 'E':
					save_voie = voiecur;
					pvoie->ch_mon = -1;
					selvoie (pvoie->cross_connect);
					dec (voiecur, 1);
					dec (voiecur, 1);
					selvoie (save_voie);
					fin_tnc ();
					return;
				case 'K':
					texte (T_GAT + 5);
					ch_niv3 (3);
					return;
				case 'D':
					save_voie = voiecur;
					selvoie (pvoie->cross_connect);
					*buffer = '\0';
					nb = 0;
					deb_io ();
					command = 2;
					sta_drv (voiecur, CMDE, (void *) &command);
					fin_io ();
					selvoie (save_voie);
					outs (buffer, nb);
					break;
				case 'P':
					save_voie = voiecur;
					pvoie->ch_mon = -1;
					selvoie (pvoie->cross_connect);
					dec (voiecur, 1);
					dec (voiecur, 1);
					tnc_cmd (TNC_END, voiecur);
					pvoie->kiss = -1;
					pvoie->sta.connect = FALSE;
					selvoie (save_voie);
					pvoie->cross_connect = -1;
					incindd ();
					if (*indd)
						ch_niv3 (1);
					else
						ch_niv3 (0);
					duplex_tnc ();
					/* prog_indic (pvoie->cross_connect); */
					return;
				case 'S':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						incindd ();
						strcpy (pvoie->appendf, indd);
						if ((stat (pvoie->appendf, &bufstat) == -1) ||
							((bufstat.st_mode & S_IFREG) == 0))
						{
							texte (T_ERR + 11);
							break;
						}
						texte (T_GAT + 10);
						save_voie = voiecur;
						pvoie->ch_mon = -1;
						selvoie (pvoie->cross_connect);
						status (voiecur);
						strcpy (pvoie->sr_fic, indd);
						pvoie->enrcur = 0L;
						fin = senddata (0);
						if (!fin)
						{
							maj_niv (N_TELL, 0, 4);
							prompt = 0;
						}
						selvoie (save_voie);
						if (!fin)
							ch_niv3 (5);
						else
						{
							texte (T_GAT + 5);
							ch_niv3 (3);
						}
					}
					else
						texte (T_GAT + 3);
					break;
				case 'W':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						incindd ();
						if (*indd)
						{
							strcpy (pvoie->appendf, indd);
							if (open_capture ())
							{
								texte (T_GAT + 11);
								close (capture);
							}
						}
						else
						{
							if (capture)
							{
								texte (T_GAT + 12);
								capture = 0;
							}
							else
								texte (T_ERR + 0);
						}
					}
					else
						texte (T_GAT + 3);
					break;
				case 'Y':
					if (voiecur != CONSOLE)
					{
						indd[40] = '\0';
						var_cpy (0, indd);
						texte (T_GAT + 9);
						break;
					}
					if (svoie[pvoie->cross_connect]->sta.connect)
					{
						strcpy (pvoie->dos_path, "\\");
						ptr = ++indd;
						if (toupper (*ptr) == 'U')
							*ptr = 'D';
						else if (toupper (*ptr) == 'D')
							*ptr = 'U';
						incindd ();
						strcpy (pvoie->appendf, indd);
						if (*ptr == 'D')
						{
#ifdef ENGLISH
							sprintf (buffer, "Yapp sending %s ...", indd);
#else
							sprintf (buffer, "Yapp envoie %s ... ", indd);
#endif
#ifdef __FBBDOS__
							trait (0, buffer);
#endif
						}
						else
						{
#ifdef ENGLISH
							sprintf (buffer, "Yapp receiving %s ...", indd);
#else
							sprintf (buffer, "Yapp re‡oit %s ...   ", indd);
#endif
#ifdef __FBBDOS__
							trait (0, buffer);
#endif
						}
						indd = ptr;
						save_voie = voiecur;
						pvoie->ch_mon = -1;
						selvoie (pvoie->cross_connect);
						status (voiecur);

						maj_niv (N_YAPP, 0, 0);
						pvoie->temp1 = pvoie->niv1;
						pvoie->kiss = -2;
						menu_yapp ();
						if (pvoie->binary)
						{
							selvoie (save_voie);
							ch_niv3 (6);
						}
						else
						{
							selvoie (save_voie);
						}
						prompt = 0;
					}
					else
					{
						var_cpy (0, " ");
						texte (T_TRT + 2);
					}
					break;
				case 'H':
				case '?':
					gw_help ();
					break;
				default:		/* if (!defaut()) { */
					indd[40] = '\0';
					var_cpy (0, indd);
					texte (T_GAT + 9);
					/* } */
					break;
				}
			}
#endif
		}
		if (prompt)
			texte (T_GAT + 4);
		break;
	case 3:
		pvoie->lignes = -1;
		ptr = indd;
		while (*ptr)
		{
			if (*ptr == '\n')
				*ptr = '\r';
			++ptr;
		}
		if (((*indd == '\033') || (*indd == '>')) && (strlen (indd) == 2))
		{
			ch_niv3 (2);
			texte (T_GAT + 4);
		}
		else
		{
			int len = strlen (indd);

			save_voie = voiecur;
			selvoie (pvoie->cross_connect);
			snd_drv (voiecur, DATA, indd, len, NULL);
			tor_stop (voiecur);
			pvoie->pack = 0;
			write_capture (indd, len);
			selvoie (save_voie);
			write_capture (indd, len);
		}
		break;
	case 4:
		fin = senddata (0);
		if (fin)
		{
			save_voie = voiecur;
			selvoie (pvoie->kiss);
			texte (T_GAT + 5);
			ch_niv3 (3);
			selvoie (save_voie);
			maj_niv (0, 0, 0);
		}
		break;
	case 5:
		if (*indd == '\033')
		{
			selvoie (pvoie->cross_connect);
			/* Arret du transfert */
			selvoie (CONSOLE);
#ifdef ENGLISH
			sprintf (buffer, "Transfer aborted");
#else
			sprintf (buffer, "Transfert arr‚t‚");
#endif
#ifdef __FBBDOS__
			trait (-1, buffer);
#endif
			sleep_ (1);
		}
		break;
	case 6:
		if (*indd == '\033')
		{
			selvoie (pvoie->cross_connect);
			cancel ("Abort\r");
			selvoie (CONSOLE);
#ifdef ENGLISH
			sprintf (buffer, "Transfer aborted");
#else
			sprintf (buffer, "Transfert arr‚t‚");
#endif
#ifdef __FBBDOS__
			trait (-1, buffer);
#endif
			sleep_ (1);
		}
		break;
	}
}


static int ch_canal (int port)
{
	/*
	 * Cherche une voie libre sur un port.
	 * Commence par la derniere voie du port
	 * Teste les voies reservees
	 */

	int i, j;

	if (port == 0)
	{
		if (svoie[CONSOLE]->sta.connect)
			return (-1);
		return (0);
	}

	if (p_port[port].pvalid == 0)
		return (-1);

#ifdef __linux__
	if (S_LINUX (port))
	{
		if (port_free (port) == 0)
			return (-1);
		for (i = nbcan_linux (); i > 0; i--)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == i) &&
					(S_LINUX (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)) &&
					(!svoie[j]->localmode)
					)
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					return (j);
				}
			}
		}
	}
	else
#endif
	if (DRSI (port))
	{
		if (port_free (port) == 0)
			return (-1);
		for (i = nbcan_drsi (); i > 0; i--)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == i) &&
					(DRSI (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)) &&
					(!svoie[j]->localmode)
					)
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					return (j);
				}
			}
		}
	}
	else if (HST (port))
	{
		if (port_free (port) == 0)
			return (-1);
		if (p_port[port].ccanal == 0)
		{
			/* Port Pactor. Cherche la voie correspondante */

			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == PACTOR_CH) &&
					(HST (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)) &&
					(!svoie[j]->localmode)
					)
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					return (j);
				}
			}
			return (-1);
		}

		for (i = nbcan_hst (); i > 0; i--)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == i) &&
					(HST (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)) &&
					(!svoie[j]->localmode)
					)
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					return (j);
				}
			}
		}
	}
	else if (BPQ (port))
	{
		if (port_free (port) == 0)
			return (-1);
		for (i = nbcan_bpq (); i > 0; i--)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.canal == i) &&
					(BPQ (no_port (j))) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)) &&
					(!svoie[j]->localmode)
					)
				{
					svoie[j]->affport.port = port;
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					return (j);
				}
			}
		}
	}
	else
	{
		for (i = p_port[port].nb_voies; i > 0; i--)
		{
			for (j = 1; j < NBVOIES; j++)
			{
				if ((svoie[j]->affport.port == port) &&
					(svoie[j]->affport.canal == i) &&
					(!svoie[j]->sta.connect) &&
					(!voie_forward (j)) &&
					(!svoie[j]->localmode)
					)
				{
					for (i = 0; i < 8; i++)
						*(svoie[j]->sta.relais[i].call) = '\0';
					return (j);
				}
			}
		}
	}
	return (-1);
}


void prog_indic (int novoie)
{
	char s[81];

	switch (p_port[no_port (novoie)].typort)
	{
	case TYP_DED:
	case TYP_HST:
	case TYP_SCK:
	case TYP_ETH:
	case TYP_FLX:
#ifdef __WINDOWS__
	case TYP_AGW:
#endif
		if (P_TOR (novoie))
			sprintf (s, "I %s", pvoie->sta.indicatif.call);
		else
			sprintf (s, "I %s-%d", pvoie->sta.indicatif.call, pvoie->sta.indicatif.num);
		break;
	case TYP_BPQ:
		sprintf (s, "*** Linked to %s-%d\r", pvoie->sta.indicatif.call, pvoie->sta.indicatif.num);
		break;
	}
	strcpy (svoie[novoie]->sta.indicatif.call, pvoie->sta.indicatif.call);
	if (P_TOR (novoie))
		svoie[novoie]->sta.indicatif.num = 0;
	else
		svoie[novoie]->sta.indicatif.num = pvoie->sta.indicatif.num;
	selcanal (no_port (novoie));

	switch (p_port[no_port (novoie)].typort)
	{
	case TYP_DED:
	case TYP_HST:
	case TYP_SCK:
	case TYP_ETH:
	case TYP_FLX:
#ifdef __WINDOWS__
	case TYP_AGW:
#endif
		tnc_commande (novoie, s, SNDCMD);
		break;
	case TYP_BPQ:
		snd_drv (novoie, DATA, s, strlen (s), NULL);
		break;
	}

	selcanal (no_port (voiecur));
}


int nbgate (void)
{
	int nb = 0;
	int port;

	for (port = 1; port < NBPORT; port++)
	{
		if ((p_port[port].pvalid) &&
			(port != no_port (voiecur)) &&
			((p_port[port].moport & 8) == 0) &&
			((p_port[port].moport & 0x10) || (droits (ACCGATE))))
		{
			++nb;
		}
	}
	return (nb);
}


void aff_freq (void)
{
	int port;
	char s[80];

	for (port = 1; port < NBPORT; port++)
	{
		if ((p_port[port].pvalid) &&
			(port != no_port (voiecur)) &&
			(((p_port[port].moport & 8) == 0) || (voiecur == CONSOLE)) &&
			((p_port[port].moport & 0x10) || (droits (ACCGATE))))
		{
			sprintf (s, "%d : %s", port, p_port[port].freq);
			outln (s, strlen (s));
		}
	}
}


void fin_tnc (void)
{
	char s[80];
	int autre_voie = pvoie->cross_connect;

	if (autre_voie > 0)
	{
		tnc_cmd (TNC_END, autre_voie);

		/* Remet l'indicatif de la BBS */
		switch (p_port[no_port (autre_voie)].typort)
		{
		case TYP_DED:
		case TYP_HST:
		case TYP_SCK:
		case TYP_ETH:
		case TYP_FLX:
#ifdef __WINDOWS__
		case TYP_AGW:
#endif
			if (P_TOR (autre_voie))
				sprintf (s, "I %s", mycall);
			else
				sprintf (s, "I %s-%d", mycall, myssid);
			break;
		case TYP_BPQ:
			sprintf (s, "*** Linked to %s-%d\r", mycall, myssid);
			break;
		}

		selcanal (no_port (autre_voie));

		switch (p_port[no_port (autre_voie)].typort)
		{
		case TYP_DED:
		case TYP_HST:
		case TYP_SCK:
		case TYP_ETH:
		case TYP_FLX:
#ifdef __WINDOWS__
		case TYP_AGW:
#endif
			tnc_commande (autre_voie, s, SNDCMD);
			break;
		case TYP_BPQ:
			snd_drv (autre_voie, DATA, s, strlen (s), NULL);
			break;
		}

		selcanal (no_port (voiecur));

		svoie[autre_voie]->kiss = -1;
		svoie[autre_voie]->sta.connect = FALSE;
#ifndef __linux__
		if (p_port[no_port (autre_voie)].typort == TYP_MOD)
			re_init_modem (autre_voie);
#endif
	}
	pvoie->ch_mon = -1;
	pvoie->cross_connect = -1;
	*indd = '\0';
	if (pvoie->temp3 == N_MBL)
	{
		aff_nbsta ();
		retour_mbl ();
	}
	else if (pvoie->temp3 == 0)
	{
		pvoie->sta.connect = FALSE;
		maj_niv (0, 0, 0);
		if (voiecur == CONSOLE)
			console_off ();
	}
#ifndef MINISERV
	else
	{
		maj_niv (0, 1, 0);
		choix ();
	}
#endif

	curseur ();
	aff_nbsta ();
}


void gw_help (void)
{
	incindd ();
	if (*indd == '\0')
		strcpy (indd, "H");
	out_help (indd);
}

static void error_file (int nolig)
{
	char wtexte[200];

	deb_io ();
#ifdef ENGLISH
	if (operationnel)
	{
		sprintf (wtexte, "\r\nError in file gateway.sys line %d  \r\n\a", nolig);
		if (w_mask & W_FILE)
			mess_warning (admin, "*** FILE ERROR ***    ", wtexte);
	}
	sprintf (wtexte, "Error in file gateway.sys line %d  ", nolig);
#else
	if (operationnel)
	{
		sprintf (wtexte, "\r\nErreur fichier gateway.sys ligne %d\r\n\a", nolig);
		if (w_mask & W_FILE)
			mess_warning (admin, "*** ERREUR FICHIER ***", wtexte);
	}
	sprintf (wtexte, "Erreur fichier gateway.sys ligne %d", nolig);
#endif
	fin_io ();
	win_message (5, wtexte);
}

static int cmde (char *com_buf)
{
	int type;
	char *ptr = com_buf, *lptr = com_buf;

	sup_ln (com_buf);

	while ((*ptr) && (!ISGRAPH (*ptr)))
		++ptr;
	type = toupper (*ptr);

	++ptr;

	/* ELSE */
	if ((type == 'E') && (toupper (*ptr) == 'L'))
		type = '@';

	while (ISGRAPH (*ptr))
		++ptr;

	while ((*ptr) && (!ISGRAPH (*ptr)))
		++ptr;

	while ((*lptr++ = *ptr++) != '\0');

	return (type);
}

static void clear_freq (int port)
{
	ListFreq *plf;

	while (p_port[port].lfreq)
	{
		plf = p_port[port].lfreq;
		p_port[port].lfreq = plf->next;
		m_libere (plf, sizeof (ListFreq));
	}
}

static int add_freq (int port, char *buf)
{
	int v;
	char *freq;
	ListFreq *plf;
	ListFreq *phead = p_port[port].lfreq;

	v = atoi (buf);
	cmde (buf);

	if ((v < 1) || (v > 99))
		return (FALSE);

	freq = buf;
	while (ISGRAPH (*buf))
		++buf;

	if (!isprint (*buf))
		return FALSE;

	*buf++ = '\0';				/* buf pointe la commande */

	if (phead)
	{
		while (phead->next)
			phead = phead->next;
		plf = m_alloue (sizeof (ListFreq));
		phead->next = plf;
	}
	else
	{
		plf = m_alloue (sizeof (ListFreq));
		p_port[port].lfreq = plf;
	}

	plf->val = v;
	plf->freq = atof (freq);
	n_cpy (sizeof (plf->cmde) - 1, plf->cmde, buf);
	plf->next = NULL;

	return TRUE;
}

static void tnc_cmd (int cmd, int voie)
{
	int c;
	int d;
	int cptif = 0;
	int nolig = 0;
	int temp;
	int port;
	long h_time = time (NULL);
	FILE *fptr;
	typ_pfwd *ptnc = NULL;
	char com_buf[300];

	if (voie <= 0)
		return;

	port = no_port (voie);
	clear_freq (port);

	fptr = fopen (c_disque ("gateway.sys"), "r");
	if (fptr == NULL)
		return;

	while (fgets (com_buf, sizeof (com_buf), fptr))
	{
		++nolig;
		c = cmde (com_buf);

		switch (c)
		{
		case '\0':
		case '#':				/* comment */
			break;

		case 'E':				/* ENDIF */
			--cptif;
			break;

		case 'I':				/* IF */
			++cptif;
			if (tst_fwd (com_buf, 0, h_time, port, NULL, 0, port) == FALSE)
			{
				temp = cptif - 1;
				while (cptif != temp)
				{
					if (fgets (com_buf, sizeof (com_buf), fptr) == NULL)
						break;

					++nolig;
					d = cmde (com_buf);

					switch (d)
					{
					case 'I':	/* if */
						++cptif;
						break;
					case 'E':	/* endif */
						--cptif;
						break;
					case '@':	/* else */
						if (cptif == (temp + 1))
							++temp;
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
				if (fgets (com_buf, sizeof (com_buf), fptr) == NULL)
					break;

				++nolig;
				d = cmde (com_buf);

				switch (d)
				{
				case 'I':		/* if */
					++cptif;
					break;
				case 'E':		/* endif */
					--cptif;
					break;
				default:
					break;
				}
			}
			break;

		case 'B':				/* BEGIN */
			if (cmd == TNC_START)
			{
				d = cmde (com_buf);
				switch (d)
				{
				case 'D':
					param_tnc (1, &ptnc, com_buf);
					break;
				case 'L':		/* Ligne de commande TNC */
					param_tnc (0, &ptnc, com_buf);
					break;
				case 'X':		/* Commande TNC */
					param_tnc (2, &ptnc, com_buf);
					break;
				default:		/* ??? */
					error_file (nolig);
					break;
				}
			}
			break;

		case 'F':				/* CMD */
			if (!add_freq (port, com_buf))
				error_file (nolig);
			break;

		case 'S':				/* STOP */
			if (cmd == TNC_END)
			{
				d = cmde (com_buf);
				switch (d)
				{
				case 'D':
					param_tnc (1, &ptnc, com_buf);
					break;
				case 'L':		/* Ligne de commande TNC */
					param_tnc (0, &ptnc, com_buf);
					break;
				case 'X':		/* Commande TNC */
					param_tnc (2, &ptnc, com_buf);
					break;
				default:		/* ??? */
					error_file (nolig);
					break;
				}
			}
			break;

		default:				/* ??? */
			error_file (nolig);
			break;

		}
	}

	fclose (fptr);

	if (ptnc)
		program_fwd (FALSE, TRUE, &ptnc, voie);
}

static void disp_freq (int port)
{
	char buf[80];
	int nb = 0;
	ListFreq *plf = p_port[port].lfreq;

	while (plf)
	{
		sprintf (buf, "%2d:%-10g ", plf->val, plf->freq);
		out (buf, strlen (buf));
		if (++nb == 4)
		{
			cr ();
			nb = 0;
		}
		plf = plf->next;
	}
	if (nb)
		cr ();
}

static int pactor_cq (void)
{
	char buffer[80];
	char callsign[20];
	char name[20];
	char qth[40];
	int i;
	int autre_voie = pvoie->cross_connect;

	if (IS_PACTOR (no_port (autre_voie)))
	{
		if (BUSY (no_port (autre_voie)))
		{
			sprintf (buffer, "%s: FREQ-BUSY fm PACTOR", mycall);
			outsln (buffer, strlen (buffer));
			return (1);
		}

		/* Indicatif de l'appelant */
		strcpy (callsign, pvoie->sta.indicatif.call);

		/* Prenom de l'appelant */
		if (*(pvoie->finf.prenom))
			strcpy (name, pvoie->finf.prenom);
		else
			strcpy (name, "???");

		/* QTH de l'appelant */
		if (*(pvoie->finf.ville))
			strcpy (qth, pvoie->finf.ville);
		else
			strcpy (qth, "???");

		prog_indic (autre_voie);
		tnc_commande (autre_voie, "!U", SNDCMD);

#define NB_CALL 3
		for (i = 0; i < NB_CALL; i++)
		{
/* 			sprintf (buffer, "CQ CQ CQ de %s %s %s (%d)\r", callsign, callsign, callsign, NB_CALL - i);*/
/* Add from DM3TT compilation */
/* Added 'PACTOR' as suggested by G4APL SysOP GB7CIP */			
			sprintf (buffer, "CQ CQ CQ PACTOR de %s %s %s @ %s (%d)\r", callsign, callsign, callsign, mycall, NB_CALL - i);
			snd_drv (autre_voie, DATA, buffer, strlen (buffer), NULL);
			outs (buffer, strlen (buffer));
		}

		/* sprintf (buffer, "Op %s fm %s is QRV ... Pse K\r", name, qth);*/
/* Add from DM3TT compilation */
		sprintf (buffer, "Op %s fm %s is QRV for QSO pse Connect %s\rPse K\r", name, qth, callsign);
		snd_drv (autre_voie, DATA, buffer, strlen (buffer), NULL);
		outs (buffer, strlen (buffer));

		tnc_commande (autre_voie, "!D", SNDCMD);

		aff_nbsta ();
		return (1);
	}

	else
		return (0);
}
