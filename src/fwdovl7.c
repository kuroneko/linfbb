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
 *    MODULE FORWARDING OVERLAY 7
 */

#include <serv.h>

static int all_digit (char *);

static void maint_bid (char *);

void delete_bid (char *bid)
{
	FILE *fptr;
	bidfwd fwbid;
	int pos;
	int i;
	char *ptr;

	pos = search_bid (bid);

	if (bid_ptr)
	{
		ptr = bid_ptr + ((pos - 1) * BIDCOMP);
		for (i = 0; i < BIDCOMP; i++, *ptr++ = '\0');
	}
	else if (EMS_BID_OK ())
	{
		delete_exms_bid (pos);
	}

	if ((fptr = fopen (d_disque ("WFBID.SYS"), "r+b")) != NULL)
	{
		ptmes->type = '\0';
		ptmes->numero = 0L;
		*ptmes->bid = '\0';
		fseek (fptr, (long) pos * sizeof (bidfwd), 0);
		fwrite (&fwbid, sizeof (fwbid), 1, fptr);
		fclose (fptr);
	}
}


int dec_fwd (char *bbs)
{
	Forward *pfwd;
	int port, noport;
	int voie;
	int nobbs;

	strupr (sup_ln (bbs));
	if ((isdigit (*bbs)) && (!ISGRAPH (*(bbs + 1))))
	{
		noport = *bbs - '0';
		if (noport == 9)
		{						/* Stoppe le forward sur tous les ports */
			*bbs = '\0';
			for (port = 1; port < NBPORT; port++)
			{
				if (p_port[port].pvalid)
				{
					pfwd = p_port[port].listfwd;
					while (pfwd)
					{
						if (pfwd->forward)
						{
							if (pfwd->forward > 0)
							{
								deconnexion (pfwd->forward, 1);
								deconnexion (pfwd->forward, 1);
								deconnexion (pfwd->forward, 1);
								svoie[pfwd->forward]->curfwd = NULL;
							}
							*pfwd->fwdbbs = '\0';
							pfwd->forward = 0;
							pfwd->fwdpos = 0;
							pfwd->fin_fwd = pfwd->cptif = pfwd->fwdlig = 0;
							pfwd->reverse = 0;
						}
						pfwd = pfwd->suite;
					}
				}
			}
		}
		else if (noport >= 0)
		{						/* Selectionne un port */
			*bbs = '\0';
			if (p_port[noport].pvalid)
			{
				pfwd = p_port[noport].listfwd;
				while (pfwd)
				{
					if (pfwd->forward)
					{
						if (pfwd->forward > 0)
						{
							deconnexion (pfwd->forward, 1);
							deconnexion (pfwd->forward, 1);
							deconnexion (pfwd->forward, 1);
							svoie[pfwd->forward]->curfwd = NULL;
						}
						*pfwd->fwdbbs = '\0';
						pfwd->forward = 0;
						pfwd->fwdpos = 0;
						pfwd->fin_fwd = pfwd->cptif = pfwd->fwdlig = 0;
						pfwd->reverse = 0;
					}
					pfwd = pfwd->suite;
				}
			}
		}
	}
	else
	{
		nobbs = num_bbs (bbs);
		if (nobbs)
		{						/* Selectionne une BBS */
			noport = -1;
			for (port = 1; port < NBPORT; port++)
			{
				if (p_port[port].pvalid)
				{
					pfwd = p_port[port].listfwd;
					while (pfwd)
					{
						if (pfwd->no_bbs == nobbs)
						{
							if (pfwd->forward > 0)
							{
								voie = pfwd->forward;
								if (svoie[voie]->sta.connect)
								{
									deconnexion (voie, 1);
									deconnexion (voie, 1);
									deconnexion (voie, 1);
								}
								else
								{	/* Incident, la voie n'est pas connect‚e */

									pfwd->forward = -1;
									pfwd->no_bbs = 0;
								}
							}
							noport = port;
							break;
						}
						pfwd = pfwd->suite;
					}
				}
			}
		}
		else
			noport = -2;
	}
	aff_forward ();
	return (noport);
}


static int all_digit (char *ptr)
{
	while (*ptr)
	{
		if (!isdigit (*ptr))
			return (0);
		++ptr;
	}
	return (1);
}

static void maint_bid (char *ptr)
{
	FILE *fptr;
	bidfwd fwbid;
	char choix = '?';
	char bid[25];
	char s[80];
	int err = 0;
	int pos;

	ptr[20] = '\0';
	if (sscanf (ptr, "%s %c", bid, &choix) == 0)
		err = 1;
	strupr (bid);

	if (!err)
	{
		pos = search_bid (bid);

		switch (choix)
		{
		case '?':
			if (pos)
			{
				if ((fptr = fopen (d_disque ("WFBID.SYS"), "r+b")) != NULL)
				{
					fseek (fptr, (long) pos * sizeof (bidfwd), 0);
					fread (&fwbid, sizeof (fwbid), 1, fptr);
					sprintf (s, "$:%s #:%ld T:%c",
							 fwbid.fbid, fwbid.numero, fwbid.mode);
					outsln (s, strlen (s));
					fclose (fptr);
				}
				else
					texte (T_ERR + 19);
			}
			else
				texte (T_ERR + 19);
			break;
		case '+':
			if (!pos)
			{
				ptmes->type = 'M';
				ptmes->numero = 0L;
				strn_cpy (12, ptmes->bid, bid);
				w_bid ();
			}
			break;
		case '-':
			if (pos)
			{
				delete_bid (bid);
			}
			else
				texte (T_ERR + 19);
			break;
		default:
			err = 1;
			break;
		}
	}

	if (err)
		texte (T_ERR + 0);
}

void fwd_value (char *maxfwd, char *typfwd, char *typdat)
{
	int cptif;
	int nobbs = 0;
	int temp;
	int i;
	char combuf[80];
	char *pcom;
	long h_time = time (NULL);

	for (i = 0; i <= NBBBS; i++)
	{
		maxfwd[i] = 0xff;
		typfwd[i] = 0;
		typdat[i] = 1;
	}

	cptif = 0;

	rewind_fwd ();

	while (fwd_get (combuf))
	{
		pcom = combuf;

		switch (*pcom++)
		{
		case 'A':				/* BBS destinataire */
			nobbs = num_bbs (pcom);
			break;
		case 'E':				/* ENDIF */
			--cptif;
			break;
		case 'I':				/* IF */
			++cptif;
			if (tst_fwd (pcom, nobbs, h_time, 0, NULL, 1, -1) == FALSE)
			{
				temp = cptif - 1;
				while (cptif != temp)
				{
					if (fwd_get (combuf) == 0)
						break;
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
			typdat[nobbs] = atoi (pcom);
			break;
		case 'T':				/* limite la taille du fichier forwarde */
			while (*pcom)
			{
				if (toupper (*pcom) == 'M')
				{
					++pcom;
					maxfwd[nobbs] = (uchar) atoi (pcom);
					while (isdigit (*pcom))
						++pcom;
				}
				else if (toupper (*pcom) == 'O')
				{
					++pcom;
					while (isdigit (*pcom))
						++pcom;
				}
				else if (toupper (*pcom) == 'P')
				{
					typfwd[nobbs] |= FWD_PRIV;
					++pcom;
				}
				else if (toupper (*pcom) == 'S')
				{
					typfwd[nobbs] |= FWD_SMALL;
					++pcom;
				}
				else if (toupper (*pcom) == 'D')
				{
					typfwd[nobbs] |= FWD_DUPES;
					++pcom;
				}
				else if (isdigit (*pcom))
				{
					maxfwd[nobbs] = (uchar) atoi (pcom);
					while (isdigit (*pcom))
						++pcom;
				}
				else
					++pcom;
			}
			break;
		case '@':				/* ELSE */
			temp = cptif - 1;
			while (cptif != temp)
			{
				if (fwd_get (combuf) == 0)
					break;
				pcom = combuf;
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
		case '-':				/* fin de bloc */
			break;
		}
	}

}

static void no_route (int c_r)
{
#ifdef ENGLISH
	out ("No route    ", 12);
#else
	out ("Pas de route", 12);
#endif
	if (c_r)
		cr ();
}

int maint_fwd (void)
{
	int c, voie, nobbs, trouve = 0, err = 0;
	int noport, port_fwd, reverse = 0;
	int nb;
	long nmess;
	char bbs[80], s[80];
	char filename[MAXPATH];
	atfwd *nbmess;
	char *ptr;
	char maxfwd[NBBBS + 1];
	char typfwd[NBBBS + 1];
	char typdat[NBBBS + 1];

	strupr (indd);
	c = *indd++;
	if (!teste_espace ())
		err = 2;
	ptr = indd;

	switch (c)
	{

	case 'A':	/* Mark a message to be forwarded to a BBS */
		if (err)
			break;
		if ((nmess = lit_chiffre (1)) != 0L)
		{
			if (sscanf (indd, "%s", bbs) != 1)
				texte (T_ERR + 0);
			else if (!ch_record (ptmes, nmess, '\0'))
			{
				ptmes->numero = nmess;
				texte (T_ERR + 10);
			}
			else if ((nobbs = num_bbs (bbs)) == 0)
				texte (T_ERR + 0);
			else
			{
				indd = ptr;
				set_bit_fwd (ptmes->fbbs, nobbs);
				clr_bit_fwd (ptmes->forw, nobbs);
				if (*(ptmes->bbsv) == '\0')
					strn_cpy (40, ptmes->bbsv, bbs);
				if (ptmes->status == 'F')
				{
					if (ptmes->type == 'B')
						ptmes->status = '$';
					else
					{
						ptmes->status = 'Y';
					}
				}
				maj_rec (nmess, ptmes);
				clear_fwd (nmess);
				ins_fwd (ptmes);
				mbl_disbul ();
			}
		}
		break;

	case 'B':	/* Print the forward list of messages forwarded to a BBS */
		if (*indd)
		{
			char maxfwd[NBBBS + 1];
			char typfwd[NBBBS + 1];
			char typdat[NBBBS + 1];

			fwd_value (maxfwd, typfwd, typdat);

			if ((nobbs = num_bbs (sup_ln (indd))) == 0)
			{
				texte (T_ERR + 0);
				break;
			}
			pvoie->typlist = 1;
			print_fwd (nobbs, maxfwd[nobbs], 0, typfwd[nobbs], typdat[nobbs]);
			err = 3;
		}
		else
		{
			texte (T_ERR + 7);
		}
		break;

	case 'C':	/* Forward check : Gives the route */
		if (*indd)
		{
			if (err)
				break;
			ini_champs (voiecur);
			sup_ln (indd);
			strn_cpy (40, s, indd);
			ptr = bbs_via(indd);
			if (find (ptr))
			{
				ptmes->type = 'P';
				ptmes->status = 'N';
				strn_cpy (40, ptmes->bbsv, s);
			}
			else if (all_digit (ptr))
			{
				ptmes->type = 'T';
				ptmes->status = 'N';
				strn_cpy (6, ptmes->desti, indd);
			}
			else
			{
				ptmes->type = 'B';
				ptmes->status = '$';
				strn_cpy (40, ptmes->bbsv, s);
			}
			if (!test_forward (0))
				no_route (0);
			cr ();
		}
		else
		{
			err = 0;
			nobbs = 0;
		}
		break;

		/*
		   case 'K' : break ;
		 */

	case 'D':	/* Un-mark a message to be forwarded to a BBS */
		if (err)
			break;
		if ((nmess = lit_chiffre (1)) != 0L)
		{
			if (sscanf (indd, "%s", bbs) != 1)
				texte (T_ERR + 0);
			else if (!ch_record (ptmes, nmess, '\0'))
			{
				ptmes->numero = nmess;
				texte (T_ERR + 10);
			}
			else if ((nobbs = num_bbs (bbs)) == 0)
				texte (T_ERR + 0);
			else
			{
				indd = ptr;
				clr_bit_fwd (ptmes->fbbs, nobbs);
				set_bit_fwd (ptmes->forw, nobbs);
				if (*(ptmes->bbsv) == '\0')
					strn_cpy (40, ptmes->bbsv, bbs);
				maj_rec (nmess, ptmes);
				clear_fwd (nmess);
				ins_fwd (ptmes);
				mbl_disbul ();
			}
		}
		break;

	case 'G':	/* Print the maessages partly sent */
		err = 0;
		print_part ();
		break;

	case 'H':	/* Prints the H-Route of a BBS */
		if (*indd)
		{
			if (err)
				break;
			strn_cpy (40, ptmes->bbsv, indd);
			if (cherche_route (ptmes))
			{
				texte (T_MBL + 41);
			}
			else
				no_route (1);
		}
		break;

	case 'I':	/* BID maintenance */
		if (*indd)
		{
			if (err)
				break;
			maint_bid (indd);
		}
		break;

	case 'L':	/* Lists the pending forward */
		if (*indd)
		{
			if (err)
				break;
			if ((nobbs = num_bbs (sup_ln (indd))) == 0)
			{
				texte (T_ERR + 0);
				break;
			}
		}
		else
		{
			err = 0;
			nobbs = 0;
		}
		if (!affiche_forward (nobbs))
			texte (T_MBL + 3);
		break;

	case 'M':	/* Import BBS forwarding from file*/
		/* Read the callsign and the filename */
		nb = sscanf (indd, "%s %s", bbs, filename);
		if (nb <= 0)
			texte (T_ERR + 7);
		else if (nb == 1)
			texte (T_ERR + 20);
		else if (nb == 2)
		{
			int fd;
			int sav_voie;
			
			if ((fd = open(filename, S_IREAD)) != -1)
				close(fd);
			else 
			{
				strcpy(pvoie->appendf, filename);
				texte (T_ERR + 11);
				break;
			}

			/* Import channel free ? */
			if ((!is_room ()) || (svoie[INEXPORT]->sta.connect) || (inexport))
			{
				break;
			}
			
			sav_voie = voiecur;
			
			extind(bbs, svoie[INEXPORT]->sta.indicatif.call) ;
			svoie[INEXPORT]->sta.indicatif.num = 0 ;
			strcpy(io_fich, filename);
  
			mail_ch = INEXPORT;
			selvoie(mail_ch) ;
			pvoie->sta.connect = inexport = 4 ;
			pvoie->enrcur = 0L ;
			pvoie->debut = time(NULL);
			pvoie->mode = F_FOR | F_HIE | F_BID | F_MID ;
			pvoie->finf.lang = langue[0]->numlang ;
			aff_event(voiecur, 1);
			maj_niv(N_MBL, 99, 0) ;
			mbl_emul();

			selvoie(sav_voie) ;
			outln("OK", 2);
		}
		else
			texte (T_ERR + 0);
		break;
		
	case 'N':	/* Gives the forwarding status of a message */
		if (err)
			break;
		mbl_disbul ();
		break;

	case 'P':	/* Gives the swap information */
		if (err)
			break;
		indd[50] = '\0';
		ini_champs (voiecur);
#define DIEZE "#"
		strcpy (ptmes->exped, DIEZE);
		strcpy (ptmes->bbsv, DIEZE);
		strcpy (ptmes->desti, DIEZE);

		nb = sscanf (indd, "%c %s", (char *)&c, s);
		if (nb == 2)
		{
			switch (c)
			{
			case '<':
				strn_cpy (6, ptmes->exped, s);
				swapp_bbs (ptmes);
				if (strcmp (ptmes->exped, s) == 0)
					strcpy (ptmes->exped, DIEZE);
				break;
			case '>':
				strn_cpy (6, ptmes->desti, s);
				swapp_bbs (ptmes);
				if (strcmp (ptmes->desti, s) == 0)
					strcpy (ptmes->desti, DIEZE);
				break;
			case '@':
				strn_cpy (6, ptmes->bbsv, s);
				swapp_bbs (ptmes);
				if (strcmp (ptmes->bbsv, s) == 0)
					strcpy (ptmes->bbsv, DIEZE);
				break;
			default:
				c = 0;
				break;
			}
			if (c > 0)
			{
				swapp_bbs (ptmes);
				if (*ptmes->exped != '#')
				{
					sprintf (s, "< %s ", ptmes->exped);
					out (s, strlen (s));
					trouve = 1;
				}
				if (*ptmes->bbsv != '#')
				{
					sprintf (s, "@ %s ", ptmes->bbsv);
					out (s, strlen (s));
					trouve = 1;
				}
				if (*ptmes->desti != '#')
				{
					sprintf (s, "> %s ", ptmes->desti);
					out (s, strlen (s));
					trouve = 1;
				}
				if (trouve)
					cr ();
			}
			else
				texte (T_ERR + 0);
		}
		else
			texte (T_ERR + 0);
		break;

	case 'S':	/* Forward stop */
		if (err)
			break;
		if (*indd)
		{
			int save_voie = voiecur;

			switch (noport = dec_fwd (indd))
			{
			case -1:
				sprintf (s, "BBS %s is not forwarding", indd);
				break;
			case -2:
				sprintf (s, "Unknown BBS %s", indd);
				break;
			default:
				if (*indd)
					sprintf (bbs, "with %s ", indd);
				else
					*bbs = '\0';
				sprintf (s, "Stopping forwarding %son port %d", bbs, noport);
				break;
			}
			selvoie (save_voie);
			outln (s, strlen (s));
		}
		else
		{
			texte (T_ERR + 7);
		}
		break;

	case 'T':	/* Pending forward */

		fwd_value (maxfwd, typfwd, typdat);

		if (*indd)
		{
			if ((nobbs = num_bbs (sup_ln (indd))) == 0)
			{
				texte (T_ERR + 0);
				break;
			}
			if ((nbmess = attend_fwd (nobbs, maxfwd[nobbs], 0, typfwd[nobbs], typdat[nobbs])) != 0L)
			{
				sprintf (s, "%s: %dP /%dB - %d KB",
						 indd, nbmess->nbpriv, nbmess->nbbul, nbmess->nbkb);
				outln (s, strlen (s));
				trouve = 1;
			}
		}
		else
		{
			ptr = bbs_ptr;
			for (nobbs = 1; nobbs <= NBBBS; ++nobbs, ptr += 7)
			{
				if ((nbmess = attend_fwd (nobbs, maxfwd[nobbs], 0, typfwd[nobbs], typdat[nobbs])) != 0L)
				{
					n_cpy (6, bbs, ptr);
					sprintf (s, "%-6s : %2dP /%3dB - %d KB",
						  bbs, nbmess->nbpriv, nbmess->nbbul, nbmess->nbkb);
					outln (s, strlen (s));
					trouve = 1;
				}
			}
		}
		if (!trouve)
			texte (T_MBL + 3);
		err = 0;
		break;

	case 'U':	/* Disconnect a forwarding channel */
		if (err)
			break;
		if (isdigit (*indd))
		{
			voie = atoi (indd) + 1;
			if ((voie > 0) && (voie < NBVOIES) && (svoie[voie]->sta.connect))
			{
				sprintf (s, "Disconnecting channel %d", virt_canal (voie));
				force_deconnexion (voie, 1);
			}
			else
			{
				sprintf (s, "Channel %d is not connected", virt_canal (voie));
			}
			outln (s, strlen (s));
		}
		else
			texte (T_ERR + 3);
		break;


	case 'V':	/* Start message scanning */
		p_forward = 1;
		maj_options ();
		init_buf_fwd ();
		init_buf_swap ();
		init_buf_rej ();
		init_bbs ();
		aff_nbsta ();
		outln ("Message scanning ok", 19);
		err = 0;
		break;

	case 'R':	/* Start reverse forwarding */
		reverse = 1;

	case 'W':	/* Start forwarding */
		if (err)
			break;
		if (*indd)
		{
			switch (noport = val_fwd (indd, &port_fwd, reverse))
			{
			case -1:
				sprintf (s, "No forwarding channel available on port %d", port_fwd);
				break;
			case -2:
				sprintf (s, "No port defined for %s", indd);
				break;
			case -3:
				sprintf (s, "Unknown BBS %s", indd);
				break;
			case -4:
				sprintf (s, "BBS %s already connected", indd);
				break;
			default:
				sprintf (s, "Forwarding %s on port %d", indd, noport);
				break;
			}
			outln (s, strlen (s));
		}
		else
		{
			texte (T_ERR + 7);
		}
		break;

	default:
		err = 1;
		--indd;
		break;
	}
	if (err == 2)
	{
		texte (T_ERR + 2);
		err = 0;
	}
	if (err == 0)
		retour_mbl ();
	if (err == 3)
		err = 0;
	return (err);
}
