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
 * MBL_LOG.C
 *
 */

#include <serv.h>

void ouvre_log (void)
{
	long temps;
	struct tm *sdate;
	char fichier[80];

	temps = time (NULL);
	sdate = localtime (&temps);
	if (num_semaine == 53) num_semaine = 1;
	sprintf (fichier, "LOG\\FBBLOG%02d.%02d", sdate->tm_year %100, num_semaine);
	log_ptr = fappend (d_disque (fichier), "b");
}


void ferme_log (void)
{
	if (log_ptr)
	{
		fclose (log_ptr);
		log_ptr = NULL;
	}
}
