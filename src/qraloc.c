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

static char qral[7];

#define MILES 1.609

static int tstll (double x)
{
	return ((x > -180) && (x < 180));
}


static int end (char *s)
{
	strupr (s);
	return ((strcmp (s, "Q") == 0) || (strcmp (s, "F") == 0));
}

void lonlat (char *qra, double *w, double *l)
{
	*w = 180.0 - ((double) (qra[0] - 'A')) * 20.0 -
		((double) (qra[2] - '0')) * 2.0 -
		((double) qra[4] - 64.5) / 12.0;
	*l = -90.0 + ((double) (qra[1] - 'A')) * 10.0 +
		((double) (qra[3] - '0')) +
		((double) qra[5] - 64.5) / 24.0;
}


static char *qra_loc (double w, double l)
{
	qral[0] = 'A' + (int) (w = (180 - w) / 20);
	qral[1] = 'A' + (int) (l = (90 + l) / 10);
	qral[2] = '0' + (int) (w = (w - floor (w)) * 10);
	qral[3] = '0' + (int) (l = (l - floor (l)) * 10);
	qral[4] = 'A' + (int) ((w - floor (w)) * 24);
	qral[5] = 'A' + (int) ((l - floor (l)) * 24);
	qral[6] = '\0';
	return (qral);
}


static double dist (double w, double l, double wa, double la)
{
	double x1, y1;

	y1 = cos (l / DEGRAD) * cos (la / DEGRAD) * cos ((w - wa) / DEGRAD) +
		sin (l / DEGRAD) * sin (la / DEGRAD);
	if (y1 > 1)
		y1 = 1;
	if (y1 < -1)
		y1 = -1;
	x1 = atan (sqrt (1 - y1 * y1) / y1);
	if (y1 < 0)
		x1 = 180 / DEGRAD + x1;
	return (6371.3 * x1);
}


static double azimut (double w, double l, double wa, double la)
{
	double x1, y1, z1;

	x1 = dist (w, l, wa, la) / 6371.3;
	if (x1)
	{
		y1 = (sin (la / DEGRAD) - sin (l / DEGRAD) * cos (x1)) / (cos (l / DEGRAD) * sin (x1));
		if (y1 > 1)
			y1 = 1;
		if (y1 < -1)
			y1 = -1;
		z1 = fabs (DEGRAD * atan (sqrt (1 - y1 * y1) / y1));
		if (y1 < 0)
			z1 = 180 - z1;
		if (sin ((w - wa) / DEGRAD) < 0)
			z1 = 360 - z1;
		return (z1);
	}
	else
		return (0);
}


#if 0
double litllat (chaine)
	 char *chaine;
{
	char ligne[81];
	char *scan = chaine, *ptr = ligne;
	char Sud, Est;
	double temp;

	Sud = *(langue[vlang]->plang[T_DEB + 7 - 1]);
	Est = *(langue[vlang]->plang[T_DEB + 8 - 1]);
	/*
	   Nord  = *(langue[vlang]->plang[T_DEB+6-1]) ;
	   Ouest = *(langue[vlang]->plang[T_DEB+9-1]) ;
	 */

	while (isdigit (*scan) || (*scan == '.') || (*scan == ','))
	{
		if (*scan == ',')
			*scan = '.';
		*ptr++ = *scan++;
	}
	*ptr = '\0';
	temp = atof (ligne);
	while (*scan && !ISGRAPH (*scan))
		scan++;
	if (toupper (*scan) == 'G')
	{
		temp *= 0.9;
		while (ISGRAPH (*scan))
			++scan;
	}
	else
	{
		if (toupper (*scan) == 'D')
		{
			while (ISGRAPH (*scan))
				++scan;
			while (*scan && !ISGRAPH (*scan))
				++scan;
			ptr = ligne;
			if (isdigit (*scan))
			{
				while (isdigit (*scan) || (*scan == '.') || (*scan == ','))
				{
					if (*scan == ',')
						*scan = '.';
					*ptr++ = *scan++;
				}
				*ptr = '\0';
				temp += (atof (ligne) / 60.0);
				while (*scan && !ISGRAPH (*scan))
					scan++;
				if (*scan == '\'')
					++scan;
			}
		}
		else
			return (1000.0);
	}
	if (!tstll (temp))
		return (1000.0);
	while (*scan && !ISGRAPH (*scan))
		++scan;
	if ((toupper (*scan) == Est) || (toupper (*scan) == Sud))
		temp = -temp;
	else if ((toupper (*scan) != Oui) && (toupper (*scan) != Non))
		return (1000.0);
	return (temp);
}


#endif

static double litllat (char *chaine, int mode)
{
	int deg, min, sec, grade = 1;
	char ctemp;
	char *scan = chaine;
	char Nord = 'N', Sud = 'S', Est = 'E', Ouest = 'W';
	double temp;

	if (mode)
	{							/* longitude */
		Nord = *(langue[vlang]->plang[T_DEB + 6 - 1]);
		Sud = *(langue[vlang]->plang[T_DEB + 7 - 1]);
	}
	else
	{							/* Lattitude */
		Est = *(langue[vlang]->plang[T_DEB + 8 - 1]);
		Ouest = *(langue[vlang]->plang[T_DEB + 9 - 1]);
	}

	while (*scan)
	{
		if (*scan == ',')
			*scan = '.';
		if (*scan == ':')
			grade = 0;
		scan++;
	}

	if (grade)
	{
		if (sscanf (chaine, "%lg %c", &temp, &ctemp) != 2)
			return (1000.0);
		temp *= 0.9;
	}
	else
	{
		if (sscanf (chaine, "%d:%d:%d %c", &deg, &min, &sec, &ctemp) != 4)
			return (1000.0);
		if ((deg > 180) || (min >= 60) || (sec >= 60))
			return (1000.0);
		temp = (double) deg + (double) min / 60.0 + (double) sec / 3600.0;
	}

	if (!tstll (temp))
		return (1000.0);

	ctemp = toupper (ctemp);
	if (mode)
	{
		if (ctemp == Sud)
			temp = -temp;
		else if (ctemp != Nord)
			temp = (1000.0);
	}
	else
	{
		if (ctemp == Est)
			temp = -temp;
		else if (ctemp != Ouest)
			temp = (1000.0);
	}
	return (temp);
}


static void longla (void)
{
	double w, l, wdeg, wmn, ldeg, lmn;
	char s[81], longi[6], latit[6];

	switch (pvoie->niv3)
	{
	case 0:
		texte (T_QRA + 3);
		maj_niv (2, 1, 1);
		break;
	case 1:
		epure (s, 7);
		if (tstqra (strupr (s)))
		{
			lonlat (s, &w, &l);
			if (w >= 0)
				n_cpy (5, longi, langue[vlang]->plang[T_DEB + 9 - 1]);
			else
			{
				w = fabs (w);
				n_cpy (5, longi, langue[vlang]->plang[T_DEB + 8 - 1]);
			}
			if (l >= 0)
				n_cpy (5, latit, langue[vlang]->plang[T_DEB + 6 - 1]);
			else
			{
				l = fabs (l);
				n_cpy (5, latit, langue[vlang]->plang[T_DEB + 7 - 1]);
			}
			wmn = modf (w, &wdeg) * 60.0;
			if (wmn >= 59.5)
			{
				wmn = 0;
				wdeg++;
			}
			w /= 0.9;
			lmn = modf (l, &ldeg) * 60.0;
			if (lmn >= 59.5)
			{
				lmn = 0;
				ldeg++;
			}
			l /= 0.9;
			texte (T_QRA + 4);
			sprintf (varx[0], "%3.0f", wdeg);
			sprintf (varx[1], "%02.0f", wmn);
			var_cpy (2, longi);
			sprintf (varx[3], "%5.1f", w);
			texte (T_QRA + 5);
			sprintf (varx[0], "%3.0f", ldeg);
			sprintf (varx[1], "%02.0f", lmn);
			var_cpy (2, latit);
			sprintf (varx[3], "%5.1f", l);
			texte (T_QRA + 6);
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else if (end (s))
		{
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_QRA + 3);
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "LONG-LATT", pvoie->niv3);
		break;
	}
}


static void calcul_qra (void)
{
	double l;
	char s[81];

	switch (pvoie->niv3)
	{
	case 0:
		texte (T_QRA + 7);
		texte (T_QRA + 8);
		maj_niv (2, 2, 1);
		break;
	case 1:
		epure (s, 80);
		if ((pvoie->w = litllat (s, 0)) != 1000.0)
		{
			texte (T_QRA + 9);
			maj_niv (2, 2, 2);
		}
		else if (end (s))
		{
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_QRA + 7);
			texte (T_QRA + 8);
		}
		break;
	case 2:
		epure (s, 80);
		if ((l = litllat (s, 1)) != 1000.0)
		{
			var_cpy (0, qra_loc (pvoie->w, l));
			texte (T_QRA + 10);
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else if (end (s))
		{
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_QRA + 9);
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "COMPUT-QRA", pvoie->niv3);
		break;
	}
}


static void disazi (void)
{
	char s[81];
	double d, wa, la, azim, deg, mn;

	switch (pvoie->niv3)
	{
	case 0:
		texte (T_QRA + 11);
		maj_niv (2, 3, 1);
		break;
	case 1:
		epure (s, 7);
		if (tstqra (strupr (s)))
		{
			lonlat (s, &(pvoie->w), &(pvoie->l));
			texte (T_QRA + 12);
			maj_niv (2, 3, 2);
		}
		else if (end (s))
		{
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_QRA + 11);
		}
		break;
	case 2:
		epure (s, 7);
		if (tstqra (strupr (s)))
		{
			lonlat (s, &wa, &la);
			azim = azimut (pvoie->w, pvoie->l, wa, la);
			mn = modf (azim, &deg) * 60.0;
			if (mn >= 59.5)
			{
				mn = 0;
				deg++;
			}
			azim /= 0.9;

			d = dist (pvoie->w, pvoie->l, wa, la);
			sprintf (varx[0], "%1.1f", d);
			sprintf (varx[1], "%1.1f", d / MILES);
			texte (T_QRA + 14);
			sprintf (varx[0], "%1.0f", deg);
			sprintf (varx[1], "%1.0f", mn);
			sprintf (varx[2], "%1.1f", azim);
			texte (T_QRA + 13);
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else if (end (s))
		{
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_QRA + 12);
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "DIST-AZIM", pvoie->niv3);
		break;
	}
}


static void cumul_disazi (void)
{
	char s[81];
	double wa, la, azim, deg, mn, d;

	switch (pvoie->niv3)
	{
	case 0:
		texte (T_QRA + 15);
		maj_niv (2, 4, 1);
		break;
	case 1:
		epure (s, 7);
		if (tstqra (strupr (s)))
		{
			lonlat (s, &(pvoie->w), &(pvoie->l));
			texte (T_QRA + 16);
			maj_niv (2, 4, 2);
		}
		else if (end (s))
		{
			maj_niv (2, 0, 0);
			prompt (pvoie->finf.flags, pvoie->niv1);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_QRA + 15);
		}
		break;
	case 2:
		epure (s, 7);
		if (tstqra (strupr (s)))
		{
			lonlat (s, &wa, &la);
			azim = azimut (pvoie->w, pvoie->l, wa, la);
			mn = modf (azim, &deg) * 60.0;
			if (mn >= 59.5)
			{
				mn = 0;
				deg++;
			}
			azim /= 0.9;
			d = dist (pvoie->w, pvoie->l, wa, la);
			if (d == 0.)
				d = 1.;

			sprintf (varx[0], "%1.1f", d);
			sprintf (varx[1], "%1.1f", d / MILES);
			texte (T_QRA + 14);
			pvoie->noenr_menu++;
			pvoie->cumul_dist += d;
			ltoa (pvoie->noenr_menu, varx[0], 10);
			sprintf (varx[1], "%1.0f", pvoie->cumul_dist);
			sprintf (varx[2], "%1.0f", pvoie->cumul_dist / MILES);
			texte (T_QRA + 17);
			texte (T_QRA + 16);
		}
		else
		{
			if (end (s))
			{
				texte (T_QRA + 18);
				maj_niv (2, 0, 0);
				prompt (pvoie->finf.flags, pvoie->niv1);
			}
			else
			{
				texte (T_ERR + 0);
				texte (T_QRA + 17);
				texte (T_QRA + 16);
			}
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "CUMUL-DISAZI", pvoie->niv3);
		break;
	}
}


static void menu_qraloc (void)
{
	int error = 0;
	char com[80];

	limite_commande ();
	while (*indd && (!ISGRAPH (*indd)))
		indd++;
	strn_cpy (70, com, indd);

	switch (toupper (*indd))
	{
	case 'Q':
		maj_niv (2, 1, 0);
		longla ();
		break;
	case 'L':
		maj_niv (2, 2, 0);
		calcul_qra ();
		break;
	case 'D':
		maj_niv (2, 3, 0);
		disazi ();
		break;
	case 'C':
		pvoie->noenr_menu = 0;
		pvoie->cumul_dist = 0.0;
		maj_niv (2, 4, 0);
		cumul_disazi ();
		break;
	case 'F':
		maj_niv (0, 1, 0);
		incindd ();
		choix ();
		break;
	case 'B':
		maj_niv (N_MENU, 0, 0);
		sortie ();
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
		cmd_err (indd);
	}
}


void qraloc (void)
{
	switch (pvoie->niv2)
	{
	case 0:
		menu_qraloc ();
		break;
	case 1:
		longla ();
		break;
	case 2:
		calcul_qra ();
		break;
	case 3:
		disazi ();
		break;
	case 4:
		cumul_disazi ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "QRA-LOC", pvoie->niv2);
		break;
	}
}
