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

#define ENGLISH

/*

 *  This program returns values from the fbb configuration values
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef __linux__
#include <stdlib.h>
#include <unistd.h>
#else
#include <alloc.h>
#include <io.h>
#endif

#include <fbb_conf.h>
#include <config.h> 

char *conffile = NULL;

void usage(void);

void usage()
{
	fprintf(stderr, "\nfbbgetconf v%s\n", VERSION);
	fprintf(stderr, "Usage : fbbgetconf [-a] [-d] [-h] [-f configfile] [key] [key] ...\n");
}

int main(int ac, char **av)
{
	int c = 0;
	int list_all = 0;
	int list_def = 0;

	while ((c = getopt(ac, av, "adhf:")) != -1)
	{
		switch (c)
		{
		case 'a':
			list_all = 1;
			break;

		case 'd':
			list_def = 1;
			break;

		case 'f':
			conffile = optarg;
			break;

		case 'h':
			usage();
			exit(0);

		}
	}

	if (ac == 1)
	{
		usage();
		exit(0);
	}

	if (read_fbb_conf(conffile) > 0)
	{
#ifdef ENGLISH
		fprintf(stderr, "Cannot open fbb configuration file       \n");
#else
		fprintf(stderr, "Erreur ouverture fichier de configuration\n");
#endif

		exit(1);				/* and users base directory */
	}

	if (list_all)
	{
		char *ptr;
		
		while ((ptr = get_fbb_all(1)) != NULL)
		{
			printf("%s\n", ptr);
			free(ptr);
		}

		return 0;
	}
	
	if (list_def)
	{
		char *ptr;
		
		while ((ptr = get_fbb_def(1)) != NULL)
		{
			printf("%s\n", ptr);
			free(ptr);
		}

		return 0;
	}
	
	while (optind < ac)
	{
		char *key = av[optind];
		char *ptr;
		
		ptr = find_fbb_conf(key, 0);
		if (ptr == NULL)
			ptr = def_fbb_conf(key);
		if (ptr == NULL)
		{
#ifdef ENGLISH
			fprintf(stderr, "Unknown keyword %s\n", key);
#else
			fprintf(stderr, "Mot cl� %s inconnu\n", key);
#endif
			exit(1);
		}
		printf("%s\n", ptr);
		free(ptr);

		while (ptr)
		{
			ptr = find_fbb_conf(key, 1);
			if (ptr)
			{
				printf("%s\n", ptr);
				free(ptr);
			}
		}
		
		++optind;
	}

	return 0;
}
