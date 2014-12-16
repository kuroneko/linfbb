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

/**************************************************************
				lzhuf.c
				written by Haruyasu Yoshizaki 11/20/1988
		some minor changes 4/6/1989
		comments translated by Haruhiko Okumura 4/7/1989
				adapted by Jean-Paul ROUBELAT F6FBB 10/31/90
**************************************************************/

#include <serv.h>
#include <crc.h>

static FILE *infile, *outfile;
static char *in_f, *ou_f;
static unsigned int textsize, codesize, printcount;
static char *head;
static char bin_header[257];
static short headlen;
static void write_huf_error (char *);
static short take (void);
static long numero;
static ushort crc;
static bullist *pbul;
static short basic;
static short init_huf = 0;
static int crlf = 0;

/********** LZSS compression **********/

#define N               2048	/* buffer size */
#define F               60		/* lookahead buffer size */
#define THRESHOLD       2
#define NIL             N		/* leaf of tree */

static unsigned char text_buf[N + F - 1];
static ushort chck;
static short match_position, match_length;
static short *lson, *rson, *dad;

/* Declaration de fonctions */

static short crc_fgetc (FILE *);
static ushort crc_fputc (ushort, FILE *);

static void ins_route (int, char *);

/**********************/


char *make_header (bullist * pbul, char *header)
{
	long temps;
	struct tm *sdate;
	char tempo[200];
	bullist *psauv;

	psauv = ptmes;
	ptmes = pbul;

	temps = ptmes->date;
	sdate = gmtime (&temps);

	if (std_header & 8)
		sprintf (tempo, "R:%02d%02d%02d/%02d%02dZ %%N@%%R%s",
				 sdate->tm_year % 100, sdate->tm_mon + 1, sdate->tm_mday,
				 sdate->tm_hour, sdate->tm_min, txtfwd);
	else
		sprintf (tempo, "R:%02d%02d%02d/%02d%02dZ @:%%R #:%%N%s",
				 sdate->tm_year % 100, sdate->tm_mon + 1, sdate->tm_mday,
				 sdate->tm_hour, sdate->tm_min, txtfwd);
	n_cpy (150, header, var_txt (tempo));

	if (((std_header & 4) == 0) && (strlen (header) > 79))
	{
		char *ptr;

		ptr = header + 79;
		while (*ptr != ' ')
		{
			--ptr;
		}
		*ptr = '\0';
	}
	strcat (header, "\r");

	ptmes = psauv;

	return (header);
}


void dde_huf (int voie, bullist * pbul, int mode)
{
	desc_huf pthuf;
	long taille;
	FILE *fptr;
	char fic[128];
	char temp[128];

	make_header (pbul, pthuf.header);	/* <--------- A mettre ds ENCODE */
	pthuf.next = NULL;
	pthuf.voie = voie;
	pthuf.mode = mode;
	pthuf.bull = pbul;

	deb_io ();
	taille = 0L;
	if (pthuf.mode == ENCODE)
	{
		strcpy (fic, mess_name (MBINDIR, pthuf.bull->numero, temp));
		if ((fptr = fopen (fic, "r")) != NULL)
		{
			if (fread (&taille, sizeof (long), 1, fptr) == 0)
				  taille = 0L;

			fclose (fptr);
		}
	}
	fin_io ();
	if (taille == 0L)
	{
		taille = lzhuf (&pthuf);
	}

	svoie[voie]->ask = taille;
}

static ushort crc_fputc (ushort c, FILE * outfile)
{
	crc = updcrc ((uchar)c, crc);
	chck += (c & 0xff);
	return (fputc (c, outfile));
}

static short crc_fgetc (FILE * infile)
{
	short retour = getc (infile);

	if (retour != -1)
	{
		crc = updcrc (retour, crc);
	}
	return (retour);
}

/*
 * Date en chaine de caracteres sous la forme 960225 (25fev 96)
 * Retour en secondes depuis le 1 Jan 1970
 */
long date_to_time (char *indd)
{
	static char Days[12] =
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	long x;
	short i;
	short days;
	short jour, mois, annee;

	for (i = 0; i < 6; i++)
		if (!isdigit (indd[i]))
			return (0L);

 	if (indd[0] < '8')
 		annee = ((indd[0] - '0') * 10) + 20 + (indd[1] - '0');
 	else
 		annee = ((indd[0] - '8') * 10) + (indd[1] - '0');
	mois = ((indd[2] - '0') * 10) + (indd[3] - '0');
	jour = ((indd[4] - '0') * 10) + (indd[5] - '0');

	if ((annee < 0) || (jour < 1) || (jour > 31) || (mois < 1) || (mois > 12))
	{
		/* La date est incoherente */
		x = 0L;
	}
	else
	{
		x = 315532800L + _timezone;		/* Convertit de 1980 a 1970 */
		x += (long) (annee >> 2) * 126230400L;	/* 4 annees */
		x += (long) (annee & 3) * 31536000L;	/* 1 annee */
		if (annee & 3)
			x += 86400L;
		days = 0;
		i = mois - 1;
		while (i > 0)
		{
			i--;
			days += Days[i];
		}
		days += jour - 1;
		if ((mois > 2) && ((annee & 3) == 0))
			days++;				/* bissextile */
		x += ((long) days * 86400L);
	}
	return (x);
}

/*
 * Heure sous la forme 1031 (10h31mn)
 * Retour en secondes depuis 00h00
 */

long hour_to_time (char *indd)
{
	long x;
	short i;
	short hour, minute;

	for (i = 0; i < 4; i++)
		if (!isdigit (indd[i]))
			return (0L);

	hour = ((indd[0] - '0') * 10) + (indd[1] - '0');
	minute = ((indd[2] - '0') * 10) + (indd[3] - '0');

	x = (long) hour *3600L + (long) minute *60L;

	return (x);
}

char *bbs_via (char *s)
{
	short nb = 0;
	static char bbs[7];

	while (ISGRAPH (*s) && (*s != '.'))
	{
		if (nb == 6)
			break;
		bbs[nb++] = toupper (*s);
		++s;
	}
	bbs[nb] = '\0';
	return (bbs);
}



static void ins_route (int voie, char *route)
{
	int i, alloue = 0;
	Hroute *pcurr, *pprec;

	if (svoie[voie]->r_tete == NULL)
	{
		svoie[voie]->r_tete = svoie[voie]->r_curr = (Route *) m_alloue (sizeof (Route));
		alloue = 1;
	}
	if (svoie[voie]->r_pos == NBROUTE)
	{
		svoie[voie]->r_curr->suite = (Route *) m_alloue (sizeof (Route));
		svoie[voie]->r_curr = svoie[voie]->r_curr->suite;
		alloue = 1;
	}
	if (alloue)
	{
		svoie[voie]->r_pos = 0;
		svoie[voie]->r_curr->suite = NULL;
		for (i = 0; i < NBROUTE; i++)
			*(svoie[voie]->r_curr->call[i]) = '\0';
	}
	strn_cpy (6, svoie[voie]->r_curr->call[svoie[voie]->r_pos++], bbs_via (route));


	if (h_ok)
	{
		pcurr = throute;
		pprec = NULL;

		if (pcurr)
		{
			while (pcurr)
			{
				pprec = pcurr;
				pcurr = pcurr->suiv;
			}
			pcurr = pprec->suiv = (Hroute *) m_alloue (sizeof (Hroute));
		}
		else
		{
			pcurr = throute = (Hroute *) m_alloue (sizeof (Hroute));
		}
		strn_cpy (40, pcurr->route, route);
		pcurr->suiv = NULL;
	}
}

void analyse_header (int voie, char *ptr)
{
	int c;
	int champ;
	int nb;
	int bbs;
	long date;
	char rbbs[80];
	char home[41];
	char qth[31];
	char zip[13];

	ptr += 2;
	champ = 2;
	nb = 0;
	bbs = 0;

	*home = *zip = *qth = '\0';
	date = 0L;

	svoie[voie]->mess_num = -1L;

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
				rbbs[11] = '\0';
				rbbs[6] = '\0';
				if ((date = date_to_time (rbbs)) != 0L)
				{
					date += hour_to_time (rbbs + 7);
					svoie[voie]->messdate = date;
				}
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
				ins_route (voie, rbbs);
				strn_cpy (40, home, rbbs);
				strn_cpy (40, svoie[voie]->mess_home, rbbs);
				svoie[voie]->header = 1;
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
				n_cpy (30, qth, rbbs);
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
				strn_cpy (8, zip, rbbs);
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
				if (nb)
					svoie[voie]->mess_num = atol (rbbs);
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
				if (nb)
					strn_cpy (12, svoie[voie]->mess_bid, rbbs);
				champ = 0;
			}
			break;

		}

		++ptr;
	}
	while (ISPRINT (c));

	header_wp (date, home, qth, zip);
}

void entete_mess_fwd (bullist * pbul, char *header)
{
	char s[128];
	bullist *sav_bul;

	deb_io ();
	sav_bul = ptmes;
	ptmes = pbul;
	*msg_header = '\0';

	if ((ptmes->type == 'A') || (strcmp (ptmes->desti, "WP") == 0))
	{
		if (*(ptmes->bbsf) == '\0')
		{
			strcpy (msg_header, header);
			strcat (msg_header, "\r");
		}
	}
	else if (*(ptmes->bbsf))
	{
		strcpy (msg_header, header);
	}
	else
	{
		strcpy (msg_header, header);
		sprintf (s, "\rFrom: %s@%s\rTo  : %s@%s\r\r",
				 ptmes->exped, mypath, ptmes->desti, ptmes->bbsv);
		strcat (msg_header, s);
	}

	ptmes = sav_bul;
	fin_io ();
}


static void write_huf_error (char *filename)
{
	deb_io ();
	fclose (infile);
	fclose (outfile);
	unlink (ou_f);
	write_error (filename);
	fin_io ();
}


static void InitTree (void)		/* initialize trees */
{
	int i;

	for (i = N + 1; i <= N + 256; i++)
		rson[i] = NIL;			/* root */
	for (i = 0; i < N; i++)
		dad[i] = NIL;			/* node */
}


static void InsertNode (short r)	/* insert to tree */
{
	short i, p, cmp;
	unsigned char *key;
	ushort c;

	cmp = 1;
	key = &text_buf[r];
	p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;
	match_length = 0;
	for (;;)
	{
		if (cmp >= 0)
		{
			if (rson[p] != NIL)
				p = rson[p];
			else
			{
				rson[p] = r;
				dad[r] = p;
				return;
			}
		}
		else
		{
			if (lson[p] != NIL)
				p = lson[p];
			else
			{
				lson[p] = r;
				dad[r] = p;
				return;
			}
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)
				break;
		if (i > THRESHOLD)
		{
			if (i > match_length)
			{
				match_position = ((r - p) & (N - 1)) - 1;
				if ((match_length = i) >= F)
					break;
			}
			else if (i == match_length)
			{
				if ((c = ((r - p) & (N - 1)) - 1) < match_position)
				{
					match_position = c;
				}
			}
		}
	}
	dad[r] = dad[p];
	lson[r] = lson[p];
	rson[r] = rson[p];
	dad[lson[p]] = r;
	dad[rson[p]] = r;
	if (rson[dad[p]] == p)
		rson[dad[p]] = r;
	else
		lson[dad[p]] = r;
	dad[p] = NIL;				/* remove p */
}


static void DeleteNode (short p)	/* remove from tree */
{
	short q;

	if (dad[p] == NIL)
		return;					/* not registered */
	if (rson[p] == NIL)
		q = lson[p];
	else if (lson[p] == NIL)
		q = rson[p];
	else
	{
		q = lson[p];
		if (rson[q] != NIL)
		{
			do
			{
				q = rson[q];
			}
			while (rson[q] != NIL);
			rson[dad[q]] = lson[q];
			dad[lson[q]] = dad[q];
			lson[q] = lson[p];
			dad[lson[p]] = q;
		}
		rson[q] = rson[p];
		dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p)
		rson[dad[p]] = q;
	else
		lson[dad[p]] = q;
	dad[p] = NIL;
}


/* Huffman coding */

#define N_CHAR   (256 - THRESHOLD + F)
/* kinds of characters (character code = 0..N_CHAR-1) */
#define T        (N_CHAR * 2 - 1)	/* size of table */
#define R        (T - 1)		/* position of root */
#define MAX_FREQ 0x8000			/* updates tree when the */
/* root frequency comes to this value. */

/* table for encoding and decoding the upper 6 bits of position */

/* for encoding */
static uchar p_len[64] =
{
	0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};


static uchar p_code[64] =
{
	0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
	0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
	0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
	0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
	0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
	0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};


/* for decoding */
static uchar d_code[256] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
	0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
	0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
	0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
	0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
	0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
	0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
	0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
	0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
	0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
	0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
	0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B,
	0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};


static uchar d_len[256] =
{
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};


static ushort *freq;			/* frequency table */

/*
   pointers to parent nodes, except for the
   elements [T..T + N_CHAR - 1] which are used to get
   the positions of leaves corresponding to the codes.
 */

static short *prnt;

static short *fils;				/* pointers to child nodes (fils[], fils[] + 1) */

static ushort getbuf;
static uchar getlen;

static short scan_header (char *chaine)
{
	char *ptr = bin_header;

	while ((*ptr++ = *chaine) != '\0')
	{
		if (*chaine == '\r')
			*ptr++ = '\n';
		++chaine;
	}
	return (strlen (bin_header));
}


static short take (void)
{
	short c;

	if (headlen)
	{
		--headlen;
		c = ((short) *head++ & 0xff);
	}
	else
	{
		/* Gestion du LF -> CRLF */

		if (crlf)
		{
			c = '\n';
			crlf = 0;
		}
		else
		{
			do
			{
				/* Ignore les CR */
				c = getc (infile);
			}
			while (c == '\r');

			if (c == '\n')
			{
				crlf = c;
				c = '\r';
			}
		}

	}
	return (c);
}


static short GetBit (void)		/* get one bit */
{
	short i;

	while (getlen <= 8)
	{
		if ((i = crc_fgetc (infile)) < 0)
			i = 0;
		getbuf |= i << (8 - getlen);
		getlen += 8;
	}
	i = getbuf;
	getbuf <<= 1;
	getlen--;
	return (i < 0);
}


static short GetByte (void)		/* get one byte */
{
	ushort i;

	while (getlen <= 8)
	{
		if ((i = crc_fgetc (infile)) == 0xffff)
			i = 0;
		getbuf |= i << (8 - getlen);
		getlen += 8;
	}
	i = getbuf;
	getbuf <<= 8;
	getlen -= 8;
	return i >> 8;
}


static ushort putbuf;
static uchar putlen;

static void Putcode (short l, ushort c)		/* output c bits of code */
{
	putbuf |= c >> putlen;
	if ((putlen += l) >= 8)
	{
		if (crc_fputc (putbuf >> 8, outfile) == EOF)
		{
			write_huf_error (ou_f);
		}
		if ((putlen -= 8) >= 8)
		{
			if (crc_fputc (putbuf, outfile) == EOF)
			{
				write_huf_error (ou_f);
			}
			codesize += 2;
			putlen -= 8;
			putbuf = c << (l - putlen);
		}
		else
		{
			putbuf <<= 8;
			codesize++;
		}
	}
}


/* initialization of tree */

static void StartHuff (void)
{
	short i, j;

	for (i = 0; i < N_CHAR; i++)
	{
		freq[i] = 1;
		fils[i] = i + T;
		prnt[i + T] = i;
	}
	i = 0;
	j = N_CHAR;
	while (j <= R)
	{
		freq[j] = freq[i] + freq[i + 1];
		fils[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2;
		j++;
	}
	freq[T] = 0xffff;
	prnt[R] = 0;
}


/* reconstruction of tree */

static void reconst (void)
{
	short i, j, k;
	ushort f, l;

	/* collect leaf nodes in the first half of the table */
	/* and replace the freq by (freq + 1) / 2. */
	j = 0;
	for (i = 0; i < T; i++)
	{
		if (fils[i] >= T)
		{
			freq[j] = (freq[i] + 1) / 2;
			fils[j] = fils[i];
			j++;
		}
	}
	/* begin constructing tree by connecting sons */
	for (i = 0, j = N_CHAR; j < T; i += 2, j++)
	{
		k = i + 1;
		f = freq[j] = freq[i] + freq[k];
		for (k = j - 1; f < freq[k]; k--)
			;
		k++;
		l = (j - k) * 2;
		memmove (&freq[k + 1], &freq[k], l);
		freq[k] = f;
		memmove (&fils[k + 1], &fils[k], l);
		fils[k] = i;
	}
	/* connect prnt */
	for (i = 0; i < T; i++)
	{
		if ((k = fils[i]) >= T)
		{
			prnt[k] = i;
		}
		else
		{
			prnt[k] = prnt[k + 1] = i;
		}
	}
}


/* increment frequency of given code by one, and update tree */

static void update (short c)
{
	short i, j, k, l;

	if (freq[R] == MAX_FREQ)
	{
		reconst ();
	}
	c = prnt[c + T];
	do
	{
		k = ++freq[c];

		/* if the order is disturbed, exchange nodes */
		if (k > freq[l = c + 1])
		{
			while (k > freq[++l])
				;
			l--;
			freq[c] = freq[l];
			freq[l] = k;

			i = fils[c];
			prnt[i] = l;
			if (i < T)
				prnt[i + 1] = l;

			j = fils[l];
			fils[l] = i;

			prnt[j] = c;
			if (j < T)
				prnt[j + 1] = c;
			fils[c] = j;

			c = l;
		}
	}
	while ((c = prnt[c]) != 0);	/* repeat up to root */
}


static ushort code, len;

static void EncodeChar (ushort c)
{
	ushort i;
	short j, k;

	i = 0;
	j = 0;
	k = prnt[c + T];

	/* travel from leaf to root */
	do
	{
		i >>= 1;

		/* if node's address is odd-numbered, choose bigger brother node */
		if (k & 1)
			i += 0x8000;

		j++;
	}
	while ((k = prnt[k]) != R);
	Putcode (j, i);
	code = i;
	len = j;
	update (c);
}


static void EncodePosition (ushort c)
{
	ushort i;

	/* output upper 6 bits by table lookup */
	i = c >> 6;
	Putcode (p_len[i], (ushort) p_code[i] << 8);

	/* output lower 6 bits verbatim */
	Putcode (6, (c & 0x3f) << 10);
}


static void EncodeEnd (void)
{
	if (putlen)
	{
		if (crc_fputc (putbuf >> 8, outfile) == EOF)
		{
			write_huf_error (ou_f);
		}
		codesize++;
	}
}


static short DecodeChar (void)
{
	ushort c;

	c = fils[R];

	/* travel from root to leaf, */
	/* choosing the smaller child node (fils[]) if the read bit is 0, */
	/* the bigger (fils[]+1} if 1 */
	while (c < T)
	{
		c += GetBit ();
		c = fils[c];
	}
	c -= T;
	update (c);
	return c;
}


static short DecodePosition (void)
{
	ushort i, j, c;

	/* recover upper 6 bits from table */
	i = GetByte ();
	c = (ushort) d_code[i] << 6;
	j = d_len[i];

	/* read lower 6 bits verbatim */
	j -= 2;
	while (j--)
	{
		i = (i << 1) + GetBit ();
	}
	return c | (i & 0x3f);
}


/* compression */

static void Encode (short voie)	/* compression */
{
	char temp[128];
	unsigned int filesize;
	short i, c, len, r, s, last_match_length;

	head = bin_header;
	fflush (infile);
#if defined(__MSDOS__) || defined(__WINDOWS__)
	filesize = filelength (fileno (infile)) + (long) headlen;
#else
	filesize = headlen;
	/* Read the file to compute the filesize under unix. Terminations = CRLF */
	while ((c = getc (infile)) != EOF)
	{
		/* Ignore '\r' */
		if (c == '\r')
			continue;

		/* LF -> CRLF */
		if (c == '\n')
			++filesize;

		++filesize;
	}
	rewind (infile);
#endif

	textsize = 0;				/* rewind and re-read */
	crc = 0;

	/* CRC and textsize are 0 : no need to swap them for motorola */
	
	if (basic)
	{
		if (fwrite (&textsize, sizeof textsize, 1, outfile) < 1)
			write_huf_error (ou_f);		/* output size of text */
	}
	else
	{
		if (fwrite (&crc, sizeof (crc), 1, outfile) < 1)
			write_huf_error (ou_f);		/* output size of text */

		memcpy (temp, &filesize, sizeof (filesize));
		if (moto)
		{
			for (i = sizeof (filesize) -1 ; i >= 0 ; i-- )
				crc_fputc (temp[i], outfile);
		}
		else
		{
			for (i = 0; i < sizeof (filesize); i++)
				crc_fputc (temp[i], outfile);
		}
	}

	StartHuff ();
	InitTree ();
	s = 0;
	r = N - F;
	for (i = s; i < r; i++)
		text_buf[i] = ' ';
	for (len = 0; len < F && (c = take ()) != EOF; len++)
		text_buf[r + len] = c;
	textsize = len;
	for (i = 1; i <= F; i++)
		InsertNode (r - i);
	InsertNode (r);
	do
	{
		if (match_length > len)
			match_length = len;
		if (match_length <= THRESHOLD)
		{
			match_length = 1;
			EncodeChar (text_buf[r]);
		}
		else
		{
			EncodeChar (255 - THRESHOLD + match_length);
			EncodePosition (match_position);
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length && (c = take ()) != EOF; i++)
		{
			DeleteNode (s);
			text_buf[s] = c;
			if (s < F - 1)
				text_buf[s + N] = c;
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			InsertNode (r);
		}
		if ((textsize += i) > printcount)
		{
			compress_display (1, (textsize * 100) / filesize, numero);
			printcount += 500;
		}
		while (i++ < last_match_length)
		{
			DeleteNode (s);
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			if (--len)
				InsertNode (r);
		}
	}
	while (len > 0);
	EncodeEnd ();


	compress_display (1, 100, numero);

	/* Ecrit le crc du fichier original */
	deb_io ();

	rewind (outfile);
	if (basic)
	{
		memcpy (temp, &textsize, sizeof (textsize));
		if (moto)
		{
			for (i = sizeof (textsize) -1 ; i >= 0 ; i-- )
				crc_fputc (temp[i], outfile);
		}
		else
		{
			for (i = 0; i < sizeof (textsize); i++)
				crc_fputc (temp[i], outfile);
		}
	}
	else
	{
		if (moto)
			crc = xendien (crc);
		if (fwrite (&crc, sizeof (crc), 1, outfile) < 1)
			write_huf_error (ou_f);		/* output crc */
	}

	if (filesize != textsize)
	{
		cprintf ("Phase error !\r\n");
	}

	aff_header (voie);
	if (numero)
		sprintf (temp, "Compress #%ld In: %-6d- Out: %-6d- Compress: %d %%\r\n",
		   numero, textsize, codesize, 100 - ((codesize * 100) / textsize));
	else
		sprintf (temp, "Compress XFwd In: %-6d- Out: %-6d- Compress: %d %%\r\n",
				 textsize, codesize, 100 - ((codesize * 100) / textsize));
	winputs (voie, W_SNDT, temp);
	textsize = codesize + 4;
	fin_io ();
}


void check_bin (bullist * pbul, char *ptr)
{
#define NB_PATTERN 8
#define LG_PATTERN 6

	static char pattern[NB_PATTERN][LG_PATTERN + 1] =
	{
		" go_7+",
		" go_te",
		"\000\000\000\000\000\000",
		"\000\000\000\000\000\000",
		"\000\000\000\000\000\000",
		"\000\000\000\000\000\000",
		"\000\000\000\000\000\000",
		"\000\000\000\000\000\000"
	};

	short i;

	/* Checks for data message and validates bin flag */

	for (i = 0; i < NB_PATTERN; i++)
	{
		if (!(*pattern[i]))
			break;
		if (strncmp (pattern[i], ptr, LG_PATTERN) == 0)
		{
			pbul->bin = 1;
		}
	}
}

static void test_ligne (int voie, char *ligne)
{
	char deb[5];

	strn_cpy (4, deb, ligne);

	if ((svoie[voie]->m_ack) && (strncmp (deb, "/ACK", 4) == 0))
	{
		svoie[voie]->m_ack = 2;
	}
	else if ((svoie[voie]->entete) && (strncmp (deb, "R:", 2) == 0))
	{
		analyse_header (voie, ligne);
	}
	else if (*ligne)
	{
		check_bin (pbul, ligne);
		svoie[voie]->entete = 0;
	}
}


static int Decode (int voie)	/* recover */
{
	char temp[128];
	char ligne[82];
	short i, j, k, r, c;
	short pos = 0;
	ushort fcrc;
	unsigned long int count;

	deb_io ();

	fseek (infile, 0L, 2);
	codesize = ftell (infile) - sizeof (textsize);

	if (!basic)
	{
		fseek (infile, 20L, 0);

		textsize = -1;
		crc = 0;
		if ((svoie[voie]->fbb >= 2) && (fread (&fcrc, sizeof fcrc, 1, infile) < 1))
		{
			if (!svoie[CONSOLE]->sta.connect)
				cprintf ("File empty (%d)!!\r\n", textsize);
			fin_io ();
			return (0);
		}

		if (moto)
			fcrc = xendien (fcrc);
	
		memset (temp, 0, sizeof (textsize));
		if (moto)
		{
			for (i = sizeof (textsize) -1 ; i >= 0 ; i-- )
				temp[i] = crc_fgetc (infile);
		}
		else
		{
			for (i = 0; i < sizeof (textsize); i++)
				temp[i] = crc_fgetc (infile);
		}
		
		memcpy (&textsize, temp, sizeof (textsize));

		if ((textsize == 0) || (textsize == 0xffffffff))
		{
			if (!svoie[CONSOLE]->sta.connect)
				cprintf ("File empty (%d)!!\r\n", textsize);
			fin_io ();
			return (0);
		}

		if (textsize > (100 * codesize))
		{
			if (!svoie[CONSOLE]->sta.connect)
				cprintf ("xFBBd Decode basic:%d Cannot decompress to %d bytes!!\r\n", basic, textsize);
				cprintf ("codesize :%d textsize : %d sizeof %u\r\n", codesize, textsize, (unsigned int)sizeof(textsize));
			fin_io ();
			return (0);
		}

	}
	else
	{
		rewind (infile);
		if ((fread (&textsize, sizeof textsize, 1, infile) < 1) || (textsize == 0))
		{
			if (!svoie[CONSOLE]->sta.connect)
				cprintf ("File empty (%d)!!\r\n", textsize);
			fin_io ();
			return (0);
		}

		if (moto)
			textsize = xendienl (textsize);
	
		if (textsize > (100 * codesize))
		{
			if (!svoie[CONSOLE]->sta.connect)
				cprintf ("xFBBd Decode basic:%d Cannot decompress to %d bytes!!\r\n", basic, textsize);
				cprintf ("codesize :%d textsize : %d sizeof %u\r\n", codesize, textsize, (unsigned int)sizeof(textsize));
			fin_io ();
			return (0);
		}
	}

	fin_io ();

	StartHuff ();
	for (i = 0; i < N - F; i++)
		text_buf[i] = ' ';
	r = N - F;
	for (count = 0; count < textsize;)
	{
		c = DecodeChar ();
		if (c < 256)
		{
			if (!basic)
			{
				if ((c == '\r') || (c == '\n'))
				{
					ligne[pos] = '\0';
					test_ligne (voie, ligne);
					pos = 0;
				}
				else if (pos < 80)
				{
					ligne[pos++] = c;
				}
			}
			if ((c == '\n') || (c == '\r'))
			{
				if ((crlf == 0) || (crlf == c))
				{
					crlf = c;
					if (fputc ('\r', outfile) == EOF)
					{
						write_huf_error (ou_f);
					}
					if (fputc ('\n', outfile) == EOF)
					{
						write_huf_error (ou_f);
					}
				}
			}
			else
			{
				crlf = 0;
				if (fputc (c, outfile) == EOF)
				{
					write_huf_error (ou_f);
				}
			}
			text_buf[r++] = c;
			r &= (N - 1);
			count++;
		}
		else
		{
			i = (r - DecodePosition () - 1) & (N - 1);
			j = c - 255 + THRESHOLD;
			for (k = 0; k < j; k++)
			{
				c = text_buf[(i + k) & (N - 1)];
				if (!basic)
				{
					if ((c == '\r') || (c == '\n'))
					{
						ligne[pos] = '\0';
						test_ligne (voie, ligne);
						pos = 0;
					}
					else if (pos < 80)
					{
						ligne[pos++] = c;
					}
				}
				/*
				   if (fputc (c, outfile) == EOF)
				   {
				   write_huf_error (ou_f);
				   }
				 */
				if ((c == '\n') || (c == '\r'))
				{
					if ((crlf == 0) || (crlf == c))
					{
						crlf = c;
						if (fputc ('\r', outfile) == EOF)
						{
							write_huf_error (ou_f);
						}
						if (fputc ('\n', outfile) == EOF)
						{
							write_huf_error (ou_f);
						}
					}
				}
				else
				{
					crlf = 0;
					if (fputc (c, outfile) == EOF)
					{
						write_huf_error (ou_f);
					}
				}
				text_buf[r++] = c;
				r &= (N - 1);
				count++;
			}
		}
		if (count > printcount)
		{
			compress_display (2, (count * 100) / textsize, numero);
			printcount += 500;
		}
	}
	compress_display (2, 100, numero);
	deb_io ();
	aff_header (voie);
	if (numero)
		sprintf (temp, "Decompress #%ld In: %-6d- Out: %-6d- Compress: %d %%\r\n",
		   numero, codesize, textsize, 100 - ((codesize * 100) / textsize));
	else
		sprintf (temp, "Decompress XFwd In: %-6d- Out: %-6d- Compress: %d %%\r\n",
				 codesize, textsize, 100 - ((codesize * 100) / textsize));
	winputs (voie, W_RCVT, temp);

	if ((!basic) && (svoie[voie]->fbb >= 2) && (crc != fcrc))
	{
		if (!svoie[CONSOLE]->sta.connect)
			cprintf ("CRC Error : file %06x, computed %06x\r\n", fcrc, crc);
		fin_io ();
		return (0);
	}
	fin_io ();
	return (1);
}


void end_lzhuf (void)
{
	if (init_huf)
	{
		m_libere (lson, sizeof (short) * (N + 1));
		m_libere (rson, sizeof (short) * (N + 257));
		m_libere (dad, sizeof (short) * (N + 1));
		m_libere (prnt, sizeof (short) * (T + N_CHAR));
		m_libere (fils, sizeof (short) * T);

		m_libere (freq, sizeof (ushort) * (T + 1));
		init_huf = 0;
	}
}


static void alloue_lzhuf_buffers (void)
{
	lson = (short *) m_alloue (sizeof (short) * (N + 1));
	rson = (short *) m_alloue (sizeof (short) * (N + 257));
	dad = (short *) m_alloue (sizeof (short) * (N + 1));
	prnt = (short *) m_alloue (sizeof (short) * (T + N_CHAR));
	fils = (short *) m_alloue (sizeof (short) * T);

	freq = (ushort *) m_alloue (sizeof (ushort) * (T + 1));

	init_huf = 1;
}


static void init_huffman (void)
{
	static int init = 0;

	if (!init)
	{
		alloue_lzhuf_buffers ();
		init = 1;
	}
}

long lzhuf (desc_huf * huf)
{
	char bin_file[128];
	char tmp_file[128];
	char asc_file[128];

	init_huffman ();

	basic = 0;
	crlf = 0;

	numero = huf->bull->numero;

	pbul = huf->bull;

	mess_name (MBINDIR, numero, bin_file);
	mess_name (MESSDIR, numero, asc_file);

	if (huf->mode == ENCODE)
	{
		temp_name (huf->voie, tmp_file);
		in_f = asc_file;
		ou_f = tmp_file;
		deb_io ();
		entete_mess_fwd (pbul, huf->header);
		headlen = scan_header (msg_header);
		fin_io ();
	}
	else if (huf->mode == DECODE)
	{
		strcpy (tmp_file, svoie[huf->voie]->sr_fic);
		in_f = tmp_file;
		ou_f = asc_file;
	}

	if ((outfile = fopen (ou_f, "wb")) == NULL)
		write_error (ou_f);

	if ((infile = fopen (in_f, "rb")) == NULL)
	{
		fprintf (outfile, "\r\nMessage file %s missing in %s\r\n", in_f, mycall);
		fclose (outfile);
		return (40L);
	}

	textsize = 0;				/* text size counter */
	codesize = 0;				/* code size counter */
	printcount = 0;				/* counter for reporting progress every 1K bytes */
	getbuf = getlen = putbuf = putlen = 0;

	if (huf->mode == ENCODE)
	{
		Encode (huf->voie);
	}
	else if (huf->mode == DECODE)
	{
		if (!Decode (huf->voie))
			textsize = -1L;
	}

	fclose (infile);
	fclose (outfile);

	deb_io ();
	if (huf->mode == ENCODE)
	{
		rename_temp (huf->voie, bin_file);
	}

	tot_mem = 0;
	free_mem ();
	compress_display (0, 0, numero);

	fin_io ();

	return (textsize);
}

long basic_lzhuf (int mode, char *in_file, char *out_file)
{
	chck = 0;
	basic = 1;
	crlf = 0;

	init_huffman ();

	in_f = in_file;
	ou_f = out_file;

	if ((infile = fopen (in_f, "rb")) == NULL)
		return 0L;

	if ((outfile = fopen (ou_f, "wb")) == NULL)
		write_error (ou_f);

	numero = 0;
	textsize = 0;				/* text size counter */
	codesize = 0;				/* code size counter */
	printcount = 0;				/* counter for reporting progress every 1K bytes */
	getbuf = getlen = putbuf = putlen = 0;

	if (mode == ENCODE)
	{
		Encode (voiecur);
	}
	else if (mode == DECODE)
	{
		if (!Decode (voiecur))
			textsize = -1L;
	}

	fclose (infile);
	fclose (outfile);

	tot_mem = 0;
	free_mem ();
	compress_display (0, 0, 0);

	svoie[voiecur]->checksum = chck;
	return (textsize);
}
