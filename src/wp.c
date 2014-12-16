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


extern void *tete_wp;

static char *l2call (lcall);

int init_white_pages (void)
{
#if defined(__WINDOWS__) || defined(__LINUX__)
	char buf[80];

#endif
#ifdef __FBBDOS__
	fen *fen_ptr;

#endif
	FILE *fptr;
	Wps rec;
	Wp wp;
	unsigned record = 0;

	if (!EMS_WPG_OK ())
		return (0);

	init_wp_cache ();

	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (1);

#ifdef ENGLISH
	cprintf ("White Pages Set-up           \r\n");
#else
	cprintf ("Initialisation Pages Blanches\r\n");
#endif

	deb_io ();

#ifdef __FBBDOS__
#ifdef ENGLISH
	fen_ptr = open_win (10, 5, 50, 8, INIT, "  White Pages ");
#else
	fen_ptr = open_win (10, 5, 50, 8, INIT, "Pages Blanches");
#endif
#endif /* __FBBDOS__ */

	while (fread (&rec, sizeof (Wps), 1, fptr))
	{
		if ((record % 100) == 0)
		{
#if defined(__WINDOWS__) || defined(__LINUX__)
			InitText (ltoa ((long) record, buf, 10));
#endif
#ifdef __FBBDOS__
			cprintf ("\rrecord %u", record);
#endif
		}
		wp.callsign = call2l (rec.callsign);
		wp.home = call2l (bbs_via (rec.first_homebbs));

		write_wp (record, &wp);
		++record;
	}
	fclose (fptr);
#if defined(__WINDOWS__) || defined(__LINUX__)
	InitText (ltoa ((long) record, buf, 10));
#endif
#ifdef __FBBDOS__
	cprintf ("\rrecord %u", record);
	attend_caractere (2);
	close_win (fen_ptr);
#endif
	fin_io ();
#ifdef ENGLISH
	cprintf ("%u records updated\r\n", record);
#else
	cprintf ("%u records updated\r\n", record);
#endif
	return (1);
}

static void mess_wp (char *chaine)
{
	Wpr rec;
	char *ptr;
	char source = '\0';
	char date[80];
	char call[80];
	char route[80];
	char zip[80];
	char name[80];
	char qth[80];

	static char *qmark = "?";

	ini_rec (&rec);

	*date = '\0';
	*call = '\0';
	strcpy (route, qmark);
	strcpy (zip, qmark);
	strcpy (name, qmark);
	strcpy (qth, qmark);
	sscanf (chaine, "%*s %s %s %*s %s %*s %s %s %[^\n]",
			date, call, route, zip, name, qth);

	if ((*date == '\0') || (*call == '\0'))
	{
		char debwp[300];

		sprintf (debwp, "invalid update :\n  %s\n", chaine);
		debug_wp (debwp);
		return;
	}

	ptr = strrchr (call, '/');
	if (ptr)
	{
		*ptr++ = '\0';
		if (isalpha (*ptr))
			source = toupper (*ptr);
	}

	rec.local = 1;				/* Pas de ligne si update par message */
	rec.source = source;
	rec.last = date_to_time (date);
	strn_cpy (6, rec.callsign, call);
	strn_cpy (40, rec.homebbs, route);
	if (*zip != '?')
		strn_cpy (8, rec.zip, zip);
	if (*name != '?')
		n_cpy (12, rec.name, name);
	if (*qth != '?')
		n_cpy (30, rec.qth, qth);

	if ((*route != '?') && (addr_check (route)))
		wp_upd (&rec, 0);
	else
	{
		char debwp[300];

		debug_wp (chaine);
		sprintf (debwp, "Address <%s> is not valid, WP not updated\n\n", route);
		debug_wp (debwp);
	}
}

void wp_read_mess (bullist * pbul)
{
	char chaine[256];
	FILE *fptr;
	ind_noeud *noeud;
	mess_noeud *mptr;
	char bbs[10];

	if (!EMS_WPG_OK ())
		return;

	deb_io ();

	aff_etat ('X');

	strcpy (bbs, bbs_via (pbul->bbsv));

	if ((fptr = ouvre_mess (O_TEXT, pbul->numero, '\0')) != NULL)
	{
		while (fgets (chaine, 255, fptr))
		{
			if (strncmp (chaine, "On", 2) == 0)
				mess_wp (chaine);
			trait_time = 0;
		}
		fclose (fptr);
	}
	fin_io ();

	if ((*pbul->bbsv == '\0') && ((mptr = findmess (pbul->numero)) != NULL))
	{							/* Tue le message */
		ouvre_dir ();
		pbul->status = 'K';
		write_dir (mptr->noenr, pbul);
		ferme_dir ();
		noeud = cher_noeud (pbul->desti);
		--(noeud->nbnew);
		--(noeud->nbmess);
		chg_mess (0xffff, pbul->numero);
	}

	aff_etat ('A');
}

static int wp_stats (void)
{
	char s[256];
	long record;
	long bbs;
	long users;
	int retour = 1;
	double ratio;
	Wp wp;

	bbs = users = 0L;

	for (record = 0L;; ++record)
	{
		if (!read_wp ((unsigned) record, &wp))
			break;

		if (wp.callsign == wp.home)
			++bbs;
		else
			++users;
	}
	sprintf (s, "%6ld records in WP", record);
	outln (s, strlen (s));

	ratio = (double) users *100.0 / (double) record;

	sprintf (s, "%6ld Users (%2d%%)", users, (int) (ratio + .5));
	outln (s, strlen (s));

	ratio = (double) bbs *100.0 / (double) record;

	sprintf (s, "%6ld BBS   (%2d%%)", bbs, (int) (ratio + .5));
	outln (s, strlen (s));

	return (retour);
}

static int str_alnum (char *mask)
{
	while (*mask)
	{
		if (!isalnum (*mask))
			return (0);
		++mask;
	}
	return (1);
}

static int display_wp_line (Wps * rec)
{
	char s[256];

	if (*rec->first_homebbs)
	{
		sprintf (s, "%-6s @%s %s %s %s",
				 rec->callsign,
				 rec->first_homebbs,
				 (*rec->name) ? rec->name : "?",
				 (*rec->first_zip) ? rec->first_zip : "?",
				 (*rec->first_qth) ? rec->first_qth : "?"
			);
		outln (s, strlen (s));
		return (1);
	}
	return (0);
}

static int wp_search_call (void)
{
	char mask[42];
	FILE *fptr;
	unsigned record;
	int retour = 1;
	int no_mask;
	lcall callsign = 0;
	Wps rec;
	Wp wp;

	if (!*pvoie->ch_temp)
		return (retour);

	pvoie->sr_mem = pvoie->seq = FALSE;

	record = (unsigned) pvoie->noenr_menu;
	strn_cpy (40, mask, pvoie->ch_temp);

	if ((no_mask = str_alnum (mask)) != 0)
		callsign = call2l (mask);

	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (retour);

	if (no_mask)
	{
		record = search_wp_record (callsign, USR_CALL, 0);
		if (record < 0xffff)
		{
			deb_io ();
			fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
			fread (&rec, sizeof (Wps), 1, fptr);
			fin_io ();

			if (display_wp_line (&rec))
				pvoie->temp1 = 1;
		}
	}

	else
	{
		while (read_wp (record, &wp))
		{
			if ((wp.callsign) && (strmatch (l2call (wp.callsign), mask)))
			{
				deb_io ();
				fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
				fread (&rec, sizeof (Wps), 1, fptr);
				fin_io ();

				if (display_wp_line (&rec))
					pvoie->temp1 = 1;
			}
			if (pvoie->memoc >= MAXMEM)
			{
				pvoie->sr_mem = TRUE;
				retour = 0;
				break;
			}
			if (trait_time > MAXTACHE)
			{
				pvoie->seq = TRUE;
				retour = 0;
				break;
			}
			++record;
		}
	}

	fclose (fptr);

	pvoie->noenr_menu = record;

	if (retour && (pvoie->temp1 == 0))
		texte (T_ERR + 19);

	return (retour);
}

Wps *wp_find (char *callsign, int update)
{
	FILE *fptr;
	unsigned record = 0;
	static Wps rec;

	if (!EMS_WPG_OK ())
		return (NULL);

	record = search_wp_record (call2l (callsign), USR_CALL, 0);
	if (record == 0xffff)
		return (NULL);

	deb_io ();
	fptr = fopen (d_disque ("WP\\WP.SYS"), "r+b");
	if (fptr == NULL)
	{
		fin_io ();
		return (NULL);
	}

	fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
	fread (&rec, sizeof (Wps), 1, fptr);

	if (update)
	{
		rec.last_seen = time (NULL);

		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		fwrite (&rec, sizeof (Wps), 1, fptr);
	}
	
	fclose (fptr);
	fin_io ();
	return (&rec);
}

static void display_wp_user (long record, Wps * rec)
{
	char s[256];
	char modif[80];
	char seen[80];

	strcpy (modif, date_mbl (rec->last_modif));
	strcpy (seen, date_mbl (rec->last_seen));

	sprintf (s, "%-4ld: %s %s %-6s (%u) %s",
			 record,
			 modif, seen,
			 rec->callsign,
			 rec->seen,
			 (*rec->name) ? rec->name : "?"
		);
	outln (s, strlen (s));

	sprintf (s, "  [1] @%s Zip:%s Qth:%s",
			 rec->first_homebbs,
			 (*rec->first_zip) ? rec->first_zip : "?",
			 (*rec->first_qth) ? rec->first_qth : "?"
		);
	outln (s, strlen (s));

	sprintf (s, "  [2] @%s Zip:%s Qth:%s",
			 rec->secnd_homebbs,
			 (*rec->secnd_zip) ? rec->secnd_zip : "?",
			 (*rec->secnd_qth) ? rec->secnd_qth : "?"
		);
	outln (s, strlen (s));

}

static int wp_list_user (void)
{
	char mask[42];
	FILE *fptr;
	unsigned record;
	int retour = 1;
	lcall lc;
	Wps rec;
	Wp wp;

	if (!*pvoie->ch_temp)
	{
		texte (T_ERR + 2);
		return (retour);
	}

	pvoie->sr_mem = pvoie->seq = FALSE;

	record = (unsigned) pvoie->noenr_menu;
	strn_cpy (40, mask, pvoie->ch_temp);

	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (retour);

	lc = call2l (mask);

	while (read_wp (record, &wp))
	{

		if (wp.callsign == lc)
		{
			deb_io ();
			fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
			fread (&rec, sizeof (Wps), 1, fptr);
			fin_io ();

			if (*rec.first_homebbs)
			{
				display_wp_user ((long) record, &rec);

				pvoie->temp1 = 1;
				break;
			}
		}
		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 0;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 0;
			break;
		}
		++record;
	}

	fclose (fptr);

	pvoie->noenr_menu = record;

	if (retour && (pvoie->temp1 == 0))
	{
		texte (T_ERR + 19);
		pvoie->noenr_menu = -1L;
	}


	return (retour);
}

static int wp_search_bbs (void)
{
	/* char     s[256]; */
	char mask[42];
	FILE *fptr;
	Wps rec;
	Wp wp;
	unsigned record;
	int retour = 1;
	int no_mask;
	lcall callsign;

	if (!*pvoie->ch_temp)
	{
		texte (T_ERR + 2);
		return (retour);
	}

	pvoie->sr_mem = pvoie->seq = FALSE;

	record = (unsigned) pvoie->noenr_menu;
	strn_cpy (40, mask, pvoie->ch_temp);

	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (1);

	if ((no_mask = str_alnum (mask)) != 0)
		callsign = call2l (mask);

	if (no_mask)
	{
		while (read_wp (record, &wp))
		{
			if (wp.home == callsign)
			{

				deb_io ();
				fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
				fread (&rec, sizeof (Wps), 1, fptr);
				fin_io ();

				if (display_wp_line (&rec))
					pvoie->temp1 = 1;

			}
			if (pvoie->memoc >= MAXMEM)
			{
				pvoie->sr_mem = TRUE;
				retour = 0;
				break;
			}
			if (trait_time > MAXTACHE)
			{
				pvoie->seq = TRUE;
				retour = 0;
				break;
			}
			++record;
		}

	}
	else
	{
		while (read_wp (record, &wp))
		{
			if ((wp.callsign) && (strmatch (l2call (wp.home), mask)))
			{

				deb_io ();
				fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
				fread (&rec, sizeof (Wps), 1, fptr);
				fin_io ();

				if (display_wp_line (&rec))
					pvoie->temp1 = 1;

			}
			if (pvoie->memoc >= MAXMEM)
			{
				pvoie->sr_mem = TRUE;
				retour = 0;
				break;
			}
			if (trait_time > MAXTACHE)
			{
				pvoie->seq = TRUE;
				retour = 0;
				break;
			}
			++record;
		}
	}

	fclose (fptr);

	pvoie->noenr_menu = record;

	if (retour && (pvoie->temp1 == 0))
		texte (T_ERR + 19);

	return (retour);
}

static int wp_search_name (void)
{
	char mask[42];
	FILE *fptr;
	Wps rec;
	unsigned record;
	int retour = 1;
	int ok;

	if (!*pvoie->ch_temp)
	{
		texte (T_ERR + 2);
		return (retour);
	}

	pvoie->sr_mem = pvoie->seq = FALSE;

	record = (unsigned) pvoie->noenr_menu;
	strn_cpy (40, mask, pvoie->ch_temp);


	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (1);

	while (1)
	{

		deb_io ();
		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		ok = fread (&rec, sizeof (Wps), 1, fptr);
		fin_io ();

		if (ok == 0)
			break;

		if ((*rec.first_homebbs) && (*rec.name) && (*rec.name != '?'))
		{
			if ((*rec.callsign) && (strstr (strupr (rec.name), mask)))
			{
				display_wp_line (&rec);
				pvoie->temp1 = 1;
			}
		}
		++record;
		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 0;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 0;
			break;
		}
	}
	fclose (fptr);

	pvoie->noenr_menu = record;

	if (retour && (pvoie->temp1 == 0))
		texte (T_ERR + 19);

	return (retour);
}

static int wp_search_qth (void)
{
	char mask[42];
	FILE *fptr;
	Wps rec;
	unsigned record;
	int retour = 1;
	int ok;

	if (!*pvoie->ch_temp)
	{
		texte (T_ERR + 2);
		return (retour);
	}

	pvoie->sr_mem = pvoie->seq = FALSE;

	record = (unsigned) pvoie->noenr_menu;
	strn_cpy (40, mask, pvoie->ch_temp);


	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (1);

	while (1)
	{

		deb_io ();
		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		ok = fread (&rec, sizeof (Wps), 1, fptr);
		fin_io ();

		if (ok == 0)
			break;

		if ((*rec.first_homebbs) && (*rec.first_qth) && (*rec.first_qth != '?'))
		{
			if ((*rec.callsign) && (strstr (strupr (rec.first_qth), mask)))
			{
				display_wp_line (&rec);
				pvoie->temp1 = 1;
			}
		}
		++record;
		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 0;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 0;
			break;
		}
	}
	fclose (fptr);

	pvoie->noenr_menu = record;

	if (retour && (pvoie->temp1 == 0))
		texte (T_ERR + 19);

	return (retour);
}

static int wp_search_zip (void)
{
	char mask[42];
	FILE *fptr;
	Wps rec;
	unsigned record;
	int retour = 1;
	int ok;

	if (!*pvoie->ch_temp)
	{
		texte (T_ERR + 2);
		return (retour);
	}

	pvoie->sr_mem = pvoie->seq = FALSE;

	record = (unsigned) pvoie->noenr_menu;
	strn_cpy (40, mask, pvoie->ch_temp);


	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (1);

	while (1)
	{

		deb_io ();
		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		ok = fread (&rec, sizeof (Wps), 1, fptr);
		fin_io ();

		if (ok == 0)
			break;

		if (*rec.first_homebbs)
		{
			if ((*rec.callsign) && (strmatch (rec.first_zip, mask)))
			{
				display_wp_line (&rec);
				pvoie->temp1 = 1;
			}
		}
		++record;
		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 0;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 0;
			break;
		}
	}
	fclose (fptr);

	pvoie->noenr_menu = record;

	if (retour && (pvoie->temp1 == 0))
		texte (T_ERR + 19);

	return (retour);
}

static int wp_search_area (void)
{
	char *ptr;

	char mask[42];
	char route[42];
	FILE *fptr;
	Wps rec;
	unsigned record;
	int retour = 1;
	int ok;


	if (!*pvoie->ch_temp)
	{
		texte (T_ERR + 2);
		return (retour);
	}

	pvoie->sr_mem = pvoie->seq = FALSE;

	record = (unsigned) pvoie->noenr_menu;
	strn_cpy (40, mask, pvoie->ch_temp);


	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return (1);

	while (1)
	{

		deb_io ();
		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		ok = fread (&rec, sizeof (Wps), 1, fptr);
		fin_io ();

		if (ok == 0)
			break;

		if (*rec.first_homebbs)
		{
			strn_cpy (40, route, rec.first_homebbs);
			ptr = strtok (route, ".");

			while (ptr)
			{
				if (strcmp (ptr, mask) == 0)
				{
					display_wp_line (&rec);
					pvoie->temp1 = 1;
					break;
				}
				ptr = strtok (NULL, ".");
			}
		}

		++record;
		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 0;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 0;
			break;
		}
	}
	fclose (fptr);

	pvoie->noenr_menu = record;

	if (retour && (pvoie->temp1 == 0))
		texte (T_ERR + 19);

	return (retour);
}

static char *replace_space (char *ptr)
{
	char *scan = ptr;

	while (*scan)
	{
		if (*scan == ' ')
			*scan = '-';
		++scan;
	}
	return (ptr);
}

static void update_user_home_bbs (char *callsign, char *homebbs)
{
	FILE *fpti;
	info frec;
	unsigned num_indic;
	char call[8];
	char *ptr;

	/* Modifie le user eventuellement */
	pvoie->emis = insnoeud (callsign, &num_indic);
	if (pvoie->emis->coord != 0xffff)
	{
		/* Seulement l'indicatif */
		n_cpy (6, call, homebbs);
		ptr = strchr (call, '.');
		if (ptr)
			*ptr = '\0';

		fpti = ouvre_nomenc ();
		fseek (fpti, (long) pvoie->emis->coord * sizeof (info), 0);
		fread (&frec, sizeof (info), 1, fpti);
		strcpy (frec.home, call);
		fseek (fpti, (long) pvoie->emis->coord * sizeof (info), 0);
		fwrite (&frec, sizeof (info), 1, fpti);
		ferme (fpti, 39);
	}
}

static void modif_wp_record (long record)
{
	int c;
	int err = 0;
	FILE *fptr;
	Wp wp;
	Wps rec;
	unsigned num_indic;

	fptr = fopen (d_disque ("WP\\WP.SYS"), "r+b");
	if (fptr == NULL)
		return;

	deb_io ();

	fseek (fptr, record * sizeof (Wps), SEEK_SET);
	fread (&rec, sizeof (Wps), 1, fptr);

	c = *indd++;
	switch (toupper (c))
	{
	case 'N':
		incindd ();
		n_cpy (12, rec.name, replace_space (indd));
		break;
	case 'U':
		n_cpy (40, rec.first_homebbs, rec.secnd_homebbs);
		n_cpy (30, rec.first_qth, rec.secnd_qth);
		n_cpy (8, rec.first_zip, rec.secnd_zip);
		read_wp ((unsigned) record, &wp);
		wp.home = call2l (bbs_via (rec.first_homebbs));
		write_wp ((unsigned) record, &wp);
		update_user_home_bbs (rec.callsign, rec.first_homebbs);
		break;
	case '1':
		c = *indd++;
		incindd ();

		if (strcmp (indd, ".") == 0)
			*indd = '\0';

		switch (toupper (c))
		{
		case 'H':
			strn_cpy (40, rec.first_homebbs, extend_bbs (indd));
			read_wp ((unsigned) record, &wp);
			wp.home = call2l (bbs_via (rec.first_homebbs));
			write_wp ((unsigned) record, &wp);
			update_user_home_bbs (rec.callsign, rec.first_homebbs);
			break;
		case 'Q':
			n_cpy (30, rec.first_qth, indd);
			break;
		case 'Z':
			strn_cpy (8, rec.first_zip, replace_space (indd));
			/* Modifie le zip eventuellement */
			pvoie->emis = insnoeud (rec.callsign, &num_indic);
			if (pvoie->emis->coord != 0xffff)
			{
				FILE *fpti;
				info frec;

				fpti = ouvre_nomenc ();
				fseek (fpti, (long) pvoie->emis->coord * sizeof (info), 0);
				fread (&frec, sizeof (info), 1, fpti);
				strcpy (frec.zip, rec.first_zip);
				fseek (fpti, (long) pvoie->emis->coord * sizeof (info), 0);
				fwrite (&frec, sizeof (info), 1, fpti);
				ferme (fpti, 39);
			}
			break;
		default:
			err = 1;
			texte (T_ERR + 0);
			break;
		}
		break;
	case '2':
		c = *indd++;
		incindd ();
		switch (toupper (c))
		{
		case 'H':
			strn_cpy (40, rec.secnd_homebbs, extend_bbs (indd));
			break;
		case 'Q':
			n_cpy (30, rec.secnd_qth, indd);
			break;
		case 'Z':
			strn_cpy (8, rec.secnd_zip, replace_space (indd));
			break;
		default:
			err = 1;
			texte (T_ERR + 0);
			break;
		}
		break;
	default:
		err = 1;
		texte (T_ERR + 0);
		break;
	}

	if (!err)
	{
		rec.changed = 'U';
		fseek (fptr, record * sizeof (Wps), SEEK_SET);
		fwrite (&rec, sizeof (Wps), 1, fptr);
	}

	fin_io ();

	fclose (fptr);

	display_wp_user (record, &rec);
}

static void kill_wp_record (long record)
{
	FILE *fptr;
	Wp wp;
	Wps rec;

	fptr = fopen (d_disque ("WP\\WP.SYS"), "r+b");
	if (fptr == NULL)
		return;

	deb_io ();

	fseek (fptr, record * sizeof (Wps), SEEK_SET);
	fread (&rec, sizeof (Wps), 1, fptr);

	*rec.callsign = '\0';
	*rec.first_homebbs = '\0';

	fseek (fptr, record * sizeof (Wps), SEEK_SET);
	fwrite (&rec, sizeof (Wps), 1, fptr);

	read_wp ((unsigned) record, &wp);
	wp.callsign = 0L;
	wp.home = 0L;
	write_wp ((unsigned) record, &wp);

	fin_io ();

	fclose (fptr);
}


static int wp_edit (void)
{
	int fin = 1;
	int prompt = 1;
	char s[80];

	sup_ln (indd);

	switch (pvoie->niv3)
	{
	case 20:
		prompt = 0;
		if (wp_list_user () == 0)
		{
			return (0);
		}
		if ((!*pvoie->ch_temp) || (pvoie->noenr_menu == -1L))
		{
			fin = 1;
		}
		else
		{
			ch_niv3 (21);
			var_cpy (0, pvoie->ch_temp);
			texte (T_MBL + 30);
			fin = 0;
		}
		break;
	case 21:
		if (toupper (*indd) == Oui)
		{
			kill_wp_record (pvoie->noenr_menu);
			fin = 1;
			prompt = 0;
		}
		else
		{
			fin = 0;
			ch_niv3 (22);
		}
		break;
	case 22:
		if (*indd)
		{
			modif_wp_record (pvoie->noenr_menu);
			fin = 0;
		}
		else
		{
			fin = 1;
			prompt = 0;
		}
		break;
	}
	if (prompt)
	{
		sprintf (s, "(U)pdate,(N)ame,(1H)ome,(1Z)ip,(1Q)th,(2H)ome,(2Z)ip,(2Q)th,(Return) >");
		outln (s, strlen (s));
	}
	return (fin);
}

int menu_wp_search (void)
{
	int menu = 1;
	int error = 0;
	int c;

	sup_ln (indd);
	c = *indd++;

	while_space ();

	pvoie->temp1 = 0;
	pvoie->noenr_menu = 0;
	strn_cpy (40, pvoie->ch_temp, indd);

	if (c == '\0')
	{
		mbl_info ();
		retour_mbl ();
		return (0);
	}

	if (!EMS_WPG_OK ())
	{
		return (1);
	}

	switch (toupper (c))
	{

	case ' ':
		if (wp_search_call () == 0)
		{
			menu = 0;
			ch_niv3 (1);
		}
		break;

	case '@':
		if (wp_search_bbs () == 0)
		{
			menu = 0;
			ch_niv3 (2);
		}
		break;

	case 'D':
		if (wp_stats () == 0)
		{
			menu = 0;
			ch_niv3 (10);
		}
		break;

	case 'E':
		if (droits (COSYSOP))
		{
			ch_niv3 (20);
			if (wp_edit () == 0)
			{
				menu = 0;
			}
		}
		else
			error = 1;
		break;

	case 'H':
		if (wp_search_area () == 0)
		{
			menu = 0;
			ch_niv3 (4);
		}
		break;

	case 'L':
		if (droits (COSYSOP))
		{
			if (wp_list_user () == 0)
			{
				menu = 0;
				ch_niv3 (5);
			}
		}
		else
			error = 1;
		break;

	case 'N':
		if (wp_search_name () == 0)
		{
			menu = 0;
			ch_niv3 (6);
		}
		break;

	case 'Q':
		if (wp_search_qth () == 0)
		{
			menu = 0;
			ch_niv3 (7);
		}
		break;

	case 'Z':
		if (wp_search_zip () == 0)
		{
			menu = 0;
			ch_niv3 (3);
		}
		break;

	default:
		error = 1;
		break;

	}
	if (!error && menu)
		retour_mbl ();

	return (error);
}

void wp_search (void)
{
	int fin;

	switch (pvoie->niv3)
	{
	case 1:
		fin = wp_search_call ();
		break;
	case 2:
		fin = wp_search_bbs ();
		break;
	case 3:
		fin = wp_search_zip ();
		break;
	case 4:
		fin = wp_search_area ();
		break;
	case 5:
		fin = wp_list_user ();
		break;
	case 6:
		fin = wp_search_name ();
		break;
	case 7:
		fin = wp_search_qth ();
		break;
	case 10:
		fin = wp_stats ();
		break;
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
		fin = wp_edit ();
		break;
	default:
		fin = 1;
		fbb_error (ERR_NIVEAU, "WP-SEARCH", pvoie->niv3);
		break;
	}
	if (fin)
		retour_mbl ();
}

/* Service WP */

static void wp_serv_callsign (FILE * fpto, char *mask)
{
	unsigned record = 0;
	FILE *fptr;
	int nb = 0;
	Wps rec;
	Wp wp;


	fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
	if (fptr == NULL)
		return;

	while (read_wp (record, &wp))
	{
		if ((wp.callsign) && (strmatch (l2call (wp.callsign), mask)))
		{
			deb_io ();
			fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
			fread (&rec, sizeof (Wps), 1, fptr);
			fin_io ();

			if (*rec.first_homebbs)
			{

				fprintf (fpto, "On %s %s @ %s zip %s %s %s\r\n",
						 date_mbl (rec.last_modif),
						 rec.callsign,
						 (*rec.first_homebbs) ? rec.first_homebbs : "?",
						 (*rec.first_zip) ? rec.first_zip : "?",
						 (*rec.name) ? rec.name : "?",
						 (*rec.first_qth) ? rec.first_qth : "?"
					);

				if (++nb == 100)
				{
					fprintf (fpto, "*** More than 100 entries for %s.\r\n", mask);
					break;
				}
			}
		}
		++record;
	}

	fclose (fptr);

	if (nb == 0)
	{
		fprintf (fpto, "*** %s not found.\n", mask);
	}

}

int wp_service (char *filename)
{
	int ok;
	int premier = 1;
	FILE *fptr;
	FILE *fpto;
	char buffer[256];
	char sender[80];
	char commande[80];
	char qmark[80];

	if (!EMS_WPG_OK ())
		return (1);

	fptr = fopen (filename, "rt");	/* Open the received message */
	if (fptr == NULL)
		return (1);

	fgets (buffer, 80, fptr);	/* Read the command line */
	sscanf (buffer, "%s %s %s %s\n", commande, commande, commande, sender);

	*qmark = '\0';
	fgets (buffer, 80, fptr);	/* Read the subject */


	if ((strcmp (sender, "WP") == 0) || (is_wpupdate (buffer)))
	{
		fclose (fptr);
		return (1);
	}

	fpto = fappend (MAILIN, "b");
	if (fpto == NULL)
		return (1);

	while (fgets (buffer, 80, fptr))
	{							/* Read the lines */
		strupr (buffer);
		*qmark = '\0';			/* Capitalize */
		sscanf (buffer, "%s %s", commande, qmark);	/* Scan lines */
		ok = 0;
		if (*qmark == '?')
		{
			ok = 1;
		}
		else if (*qmark == '\0')
		{
			if (commande[strlen (commande) - 1] == '?')
			{
				commande[strlen (commande) - 1] = '\0';
				ok = 1;
			}
		}
		if (ok)
		{
			if (premier)
			{
				fprintf (fpto, "#\r\n");	/* Tell that this is a message from this BBS */
				fprintf (fpto, "SP %s @ %s < %s\r\n", sender, pvoie->mess_home, mycall);
				fprintf (fpto, "WP Reply\r\n");
				premier = 0;
			}
			wp_serv_callsign (fpto, commande);
		}
	}

	if (!premier)
		fprintf (fpto, "[ From WP @ %s ]\r\n/EX\r\n", mycall);

	fclose (fpto);

	fclose (fptr);				/* All needed is read */
	return (0);					/* Retour ok */

}

/*

   Cherche un home BBS pour le destinataire specifie et le met dans bbsv
   le champ bbsv doit etre vide en entree.

   retourne 1 si le destinataire a ete trouve dans la base.

 */

int route_wp_home (bullist * pbull)
{
	FILE *fptr;
	unsigned record;
	int retour = 0;
	Wps rec;

	record = search_wp_record (call2l (pbull->desti), USR_CALL, 0);
	if (record < 0xffff)
	{

		deb_io ();

		fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
		if (fptr == NULL)
		{
			fin_io ();
			return (0);
		}

		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		fread (&rec, sizeof (Wps), 1, fptr);

		fclose (fptr);

		if (*rec.first_homebbs)
		{
			strn_cpy (40, pbull->bbsv, rec.first_homebbs);
			retour = 1;
		}

		fin_io ();
	}

	return (retour);
}

/*

   Cherche l'extension hierarchique d'une BBS et la met dans bbsv
   le champ bbsv ne doit pas avoir d'extension en entree.

   retourne 1 si l'extension a ete trouvee dans la base.

 */

int route_wp_hier (bullist * pbull)
{
	FILE *fptr;
	unsigned record;
	int retour = 0;
	Wps rec;

	if (!EMS_WPG_OK ())
		return (0);

	record = search_wp_record (call2l (pbull->bbsv), BBS_CALL, 0);
	if (record < 0xffff)
	{

		deb_io ();

		fptr = fopen (d_disque ("WP\\WP.SYS"), "rb");
		if (fptr == NULL)
		{
			fin_io ();
			return (0);
		}

		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		fread (&rec, sizeof (Wps), 1, fptr);

		if (*rec.first_homebbs)
		{
			strn_cpy (40, pbull->bbsv, rec.first_homebbs);
			retour = 1;
		}

		fclose (fptr);

		fin_io ();
	}

	return (retour);
}


static char *l2call (lcall val)
{
	static char callsign[7];
	char *ptr = callsign;
	unsigned long c;

	c = val / 69343957L;
	if (c)
	{
		val -= (c * 69343957L);
		*ptr++ = (char) ((c >= 11) ? c + 54 : c + 47);
	}

	c = val / 1874161L;
	if (c)
	{
		val -= (c * 1874161L);
		*ptr++ = (char) ((c >= 11) ? c + 54 : c + 47);
	}

	c = val / 50653L;
	if (c)
	{
		val -= (c * 50653L);
		*ptr++ = (char) ((c >= 11) ? c + 54 : c + 47);
	}

	c = val / 1369L;
	if (c)
	{
		val -= (c * 1369L);
		*ptr++ = (char) ((c >= 11) ? c + 54 : c + 47);
	}

	c = val / 37L;
	if (c)
	{
		val -= (c * 37L);
		*ptr++ = (char) ((c >= 11) ? c + 54 : c + 47);
	}

	c = val;
	if (c)
	{
		*ptr++ = (char) ((c >= 11) ? c + 54 : c + 47);
	}

	*ptr = '\0';
	return (callsign);
}
