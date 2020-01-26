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

 * Generation des messages WP vers le reseau.
 *
 * Les routes a generer sont definies dans INIT.SRV
 *
 */

#include <serv.h>

static int copy_lines (int fd_orig, int fd_dest)
{
#define TAIBUF 5000

	int nb_lus;
	char c;
	char *buffer;

	buffer = m_alloue (TAIBUF);

	nb_lus = read (fd_orig, buffer, TAIBUF);
	if (nb_lus > 0)
		write (fd_dest, buffer, nb_lus);

	if (nb_lus == TAIBUF)
	{
		/* Termine la ligne en cours */
		do
		{
			if (read (fd_orig, &c, 1) > 0)
			{
				write (fd_dest, &c, 1);
				++nb_lus;
			}
			else
				break;
		}
		while (c != '\n');
	}

	m_libere (buffer, TAIBUF);

	return (nb_lus);
}


void send_wp_mess (void)
{
	int fd_orig;
	int fd_dest;
	int sav_voie = voiecur;
	int sav_lang = vlang;
	int mess;
	char route[80];
	char *ptr;
	char *scan;
	FILE *fptr;

#ifndef R_OK
#define R_OK	004
#define W_OK	002
#endif

	if (access (d_disque ("WP\\MESS.WP"), R_OK | W_OK) == -1)
		return;

	if (voiecur == MWARNING)
		return;					/* Deja en warning */

	if (*wp_line == '\0')
	{
		unlink (d_disque ("WP\\MESS.WP"));
		return;
	}

	if ((fd_orig = open (d_disque ("WP\\MESS.WP"), O_RDONLY | O_BINARY)) == EOF)
	{
		return;
	}

	/* On ouvre le fichier MAIL.IN */
	fptr = fappend (MAILIN, "b");
	if (fptr == NULL)
		return;

	selvoie (MWARNING);
	if (FOR (svoie[sav_voie]->mode))
		pvoie->mode |= F_FOR;
	status (voiecur);


	mess = 0;

	for (;;)
	{
#if defined(__WINDOWS__) || defined(__linux__)
		char txt[80];

#endif
		int nb;
		int fd;

		/* Boucle sur des messages de 5K Max */

		if ((fd_dest = open ("TEMP.WP", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == EOF)
		{
			close (fd_orig);
			break;
		}

		nb = copy_lines (fd_orig, fd_dest);

		close (fd_dest);

#if defined(__WINDOWS__) || defined(__linux__)
		wsprintf (txt, "Preparing Message #%d", ++mess);
		InfoMessage (-1, txt, "WP-Messages");
#endif

		if (nb <= 0)
		{
			close (fd_orig);
			break;
		}

		scan = wp_line;
		while ((*scan) && !(ISGRAPH (*scan)))
			++scan;
		ptr = route;
		while ((*scan) && (ISGRAPH (*scan)))
			*ptr++ = *scan++;
		*ptr++ = '\0';

		/* On ouvre le fichier tout neuf pour le copier ... */
		fd = open ("TEMP.WP", O_RDONLY | O_BINARY);

		while ((fd != -1) && (*route))
		{
#if defined(__WINDOWS__) || defined(__linux__)
			wsprintf (txt, "Creating Message #%d (%s)", mess, route);
			InfoMessage (-1, txt, NULL);
#endif
			/* Genere le message via route ... */
			fprintf (fptr, "#\r\nS%c WP @ %s < %s\r\nWP Update\r\n",
					 find (bbs_via (route)) ? 'P' : 'B', route, mycall);
			fflush (fptr);

			/* On remet a zero pour la nouvelle copie ... */
			lseek (fd, 0L, SEEK_SET);

			fflush (fptr);
			copy_fic (fd, fileno (fptr), NULL);
			fflush (fptr);

			fprintf (fptr, "\r\n/EX\r\n");
			fflush (fptr);
			
			while ((*scan) && !(ISGRAPH (*scan)))
				++scan;
			ptr = route;
			while ((*scan) && (ISGRAPH (*scan)))
				*ptr++ = *scan++;
			*ptr++ = '\0';
		}
		close (fd);
	}

	/* On ferme le MAIL.IN */
	fclose (fptr);

#if defined(__WINDOWS__) || defined(__linux__)
	InfoMessage (-1, "Delete MESS.WP file", NULL);
#endif
	unlink (d_disque ("WP\\MESS.WP"));
	unlink ("TEMP.WP");
	selvoie (sav_voie);
	vlang = sav_lang;
#if defined(__WINDOWS__) || defined(__linux__)
	InfoMessage (-1, NULL, NULL);
#endif
}
