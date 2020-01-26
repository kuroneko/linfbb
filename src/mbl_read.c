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
 * Module MBL_READ.C
 */

#define NO_STATUS

static int mbl_rx (int);
static int read_mine (int);
static int teste_liste (bullist *);

/* Commande 'R' -> Read message */
/* Commande 'V' -> Read verbose message */

#include "aff_stat.c"

static int strfind (bullist * pbul, char *cherche)
{
	return (strmatch (ltitre (1, pbul), cherche));
}


static int teste_liste (bullist * lbul)
{
	if (lbul->numero < pvoie->recliste.debut)
		return (0);
	if (lbul->numero > pvoie->recliste.fin)
		return (0);
	if (lbul->date < pvoie->recliste.avant)
		return (0);
	if (lbul->date > pvoie->recliste.apres)
		return (0);

	if (!droits (COSYSOP))
	{
		if (strcmp (lbul->desti, "KILL") == 0)
			return (0);
		if (lbul->type == 'A')
			return (0);
		if ((lbul->status == 'K') && (pvoie->recliste.status != 'K'))
			return (0);
		if ((lbul->status == 'A') && (pvoie->recliste.status != 'A'))
			return (0);
	}

	if (*pvoie->recliste.find)
		return (strfind (lbul, pvoie->recliste.find));

	if ((pvoie->recliste.type) && (pvoie->recliste.type != lbul->type))
		return (0);
	if ((pvoie->recliste.status) && (pvoie->recliste.status != lbul->status))
		return (0);
	if ((*pvoie->recliste.exp) && (!strmatch (lbul->exped, pvoie->recliste.exp)))
		return (0);
	if ((*pvoie->recliste.dest) && (!strmatch (lbul->desti, pvoie->recliste.dest)))
		return (0);
	if (*pvoie->recliste.bbs)
	{
		if (*pvoie->recliste.bbs == '-')
		{
			if (*lbul->bbsv)
				return (0);
		}
		else
		{
			if (!strmatch (lbul->bbsv, pvoie->recliste.bbs))
				return (0);
		}
	}

	return (1);
}


static int read_mine (int mode)
{
	/* Lecture des messages personnels */

	bloc_mess *temp = tete_dir;
	int i, nouveau = 0, trouve = 0;
	unsigned indic;
	bullist pbul;
	mess_noeud *lptr;
	char s[80];
	rd_list *ptemp = NULL;

	if (isdigit (mode))
	{
		s[0] = (char) mode;
		s[1] = '\0';
		insnoeud (s, &indic);
	}
	else
	{
		if (mode == 'N')
			nouveau = 1;
		indic = pvoie->no_indic;
	}

	ouvre_dir ();

	while (temp->suiv)
		temp = temp->suiv;

	while (temp)
	{
		i = T_BLOC_MESS;
		while (i--)
		{
			lptr = &temp->st_mess[i];
			if ((lptr->noenr) && (lptr->no_indic == indic))
			{
				read_dir (lptr->noenr, &pbul);
				if ((pbul.type) && (pbul.status != 'H') && ((!nouveau) || (nouveau && (pbul.status == 'N'))))
				{
					trouve = 1;
					if (ptemp)
					{
						ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
						ptemp = ptemp->suite;
					}
					else
						pvoie->t_read = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
					ptemp->suite = NULL;
					ptemp->nmess = lptr->nmess;
					ptemp->verb = pvoie->recliste.l;
				}
			}
		}
		temp = prec_dir (temp);
	}

	ferme_dir ();

	if (!trouve)
	{
		if (nouveau)
			texte (T_MBL + 4);
		else
			texte (T_MBL + 3);
	}
	return (trouve);
}


int mbl_bloc_list (void)
{
	int retour = 0;
	bullist ligne;
	rd_list *ptemp;
	unsigned offset = pvoie->recliste.offset;
	bloc_mess *bptr = pvoie->recliste.ptemp;
	mess_noeud *mptr;

	ptemp = pvoie->t_read;
	while (ptemp)
		ptemp = ptemp->suite;

	pvoie->seq = FALSE;
	ouvre_dir ();
	while (bptr)
	{
		--offset;
		mptr = &(bptr->st_mess[offset]);
		read_dir (mptr->noenr, &ligne);
		if (ligne.numero < pvoie->recliste.debut)
			break;
		if ((mptr->noenr) && (droit_ok (&ligne, 1)) && (teste_liste (&ligne)))
		{
			if (pvoie->recliste.last-- == 0L)
				break;
			if (pvoie->temp1)
				pvoie->temp1 = 0;
			/* Entre le numero trouve en liste */
			if (ptemp)
			{
				ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
				ptemp = ptemp->suite;
			}
			else
			{
				pvoie->t_read = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
			}
			ptemp->suite = NULL;
			ptemp->nmess = ligne.numero;
			ptemp->verb = pvoie->recliste.l;
		}
		if (offset == 0)
		{
			bptr = prec_dir (bptr);
			offset = T_BLOC_MESS;
		}
		if (!(POP (no_port (voiecur))) && (trait_time > MAXTACHE))
		{
			pvoie->seq = TRUE;
			retour = 1;
			break;
		}
	}
	ferme_dir ();

	pvoie->recliste.offset = offset;
	pvoie->recliste.ptemp = bptr;

	if ((!retour) && (pvoie->t_read))
		retour = 2;
	return (retour);
}


int mbl_rx (int verbose)
{
	int error = 0;
	long no;
	int c, ok = TRUE;
	bullist *pbul;
	rd_list *ptemp = NULL;

	df ("mbl_rx", 1);
/*
   print_fonction(stdout); print_history(stdout); sleep_(10);
   for (;;);
 */
	sup_ln (indd);
	c = toupper (*indd);
	++indd;

	if ((c != ' ') && (*indd != ' ') && (*indd != '\0'))
	{
		ff ();
		return (1);
	}

	pvoie->aut_nc = 1;

	libere_tread (voiecur);
	init_recliste (voiecur);
	pvoie->recliste.l = verbose;

	switch (c)
	{

	case 'A':
		pvoie->recliste.status = 'A';
		break;
	case 'B':
		pvoie->recliste.type = 'B';
		break;
	case 'E':
		if (droits (COSYSOP))
		{
			int v;

			for (v = 0; v < NBVOIES; v++)
			{
				if ((svoie[v]->niv1 == N_MBL) && (svoie[v]->niv2 == 18))
				{
					texte (T_TRT + 0);
					ok = 0;		/* Interdit le multi-acces */

					break;
				}
			}
			if (ok)
			{
				maj_niv (N_MBL, 18, 0);
				ff ();
				return (review ());
			}
		}
		else
		{
			--indd;
			error = 1;
			ok = 4;
		}
		break;
	case 'F':
		pvoie->recliste.status = 'F';
		break;
	case 'K':
		pvoie->recliste.status = 'K';
		break;
	case 'L':
		if (teste_espace ())
		{
			if (isdigit (*indd))
				pvoie->recliste.last = lit_chiffre (0);
			else
			{
				texte (T_ERR + 3);
				ok = 0;
			}
		}
		else
		{
			/* texte(T_ERR + 2) ; */
			--indd;
			error = 1;
			ok = 4;
		}
		break;
	case 'M':
	case 'N':
		if (read_mine (c))
			ok = 2;
		else
			ok = 0;
		break;
	case 'P':
		pvoie->recliste.type = 'P';
		break;
	case 'S':
		if (teste_espace ())
			strn_cpy (19, pvoie->recliste.find, indd);
		else
		{
			/* texte(T_ERR + 2) ; */
			--indd;
			error = 1;
			ok = 4;
		}
		break;
	case 'T':
		pvoie->recliste.type = 'T';
		break;
	case 'U':
		pvoie->recliste.type = 'P';
		pvoie->recliste.status = 'N';
		break;
	case 'X':
		pvoie->recliste.status = 'X';
		break;
	case 'Y':
		pvoie->recliste.status = 'Y';
		break;
	case '$':
		pvoie->recliste.status = '$';
		break;
	case '<':
		if (teste_espace ())
		{
			strn_cpy (6, pvoie->recliste.exp, indd);
		}
		else
		{
			texte (T_ERR + 2);
			ok = 0;
		}
		break;
	case '>':
		if (teste_espace ())
		{
			strn_cpy (6, pvoie->recliste.dest, indd);
		}
		else
		{
			texte (T_ERR + 2);
			ok = 0;
		}
		break;
	case '@':
		if (teste_espace ())
		{
			strn_cpy (6, pvoie->recliste.bbs, indd);
		}
		else
		{
			/* texte(T_ERR + 2) ; */
			--indd;
			error = 1;
			ok = 4;
		}
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (read_mine (c))
			ok = 2;
		else
			ok = 0;
		break;
	case ' ':
		if (strchr (indd, '-'))
		{
			if (isdigit (*indd))
				pvoie->recliste.debut = lit_chiffre (1);
			else
			{
				texte (T_ERR + 3);
				ok = 0;
				break;
			}
			++indd;				/* saute le tiret */
			if (isdigit (*indd))
				pvoie->recliste.fin = lit_chiffre (1);
			if (pvoie->recliste.fin <= pvoie->recliste.debut)
				ok = 0;
		}
		else
		{
			ok = 0;
			while ((no = lit_chiffre (1)) != 0L)
			{
				if ((pbul = ch_record (NULL, no, ' ')) != NULL)
				{
					if (droit_ok (pbul, 1))
					{
						if (ptemp)
						{
							ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
							ptemp = ptemp->suite;
						}
						else
						{
							pvoie->t_read = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
						}
						ptemp->suite = NULL;
						ptemp->nmess = no;
						ptemp->verb = verbose;
						ok = 2;
					}
					else
						texte (T_ERR + 10);
				}
				else
					texte (T_ERR + 10);
					
				/* Only one message by request in POP mode */
				if (POP (no_port (voiecur)))
					break;
			}
		}
		break;
	default:
		if ((c == '\0') && (verbose))
		{
			texte (T_MBL + 8);
			ok = 0;
		}
		else
		{
			error = 1;
			--indd;
			ok = 4;
		}
		break;
	}
	switch (ok)
	{
	case 0:
		retour_mbl ();
		break;
	case 1:
		pvoie->recliste.ptemp = last_dir ();
		pvoie->recliste.offset = T_BLOC_MESS;
		pvoie->temp1 = 1;
		pvoie->sr_mem = 1;
		ch_niv3 (1);
		mbl_read (verbose);
		break;
	case 2:
		pvoie->sr_mem = 1;
		ch_niv3 (2);
		mbl_read (verbose);
		break;
	}
	ff ();
	return (error);
}

int mbl_read (int verbose)
{
	int error = 0;

	df ("mbl_read", 1);

	switch (pvoie->niv3)
	{
	case 0:
		error = mbl_rx (verbose);
		break;
	case 1:
		switch (mbl_bloc_list ())
		{
		case 0:				/* Pas de message */
			texte (T_MBL + 3);
			retour_mbl ();
		case 1:				/* Pas fini */
			break;
		case 2:				/* Termine */
			ch_niv3 (2);
			mbl_read (verbose);
			break;
		}
		break;
	case 2:
		if (mbl_mess_read () == 0)
		{
			retour_mbl ();
		}
		break;
	case 3:
		if (read_mess (1) == 0)
			ch_niv3 (2);
		break;
	default:
		fbb_error (ERR_NIVEAU, "MSG-READ", pvoie->niv3);
	}
	ff ();
	return (error);
}


static void entete_mess (bullist * ligne)
{
	*ptmes = *ligne;
	if (*(ligne->bbsv))
		sprintf (varx[0], "@%-6s  ", ligne->bbsv);
	else
		*varx[0] = '\0';
	texte (T_MBL + 35);
	texte (T_MBL + 38);
}


static void end_read_mess (void)
{
	char s[80];
	rd_list *ptemp = pvoie->t_read;

	if (ptemp)
	{
		sprintf (s, "R %ld", ptemp->nmess);
		fbb_log (voiecur, 'M', s);
		cr_cond ();
		if (POP (no_port (voiecur)))
		{
			out("\033", 1);
		}
		else	
		{
			texte (T_MBL + 33);
		}
		marque_obuf ();
		pvoie->t_read = ptemp->suite;
		m_libere (ptemp, sizeof (rd_list));
	}
}

char *pop_date(long date)
{
	char *day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char *mon[]= {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	struct tm *sdate;
	static char cdate[40];
	
	sdate = localtime (&date);
	sprintf (cdate, "%s, %02d %s %d %02d:%02d:%02d %+03ld00",
			day[sdate->tm_wday],
			sdate->tm_mday,
			mon[sdate->tm_mon], 
			sdate->tm_year %100 + 2000,
			sdate->tm_hour,
			sdate->tm_min,
			sdate->tm_sec,
			-timezone/3600);
	return cdate;
}

int read_mess (int verbose)
{
	int nb;
	FILE *fptr;
	char chaine[256];
	rd_list *ptemp = pvoie->t_read;

	nb = 0;
	
	if ((fptr = ouvre_mess (O_TEXT, ptemp->nmess, '\0')) != NULL)
	{
		fseek (fptr, pvoie->enrcur, 0);
		if (!verbose)
			fseek (fptr, supp_header (fptr, 1), 0);
		fflush (fptr);
		while ((nb = read (fileno (fptr), chaine, 250)) > 0)
		{
			outs (chaine, nb);
			if (!(POP (no_port (voiecur))) && (pvoie->memoc >= MAXMEM))
			{
				/*        if (!getvoie(CONSOLE]->connect) cprintf("Max atteint\r\n") ; */
				pvoie->enrcur = ftell (fptr);
				break;
			}
		}
		ferme (fptr, 45);
	}
	if (nb == 0)
		end_read_mess ();

	return (nb);
}


static void trans_header (char *ptr, char *bbsfrom, long date)
{
	static char ligne[80];

	int c;
	int champ;
	int nb;
	int len;
	int bbs;
	char rbbs[80];
	char home[41];

	ptr += 2;
	champ = 2;
	nb = 0;
	bbs = 0;

	*home = '\0';
	date = 0L;

	do
	{

		c = *ptr;

		switch (champ)
		{

		case 0:
			switch (c)
			{
			case '@':
				bbs = 1;
				champ = 3;
				nb = 0;
				break;
			case '#':
				champ = 6;
				nb = 0;
				break;
			case '$':
				champ = 7;
				nb = 0;
				break;
			case '[':
				champ = 4;
				nb = 0;
				break;
			case 'Z':
				if (*(ptr + 1) == ':')
				{
					++ptr;
					champ = 5;
					nb = 0;
				}
				break;
			default:
				if ((bbs == 0) && (isdigit (c)))
				{
					nb = 0;
					rbbs[nb++] = c;
					champ = 6;
				}
			}

		case 1:
			if (isspace (c))
				champ = 0;
			break;

		case 2:				/* Lecture de la date - Mettre la date la plus ancienne */
			if (nb <= 10)
				rbbs[nb] = c;
			if (nb == 10)
			{
				long rdate ;
				
				rbbs[11] = '\0';
				rbbs[6] = '\0';
				if ((rdate = date_to_time (rbbs)) != 0L)
					date = rdate + hour_to_time (rbbs + 7);
				champ = 1;
			}
			++nb;
			break;

		case 3:				/* Lecture du home BBS */
			if ((nb == 0) && (c == ':'))
				break;
			if ((ISGRAPH (c)) && (nb < 40))
			{
				rbbs[nb++] = c;
			}
			else
			{
				rbbs[nb] = '\0';
				strn_cpy (40, home, rbbs);
				champ = 0;
			}
			break;

		case 4:				/* Lecture du Qth */
			if ((c != ']') && (nb < 30))
			{
				rbbs[nb++] = c;
			}
			else
			{
				rbbs[nb] = '\0';
				champ = 0;
			}
			break;

		case 5:				/* Lecture du Zip Code */
			if ((ISGRAPH (c)) && (nb < 8))
			{
				rbbs[nb++] = c;
			}
			else
			{
				rbbs[nb] = '\0';
				champ = 0;
			}
			break;
		case 6:				/* Lecture du home premier numero */
			if ((nb == 0) && (c == ':'))
				break;
			if ((isdigit (c)) && (nb < 10))
			{
				rbbs[nb++] = c;
			}
			else
			{
				rbbs[nb] = '\0';
				if ((bbs == 0) && (c == '@'))
				{
					bbs = 1;
					champ = 3;
					nb = 0;
				}
				else
					champ = 0;
			}
			break;
		case 7:				/* Lecture du BID/MID */
			if ((nb == 0) && (c == ':'))
				break;
			if ((ISGRAPH (c)) && (nb < 12))
			{
				rbbs[nb++] = c;
			}
			else
			{
				rbbs[nb] = '\0';
				champ = 0;
			}
			break;

		}

		++ptr;
	}
	while (ISPRINT (c));

	len = sprintf(ligne, "Received: from %s ; %s", home, pop_date(date));
	outsln(ligne, len);

	strcpy(bbsfrom, home);
}

static char *read_headers(long date)
{
	int nb;
	FILE *fptr;
	char chaine[256];
	rd_list *ptemp = pvoie->t_read;
	static  char bbs[41];

	nb = 0;
	*bbs = '\0';
	
	if ((fptr = ouvre_mess (O_TEXT, ptemp->nmess, '\0')) != NULL)
	{
		while (fgets(chaine, sizeof(chaine), fptr))
		{
			if (strncmp("R:", chaine, 2) != 0)
				break;
			trans_header(chaine, bbs, date);
		}
		ferme (fptr, 45);
	}
	
	return bbs;
}

char *xuidl(long numero, char *callsign)
{
	int i;
	char call[10];
	static char str[50];
	
	sprintf(call, "%6s", callsign);
	for (i = 0 ; i < 6 ; i++)
		sprintf(str+i*2, "%02x", call[i] & 0xff);
	sprintf(str+i*2, "%032lx", numero);
	
	return str;
}

int mbl_mess_read (void)
{
	bullist *pbul;
	rd_list *ptemp;

	while ((ptemp = pvoie->t_read) != NULL)
	{
		/*    cprintf("Lit le %ld\r\n", ptemp->nmess) ;     */
		if ((pbul = ch_record (NULL, ptemp->nmess, 'Y')) != NULL)
		{
			if (POP (no_port (voiecur)))
			{
				int nb;
				char chaine[80];
				char name[80];
				char exped[80];
				char *reply;
				Wps *wps;

				/* Get the name from WP */				
				wps = wp_find(pbul->exped, 0);
				if ((wps) && (*wps->name) && (*wps->name != '?'))
				{
					sprintf(name, "%s (%s)", pbul->exped, wps->name);
				}
				else
				{
					strcpy(name, pbul->exped);
				}
				
				/* Dump headers and get the older BBS for return address */
				reply = read_headers(pbul->date);
				if (*reply == '\0')
					reply = mypath;
				
	/**** SHOULD BE CONFIGURABLE (BEGIN) ****/
	
				if (*pop_host)	
				{
					/*** address like f6fbb%f6fbb.fmlr.fra.eu@f6fbb.ampr.org */
					nb = snprintf(exped, sizeof(exped)-1, "%s%%%s@%s", pbul->exped, reply, pop_host);
					exped[sizeof(exped)-1] = 0;
				}
				else
				{
					nb = snprintf(exped, sizeof(exped)-1, "%s@%s", pbul->exped, reply);
					exped[sizeof(exped)-1] = 0;
					}
	
	/**** SHOULD BE CONFIGURABLE (END) ****/

				nb = snprintf(chaine, sizeof(chaine), "Date: %s", pop_date(pbul->datesd));
				outsln (chaine, nb);
				nb = snprintf(chaine, sizeof(chaine), "From: %s <%s>", name, exped);
				outsln (chaine, nb);
			if (pbul->type == 'B')
				{
					/* News -- When user enters Reply, this is where reply goes.  Thunderbird, at least, doesn't mind that these names don't match the group name, and it makes replies go to the correct distribution */
					if (!pbul->bbsv[0])
					{
						nb = snprintf(chaine, sizeof(chaine)-1, "Newsgroups: %s", pbul->desti );
					}
					else
					{
						nb = snprintf(chaine, sizeof(chaine)-1, "Newsgroups: %s@%s", pbul->desti, pbul->bbsv );
					}
					outsln(chaine, nb);

					nb = snprintf(chaine,sizeof(chaine), "Subject: %s", pbul->titre); // Removed [desti] to prevent Subjects from being changed during reply
						outsln (chaine, nb);
					nb = snprintf(chaine, sizeof(chaine), "Message-ID: <%s>", pbul->bid); /* Use BID for Message-ID.   This allows messages to be downloaded even if they change groups.  Reading will now ref bid */
					outsln (chaine, nb);
				}
				else
				{
					/* To: Moved here.  To: is not correct for */
					nb = snprintf(chaine,  sizeof(chaine), "To: %s", pbul->desti);
					outsln (chaine, nb);
						/* Mail */
					nb = snprintf(chaine,sizeof(chaine), "Subject: %s", pbul->titre);
					outsln (chaine, nb);
					nb = snprintf(chaine, sizeof(chaine), "Message-ID: <%ld@%s>", pbul->numero, mycall);
					outsln (chaine, nb);
					nb = snprintf(chaine, sizeof(chaine), "X-UIDL: %s", xuidl(pbul->numero, mycall));
					outsln (chaine, nb);
				}
				cr();
			}
			else
			{
				entete_mess (pbul);
			}
			pvoie->enrcur = 0L;
			if (read_mess (ptemp->verb))
			{
				ch_niv3 (3);
				return (1);
			}
		}
		else
		{
			pvoie->t_read = ptemp->suite;
			m_libere (ptemp, sizeof (rd_list));
		}
	}
	return (0);
}


void libere_tread (int voie)
{
	rd_list *ptemp = svoie[voie]->t_read;
	rd_list *pprec;

	while ((pprec = ptemp) != NULL)
	{
		ptemp = pprec->suite;
		m_libere (pprec, sizeof (rd_list));
	}
	svoie[voie]->t_read = NULL;
}


void libere_tlist (int voie)
{
	rd_list *ptemp = svoie[voie]->t_list;
	rd_list *pprec;

	while ((pprec = ptemp) != NULL)
	{
		ptemp = pprec->suite;
		m_libere (pprec, sizeof (rd_list));
	}
	svoie[voie]->t_list = NULL;
}
 
