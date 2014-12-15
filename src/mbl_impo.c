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

static long skip_header (FILE * fptr)
{
	char ligne[256];
	long record = 0L;

	record = ftell (fptr);

	while (fgets (ligne, 256, fptr))
	{
		strupr (ligne);
		if ((ligne[0] != '\n') &&
			(strncmp (ligne, "R:", 2) != 0) &&
			(strncmp (ligne, "TO:", 3) != 0) &&
			(strncmp (ligne, "TO  :", 5) != 0) &&
			(strncmp (ligne, "FROM:", 5) != 0))
			break;
		record = ftell (fptr);
	}
	return (record);
}



int mess_fic (void)
{
	int fd_orig, fd_dest;
	FILE *fptr;
	char *ptr;
	int verbose = 0;
	int header = 0;
	int append = 0;
	int access;
	bullist *pbul;
	long numess;
	long nb_oct = 1L;
	char fichier[256];

	ptr = indd;
	if (toupper (*indd) == 'V')
	{
		verbose = 1;
		++indd;
	}
	else if (toupper (*indd) == 'H')
	{
		header = 1;
		++indd;
	}

	if (toupper (*indd) == 'A')
	{
		append = 1;
		++indd;
	}

	if (ISGRAPH (*indd))
		return (1);

	if (read_only ())
	{
		retour_mbl ();
		return (0);
	}

	fbb_log (voiecur, 'M', strupr (ptr));
	strcpy (pvoie->dos_path, "\\");
	pvoie->temp1 = N_MBL;

	incindd ();
	if (((isdigit (*indd)) && ((numess = lit_chiffre (1)) != 0L)) ||
		((numess = ptmes->numero) > 0L))
	{
		if (((pbul = ch_record (NULL, numess, ' ')) != NULL) && (droit_ok (pbul, 1)))
		{
			while (isdigit (*indd))
				++indd;
			incindd ();
			strupr (sup_ln (indd));
			if (*indd)
			{
				if ((!tst_point (indd)) || (!aut_ecr (ch_slash (indd), 1)))
				{
					retour_mbl ();
					return (0);
				}
				strcpy (fichier, tot_path (ch_slash (indd), pvoie->dos_path));
				if ((fptr = ouvre_mess (O_TEXT, numess, '\0')) != NULL)
				{
					fd_orig = fileno (fptr);
					if (!verbose)
						lseek (fd_orig, skip_header (fptr), 0);
				}
				else
				{
					retour_mbl ();
					return (0);
				}

				access = O_WRONLY | O_CREAT | O_TEXT;
				if (append)
					access |= O_APPEND;
				else
					access |= O_TRUNC;
				if ((fd_dest = open (fichier, access, S_IREAD | S_IWRITE)) == EOF)
				{
					strn_cpy (40, pvoie->appendf, indd);
					texte (T_ERR + 30);
					fclose (fptr);
					retour_mbl ();
					return (0);
				}

				if (*ptmes->bbsv)
					sprintf (varx[0], "@%-6s  ", ptmes->bbsv);
				else
					*varx[0] = '\0';
				if (header || verbose)
				{
					ptr = expand (langue[vlang]->plang[T_MBL + 35 - 1]);
					nb_oct += (long) write (fd_dest, ptr, strlen (ptr));
					ptr = expand (langue[vlang]->plang[T_MBL + 38 - 1]);
					nb_oct += (long) write (fd_dest, ptr, strlen (ptr));
					nb_oct += 2L;
				}
				else if (append)
					lseek (fd_dest, 0, SEEK_END);

				nb_oct += copy_fic (fd_orig, fd_dest, NULL);

				fclose (fptr);
				close (fd_dest);
				wr_dir (fichier, pvoie->sta.indicatif.call);
				w_label (fichier, ptmes->titre);
				ltoa (nb_oct, varx[0], 10);
				texte (T_DOS + 7);
			}
			else
				texte (T_ERR + 20);
		}
		else
			texte (T_ERR + 10);
	}
	else
		texte (T_ERR + 3);
	retour_mbl ();
	return (0);
}


int routage (long no)
{
	int c, ok;
	int route = 0, n = 0;
	long curr, rec = 0L;
	FILE *fptr;
	char ligne[256];
	char *scan, *ptr;

	strn_cpy (40, ptmes->bbsv, mycall);
	mess_name (MESSDIR, no, ligne);
	if ((fptr = fopen (ligne, "rb")) != NULL)
	{							/* Binaire obligatoire */
		curr = 0L;
		while (fgets (ligne, 255, fptr))
		{
			if (strncmp ("R:", ligne, 2) == 0)
			{
				scan = ligne;
				while (*scan)
				{
					if (*scan == '@')
					{
						rec = curr;
						break;
					}
					++scan;
					++curr;
				}
			}
			else
				break;
			curr = ftell (fptr);
		}
		if (rec)
		{
			route = 1;
			fseek (fptr, rec, 0);
			ptr = ptmes->bbsv;
			ok = 0;
			while ((c = fgetc (fptr)) != EOF)
			{
				if ((!ok) && (isalnum (c)))
					ok = 1;
				if (ok)
				{
					if (ISGRAPH (c))
					{
						*ptr++ = toupper (c);
						if (++n == 40)
							break;
					}
					else
						break;
				}
			}
			*ptr = '\0';
		}
		fclose (fptr);
	}
	return (route);
}

static void lit_fich (char *champ, int nb)
{
	while ((nb--) && (ISGRAPH (*indd)))
	{
		*champ++ = toupper (*indd);
		indd++;
	}
	*champ = '\0';
}


void send_reply (void)
{
	long numess;
	bullist *pbul;
	char temp[80];

	incindd ();
	if (((isdigit (*indd) || (*indd == '#')) &&
		 ((numess = lit_chiffre (1)) != 0L)) ||
		((numess = ptmes->numero) > 0L))
	{
		if ((pbul = ch_record (NULL, numess, '\0')) != NULL)
		{
			ini_champs (voiecur);
			strn_cpy (6, ptmes->desti, pbul->exped);
			strn_cpy (6, ptmes->exped, pvoie->sta.indicatif.call);
			routage (numess);
			swapp_bbs (ptmes);
			ptmes->type = 'P';
			ptmes->status = 'N';
			reacheminement ();
			teste_espace ();
			sup_ln (indd);
			if ((*indd == '+') && (droits (ACCESDOS)))
			{
				incindd ();
				lit_fich (pvoie->appendf, 79);
				if (access (pvoie->appendf, 0) != 0)
				{
					texte (T_ERR + 11);
					retour_mbl ();
					return;
				}
				incindd ();
			}
			if (*indd)
			{
				n_cpy (60, ptmes->titre, indd);
			}
			else
			{
				/* strn_cpy (3, temp, pbul->titre); */
				if (strncmpi (pbul->titre, "RE:", 3) == 0)
					strcpy (ptmes->titre, pbul->titre);
				else if (strncmpi (pbul->titre, "CP SYSOP: ", 10) == 0)
					strcpy (ptmes->titre, 10 + pbul->titre);
				else
				{
					n_cpy (55, temp, pbul->titre);
					sprintf (ptmes->titre, "Re: %s", temp);
				}
			}
			
			texte (T_MBL + 50);
			entete_saisie ();
			maj_niv (15, 0, 2);
			texte (T_MBL + 6);

			if ((sed) && (EditorOff) && (voiecur == CONSOLE))
			{
				aff_etat ('E');
				/* send_buf (voiecur); */
				reply = 1;
				pvoie->enrcur = numess;
#ifdef __WINDOWS__
				maj_niv (N_MBL, 3, 0);
				if (win_edit () == 5)
					end_win_edit ();
#endif
#ifdef __LINUX__
				if (daemon_mode)
					editor_request = 1;
				else 
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
			return;
		}
		else
			texte (T_ERR + 10);
	}
	else
		texte (T_ERR + 3);
	retour_mbl ();
}


void send_copy (void)
{
	bullist *pbul;
	int retour = 1;

	incindd ();
	if (((isdigit (*indd)) && ((ptmes->numero = lit_chiffre (1)) != 0L)) || ((*indd == '#') && (ptmes->numero > 0L)))
	{
		if (((pbul = ch_record (NULL, ptmes->numero, ' ')) != NULL) && (droit_ok (pbul, 1)))
		{
			if (copy_mess (ptmes->numero, indd, '\0'))
				retour = 0;
		}
		else
			texte (T_ERR + 10);
	}
	else
	{
		texte (T_ERR + 3);
	}
	if (retour)
		retour_mbl ();
}


/* Ecrit les headers reduits dans data */
static long reduit_message (long numero, char *filename, char *last_header)
{
	int file;
	FILE *fptr;
	int c, first = 1;
	int nb = 0;
	int call = 0;
	char ligne[90];
	char header[90];
	char *ptr = ligne;
	int flag = FALSE;
	long record = 0L;
	int access = O_WRONLY | O_CREAT | O_TEXT | O_TRUNC;
	short int postexte = 0;
	char *hdr = header;

	*hdr = '\0';
	*last_header = '\0';

	if ((fptr = ouvre_mess (O_TEXT, numero, '\0')) == NULL)
	{
		return (0);
	}

	if ((file = open (filename, access, S_IREAD | S_IWRITE)) == EOF)
	{
		fclose (fptr);
		return (0);
	}

	while ((c = fgetc (fptr)) != EOF)
	{
		if ((flag) && (c == '\n'))
		{
			record = ftell (fptr);
			postexte = 0;
			call = 0;
			flag = FALSE;
			*hdr = '\0';
			strcpy (last_header, header);
			hdr = header;
		}
		else
		{
			switch (call)
			{
			case 0:
				break;
			case 1:
				if (isalnum (c))
				{
					*ptr++ = c;
					nb++;
					call = 2;
				}
				break;
			case 2:
				if (isalnum (c))
				{
					*ptr++ = c;
					nb++;
				}
				else
				{
					*ptr++ = '!';
					nb++;
					call = 0;
					if (nb >= 65)
					{
						*ptr++ = '\n';
						++nb;
						write (file, ligne, nb);
						nb = 0;
						ptr = ligne;
						first = 2;
					}
				}
				break;
			}
			if (postexte == 0)
			{
				if (c != 'R')	/*return(record) */
					break;
				else
					flag = TRUE;
			}
			if ((postexte == 1) && (flag) && (c != ':'))	/*return(record) */
			{
				flag = FALSE;
				break;
			}
			++postexte;
			if (postexte < 80)
				*hdr++ = c;
		}
		if ((flag) && (c == '@'))
		{
			if (first)
			{
				if (first == 1)
					write (file, "Path: !", 7);
				else
					write (file, "      !", 7);
				first = 0;
			}
			call = 1;
		}
	}

	if (nb)
	{
		*ptr++ = '\n';
		++nb;
		write (file, ligne, nb);
	}

	fseek (fptr, record, 0);

	fflush (fptr);
	copy_fic (fileno (fptr), file, NULL);

	fclose (fptr);
	close (file);

	return (1);
}


int copy_mess (long numero, char *chaine, char car_fin)
{
	int i;
	int test;
	char temp[256];
	char last_header[128];
	bullist *pbul;
	int retour = 1;

	if ((pbul = ch_record (NULL, numero, '\0')) != NULL)
	{
		ptmes->status = 'N';
		ptmes->type = 'P';
		ptmes->taille = 0L;
		ptmes->theme = 0;
		ptmes->numero = 0L;
		pvoie->messdate = time (NULL);
		pvoie->mess_num = -1L;
		*(pvoie->mess_bid) = '\0';
		*(ptmes->desti) = '\0';
		*(ptmes->bbsv) = '\0';
		*(ptmes->bbsf) = '\0';
		*(ptmes->bid) = '\0';
		strcpy (pvoie->appendf, copy_name (voiecur, temp));
		strn_cpy (6, ptmes->exped, pbul->exped);
		for (i = 0; i < NBMASK; i++)
			ptmes->forw[i] = ptmes->fbbs[i] = '\0';

		if (strcmp(pvoie->sta.indicatif.call, "SYSOP") == 0)
		{
			/* SYSOP private message keeps the original sender */
			strn_cpy (6, ptmes->exped, pbul->exped);
			/* Copy the bbsf field to avoid the from/to lines... */
			strn_cpy (6, ptmes->bbsf, pbul->bbsf);
		}
		else
		{
			/* Not a sysop message */
			strn_cpy (6, ptmes->exped, pvoie->sta.indicatif.call);
		}
		n_cpy (48, temp, pbul->titre);
		sprintf (ptmes->titre, "CP %s: %s", pvoie->sta.indicatif.call, temp);

		indd = chaine;
		if (*indd == '#')
			*indd = 'P';
		if (scan_com_fwd ())
		{
			if (deja_recu (ptmes, 1, &test) == 1)
			{
				texte (T_MBL + 45);
				retour = 0;
			}
			else
			{
				struct tm *sdate;

				reduit_message (numero, pvoie->appendf, last_header);
				sdate = gmtime (&pvoie->messdate);
				sprintf (temp, "%s\r\rOriginal to %s@%s\r\r%c",
						 last_header, pbul->desti, pbul->bbsv, car_fin);

				get_mess_fwd ('C', temp, strlen (temp), 2);
				if (car_fin == '\0')
				{
					maj_niv (15, 0, 2);
					texte (T_MBL + 6);
				}
			}
		}
		else
			retour = 0;
	}
	else
	{
		texte (T_ERR + 10);
		retour = 0;
	}
	return (retour);
}


void import_message (char *nom_fich)
{
	int i;
	int fd;
	int type, retour;
	int test;
	int recu;
	int sav_mode;
	FILE *fptr;
	char s[80];
	char ligne[81];
	char temp[256];
	char lock_name[256];


	retour = 1;
	aff_etat ('I');
	while (hupdate ());
	aff_header (voiecur);
	sav_mode = pvoie->mode;
	pvoie->lignes = -1;
	pvoie->finf.lang = langue[0]->numlang;

	strcpy (lock_name, lfile (nom_fich));

	for (i = 0; i < 2; i++)
	{
		/* Checks for the lock file and creates it */
		fd = open (lock_name, O_EXCL | O_CREAT, S_IREAD | S_IWRITE);
		if (fd == -1)
		{
#ifdef __WIN32__
			long tt;
			WIN32_FIND_DATA ff;
			SYSTEMTIME t;
			FILETIME ft;

			if (errno != EEXIST)
			{
				sprintf (temp, "*** error : Cannot create lock file %s\r", lock_name);
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				retour = 2;
				break;
			}

			if (FindFirstFile ((char *) lock_name, &ff) != (HANDLE) - 1)
			{
				FileTimeToSystemTime (&ff.ftLastWriteTime, &t);
				sprintf (temp, "*** File   %02d:%02d:%02d\r",
						 t.wHour, t.wMinute, t.wSecond);
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));

				GetSystemTime (&t);
				SystemTimeToFileTime (&t, &ft);

				sprintf (temp, "*** System %02d:%02d:%02d\r",
						 t.wHour, t.wMinute, t.wSecond);
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));

				/* Add one hour to File time (approximative) */
				ff.ftLastWriteTime.dwHighDateTime += 9;
				if (CompareFileTime (&ft, &ff.ftLastWriteTime) > 0L)
				{
					/* If more than 1 hour, delete the lock file if possible */
					unlink (lock_name);
					sprintf (temp, "*** file %s was locked since more than 1 hour, lock deleted !!\r",
							 nom_fich);
					aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				}
				else
				{
					sprintf (temp, "*** file %s locked !!\r", nom_fich);
					aff_bas (voiecur, W_RCVT, temp, strlen (temp));
					retour = 2;
					break;
				}
			}
#else
			/* Checks the date of the lock file */
			struct stat statbuf;
			long t = time (NULL);

			if (errno != EEXIST)
			{
				sprintf (temp, "*** error : Cannot create lock file %s\r", lock_name);
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				retour = 2;
				break;
			}

			if ((stat (lock_name, &statbuf) == 0) && ((t - statbuf.st_ctime) > 3600L))
			{
				/* If more than 1 hour, delete the lock file if possible */
				if (unlink (lock_name) != 0)
				{
					sprintf (temp, "*** error : Cannot unlink lock file %s, %s\r", lock_name, strerror (errno));
					aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				}
				sprintf (temp, "*** file %s was locked from more than 1 hour, lock deleted !!\r",
						 nom_fich);
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));
			}
			else
			{
				sprintf (temp, "*** file %s locked !!\r", nom_fich);
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				retour = 2;
				break;
			}
#endif
		}
		else
			break;
	}

	if (fd != -1)
	{
		close (fd);
		if ((fptr = fopen (nom_fich, "rt")) != NULL)
		{
			long pos = svoie[mail_ch]->enrcur;

			fseek (fptr, pos, 0);

			while (1)
			{
				type = 2;
				libere_route (voiecur);
				if (fgets (ligne, 80, fptr) == NULL)
					break;
				lf_to_cr (ligne);
				if (*ligne == '#')
				{
					pvoie->mode = 0;
					continue;
				}
				if ((*ligne == '\r') || (toupper (*ligne) != 'S') || ((ligne[1] != ' ') && (ligne[2] != ' ')))
					continue;

				if (aff_inexport)
					aff_bas (voiecur, W_RCVT, ligne, strlen (ligne));
				indd = ligne + 1;
				if (lit_com_fwd () == 0)
				{
					outln ("NO", 2);
					type = 3;
				}
				recu = deja_recu (&(svoie[voiecur]->entmes), 1, &test);
				if ((recu == 1) || (recu == 4))
				{
					sprintf (s, "N - Bid %s", svoie[voiecur]->entmes.bid);
					outln (s, strlen (s));
					type = 3;
				}
				if (type == 2)
				{
					/*              reacheminement() ; */
					outln ("OK", 2);
				}
				aff_etat ('E');
				send_buf (voiecur);
				if (fgets (ligne, 80, fptr) == NULL)
					break;
				lf_to_cr (ligne);
				if (type == 2 && aff_inexport)
					aff_bas (voiecur, W_RCVT, ligne, strlen (ligne));
				indd = sup_ln (ligne);
				rcv_titre ();
				while (1)
				{
					if (fgets (ligne, 80, fptr) == NULL)
						break;
					lf_to_cr (ligne);
					if (type == 2 && aff_inexport)
						aff_bas (voiecur, W_RCVT, ligne, strlen (ligne));

					/* Closes the mail.in in case of calling a server */
					if ((strchr (ligne, 'Z' - '@') != NULL) || (strncmpi (ligne, "/EX", 3) == 0))
					{
						svoie[mail_ch]->enrcur = ftell (fptr);
#ifdef __WIN32__
						/* One more than true position in WIN32 ... Why ??? */
						if (svoie[mail_ch]->enrcur > 0L)
							--svoie[mail_ch]->enrcur;
#endif
						ferme (fptr, 7);
						fptr = NULL;
						if (unlink (lock_name) != 0)
						{
							sprintf (temp, "*** error : Cannot unlink lock file %s, %s\r", lock_name, strerror (errno));
							aff_bas (voiecur, W_RCVT, temp, strlen (temp));
						}
					}

					if (get_mess_fwd ('I', ligne, strlen (ligne), type))
					{
						retour = 0;
						break;
					}
					free_mem ();
				}
				break;
			}
			if (fptr)
			{
				svoie[mail_ch]->enrcur = ftell (fptr);
#ifdef __WIN32__
				/* One more than true position in WIN32 ... Why ??? */
				if (svoie[mail_ch]->enrcur > 0L)
					--svoie[mail_ch]->enrcur;
#endif
				ferme (fptr, 7);
				if (unlink (lock_name) != 0)
				{
					sprintf (temp, "*** error : Cannot unlink lock file %s, %s\r", lock_name, strerror (errno));
					aff_bas (voiecur, W_RCVT, temp, strlen (temp));
				}
			}
		}
		else
		{
			char buf[80];

			sprintf (buf, "*** Cannot import %s\r", nom_fich);
			/* outln (buf, strlen(buf)); */
			aff_bas (voiecur, W_RCVT, buf, strlen (buf));
			if (unlink (lock_name) != 0)
			{
				sprintf (temp, "*** error : Cannot unlink lock file %s, %s\r", lock_name, strerror (errno));
				aff_bas (voiecur, W_RCVT, temp, strlen (temp));
			}
		}
	}

	/* Remet le mode par defaut */
	pvoie->mode = sav_mode;

	if (retour)
	{
		if (aff_inexport)
			aff_bas (voiecur, W_RCVT, "*** done\r", 9);
		if ((retour == 1) && (strcmp (io_fich, MAILIN) == 0))
			unlink (MAILIN);
		inexport = 0;
		deconnexion (voiecur, 1);
#if defined(__WINDOWS__) || defined(__LINUX__)
		window_disconnect (voiecur);
#endif
		init_etat ();
	}
	else
	{
		outln ("Import>", 7);
		aff_etat ('E');
		send_buf (voiecur);
	}
	aff_etat ('A');
	status (voiecur);
}
