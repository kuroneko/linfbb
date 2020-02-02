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
 * TRAIT.C
 *
 */


#include <serv.h>
#include <setjmp.h>

static int cross_connexion (void);
static int dde_prenom (void);

static void accueil (void);
static void aff_new_message (void);
static void arret_serveur (char *, int);
static void inconnu (void);
static void insere_info (int);
static void majrelai (int);
static void majstat (int);
static void menu_serveur (int, int);
static void mess_cross_connect (void);
static void pont (void);

/*
 *  PROCEDURES GENERALES A TOUS LES NIVEAUX
 *  AIDE EN LIGNE - ECHANGE DE MESSAGES
 */

#define AIDE -1

int defaut (void)
{
	int i, sum_call;

	switch (toupper (*indd))
	{
	case '?':
	case 'H':
		++indd;
		if (ISPRINT (*indd))
		{
			while (isspace (*indd))
				++indd;
			help (indd);
		}
		else
			prompt (AIDE, pvoie->niv1);
		break;
	case '#':
		texte (T_MBL + 8);
		prompt (pvoie->finf.flags, pvoie->niv1);
		break;
	case '!':
		q_mark ();
		prompt (pvoie->finf.flags, pvoie->niv1);
		break;
	case '>':
		if (pvoie->read_only)
			return (0);
		pont ();
		prompt (pvoie->finf.flags, pvoie->niv1);
		break;
	case '=':
		if (pvoie->read_only)
			return (0);
		pvoie->sniv1 = pvoie->niv1;
		pvoie->sniv2 = pvoie->niv2;
		pvoie->sniv3 = pvoie->niv3;
		maj_niv (0, 4, 0);
		if (!cross_connexion ())
		{
			maj_niv (pvoie->sniv1, pvoie->sniv2, pvoie->sniv3);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		break;
	case '%':
		mbl_stat ();
		retour_menu (pvoie->niv1);
		break;
	case '/':
		incindd ();
		if (isdigit (*indd))
		{
			i = 0;
			sum_call = 0;
			while (isalnum (mycall[i]))
				sum_call += (int) mycall[i++];
			if (atoi (indd) == sum_call)
			{
				pvoie->sniv1 = pvoie->niv1;
				pvoie->sniv2 = pvoie->niv2;
				pvoie->sniv3 = pvoie->niv3;
				maj_niv (0, 3, 2);
				while (isdigit (*indd))
					++indd;
				arret_serveur (d_disque ("ETAT.SYS"), FALSE);
			}
			else
			{
				texte (T_ERR + 0);
				prompt (pvoie->finf.flags, pvoie->niv1);
			}
		}
		else
		{
			if (droits (CMDRESET))
			{
				pvoie->sniv1 = pvoie->niv1;
				pvoie->sniv2 = pvoie->niv2;
				pvoie->sniv3 = pvoie->niv3;
				maj_niv (0, 3, 0);
				arret_serveur (d_disque ("ETAT.SYS"), TRUE);
			}
			else
			{
				texte (T_ERR + 0);
				prompt (pvoie->finf.flags, pvoie->niv1);
			}
		}
		break;
	default:
		return (0);
	}
	return (1);
}


static int ok_break (int voie)
{
	return ((svoie[voie]->niv3 == 0) &&
			(svoie[voie]->niv1 != N_DOS) &&
			(svoie[voie]->niv1 != N_FORW) &&
			(svoie[voie]->niv1 != N_TELL) &&
			(svoie[voie]->niv1 != N_CONF) &&
			(svoie[voie]->niv1 != N_GATE) &&
			(svoie[voie]->niv1 != N_YAPP) &&
			(!FOR (svoie[voie]->mode)));
}


static int convers (int voie)
{
	return ((svoie[voie]->niv1 == 0) && (svoie[voie]->niv2 == 4) && (svoie[voie]->niv3 == 1));
}

int cross_connexion (void)
{
	int voie_exp, voie_dest;
	char temp[20];
	int i = 0;

	switch (pvoie->niv3)
	{
	case 0:
		++indd;
		while (!ISGRAPH (*indd))
			++indd;
		while (isalnum (*indd))
		{
			if (i < 10)
				temp[i++] = *indd;
			++indd;
		}
		temp[i] = '\0';
		while (*indd && (!ISGRAPH (*indd)))
			++indd;
		if (find (temp))
		{
			if ((voie_dest = num_voie (temp)) != -1)
			{
				if ((!FOR (svoie[voie_dest]->mode)) &&
					(svoie[voie_dest]->cross_connect == -1) &&
					(svoie[voie_dest]->kiss == -1))
				{
					pvoie->cross_connect = voie_dest;
					svoie[voie_dest]->cross_connect = voiecur;
					if (ok_break (voie_dest))
					{
						if (voie_dest != voiecur)
						{
							svoie[voie_dest]->sniv1 = svoie[voie_dest]->niv1;
							svoie[voie_dest]->sniv2 = svoie[voie_dest]->niv2;
							svoie[voie_dest]->sniv3 = svoie[voie_dest]->niv3;
							voie_exp = voiecur;
							selvoie (voie_dest);
							maj_niv (0, 4, 1);
							selvoie (voie_exp);
						}
						mess_cross_connect ();
					}
					else
						texte (T_TRT + 0);
					maj_niv (0, 4, 1);
					return (TRUE);
				}
				else
					texte (T_TRT + 1);
			}
			else
			{
				var_cpy (0, temp);
				texte (T_TRT + 2);
			}
		}
		else
			texte (T_ERR + 0);
		return (FALSE);
	case 1:
		voie_dest = pvoie->cross_connect;
		if (voie_dest >= 0)
		{
			voie_exp = voiecur;
			selvoie (voie_dest);
			if (!pvoie->sta.connect)
			{
				svoie[voie_exp]->cross_connect = pvoie->cross_connect = -1;
			}
			else
			{
				if (*indd)
				{
					if (*indd == CTRLZ)
					{
						svoie[voie_exp]->cross_connect = pvoie->cross_connect = -1;
						if ((pvoie->niv1 == svoie[voie_exp]->niv1) &&
							(pvoie->niv2 == svoie[voie_exp]->niv2) &&
							(pvoie->niv3 == svoie[voie_exp]->niv3))
						{
							maj_niv (pvoie->sniv1, pvoie->sniv2, pvoie->sniv3);
							texte (T_TRT + 4);
							prompt (pvoie->finf.flags, pvoie->niv1);
						}
						/*      break ; */
					}
					else if (convers (voie_dest))
					{
						outs (indd, strlen (indd));
					}
				}
			}
			selvoie (voie_exp);
		}
		if ((voie_dest != voiecur) && (pvoie->cross_connect == -1))
		{
			texte (T_TRT + 4);
			maj_niv (pvoie->sniv1, pvoie->sniv2, pvoie->sniv3);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		break;
	}
	return (TRUE);
}

static void arret_serveur (char *fich_etat, int typ)
{
	FILE *file_ptr;
	long caltemps;

	while ((*indd) && (!ISGRAPH (*indd)))
		indd++;
	switch (toupper (*indd))
	{
	case 'A':	/* Stop */
		arret = TRUE;
		texte (T_TRT + 9);
		send_buf (voiecur);
		maintenance ();
#ifdef __FBBDOS__
		exit (4);
#else
		fbb_quit (4);
#endif
		break;

	case 'K':	/* Maintenance */
		texte (T_TRT + 9);
		house_keeping ();
		break;

	case 'L':	/* Re-Run */
		texte (T_TRT + 9);
		save_fic = 1;
#ifdef __FBBDOS__
		affich_logo (W_DEFL);
#endif
		type_sortie = 2;
		set_busy ();
		break;

	case 'M':	/* Immediate Re-run */
		texte (T_TRT + 9);
		aff_etat ('E');
		send_buf (voiecur);
		attend_caractere (1);
		maintenance ();
		exit (2);

	case 'R':	/* Stop */
		if ((file_ptr = fopen (fich_etat, "r+t")) == NULL)
			fbb_error (ERR_OPEN, fich_etat, 1);
		else
		{
			fseek (file_ptr, 64L, 0);
			fprintf (file_ptr, "Reset demande par %s-%c le %s\n",
					 pvoie->sta.indicatif.call, pvoie->sta.indicatif.num,
					 strdate (time (&caltemps)));
			ferme (file_ptr, 52);
		}
		texte (T_TRT + 10);
		maintenance ();			/* reboot serveur */
#ifdef __linux__
		exit (6);
#endif
#ifdef __WINDOWS__
		ExitWindows (EW_REBOOTSYSTEM, 0);
#endif
#ifdef __FBBDOS__
		reset ();
#endif
		break;
	default:
		cmd_err (indd);
		break;
	}
	maj_niv (pvoie->sniv1, pvoie->sniv2, pvoie->sniv3);
	prompt (pvoie->finf.flags, pvoie->niv1);
}


static void pont (void)
{
	int voie_exp, voie_dest;
	char temp[20];
	int i = 0;

	++indd;
	while (!ISGRAPH (*indd))
		++indd;
	while (isalnum (*indd))
	{
		if (i < 10)
			temp[i++] = *indd;
		++indd;
	}
	temp[i] = '\0';
	while (*indd && (!ISGRAPH (*indd)))
		++indd;
	if (find (temp))
	{
		if (*indd)
		{
			if ((voie_dest = num_voie (temp)) != -1)
			{
				if (ok_break (voie_dest))
				{
					voie_exp = voiecur;
					selvoie (voie_dest);	/* Ajout */
					var_cpy (0, svoie[voie_exp]->sta.indicatif.call);
					texte (T_TRT + 5);
					outs (indd, strlen (indd));
					cr_cond ();
					texte (T_TRT + 6);
					selvoie (voie_exp);		/* Ajout */
					var_cpy (0, svoie[voie_dest]->sta.indicatif.call);
					texte (T_TRT + 7);
				}
				else
				{
					texte (T_TRT + 1);
				}
			}
			else
			{
				var_cpy (0, temp);
				texte (T_TRT + 2);
			}
		}
		else
			texte (T_ERR + 4);
	}
	else
		texte (T_ERR + 0);
}


static void mess_cross_connect (void)
{
	int voie_dest, voie_exp;

	voie_dest = pvoie->cross_connect;
	var_cpy (0, svoie[voie_dest]->sta.indicatif.call);
	texte (T_TRT + 3);

	if (voie_dest != voiecur)
	{
		voie_exp = voiecur;
		selvoie (voie_dest);
		var_cpy (0, svoie[voie_exp]->sta.indicatif.call);
		texte (T_TRT + 3);
		selvoie (voie_exp);
	}
}


static void menu_serveur (int typ_aide, int pos)
{
	if (typ_aide != AIDE)
		texte (pos);
	else
	{
		if (droits (MODLABEL))
			texte (pos + 1);
		texte (pos + 2);
	}
}


static void aff_new_message (void)
{
	if (pvoie->ncur->nbnew)
	{
		itoa (pvoie->ncur->nbnew, varx[0], 10);
		if (strcmp (pvoie->ncur->indic, "MODEM") != 0)
			texte (T_MES + 10);
	}
}


void prompt (int typ_aide, int niveau)
{
	if ((pvoie->niv1 != N_TELL) && (pvoie->cross_connect != -1))
	{
		mess_cross_connect ();
		pvoie->sniv1 = pvoie->niv1;
		pvoie->sniv2 = pvoie->niv2;
		pvoie->sniv3 = pvoie->niv3;
		maj_niv (0, 4, 1);
		return;
	}

	if (pvoie->ptemp)
	{
		m_libere ((char *) pvoie->ptemp, pvoie->psiz);
		pvoie->ptemp = NULL;
		pvoie->psiz = 0;
	}

	pvoie->sr_mem = 0;

	switch (niveau)
	{
	case N_MENU:
		menu_serveur (typ_aide, T_MEN);
		break;
	case N_QRA:
		menu_serveur (typ_aide, T_QRA);
		break;
	case N_STAT:
		menu_serveur (typ_aide, T_STA);
		break;
	case N_INFO:
		doc_path ();
		menu_serveur (typ_aide, T_INF);
		break;
	case N_NOMC:
		menu_serveur (typ_aide, T_NOM);
		break;
	case N_TRAJ:
		menu_serveur (typ_aide, T_TRJ);
		break;
	case N_DOS:
		prompt_dos ();
		break;
	case N_TELL:
		if (pvoie->niv3 == 1)
			texte (T_GAT + 1);
		else
			texte (T_GAT + 4);
		break;
	default:
		if (FOR (pvoie->mode))
			texte (T_MBL + 43);
		else if (BBS (typ_aide))
			texte (T_MBL + 0);
		else if (EXP (typ_aide) || P_BBS (voiecur))
		{
			aff_new_message ();
			texte (T_MBL + 0);
		}
		else if (LOC (typ_aide) || SYS (typ_aide))
		{
			aff_new_message ();
			texte (T_MBL + 2);
		}
		else if (P_GUEST (voiecur))
		{
			aff_new_message ();
			texte (T_MBL + 1);
		}
		else
		{
			aff_new_message ();
			texte (T_MBL + 2);
		}
		break;
	}
	pvoie->maj_ok = 1;
}


char *epure (infptr, len)
	 char *infptr;
	 int len;
{
	char *ptr = infptr;

	*infptr = '\0';
	while ((*indd) && (len))
	{
		if (*indd == ' ')
		{
			while (*indd == ' ')
				indd++;
			*infptr++ = ' ';
			--len;
		}
		else if (!iscntrl (*indd))
		{
			*infptr++ = *indd++;
			--len;
		}
		else
			++indd;
	}
	if (*(infptr - 1) == ' ')
		--infptr;
	*infptr = '\0';
	return (ptr);
}


/*
 *  ENTETE - ACCUEIL SUR LE SERVEUR
 */

static char s_langue (char *indic)
{
	char chaine[300];
	char ligne[81];
	char *ptr, *ind;
	int ok, niv, val, cpt, nbc;
	FILE *fptr;

	if ((fptr = fopen (c_disque ("LANGUE.SYS"), "rt")) == NULL)
		return ('\0');
	ok = cpt = nbc = val = niv = 0;
	while (fgets (ligne, 80, fptr))
	{
		if (*ligne == '#')
			continue;
		switch (niv)
		{
		case 0:
			if (isdigit (*ligne))
			{
				nbc = atoi (ligne);
			}
			++niv;
			break;
		case 1:
			if (++cpt == nbc)
				++niv;
			break;
		case 2:
			sscanf (ligne, "%s %d", chaine, &val);
			val--;
			ptr = chaine;
			ind = indic;
			while (*ptr == *ind)
			{
				++ptr;
				++ind;
			}
			if (*ptr == '*')
				ok = 1;
			else
				val = 0;
			break;
		}
		if (ok)
			break;
	}
	fclose (fptr);
	return ((char) val);
}


void init_info (info * frec, indicat * indicatif)
{
	memset ((char *) frec, '\0', sizeof (info));

	strcpy (frec->indic.call, indicatif->call);
	frec->indic.num = indicatif->num;
	strcpy (frec->qra, "?");
	frec->flags = def_mask;
	frec->nbl = 20;
	frec->lang = s_langue (indicatif->call);
	frec->hcon = time (NULL);
	frec->lastmes = nomess - 20L;
}


static int wp_val (int lg, char *field_info, char *wp_value)
{
	char *value;

	value = (*wp_value == '?') ? "\0" : wp_value;

	if (*value)
		n_cpy (lg, field_info, value);
	else if ((*field_info) && (*field_info != '?'))
	{
		return (1);
	}
	return (0);
}

int maj_with_wp (int voie)
{
	Wps *rec;
	int ok = 0;

	if (EMS_WPG_OK ())
	{
		if ((rec = wp_find (svoie[voie]->sta.indicatif.call, 1)) != NULL)
		{
			ok += wp_val (12, svoie[voie]->finf.prenom, rec->name);
			ok += wp_val (30, svoie[voie]->finf.ville, rec->first_qth);
			ok += wp_val (8, svoie[voie]->finf.zip, rec->first_zip);
			ok += wp_val (40, svoie[voie]->finf.home, rec->first_homebbs);
		}
		else
		{
			ok = 1;
		}
		if (ok)
			user_wp (&svoie[voie]->finf);
		return (1);
	}
	return (0);
}

int nouveau (int voie)
{
	int new;
	FILE *fptr;

	/* vptr->wp = 0; */
	if (svoie[voie]->ncur->coord == 0xffff)
	{
		new = 1;
		/* creer le message d'accueil */
		init_info (&svoie[voie]->finf, &svoie[voie]->sta.indicatif);

		if (find (svoie[voie]->sta.indicatif.call))
		{
			svoie[voie]->ncur->coord = rinfo++;
			insere_info (voie);
		}
	}
	else
	{
		new = 0;
		fptr = ouvre_nomenc ();
		fseek (fptr, svoie[voie]->ncur->coord * ((long) sizeof (info)), 0);
		fread ((char *) &(svoie[voie]->finf), (int) sizeof (info), 1, fptr);
		ferme (fptr, 18);
	}
	maj_with_wp (voie);
	return (new);
}


static void inconnu (void)
{
	itoa (pvoie->ncur->nbnew, varx[0], 10);
	texte (T_MES + 9);
}


static int dde_prenom (void)
{
	if (info_ok)
	{
		pvoie->sniv1 = pvoie->niv1;
		pvoie->sniv2 = pvoie->niv2;
		pvoie->sniv3 = pvoie->niv3;
		maj_niv (N_NOMC, 5, 0);
		saisie_infos ();
		return (0);
	}
	else
	{
		if (*pvoie->finf.prenom == '\0')
			texte (T_MES + 3);
		if (*pvoie->finf.ville == '\0')
			texte (T_MES + 4);
		if (*pvoie->finf.home == '\0')
			texte (T_MES + 5);
		if (*pvoie->finf.zip == '\0')
			texte (T_MES + 6);
		return (1);
	}
}


void finentete (void)
{
	char s[80];
	struct stat bufstat;
	
	memset(&bufstat, 0x00, sizeof(struct stat));
	
	sprintf (s, "LANG\\%s.ENT", nomlang + nlang * LG_LANG);
	outfich (c_disque (s));
	
	fprintf (stderr, "%s\n", c_disque(s));
	
	sprintf (s, "LANG\\%s.NEW", nomlang + nlang * LG_LANG);
	if ((stat (c_disque (s), &bufstat) == 0) && (bufstat.st_ctime != pvoie->finf.newbanner))
	{
		pvoie->finf.newbanner = bufstat.st_ctime;
		outfich (c_disque (s));
	}
	
	fprintf (stderr, "%s\n", c_disque(s));
	
	if ((pvoie->ncur->nbmess) && (strcmp (pvoie->ncur->indic, "MODEM") != 0))
	{
		if (pvoie->ncur->nbnew)
		{
			if (NEW (pvoie->finf.flags))
			{
				cr ();
				pvoie->typlist = 0;
				list_messages (1, pvoie->no_indic, 0);
				cr ();
			}
		}
		else
		{
			itoa (pvoie->ncur->nbmess, varx[0], 10);
			texte (T_MES + 11);
		}
	}
	if ((nb_hold) && (droits_2 (COSYSOP)))
	{
		var_cpy (0, itoa (nb_hold, s, 10));
		texte (T_MBL + 57);
	}
}


static void insere_info (int voie)
{
	FILE *fptr;
	unsigned r;

	if (svoie[voie]->ncur)
	{
		r = svoie[voie]->ncur->coord;
		if (r == 0xffff)
			dump_core ();
		fptr = ouvre_nomenc ();
		fseek (fptr, ((long) r) * ((long) sizeof (info)), 0);
		fwrite ((char *) &(svoie[voie]->finf), (int) sizeof (info), 1, fptr);
		inscoord (r, &(svoie[voie]->finf), svoie[voie]->ncur);
		ferme (fptr, 19);
	}
}


int accept_cnx (void)
{
	static int test_connect = 2;
	int retour = 1;
	int ret;
	char s[256];

	indd[80] = '\0';

	if (test_connect)
	{

		char buffer[1024];

		*buffer = '\0';

#ifdef __linux__
		sprintf (s, "./c_filter %s-%d %d %u %d %d %d",
				 pvoie->sta.indicatif.call,
				 pvoie->sta.indicatif.num,
				 pvoie->niv3, pvoie->finf.flags, new_om,
				 pvoie->ncur->coord, no_port (voiecur));
		ret = filter (s, buffer, sizeof (buffer), sup_ln (indd), FILTDIR);
#else
		sprintf (s, "c_filter %s-%d %d %u %d %d %d",
				 pvoie->sta.indicatif.call,
				 pvoie->sta.indicatif.num,
				 pvoie->niv3, pvoie->finf.flags, new_om,
				 pvoie->ncur->coord, no_port (voiecur));
		ret = filter (s, buffer, sizeof (buffer), sup_ln (indd), NULL);
#endif

		buffer[1023] = '\0';
		if (*buffer)
			out (buffer, strlen (buffer));

		switch (ret)
		{
		case -1:
			if (test_connect == 2)
			{
				/* premiere fois ... Pas de C_FILTER trouve */
				test_connect = 0;
			}
			else
			{
				/* Le C_FILTER a retourne -1 ... On deconnecte ! */
				pvoie->deconnect = 6;
				retour = 0;
			}
			break;
		case 0:
			/* OK... Plus d'appel */
			retour = 1;
			break;
		case 1:
			/* Appel avec No incremente */
			retour = 0;
			break;
		case 2:
			/* Deconnexion immediate */
			pvoie->deconnect = 6;
			retour = 0;
			break;
		case 3:
			/* Mode read-only */
			pvoie->read_only = 1;
			retour = 1;
			break;
		case 4:
			/* Mode read-only */
			pvoie->msg_held = 1;
			retour = 1;
			break;
		default:
			if (ret >= 100)
			{
				/* No redefini par le filtre */
				pvoie->niv3 = ret - 1;	/* niv3 est incremente apres !! */

				retour = 0;
			}
			else
				retour = 0;
			break;
		}
	}

	if (test_connect == 2)
		test_connect = 1;

	return (retour);
}


static int premices (void)
{
	char *st;

	init_langue (voiecur);

	if ((voiecur != CONSOLE) && (!find (pvoie->sta.indicatif.call)))
	{
		fbb_log (voiecur, 'X', "I");
		pvoie->log = 0;
		pvoie->deconnect = 3;
	}
	else if ((voiecur != CONSOLE) && (EXC (pvoie->finf.flags)))
	{
		fbb_log (voiecur, 'X', "E");
		pvoie->log = 0;
		pvoie->deconnect = 3;
	}
	else if ((voiecur != CONSOLE) && (P_BBS (voiecur)) &&
			 (!LOC (pvoie->finf.flags)) && (!BBS (pvoie->finf.flags)))
	{
		fbb_log (voiecur, 'X', "G");
		pvoie->log = 0;
		texte (T_MES + 8);
		pvoie->deconnect = 6;
	}
	else
	{
		st = idnt_fwd ();
		outs (st, strlen (st));
	}

	return (pvoie->deconnect == 0);
}

static void accueil (void)
{
	df ("accueil", 0);

	if (accept_cnx ())
	{
		if (POP (no_port (voiecur)))
		{
			/* No text for pop connection */
		}
		else if (BBS (pvoie->finf.flags))
		{
			texte (T_MES + 0);
		}
		else if (EXP (pvoie->finf.flags) || (SYS (pvoie->finf.flags)))
		{
			texte (T_MES + 1);
			if (new_om)
				inconnu ();
			if (!dde_prenom ())
			{
				ff ();
				return;
			}
			finentete ();
		}
		else if (P_GUEST (voiecur) && (!LOC (pvoie->finf.flags)))
		{
			texte (T_MES + 7);
			if (new_om)
				inconnu ();
			if (!dde_prenom ())
			{
				ff ();
				return;
			}
			finentete ();
		}
		else
		{
			texte (T_MES + 2);
			if (new_om)
				inconnu ();
			if (!dde_prenom ())
			{
				ff ();
				return;
			}
			finentete ();
		}
		if (pvoie->niv1 == N_CONF)
		{
			pvoie->conf = 1;
		}
		else
		{
			maj_niv (N_MBL, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
	}
	else
		pvoie->niv3++;

	ff ();
}


int msg_find (char *s)
{
	char *t = s;
	int n = 0;

	if (*t == '_')
	{
		++t;
		while (*t)
		{
			if (*t == '.')
				break;
			if (!isdigit (*t))
				return (0);
			++n;
			++t;
		}
		return (n);
	}
	else
		return (find (s));
}


/*
 *  DECONNEXION DE L'OM
 */

void sortie (void)
{
	pvoie->deconnect = TRUE;
	if (pvoie->l_mess)
	{
		pvoie->finf.lastmes = pvoie->l_mess;
		pvoie->l_mess = 0L;
	}
	if (voiecur == CONSOLE)
		close_print ();
}


void majrelai (int voie)
{
	int i;

	strcpy (svoie[voie]->finf.indic.call, svoie[voie]->sta.indicatif.call);
	svoie[voie]->finf.indic.num = svoie[voie]->sta.indicatif.num;
	for (i = 0; i < 8; i++)
	{
		if (*(svoie[voie]->sta.relais[i].call))
		{
			strcpy (svoie[voie]->finf.relai[i].call, svoie[voie]->sta.relais[i].call);
			svoie[voie]->finf.relai[i].num = svoie[voie]->sta.relais[i].num;
		}
		else
			*(svoie[voie]->finf.relai[i].call) = '\0';
	}
	++(svoie[voie]->finf.nbcon);
}


void majstat (int voie)
{
	FILE *fptr;
	statis bufstat;

	if ((voie) && (svoie[voie]->ncur) && (svoie[voie]->ncur->coord != 0xffff))
	{
		strncpy (bufstat.indcnx, svoie[voie]->sta.indicatif.call, 6);
		bufstat.port = (uchar) no_port (voie) - 1;
		bufstat.voie = (uchar) (voie - 1);
		bufstat.datcnx = svoie[voie]->debut;
		bufstat.tpscnx = (int) (time (NULL) - svoie[voie]->debut);
		if ((fptr = ouvre_stats ()) != NULL)
		{
			fseek (fptr, 0L, 2);
			fwrite ((char *) &bufstat, sizeof (bufstat), 1, fptr);
			ferme (fptr, 20);
		}
	}
}


void majinfo (int voie, int sens)
{
	/*
	   * Si sens = 1 -> lecture
	   *    sens = 2 -> ecriture
	   *    sens = 3 -> maj_heure
	 */

	FILE *fptr;

	if ((fptr = fopen (d_disque ("TPSTAT.SYS"), "wb")) == NULL)
	{
		fbb_error (ERR_OPEN, d_disque ("TPSTAT.SYS"), 0);
	}
	fwrite ((char *) stemps, sizeof (long) * NBRUB, 1, fptr);

	ferme (fptr, 21);

	if ((svoie[voie]->ncur) && (svoie[voie]->ncur->coord != 0xffff))
	{
		fptr = ouvre_nomenc ();
		if ((sens & 1) == 1)
		{
			fseek (fptr, svoie[voie]->ncur->coord * ((long) sizeof (svoie[voie]->finf)), 0);
			fread ((char *) &(svoie[voie]->finf), (int) sizeof (info), 1, fptr);
		}
		if ((sens & 2) == 2)
		{
			fseek (fptr, svoie[voie]->ncur->coord * ((long) sizeof (info)), 0);
			svoie[voie]->finf.hcon = svoie[voie]->debut;
			fwrite ((char *) &(svoie[voie]->finf), (int) sizeof (info), 1, fptr);
		}
		ferme (fptr, 22);

	}
}


void libere_zones_allouees (int voie)
{
	if (svoie[voie]->ptemp)
	{
		m_libere ((char *) svoie[voie]->ptemp, svoie[voie]->psiz);
		svoie[voie]->ptemp = NULL;
		svoie[voie]->psiz = 0;
	}
	if (svoie[voie]->Xfwd)
	{
		m_libere (svoie[voie]->Xfwd, sizeof (XInfo));
		svoie[voie]->Xfwd = NULL;
	}
	libere (voie);				/* message en cours de creation */
	libere_tread (voie);		/* Liste de lecture de messages */
	libere_edit (voie);			/* Libere la liste de l'editeur */
/*  libere_label(voie)        ; Libere les labels de YAPP    */
	libere_route (voie);		/* Libere les routes rx forward */
#ifndef __linux__
	libere_ymodem (voie, 0);;	/* Libere la liste de fichiers  */
#endif
	clear_inbuf (voie);			/* Vide le buffer d'entree      */
	clear_outbuf (voie);		/* Vide le buffer de sortie     */
}


void majfich (int voie)
{
	if (v_tell == voie)
	{
		t_tell = -1;
		v_tell = 0;
	}
	libere_zones_allouees (voie);
	if ((svoie[voie]->ncur) && (svoie[voie]->maj_ok))
	{
		if (voie != CONSOLE)
			majstat (voie);
		majrelai (voie);
		majinfo (voie, 2);
	}
}


void retour_menu (int niveau)
{
	if (pvoie->mbl)
		maj_niv (N_MBL, 0, 0);
	else if (niveau == N_MENU)
		maj_niv (niveau, 1, 0);
	else if (niveau != N_TELL)
		maj_niv (niveau, 0, 0);

	prompt (pvoie->finf.flags, pvoie->niv1);
}


void q_mark (void)
{
	texte (T_MES + 13);
}


void limite_commande (void)
{
	if (nb_trait > 80)
	{
		indd[80] = '\0';
		nb_trait = 80;
	}
}


/*
 *  MENU PRINCIPAL - PREMIER NIVEAU
 */

#ifndef MINISERV

void choix (void)
{
	int c, modex, error = 0;
	char com[80];

	limite_commande ();
	while (*indd && (!ISGRAPH (*indd)))
		indd++;
	strn_cpy (70, com, indd);

	switch (toupper (*indd))
	{
	case 'Q':
		maj_niv (N_QRA, 0, 0);
		incindd ();
		qraloc ();
		break;
	case 'C':
		maj_niv (N_STAT, 0, 0);
		incindd ();
		statistiques ();
		break;
	case 'D':
		maj_niv (N_INFO, 0, 0);
		*pvoie->ch_temp = '\0';
		incindd ();
		documentations ();
		break;
	case 'N':
		maj_niv (N_NOMC, 0, 0);
		incindd ();
		nomenclature ();
		break;
	case 'T':
		maj_niv (N_TRAJ, 0, 0);
		incindd ();
		trajec ();
		break;
	case 'F':
		if (indd == data)
		{
			if ((*(indd + 1) == '>') && (FOR (pvoie->mode)))
			{
				maj_niv (N_FORW, 1, 0);
				fwd ();
				break;
			}
		}
		pvoie->mbl = TRUE;
		/* pvoie->mode = pvoie->finf.flags ; */
		/* ??????????????????? */
		pvoie->mode = 0;
		texte (T_TRT + 13);
		maj_niv (N_MBL, 0, 0);
		prompt (pvoie->finf.flags, N_MBL);
		break;
	case 'B':
		maj_niv (N_MENU, 0, 0);
		sortie ();
		break;
	case '[':
		modex = TRUE;
		while ((c = *indd) != '\0')
		{
			++indd;
			if (c == '\n')
				modex = FALSE;
			if ((modex) && (*indd == ']'))
			{
				pvoie->mode = F_FOR;
			}
		}
		prompt (pvoie->finf.flags, pvoie->niv1);
		break;
	case ';':
		break;
	case '\0':
		prompt (pvoie->finf.flags, pvoie->niv1);
		break;
	default:
		if (!defaut ())
			error = 1;
		break;
	}
	if (error)
	{
		ch_niv1 (N_MENU);
		cmd_err (indd);
	}
}


#endif

void menu_principal (void)
{
	df ("menu_principal", 0);

	switch (pvoie->niv2)
	{
	case 0:
		if (premices ())
		{
			ch_niv2 (2);
			accueil ();
		}
		break;
#ifndef MINISERV
	case 1:
		choix ();
		break;
#endif
	case 2:
		accueil ();
		break;
	case 3:
		arret_serveur (d_disque ("ETAT.SYS"), 1);
		break;
	case 4:
		cross_connexion ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "PRIM-MENU", pvoie->niv2);
		break;
	}

	ff ();
}
