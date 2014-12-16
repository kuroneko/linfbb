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
#include <dirent.h>
#include <sys/vfs.h>

#undef stat

void deb_io (void)
{
	static long temps = 0L;
	long nt = btime ();

	if (nt > temps)
	{
	}
}

void fin_io (void)
{
}


void randomize (void)
{
	srandom (time (NULL));
}

int random_nb (int num)
{
#ifdef __LINUX__
	return (random () % num);
#endif

#if defined(__FBBDOS__) || defined(__WINDOWS__)
	return (random (num));
#endif
}

#undef filelength
long filelength (int fd)
{
	struct stat st;
	int val;

	val = fstat (fd, &st);
	if (val == -1)
		return (-1L);

	return (st.st_size);
}

int getdisk (void)
{
	/* Only C: */
	return 2;
}

char *getcurdir (int drive, char *str)
{
	char buffer[82];

	getcwd (buffer, 80);
	strcpy (str, slash2back (buffer));
	return str;
}

int is_cdir (int chr)
{
	if ((chr == '/') || (chr == ':') || (chr == '\\'))
		return (FALSE);
	return (isprint (chr));
}

int fnsplit (char *name, char *drive, char *rep, char *base, char *ext)
{
	int nb;
	int mask = 0;
	char *ptr = NULL;
	char *scan;

	if (drive)
		*drive = '\0';

	scan = strrchr (name, '/');

	if (rep)
	{
		ptr = rep;

		if (scan)
		{
			int sav = *ptr;

			*ptr = '\0';
			strcpy (rep, name);
			*ptr = sav;
		}
		else
		{
			*rep = '\0';
		}
	}

	if (base)
	{
		/* BaseName */
		ptr = base;

		if (scan)
			++scan;
		else
			scan = name;
		for (nb = 0; nb < FBB_NAMELENGTH; nb++, scan++)
		{
			if (!is_cdir (*scan))
				break;
			*ptr++ = *scan;
		}
		*ptr = '\0';
	}

	if (ext)
	{
		/* Extension */
		ptr = ext;

		/* if ((scan = strrchr (name, '.')) != NULL)
		{
			*ptr++ = *scan++;
			for (nb = 0; nb < 3; nb++, scan++)
			{
				if (!is_cdir (*scan))
					break;
				*ptr++ = (islower (*scan)) ? toupper (*scan) : *scan;
			}
		}*/
		*ptr = '\0';
	}
	return (mask);
}

int getftime (int fd, struct ftime *ft)
{
	struct stat st;
	struct tm *tm;
	int val;

	val = fstat (fd, &st);
	if (val == -1)
		return (-1);

	tm = gmtime (&st.st_mtime);

	ft->ft_tsec = tm->tm_sec / 2;
	ft->ft_min = tm->tm_min;
	ft->ft_hour = tm->tm_hour;
	ft->ft_day = tm->tm_mday;
	ft->ft_month = tm->tm_mon;
	ft->ft_year = tm->tm_year %100;

	return (0);
}

void format_ffblk (struct ffblk *blk, struct dirent *dir)
{
	int ret;
	int year;
	struct stat st;
	struct tm *tm;
	char base[FBB_NAMELENGTH+1];	/* AGAIN : array dimension was too small, */ 
	char ext[4];			/* causing stack smashing with long names in DOS menu */
	char filename[FBB_BASELENGTH+1];

	blk->ff_attrib = 0;

	if (strcmp (blk->ff_base, "/") == 0)
		sprintf (filename, "/%s", dir->d_name);
	else
		sprintf (filename, "%s/%s", blk->ff_base, dir->d_name);

	ret = lstat (filename, &st);

	if (S_ISLNK (st.st_mode))
	{
		/* printf ("link\n"); */
		blk->ff_attrib |= FA_LINK;
		ret = stat (filename, &st);
		if (S_ISDIR (st.st_mode))
		{
			blk->ff_attrib |= FA_DIREC;
		}
	}

	if (S_ISDIR (st.st_mode))
	{
		blk->ff_attrib |= FA_DIREC;
	}

	if ((st.st_mode & S_IWUSR) == 0)
		blk->ff_attrib |= FA_RDONLY;

	blk->ff_fsize = st.st_size;

	tm = gmtime (&st.st_mtime);
	blk->ff_ftime =
		(tm->tm_sec / 2) +
		(tm->tm_min << 5) +
		(tm->tm_hour << 11);
		year = tm->tm_year %100;
	blk->ff_fdate =
		(tm->tm_mday) +
		((tm->tm_mon + 1) << 5) +
		(year << 9);

	fnsplit (dir->d_name, NULL, NULL, base, ext);

	strcpy (blk->ff_name, base);
	if (*ext)
	{
		strcat (blk->ff_name, ext);
	}
}

#undef findfirst
int findfirst (char *rech, struct ffblk *blk, int mask)
{
	struct dirent *dirent;
	char *path;

	/* Deletes X: from MSDOS path */
	if ((strlen (rech) > 2) && (rech[1] == ':'))
		rech += 2;

	path = back2slash (rech);

	if (is_dir (path))
	{
		strcpy (blk->ff_mask, "*");
	}
	else
	{
		char *ptr = strrchr (path, '/');

		if (ptr)
		{
			*ptr++ = '\0';
			strcpy (blk->ff_mask, ptr);
			if (*path == '\0')
				strcpy (path, "/");
		}
		else
		{
			strcpy (blk->ff_mask, path);
			strcpy (path, ".");
		}
	}

	strcpy (blk->ff_base, path);
	blk->ff_dir = opendir (path);
	if (blk->ff_dir)
	{
		while ((dirent = readdir (blk->ff_dir)) != NULL)
		{
			if (*dirent->d_name == '.')
				continue;
			if (!strmatch (dirent->d_name, blk->ff_mask))
				continue;
			format_ffblk (blk, dirent);
			return (0);
		}
		closedir (blk->ff_dir);
		blk->ff_dir = NULL;
	}
	return (-1);
}

#undef findnext
int findnext (struct ffblk *blk)
{
	struct dirent *dirent;

	while ((dirent = readdir (blk->ff_dir)) != NULL)
	{
		if (*dirent->d_name == '.')
			continue;
		if (!strmatch (dirent->d_name, blk->ff_mask))
			continue;
		format_ffblk (blk, dirent);
		return (0);
	}
	closedir (blk->ff_dir);
	blk->ff_dir = NULL;
	return (-1);
}

unsigned long free_disk (int disk)
{
	struct statfs dfree;
	unsigned long avail;
	char *curdisk;
	char pwd[256];

	/* Chercher le disque physique correspondant */
	if (disk == 0)
	{
		curdisk = getcwd (pwd, sizeof (pwd));
	}
	else
	{
		curdisk = PATH[disk - 1];
	}

	if (statfs (curdisk, &dfree) == 0)
		avail = (unsigned long) dfree.f_bavail * (unsigned long) (dfree.f_bsize / 1024);
	else
		avail = 0L;
	return (avail);

}

/****************************************************
 * Look for an existing filename case independant 
 * matching with the given path/filename
 *
 * path is case dependant and MUST match a directory
 ****************************************************/
 
char *long_filename(char *path, char *filename)
{
	char fname[256];
	char lpath[256];
	char *ptr;
	char *scan;
	struct dirent *dirent;
	DIR *dir;
	
	strcpy(fname, back2slash(filename));
	ptr = fname;
	
	if ((*ptr == '/') || (path == NULL))
	{
		strcpy(lpath, "/");
		++ptr;
	}
	else
	{
		strcpy(lpath, back2slash(path));
		long_filename(NULL, lpath);
	}

	do {
		scan = strchr(ptr, '/');
		if (scan)
			*scan = '\0';
		dir = opendir (lpath);
		if (dir)
		{
			while ((dirent = readdir (dir)) != NULL)
			{
				if (!strcmpi (dirent->d_name, ptr))
				{
					strcpy(ptr, dirent->d_name);
					if (strcmp(lpath, "/") != 0)
						strcat(lpath, "/");
					strcat(lpath, dirent->d_name);
					break;
				}
			}
			closedir(dir);
			
			if (scan)
			{
				*scan = '/';
				ptr = scan+1;
			}
		}
		else
			break;
	} while (scan);

	strcpy(filename, slash2back(fname));
	return filename;
}
