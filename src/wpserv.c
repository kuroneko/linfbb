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
 * Serveur White Pages
 *
 * Do not overlay !
 *
 */

#include <serv.h>

#define LG_CACHE	20
#define MAX_UPD		10

static Wpr *wp_cache = NULL;
static int pos_cache = 0;

#define WP_TRACE

#ifdef WP_TRACE
static int deb_wp = 0;

#endif

void add_wp_trace (int val)
{
	deb_wp = val;
}

int is_wpupdate (char *titre)
{
	char str[10];

	strn_cpy (9, str, titre);
	return (strcmp ("WP UPDATE", str) == 0);
}

void debug_wp (char *deb)
{
	FILE *wp_fptr;

	if (!deb_wp)
		return;

	wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");

	if (!wp_fptr)
		return;

	fputs (deb, wp_fptr);

	fclose (wp_fptr);
}

static int wp_read (Wpr * rec)
{
	static long wp_record = 0L;
	int retour;
	FILE *fptr;

	deb_io ();

	fptr = fopen (d_disque ("WP\\FBB.WP"), "rb");
	if (fptr == NULL)
	{
		fin_io ();
		wp_record = 0L;
		return (0);
	}

	fseek (fptr, wp_record * sizeof (Wpr), SEEK_SET);
	retour = fread (rec, sizeof (Wpr), MAX_UPD, fptr);

	fclose (fptr);

	if (retour == MAX_UPD)
	{
		wp_record += (long) retour;
	}
	else
	{
		unlink (d_disque ("WP\\FBB.WP"));
		wp_record = 0L;
	}

	fin_io ();

	return (retour);
}

#ifdef WP_TRACE
static void debug_wp_user (char *type, FILE * fptr, long record, Wps * rec, Wpr * ask)
{
	char modif[80];
	char seen[80];

	if (!deb_wp)
		return;

	strcpy (modif, date_mbl (rec->last_modif));
	strcpy (seen, date_mbl (rec->last_seen));

	fprintf (fptr, "[%s] %-4ld: %s %s %-6s (%u) %s (source %c)\n",
			 type,
			 record,
			 modif,
			 seen,
			 rec->callsign,
			 rec->seen,
			 (*rec->name) ? rec->name : "?",
			 rec->changed
		);

	fprintf (fptr, "  [1] @%s Zip:%s Qth:%s\n",
			 rec->first_homebbs,
			 (*rec->first_zip) ? rec->first_zip : "?",
			 (*rec->first_qth) ? rec->first_qth : "?"
		);

	fprintf (fptr, "  [2] @%s Zip:%s Qth:%s\n",
			 rec->secnd_homebbs,
			 (*rec->secnd_zip) ? rec->secnd_zip : "?",
			 (*rec->secnd_qth) ? rec->secnd_qth : "?"
		);

	fprintf (fptr, "Ask on %s source=%c local=%d :\n    %s@%s Name:%s Zip:%s Qth:%s\n",
			 date_mbl (ask->last),
			 ask->source,
			 ask->local,
			 ask->callsign,
			 ask->homebbs,
			 ask->name,
			 ask->zip,
			 ask->qth
		);
}
#endif

static void wp_update (Wpr * rec)
{
	Wps up_rec;
	char *ptr;
	FILE *fptr;
	int ok = 0;
	Wp wp;

/*  int     cur_page; */
	int maj;
	int upd;
	unsigned record;

#ifdef WP_TRACE
	FILE *wp_fptr = NULL;
	Wps old_rec;

#endif

	deb_io ();

	upd = maj = 0;

	fptr = fopen (d_disque ("WP\\WP.SYS"), "r+b");
	if (fptr == NULL)
	{
		fptr = fopen (d_disque ("WP\\WP.SYS"), "w+b");
		if (fptr == NULL)
		{
			/* remet_bloc(cur_page); */
			fin_io ();
			return;
		}
	}

	record = search_wp_record (call2l (rec->callsign), USR_CALL, 0);
	if (record < 0xffff)
		ok = 1;

	if (ok)
	{

#ifdef WP_TRACE
#endif
		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
		fread (&up_rec, sizeof (Wps), 1, fptr);

#ifdef WP_TRACE
		old_rec = up_rec;
#endif

		if (rec->last >= up_rec.last_seen)
		{

			up_rec.last_seen = rec->last;

			/* Name */

			if (*rec->name)
			{
				if ((*up_rec.name == '\0') || (rec->source == 'U'))
				{
					n_cpy (12, up_rec.name, rec->name);
					maj = 1;
#ifdef WP_TRACE
					if (deb_wp)
					{
						if (wp_fptr == NULL)
						{
							wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
							debug_wp_user ("OLD", wp_fptr, record, &old_rec, rec);
						}
						fprintf (wp_fptr, "Upd : Name = <%s>\n", rec->name);
					}
#endif

				}
			}

			/* Home BBS */
			if (*rec->homebbs)
			{
				if ((*rec->homebbs) && (strncmp (up_rec.secnd_homebbs, rec->homebbs, 40) != 0))
				{
					strn_cpy (40, up_rec.secnd_homebbs, rec->homebbs);
					upd = 1;
#ifdef WP_TRACE
					if (deb_wp)
					{
						if (wp_fptr == NULL)
						{
							wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
							debug_wp_user ("OLD", wp_fptr, record, &old_rec, rec);
						}
						fprintf (wp_fptr, "2nd : New HomeBBS = <%s>\n", rec->homebbs);
					}
#endif

				}
				if ((*up_rec.first_homebbs == '\0') || (rec->source == 'U') ||
					(!strchr (up_rec.first_homebbs, '.') && strchr (rec->homebbs, '.')))
				{
					strn_cpy (40, up_rec.first_homebbs, rec->homebbs);
					maj = 1;
#ifdef WP_TRACE
					if (deb_wp)
					{
						if (wp_fptr == NULL)
						{
							wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
							debug_wp_user ("OLD", wp_fptr, record, &old_rec, rec);
						}
						fprintf (wp_fptr, "1st : New HomeBBS = <%s>\n", rec->homebbs);
					}
#endif
				}
			}

			/* ZIP */

			if (*rec->zip)
			{
				ptr = rec->zip;
				while (*ptr)
				{				/* Remplace les espaces par '_' */
					if (isspace (*ptr))
						*ptr = '_';
					++ptr;
				}
				if (strncmp (up_rec.secnd_zip, rec->zip, 8) != 0)
				{
					strn_cpy (8, up_rec.secnd_zip, rec->zip);
					upd = 1;
#ifdef WP_TRACE
					if (deb_wp)
					{
						if (wp_fptr == NULL)
						{
							wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
							debug_wp_user ("OLD", wp_fptr, record, &old_rec, rec);
						}
						fprintf (wp_fptr, "2nd : New Zip = <%s>\n", rec->zip);
					}
#endif

				}
				if ((*up_rec.first_zip == '\0') || (rec->source == 'U'))
				{
					strn_cpy (8, up_rec.first_zip, rec->zip);
					maj = 1;
#ifdef WP_TRACE
					if (deb_wp)
					{
						if (wp_fptr == NULL)
						{
							wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
							debug_wp_user ("OLD", wp_fptr, record, &old_rec, rec);
						}
						fprintf (wp_fptr, "1st : New Zip = <%s>\n", rec->zip);
					}
#endif

				}
			}

			/* QTH */

			if (*rec->qth)
			{
				if (strncmp (up_rec.secnd_qth, rec->qth, 30) != 0)
				{
					n_cpy (30, up_rec.secnd_qth, rec->qth);
					upd = 1;
#ifdef WP_TRACE
					if (deb_wp)
					{
						if (wp_fptr == NULL)
						{
							wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
							debug_wp_user ("OLD", wp_fptr, record, &old_rec, rec);
						}
						fprintf (wp_fptr, "2nd : New Qth = <%s>\n", rec->qth);
					}
#endif

				}
				if ((*up_rec.first_qth == '\0') || (rec->source == 'U'))
				{
					n_cpy (30, up_rec.first_qth, rec->qth);
					maj = 1;
#ifdef WP_TRACE
					if (deb_wp)
					{
						if (wp_fptr == NULL)
						{
							wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
							debug_wp_user ("OLD", wp_fptr, record, &old_rec, rec);
						}
						fprintf (wp_fptr, "1st : New Qth = <%s>\n", rec->qth);
					}
#endif

				}
			}

			/* Mise a jour de la base */

			if (maj || upd)
			{
				if ((up_rec.changed != 'U') && !rec->local)
					up_rec.changed = rec->source;
				up_rec.last_modif = rec->last;
#ifdef WP_TRACE
				if (deb_wp)
				{
					if (wp_fptr == NULL)
					{
						wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
					}
					fprintf (wp_fptr, "1st = %d 2nd = %d\n", maj, upd);
					debug_wp_user ("WRT", wp_fptr, record, &up_rec, rec);
				}
#endif
			}
			else
			{
#ifdef WP_TRACE
				if (deb_wp >= 3)
				{
					if (wp_fptr == NULL)
					{
						wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
					}
					fprintf (wp_fptr, "WP not updated, no difference with WP database\n");
					debug_wp_user ("TST", wp_fptr, record, &up_rec, rec);
				}
#endif
			}

			/* Mise a jour de la date de derniere modif */

			++up_rec.seen;
			fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);
			fwrite (&up_rec, sizeof (Wps), 1, fptr);

			/*
			   if (upd || maj)
			   up_rec.secnd_date = time(NULL);
			 */

			if (maj)
			{
				wp.callsign = call2l (rec->callsign);
				wp.home = call2l (bbs_via (up_rec.first_homebbs));
				write_wp (record, &wp);
			}

		}
		else
		{
#ifdef WP_TRACE
			if (deb_wp >= 2)
			{
				if (wp_fptr == NULL)
				{
					wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
				}
				fprintf (wp_fptr, "WP not updated, information on %s is older\n",
						 date_mbl (rec->last));
				debug_wp_user ("TST", wp_fptr, record, &up_rec, rec);
			}
#endif
		}
	}
	else
	{
		fflush (fptr);
		record = (unsigned) (filelength (fileno (fptr)) / (long) sizeof (Wps));

		memset (&up_rec, '\0', sizeof (Wps));
		up_rec.free = 0;
		up_rec.changed = 0;
		up_rec.seen = 1;
		up_rec.last_modif = up_rec.last_seen = rec->last;
		strn_cpy (6, up_rec.callsign, rec->callsign);
		n_cpy (12, up_rec.name, rec->name);
		strn_cpy (40, up_rec.first_homebbs, rec->homebbs);
		strn_cpy (8, up_rec.first_zip, rec->zip);
		n_cpy (30, up_rec.first_qth, rec->qth);
		strn_cpy (40, up_rec.secnd_homebbs, rec->homebbs);
		strn_cpy (8, up_rec.secnd_zip, rec->zip);
		n_cpy (30, up_rec.secnd_qth, rec->qth);

		fseek (fptr, (long) record * sizeof (Wps), SEEK_SET);

		wp.callsign = call2l (rec->callsign);
		wp.home = call2l (bbs_via (rec->homebbs));

		if (!rec->local)
			up_rec.changed = rec->source;
		fwrite (&up_rec, sizeof (Wps), 1, fptr);
		write_wp (record, &wp);

#ifdef WP_TRACE
		if (deb_wp)
		{
			if (wp_fptr == NULL)
			{
				wp_fptr = fopen (d_disque ("WP\\WP.DBG"), "at");
			}
			fprintf (wp_fptr, "Creates new record\n");
			debug_wp_user ("CRT", wp_fptr, record, &up_rec, rec);
		}
#endif
	}

	fclose (fptr);

#ifdef WP_TRACE
	if (deb_wp)
	{
		if (wp_fptr)
		{
			fprintf (wp_fptr, "\n\n");
			fclose (wp_fptr);
		}
	}
#endif

	fin_io ();
}

int wp_server (void)
{
	int nb;
	int i;
	int retour = 0;
	Wpr rec[MAX_UPD];

	df ("wp_server", 0);

	aff_etat ('X');

	if ((nb = wp_read (rec)) != 0)
	{
		for (i = 0; i < nb; i++)
		{
			wp_update (&rec[i]);
		}
		if (nb == MAX_UPD)
			retour = 1;
	}
	if (!retour)
	{
		dde_wp_serv = 0;
	}
	ff ();
	aff_msg_cons ();
	return (retour);
}

void ini_rec (Wpr * rec)
{
	rec->local = 0;
	rec->last = time (NULL);
	rec->source = '\0';
	*rec->homebbs = '\0';
	*rec->callsign = '\0';
	*rec->zip = '\0';
	*rec->name = '\0';
	*rec->qth = '\0';
}

int is_serv (char *call)
{
	serlist *lptr;

	lptr = tete_serv;
	while (lptr)
	{
		if (strcmp (lptr->nom_serveur, call) == 0)
			return (1);
		lptr = lptr->suiv;
	}
	return (0);
}

void wp_upd (Wpr * rec, int flush)
{
	FILE *fptr;


	if ((!EMS_WPG_OK ()) || (wp_cache == NULL))
		return;

	if (rec)
	{

		if (!rec->homebbs && !rec->name && !rec->zip && !rec->qth)
			return;

		if ((!find (rec->callsign)) || (is_serv (rec->callsign)) || (strcmp (rec->callsign, "SYSOP") == 0))
			return;

	}

	deb_io ();

	if ((rec) && (pos_cache < LG_CACHE))
	{
		wp_cache[pos_cache++] = *rec;
	}

	if ((pos_cache) && ((flush) || (pos_cache == LG_CACHE)))
	{
		fptr = fopen (d_disque ("WP\\FBB.WP"), "ab");
		if (fptr == NULL)
		{
			fin_io ();
			return;
		}

		fwrite (wp_cache, sizeof (Wpr), pos_cache, fptr);

		fclose (fptr);

		dde_wp_serv = 1;

		pos_cache = 0;
	}

	fin_io ();
}

void exped_wp (char *exped, char *route)
{
	Wpr rec;

	if (!EMS_WPG_OK ())
		return;

	if (!find (exped))
		return;

	ini_rec (&rec);

	rec.source = 'G';

	strn_cpy (6, rec.callsign, exped);

	strn_cpy (40, rec.homebbs, route);

	if (addr_check (route))
		wp_upd (&rec, 0);
}

void user_wp (info * finf)
{
	Wpr rec;

	if (!EMS_WPG_OK ())
		return;

	ini_rec (&rec);

	rec.source = 'U';

	strn_cpy (6, rec.callsign, finf->indic.call);

	if (*finf->home)
		strn_cpy (40, rec.homebbs, extend_bbs (finf->home));
	else
		strn_cpy (40, rec.homebbs, mypath);

	if (*finf->zip)
		strn_cpy (8, rec.zip, finf->zip);

	if (*finf->prenom != '?')
		n_cpy (12, rec.name, finf->prenom);

	if (*finf->ville)
		n_cpy (30, rec.qth, finf->ville);

	if (addr_check (rec.homebbs))
	{
		wp_upd (&rec, 1);
		flush_wp_cache ();
	}
}

void header_wp (long date, char *home, char *qth, char *zip)
{
	char st[80];
	char *ptr;
	Wpr rec;


	if (!EMS_WPG_OK ())
		return;

	ini_rec (&rec);

	rec.source = 'I';

	rec.last = date;
	strn_cpy (40, rec.homebbs, home);
	n_cpy (30, rec.qth, qth);
	strn_cpy (8, rec.zip, zip);

	strn_cpy (40, st, home);
	ptr = strchr (st, '.');
	if (ptr)
		*ptr = '\0';
	strn_cpy (6, rec.callsign, st);

	if (addr_check (home))
		wp_upd (&rec, 0);
}

void end_wp (void)
{
	if (wp_cache)
	{
		m_libere (wp_cache, sizeof (Wpr) * LG_CACHE);
		wp_cache = NULL;
	}
}

void init_wp_cache (void)
{
	wp_cache = (Wpr *) m_alloue (sizeof (Wpr) * LG_CACHE);
	pos_cache = 0;
}

void flush_wp_cache (void)
{
	wp_upd (NULL, 1);
}

lcall call2l (char *callsign)
{
	char *ptr = callsign;
	int c;
	lcall val = 0L;

	while ((c = (int) *ptr++) != 0)
	{
		if (c < 48)
			return (0xffffffffL);
		c -= 47;
		if (c > 10)
		{
			c -= 7;
			if (c > 36)
			{
				return (0xffffffffL);
			}
			else if (c < 11)
			{
				return (0xffffffffL);
			}
		}
		val *= 37;
		val += c;
	}
	return (val);
}
