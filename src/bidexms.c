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
 * Gestion des BID memoire EMS/XMS
 *
 */

#include <serv.h>

static unsigned nb_record;

static void tst_vbid (bullist *, int, int *);
static void tst_bid (bullist *, int, int *);
static int where_bid (char *bid);

/*
 * Routines standard
 */

void w_bid (void)
{
	int i;
	long lnb, maxbid;
	char *ptr;
	FILE *fptr;
	bidfwd fwbid;

	maxbid = maxbbid;
	if ((fptr = fopen (d_disque ("WFBID.SYS"), "r+b")) == NULL)
	{
#ifdef ENGLISH
		if (!svoie[CONSOLE]->sta.connect)
			cprintf ("\r\nw_bid() Warning : Creating file %s      \r\n\n", d_disque("WFBID.SYS"));
#else
		if (!svoie[CONSOLE]->sta.connect)
			cprintf ("\r\nw_bid() Warning : Cr‚ation du fichier %s\r\n\n", d_disque("WFBID.SYS"));
#endif
		if ((fptr = fopen (d_disque ("WFBID.SYS"), "wb")) == NULL)
		{
#ifdef ENGLISH
			cprintf ("\r\nw_bid() Error : Impossible to create file %s   \r\n\n", d_disque("WFBID.SYS"));
#else
			cprintf ("\r\nw_bid() Erreur : Impossible de creer le fichier %s\r\n\n", d_disque("WFBID.SYS"));
#endif
			fbb_error (ERR_CREATE, d_disque ("WFBID.SYS"), 0);
			exit(1);
		}
		else {
			ptr = (char *) &fwbid;
			for (i = 0; i < sizeof (bidfwd); i++)
			*ptr++ = '\0';
			fwrite (&fwbid, sizeof (bidfwd), 1, fptr);
			rewind (fptr);
		}
	}
	fread (&fwbid, sizeof (fwbid), 1, fptr);
	++fwbid.numero;
	if ((fwbid.numero > maxbid) || (fwbid.numero < 1L))
		fwbid.numero = 1L;
	lnb = fwbid.numero;
	rewind (fptr);
	fwrite (&fwbid, sizeof (fwbid), 1, fptr);
	fseek (fptr, fwbid.numero * sizeof (bidfwd), 0);
	fwbid.mode = ptmes->type;
	strncpy (fwbid.fbid, ptmes->bid, 12);
	fwbid.fbid[12] = '\0';
	fwbid.numero = ptmes->numero;
	fwrite (&fwbid, sizeof (fwbid), 1, fptr);
	ferme (fptr, 59);

	--lnb;
	if (bid_ptr)
	{
		/* Mise a jour du tableau */
		ptr = bid_ptr + (int) (BIDCOMP * lnb);
		memcpy (ptr, comp_bid (ptmes->bid), BIDCOMP);
	}
	else if (EMS_BID_OK ())
	{
		write_bid ((int) lnb, comp_bid (ptmes->bid));
	}
}

int search_bid (char *bid)
{
	int i;
	char *ptr;
	char t_bid[BIDCOMP];

	if (bid_ptr)
	{
		memcpy (t_bid, comp_bid (bid), BIDCOMP);
		ptr = bid_ptr;
		for (i = 0; i < maxbbid; i++)
		{
			if (memcmp (ptr, t_bid, BIDCOMP) == 0)
			{
				return (i + 1);
			}
			ptr += BIDCOMP;
		}
		return (0);
	}
	else if (EMS_BID_OK ())
		return (where_exms_bid (bid));
	else
		return (where_bid (bid));
}

void libere_bid (void)
{
	if (bid_ptr)
	{
		m_libere (bid_ptr, (unsigned) (BIDCOMP * maxbbid));
		bid_ptr = NULL;
	}
}

void end_bids (void)
{
	libere_bid ();
}

void cree_bid (void)
{
	int i;
	FILE *fptr;
	bidfwd fwbuf;
	bidfwd fwbid;
	char *ptr;

#if defined(__WINDOWS__) || defined(__linux__)
	char buffer[80];

#endif
#ifdef __FBBDOS__
	fen *fen_ptr;

#endif

	/* bid_exms = 0; */
	nb_record = 0;

	if ((fptr = fopen (d_disque ("WFBID.SYS"), "rb")) == NULL)
	{
		fprintf (stderr, "WFBID.SYS file is not present. Creating %s\n", d_disque("WFBID.SYS"));
		printf ("WFBID.SYS is not present. Creating %s\n",d_disque("WFBID.SYS"));

	if ((fptr = fopen (d_disque ("WFBID.SYS"), "wb")) == NULL)
		{
#ifdef ENGLISH
			cprintf ("\r\nError : Impossible to create file %s   \r\n\n", d_disque("WFBID.SYS"));
#else
			cprintf ("\r\nErreur : Impossible de cr<82>er le fichier %s\r\n\n", d_disque("WFBID.SYS"));
#endif
			fbb_error (ERR_CREATE, d_disque ("WFBID.SYS"), 0);
			exit(1);
		}
		else
		{
			ptr = (char *) &fwbid;
			for (i = 0; i < sizeof (bidfwd); i++)
			*ptr++ = '\0';
			fwrite (&fwbid, sizeof (bidfwd), 1, fptr);
			rewind (fptr);
		}
	}
		
	deb_io ();

#ifdef __FBBDOS__
	fen_ptr = open_win (10, 5, 50, 8, INIT, "BID");
#endif

	if (EMS_BID_OK ())
	{
		init_exms_bid (fptr);
	}
	else
	{
		if ((bid_ptr == NULL) && (maxbbid <= 5000) && (tot_mem > 100000L))
		{
			bid_ptr = m_alloue ((unsigned) (BIDCOMP * maxbbid));
			memset (bid_ptr, 0, (size_t) maxbbid * BIDCOMP);
			ptr = bid_ptr;
			for (i = 0; i < maxbbid; i++)
			{
				if ((i % 500) == 0)
				{
#if defined(__WINDOWS__) || defined(__linux__)
					InitText (ltoa ((long) i, buffer, 10));
#endif
#ifdef __FBBDOS__
					cprintf ("\r%d BIDs", i);
#endif
				}
				fread (&fwbuf, sizeof (bidfwd), 1, fptr);
				memcpy (ptr, comp_bid (fwbuf.fbid), BIDCOMP);
				ptr += BIDCOMP;
			}
#if defined(__WINDOWS__) || defined(__linux__)
			InitText (ltoa ((long) i, buffer, 10));
#endif
#ifdef __FBBDOS__
			cprintf ("\r%d BIDs", i);
#endif

		}

	}
	ferme (fptr, 71);
#ifdef __FBBDOS__
	sleep_ (1);
	close_win (fen_ptr);
#endif
	fin_io ();
}


static void tst_bid (bullist * fb_mess, int nb, int *t_res)
{
	int i, j, res;
	FILE *fptr;
	bidfwd fwbid;

	if ((fptr = fopen (d_disque ("WFBID.SYS"), "rb")) != NULL)
	{
		if (fread (&fwbid, sizeof (fwbid), 1, fptr) == 0)
			return;
		for (i = 0; i < maxbbid; i++)
		{
			if (fread (&fwbid, sizeof (fwbid), 1, fptr) == 0)
				break;
			res = 1;
			for (j = 0; j < nb; j++)	/* Test si tous sont trouves */
			{
				if ((t_res[j] == 0) && (*fb_mess[j].bid))
				{
					res = 0;
					break;
				}
			}
			if (res)
				break;
			for (j = 0; j < nb; j++)
			{
				if (t_res[j])
					continue;	/* Deja trouve ? */
				if (*fb_mess[j].bid == '\0')
					continue;
				if (strcmp (fwbid.fbid, fb_mess[j].bid) == 0)
				{
					t_res[j] = 1;
				}
			}
		}
		ferme (fptr, 70);
	}
}


static int where_bid (char *bid)
{
	int i;
	FILE *fptr;
	bidfwd fwbid;

	if ((fptr = fopen (d_disque ("WFBID.SYS"), "rb")) != NULL)
	{
		if (fread (&fwbid, sizeof (fwbid), 1, fptr) == 0)
			return (0);
		for (i = 0; i < maxbbid; i++)
		{
			if (fread (&fwbid, sizeof (fwbid), 1, fptr) == 0)
				break;
			if (strcmp (fwbid.fbid, bid) == 0)
			{
				return (i + 1);
			}
		}
		ferme (fptr, 70);
	}
	return (0);
}


static void tst_vbid (bullist * fb_mess, int nb, int *t_res)
{
	int i, j, res;
	char *ptr;
	char t_bid[MAX_FB][BIDCOMP];

	for (i = 0; i < nb; i++)
		if (*fb_mess[i].bid)
			memcpy (t_bid[i], comp_bid (fb_mess[i].bid), BIDCOMP);

	ptr = bid_ptr;
	for (i = 0; i < maxbbid; i++)
	{
		res = 1;
		for (j = 0; j < nb; j++)
		{						/* Test si tous sont trouves */
			if ((t_res[j] == 0) && (*fb_mess[j].bid))
			{
				res = 0;
				break;
			}
		}
		if (res)
			break;
		for (j = 0; j < nb; j++)
		{
			if (t_res[j])
				continue;		/* Deja trouve ? */
			if (*fb_mess[j].bid == '\0')
				continue;
			if (strncmp (ptr, t_bid[j], BIDCOMP) == 0)
			{
				t_res[j] = 1;
			}
		}
		ptr += BIDCOMP;
	}
}


/*
   Tests divers lors de la reception d'un message
   Retour :
   0 : Message accepte
   1 : Message deja recu [ ou rejete (protocole 0) ]
   2 : en cours de reception sur un autre canal
   3 :
   4 : Message rejete (protocole 1).
   5 : Message retenu (protocole 1).
 */
int deja_recu (bullist * fb_mess, int nb, int *t_res)
{
	int i, j, voie;


	for (i = 0; i < nb; i++)
	{
		t_res[i] = rejet (&fb_mess[i]);
		/*
		   if (t_res[i] == 0)
		   t_res[i] = retenu(&fb_mess[i]);
		 */
	}

	for (i = 0; i < (nb - 1); i++)
	{
		for (j = i + 1; j < nb; j++)
		{
			if (t_res[j])
				continue;
			if (strcmp (fb_mess[i].bid, fb_mess[j].bid) == 0)
			{
				t_res[j] = 1;
				break;
			}
		}
	}

	if (bid_ptr)
		tst_vbid (fb_mess, nb, t_res);
	else if (EMS_BID_OK ())
		tst_exms_bid (fb_mess, nb, t_res);
	else
		tst_bid (fb_mess, nb, t_res);

	for (i = 0; i < nb; i++)
	{
		if (t_res[i])
			continue;

		if (*fb_mess[i].bid == '\0')
			continue;

		/* Teste si en cours de reception */
		for (voie = 0; voie < NBVOIES; voie++)
		{
			if ((voie == voiecur) || (!svoie[voie]->sta.connect))
				continue;

			if (strcmp (svoie[voie]->entmes.bid, fb_mess[i].bid) == 0)
			{
				t_res[i] = 2;
				break;
			}

			for (j = 0; j < MAX_FB; j++)
			{
				if (svoie[voie]->fb_mess[j].type == '\0')
					continue;
				if (strcmp (svoie[voie]->fb_mess[j].bid, fb_mess[i].bid) == 0)
				{
					t_res[i] = 2;
					break;
				}
			}
			if (t_res[i])
				break;
		}
	}

	for (i = 0; i < nb; i++)
	{
		if (t_res[i] == 0)
			t_res[i] = retenu (&fb_mess[i]);
	}


	return (t_res[0]);
}
