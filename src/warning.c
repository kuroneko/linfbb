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

/* Module gerant les messages "warning" */




/*
 * Demande de message warning :
 *
 * 1 - PingPong
 * 2 - Route inconnue
 * 3 - NTS inconnu
 *
 */

void dde_warning (int type)
{
	pvoie->warning |= type;
}

void tst_warning (bullist * ptmes)
{
	char wtexte[256];
	char nom[128];
	unsigned warning = pvoie->warning;

	while (warning)
	{
		if (warning & W_PPG)
		{
#ifdef ENGLISH
			sprintf (wtexte,
					 "Duplicate reverse forward of # %ld to %s, route to %s",
					 ptmes->numero, ptmes->desti, ptmes->bbsv
				);
			if (w_mask & W_PINGPONG)
				mess_warning (admin, "*** PING-PONG ***", wtexte);
#else
			sprintf (wtexte,
					 "Presentation en retour du # %ld a %s, routage vers %s",
					 ptmes->numero, ptmes->desti, ptmes->bbsv
				);
			if (w_mask & W_PINGPONG)
				mess_warning (admin, "*** PING-PONG ***", wtexte);
#endif
			warning &= ~(W_PPG);
		}
		else if (warning & W_ROU)
		{
#ifdef ENGLISH
			sprintf (wtexte,
					 "Unknown route to %s.   \rMessage #%ld written by %s received %s     \r",
					 ptmes->bbsv, ptmes->numero,
					 ptmes->exped, strdate (time (NULL))
				);
			if (w_mask & W_NO_ROUTE)
				mess_warning (admin, "*** UNKNOWN ROUTE ***       ", wtexte);
#else
			sprintf (wtexte,
					 "Acheminement vers %s inconnu.\rMessage %ld demande par %s recu le %s\r",
					 ptmes->bbsv, ptmes->numero,
					 ptmes->exped, strdate (time (NULL))
				);
			if (w_mask & W_NO_ROUTE)
				mess_warning (admin, "*** ACHEMINEMENT INCONNU ***", wtexte);
#endif
			warning &= ~(W_ROU);
		}
		else if (warning & W_NTS)
		{
#ifdef ENGLISH
			sprintf (wtexte,
			   "Unknown NTS %s. \rMessage #%ld written by %s received %s\r",
					 ptmes->desti, ptmes->numero,
					 ptmes->exped, strdate (time (NULL))
				);
			if (w_mask & W_NO_NTS)
				mess_warning (admin, "*** UNKNOWN NTS ***", wtexte);
#else
			sprintf (wtexte,
			   "NTS  %s inconnu.\rMessage %ld demande par %s recu le %s \r",
					 ptmes->desti, ptmes->numero,
					 ptmes->exped, strdate (time (NULL))
				);
			if (w_mask & W_NO_NTS)
				mess_warning (admin, "*** NTS INCONNU ***", wtexte);
#endif
			warning &= ~(W_NTS);
		}
		else if (warning & W_ASC)
		{
			mess_name (MESSDIR, ptmes->numero, nom);
#ifdef ENGLISH
			sprintf (wtexte,
					 "Ascii file %s not found    \r", nom
				);
			if (w_mask & W_MESSAGE)
				mess_warning (admin, "*** ASCII FILE NOT FOUND ***", wtexte);
#else
			sprintf (wtexte,
					 "Fichier ascii %s pas trouve\r", nom
				);
			if (w_mask & W_MESSAGE)
				mess_warning (admin, "*** ERREUR FICHIER ASCII ***", wtexte);
#endif
			warning &= ~(W_ASC);
		}
		else if (warning & W_BIN)
		{
			mess_name (MBINDIR, ptmes->numero, nom);
#ifdef ENGLISH
			sprintf (wtexte,
					 "Binary file %s not found     \r", nom
				);
			if (w_mask & W_MESSAGE)
				mess_warning (admin, "*** BIN FILE NOT FOUND ***", wtexte);
#else
			sprintf (wtexte,
					 "Fichier binaire %s pas trouve\r", nom
				);
			if (w_mask & W_MESSAGE)
				mess_warning (admin, "*** ERREUR FICHIER BIN ***", wtexte);
#endif
			warning &= ~(W_BIN);
		}
	}
	pvoie->warning = 0;
}

void mess_warning (char *w_desti, char *w_titre, char *w_texte)
{
	int nb, sav_voie = voiecur, sav_lang = vlang;

	if (voiecur == MWARNING)
		return;					/* Deja en warning */

	deb_io ();
	selvoie (MWARNING);
	if (FOR (svoie[sav_voie]->mode))
		pvoie->mode |= F_FOR;
	entete_saisie ();
	ini_champs (voiecur);
	strn_cpy (6, ptmes->desti, w_desti);
	strcpy (ptmes->exped, mycall);
	ptmes->type = 'P';
	swapp_bbs (ptmes);
	reacheminement ();
	strn_cpy (60, ptmes->titre, w_titre);
	pvoie->mode = F_FOR;
	nb = strlen (w_texte);
	w_texte[nb] = '\032';
	get_mess_fwd ('\0', w_texte, nb + 1, 2);
	selvoie (sav_voie);
	vlang = sav_lang;
	fin_io ();
}
