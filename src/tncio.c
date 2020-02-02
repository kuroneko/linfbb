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
 *  MODULE TNCIO.C : ENTREES-SORTIE
 */

#include <serv.h>
#include <stdint.h>

#define INT10 0x10

static void clear_marque (int);
static void aff_bas_suite (int, int, char *, int);

#ifndef dump_core
static void dump_data (char *title, FILE * fptr, char *ptr, int len)
{
	int i;
	int j;

	fprintf (fptr, "\r\nDump of %s\r\n", title);
	for (i = 0; i < len; i += 16)
	{
		fprintf (fptr, "%05d  ", i);
		for (j = 0; j < 16; j++)
		{
			if ((i + j) < len)
				fprintf (fptr, "%02x ", ptr[i + j] & 0xff);
			else
				fprintf (fptr, "   ");
		}
		for (j = 0; j < 16; j++)
		{
			if ((i + j) < len)
				fprintf (fptr, "%c", (ptr[i + j] >= 0x20) ? ptr[i + j] : '.');
		}
		fprintf (fptr, "\r\n");
	}
}

void dump_core (void)
{
	time_t temps = time (NULL);
	struct tm *tm = localtime (&temps);
	FILE *fptr = fopen ("debug.bug", "ab");

	if (!fptr)
		return;

	fprintf (fptr, "\r\n\r\nOn %02d/%02d %02d:%02d:%02d\r\n\r\n", 
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	fprintf (fptr, "Channel %d\r\n\r\n", voiecur - 1);

	wreq_cfg (fptr);

	dump_data ("svoie", fptr, (char *) svoie[voiecur], sizeof (Svoie));

	fclose (fptr);

/*  *((char *)NULL) = 0; */
	exit (999);
}
#endif

#ifdef __WINDOWS__
void BWinSleep (int temps)
{
	long cur;
	long fin;
	long ncur;

	temps /= 50;

	if (temps == 0)
		temps = 1;

	cur = btime ();
	fin = cur + (long) temps;

	for (;;)
	{
		deb_io ();
		ncur = btime ();
		if (ncur > fin)
			break;
		fin_io ();
	}
}
#endif

int aff_ok(int voie)
{
	if (svoie[voie]->binary)
		return 0;
	if (voie == CONSOLE)
		return 0;
	if ((voie == INEXPORT) && !aff_inexport)
		return 0;
	if (POP(no_port(voie)) && !aff_popsmtp)
		return 0;
		
	return 1;
}

static void outbuf (int mod, char *si, int nb)
{
	obuf *optr = pvoie->outptr;
	char *ptr, *sv, *debut;
	int nbout, c, ncars, var = 0, fin_buf = 0, nbbuf = 0;

	/*
	   * mod = 0 : sans variables
	   * mod = 1 : avec variables
	   * mod = 2 : sans variables + cr
	   * mod = 3 : avec variables + cr
	 */

	df ("outbuf", 4);

	if (optr)
	{
		while (optr->suiv)
		{
			optr = optr->suiv;
			++nbbuf;
		}
	}
	else
	{
		int i;

		pvoie->memoc += 250;
		optr = (obuf *) m_alloue (sizeof (obuf));
		pvoie->outptr = optr;
		optr->nb_car = optr->no_car = 0;
		for (i = 0; i < NB_MARQUES; i++)
			optr->marque[i] = -1;
		optr->suiv = NULL;
	}
	ncars = optr->nb_car;
	ptr = optr->buffer + ncars;
	nbout = 0;
	debut = ptr;

	for (;;)
	{
		if (nb == 0)
		{
			if (mod & 2)
			{
				c = '\r';
				mod = 0;
			}
			else
				break;
		}
		else
		{
			nb--;
			c = *si++;
		}
		if ((c == '\r') && (!pvoie->binary))
			pvoie->ret = TRUE;
		else if ((c == '\n') && (!pvoie->binary))
		{
			if (!pvoie->ret)
				c = '\r';
			else
				continue;
		}
		else
			pvoie->ret = FALSE;
		if ((var == 1) && (c == 'W'))
		{
			c = '\r';
			var = 0;
		}
		if (var == 1)
		{
			var = 0;
			sv = variable (c);
			while (*sv)
			{
				if (ncars == 250)
				{
					int i;

					pvoie->memoc += 250;
					if ((nbout) && aff_ok(voiecur))
					{
						deb_io ();
						aff_bas (voiecur, W_SNDT, debut, nbout);
						fin_io ();
					}
					optr->nb_car = ncars;
					optr->suiv = (obuf *) m_alloue (sizeof (obuf));
					optr = optr->suiv;
					optr->nb_car = optr->no_car = ncars = 0;
					for (i = 0; i < NB_MARQUES; i++)
						optr->marque[i] = -1;
					optr->suiv = NULL;
					ptr = optr->buffer;
					nbout = 0;
					debut = ptr;
					++nbbuf;
				}
				++ncars;
				++nbout;
				*ptr++ = *sv++;
			}
			if (fin_buf)
				ncars = 250;
		}
		else if (var == 2)
		{
			var = 0;
			sv = alt_variable (c);
			while (*sv)
			{
				if (ncars == 250)
				{
					int i;

					pvoie->memoc += 250;
					if ((nbout) && aff_ok(voiecur))
					{
						deb_io ();
						aff_bas (voiecur, W_SNDT, debut, nbout);
						fin_io ();
					}
					optr->nb_car = ncars;
					optr->suiv = (obuf *) m_alloue (sizeof (obuf));
					optr = optr->suiv;
					optr->nb_car = optr->no_car = ncars = 0;
					for (i = 0; i < NB_MARQUES; i++)
						optr->marque[i] = -1;
					optr->suiv = NULL;
					ptr = optr->buffer;
					nbout = 0;
					debut = ptr;
					++nbbuf;
				}
				++ncars;
				++nbout;
				*ptr++ = *sv++;
			}
			if (fin_buf)
				ncars = 250;
		}
		else
		{
			if ((c == '$') && (mod & 1))
				var = 1;
			else if ((c == '%') && (mod & 1))
				var = 2;
			else
			{
				if (ncars == 250)
				{
					int i;

					pvoie->memoc += 250;
					if ((nbout) && aff_ok(voiecur))
					{
						deb_io ();
						aff_bas (voiecur, W_SNDT, debut, nbout);
						fin_io ();
					}
					optr->nb_car = ncars;
					optr->suiv = (obuf *) m_alloue (sizeof (obuf));
					optr = optr->suiv;
					optr->nb_car = optr->no_car = ncars = 0;
					for (i = 0; i < NB_MARQUES; i++)
						optr->marque[i] = -1;
					optr->suiv = NULL;
					ptr = optr->buffer;
					nbout = 0;
					debut = ptr;
					++nbbuf;
				}
				++ncars;
				++nbout;
				*ptr++ = c;
			}
		}
	}
	optr->nb_car = ncars;

	if (pvoie->dde_int != 2)
	{
		if ((ncars) && aff_ok(voiecur))
		{
			deb_io ();
			aff_bas (voiecur, W_SNDT, debut, nbout);
			fin_io ();
		}
	}
	ff ();
	return;
}


void cr (void)
{
	df ("cr", 0);

	out ("$W", 2);

	ff ();
	return;
}

void outs (char *si, int nb)
{
	df ("outs", 3);

	outbuf (0, si, nb);

	ff ();
	return;
}

void out (char *si, int nb)
{
	df ("out", 3);

	outbuf (1, si, nb);

	ff ();
	return;
}

void outsln (char *si, int nb)
{
	df ("outsln", 3);

	outbuf (2, si, nb);

	ff ();
	return;
}

void outln (char *si, int nb)
{
	df ("outln", 3);

	outbuf (3, si, nb);

	ff ();
	return;
}

void marque_obuf (void)
{
	int i;

	obuf *optr = pvoie->outptr;

	df ("marque_obuf", 0);

	if (optr)
	{
		while (optr->suiv)
		{
			optr = optr->suiv;
		}
		for (i = 0; i < NB_MARQUES; i++)
		{
			if (optr->marque[i] == -1)
				optr->marque[i] = optr->nb_car;
		}
	}
	ff ();
	return;
}

int get_inbuf (int voie)
{
	ibuf *bufptr = &(svoie[voie]->inbuf);
	lbuf *ligptr;

	if ((bufptr->curr == NULL) || (bufptr->nbcar == 0))
		return (0);				/* pas de buffer !! */

	if (bufptr->curr->lgbuf)
	{
		char *ptr = bufptr->curr->buffer;

		--bufptr->curr->lgbuf;
		--bufptr->nbcar;

		if (bufptr->curr->lgbuf == 0)
		{
			/* libere le buffer */
			if (bufptr->tete == bufptr->curr)
			{
				/* C'est le premier buffer */
				m_libere ((char *) bufptr->curr->buffer, 1);
				m_libere ((char *) bufptr->curr, sizeof (lbuf));
				bufptr->tete = bufptr->curr = NULL;
			}
			else
			{
				/* Cherche l'avant-dernier buffer ... */
				ligptr = bufptr->tete;
				while (ligptr->suite != bufptr->curr)
				{
					ligptr = ligptr->suite;
					if (ligptr == NULL)
						return (0);		/* Okazou ! */
				}

				/* Ok... */
				m_libere ((char *) bufptr->curr->buffer, 1);
				m_libere ((char *) bufptr->curr, sizeof (lbuf));
				bufptr->curr = ligptr;
				bufptr->curr->suite = NULL;
			}
		}
		else
		{
			/* Realloue le buffer */
			bufptr->curr->buffer = (char *) m_alloue (bufptr->curr->lgbuf);
			memcpy (bufptr->curr->buffer, ptr, bufptr->curr->lgbuf);
			m_libere (ptr, bufptr->curr->lgbuf + 1);
		}
	}
	return (1);
}

void in_buf (int voie, char *buffer, int lgbuf)
{
	ibuf *bufptr = &(svoie[voie]->inbuf);
	lbuf *ligptr;
	char *ptr;
	int car;

	df ("in_buf", 4);

	if (lgbuf == 0)
		return;

	deb_io ();

	ligptr = (lbuf *) m_alloue (sizeof (lbuf));
	ligptr->suite = NULL;
	if (bufptr->tete == NULL)
	{
		bufptr->tete = bufptr->curr = ligptr;
		bufptr->nocar = bufptr->nbcar = bufptr->nblig = 0;
	}
	else
	{
		bufptr->curr->suite = ligptr;
		bufptr->curr = ligptr;
	}
	ptr = bufptr->curr->buffer = (char *) m_alloue (lgbuf);

	bufptr->curr->lgbuf = lgbuf;
	bufptr->nbcar += lgbuf;

	while (lgbuf--)
	{
		car = *buffer++;
		*ptr++ = car;
		if (car == '\r')
		{
			if ((svoie[voie]->binary == 0) &&
				(svoie[voie]->kiss == -1) &&
				(svoie[voie]->cross_connect == -1) &&
				(svoie[voie]->seq || svoie[voie]->outptr) &&
				(bufptr->curr->lgbuf == 2))
			{
				if ((*(bufptr->curr->buffer) == 'A') ||
					(*(bufptr->curr->buffer) == 'a'))
				{
					svoie[voie]->dde_int = 1;
					clear_inbuf (voie);
					break;
				}
				if ((*(bufptr->curr->buffer) == 'C') ||
					(*(bufptr->curr->buffer) == 'c'))
				{					
					if (svoie[voie]->aut_nc)
						svoie[voie]->dde_int = 3;
					clear_inbuf (voie);
					break;
				}
				if ((*(bufptr->curr->buffer) == 'N') ||
					(*(bufptr->curr->buffer) == 'n'))
				{
					if (svoie[voie]->aut_nc)
						svoie[voie]->dde_int = 2;
				}
			}
			bufptr->nblig++;
		}
	}

	fin_io ();

	ff ();
	return;
}


void interruption (int voie)
{
#ifdef __FBBDOS__
	FScreen *screen = &conbuf;
#endif

	df ("interruption", 1);

	switch (svoie[voie]->dde_int)
	{
	case 1:					/* Abort */
		clear_outbuf (voie);
		libere_tread (voie);
		svoie[voie]->seq = svoie[voie]->sta.ack = svoie[voie]->stop = 0;
		svoie[voie]->sr_mem = svoie[voie]->dde_int = 0;
		svoie[voie]->lignes = -1;
		selvoie (voie);
		init_langue (voie);
		aff_header (voie);
#ifdef __FBBDOS__
		if ((voie == CONSOLE) && (screen->totlig))
			inputs (CONSOLE, W_CNST, "A\r");
#endif
		texte (T_QST + 3);
		if ((pvoie->mbl) && (pvoie->niv2 == 1))
		{
			/* Liste ! */
			texte (T_QST + 6);
			ch_niv3 (2);
		}
		else if (pvoie->niv1 == N_THEMES)
		{
			/* Themes */
			maj_niv (N_THEMES, 0, 0);
			texte (T_THE + 2);
		}
		else
			retour_menu (svoie[voie]->niv1);
		break;

	case 2:					/* Next */
		/* svoie[voie]->dde_int = 0; */
		svoie[voie]->stop = 0;
		clear_marque (voie);
		svoie[voie]->dde_int = 0;
		break;

	case 3:					/* Continu */
		/* svoie[voie]->dde_int = 0; */
		svoie[voie]->dde_int = svoie[voie]->stop = 0;
		svoie[voie]->lignes = -1;
		break;

	default:
		retour_menu (svoie[voie]->niv1);
		break;

	}
	ff ();
	return;
}

int lig_bufi (int voie)
{
	int val;

	df ("lig_bufi", 1);

	val = svoie[voie]->inbuf.nblig;
	ff ();
	return (val);
}


void cr_cond (void)
{
	obuf *optr = pvoie->outptr;
	char *ptr;

	df ("cr_cond", 0);

	if (optr)
	{
		while (optr->suiv)
			optr = optr->suiv;
		ptr = optr->buffer + optr->nb_car - 1;
		if (*ptr == '\r')
		{
			ff ();
			return;
		}
	}
	cr ();
	ff ();
	return;
}


int send_buf (int voie)
{
	int ncars, nbcars, nocars;
	int retour = 0;
	int fin = 0;
	char buf[257];
	char *lptr = NULL;
	obuf *optr;
	char *ptr, *tptr;
	int maxcar;

	df ("send_buf", 1);

	/* Commute le TNC en emission (TOR mode) */
	if ((voie) && (svoie[voie]->pack == 0))
	{
		/* tor_start (voie); */
		svoie[voie]->pack = 1;
	}

	if ((voie == CONSOLE) || (voie == INEXPORT))
		maxcar = 250;
	else
		maxcar = svoie[voie]->paclen;

	if (maxcar == 0)
		sta_drv (voie, PACLEN, &maxcar);

	if ((voie == CONSOLE) && (kb_vide () == 0))
	{
		ff ();
		return (1);
	}

	if (!svoie[voie]->stop)
	{
		init_timout (voie);
		if ((optr = svoie[voie]->outptr) != NULL)
		{
			ncars = 0;
			tptr = buf;
			nbcars = optr->nb_car;
			nocars = optr->no_car;
			ptr = optr->buffer + nocars;
			while (1)
			{
				if (nbcars == nocars)
				{
					svoie[voie]->outptr = optr->suiv;
					m_libere (optr, sizeof (obuf));
					svoie[voie]->memoc -= 250;
					if ((optr = svoie[voie]->outptr) != NULL)
					{
						nbcars = optr->nb_car;
						nocars = 0;
						ptr = optr->buffer;
					}
					else
					{
						/*            cprintf(" Vide ") ; */
						break;
					}
				}
				*tptr++ = *ptr;
				++nocars;
				if ((*ptr == '\r') && (svoie[voie]->lignes > 0))
				{
					if ((--svoie[voie]->lignes == 0) &&
						((svoie[voie]->sr_mem) || (svoie[voie]->seq) || (svoie[voie]->t_tr) ||
						 (nbcars != nocars) || (optr->suiv)))
					{
						init_langue (voie);
						svoie[voie]->stop = 1;
						optr->no_car = nocars;
						lptr = langue[vlang]->plang[T_QST - 1];
						while (*lptr)
						{
							if (++ncars == maxcar)
							{
								fin = 1;
								break;
							}
							*tptr++ = *lptr++;
						}
						if (!fin)
						{
							lptr = NULL;
							++ncars;
						}
						break;
					}
				}
				if (++ncars == maxcar)
				{
					optr->no_car = nocars;
					break;
				}
				++ptr;
			}
			if (voie == CONSOLE)
			{
				deb_io ();
				aff_bas (voie, W_SNDT, buf, ncars);
				if (lptr)
					aff_bas (voie, W_SNDT, lptr, strlen (lptr));
				is_idle = 0;
				fin_io ();
				retour = 1;
			}
/*			else if (voie == INEXPORT)
			{
				deb_io ();
				aff_bas (voie, W_SNDT, buf, ncars);
				if (lptr)
					aff_bas (voie, W_SNDT, lptr, strlen (lptr));
				is_idle = 0;
				fin_io ();
				retour = 1;
			} */
			else
			{
				retour = snd_drv (voie, DATA, buf, ncars, NULL);
				++svoie[voie]->sta.ack;
				if (lptr)
				{
					retour = snd_drv (voie, DATA, lptr, strlen (lptr), NULL);
					++svoie[voie]->sta.ack;
				}
			}
		}
	}
	free_mem ();

	ff ();
	return (retour);
}


void clear_outbuf (int voie)
{
	obuf *optr;

	df ("clear_outbuf", 1);

	while ((optr = svoie[voie]->outptr) != NULL)
	{
		svoie[voie]->outptr = optr->suiv;
		m_libere (optr, sizeof (obuf));
		svoie[voie]->memoc -= 250;
	}
	free_mem ();
	ff ();
	return;
}


void clear_marque (int voie)
{
	int i;
	int trouve = 0;
	obuf *optr;

	df ("clear_marque", 1);

	while (svoie[voie]->t_read || svoie[voie]->outptr)
	{
		while ((optr = svoie[voie]->outptr) != NULL)
		{
			for (i = 0; i < NB_MARQUES; i++)
			{
				if (optr->marque[i] >= optr->no_car)
				{
					optr->no_car = optr->marque[i];
					optr->marque[i] = -1;
					trouve = 1;
					break;
				}
			}
			if (trouve)
				break;
			svoie[voie]->outptr = optr->suiv;
			m_libere (optr, sizeof (obuf));
			svoie[voie]->memoc -= 250;
		}
		if (trouve)
			break;
		if ((svoie[voie]->memoc <= MAXMEM / 2) && (svoie[voie]->sr_mem))
		{
			selvoie (voie);
			premier_niveau ();
		}
	}
	svoie[voie]->stop = 1;
	free_mem ();
	ff ();
	return;
}

int no_port (int voie)
{
	int val;

	df ("no_port", 1);

	if (voie == CONSOLE)
	{
		ff ();
		return (0);
	}

	val = svoie[voie]->affport.port;

	ff ();
	return (val);
}


int no_canal (int voie)
{
	int val;

	df ("no_canal", 1);

	val = svoie[voie]->affport.canal;
	ff ();
	return (val);
}

int tncin (int voie)
{
	int val;

	df ("tncin", 1);

	val = rcv_tnc (no_port (voie));
	ff ();
	return (val);
}


void tncout (int voie, int ch)
{
	int port;

	df ("tncout", 2);

	port = no_port (voie);

	while (car_tx (port))
		;						/* attend que le buffer d'emission soit vide */
	send_tnc (port, ch);
	ff ();
	return;
}


void tncstr (int port, char *ptr, int echo)
{
	int c;

	df ("tncstr", 4);

	while (car_tx (port))
		;						/* attend que le buffer d'emission soit vide */
	while ((c = *ptr++) != '\0')
	{
		send_tnc (port, c);
#ifdef __FBBDOS__
		if (echo)
		{
			putch (c);
			if (c == '\r')
				putch ('\n');
		}
#endif
	}
	ff ();
	return;
}


void ctrl_z (void)
{
	df ("ctrl_z", 0);

	outs ("\032\r", 2);

	ff ();
	return;
}


#ifdef __FBBDOS__
void curon (void)
/*******/
/* allume le curseur sur ecran alphanum */
{
	df ("curon", 0);

	_setcursortype (_NORMALCURSOR);
	ff ();
	return;
}


void curoff (void)
/********/
/* eteind le curseur sur l'ecran alphanum */
{
	df ("curoff", 0);

	_setcursortype (_NOCURSOR);
	ff ();
	return;
}


void cursor (int val)
{
	union REGS registres;

	df ("cursor", 1);

	registres.h.ah = 1;
	registres.x.cx = val;
	int86 (INT10, &registres, &registres);
	ff ();
	return;
}


int attcurs (void)
{
	int *ptr = MK_FP (0x40, 0x60);

	return (*ptr);
}
#endif

static void wheader (int voie, int color, char *s)
{
#ifdef __FBBDOS__
	FScreen *screen;
#endif

	deb_io ();

#if defined(__WINDOWS__) || defined(__linux__)
	aff_bas_suite (voie, color, "\r", 1);
#endif
#ifdef __FBBDOS__
	screen = (voie == CONSOLE) ? &conbuf : &winbuf;
	if (screen->carpos)
	{
		aff_bas_suite (screen->voie, screen->color, "\r", 1);
	}
#endif
	aff_bas_suite (voie, color, s, strlen (s));

	fin_io ();

	return;
}


void aff_header (int voie)
{
	char s[80];
	char *ptr = s;

/*	if (voie == CONSOLE) */
	if (!aff_ok(voie))
		return;

	df ("aff_header", 1);

	if ((ok_aff) && (voie != lastaff) && (svoie[voie]->sta.connect > 1))
	{
		deb_io ();
		sprintf (ptr, "[PORT %d (%s) - %2d - %s-%d]\r",
			 no_port (voie), p_port[no_port (voie)].freq, virt_canal (voie),
		   svoie[voie]->sta.indicatif.call, svoie[voie]->sta.indicatif.num);
		wheader (voie, -1, s);
		lastaff = voie;
		fin_io ();
	}
	ff ();
	return;
}


void aff_bas (int voie, int color, char *str, int nb)
{
	df ("affbas", 4);

	aff_header (voie);
	aff_bas_suite (voie, color, str, nb);

	ff ();
	return;
}


static void aff_bas_suite (int voie, int color, char *str, int nb)
{
	int i;
	int nbp = nb;
	char *ptr = str;
	static char buf[600];
	char *p_str = buf;

	if (nb == 0)
		return;

	df ("aff_bas_suite", 5);

	while (nb--)
	{
		if (*ptr >= '\040')
			*p_str++ = *ptr;
		else
		{
			switch (*ptr)
			{
			case '\a':
				*p_str++ = 14;
				break;
			case '\r':
				*p_str++ = '\r';
				*p_str++ = '\n';
				break;
			case '\t':
				for (i = 0; i < 8; i++)
					*p_str++ = ' ';
				break;
			case '\n':
			case '\032':
				*p_str++ = *ptr;
				break;
			}
		}
		++ptr;
	}
	*p_str = '\0';
	winputs (voie, color, buf);
	if (voie == CONSOLE)
	{
#if defined(__WINDOWS__) || defined(__linux__)
		if (print)
		{
			trait_time = 0;
			SpoolLine (voie, color, str, nbp);
		}
#else
		if (print)
			trait_time = 0;
		while ((print) && (nbp--))
		{
			if (*str >= '\040')
				fputc (*str, file_prn);
			else
			{
				switch (*str)
				{
				case '\a':
					fputc ('^', file_prn);
					fputc ('G', file_prn);
					break;
				case '\r':
					fputc ('\r', file_prn);
					fputc ('\n', file_prn);
					break;
				case '\t':
					for (i = 0; i < 8; i++)
						fputc (' ', file_prn);
					break;
				case '\032':
					fputc ('^', file_prn);
					fputc ('G', file_prn);
					break;
				}
			}
			++str;
		}
#endif
	}
	ff ();
	return;
}


char extind (char *chaine, char *indicatif)
{
	int i = 6;
	int ssid = 0;

	df ("extind", 4);

	while ((i--) && isalnum (*chaine))
	{
		*indicatif++ = *chaine++;
	}
	*indicatif = '\0';
	if ((*chaine) && (*chaine != ' '))
	{
		while (*chaine && !isdigit (*chaine))
			++chaine;
		if (*chaine)
		{
			ssid = atoi (chaine);
		}
	}
	ff ();
	return ((char) ssid);
}


void tst_appel (void)
{
	struct stat st;

	df ("tst_appel", 0);

	if (stat (d_disque ("OPTIONS.SYS"), &st) == 0)
	{
		if (st.st_mtime != t_appel)
		{
			t_appel = st.st_mtime;
			lit_appel ();
		}
	}
	ff ();
	return;
}

void tnc_commande (int select, char *command, int cmd)
{
	int voie;
	char buffer[600];

	/*
	   cmd = ECHOCMD  select = voie
	   cmd = SNDCMD   select = voie
	   cmd = PORTCMD  select = port
	 */

	if (*command == '\0')
		return;

	if (cmd == PORTCMD)
	{
		int nb = 0;
		char *ptr = buffer, *sv;

		voie = p_port[select].pr_voie;

		deb_io ();
		while (*command)
		{
			if (*command == '$')
			{
				++command;
				sv = variable (*command++);
				while (*sv)
				{
					*ptr++ = *sv++;
					if (++nb == 250)
						break;
				}
			}
			else if (*command == '%')
			{
				++command;
				sv = alt_variable (*command++);
				while (*sv)
				{
					*ptr++ = *sv++;
					if (++nb == 250)
						break;
				}
			}
			else
			{
				*ptr++ = *command++;
				nb++;
			}
			if (nb == 250)
				break;
		}
		*ptr = '\0';
		command = buffer;
		fin_io ();

#ifdef __FBBDOS__

		if (!operationnel)
		{
			ptr = buffer;
			cprintf ("%d : ", select);
			nb = 0;
			while (*ptr)
			{
				if (*ptr == '\r')
					putch ('\n');
				putch (*ptr);
				++ptr;
				if ((++nb == 2) && (p_port[select].typort == TYP_PK))
					putch (' ');
			}
			putch ('\n');
			putch ('\r');
		}
#endif
	}
	else
		voie = select;

	sta_drv (voie, cmd, (void *) command);
}

/* Called when Pactor was disconnected after 10 seconds */
static void FAR hst_disc (int port, void *userdata)
{
	/* Imediate disconnection */
	force_deconnexion ((uintptr_t) userdata, 1);
}

void dec (int voie, int mode)	/* Mode = 1 deconnexion   2 retour au nodal */
{
#ifndef __linux__
	int command;
#endif

	df ("dec", 2);

	deb_io ();

	switch (p_port[no_port (voie)].typort)
	{
	case TYP_DED:
	case TYP_FLX:
		tnc_commande (voie, "D", SNDCMD);
		break;
	case TYP_HST:
		tnc_commande (voie, "D", SNDCMD);
		if (P_TOR (voie))
		{
			/* Timer to force disconnection after 10 seconds */
			del_timer (p_port[port].t_delay);
			p_port[port].t_delay = add_timer (10, port, (void FAR *) hst_disc, (void *)(intptr_t)voie);
		}
		break;
	case TYP_PK:
		tnc_commande (voie, "DI", SNDCMD);
		break;
	case TYP_MOD:
		deconnect_modem (voie);
		break;
	case TYP_KAM:
		kam_commande (voie, "D");
		break;
#ifdef __WINDOWS__
	case TYP_TCP:
	case TYP_ETH:
	case TYP_AGW:
		tnc_commande (voie, "D", SNDCMD);
		break;
#endif
#ifdef __linux__
	case TYP_TCP:
	case TYP_ETH:
	case TYP_SCK:
	case TYP_POP:
		tnc_commande (voie, "D", SNDCMD);
		break;
#else
	case TYP_BPQ:
		command = mode + 1;
		sta_drv (voie, CMDE, (void *) &command);
		break;
#endif
	}
	fin_io ();

	ff ();
	return;
}

void programm_indic (int voie)
{
	char buffer[257];

	df ("programm_indic", 1);

	if ((svoie[voie]->sta.callsign.call[0]) && (p_port[no_port (voie)].typort == TYP_DED))
	{
		if ((!DEBUG) && (p_port[no_port (voie)].pvalid))
		{
			sprintf (buffer, "I %s-%d", svoie[voie]->sta.callsign.call, svoie[voie]->sta.callsign.num);
			tnc_commande (voie, buffer, ECHOCMD);
			if (*buffer & !svoie[CONSOLE]->sta.connect)
				cprintf ("%s\r\n",buffer);
		}
	}
	ff ();
	return;
}


void aff_event (int voie, int event)
{
	static char *event_string[3] =
	{
#ifdef ENGLISH
		"", "Connect  ", "Disconnect ",
#else
		"", "Connexion", "D‚connexion",
#endif
	};
	char s[80];
	char *ptr = s;

	if (!aff_ok(voie))
		return;

	df ("aff_event", 2);

	if (!svoie[CONSOLE]->sta.connect)
	{
		deb_io ();
		sprintf (ptr, "[PORT %d (%s) - %2d - %s-%d] %s %s\r",
			 no_port (voie), p_port[no_port (voie)].freq, virt_canal (voie),
			svoie[voie]->sta.indicatif.call, svoie[voie]->sta.indicatif.num,
				 strheure (time (NULL)), event_string[event]);
		wheader (voie, W_VOIE, s);
		if (voie != CONSOLE)
			lastaff = voie;
		fin_io ();
	}
	ff ();
	return;
}


int dec_voie (int voie)
{
	char temp[80];
	int save_voie, autre_voie;

	df ("dec_voie", 1);

	if (svoie[voie]->kiss != -1)
	{
		if (svoie[voie]->kiss < 0)
			svoie[voie]->kiss = CONSOLE;
		if (p_port[no_port (voie)].typort == TYP_BPQ)
		{
			int kiss = svoie[voie]->kiss;

			selvoie (kiss);
			texte (T_GAT + 0);
			aff_freq ();
			texte (T_GAT + 1);
			aff_etat ('E');
			send_buf (voie);
			svoie[kiss]->niv3 = 1;
			svoie[kiss]->cross_connect = -1;
			svoie[voie]->kiss = -1;
			selvoie (voie);
		}
		else
		{
			int kiss = svoie[voie]->kiss;

			selvoie (kiss);
			svoie[kiss]->niv3 = 2;
			texte (T_GAT + 4);
			selvoie (voie);
			ff ();
			return (0);
		}
	}

	if ((svoie[voie]->binary) && (svoie[voie]->niv1 == N_FORW) && (svoie[voie]->niv2 == 5) && (svoie[voie]->niv3 == 4))
	{
		/* Vide le message en cours de reception */
		if (svoie[voie]->fbb >= 2)
		{
			write_temp_bin (voie, FALSE);
			libere (voie);
		}
		else
		{
			del_part (voie, svoie[voie]->entmes.bid);
			libere (voie);
		}
	}

	programm_indic (voie);
	del_temp (voie);
	del_copy (voie);
	if (svoie[voie]->cross_connect != -1)
	{
		autre_voie = svoie[voie]->cross_connect;
		if (svoie[autre_voie]->kiss != -1)
		{
			if (svoie[autre_voie]->kiss < 0)
				svoie[autre_voie]->kiss = CONSOLE;
			save_voie = voie;
			selvoie (autre_voie);
			dec (autre_voie, 1);
			dec (autre_voie, 1);
			selvoie (save_voie);
			fin_tnc ();
		}
		svoie[autre_voie]->cross_connect = svoie[autre_voie]->kiss = -1;
	}
	if (v_tell == voie)
	{
		console_off ();
		v_tell = 0;
		music (0);
		svoie[CONSOLE]->sta.connect = 0;
		svoie[CONSOLE]->niv1 = svoie[CONSOLE]->niv2 = svoie[CONSOLE]->niv3 = 0;
	}
	if (svoie[voie]->l_yapp)
	{
		svoie[voie]->finf.lastyap = svoie[voie]->l_yapp;
		svoie[voie]->l_yapp = 0L;
	}
	if (svoie[voie]->sta.connect)
	{
		if (svoie[voie]->sta.connect > 1)
			aff_event (voie, 2);
		status (voie);
		if (((!voie_forward (voie)) && (svoie[voie]->sta.connect != 17)) ||
			((voie_forward (voie)) && (svoie[voie]->curfwd->no_con >= 8)))
		{
			majfich (voie);
		}
		svoie[voie]->deconnect = svoie[voie]->sta.connect = FALSE;
		aff_nbsta ();
		curseur ();
	}

	if (svoie[voie]->curfwd)
	{
		svoie[voie]->curfwd->forward = -1;
		svoie[voie]->curfwd->no_bbs = 0;
	}
	/********* A VERIFIER !! **********
	if (voie == p_port[no_port(voie)].forward) {
		p_port[no_port(voie)].forward = -1 ;
	}
	*/
	svoie[voie]->mode = 0;
#ifndef __WINDOWS__
	if (svoie[voie]->binary)
		aff_yapp (voie);
#endif
#ifdef __linux__
	kill_rzsz (voie);
#endif

	if (P_TOR (voie))
	{
		int port = no_port (voie);

		clear_queue (voie);
		if (p_port[port].t_busy)
		{
			m_libere (p_port[port].t_busy->userdata, sizeof (PortData));
			del_timer (p_port[port].t_busy);
			p_port[port].t_busy = NULL;
		}
		if (p_port[port].t_wait)
		{
			m_libere (p_port[port].t_wait->userdata, sizeof (PortData));
			del_timer (p_port[port].t_wait);
			p_port[port].t_wait = NULL;
		}
		del_timer (p_port[port].t_iss);
		p_port[port].t_iss = NULL;
	}

	svoie[voie]->deconnect = svoie[voie]->sta.connect = svoie[voie]->sta.stat = 0;
	svoie[voie]->temp1 = 1;
	svoie[voie]->fbb = bin_fwd;
	svoie[voie]->timout = time_n;
	if (svoie[voie]->conf)
	{
		svoie[voie]->conf = 0;
		text_conf (T_CNF + 5);
	}
	svoie[voie]->curfwd = NULL;
	set_binary (voie, 0);
	set_bs (voie, TRUE);
	svoie[voie]->conf = svoie[voie]->seq = 0;
	svoie[voie]->niv1 = svoie[voie]->niv2 = svoie[voie]->niv3 = 0;
	svoie[voie]->paclen = p_port[no_port (voie)].pk_t;
	svoie[voie]->cross_connect = -1;
	svoie[voie]->kiss = -1;
	svoie[voie]->groupe = -1;
	svoie[voie]->cur_bull = -1L;
	svoie[voie]->ch_mon = -1;
	svoie[voie]->clock = '\0';
	svoie[voie]->tmach = 0;
	svoie[voie]->memoc = 0;
	svoie[voie]->rev_mode = 1;
	svoie[voie]->type_yapp = 0;
	svoie[voie]->aut_linked = 1;
	svoie[voie]->maj_ok = FALSE;
	svoie[voie]->ind_mess = 0;
	svoie[voie]->nb_prompt = 0;
	svoie[voie]->prot_fwd = prot_fwd;
	if (svoie[voie]->ctnc)
		libere_tnc (&(svoie[voie]->ctnc));

/*************** svoie[voie]->ncur->coord = 0xffff; *****************/
	unlink (copy_name (voie, temp));
	libere_zones_allouees (voie);	/* Vide les eventuelles listes */
	init_fb_mess (voie);
	fbb_log (voie, 'X', "D");
	aff_forward ();
#if defined(__WINDOWS__) || defined(__linux__)
	window_disconnect (voie);
#endif
	ff ();
	return (1);
}


int port_free (int port)
{
	int i;
	int free;

	df ("port_free", 1);

	free = p_port[port].nb_voies;

	for (i = 1; i < NBVOIES; i++)
	{
		if ((no_port (i) == port) && (svoie[i]->sta.connect))
			--free;
	}

	ff ();
	return (free);
}

#define STATIMER 5
static void pactor_data (int port, int voie)
{
	static long prev = 0;
	static long pprev = 0;
	long delta;
	int tempo;


	if (pprev)
	{
		delta = p_port[port].cur - pprev;
		tempo = STATIMER * 2;
	}
	else
	{
		delta = p_port[port].cur - prev;
		tempo = STATIMER;
	}

	if (delta < 0)
		delta = 0;

	pprev = prev;
	prev = p_port[port].cur;

	sta_drv (voie, TOR, "%T");
	if (svoie[voie]->sta.connect)
	{
		p_port[port].nbc = (int) (delta / tempo);
		add_timer (STATIMER, port, pactor_data, (void *)(intptr_t) voie);
	}
	else
	{
		prev = pprev = 0;
		p_port[port].nbc = 0;
	}
	aff_traite (voie, -1);
}

int con_voie (int voie, char *ptr)
{
	int i;
	int port, canal;

	df ("con_voie", 3);

	if (svoie[voie]->kiss != -1)
	{
		int kiss_voie = svoie[voie]->kiss;

		svoie[kiss_voie]->niv3 = 3;
		ff ();
		return (0);
	}
	else
	{
		if (arret)
			svoie[voie]->dde_marche = TRUE;
		else
			svoie[voie]->dde_marche = FALSE;
		init_timout (voie);
		svoie[voie]->nb_err = svoie[voie]->seq = svoie[voie]->stop = 0;
		connect_fen ();

		lastaff = -1;
		svoie[voie]->private_dir = 0;
		/* svoie[voie]->wp = 0; */
		svoie[voie]->ret = 0;
		svoie[voie]->sid = 0;
		svoie[voie]->pack = 0;
		svoie[voie]->read_only = 0;
		svoie[voie]->aut_nc = 0;
		svoie[voie]->vdisk = 2;
		svoie[voie]->cmd_new = 0;
		svoie[voie]->rev_param = 0;
		svoie[voie]->log = 1;
		svoie[voie]->sta.stat = svoie[voie]->sta.connect = 4;
		svoie[voie]->deconnect = FALSE;
		svoie[voie]->ch_mon = svoie[voie]->cross_connect = -1;
		set_binary (voie, 0);
		svoie[voie]->sr_mem = svoie[voie]->conf = 0;
		*svoie[voie]->sr_fic = '\0';
/*      p_debug(no_port(voie), "Bip"); */
		bipper ();
		svoie[voie]->tstat = svoie[voie]->debut = time (NULL);
		svoie[voie]->aut_linked = 1;
		svoie[voie]->tmach = 0L;
		svoie[voie]->sta.ret = svoie[voie]->sta.ack = svoie[voie]->mode = 0;
		svoie[voie]->msg_held = 0;
		svoie[voie]->xferok = svoie[voie]->mess_recu = 1;
		svoie[voie]->entmes.numero = 0L;
		svoie[voie]->nb_egal = 0;
		svoie[voie]->mbl_ext = 1;
		svoie[voie]->rev_mode = 1;
		svoie[voie]->r_tete = NULL;
		*svoie[voie]->passwd = '\0';
		if (strtok (NULL, " "))
		{
			ptr = strtok (NULL, " ");	/* sta.indicatif */
			if (*(ptr + 1) == ':')
			{					/* Connexion DRSI ou BPQ */
				canal = *ptr - '0';
				if (DRSI (no_port (voie)))
				{
					port = drsi_port (no_port (voie), canal);
					svoie[voie]->affport.port = port;
				}
				else if (HST (no_port (voie)))
				{
					port = hst_port (no_port (voie), canal);
					svoie[voie]->affport.port = port;
				}
				else if (BPQ (no_port (voie)))
				{
					port = bpq_port (no_port (voie), canal);
					svoie[voie]->affport.port = port;
				}
#ifdef __WINDOWS__
				else if (AGW (no_port (voie)))
				{
					port = agw_port (no_port (voie), canal);
					svoie[voie]->affport.port = port;
				}
#endif
#ifdef __linux__
				else if (S_LINUX (no_port (voie)))
				{
					port = linux_port (no_port (voie), canal);
					svoie[voie]->affport.port = port;
				}
#endif
				else
				{
					port = no_port (voie);
				}

				ptr += 2;		/* Drsi Port */
				if (port_free (port) < 0)
					svoie[voie]->deconnect = 7;
			}
			/* if (*ptr == '!') */
			if (*ptr && !isalnum(*ptr))	/* PTC-II prefixes the call with some characters */
				++ptr;
			svoie[voie]->sta.indicatif.num = extind (ptr, svoie[voie]->sta.indicatif.call);
		}

		for (i = 0; i < 8; i++)
			*(svoie[voie]->sta.relais[i].call) = '\0';
		ptr = (strtok (NULL, " "));

		if (ptr && ((*ptr == 'v') || (*ptr == 'V')))
		{						/* relais */
			i = 0;
			while ((ptr = strtok (NULL, " ")) != NULL)
			{
				svoie[voie]->sta.relais[i].num = extind (ptr, svoie[voie]->sta.relais[i].call);
				i++;
			}
		}
		/* svoie[voie]->paclen = p_port[no_port(voie)].pk_t; */
		aff_event (voie, 1);
		status (voie);
		curseur ();
#if defined(__WINDOWS__) || defined(__linux__)
		window_connect (voie);
#endif
		svoie[voie]->entmes.bid[0] = '\0';
		init_fb_mess (voie);
		connexion (voie);		/* positionne ncur */
		new_om = nouveau (voie);
		/* nb de lignes de la pagination */
		/* svoie[voie]->mode = svoie[voie]->finf.flags & 0xff ; */
		svoie[voie]->mode = 0;
		svoie[voie]->l_mess = 0L;
		svoie[voie]->l_yapp = 0L;
		if ((fbb_fwd) && (svoie[voie]->fbb))
		{
			svoie[voie]->mode |= F_NFW;		/* Supporte le protocole FBB */
		}
		svoie[voie]->mbl = TRUE;
		svoie[voie]->maj_ok = 0;
		if (svoie[voie]->niv1 == 0)
			connect_log (voie, "\0");
		change_droits (voie);
		strcpy (svoie[voie]->dos_path, "\\");
		aff_nbsta ();

		if ((P_READ (voie)) && (!SYS (svoie[voie]->finf.flags)) && (!LOC (svoie[voie]->finf.flags)))
		{
			svoie[voie]->read_only = 1;
			/* read_only_alert (0, indd); */
		}

		if (P_TOR (voie))
		{
			/* clear_queue (voie); TEST F6FBB */
			sta_drv (voie, TOR, "%T");
			add_timer (STATIMER, no_port (voie), pactor_data, (void *)(intptr_t) voie);
			if (svoie[voie]->niv1 == 0)
			{
				/* Appel entrant */
				tor_start (voie);
				/* WinDebug("Incoming Call\r\n"); */
			}
			else
			{
				/* Appel entrant */
				tor_stop (voie);
				/* WinDebug("Outgoing Call\r\n"); */
			}
		}

		if (POP (no_port (voie)))
		{
			send_list(voie);
			
			svoie[voie]->mode |= F_FOR;
		}
		
		if ((svoie[voie]->ctnc == NULL) && (BBS (svoie[voie]->finf.flags)))
			prog_rev_tnc (voie);

		ff ();
		return (1);
	}
}


void connect_log (int voie, char *ajoute)
{
	int i;
	char s[256], s1[80];

	df ("connect_log", 3);

	sprintf (s, "%c %s-%d", (char) no_port (voie) + '@',
		   svoie[voie]->sta.indicatif.call, svoie[voie]->sta.indicatif.num);
	for (i = 0; i < 8; i++)
	{
		if (*(svoie[voie]->sta.relais[i].call) == '\0')
			break;
		if (i == 0)
			strcat (s, " VIA ");
		else
			strcat (s, ",");
		if (svoie[voie]->sta.relais[i].num)
			sprintf (s1, "%s-%d", svoie[voie]->sta.relais[i].call, svoie[voie]->sta.relais[i].num);
		else
			sprintf (s1, "%s", svoie[voie]->sta.relais[i].call);
		strcat (s, s1);
	}
	strcat (s, ajoute);
	fbb_log (voie, 'C', s);
	ff ();
	return;
}

#ifdef LOG

FILE *logfp;
int curlog = 0;
int flog = 0;
char liglog[80];
char *ptrlog;

open_log (void)
{
	df ("open_log", 1);

	logfp = fopen ("RES.RES", "wt");
	curlog = 0;
	flog = 1;

	ff ();
	return;
}


write_log (int c)
{
	df ("write_log", 1);

	if (flog == 0)
	{
		ff ();
		return;
	}

	liglog[curlog] = (isgraph (c)) ? c : '.';
	fprintf (logfp, "%02x ", c & 0xff);
	if (++curlog == 16)
	{
		liglog[curlog] = '\0';
		fprintf (logfp, " %s\n", liglog);
		curlog = 0;
	}
	ff ();
	return;
}


close_log (void)
{
	df ("close_log", 1);

	fclose (logfp);
	flog = 0;

	ff ();
	return;
}


#endif


int kam_commande (int voie, char *chaine)
{
	int retour;

	retour = sta_drv (voie, SNDCMD, (void *) chaine);

	return (retour);
}


void monitor (int port, char *buffer, int nb)
{
	int i;
	int old_voie;

	for (i = 0; i < NBVOIES; i++)
	{
		if (svoie[i]->ch_mon == port)
		{
			old_voie = voiecur;
			selvoie (i);
			prog_more (i);
			/* cprintf("Envoie monitor sur voie %d\r\n") ; */
			outsln (buffer, nb);
			selvoie (old_voie);
		}
	}
	return;
}


int rcv_tnc (int port)
{
	int c;
	int nb = 0;

	char s[80];

	df ("rcv_tnc", 1);

	if (!DEBUG)
	{
		tempo = 50;
		while (tempo)
		{
			if ((c = rec_tnc (port)) >= 0)
			{
				ff ();
				return (c);
			}
#ifdef __WINDOWS__
			BWinSleep (50);
			if (tempo > 0)
				--tempo;
#endif
		}
		if (p_port[port].typort == TYP_DED)
		{
			while (((c = rec_tnc (port)) == -1) && (nb < 255))
			{
				if (operationnel)
				{
				}
				nb++;
				/* a mettre dans la fenetre */
#ifdef __WINDOWS__
				DisplayResync (port, nb);
#else
				sprintf (s, "Port %d Resync %-4d", port, nb);
				aff_chaine (W_DEFL, 1, 1, s);
#endif
				/*if (!getvoie(CONSOLE)->sta.connect)
				   cprintf("\r%s", s) ; */
				/*      cprintf(" %d", nb_int) ; */
				while (car_tx (port))
					;			/* attend que le buffer d'emission soit vide */
				send_tnc (port, '\001');
				tempo = 2;
				while (tempo)
				{
#ifdef __WINDOWS__
					BWinSleep (50);
					if (tempo > 0)
						--tempo;
#endif
					/* environ 100 ms */
				}
			}
			if (c == -1)
			{
				if (!svoie[CONSOLE]->sta.connect)
					cprintf ("Resynchronisation impossible. Systeme arrete !!\r\n");
				fbb_error (ERR_SYNCHRO, "DED RECEIVE DATA", port);
				ff ();
				return (-1);
			}
		}
		vide (port, 0);
		++com_error;
#ifdef __WINDOWS__
		DisplayResync (port, 0);
#endif
		ff ();
		return (-1);
	}
	ff ();
	return (-1);
}


int vide (int port, int echo)
{
	int c, nb = 0;

	df ("vide", 2);

	if ((DEBUG) || (!p_port[port].pvalid))
	{
		deb_io ();
		cprintf ("Erreur port %d!\n", port);
		fin_io ();
		ff ();
		return (0);
	}

	p_port[port].portind = 0;

	tempo = 20;
	deb_io ();
	while (tempo)
	{
		if ((c = rec_tnc (port)) >= 0)
		{
#ifdef __FBBDOS__
			if (echo)
			{
				putch (c);
				if (c == '\r')
					putch ('\n');
			}
#endif
			tempo = 20;
			++nb;
		}
#ifdef __WINDOWS__
		else
		{
			BWinSleep (100);
			if (tempo > 0)
				--tempo;
		}
#endif
#ifdef __linux__
		else
		{
			usleep (100000);
			if (tempo > 0)
				--tempo;
		}
#endif
	}
	fin_io ();
	ff ();
	return (nb);
}



/*

 * end of tncio.c
 *
 */
