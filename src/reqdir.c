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
 * REQDIR.C       Server example.
 *
 * Version 1.1  01/01/92
 * Version 1.2  05/05/92
 * Version 1.3  05/23/92
 *
 *
 * This server answers to a message like this :
 *
 *  SP REQDIR < FC1EBN
 *  *.ZIP @ F6ABJ
 *  Text is not necessary
 *  /EX
 *
 * by a message like this
 *
 *  SP FC1EBN @ F6ABJ < F6FBB
 *  Req Dir : *.ZIP
 *  FBB.ZIP  etc.....
 *  /EX
 *
 * ============================================
 * The server must APPEND its answer to MAIL.IN
 * file to avoid destroying existing mail.
 * ============================================
 *
 * The server receives from FBB software 1 argument :
 *
 * argv[1] = Name of the file including the message received from FBB software.
 *
 * As this server opens the INIT.SRV file, it must be in the same directory.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#include <fbb_conf.h>

#define MAX_DISK 8

void send_dir (FILE * fptr, char *base_dir, char *dir, int disk, int avail_disk[]);

char *back2slash (char *str)
{
	static char buf[256];
	char *ptr = buf;
	int nb = 0;

	if ((strlen (str) > 2) && (str[1] == ':'))
		str += 2;

	while (*str)
	{
		if (*str == '\\')
			*ptr = '/';
		else if (isupper (*str))
			*ptr = tolower (*str);
		else
			*ptr = *str;
		++ptr;
		++str;
		if (++nb == 255)
			break;
	}
	*ptr = '\0';
	return (buf);
}

int main (int argc, char **argv)
{
	int i;
	int disk = 2;
	int avail_disk[MAX_DISK];

	FILE *fptr;
	char *ptr;
	char *dir;
	char buffer[128];
	char sender[80];
	char route[80];
	char path[80];
	char bbs_call[80];
	char base_dir[80];
	char mail_in[80];

	if (argc != 2)
		exit (1);				/* Check the arguments */

	dir = path;

	fptr = fopen (back2slash (argv[1]), "r");	/* Open the received message */
	if (fptr == NULL)
		exit (1);

	fgets (buffer, 80, fptr);	/* Read the command line */
	sscanf (buffer, "%*s %*s %*s %s\n", sender);

	*dir = *route = '\0';
	fgets (buffer, 80, fptr);	/* Read the subject */

	i = 0;
	ptr = buffer;
	while (*ptr)
	{							/* No spaces */
		if (!isspace (*ptr))
		{
			if (isupper (*ptr))
				buffer[i] = tolower (*ptr);	/* Capitalise */
			++i;
		}
		++ptr;
	}
	buffer[i] = '\0';

	sscanf (buffer, "%[^@]%s", dir, route);		/* Scan dir and route */

	fclose (fptr);				/* All needed is read */
	unlink (back2slash(argv[1]));

	if (dir[1] == ':')
	{							/* Checks asked disk */
		disk = toupper (dir[0]) - 'A';
		dir += 2;
	}

	if ((*dir == '\\') || (*dir == '/'))
		++dir;

	if (*dir == '\0')
		strcpy (dir, "*.*");	/* All files ? */

	*base_dir = '\0';

	if (read_fbb_conf(NULL) > 0)
		exit (1);

	ptr = find_fbb_conf("call", 0);
	if (ptr == NULL)
		exit (1);				/* and users base directory */
	sscanf (ptr, "%[0-9A-Za-z]", bbs_call);	/* Callsign */

	ptr = find_fbb_conf("fbbd", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("fbbd");
	if (ptr == NULL)
		exit (1);				/* and users base directory */
	ptr = strtok (ptr, ", \n");	/* Users directory */
	for (i = 0; i < MAX_DISK; i++)
	{					/* selects asked disk path */
		if ((i == disk) && (ptr) && (*ptr != '*'))
			strcpy (base_dir, ptr);
		avail_disk[i] = ((ptr) && (*ptr != '*'));
		ptr = strtok (NULL, ", \n");
	}

	ptr = find_fbb_conf("impo", 0);
	if (ptr == NULL)
		ptr = def_fbb_conf("impo");
	if (ptr == NULL)
		exit (1);				/* and users base directory */
	sscanf (ptr, "%s", mail_in);		/* Mail in file */

	/* Append the answer to mail in file */

	if ((fptr = fopen (back2slash (mail_in), "a")) != NULL)
	{
		fprintf (fptr, "#\r\n");	/* Tell that this is a message from this BBS */
		fprintf (fptr, "SP %s %s < %s\r\n", sender, route, bbs_call);	/* Command line */
		send_dir (fptr, base_dir, dir, disk, avail_disk);
		fprintf (fptr, "\r\n/EX\r\n");
		fclose (fptr);
	}
	exit (0);
}

int points (char *ptr)			/* Looks for a ".." sequence in the path */
{
	while (*ptr)
	{
		if ((*ptr == '.') && (*(ptr + 1) == '.'))
			return (1);
		++ptr;
	}
	return (0);					/* ".." not fond ! */
}

char *strupr (char *str)
{
	char *tmp = str;

	while (*tmp)
	{
		if (islower (*tmp))
			*tmp = toupper (*tmp);
		++tmp;
	}
	return str;
}

int is_dir (char *path)
{
	struct stat st;

	if (stat (path, &st) != -1)
		return ((st.st_mode & S_IFDIR) != 0);
	return 0;
}

int strmatch (char *chaine, char *masque)
{
	while (1)
	{
		switch (*masque)
		{
		case '\0':
			return (toupper (*masque) == toupper (*chaine));
		case '&':
			if ((*chaine == '\0') || (*chaine == '.'))
				return (1);
			break;
		case '?':
			if (!isalnum (*chaine))
				return (0);
			break;
		case '#':
			if ((*chaine != '#') && (!isdigit (*chaine)))
				return (0);
			break;
		case '@':
			if (!isalpha (*chaine))
				return (0);
			break;
		case '=':
			if (!isgraph (*chaine))
				return (0);
			break;
		case '*':
			while (*++masque == '*')
				;
			if (*masque == '\0')
				return (1);
			while (!strmatch (chaine, masque))
				if (*++chaine == '\0')
					return (0);
			break;
		default:
			if ((toupper (*chaine)) != (toupper (*masque)))
				return (0);
			break;
		}
		++chaine;
		++masque;
	}
}

void send_dir (FILE * fptr, char *base_dir, char *dir, int disk, int avail_disk[])
{
	int i;
	int found = 0;
	char *ptr;
	char path[256];
	char filename[256];
	char mask[10];

	sprintf (path, "%s/%s", base_dir, dir);	/* Complete path */

	ptr = path;
	while (*ptr)
	{
		if (*ptr == '\\')
			*ptr = '/';
		else if (isupper (*ptr))
			*ptr = tolower (*ptr);
		++ptr;
	}

	if (is_dir (path))
	{
		strcpy (mask, "*");
		strcat (dir, "\\*.*");
	}
	else
	{
		ptr = strrchr (path, '/');
		if (ptr)
		{
			*ptr++ = '\0';
			strcpy (mask, ptr);
			if (*path == '\0')
				strcpy (path, "/");
		}
	}

	if (strcmp (mask, "*.*") == 0)
		strcpy (mask, "*");

	fprintf (fptr, "ReqDir %c:\\%s\r\n", disk + 'A', dir);	/* Subject */

	fprintf (fptr, "\r\nReqDir V 1.5 for LINUX (C) F6FBB 1996-2000\r\nVolume %c: - Path \\%s\r\n\r\n", disk + 'A', dir);		/* Subject */

	fprintf (fptr, "Available volumes: ");

	for (i = 0; i < MAX_DISK; i++)
		if (avail_disk[i])
			fprintf (fptr, "%c: ", i + 'A');
	fprintf (fptr, "\r\n\r\n");


	if (*base_dir)
	{
		struct dirent *dirent;
		DIR *dir;
		struct stat st;

		dir = opendir (path);
		if (dir)
		{
			while ((dirent = readdir (dir)) != NULL)
			{
				if (*dirent->d_name != '.')
				{
					found = 1;
					sprintf (filename, "%s/%s", path, dirent->d_name);
					if (stat (filename, &st) == -1)
						continue;
					if (st.st_mode & S_IFDIR)
						fprintf (fptr, "       <DIR>  %s\n", strupr (dirent->d_name));
					else if (strmatch(dirent->d_name, mask))
						fprintf (fptr, "%12ld  %s\n", st.st_size, strupr (dirent->d_name));
				}
			}
		}

		if (!found)
			fprintf (fptr, "No file !\r\n");
	}
	else
		fprintf (fptr, "Volume %c: does not exist !\r\n", disk + 'A');
}
