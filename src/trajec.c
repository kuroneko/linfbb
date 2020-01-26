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
 *  MODULE TRAJECTOGRAPHIE
 */

#include <serv.h>

/*
 *  Constantes de la trajectographie satellite
 */

#ifndef PI
#define PI      3.1415926535
#endif
#define P0      0.017453292519
#define CC      2.997925e5
#define R0      6378.16			/* Rayon terrestre a l'equateur */
#define F       0.00335289187
#define F2      0.001676445935	/* f/2 */
#define G0      7.5369793e13
#define G1      1.0027379093
#define EPSILON 1e-8

/* parametres satellite */
static double i0;		/* inclinaison plan orbite sur plan equateur */
static double a0;
static double a8;
static double a9;
static double c1;
static double c11;
static double c12;
static double c21;
static double c22;
static double c31;
static double c32;
static double c7;
static double c8;
static double c9;
static double e0;
static double e1;
static double e2;
static double e8;
static double e9;
static double f1;
static double f9;
static double g2;
static double g3;
static double h9;
static double jjdeb;
static double jjref;
static double k;
static double k0;
static double k_prec;
static double l5;
static double l9;
static double m0;
static double m8;
static double m9;
static double n0;
static double n8;
static double o0;
static double pas_var;
static double q0;
static double q3;
static double r5;
static double r8;
static double r_doppler;
static double s1;
static double s7;
static double s8;
static double s9;
static double t9;
static double v1;
static double w0;
static double w5;
static double w9;
static double x9;
static double x_sat;
static double y9;
static double y_sat;
static double z9;
static double z_sat;



static double julien (double, int, int, int, int, int);

static int arondi (double);

static long julien_to_pc (double);

static void calc1_trajec (void);
static void calcul_page_trajec (int);
static void calpas (void);
static void entete_sat (char *);
static void ligne_sat (int, int, int, int, double, long, long, int, int, int);
static void load_param_satel (int entete);
static void load_param_station (void);
static void orbite_sat (double date_sat, long no);
static void precal_trajec (void);
static void resimp_trajec (void);
static void satpos (void);
static void satsta (void);


static int arondi (double val)
{
	if (val > 0.0)
		return ((int) (val + 0.5));
	else
		return ((int) (val - 0.5));
}

static double julien (double d3, int n3, int y3, int h3, int m3, int s3)
{
	double j8;

	if (n3 == 0)
		n3 = 1;
	if (n3 < 3)
	{
		--y3;
		n3 += 12;
	}
	if (y3 < 80)
		y3 += 2000;
	else if (y3 < 100)
		y3 += 1900;
	j8 = floor ((double) y3 / 100.);
	j8 = floor (j8 / 4.) - j8 + floor (365.25 * (double) y3) +
		floor (30.6001 * (double) (n3 + 1)) + floor (d3) + 1720997.;
	j8 += (d3 - floor (d3)
		   + (3600. * (double) h3 + 60. * (double) m3 + (double) s3) / 86400.
		   + 2e-6);
	return (j8);
}


static long julien_to_pc (double jj)
{
	/* Conversion jour julien en date systeme */
	long tps_jul;

	tps_jul = (long) ((jj - 2440588.) * 86400.);
	return (tps_jul);
}

long date_to_pc (int d3, int n3, int y3, int h3, int m3, int s3)
{
	return (julien_to_pc (julien ((double) d3, n3, y3, h3, m3, s3)));
}

/*
 *  CALCULS TRAJECTOGRAPHIE
 */

static void init_trajec (void)
{
	/* r6, r4 = sauvegardes des 2 dernieres distances pour calcul doppler */
	pvoie->r6 = 0.0;
	pvoie->r4 = 0.0;

	pvoie->t_trajec = 0.0;
}


static void precal_trajec (void)
{
	/* en entree : pvoie->tdeb, e0, i0, m0, k0
	   * en sortie :  constantes g2, e2, e1, s1, c1, q0
	   *              k_prec, pas_var
	 */

	double t2, r2;
	double y4;

	y4 = (double) (pvoie->tdeb.annee - 1);
	t2 = floor (y4 / 400.0) - floor (y4 / 100.0) + floor (365.25 * y4);
	t2 = (t2 - 693595.5) / 36525.0;
	r2 = 6.6460656 + 2400.051262 * t2 + 0.00002581 * t2 * t2;
	g2 = (r2 - (24.0 * (y4 - 1899.0))) / 24.0;

	e2 = 1.0 - e0 * e0;
	e1 = sqrt (e2);
	s1 = sin (i0 * P0);
	c1 = cos (i0 * P0);

	q0 = m0 / 360.0 + (double) k0;

	k_prec = 0L;				/* orbite precedente pour affichage message de changement
								   d'orbite */
	pas_var = t9;				/* pas de calcul variable en fonction de la visibilite */
}


static void calc1_trajec (void)
{
	/* en entree : n0, jjdeb, jjref, q3, e2, o0, c1, s1, k
	   * en sortie : n8 = mouvement moyen corrige
	   *             a8 = demi grand axe corrige
	   *             o8 = raan corrige
	   *             m8, m9 = phase
	   *             c11, c12, c21, c22, c31, c32 = coef matrice passage
	   *             repere plan orbite a repere inertie celeste
	   *             c7, s7 = coef passage repere inertie celeste
	   *             a repere terrestre
	 */
	double u8, k2, o8, w8, s0, c0, s2, c2, gg7, q, temp;

	u8 = jjdeb + pvoie->t_trajec - jjref;
	n8 = n0 + q3 * u8;
	a8 = pow ((G0 / (n8 * n8)), 1.0 / 3.0);
	k2 = (9.95 * pow ((R0 / a8), 3.5) * u8) / (e2 * e2);
	o8 = o0 - k2 * c1;
	s0 = sin (o8 * P0);
	c0 = cos (o8 * P0);
	w8 = w0 + k2 * (2.5 * c1 * c1 - 0.5);
	s2 = sin (w8 * P0);
	c2 = cos (w8 * P0);

	c11 = c2 * c0 - s2 * s0 * c1;
	c12 = -s2 * c0 - c2 * s0 * c1;
	c21 = c2 * s0 + s2 * c0 * c1;
	c22 = c2 * c0 * c1 - s2 * s0;
	c31 = s2 * s1;
	c32 = c2 * s1;

	gg7 = (g3 + u8) * G1 + g2;	/* calcul temps sideral */
	gg7 = modf (gg7, &temp) * PI * 2.0;
	s7 = -sin (gg7);
	c7 = cos (gg7);

	/* calcul de la phase */
	q = q0 + n8 * u8;
	k = (long) q;
	if (q < 0.)
		k--;
	m9 = (int) ((q - (double) k) * 256.0 + 0.5);
	m8 = (q - (double) k) * PI * 2.0;
}


static void satpos (void)
{
	/* en entree : m8, e0, a8, e1
	   * en sortie : x_sat, y_sat, z_sat, r8
	 */

	double e, s4, c4, r3, am5, x0, y0, x1, y1, z1;

	/* e = solution equation de Kepler. */
	e = m8 + sin (m8) * e0 + 0.5 * e0 * e0 * sin (2.0 * m8);
	do
	{
		/*    putchar('#') ; */
		s4 = sin (e);
		c4 = cos (e);
		r3 = 1.0 - e0 * c4;
		am5 = e - e0 * s4 - m8;
		e -= am5 / r3;
	}
	while (fabs (am5) > EPSILON);

	/* Position satellite ds plan orbite */
	x0 = a8 * (c4 - e0);
	y0 = a8 * e1 * s4;
	r8 = a8 * r3;

	/* Position du satellite ds repere inertie celeste */
	x1 = x0 * c11 + y0 * c12;
	y1 = x0 * c21 + y0 * c22;
	z1 = x0 * c31 + y0 * c32;

	/* Position satellite ds repere terrestre */
	x_sat = x1 * c7 - y1 * s7;
	y_sat = x1 * s7 + y1 * c7;
	z_sat = z1;
}


static void satsta (void)
{
	/* en entree : x_sat, y_sat, z_sat, x9, y9, z9
	   *             pas_var
	   *             f1, v1, pvoie->r4, pvoie->r6
	   * en sortie : r_doppler, pvoie->r4, pvoie->r6, f9
	   *             r5, e9, a9, l5, w5
	 */

	double x5, y5, z5, x8, y8, z8, s5, c5, b5;

	x5 = x_sat - x9;
	y5 = y_sat - y9;
	z5 = z_sat - z9;
	r5 = sqrt (x5 * x5 + y5 * y5 + z5 * z5);	/* distance du satellite */

	/* parametres doppler */
	if (pvoie->t_trajec > t9)
		r_doppler = (pvoie->r4 - 4.0 * pvoie->r6 + 3.0 * r5) / pas_var / 172800.0;
	else
		r_doppler = 0.0;
	pvoie->r4 = pvoie->r6;
	pvoie->r6 = r5;
	f9 = -f1 * 1e6 * r_doppler / CC - 1000.0 * v1;

	/* Position satellite ds repere station */
	x8 = z5 * c9 - x5 * c8 * s9 - y5 * s8 * s9;
	y8 = y5 * c8 - x5 * s8;
	z8 = x5 * c8 * c9 + y5 * s8 * c9 + z5 * s9;

	/* Calcul elevation et azimut antenne */
	s5 = z8 / r5;
	c5 = sqrt (1.0 - s5 * s5);
	e9 = atan (s5 / c5) / P0;	/* elevation */
	w5 = -atan2 (y_sat, x_sat) / P0;	/* longitude projection du satellite */
	if (w5 < 0.0)
		w5 += 360.0;

	a9 = atan2 (y8, x8) / P0;	/* azimut */
	if (a9 < 0.0)
		a9 += 360.0;

	b5 = z_sat / r8;
	l5 = b5 / sqrt (1.0 - b5 * b5);
	l5 = atan (l5) / P0;		/* latitude projection du satellite */
}


static void resimp_trajec (void)
{
	/* impression des resultats trajecto */
	int min, h, m;
	long dist, alt;
	double i_part, dp;

	min = arondi (modf (jjdeb + pvoie->t_trajec, &i_part) * 1440.0);
	m = min % 60;
	h = min / 60;
	dp = f9 / 1000.0;
	alt = (long) (r8 - R0 + 0.5);
	dist = (long) (r5 + 0.5);
	ligne_sat (h, m, arondi (a9), arondi (e9), dp, dist, alt,
			   arondi (w5), arondi (l5), m9);
}


static void calpas (void)
{
	/* en entree : e9 - e8 < 0
	   * en sortie : pas_var (pas augmente si satellite non visible)
	 */

	if ((e9 - e8) < -60.0)
		pas_var = t9 * 4.0;
	else if (r_doppler >= 0.)
		pas_var = t9 * 6.0;
	else if ((e9 - e8) < -30.0)
		pas_var = t9 * 2.0;
	else
		pas_var = t9;
}


static void calcul_page_trajec (int entete)
{
	char s[81];
	int cpt_lig = 0;
	int nb_lig = 0;

	pvoie->lignes = -1;
	load_param_satel (entete);
	load_param_station ();
	precal_trajec ();
	nb_lig = pvoie->finf.nbl;
	if (nb_lig > MAXLIGNES)
		nb_lig = MAXLIGNES;
	if (entete)
		nb_lig -= 5;
	else
		nb_lig -= 1;

	if (nb_lig <= 4)
		nb_lig = 4;

	/* Boucle de calcul */
	do
	{
		calc1_trajec ();
		satpos ();
		satsta ();
		sprintf (s, "Traj : %s", strheure (julien_to_pc (jjdeb + pvoie->t_trajec)));
		aff_chaine (W_DEFL, 69, 3, s);
		if (e9 >= e8)
		{
			/* changement d'orbite ? */
			if (k != k_prec)
			{
				orbite_sat (jjdeb + pvoie->t_trajec, k);
				k_prec = k;
				cpt_lig++;
			}
			/* satellite en visibilite */
			resimp_trajec ();
			cpt_lig++;
			pas_var = t9;
		}
		else
		{
			/* satellite non visible */
			if (t9 < 0.007)
				calpas ();		/* pas de calcul < 10mn */
		}
		pvoie->t_trajec += pas_var;
		if (trait_time > 15)
		{
			outln ("No visibility ...", 17);
			break;
		}
	}
	while (cpt_lig < nb_lig);
	aff_chaine (W_DEFL, 69, 3, "            ");
}


static void ligne_sat (int h, int m, int az, int el, double dp,
					   long dist, long alt, int lon, int lat, int ph)
{
	sprintf (varx[0], "%02d", h);
	sprintf (varx[1], "%02d", m);
	sprintf (varx[2], "%3d", az);
	sprintf (varx[3], "%3d", el);
	sprintf (varx[4], "%4.1f", dp);
	sprintf (varx[5], "%5ld", dist);
	sprintf (varx[6], "%5ld", alt);
	sprintf (varx[7], "%3d", lon);
	sprintf (varx[8], "%3d", lat);
	sprintf (varx[9], "%3d", ph);
	texte (T_TRJ + 8);
}



static void entete_sat (char *satellite)
{
	if (*(pvoie->finf.qra) == '?')
		var_cpy (0, qra_locator);
	else
		var_cpy (0, pvoie->finf.qra);
	var_cpy (1, satellite);
	texte (T_TRJ + 3);
	texte (T_TRJ + 4);
	texte (T_TRJ + 5);
	texte (T_TRJ + 6);
}



static void orbite_sat (double date_sat, long no)
{
	ptmes->date = julien_to_pc (date_sat);
	sprintf (varx[0], "%5ld", no);
	texte (T_TRJ + 7);
}


void trajecto (void)
{
	char c;
	int i, jour;
	FILE *fptr;

	switch (pvoie->niv3)
	{
	case 0:
		if ((i = selection_sat ()) == -1)
			break;
		fptr = ouvre_sat ();
		pvoie->enrcur = (long) sizeof (satel) * (long) i;
		fflush (fptr);
		if ((i < 0) || (pvoie->enrcur >= filelength (fileno (fptr))))
		{
			ferme (fptr, 23);
			texte (T_ERR + 0);
			menu_sat ();
		}
		else
		{
			ferme (fptr, 24);
			texte (T_TRJ + 9);
			maj_niv (pvoie->niv1, pvoie->niv2, 2);
		}
		break;
	case 2:
		if (test_date (indd))
		{
			sscanf (indd, "%d %d %d",
					&jour, &(pvoie->tdeb.mois),
					&(pvoie->tdeb.annee));
			pvoie->tdeb.jour = (double) jour;
			if (pvoie->tdeb.annee < 80)
				pvoie->tdeb.annee += 2000;
			else if (pvoie->tdeb.annee < 100)
				pvoie->tdeb.annee += 1900;
			texte (T_TRJ + 10);
			maj_niv (pvoie->niv1, pvoie->niv2, 3);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_TRJ + 9);
		}
		break;
	case 3:
		if (test_heure (indd))
		{
			sscanf (indd, "%d %d", &(pvoie->tdeb.heure), &(pvoie->tdeb.mn));
			texte (T_TRJ + 11);
			/*              prog_more(voiecur) ; */
			aff_etat ('T');
			pvoie->tdeb.sec = 0;
			if (min_ok (voiecur))
			{
				aff_etat ('E');
				send_buf (voiecur);
			}
			init_trajec ();
			calcul_page_trajec (1);
			texte (T_TRT + 11);
			maj_niv (pvoie->niv1, pvoie->niv2, 4);
		}
		else
		{
			texte (T_ERR + 0);
			texte (T_TRJ + 10);
		}
		break;
	case 4:
		c = toupper (*indd);
		if ((c == 'A') || (c == Non))
		{
			incindd ();
			maj_niv (pvoie->niv1, 0, 0);
			menu_trajec ();
		}
		else
		{
			calcul_page_trajec (0);
			texte (T_TRT + 11);
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "COMP-TRAJ", pvoie->niv3);
		break;
	}
}


void menu_trajec (void)
{
	int error = 0;
	char com[80];

	limite_commande ();
	sup_ln (indd);
	while (*indd && (!ISGRAPH (*indd)))
		indd++;
	strn_cpy (70, com, indd);

	switch (toupper (*indd))
	{
	case 'T':
		maj_niv (6, 1, 0);
		incindd ();
		trajecto ();
		break;
	case 'P':
		maj_niv (6, 2, 0);
		incindd ();
		param_satel ();
		break;
	case 'C':
		maj_niv (6, 3, 0);
		incindd ();
		carac_satel ();
		break;
	case 'M':
		if (droits (MODLABEL))
		{
			maj_niv (6, 9, 0);
			incindd ();
			modif_satel ();
		}
		else
		{
			error = 1;
		}
		break;
	case 'B':
		maj_niv (N_MENU, 0, 0);
		sortie ();
		break;
	case 'F':
		maj_niv (0, 1, 0);
		incindd ();
		choix ();
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


static void load_param_satel (int entete)
{
	/* recupere caracteristiques satellite ds fichier
	   * en sortie : i0, o0, e0, w0, a0, n0, q3, k0, f1, v1,
	   *             t9, g3, jjref, jjdeb
	 */
	satel bufsat;

	lit_sat (&bufsat);
	i0 = bufsat.i0;
	o0 = bufsat.o0;
	e0 = bufsat.e0;
	w0 = bufsat.w0;
	m0 = bufsat.m0;
	a0 = bufsat.a0;
	n0 = bufsat.n0;
	q3 = bufsat.q3;
	k0 = bufsat.k0;
	f1 = bufsat.f1;
	v1 = bufsat.v1;
	t9 = ((double) bufsat.pas) / 1440.0;
	if (a0 != 0.0)
		n0 = sqrt (G0 / (a0 * a0 * a0));
	else if (n0 != 0.0)
		a0 = pow ((G0 / (n0 * n0)), 1. / 3.);

	/* date references orbitales */
	g3 = bufsat.d3;
	jjref = julien (g3, 0, bufsat.y3, 0, 0, 0);

	/* date debut de calcul */
	jjdeb = julien (pvoie->tdeb.jour, pvoie->tdeb.mois, pvoie->tdeb.annee,
					pvoie->tdeb.heure, pvoie->tdeb.mn, pvoie->tdeb.sec);

	if (entete)
		entete_sat (bufsat.dd);
}


static void load_param_station (void)
{
	/* en sortie : l9, w9, e8, s9, c9, s8, c8, x9, y9, z9
	 */

	double ll8, r9, l8;

	/* recuperer QRA locator ds fichier */
	/*  l9 = 43.657 ;
	   w9 = -1.505 ; */
	if (*(pvoie->finf.qra) == '?')
		lonlat (qra_locator, &w9, &l9);
	else
		lonlat (pvoie->finf.qra, &w9, &l9);
	h9 = 180.0;					/* Altitude */
	e8 = -5.0;					/* Elevation minimale */

	/* coord. station ds repere terrestre */
	ll8 = l9 * P0;
	s9 = sin (ll8);
	c9 = cos (ll8);
	s8 = sin (-w9 * P0);
	c8 = cos (w9 * P0);
	r9 = R0 * (1.0 - F2 + F2 * cos (2.0 * ll8)) + h9 / 1000.0;
	l8 = atan ((1.0 - F) * (1.0 - F) * s9 / c9);
	x9 = r9 * cos (l8) * c8;
	y9 = r9 * cos (l8) * s8;
	z9 = r9 * sin (l8);
}
