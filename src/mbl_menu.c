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
 * MBL_MENU.C
 *
 */

#include <serv.h>

/*
 * Pas de forward si read-only
 *                si pas déclaré BBS sur un port GUEST
 */
int forward_auth (int voie)
{
	/* Toujours OK sur l'import/export */
	if (voie == INEXPORT)
		return (1);

	if (POP (no_port (voie)))
		return (1);
	
	if (svoie[voie]->read_only)
		return 0;

	if (std_header & 128)
	{
		/*  N'accepte que si BBS declarees */
		if ((BBS (svoie[voie]->finf.flags)) || (PMS (svoie[voie]->finf.flags)))
			return (1);
		else
			return (0);
	}

	if ((P_GUEST (voie)) && (!BBS (svoie[voie]->finf.flags)))
		return 0;
	return 1;
}

static void menu_mbl (void)
{
	int c, error = 0;
	int cmd = 1;
	char com[80];
	char *pt_err = indd;
	char *ptr;

	limite_commande ();
	while (*indd && (!ISGRAPH (*indd)))
		indd++;
	strn_cpy (70, com, indd);
	ptr = strchr (com, ' ');
	if (ptr)
		*ptr = '\0';
	c = *indd++;
	if ((!SYS (pvoie->finf.flags)) && (voiecur != CONSOLE) &&
		(!LOC (pvoie->finf.flags)) && (P_GUEST (voiecur)))
	{
		switch (toupper (c))
		{
		case 'A':
			interruption (voiecur);
			break;
		case 'B':
			if (bye ())
			{
				maj_niv (N_MENU, 0, 0);
				sortie ();
			}
			else
			{
				cmd_err (--indd);
			}
			break;
		case 'F':
			if (FOR (pvoie->mode))
			{
				if (*indd == '>')
				{
					maj_niv (N_FORW, 1, 0);
					rcv_rev_fwd ();
				}
				else
				{
					maj_niv (N_MENU, 0, 0);
					sortie ();
				}
			}
			else
				error = 1;
			break;
		case 'K':
			if (!read_only ())
			{
				if (toupper (*indd) == 'M')
				{
					maj_niv (N_MBL, 4, 0);
					mbl_kill ();
				}
				else
					error = 1;
			}
			else
				retour_mbl ();
			break;
		case 'L':
			if ((toupper (*indd) == 'M') || (toupper (*indd) == 'N'))
			{
				maj_niv (N_MBL, 1, 0);
				mbl_list ();
			}
			else
				error = 1;
			break;
		case 'N':
			maj_niv (N_MBL, 7, 0);
			error = mbl_name ();
			break;
		case 'O':
			maj_niv (N_MBL, 11, 0);
			error = mbl_options ();
			retour_mbl ();
			break;
 /* Added by N1URO for cross-node command set compatability */
 		case 'Q':
 			if (quit ())
 			{
 				maj_niv (N_MENU, 0, 0);
 				sortie ();
 			}
		case 'R':
			if ((toupper (*indd) == 'M') || (toupper (*indd) == 'N'))
			{
				maj_niv (N_MBL, 2, 0);
				mbl_read (0);
			}
			else
				error = 1;
			break;
		case 'S':
			maj_niv (N_MBL, 3, 0);
			mbl_send ();
			break;
		case 'T':
			maj_niv (N_MBL, 9, 0);
			mbl_tell ();
			break;
		case 'X':
			if (EXP (pvoie->finf.flags))
			{
				pvoie->finf.flags &= (~F_EXP);
				texte (T_MBL + 18);
			}
			else
			{
				pvoie->finf.flags |= F_EXP;
				texte (T_MBL + 17);
			}
			/* pvoie->finf.flags = pvoie->mode ; */
			if (!pvoie->read_only)
				ch_info ();
			retour_mbl ();
			break;
		case '[':
			if (forward_auth (voiecur))
			{
				while (ISGRAPH (*indd))
					++indd;
				if (*(indd - 1) == ']')
				{
					analyse_idnt (pt_err);
					if (pvoie->prot_fwd & FWD_FBB)
					{
						pvoie->temp1 = 0;
						pvoie->ind_mess = 0;
						pvoie->temp2 = FALSE;
						init_rec_fwd (voiecur);
						maj_niv (N_FORW, 5, 2);
						return;
					}
					else
						pvoie->mode |= F_BBS;
					prompt (pvoie->finf.flags, pvoie->niv1);
				}
				else
					cmd_err (pt_err);
			}
			else
				cmd_err (--indd);
			break;
		case '*':
			/* if ((*indd == '*') && (*(indd + 1) == '*'))
			   {
			   indd += 3;
			   if (test_linked ())
			   {
			   traite_voie (voiecur);
			   }
			   }
			   else */
			{
				cmd_err (--indd);
			}
			cmd = 0;
			break;
		case 'H':
			if ((*indd) && (strncmpi (indd, "ELP", 3) == 0))
			{
				int nb = 0;

				while ((*indd) && (!isspace (*indd)))
				{
					++indd;
					++nb;
				}
				if (nb != 3)
				{
					error = 1;
					break;
				}
			}
			else if (ISGRAPH (*indd))
			{
				error = 1;
				break;
			}
		case '?':
			while (isspace (*indd))
				++indd;
			help (indd);
			break;
		case ';':
			cmd = 0;
			break;
		case '\0':
			cmd = 0;
			prompt (pvoie->finf.flags, N_MBL);
			break;
		default:
			cmd = 0;
			--indd;
			if (!defaut ())
				error = 1;
			break;
		}
		if (error)
		{
			cmd_err (pt_err);
		}
	}
	else
	{
		if (!appel_pg (com))
		{
			switch (toupper (c))
			{
			case 'A':
				if (!ISGRAPH (*indd))
					interruption (voiecur);
				else
					error = 1;
				break;
			case 'B':
				if (bye ())
				{
					maj_niv (N_MENU, 0, 0);
					sortie ();
				}
				else
					error = 1;
				/* {
				   cmd_err(--indd) ;
				   }   */
				break;
			case 'C':
				maj_niv (N_CONF, 0, 0);
				error = conference ();
				break;
			case 'D':
				maj_niv (N_MBL, 8, 0);
				error = mbl_dump ();
				break;
			case 'E':
				if (droits (COSYSOP))
				{
					maj_niv (N_MBL, 11, 0);
					error = mbl_edit ();
				}
				else
					error = 1;
				/* {
				   cmd_err(--indd) ;
				   } */
				break;
			case 'F':
				switch (toupper (*indd))
				{
				case '>':
					if (FOR (pvoie->mode))
					{
						if (pvoie->prot_fwd & FWD_XPRO)
						{
							maj_niv (N_XFWD, 1, 5);
							xfwd ();
						}
						else
						{
							maj_niv (N_FORW, 1, 0);
							rcv_rev_fwd ();
						}
					}
					else
						error = 1;
					break;
				case '<':
					if (FOR (pvoie->mode))
					{
						send_binary_mess ();
					}
					else
						error = 1;
					break;
				case '\r':
					if (miniserv & 1)
					{
						pvoie->mbl = FALSE;
						texte (T_MBL + 19);
						/* pvoie->finf.flags |= F_EXP ; */
						maj_niv (0, 1, 0);
						prompt (pvoie->finf.flags, pvoie->niv1);
					}
					else
						error = 1;
					break;
				default:
					if (droits (COSYSOP))
						error = maint_fwd ();
					else
						error = 1;
					break;
				}
				break;
			case 'G':
				if (!ISGRAPH (*indd))
				{
					if (((gate) && nbgate () && (!pvoie->read_only)) || (voiecur == CONSOLE))
					{
						pvoie->temp3 = pvoie->niv1;
						maj_niv (N_TELL, 0, 0);
						pvoie->mbl = 0;
						duplex_tnc ();
					}
					else
					{
						texte (T_GAT + 8);
						retour_mbl ();
					}
				}
				else
					error = 1;
				break;
			case 'H':
				sup_ln (indd);
				if (*indd)
				{
					if (strncmpi (indd, "ELP", 3) == 0)
					{
						int nb = 0;

						while ((*indd) && (!isspace (*indd)))
						{
							++indd;
							++nb;
						}
						if (nb != 3)
						{
							error = 1;
							break;
						}
					}
					else
					{
						error = mbl_hold ();
						break;
					}
				}
			case '?':
				while (isspace (*indd))
					++indd;
				help (indd);
				break;
			case 'I':
				maj_niv (N_MBL, 5, 0);
				error = menu_wp_search ();
				break;
			case 'J':
				maj_niv (N_MBL, 10, 0);
				error = mbl_jheard ();
				break;
			case 'K':
				if (!read_only ())
				{
					maj_niv (N_MBL, 4, 0);
					error = mbl_kill ();
				}
				else
					retour_mbl ();
				break;
			case 'L':
				maj_niv (N_MBL, 1, 0);
				pvoie->typlist = 0;
				error = mbl_list ();
				break;
			case 'M':
				c = toupper (*indd);
				if ((!ISGRAPH (c)) || (c == 'V') || (c == 'H') || (c == 'A'))
				{
					error = mess_fic ();
				}
				else
				{
					if (droits (COSYSOP))
					{
						maj_niv (N_MBL, 1, 0);
						pvoie->typlist = 1;
						error = mbl_list ();
					}
					else
					{
						error = 1;
						/* cmd_err(--indd) ; */
					}
				}
				break;
			case 'N':
				maj_niv (N_MBL, 7, 0);
				error = mbl_name ();
				break;
			case 'O':
				maj_niv (N_MBL, 11, 0);
				error = mbl_options ();
				retour_mbl ();
				break;
			case 'P':
				if (!p_cmd ())
					error = 1;
				break;
 /* Added by N1URO for cross-node command set compatability */
 			case 'Q':
 				if (quit())
 				{
 				maj_niv (N_MENU, 0, 0);
 				sortie();
 				}
 				else
 				error = 1;
 				break;
			case 'R':
				maj_niv (N_MBL, 2, 0);
				error = mbl_read (0);
				break;
			case 'S':
				maj_niv (N_MBL, 3, 0);
				error = mbl_send ();
				break;
			case 'T':
				if (!ISGRAPH (*indd))
				{
					maj_niv (N_MBL, 9, 0);
					mbl_tell ();
				}
				else if (toupper (*indd) == 'H')
				{
					maj_niv (N_THEMES, 0, 0);
					error = themes ();
				}
				else
					error = 1;
				break;
			case 'U':
				if (!ISGRAPH (*indd))
				{
					if (!is_room ())
					{
						outln ("*** Disk full !", 15);
						retour_mbl ();
						break;
					}
					--indd;
					pvoie->temp1 = pvoie->niv1;
					strcpy (pvoie->dos_path, "\\");
					maj_niv (N_DOS, 3, 0);
					receive_file ();
				}
				else
					error = 1;
				break;
			case 'V':
				maj_niv (N_MBL, 6, 0);
				error = mbl_read (1);
				break;
			case 'W':
				if (!ISGRAPH (*indd))
				{
					--indd;
					pvoie->temp1 = pvoie->niv1;
					strcpy (pvoie->dos_path, "\\");
					maj_niv (N_DOS, 1, 0);
					dir ();
				}
				else
					error = 1;
				break;
			case 'X':
				if (!ISGRAPH (*indd))
				{
					if (EXP (pvoie->finf.flags))
					{
						pvoie->finf.flags &= (~F_EXP);
						texte (T_MBL + 18);
					}
					else
					{
						pvoie->finf.flags |= F_EXP;
						texte (T_MBL + 17);
					}
					/* pvoie->finf.flags = pvoie->mode ; */
					if (!pvoie->read_only)
						ch_info ();
					retour_mbl ();
				}
				else
					error = 1;
				break;
			case 'Y':
				if (ISGRAPH (*indd))
				{
					if ((toupper (*indd) == 'D') && (!user_ok ()))
					{
						texte (T_ERR + 18);
						retour_mbl ();
					}
					else
					{
						maj_niv (N_YAPP, 0, 0);
						pvoie->temp1 = pvoie->niv1;
						error = (menu_yapp () == 0);
					}
				}
				else
					help ("Y");
				break;
			case 'Z':
				if (!ISGRAPH (*indd))
				{
					--indd;
					pvoie->temp1 = pvoie->niv1;
					strcpy (pvoie->dos_path, "\\");
					maj_niv (N_DOS, 7, 0);
					del_file ();
				}
				else
					error = 1;
				break;
			case '$':
				if (droits (COSYSOP))
				{
					incindd ();
					mbl_disbul ();
					retour_mbl ();
				}
				else
					error = 1;
				break;
			case '[':
				if (forward_auth (voiecur))
				{
					while (ISGRAPH (*indd))
						++indd;
					if (*(indd - 1) == ']')
					{
						analyse_idnt (pt_err);
						if (pvoie->prot_fwd & FWD_FBB)
						{
							pvoie->temp1 = 0;
							pvoie->ind_mess = 0;
							pvoie->temp2 = FALSE;
							init_rec_fwd (voiecur);
							maj_niv (N_FORW, 5, 2);
							return;
						}
						else
							pvoie->mode |= F_BBS;
						prompt (pvoie->finf.flags, pvoie->niv1);
					}
					else
						cmd_err (pt_err);
				}
				else
					cmd_err (--indd);
				break;
			case '*':
				if ((*indd == '*') && (*(indd + 1) == '*'))
				{
					indd += 3;
					if (test_linked ())
					{
						traite_voie (voiecur);	/* Il y a des actions */
					}
				}
				else
				{
					cmd_err (--indd);
				}
				cmd = 0;
				break;
			case ';':
				cmd = 0;
				break;
			case '\0':
				cmd = 0;
				prompt (pvoie->finf.flags, N_MBL);
				break;
			default:
				cmd = 0;
				--indd;
				if (defaut () == 0)
				{
					++indd;
					if (appel_pg (com) == 0)
						error = 1;
				}
				break;
			}
			if (error)
			{
				cmd_err (com);
			}
		}
	}
	if (cmd)
	{
		pvoie->aut_linked = 0;
	}
}


void mbl_emul (void)
{
	df ("mbl_emul", 0);

	switch (pvoie->niv2)
	{

	case N_MENU:
		menu_mbl ();
		break;

	case N_FORW:
		mbl_send ();
		break;

	case 1:
		mbl_list ();
		break;

	case 2:
		mbl_read (pvoie->recliste.l);
		break;

	case 3:
		break;

	case 4:
		mbl_kill ();
		break;

	case 5:
		wp_search ();
		break;

	case 6:
		mbl_read (pvoie->recliste.l);
		break;

	case 7:
		mbl_name ();
		break;

	case 8:
		mbl_dump ();
		break;

	case 9:
		mbl_tell ();
		break;

	case 11:
		mbl_edit ();
		break;

	case 13:
		duplex_tnc ();
		break;

	case 14:
		mbl_passwd ();
		break;

	case 16:
		mess_liste (1);
		break;

	case 17:
		exec_pg ();
		break;

	case 18:
		review ();
		break;

	case 19:
#ifdef __WINDOWS__
		end_win_edit ();
#endif
#ifdef __FBBDOS__
		end_mini_edit ();
#endif
		break;

#ifdef __linux__
	case 20:
		exec_cmd (NULL);
		break;
#endif
	case 98:
		export_message (io_fich);
		break;

	case 99:
		import_message (io_fich);
		break;

	}
	ff ();
}
