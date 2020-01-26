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
 * Module commande LC
 */

#define NB_THEMES	1000

static void list_themes (void);

typedef struct theme
{
	char nom[6];
	int nb;
	struct theme *suiv;
}
Theme;


int list_lc (void)
{
	int error = 0;

	sup_ln (indd);
/************** Tester l'espace ******************/
	incindd ();

	if ((*indd == ';') || (*indd == '?'))
		list_themes ();

	else
	{
		if (*indd)
		{
			strn_cpy (6, pvoie->finf.filtre, indd);
			if (strcmp (pvoie->finf.filtre, "*") == 0)
			{
				*pvoie->finf.filtre = '\0';
			}
		}
		outln ("=> $l", 5);
	}

	retour_mbl ();
	return (error);
}

static void list_themes (void)
{
	char s[30];
	char temp[30];
	Theme *tptr = NULL;
	Theme *cptr;
	Theme *sptr;
	Theme *prec;
	int nb;
	int comp;
	unsigned offset;
	bloc_mess *bptr = tete_dir;
	bullist ligne;

	ouvre_dir ();

	offset = 0;
	while (bptr)
	{
		if (bptr->st_mess[offset].noenr)
		{
			read_dir (bptr->st_mess[offset].noenr, &ligne);

			if ((ligne.type == 'B') && (droit_ok (&ligne, 1)))
			{
				if (tptr == NULL)
				{
					tptr = (Theme *) m_alloue (sizeof (Theme));
					tptr->suiv = NULL;
					strncpy (tptr->nom, ligne.desti, 6);
					tptr->nb = 1;
				}
				else
				{
					prec = NULL;
					sptr = tptr;
					for (;;)
					{
						if (sptr == NULL)
						{
							cptr = (Theme *) m_alloue (sizeof (Theme));
							cptr->suiv = NULL;
							strncpy (cptr->nom, ligne.desti, 6);
							cptr->nb = 1;
							if (prec)
								prec->suiv = cptr;
							else
								tptr = cptr;
							break;
						}
						comp = strncmp (sptr->nom, ligne.desti, 6);
						if (comp == 0)
						{
							++sptr->nb;
							break;
						}
						if (comp > 0)
						{
							cptr = (Theme *) m_alloue (sizeof (Theme));
							strncpy (cptr->nom, ligne.desti, 6);
							cptr->nb = 1;
							if (prec)
								prec->suiv = cptr;
							else
								tptr = cptr;
							cptr->suiv = sptr;
							break;
						}
						prec = sptr;
						sptr = sptr->suiv;
					}
				}
			}
		}

		if (++offset == T_BLOC_MESS)
		{
			bptr = bptr->suiv;
			offset = 0;
		}

	}

	ferme_dir ();

	nb = 0;
	cptr = tptr;
	while (cptr)
	{
		strn_cpy (6, temp, cptr->nom);
		sprintf (s, "%-6s %d          ", temp, cptr->nb);
		out (s, 13);
		if (++nb == 6)
		{
			cr ();
			nb = 0;
		}
		prec = cptr;
		cptr = cptr->suiv;
		m_libere (prec, sizeof (Theme));
	}

	if (nb != 0)
		cr ();

}
