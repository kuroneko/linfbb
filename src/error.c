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
#undef fopen

/*
 * Writes an error message in ERROR.SYS file
 *
 * Do not Overlay !
 *
 */

static int error_type = 0;
static int erreur = 0;

void boot_prg (void)
{
	FILE *fichier;

	if ((fichier = fopen ("ERROR.SYS", "a+")) != NULL)
	{
#ifdef ENGLISH
		fprintf (fichier, "> *** System boot on %s *** \n", strdate (time (NULL)));
#else
		fprintf (fichier, "> *** Boot systeme le %s ***\n", strdate (time (NULL)));
#endif
		fclose (fichier);
	}
}

void err_alloc (unsigned numero)
{
	fbb_error (ERR_MEMORY, "malloc", numero);
}

static void fbb_w (unsigned type, char *texte, unsigned numero)
{
	FILE *fichier;
	int nbfiles = 0;

	static char *type_error[] =
	{
		"WARNING :",
		"FATAL   :",
		"EXCEPTION :",
	};

	static char *text_error[] =
	{
		"Cannot open file",
		"Cannot create file",
		"Cannot close file",
		"Cannot resynchronize TNC",
		"Not enough memory",
		"Wrong channel selection",
		"Wrong procedure level",
		"Cannot write to file",
		"Syntax error",
		"Error communication/TNC",
		"Pointer already allocated",
		"XMS/EMS error",
		"Divide by zero",
		"Memory exception",
	};

	if (error_type == 1)
		nbfiles = fbb_fcloseall ();

	if ((fichier = fopen ("ERROR.SYS", "a+")) != NULL)
	{
		fprintf (fichier, "> %s Station %s, Ch:%d (%d-%d-%d) %s\n  Version %s (%s)\n",
				 type_error[error_type], pvoie->sta.indicatif.call,
				 voiecur, pvoie->niv1, pvoie->niv2, pvoie->niv3,
			  strdate (time (NULL)), version(), date ());
		if (error_type == 1)
			fprintf (fichier, "  %d open files\n", nbfiles);
		fprintf (fichier, "  %s %u, errno %d=%s  %s\n\n",
				 texte, numero, erreur, strerror (erreur), text_error[type]);
/*
   #if FBB_DEBUG
   fbb_printfiles(fichier);
   if (error_type)
   {
   print_stack(numero, fichier);
   }
   #endif
 */
		fclose (fichier);
	}
}

void fbb_error (unsigned type, char *texte, unsigned numero)
{
	erreur = errno;

	error_type = 2;
	fbb_w (type, texte, numero);
	port_log (0, 0, 'S', "Q *** BBS Quit");
	ferme_log ();
#ifdef __WINDOWS__
	fbb_quit (0);
#else
	sleep (5);
	exit (0);
#endif
}

void fbb_except (unsigned type, char *texte, unsigned numero)
{
	erreur = errno;

	error_type = 1;
	port_log (0, 0, 'S', "Q *** BBS Quit");
	fbb_w (type, texte, numero);
	ferme_log ();
}

void fbb_warning (unsigned type, char *texte, unsigned numero)
{
	erreur = errno;

	error_type = 0;
	fbb_w (type, texte, numero);
}

void win_message (int temps, char *texte)
{
#ifdef __WINDOWS__
	WinMessage (temps, texte);
#endif
#ifdef __linux__
	WinMessage (temps, texte);
#endif
#ifdef __FBBDOS__
	fen *fen_ptr;

	deb_io ();
	fen_ptr = open_win (15, 7, 15 + strlen (texte) + 3, 9, INIT, "message");
	_wscroll = 0;
	cputs (texte);
	putch ('\a');
	_wscroll = 1;
	sleep_ (temps);
	close_win (fen_ptr);
	fin_io ();
#endif
}

void write_error (char *filename)
{
	int i;
	char text[256];

	sprintf (text, "Cannot access to %s", filename);

	for (i = 0; i < 5; i++)
	{
		deb_io ();
		win_message (2, text);
#ifdef __FBBDOS__
		putch ('\a');
#endif
		fin_io ();
	}
	fbb_error (ERR_WRITE, filename, 0);
}
