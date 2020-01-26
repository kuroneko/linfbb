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
 * MBL_PRN.C
 */

#include <serv.h>

#ifdef __FBBDOS__
static char print_name[128];
static int print_string (char *, FILE *);
#endif

static int open_print (void);
static int print_mess (int, bullist *);

#ifdef __FBBDOS__
static void print_file (char *ptr);

#endif


int mbl_print (void)
{
	bullist *pbul;
	long no;
	int ok = 0;
	int verbose = 0;
	int old_print = print;

	if (voiecur != CONSOLE)
		return (0);

	incindd ();

	if (!ISGRAPH (*indd))
	{
		if (print)
		{
			close_print ();
		}
		else
		{
			open_print ();
		}
		return (1);
	}


#ifdef __FBBDOS__
	if (*indd == '>')
	{
		incindd ();
		print_file (indd);
		return (1);
	}
#endif

	if (toupper (*indd) == 'V')
	{
		verbose = 1;
		incindd ();
	}

	if (!print)
		open_print ();

	while ((no = lit_chiffre (1)) != 0L)
	{
		ok = 1;
		if ((pbul = ch_record (NULL, no, '\0')) != NULL)
		{
			if (!print_mess (verbose, pbul))
				break;
		}
		else
			texte (T_ERR + 10);

	}

	if (!old_print)
		close_print ();

	return (ok);
}

static int print_mess (int verbose, bullist * pbul)
{
#ifdef __FBBDOS__
	char *ptr;
	FILE *fptr;
	int fd;
	int c, first = 1;
	int nb = 0;
	int call = 0;
	char ligne[90];
	char chaine[256];
	int flag = FALSE;
	long record = 0L;
	short postexte = 0;

	trait_time = 0;

	*ptmes = *pbul;

	if (*ptmes->bbsv)
		sprintf (varx[0], "@%-6s  ", ptmes->bbsv);
	else
		*varx[0] = '\0';


	fputc ('\r', file_prn);
	if (!print)
		return (0);

	fputc ('\n', file_prn);
	if (!print)
		return (0);

	ptr = expand (langue[vlang]->plang[T_MBL + 35 - 1]);
	if (!print_string (ptr, file_prn))
		return (0);

	ptr = expand (langue[vlang]->plang[T_MBL + 38 - 1]);
	if (!print_string (ptr, file_prn))
		return (0);

	if ((fptr = fopen (mess_name (MESS, ptmes->numero, chaine), "rb")) == NULL)
		return (0);
	if (!verbose)
	{
		/* fseek(fptr, supp_header(fptr, 0), 0) ; */

		ptr = ligne;

		while ((c = fgetc (fptr)) != EOF)
		{
			if ((flag) && (c == '\n'))
			{
				record = ftell (fptr);
				postexte = 0;
				call = 0;
				flag = FALSE;
			}
			else
			{
				switch (call)
				{
				case 0:
					break;
				case 1:
					if (isalnum (c))
					{
						*ptr++ = c;
						nb++;
						call = 2;
					}
					break;
				case 2:
					if (isalnum (c))
					{
						*ptr++ = c;
						nb++;
					}
					else
					{
						*ptr++ = '!';
						nb++;
						call = 0;
						if (nb >= 65)
						{
							*ptr++ = '\r';
							*ptr++ = '\n';
							nb += 2;
							fflush (file_prn);
							write (fileno (file_prn), ligne, nb);
							nb = 0;
							ptr = ligne;
							first = 2;
						}
					}
					break;
				}
				if (postexte == 0)
				{
					if (c != 'R')	/*return(record) */
						break;
					else
						flag = TRUE;
				}
				if ((postexte == 1) && (flag) && (c != ':'))	/*return(record) */
					break;
				++postexte;
			}
			if ((flag) && (c == '@'))
			{
				if (first)
				{
					fflush (file_prn);
					if (first == 1)
					{
						write (fileno (file_prn), "Path: !", 7);
					}
					else
					{
						write (fileno (file_prn), "      !", 7);
					}
					first = 0;
				}
				call = 1;
			}
		}
		if (nb)
		{
			*ptr++ = '\r';
			*ptr++ = '\n';
			nb += 2;
			fflush (file_prn);
			write (fileno (file_prn), ligne, nb);
		}

	}

	if (!print)
		return (0);

	fseek (fptr, record, 0);

	fflush (fptr);
	fflush (file_prn);
	fd = fileno (fptr);
	copy_fic (fd, fileno (file_prn), NULL);

	fclose (fptr);

#endif
	return (1);
}

#ifdef __FBBDOS__
static int print_string (char *ptr, FILE * fptr)
{
	while (*ptr)
	{
		if (*ptr == '\n')
		{
			fputc ('\r', fptr);
			if (!print)
				return (0);
		}
		fputc (*ptr, fptr);
		if (!print)
			return (0);
		++ptr;
	}
	return (1);
}
#endif

void init_print (void)
{
#ifdef __FBBDOS__
	file_prn = stdprn;
	strcpy (print_name, "PRN:");
#endif
}

#ifdef __FBBDOS__
static void print_file (char *ptr)
{
	char old[128];
	char s[256];
	int ok = print;

	sup_ln (ptr);

	if (*ptr)
	{

		strcpy (old, print_name);

		close_print ();

		if (*ptr == '-')
		{
			file_prn = stdprn;
			strcpy (print_name, "PRN:");
		}
		else
		{
			strcpy (print_name, strupr (ptr));
		}

		ok = open_print ();
		print = 0;
		if (!ok)
		{

#ifdef ENGLISH
			sprintf (s, "Error printing to %s    ", print_name);
#else
			sprintf (s, "Erreur impression sur %s", print_name);
#endif
			strcpy (print_name, old);

		}
	}

#ifdef ENGLISH
	sprintf (s, "Printing to %s   ", print_name);
#else
	sprintf (s, "Impression sur %s", print_name);
#endif
	outln (s, strlen (s));

	print = ok;

}
#endif

void close_print (void)
{
	print = 0;

#if defined( __WINDOWS__) || defined(__linux__)
	SpoolLine (0, 0, NULL, 0);
#endif
#ifdef __FBBDOS__
	if ((file_prn != stdprn) && (file_prn))
	{
		fclose (file_prn);
	}

	file_prn = NULL;
	trait (0, "");
#endif
}

static int open_print (void)
{
#ifdef __linux__
	print = 1;
#endif
#ifdef __WINDOWS__
	print = 1;
#endif
#ifdef __FBBDOS__
	if (file_prn)
		close_print ();

	if (strcmp (print_name, "PRN:") == 0)
	{
		print = 1;
		file_prn = stdprn;
	}
	else if ((file_prn = fopen (print_name, "ab")) != NULL)
	{
		print = 1;
	}

	trait (0, "");

#endif
	return (print);
}
