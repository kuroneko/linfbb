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

#include <serv.h>

/*
 * Module emulation WA7MBL
 */

static int len_bbs (char *);

void mbl_disbul (void)
{
	long no;
	int nobbs, cmpmsk, noctet, debut;
	char s[80];
	bullist *pbul;

	if ((*indd) && ((no = lit_chiffre (1)) != 0L))
	{
		if ((pbul = ch_record (NULL, no, '\0')) != NULL)
		{
			debut = 1;
			s[0] = ' ';
			ltoa (no, varx[0], 10);
			texte (T_MBL + 34);
			for (nobbs = 0; nobbs < NBBBS; nobbs++)
			{
				noctet = (nobbs - 1) / 8;
				cmpmsk = 1 << ((nobbs - 1) % 8);
				if ((pbul->type) && (pbul->fbbs[noctet] & cmpmsk))
				{
					strn_cpy (6, s + 1, bbs_ptr + (nobbs - 1) * 7);
					if (debut)
					{
						debut = 0;
						out ("Fwd:", 4);
					}
					out (s, strlen (s));
				}
			}
			if (!debut)
				cr ();
			debut = 1;
			for (nobbs = 0; nobbs < NBBBS; nobbs++)
			{
				noctet = (nobbs - 1) / 8;
				cmpmsk = 1 << ((nobbs - 1) % 8);
				if ((pbul->type) && (pbul->forw[noctet] & cmpmsk))
				{
					if (*(bbs_ptr + (nobbs - 1) * 7))
					{
						if (debut)
						{
							debut = 0;
							out ("Ok :", 4);
						}
						strn_cpy (6, s + 1, bbs_ptr + (nobbs - 1) * 7);
						out (s, strlen (s));
					}
				}
			}
			if (!debut)
				cr ();
		}
		else
		{
			ptmes->numero = no;
			texte (T_ERR + 10);
		}
	}
	else
		texte (T_ERR + 3);
}

void mbl_tell (void)
{
	int sav_voie = voiecur;
	char s[80];

	pvoie->lignes = -1;
	switch (pvoie->niv3)
	{
	case 0:
		tst_appel ();
		if ((!ok_tell) || (svoie[CONSOLE]->sta.connect) || (v_tell))
		{
			texte (T_MBL + 12);
			retour_mbl ();
		}
		else
		{
			pvoie->sniv1 = N_MBL;
			pvoie->sniv2 = 0;
			pvoie->sniv3 = 0;
			texte (T_MBL + 13);
#ifdef ENGLISH
			sprintf (s, "%6s (%s) calling ", pvoie->sta.indicatif.call, pvoie->finf.prenom);
#else
			sprintf (s, "Appel de %6s (%s)", pvoie->sta.indicatif.call, pvoie->finf.prenom);
#endif
#if defined(__WINDOWS__) || defined(__linux__)
			sysop_call (s);
#endif
#ifdef __FBBDOS__
			console_on ();
			trait (-1, s);
#endif
			v_tell = voiecur;
			music (1);
			ch_niv3 (1);
		}
		break;
	case 1:
		if (t_tell == 0)
		{
#if defined(__WINDOWS__) || defined(__linux__)
			sysop_end ();
#endif
#ifdef __FBBDOS__
			console_off ();
			trait (0, " ");
#endif
			t_tell = -1;
			texte (T_MBL + 14);
			v_tell = 0;
			retour_mbl ();
		}
		break;
	case 2:
		if (v_tell)
		{
			if (voiecur == v_tell)
			{
				selvoie (CONSOLE);
				outs (indd, strlen (indd));
			}
			else
			{
				selvoie (v_tell);
				outs (indd, strlen (indd));
			}
			selvoie (sav_voie);
		}
		else
		{
			console_off ();
			maj_niv (0, 0, 0);
			pvoie->deconnect = -1;
			close_print ();
		}
		if (t_tell == 0)
			t_tell = -1;
		break;
	}
}


int affiche_forward (int nobbs)
{
	int pos, noctet, i, j;
	int ok = 0;
	char cmpmsk;
	char *ptr;
	char s[80];
	recfwd *prec;
	lfwd *ptr_fwd = tete_fwd;

	unsigned offset = 0;
	bloc_mess *bptr = tete_dir;
	bullist bul;

	pos = 0;
	noctet = (nobbs - 1) / 8;
	cmpmsk = 1 << ((nobbs - 1) % 8);
	ptr = bbs_ptr + (nobbs - 1) * 7;

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
			if ((prec->type) && ((nobbs == 0) || (prec->fbbs[noctet] & cmpmsk)))
			{
				sprintf (s, "%c %6ld %2d KB ", prec->type, prec->nomess, prec->kb);
				outs (s, strlen (s));
				++ok;
				if (nobbs == 0)
				{
					ptr = bbs_ptr;
					for (i = 0; i < NBMASK; i++)
					{
						cmpmsk = '\001';
						for (j = 0; j < 8; j++)
						{
							if (prec->fbbs[i] & cmpmsk)
							{
								outs (ptr, len_bbs (ptr));
								outs (" ", 1);
							}
							ptr += 7;
							cmpmsk <<= 1;
						}
					}
					cr ();
				}
				else
				{
					outsln (ptr, len_bbs (ptr));
				}
			}
			pos++;
		}
	}
	else
	{
		ouvre_dir ();

		while (bptr)
		{
			if (bptr->st_mess[offset].noenr)
			{
				read_dir (bptr->st_mess[offset].noenr, &bul);

				if (bul.type)
				{
					int kb = (int) (bul.taille >> 10);

					if ((bul.type) && ((nobbs == 0) || (bul.fbbs[noctet] & cmpmsk)))
					{
						sprintf (s, "%c %6ld %2d KB ", bul.type, bul.numero, kb);
						outs (s, strlen (s));
						++ok;
						if (nobbs == 0)
						{
							ptr = bbs_ptr;
							for (i = 0; i < NBMASK; i++)
							{
								cmpmsk = '\001';
								for (j = 0; j < 8; j++)
								{
									if (bul.fbbs[i] & cmpmsk)
									{
										outs (ptr, len_bbs (ptr));
										outs (" ", 1);
									}
									ptr += 7;
									cmpmsk <<= 1;
								}
							}
							cr ();
						}
						else
						{
							outsln (ptr, len_bbs (ptr));
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
	return (ok);
}

int bye (void)
{
	sup_ln (indd);
	return ((strcmpi (indd, "YE") == 0) || (!ISGRAPH (*indd)));
}

/* added by N1URO for URONode, FlexNet, and Xnet command set
    compatability */
 int quit (void)
 {
         sup_ln (indd);
         return ((strcmpi (indd, "YE") == 0) || (!ISGRAPH (*indd)));
 }
 
static int len_bbs (char *bbs)
{
	int nb = 0;

	while (nb < 6)
	{
		if (*bbs++ == '\0')
			break;
		++nb;
	}
	return (nb);
}

static void opt_choix (char c)
{
	if (c == Oui)
		strcpy (varx[0], langue[vlang]->plang[OUI - 1]);
	else
		strcpy (varx[0], langue[vlang]->plang[NON - 1]);
}

int mbl_options (void)
{
	int nb, ok = 0;
	int error = 0;
	FILE *fptr;
	info frec;
	long base;
	char c;

	if (((c = *indd++) != 0) && (ISGRAPH (c)))
	{
		switch (toupper (c))
		{
		case 'L':				/* Langue */
			if (*indd == ' ')
			{
				teste_espace ();
				if (isdigit (*indd))
				{
					nb = atoi (indd);
					if ((ok = ch_language (nb, pvoie->ncur, &frec)) != 0)
					{
						pvoie->finf.lang = nb - 1;
						init_langue (voiecur);
						texte (2);
					}
					else
						texte (T_ERR + 22);
				}
				else
					texte (T_ERR + 3);
			}
			else
			{
				texte (T_MBL + 31);
				for (nb = 0; nb < maxlang; nb++)
				{
					sprintf (varx[0], "%2d", nb + 1);
					sprintf (varx[1], "%-10s", nomlang + nb * LG_LANG);
					/*                                   itoa(nb + 1, varx[0], 10) ;
					   var_cpy(1, nomlang + nb * LG_LANG) ; */
					texte (T_MBL + 32);
					if (((nb + 1) % 4) == 0)
						cr ();
				}
				cr_cond ();
			}
			break;

		case 'M':				/* Nouveaux messages */
			if (NEW (pvoie->finf.flags))
			{
				ch_bit (pvoie->ncur, &frec, F_NEW, Non);
				pvoie->finf.flags &= (~F_NEW);
				opt_choix (Non);
			}
			else
			{
				ch_bit (pvoie->ncur, &frec, F_NEW, Oui);
				pvoie->finf.flags |= (F_NEW);
				opt_choix (Oui);
			}
			texte (T_MBL + 52);
			break;

		case 'N':				/* Base messages */
			if (*indd == ' ')
			{
				teste_espace ();
				if (isdigit (*indd))
				{
					base = atol (indd);
					if (base > (nomess >> 7))
						base /= 1000L;
					pvoie->finf.on_base = (int) base;
				}
				else
					texte (T_ERR + 3);
			}
/************ SERA A SUPPRIMER -> REMPLACE PAR $q ************/
			ltoa (1000L * (long) pvoie->finf.on_base, varx[0], 10);
			texte (T_MBL + 26);
			break;

		case 'P':				/* Pagination */
			if (*indd == ' ')
			{
				teste_espace ();
				if (isdigit (*indd))
				{
					ch_bit (pvoie->ncur, &frec, F_PAG, Oui);
					nb = atoi (indd);
					if (nb < 4)
						nb = 4;
					if (nb > 100)
						nb = 100;
					pvoie->finf.nbl = nb;
					pvoie->finf.flags |= (F_PAG);
					ok = 1;
					texte (T_MBL + 23);
				}
				else
					texte (T_ERR + 3);
			}
			else
			{
				if (PAG (pvoie->finf.flags))
				{
					ch_bit (pvoie->ncur, &frec, F_PAG, Non);
					pvoie->finf.flags &= (~F_PAG);
					texte (T_MBL + 22);
				}
				else
				{
					ch_bit (pvoie->ncur, &frec, F_PAG, Oui);
					pvoie->finf.flags |= (F_PAG);
					texte (T_MBL + 23);
				}
			}
			break;

		case 'R':				/* Lecture restreinte */
			if (droits (CONSMES))
			{
				if (PRV (pvoie->finf.flags))
				{
					ch_bit (pvoie->ncur, &frec, F_PRV, Non);
					pvoie->finf.flags &= (~F_PRV);
					opt_choix (Non);
				}
				else
				{
					ch_bit (pvoie->ncur, &frec, F_PRV, Oui);
					pvoie->finf.flags |= (F_PRV);
					opt_choix (Oui);
				}
				texte (T_MBL + 51);
			}
			else
			{
				sprintf (varx[0], "O%c", c);
				texte (T_ERR + 1);
			}
			break;

		default:				/* Erreur */
			error = 1;
			--indd;
			/*
			   sprintf(varx[0], "O%c", c) ;
			   texte(T_ERR + 1) ;
			 */
			break;
		}
	}
	else
	{
		if (PAG (pvoie->finf.flags))
			texte (T_MBL + 23);
		else
			texte (T_MBL + 22);

		if (droits (CONSMES))
		{
			if (PRV (pvoie->finf.flags))
				opt_choix (Oui);
			else
				opt_choix (Non);
			texte (T_MBL + 51);
		}

		if (NEW (pvoie->finf.flags))
			opt_choix (Oui);
		else
			opt_choix (Non);
		texte (T_MBL + 52);

		texte (2);
/************ SERA A SUPPRIMER -> REMPLACE PAR $q ************/
		ltoa (1000L * (long) pvoie->finf.on_base, varx[0], 10);
		texte (T_MBL + 26);
	}
	if (!pvoie->read_only && ok)
	{
		if (pvoie->ncur->coord == 0xffff)
			dump_core ();
		fptr = ouvre_nomenc ();
		fseek (fptr, (long) pvoie->ncur->coord * sizeof (info), 0);
		fread (&frec, sizeof (info), 1, fptr);
		frec.nbl = pvoie->finf.nbl;
		frec.lang = pvoie->finf.lang;
		fseek (fptr, (long) pvoie->ncur->coord * sizeof (info), 0);
		fwrite (&frec, sizeof (info), 1, fptr);
		ferme (fptr, 36);
	}
	return (error);
}

int ch_language (int nb, ind_noeud * noeud, info * frec)
{
	int ok = 1;
	FILE *fptr;

	if ((nb < 1) || (nb > maxlang))
		ok = 0;
	else if (!pvoie->read_only)
	{
		if (noeud->coord == 0xffff)
			dump_core ();
		fptr = ouvre_nomenc ();
		fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
		fread (frec, sizeof (info), 1, fptr);
		frec->lang = nb - 1;
		fseek (fptr, ((long) noeud->coord * sizeof (info)), 0);
		fwrite (frec, sizeof (info), 1, fptr);
		ferme (fptr, 36);
	}
	return (ok);
}
