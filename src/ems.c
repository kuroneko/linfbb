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
 * Gestion de la memoire EMS/XMS
 *
 */

#include <serv.h>

#ifdef __WIN32__
#define huge
#endif

#define	CTRL_Z		'\032'

static int read_exms_record (int, unsigned, char *, unsigned, unsigned);
static int realloc_bloc (int);
static int select_bloc (char *);
static int write_exms_record (int, unsigned, char *, unsigned, unsigned);

#define EMS_VECT   	0x67
#define	MAX_BUFLIG	16
#define NB_BLOCK	4

#define NB_LIG		(EMS_BLOC / sizeof(Ligne))

#ifdef __FBBDOS__
static Ligne *buf_ligne[MAX_BUFLIG];

#endif

#define DIRMES_REC	(EMS_BLOC / sizeof(bullist))
#define BID_REC 	(EMS_BLOC / BIDCOMP)
#define WPG_REC 	(EMS_BLOC / sizeof(Wp))
#define REJ_REC 	(EMS_BLOC / sizeof(Rej_rec))
#define CHN_REC  	(EMS_BLOC / sizeof(Svoie))


static char *ems_ptr = NULL;

Desc *desc;

static FILE *dir_ptr = NULL;
static int nb_dir = 0;			/* Nombre de d'ouvertures DIRMES en cours */

static int exms = NO_MS;
static int exms_pages = 0;

/* Ecritures / Lectures de record */

void end_exms (void)
{
	int file;
	int bloc;

	if (desc == NULL)
		return;

	for (file = 0; file < NB_EMS; file++)
	{
		for (bloc = 0; bloc < desc[file].max_bloc ; bloc++)
		{
			if (desc[file].alloc[bloc])
			{
				free (desc[file].alloc[bloc]);
				desc[file].alloc[bloc] = NULL;
			}
		}
	}
	free (desc);
	desc = NULL;
}

int read_dirmes (unsigned record, bullist * pbul)
{
	return (read_exms_record (DIRMES, record, (char *) pbul, sizeof (bullist), DIRMES_REC));
}

int write_dirmes (unsigned record, bullist * pbul)
{
	return (write_exms_record (DIRMES, record, (char *) pbul, sizeof (bullist), DIRMES_REC));
}

int read_fwd (unsigned record, bullist * pbul)
{
	return (read_exms_record (DIRMES, record, (char *) pbul, sizeof (bullist), DIRMES_REC));
}

int write_fwd (unsigned record, bullist * pbul)
{
	return (write_exms_record (DIRMES, record, (char *) pbul, sizeof (bullist), DIRMES_REC));
}


int read_wp (unsigned record, Wp * wp)
{
	return (read_exms_record (WPG, record, (char *) wp, sizeof (Wp), WPG_REC));
}

int write_wp (unsigned record, Wp * wp)
{
	return (write_exms_record (WPG, record, (char *) wp, sizeof (Wp), WPG_REC));
}


int read_bid (unsigned record, char *pbul)
{
	return (read_exms_record (WBID, record, (char *) pbul, BIDCOMP, BID_REC));
}

int write_bid (unsigned record, char *pbul)
{
	return (write_exms_record (WBID, record, (char *) pbul, BIDCOMP, BID_REC));
}

int read_rej (unsigned record, Rej_rec * rej)
{
	return (read_exms_record (REJET, record, (char *) rej, sizeof (Rej_rec), REJ_REC));
}

int write_rej (unsigned record, Rej_rec * rej)
{
	return (write_exms_record (REJET, record, (char *) rej, sizeof (Rej_rec), REJ_REC));
}

static int read_exms_record (int file, unsigned record, char *ptr, unsigned size, unsigned nb_rec)
{
	char *bloc;
	char *mptr;

	if (record >= desc[file].nb_records)
	{
		return (0);
	}

	bloc = desc[file].alloc[record / nb_rec];
	if (bloc == NULL)
		return (0);

	deb_io ();

	select_bloc (bloc);
	mptr = ems_ptr + (record % nb_rec) * size;

	memcpy (ptr, mptr, size);

	fin_io ();

	return (size);
}

static int write_exms_record (int file, unsigned record, char *ptr, unsigned size, unsigned nb_rec)
{
	char *bloc;
	char *mptr;
	int num_bloc = record / nb_rec;
	Desc *dsk = &desc[file];


	if (record > dsk->nb_records)
	{
		return (0);
	}

	if (record == dsk->nb_records)
		++(dsk->nb_records);

	while (num_bloc >= dsk->max_bloc)
	{
		int i;
		dsk->alloc = realloc(dsk->alloc, sizeof(char *) * (dsk->max_bloc+NB_BLOCK));
		for (i = dsk->max_bloc ; i < dsk->max_bloc+NB_BLOCK ; i++)
		{
			dsk->alloc[i] = NULL;
		}
		dsk->max_bloc += NB_BLOCK;
	}

	bloc = dsk->alloc[num_bloc];

	deb_io ();

	if (bloc == NULL)
	{
		if (!realloc_bloc (file))
		{
			return (0);
		}
		bloc = dsk->alloc[num_bloc];
		desc[file].size += (long) EMS_BLOC;
	}
	select_bloc (bloc);
	mptr = ems_ptr + (record % nb_rec) * size;

	memcpy (mptr, ptr, size);

	fin_io ();

	return (size);
}

char *new_bloc (int file)
{
	realloc_bloc (file);
	desc[file].size += (long) EMS_BLOC;
	return (ems_ptr);
}

char *sel_bloc (int file, int numero)
{
	if (desc[file].alloc[numero] != NULL)
	{
		select_bloc (desc[file].alloc[numero]);
		return (ems_ptr);
	}
	return (NULL);
}

void seek_exms_string (int file, long pos)
{
	char *bloc;
	Desc *dsk = &desc[file];

	dsk->pos = pos;
	dsk->num_bloc = (int) (pos / (long) EMS_BLOC);

	bloc = dsk->alloc[dsk->num_bloc];
	if (bloc == NULL)
		return;
	select_bloc (bloc);

	dsk->nb = EMS_BLOC - (int) (pos % (long) EMS_BLOC);
	dsk->ptr = ems_ptr + (int) (pos % (long) EMS_BLOC);
}

long tell_exms_string (int file)
{
	Desc *dsk = &desc[file];

	return (dsk->pos);
}

int read_exms_string (int file, char *ptr)
{
	int c;
	char *bloc;
	Desc *dsk = &desc[file];
	int nbcar;
	char *mptr;

	bloc = dsk->alloc[dsk->num_bloc];
	if (bloc == NULL)
		return (0);
	select_bloc (bloc);
	nbcar = dsk->nb;
	mptr = dsk->ptr;

	do
	{
		if (--nbcar == 0)
		{
			++dsk->num_bloc;
			if (dsk->num_bloc == dsk->max_bloc)
			{
				int i;
				dsk->max_bloc += NB_BLOCK;
				dsk->alloc = realloc(dsk->alloc, sizeof(char *) * dsk->max_bloc);
				for (i = dsk->num_bloc ; i < dsk->max_bloc ; i++)
				{
					dsk->alloc[i] = NULL;
				}
			}
			bloc = dsk->alloc[dsk->num_bloc];
			if (bloc == NULL)
				return (0);
			select_bloc (bloc);
			mptr = ems_ptr;
			nbcar = EMS_BLOC;
		}
		c = *mptr++;
		if (c == CTRL_Z)
		{
			--mptr;
			return (0);
		}
		*ptr++ = c;
		++dsk->pos;

	}
	while (c);

	*ptr = '\0';
	dsk->nb = nbcar;
	dsk->ptr = mptr;
	return (1);
}

int write_exms_string (int file, char *ptr)
{
	int c;
	char *bloc;
	Desc *dsk = &desc[file];
	int nbcar;
	char *mptr;

	bloc = dsk->alloc[dsk->num_bloc];
	if (bloc == NULL)
	{
		if (!realloc_bloc (file))
			return (0);
		bloc = dsk->alloc[dsk->num_bloc];
		dsk->nb = EMS_BLOC;
		dsk->ptr = ems_ptr;
		desc[file].size += (long) EMS_BLOC;

	}
	select_bloc (bloc);
	nbcar = dsk->nb;
	mptr = dsk->ptr;

	do
	{
		if (--nbcar == 0)
		{
			++dsk->num_bloc;
			if (dsk->num_bloc == dsk->max_bloc)
			{
				int i;
				dsk->max_bloc += NB_BLOCK;
				dsk->alloc = realloc(dsk->alloc, sizeof(char *) * dsk->max_bloc);
				for (i = dsk->num_bloc ; i < dsk->max_bloc ; i++)
				{
					dsk->alloc[i] = NULL;
				}
			}
			bloc = dsk->alloc[dsk->num_bloc];
			if (bloc == NULL)
			{
				if (!realloc_bloc (file))
					return (0);
				bloc = dsk->alloc[dsk->num_bloc];
				dsk->nb = EMS_BLOC;
				dsk->ptr = ems_ptr;
			}
			select_bloc (bloc);
			mptr = ems_ptr;
			nbcar = EMS_BLOC;
		}
		c = *ptr++;
		*mptr++ = c;
		++dsk->pos;

	}
	while (c);

	dsk->nb = nbcar;
	dsk->ptr = mptr;
	return (1);
}

static int select_bloc (char *bloc)
{
	if (bloc == ems_ptr)
		return (1);

	if (bloc == NULL)
		return (0);

	deb_io ();
	ems_ptr = bloc;
	fin_io ();

	return (1);
}

static int realloc_bloc (int file)
{
	char *mptr;

	++exms_pages;

	mptr = ems_ptr = calloc (1, EMS_BLOC);
	if (mptr == NULL)
	{
		fbb_error (ERR_EXMS, "REALLOCATE: Error DPMI", 0);
		libere_xems ();
		return (0);
	}

	desc[file].alloc[desc[file].tot_bloc] = mptr;
	++desc[file].tot_bloc;
	return (1);
}

void init_exms (void)
{
	int i;
	int j;

	if (ems_aut)
		ems_aut = 2;

	desc = (Desc *) calloc (NB_EMS, sizeof (Desc));
	if (desc == NULL)
	{
		ems_aut = 0;
	}

	for (i = 0; i < NB_EMS; i++)
	{
		desc[i].tot_bloc = 0;
		desc[i].num_bloc = 0;
		desc[i].nb_records = 0;
		desc[i].ptr = NULL;
		desc[i].nb = EMS_BLOC;
		desc[i].size = 0;
		desc[i].max_bloc = NB_BLOCK;
		desc[i].alloc = malloc(sizeof(char *) * desc[i].max_bloc);
		for (j = 0; j < NB_BLOCK; j++)
			desc[i].alloc[j] = NULL;
	}

	exms = NO_MS;

	if (ems_aut)
	{

		exms = XMS;
		exms_pages = 0;

#ifdef ENGLISH
		if (!operationnel)
			cprintf ("%s Set-up    \r\n", typ_exms ());
#else
		if (!operationnel)
			cprintf ("Initialise %s\r\n", typ_exms ());
#endif
	}
	else
		in_exms = 0;

}

#ifdef __WINDOWS__

/*
 * Routines specifiques Windows
 */

static HGLOBAL hMem;

static int read_xms_bloc (unsigned page)
{
	char huge *mptr;

	if (page == 0xffff)
		return (1);

	mptr = GlobalLock (hMem);
	if (mptr == 0)
	{
		fbb_error (ERR_EXMS, "Error WIN_READ", 0);
		return (0);
	}

	_fmemcpy (ems_ptr, mptr + ((long) page * (long) EMS_BLOC), EMS_BLOC);

	GlobalUnlock (hMem);

	return (1);
}


static int write_xms_bloc (unsigned page)
{
	char huge *mptr;

	if (page == 0xffff)
		return (1);

	mptr = GlobalLock (hMem);
	if (mptr == NULL)
	{
		fbb_error (ERR_EXMS, "Error WIN_WRITE", 0);
		return (0);
	}

	_fmemcpy (mptr + ((long) page * (long) EMS_BLOC), ems_ptr, EMS_BLOC);

	GlobalUnlock (hMem);
	return (1);
}


static int xms_ok (void)
{
	return (1);
}

static int alloue_xms (void)
{
	hMem = GlobalAlloc (GMEM_MOVEABLE, EMS_BLOC);
	if (hMem == NULL)
	{
		fbb_error (ERR_EXMS, "ALLOCATE: Error WIN", 0);
		libere_xems ();
	}

	if (hMem)
		exms_pages = 0;

	return ((hMem != NULL));
}


static int realloue_xms_page (void)
{
	DWORD nb_bytes;

	deb_io ();
	++exms_pages;

	nb_bytes = (long) exms_pages *(long) EMS_BLOC;

	hMem = GlobalReAlloc (hMem, nb_bytes, GMEM_MOVEABLE);
	FbbMem ();
	if (hMem == NULL)
	{
		fbb_error (ERR_EXMS, "REALLOCATE: Error WIN", 0);
		libere_xems ();
		return (0);
	}
	else
		return (1);
}

#endif


char *typ_exms (void)
{
	static char s[30];

	if (exms == NO_MS)
		sprintf (s, "No high memory");

	else
	{
#ifdef __WINDOWS__
		sprintf (s, "Windows virtual paged memory");
#elif __LINUX__
		sprintf (s, "LINUX virtual paged memory");
#else
		sprintf (s, "DPMI virtual paged memory");
#endif
	}

	return (s);
}

int nb_ems_pages (void)
{
	return (exms_pages);
}

void libere_xems (void)
{

	/* Liberer les zones allouees !! */

	exms = NO_MS;
	in_exms = 0;
}


void ouvre_dir (void)
{
	++nb_dir;

	if (!EMS_MSG_OK ())
	{
		/* Pas d'EMS, on ouvre le fichier */
		if (dir_ptr == NULL)
			dir_ptr = ouvre_dirmes ();
	}
}


void ferme_dir (void)
{
	if (--nb_dir == 0)
	{
		if (dir_ptr)
			ferme (dir_ptr, 45);
		dir_ptr = NULL;
	}
}


#define ECRIT 1
#define LIT   2

unsigned length_dir (void)
{
	if (EMS_MSG_OK ())
		return (desc[DIRMES].nb_records);

	fseek (dir_ptr, 0L, SEEK_END);
	return ((unsigned) (ftell (dir_ptr) / (long) sizeof (bullist)));
}


int read_dir (unsigned record, bullist * pbul)
{
	if (EMS_MSG_OK ())
		return (read_dirmes (record, pbul));

	fseek (dir_ptr, (long) record * sizeof (bullist), SEEK_SET);
	return (fread (pbul, sizeof (bullist), 1, dir_ptr));
}


int write_dir (unsigned record, bullist * pbul)
{
	if (dir_ptr == NULL)
		dir_ptr = ouvre_dirmes ();

	if (EMS_MSG_OK ())
	{
		/*      if (operationnel) printf("Ecrit record %u offset %d Max %u\n", record, offset, nb_record); */
		if (!write_dirmes (record, pbul))
			in_exms &= (~EMS_MSG);
	}

	fseek (dir_ptr, (long) record * sizeof (bullist), SEEK_SET);
	return (fwrite (pbul, sizeof (bullist), 1, dir_ptr));
}

/* Gestion des BIDs */
int where_exms_bid (char *bid)
{
	char *bloc;
	int page;
	int i;
	int nb_bid;
	char *ptr = NULL;
	char t_bid[BIDCOMP];

	memcpy (t_bid, comp_bid (bid), BIDCOMP);

	page = 0;
	nb_bid = BID_REC;

	for (i = 0; i < maxbbid; i++)
	{
		if (nb_bid == BID_REC)
		{
			bloc = desc[WBID].alloc[page];
			select_bloc (bloc);
			nb_bid = 0;
			ptr = ems_ptr;
			/* dprintf("cherche dans page virtuelle %d, reelle %d\r\n", page, bloc); */
			++page;
		}
		if (strncmp (ptr, t_bid, BIDCOMP) == 0)
		{
			return (i + 1);
		}
		ptr += BIDCOMP;
		++nb_bid;
	}
	return (0);
}

void tst_exms_bid (bullist * fb_mess, int nb, int *t_res)
{
	int j;
	char *bloc;
	int page;
	int i;
	int nb_bid;
	char *ptr = NULL;
	char t_bid[BIDCOMP];

	for (j = 0; j < nb; j++)
	{

		if ((t_res[j]) || (*fb_mess[j].bid == '\0'))
			continue;

		memcpy (t_bid, comp_bid (fb_mess[j].bid), BIDCOMP);

		page = 0;
		nb_bid = BID_REC;

		deb_io ();
		fin_io ();				/* Pour laisser un peu de temps aux autres ! */

		for (i = 0; i < maxbbid; i++)
		{
			if (nb_bid == BID_REC)
			{
				bloc = desc[WBID].alloc[page];
				select_bloc (bloc);
				nb_bid = 0;
				ptr = ems_ptr;
				++page;
			}
			if (strncmp (ptr, t_bid, BIDCOMP) == 0)
			{
				t_res[j] = 1;
				break;
			}
			ptr += BIDCOMP;
			++nb_bid;
		}
	}
}

void delete_exms_bid (int pos)
{
	int j;
	char *bloc;
	int page;
	int i;
	int nb_bid;
	char *ptr = NULL;

	page = 0;
	--pos;
	nb_bid = BID_REC;

	for (i = 0; i < maxbbid; i++)
	{
		if (nb_bid == BID_REC)
		{
			bloc = desc[WBID].alloc[page];
			select_bloc (bloc);
			nb_bid = 0;
			ptr = ems_ptr;
			++page;
		}
		if (pos == i)
		{
			for (j = 0; j < BIDCOMP; j++, *ptr++ = '\0');
			break;
		}
		ptr += BIDCOMP;
		++nb_bid;
	}
}

void init_exms_bid (FILE * fptr)
{
	int i;

#ifdef __FBBDOS__
	int page = 0;
#endif

	int nb_bid = BID_REC;
	bidfwd fwbuf;

	for (i = 0; i < maxbbid; i++)
	{
		if (nb_bid == BID_REC)
		{
#if defined(__WINDOWS__) || defined(__LINUX__)
			char buf[80];

			InitText (itoa (i, buf, 10));
#else
			cprintf ("\rPage %d", page++);
#endif
			nb_bid = 0;
		}
		fread (&fwbuf, sizeof (bidfwd), 1, fptr);
		write_bid (i, comp_bid (fwbuf.fbid));
		++nb_bid;
	}
#if defined(__WINDOWS__) || defined(__LINUX__)
	{
		char buf[80];

		InitText (itoa (i, buf, 10));
	}
#else
	cprintf ("\rPage %d", page);
#endif
}

#ifdef __WINDOWS__
/* Rien sous Windows */
#elif __DPMI16__
static Ligne *current_line (int bloc, int off)
{
	unsigned offset;
	char *page;
	char *mptr;
	static Ligne ligne;

	page = desc[SCREEN].alloc[bloc];
	offset = (unsigned) off *sizeof (Ligne);

	mptr = page;
	if (mptr == NULL)
	{
		fbb_error (ERR_EXMS, "Error DPMI_READ", 0);
		return (NULL);
	}

	memcpy (&ligne, mptr + offset, sizeof (Ligne));

	return (&ligne);
}

Ligne *sel_scr (FScreen * screen, int lig)
{
	int bloc;
	int off;

	lig += screen->first;
	off = lig % NB_LIG;
	bloc = lig / NB_LIG;

	if (EMS_SCR_OK ())
	{
		return (current_line (bloc, off));
	}
	else
	{
		return (&(buf_ligne[bloc][off]));
	}
}

void wr_scr (FScreen * screen, Ligne * ligne, int lig)
{
	unsigned offset;
	int bloc;
	int off;
	char *page;
	char *mptr;

	if ((!EMS_SCR_OK ()) || (exms != XMS))
		return;

	lig += screen->first;
	off = lig % NB_LIG;
	bloc = lig / NB_LIG;

	page = desc[SCREEN].alloc[bloc];
	offset = (unsigned) off *sizeof (Ligne);

	mptr = page;
	if (mptr == NULL)
	{
		fbb_error (ERR_EXMS, "Error DPMI_WRITE", 0);
		return;
	}

	memcpy (mptr + offset, ligne, sizeof (Ligne));
}

void alloue_screen (int nb_lignes)
{
	int i;
	int nb;

	if (nb_lignes == 0)
		return;

#ifdef ENGLISH
	cprintf ("Allocating screen buffers\r\n");
#else
	cprintf ("Alloue les buffers ‚cran \r\n");
#endif

	if (EMS_SCR_OK ())
	{
		for (i = 0;; i++)
		{
			nb = (nb_lignes > NB_LIG) ? NB_LIG : nb_lignes;
			realloc_bloc (SCREEN);
			desc[SCREEN].size += (long) EMS_BLOC;
			nb_lignes -= nb;
			if (nb_lignes == 0)
				break;
		}
	}
	else
	{
		for (i = 0; i < MAX_BUFLIG; i++)
		{
			nb = (nb_lignes > NB_LIG) ? NB_LIG : nb_lignes;
			buf_ligne[i] = (Ligne *) calloc (nb, sizeof (Ligne));
			nb_lignes -= nb;
			if (nb_lignes == 0)
				break;
		}
	}
}

#endif

unsigned search_wp_record (lcall icall, int what, unsigned first_record)
{
	unsigned record;
	unsigned max;

	char *bloc;
	int page;
	int nb_wp_page;
	Wp *wp = NULL;

	max = desc[WPG].nb_records;

	nb_wp_page = WPG_REC;

	page = 0;

	if (what == USR_CALL)
	{
		for (record = first_record; record < max; record++)
		{

			if (nb_wp_page == WPG_REC)
			{
				deb_io ();
				fin_io ();		/* Pour laisser un peu de temps aux autres ! */
				bloc = desc[WPG].alloc[page];
				select_bloc (bloc);
				nb_wp_page = 0;
				wp = (Wp *) ems_ptr;
				/* dprintf("cherche dans page virtuelle %d, reelle %d\r\n", page, bloc); */
				++page;
			}
			if (wp->callsign == icall)
			{
				return (record);
			}
			++wp;
			++nb_wp_page;
		}
	}
	else if (what == BBS_CALL)
	{
		for (record = first_record; record < max; record++)
		{

			if (nb_wp_page == WPG_REC)
			{
				deb_io ();
				fin_io ();		/* Pour laisser un peu de temps aux autres ! */
				bloc = desc[WPG].alloc[page];
				select_bloc (bloc);
				nb_wp_page = 0;
				wp = (Wp *) ems_ptr;
				++page;
			}
			if ((wp->callsign == icall) && (wp->home == icall))
			{
				return (record);
			}
			++wp;
			++nb_wp_page;
		}
	}
	return (0xffff);
}

int high_memory_type (void)
{
	return (exms);
}


unsigned xms_free (void)
{
	return (256);
}

void load_dirmes (void)
{
	FILE *fptr;
	int page = 0;

#ifdef __FBBDOS__
	fen *fen_ptr;

#endif
	bullist bull;
	int record = 0;

	if (!EMS_MSG_OK ())
		return;

	deb_io ();

#ifdef __FBBDOS__
	fen_ptr = open_win (10, 5, 50, 8, INIT, "Messages");
#endif

	fptr = ouvre_dirmes ();

	while (fread (&bull, sizeof (bullist), 1, fptr))
	{
		/* printf("Entre le %ld, record %d\n", bull.numero, record); */
		if (!write_dirmes (record, &bull))
		{
			in_exms &= (~EMS_MSG);
			break;
		}
		if ((record++ % DIRMES_REC) == 0)
		{
#if defined(__WINDOWS__) || defined(__LINUX__)
			char buf[80];

			sprintf (buf, "Page %d", page);
			InitText (buf);
#else
			cprintf ("\rPage %d", page);
#endif
			page++;
		}
	}

	ferme (fptr, 56);

#ifdef __FBBDOS__
	sleep_ (1);
	close_win (fen_ptr);
#endif
	fin_io ();
}
