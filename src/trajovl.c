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
 *  MODULE TRAJECTOGRAPHIE  (OVELAY)
 */

#include <serv.h>

static void affiche_param (satel *);
static void ecrit_sat (satel *);
static void supprime_sat (void);
static void vis_param (satel *);

/*
 *        CARACTERISTIQUES SATELLITE
 */

void carac_satel (void)
{
	int i;
	FILE *fptr;
	satel bufsat;
	char satstr[80];

	if ((i = selection_sat ()) == -1)
		return;
	fptr = ouvre_sat ();
	pvoie->enrcur = (long) sizeof (bufsat) * (long) i;
	fflush (fptr);
	if (pvoie->enrcur < filelength (fileno (fptr)))
	{
		fseek (fptr, pvoie->enrcur, 0);
		fread ((char *) &bufsat, sizeof (bufsat), 1, fptr);
		sprintf (satstr, "SAT\\%ld.SAT", bufsat.cat);
		if (!outfichs (d_disque (satstr)))
		{
			texte (T_TRJ + 12);
		}
	}
	ferme (fptr, 25);
	maj_niv (6, 0, 0);
	prompt (pvoie->finf.flags, pvoie->niv1);
}


/*
 *       SAISIE PARAMETRES SATELLITE
 */

void modif_satel (void)
{
	char c;
	int i;
	satel bufsat;
	FILE *fptr;

	switch (pvoie->niv3)
	{
	case 0:
		if ((i = selection_sat ()) == -1)
			break;
		if (i < 0)
		{
			texte (T_ERR + 0);
			menu_sat ();
		}
		else
		{
			fptr = ouvre_sat ();
			pvoie->enrcur = (long) sizeof (bufsat) * (long) i;
			fflush (fptr);
			if (pvoie->enrcur < filelength (fileno (fptr)))
			{
				fseek (fptr, pvoie->enrcur, 0);
				fread ((char *) &bufsat, sizeof (bufsat), 1, fptr);
				vis_param (&bufsat);
				ptmes->date = bufsat.maj;
				pvoie->temp1 = 0;
				texte (T_TRJ + 13);
				var_cpy (0, bufsat.dd);
				texte (T_MBL + 30);
			}
			else
			{
				fflush (fptr);
				pvoie->enrcur = filelength (fileno (fptr));
				pvoie->temp1 = 1;
				texte (T_QST + 4);
			}
			maj_niv (6, 9, 2);
			ferme (fptr, 25);
		}
		break;
	case 2:
		c = toupper (*indd);
		if (pvoie->temp1)
		{						/* Creation */
			if (c == Oui)
			{
				texte (T_TRJ + 14);
				maj_niv (6, 9, 3);
			}
			else if (c == Non)
			{
				maj_niv (6, 0, 0);
				prompt (pvoie->finf.flags, pvoie->niv1);
			}
			else
			{
				texte (T_ERR + 0);
				texte (T_QST + 4);
			}
		}
		else
		{						/* Suppression */
			if (c == Non)
			{
				texte (T_TRJ + 14);
				maj_niv (6, 9, 3);
			}
			else if (c == Oui)
			{
				/* Supprime le record */
				supprime_sat ();
				maj_niv (6, 0, 0);
				prompt (pvoie->finf.flags, pvoie->niv1);
			}
			else
			{
				texte (T_ERR + 0);
				texte (T_QST + 1);
			}
		}
		break;
	case 3:
		lit_sat (&bufsat);
		i = 0;
		bufsat.maj = time (NULL);
		if (ISPRINT (*indd))
		{
			while (ISGRAPH (*indd))
				bufsat.dd[i++] = toupper (*indd++);
			bufsat.dd[i] = '\0';
		}
		ecrit_sat (&bufsat);
		texte (T_TRJ + 15);
		maj_niv (6, 9, 4);
		break;
	case 4:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.k0 = atol (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 16);
		maj_niv (6, 9, 5);
		break;
	case 5:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.y3 = atoi (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 17);
		maj_niv (6, 9, 6);
		break;
	case 6:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.d3 = atof (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 18);
		maj_niv (6, 9, 7);
		break;
	case 7:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.m0 = atof (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 19);
		maj_niv (6, 9, 8);
		break;
	case 8:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.w0 = atof (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 20);
		maj_niv (6, 9, 9);
		break;
	case 9:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.o0 = atof (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 21);
		maj_niv (6, 9, 10);
		break;
	case 10:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.i0 = atof (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 22);
		maj_niv (6, 9, 11);
		break;
	case 11:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.e0 = atof (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 23);
		maj_niv (6, 9, 12);
		break;
	case 12:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
		{
			bufsat.n0 = atof (indd);
			bufsat.a0 = 0.0;
		}
		ecrit_sat (&bufsat);
		texte (T_TRJ + 24);
		maj_niv (6, 9, 13);
		break;
	case 13:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.q3 = atof (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 25);
		maj_niv (6, 9, 14);
		break;
	case 14:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
			bufsat.pas = atoi (indd);
		ecrit_sat (&bufsat);
		texte (T_TRJ + 26);
		maj_niv (6, 9, 15);
		break;
	case 15:
		lit_sat (&bufsat);
		if (ISGRAPH (*indd))
		{
			bufsat.f1 = atof (indd);
			bufsat.v1 = 0.0;
		}
		ecrit_sat (&bufsat);
		texte (T_QST + 2);
		maj_niv (6, 0, 0);
		prompt (pvoie->finf.flags, pvoie->niv1);
		break;
	default:
		fbb_error (ERR_NIVEAU, "MODIF-SAT", pvoie->niv3);
		break;
	}
}


static void supprime_sat (void)
{
	FILE *fptr;
	FILE *tptr;
	satel bufsat;
	char temp[128];
	long pos = 0L;

	if ((tptr = fopen (temp_name (voiecur, temp), "wb")) == NULL)
		return;

	if ((fptr = fopen (d_disque ("SAT\\SATEL.DAT"), "rb")) != NULL)
	{
		while (fread ((char *) &bufsat, sizeof (satel), 1, fptr))
		{
			if (pos != pvoie->enrcur)
				fwrite ((char *) &bufsat, sizeof (satel), 1, tptr);
			pos += (long) sizeof (satel);
		}
		ferme (fptr, 27);
	}
	ferme (tptr, 28);
	rename_temp (voiecur, d_disque ("SAT\\SATEL.DAT"));		/* Le fichier est mis en place */
}


static void ecrit_sat (satel * bufsat)
{
	FILE *fptr;

	fptr = ouvre_sat ();
	fseek (fptr, pvoie->enrcur, 0);
	fwrite ((char *) bufsat, sizeof (*bufsat), 1, fptr);
	ferme (fptr, 27);
}


void param_satel (void)
{
	int i;
	satel bufsat;
	FILE *fptr;

	switch (pvoie->niv3)
	{
	case 0:
		if ((i = selection_sat ()) == -1)
			break;
		else
		{
			fptr = ouvre_sat ();
			pvoie->enrcur = (long) sizeof (bufsat) * (long) i;
			fflush (fptr);
			if ((i < 0) || (pvoie->enrcur >= filelength (fileno (fptr))))
			{
				ferme (fptr, 28);
				texte (T_ERR + 0);
				menu_sat ();
			}
			else
			{
				fseek (fptr, pvoie->enrcur, 0);
				fread ((char *) &bufsat, sizeof (bufsat), 1, fptr);
				ferme (fptr, 29);
				if (*(bufsat.dd))
				{
					affiche_param (&bufsat);
					retour_menu (N_TRAJ);
				}
				else
				{
					texte (T_ERR + 3);
					menu_sat ();
				}
			}
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "PARAM-SAT", pvoie->niv3);
		break;
	}
}


static void affiche_param (satel * bufsat)
{
	lit_sat (bufsat);
	vis_param (bufsat);
}


static void vis_param (satel * bufsat)
{
	var_cpy (0, bufsat->dd);
	sprintf (varx[1], "%ld", bufsat->cat);
	texte (T_TRJ + 27);
	sprintf (varx[0], "%5ld", bufsat->k0);
	texte (T_TRJ + 28);
	sprintf (varx[0], "%5d", bufsat->y3);
	texte (T_TRJ + 29);
	sprintf (varx[0], "%14.8f", bufsat->d3);
	texte (T_TRJ + 30);
	sprintf (varx[0], "%14.8f", bufsat->m0);
	texte (T_TRJ + 31);
	sprintf (varx[0], "%14.8f", bufsat->w0);
	texte (T_TRJ + 32);
	sprintf (varx[0], "%14.8f", bufsat->o0);
	texte (T_TRJ + 33);
	sprintf (varx[0], "%14.8f", bufsat->i0);
	texte (T_TRJ + 34);
	sprintf (varx[0], "%14.8f", bufsat->e0);
	texte (T_TRJ + 35);
	sprintf (varx[0], "%14.8f", bufsat->n0);
	texte (T_TRJ + 36);
	sprintf (varx[0], "%14.8f", bufsat->q3);
	texte (T_TRJ + 37);
	sprintf (varx[0], "%5d", bufsat->pas);
	texte (T_TRJ + 38);
}


void lit_sat (satel * bufsat)
{
	FILE *fptr;

	fptr = ouvre_sat ();
	fseek (fptr, pvoie->enrcur, 0);
	fread ((char *) bufsat, sizeof (*bufsat), 1, fptr);
	ferme (fptr, 26);
}


int test_date (char *ptr)
{
	char *sptr = ptr;
	int j, m, a;
	struct tm *sdate;
	long temps = time (NULL);

	sdate = gmtime (&temps);

	if (!ISGRAPH (*ptr))
	{
		sprintf (ptr, "%02d/%02d/%02d", sdate->tm_mday, sdate->tm_mon + 1, sdate->tm_year % 100);
		outln (ptr, strlen (ptr));
	}

	if (!isdigit (*ptr))
		return (FALSE);
	while (isdigit (*ptr))
		++ptr;
	if ((!*ptr) || (isalnum (*ptr)))
		return (FALSE);
	*ptr++ = ' ';
	if (!isdigit (*ptr))
		return (FALSE);
	while (isdigit (*ptr))
		++ptr;
	if ((!*ptr) || (isalnum (*ptr)))
		return (FALSE);
	*ptr++ = ' ';
	if (!isdigit (*ptr))
		return (FALSE);
	while (isdigit (*ptr))
		++ptr;
	if (!ISGRAPH (*ptr))
	{
		sscanf (sptr, "%d %d %d", &j, &m, &a);
		if ((j < 1) || (j > 31))
			return (FALSE);
		if ((m < 1) || (m > 12))
			return (FALSE);
		if ((a > 99) && (a < 1980 || a > 2030))
			return (FALSE);
		return (TRUE);
	}
	return (FALSE);
}


int test_heure (char *ptr)
{
	char *sptr = ptr;
	int h, m;
	struct tm *sdate;
	long temps = time (NULL);

	sdate = gmtime (&temps);

	if (!ISGRAPH (*ptr))
	{
		sprintf (ptr, "%02d:%02d", sdate->tm_hour, sdate->tm_min);
		outln (ptr, strlen (ptr));
	}

	if (!isdigit (*ptr))
		return (FALSE);
	while (isdigit (*ptr))
		++ptr;
	if ((!*ptr) || (isalnum (*ptr)))
		return (FALSE);
	*ptr++ = ' ';
	if (!isdigit (*ptr))
		return (FALSE);
	while (isdigit (*ptr))
		++ptr;
	if (!ISGRAPH (*ptr))
	{
		sscanf (sptr, "%d %d", &h, &m);
		if ((h < 0) || (h > 23))
			return (FALSE);
		if ((m < 0) || (m > 59))
			return (FALSE);
		return (TRUE);
	}
	return (FALSE);
}


void menu_sat (void)
{
	int i = 0;
	satel bufsat;
	FILE *fptr;
	char satstr[80];
	FILE *fptr2;

	cr ();
	fptr = ouvre_sat ();
	while (fread ((char *) &bufsat, sizeof (bufsat), 1, fptr))
	{
		sprintf (varx[0], "%3d", i++);
		sprintf (varx[1], "%-17s", bufsat.dd);
		sprintf (satstr, "sat\\%ld.sat", bufsat.cat);
		if ((fptr2 = fbb_fopen( d_disque (satstr),"r+b")) != NULL)
		{
			strcat (varx[0]," *");
		} else
        	strcat (varx[0],"  ");

		texte (T_TRJ + 39);
		if ((i % 3) == 0)
			cr_cond ();
	}
	ferme (fptr, 30);
	if (i % 3)
		cr_cond ();
	texte (T_TRJ + 40);
	texte (T_QST + 5);
}


int selection_sat (void)
{
	int i;

	switch (toupper (*indd))
	{
	case '\0':
		texte (T_QST + 5);
		return (-1);
	case 'L':
	case 'W':
		menu_sat ();
		return (-1);
	case 'F':
		maj_niv (pvoie->niv1, 0, 0);
		prompt (pvoie->finf.flags, pvoie->niv1);
		return (-1);
	default:
		if (isdigit (*indd))
		{
			i = atoi (indd);
			return (i);
		}
		texte (T_ERR + 0);
		texte (T_QST + 5);
		return (-1);
	}
}


void trajec (void)
{
	switch (pvoie->niv2)
	{
	case 0:
		menu_trajec ();
		break;
	case 1:
		trajecto ();
		break;
	case 2:
		param_satel ();
		break;
	case 3:
		carac_satel ();
		break;
	case 9:
		modif_satel ();
		break;
	default:
		fbb_error (ERR_NIVEAU, "TRAJEC", pvoie->niv2);
		break;
	}
}
