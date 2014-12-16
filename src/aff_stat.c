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


/* Affiche une ligne de liste message */

static char *ltitre (int mode, bullist * pbul)	/* Mode = 1 pour LS */
{
	/*  int lg = (mode) ? 80 : 36 ; */
	int lg = 80;
	static char buf[100];

	if (pvoie->typlist)
	{
		sprintf (buf, "%-12s ", pbul->bid);
		if (mode)
			strn_cpy (lg, buf + 13, pbul->titre);
		else
			n_cpy (lg, buf + 13, pbul->titre);
	}
	else
	{
		if (mode)
			strn_cpy (lg, buf, pbul->titre);
		else
			n_cpy (lg, buf, pbul->titre);
	}
	return (buf);
}


#ifndef NO_STATUS

static void aff_status (bullist * ligne)
{
	char *ptr;

	*ptmes = *ligne;
	if (*(ligne->bbsv))
		sprintf (varx[0], "@%-6s", bbs_via (ligne->bbsv));
	else
		strcpy (varx[0], "       ");
	var_cpy (1, ltitre (0, ligne));

	ptr = var_txt (langue[vlang]->plang[T_MBL + 37 - 1]);
	/*  texte(T_MBL+37) ; */
	if (strlen (ptr) > 80)
	{
		ptr[79] = '\r';
		ptr[80] = '\0';
	}
	outs (ptr, strlen (ptr));
}


#endif
