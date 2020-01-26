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
 * Gestion des serveurs de donnees. SERVEUR.H
 */

#include <serv.h>

static int is_redist (char *);

void tst_serveur (bullist * pbul)
{
	serlist *lptr;
	bullist psauve;

	if ((strcmp (pbul->desti, "WP") == 0) &&
		(strcmp (pbul->exped, mycall) != 0) &&
		(is_wpupdate (pbul->titre)))
	{							/* WP Update ? */
		wp_read_mess (pbul);	/* Lit le message pour MAJ WP */

		return;
	}

	if ((*pbul->bbsv) || (pbul->type != 'P'))
		return;

	psauve = *pbul;

	if (is_redist (pbul->desti))
	{
		lptr = tete_serv;
		while (lptr)
		{
			if (strcmp (lptr->nom_serveur, "REDIST") == 0)
			{
				appel_serveur (lptr, pbul);
				break;
			}
			lptr = lptr->suiv;
		}
	}
	else
	{
		lptr = tete_serv;
		while (lptr)
		{
			if (strcmp (lptr->nom_serveur, pbul->desti) == 0)
			{
				appel_serveur (lptr, pbul);
				break;
			}
			lptr = lptr->suiv;
		}
	}
	*pbul = psauve;
}


int p_cmd (void)
{
	serlist *lptr;
	char s[200];
	int ok = 1;
	int c = toupper (*indd);

	switch (c)
	{

	case 'G':
		incindd ();
		maj_niv (N_MBL, 17, 0);
		exec_pg ();
		ok = 2;
		break;

	case 'R':
		ok = mbl_print ();
		break;

	case 'S':
		lptr = tete_serv;
		while (lptr)
		{
			sprintf (s, "%6s: %s", lptr->nom_serveur, lptr->com_serveur);
			outln (s, strlen (s));
			lptr = lptr->suiv;
		}
		break;

	default:
		ok = 0;
		break;
	}

	if (ok == 1)
		retour_mbl ();
	return (ok);
}


void libere_serveurs (void)
{
	serlist *lptr;

	while (tete_serv)
	{
		lptr = tete_serv;
		tete_serv = tete_serv->suiv;
		m_libere (lptr, sizeof (serlist));
	}
}

void init_serveur (char *ligne, int nolig)
{
	char nom[80], pg[80], com[80];
	serlist *lptr;

	*com = '\0';
	if (sscanf (ligne, "%s %s %[^\n\r]", nom, pg, com) < 2)
		err_init (nolig);

	strupr (nom);

	/* Cherche si le service existe deja */
	lptr = tete_serv;
	while (lptr)
	{
		if (strcmp (lptr->nom_serveur, nom) == 0)
			return;
		lptr = lptr->suiv;
	}

	lptr = tete_serv;
	if (lptr)
	{
		while (lptr->suiv)
		{
			lptr = lptr->suiv;
		}
		lptr->suiv = (serlist *) m_alloue (sizeof (serlist));
		lptr = lptr->suiv;
	}
	else
	{
		tete_serv = lptr = (serlist *) m_alloue (sizeof (serlist));
	}
	lptr->suiv = NULL;
	strn_cpy (6, lptr->nom_serveur, nom);
	strcpy (lptr->nom_pg, pg);
	strcpy (lptr->com_serveur, com);
}

#ifdef __WINDOWS__
#pragma argsused
#endif
void affich_serveurs (int tp)
{
#ifdef __linux__
	int tot = 0;
	char text[80];
	serlist *lptr = tete_serv;

	while (lptr)
	{
		++tot;
		sprintf (text, "%d: %s", tot, lptr->nom_serveur);
		InitText (text);
		lptr = lptr->suiv;
	}
#endif
#ifdef __WINDOWS__
	int tot = 0;
	char text[80];
	serlist *lptr = tete_serv;

	while (lptr)
	{
		++tot;
		wsprintf (text, "%d: %s %s", tot, lptr->nom_serveur, lptr->com_serveur);
		InitText (text);
		lptr = lptr->suiv;
	}
#endif
#ifdef __FBBDOS__
	fen *fen_ptr;
	serlist *lptr;
	int size, nb_serv = 0;

	lptr = tete_serv;
	while (lptr)
	{
		++nb_serv;
		lptr = lptr->suiv;
	}

	if (nb_serv == 0)
		return;

	size = (nb_serv > 19) ? 19 : nb_serv;
	lptr = tete_serv;
	deb_io ();

#ifdef ENGLISH
	fen_ptr = open_win (10, 2, 75, 4 + size, INIT, "Servers ");
	while (lptr)
	{
		cprintf ("Server %s: %s \r\n", lptr->nom_serveur, lptr->com_serveur);
		lptr = lptr->suiv;
	}
	attend_caractere (tp);
	close_win (fen_ptr);
#else
	fen_ptr = open_win (10, 2, 75, 4 + size, INIT, "Services");
	while (lptr)
	{
		cprintf ("Service %s: %s\r\n", lptr->nom_serveur, lptr->com_serveur);
		lptr = lptr->suiv;
	}
	attend_caractere (tp);
	close_win (fen_ptr);
#endif

	fin_io ();
#endif /* __WINDOWS__ */
}

static int is_redist (char *server_name)
{
	if (
		   (strcmp (server_name, "REDIST") == 0) ||
		   (strcmp (server_name, "LOCBBS") == 0) ||
		   (strcmp (server_name, "LOCAL") == 0) ||
		   (strcmp (server_name, "REGION") == 0) ||
		   (strcmp (server_name, "NATION") == 0)
		)
	{
		return (1);
	}
	return (0);
}


int appel_serveur (serlist * lptr, bullist * pbul)
{
	static int num_file = 1;

	ind_noeud *noeud;
	mess_noeud *mptr;

	FILE *fptr;
	FILE *fptm;
	char nom_fichier[80];
	char texte[80];

#ifdef __FBBDOS__
	int disk;
	int duplic;
	char cur_dir[MAXPATH];
	char localdir[61];
#endif

#ifdef __WINDOWS__
	char localdir[61];
#endif

	int retour = 0;
	int supprime = 1;

	/* Affiche le nom du service dans le bandeau */
	sprintf (texte, "%-7s", lptr->nom_serveur);
	aff_chaine (W_STAT, 2, 4, texte);

	/* Cree le fichier copie */
#ifdef __linux__
	sprintf (nom_fichier, "%sl_serv%02d.in", DATADIR, num_file);
	if ((fptr = fopen (nom_fichier, "w")) != NULL)
#endif
#ifdef __WINDOWS__
	sprintf (nom_fichier, "%s\\w_serv%02d.IN", getcwd (localdir, 60), num_file);
	if ((fptr = fopen (nom_fichier, "wt")) != NULL)
#endif
#ifdef __FBBDOS__
	sprintf (nom_fichier, "%s\\d_serv%02d.IN", getcwd (localdir, 60), num_file);
	if ((fptr = fopen (nom_fichier, "wt")) != NULL)
#endif
	{
		fprintf (fptr, "SP %s < %s\n", pbul->desti, pbul->exped);
		fprintf (fptr, "%s\n", pbul->titre);
		fflush (fptr);

		/* Fait une copie du message original */

		if ((fptm = ouvre_mess (O_TEXT, pbul->numero, pbul->status)) != NULL)
		{
			fflush (fptr);
			fflush (fptm);
			copy_fic (fileno (fptm), fileno (fptr), NULL);
			fprintf (fptr, "\n/EX\n");
			ferme (fptm, 75);
			ferme (fptr, 76);

			/* Teste les services "speciaux" */

			if ((*lptr->nom_pg == '*') && strcmp ("REQCFG", lptr->nom_serveur) == 0)
			{
				retour = req_cfg (nom_fichier);
			}
			else if ((*lptr->nom_pg == '*') && strcmp ("REDIST", lptr->nom_serveur) == 0)
			{
				retour = redist (nom_fichier);
			}
			else if ((*lptr->nom_pg == '*') && strcmp ("WP", lptr->nom_serveur) == 0)
			{

				if (strcmp (pbul->exped, mycall) == 0)
				{				/* Exped = moi ? */

					if (*pbul->bbsv)
						supprime = 0;	/* Ne supprime pas le message */
					else
						retour = wp_service (nom_fichier);
				}

				else if (*pbul->bbsv == '\0')
					retour = wp_service (nom_fichier);	/* Exped = autre */

				else
					supprime = 0;

			}
			else
			{
#ifdef __linux__
				char commande[256];
				FILE *fp;

				sprintf (commande, "cd %s ; ./%s %s", SERVDIR, lptr->nom_pg, back2slash(nom_fichier));

				fp = popen (commande, "r");
				if (fp == NULL)
					printf ("Failed to run command\n" );
				else
					retour = pclose(fp);

				retour = retour >> 8;

				if (++num_file == 10)
					num_file = 1;
#endif
#ifdef __WINDOWS__
				char commande[256];
				char *extension;

				extension = strrchr (lptr->nom_pg, '.');
				if (extension && (strcmp (extension, ".DLL") == 0))
				{
					/* DLL appel immediat... */
					HINSTANCE hinstFilter;

					retour = -1;

					hinstFilter = LoadLibrary (lptr->nom_pg);
					if (hinstFilter > HINSTANCE_ERROR)
					{
						int (FAR PASCAL * DllProc) (int ac, char FAR ** av);
						int ac = 0;
						char *av[30];

						ac = 0;
						av[ac++] = lptr->nom_pg;
						av[ac++] = nom_fichier;
						av[ac] = NULL;

						(FARPROC) DllProc = GetProcAddress (hinstFilter, "svc_main");
						if (DllProc)
							retour = (*DllProc) (ac, (char FAR **) av);

						FreeLibrary (hinstFilter);
					}
				}
				else
				{
					sprintf (commande, "%s %s", lptr->nom_pg, nom_fichier);


					/* Appel serveurs via Windows */
					retour = (fbb_exec (commande) == 1) ? 0 : -1;
				}
				if (++num_file == 10)
					num_file = 1;
#endif
#ifdef __FBBDOS__
				deb_io ();		/* Stoppe le multitaches */
				disk = getdisk ();
				strcpy (cur_dir, "X:\\");
				cur_dir[0] = 'A' + disk;
				getcurdir (0, cur_dir + 3);

				operationnel = FALSE;	/* Stoppe les stats de taches */
				duplic = dup (1);	/* Ferme le stdout */
				close (1);
				break_ok ();	/* Valide le Ctrl C */
				retour = spawnlp (P_WAIT, lptr->nom_pg, lptr->nom_serveur, nom_fichier, NULL);
				break_stop ();	/* Devalide le Ctrl C */
				dup2 (duplic, 1);	/* Remet le stdout */
				close (duplic);
				operationnel = TRUE;
				setdisk (disk);
				chdir (cur_dir);
				fin_io ();
				unlink (nom_fichier);
#endif /* __WINDOWS__ */

			}
			if ((retour != 0) && (retour != 10))
			{
				/* Erreur de lancement : Envoi d'un message SYSOP */
#ifdef ENGLISH
				cprintf ("Error starting server %s (%s)\r\n", lptr->nom_serveur, lptr->nom_pg);
				sprintf (texte, "Error starting server %s (%s)\n", lptr->nom_serveur, lptr->nom_pg);
				if (w_mask & W_SERVER)
					mess_warning (admin, "*** SERVER ERROR ***  ", texte);
#else
				cprintf ("Erreur appel serveur %s (%s) \r\n", lptr->nom_serveur, lptr->nom_pg);
				sprintf (texte, "Erreur appel serveur %s (%s) \n", lptr->nom_serveur, lptr->nom_pg);
				if (w_mask & W_SERVER)
					mess_warning (admin, "*** ERREUR SERVEUR ***", texte);
#endif
			}
			if (retour == 10)
				supprime = 0;
		}
		else
			ferme (fptr, 77);
		timeprec = 0L;
	}


	if (supprime)
	{
		if ((mptr = findmess (pbul->numero)) != NULL)
		{						/* Tue le message */
			ouvre_dir ();
			pbul->status = 'K';
			write_dir (mptr->noenr, pbul);
			ferme_dir ();
			noeud = cher_noeud (pbul->desti);
			--(noeud->nbnew);
			--(noeud->nbmess);
			chg_mess (0xffff, pbul->numero);	/* Plus de destinataire */
		}
	}

	return (supprime);
}

int ptctrx (int port, char *cmde)
{
	int pp;
	int ok = 0;
	char cmdbuf[80];

	for (pp = 1; pp < NBPORT; pp++)
	{
		if ((HST (pp)) && (p_port[pp].moport & 0x80))
		{
			sprintf (cmdbuf, "!%s", cmde + 3);
			tnc_commande (pp, cmdbuf, PORTCMD);
			ok = 1;
			break;
		}
	}

	/* Les autres ports attendent deux secondes
	   pour etre sur que la commande a ete envoyee */

	if ((ok) && (pp != port))
	{
		p_port[port].stop = 2;
	}

	return (ok);
}
