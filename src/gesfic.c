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
#define FBB_IO

#include <serv.h>

#if defined(__FBBDOS__) || defined(__WINDOWS__)
#include <share.h>
#endif

#define	CTRL_Z		'\032'

static unsigned long *lmode = NULL;
static int nb_lmode = 0;
#define MODE_INC 32

#if !defined(__WIN32__) && (defined(__FBBDOS__) || defined(__WINDOWS__))
extern unsigned int _openfd[];

#endif

int fbb_open (char *filename, int acces, unsigned mode);

static int set_mode (int fd, unsigned long mode)
{
	if (lmode == NULL)
	{
		nb_lmode = MODE_INC;
		lmode = malloc(sizeof(unsigned long) * nb_lmode);
		if (lmode == NULL)
		{
			nb_lmode = 0;
			return 0;
		}
	}
	
	while (fd >= nb_lmode)
	{
		nb_lmode += MODE_INC;
		lmode = realloc(lmode, sizeof(unsigned long) * nb_lmode);
		if (lmode == NULL)
		{
			nb_lmode = 0;
			return 0;
		}
	}
	
	lmode[fd] = mode;
	
	return 1;
}

static unsigned long get_mode (int fd)
{
	return lmode[fd];
}

char *lfile (char *filename)
{
	static char lockname[256];
	char *ptr;

	strcpy (lockname, filename);
	ptr = strrchr (lockname, '.');
	if (ptr)
		*ptr = '\0';
	strcat (lockname, ".lck");
	return (lockname);
}

char *slash2back (char *str)
{
	static char buf[256];
	char *ptr = buf;
	int nb = 0;

	while (*str)
	{
		if (*str == '/')
			*ptr = '\\';
//		else if (islower (*str))
//			*ptr = toupper (*str);
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
//		else if (isupper (*str))
//			*ptr = tolower (*str);
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

int fbb_access (char *filename, int mode)
{
#ifdef __linux__
	return (access (back2slash (filename), mode));
#else
	return (access (filename, mode));
#endif
}

int fbb_stat (char *filename, struct stat *buf)
{
#ifdef __linux__
	return (stat (back2slash (filename), buf));
#else
	return (stat (filename, buf));
#endif
}

#ifdef __linux__
int fbb_mkdir (char *filename, int mode)
{
	return (mkdir (back2slash (filename), mode));
}
#else
int fbb_mkdir (char *filename)
{
	return (mkdir (filename));
}
#endif

int fbb_rmdir (char *filename)
{
#ifdef __linux__
	return (rmdir (back2slash (filename)));
#else
	return (rmdir (filename));
#endif
}

#ifdef __linux__
int fbb_statfs (char *filename, struct statfs *buf)
{
	return (statfs (back2slash (filename), buf));
}
#endif

void fbb_textattr (int att)
{
#ifdef __FBBDOS__
	if (video_off == 2)
		att = 0;
	textattr (att);
#endif
}


char *fbb_fgets (char *s, int n, FILE * fptr)
{
	char *pres;

#ifdef __linux__
	pres = fgets (s, n, fptr);
	if (lmode[fileno (fptr)] & O_TEXT)
	{
		if (pres)
		{
			char *ptr = pres;
			char *out = pres;

			while (*ptr)
			{
				if (*ptr == '\r')
				{
					++ptr;
					continue;
				}
				if (*ptr == CTRL_Z)
					break;
				*out++ = *ptr++;
			}
			*out = '\0';
		}
	}
#else
	deb_io ();
	pres = fgets (s, n, fptr);
	fin_io ();
#endif
	return (pres);
}


int fbb_fputs (char *s, FILE * fptr)
{
	int res;

	deb_io ();
	res = fputs (s, fptr);
	fin_io ();
	return (res);
}

int fbb_fgetc (FILE * fptr)
{
	int res;

	deb_io ();
	res = fgetc (fptr);
#ifdef __linux__
	if (get_mode(fileno (fptr)) & O_TEXT)
	{
		if (res == CTRL_Z)
			res = EOF;
		else if (res == '\r')
		{
			res = fgetc (fptr);
			if (res == CTRL_Z)
				res = EOF;
		}
	}
#endif
	fin_io ();
	return (res);
}


int fbb_fputc (int c, FILE * fptr)
{
	int res;

	deb_io ();
	res = fputc (c, fptr);
	fin_io ();
	return (res);
}


FILE *fbb_fopen (char *filename, char *mode)
{
	FILE *fptr;

	deb_io ();
#ifdef __linux__
	fptr = fopen (back2slash (filename), mode);
#else
	fptr = fopen (filename, mode);
#endif
	fin_io ();
	if (fptr)
	{
		char *ptr = mode;

		set_mode(fileno (fptr), 0);
		while (*ptr)
		{
			if (*ptr == 't')
				set_mode(fileno (fptr), O_TEXT);
			if (*ptr == 'a')
				fseek (fptr, 0L, SEEK_END);
			++ptr;
		}
	}

	return (fptr);
}


FILE *fsopen (char *filename, char *mode)
{
#ifdef __linux__
	FILE *fptr;

	fptr = fopen (back2slash (filename), mode);
	if (fptr)
	{
		char *ptr = mode;

		set_mode(fileno (fptr), 0);
		while (*ptr)
		{
			if (*ptr == 't')
				set_mode(fileno (fptr), O_TEXT);
			if (*ptr == 'a')
				fseek (fptr, 0L, SEEK_END);
			++ptr;
		}
	}
	return (fptr);
#endif
#if defined(__FBBDOS__) || defined(__WINDOWS__)
	int fd;
	char s[80];
	FILE *fptr;

	deb_io ();
	for (;;)
	{
		fd = sopen (filename, O_RDONLY | O_TEXT, SH_DENYWR, 0);
		if ((fd >= 0) || (errno != EACCES))
			break;
		aff_etat ('Z');
		fin_io ();
#ifdef ENGLISH
		sprintf (s, "SHARING VIOLATION (%s)", filename);
#else
		sprintf (s, "VIOLATION PARTAGE (%s)", filename);
#endif
		win_message (2, s);
		deb_io ();
	}
	if (fd < 0)
		fptr = NULL;
	else
		fptr = fdopen (fd, mode);
	fin_io ();
	return (fptr);
#endif
}


int fbb_fclose (FILE * fptr)
{
	int res;
	int fd = fileno (fptr);

	deb_io ();
	res = fclose (fptr);

	set_mode(fd, 0);

	fin_io ();
	return (res);
}

int fbb_open (char *filename, int acces, unsigned mode)
{
	int fd;

	deb_io ();
#ifdef __linux__
	fd = open (back2slash (filename), acces & (~O_TEXT), mode);
#else
	fd = open (filename, acces, mode);
#endif
	fin_io ();
	if (fd > 0)
		set_mode(fd, acces & O_TEXT);
	return (fd);
}


int fbb_close (int fd)
{
	int res;

	deb_io ();
	res = close (fd);
	set_mode(fd, 0);
	fin_io ();
	return (res);
}


int fbb_read (int fd, void *buf, unsigned nb)
{
	int retour;

	deb_io ();
#ifdef __linux__
	retour = read (fd, buf, nb);
	/* search for ^Z */
	if ((retour > 0) && (get_mode(fd) & O_TEXT))
	{
		int n;
		char *ptr = (char *) buf;

		for (n = 0; n < retour; n++)
		{
			if (ptr[n] == CTRL_Z)
			{
				retour = n;
				break;
			}
		}
	}
#else
	retour = _read (fd, buf, nb);

	if ((retour > 0) && (get_mode(fd) & O_TEXT))
	{
		int i, n;
		char *ptri = (char *) buf;
		char *ptro = (char *) buf;

		for (i = 0, n = 0; i < retour; i++)
		{
			if (ptri[i] == '\r')
				continue;
			else if (ptri[i] == CTRL_Z)
				break;
			else
				ptro[n++] = ptri[i];
		}
		retour = n;
	}
#endif
	fin_io ();
	return (retour);
}


int fbb_write (int fd, void *buf, unsigned nb)
{
	int retour;

	deb_io ();
#ifdef __linux__
	if (get_mode(fd) & O_TEXT)
	{
		int i;
		int len;
		char *ptro, *ptri;

		/* Ignorer CR, Remplacer LF par CRLF */

		ptri = (char *) buf;
		if (nb)
		{
			ptro = (char *) malloc (nb * 2);

			for (len = 0, i = 0; i < nb; i++)
			{
				if (ptri[i] == '\r')
				{
					continue;
				}
				else if (ptri[i] == '\n')
				{
					ptro[len++] = '\r';
					ptro[len++] = '\n';
				}
				else
				{
					ptro[len++] = ptri[i];
				}
			}
			retour = write (fd, ptro, len);
			if (retour == len)
				retour = nb;
			free (ptro);
		}
		else
		{
			retour = write (fd, ptri, nb);
		}
	}
	else
#endif
		retour = write (fd, buf, nb);
	fin_io ();
	return (retour);
}


int fbb_fread (void *ptr, size_t taille, size_t n, FILE * fp)
{
	int retour;

	deb_io ();
	retour = fread (ptr, taille, n, fp);
#ifdef __linux__
	/* search for ^Z */
	if ((retour > 0) && (get_mode(fileno (fp)) & O_TEXT))
	{
		int i;
		char *p = (char *) ptr;

		for (i = 0; i < taille * n; i++)
		{
			if (p[i] == CTRL_Z)
			{
				p[i] = '\0';
				break;
			}
		}
	}
#endif
	fin_io ();
	return (retour);
}


int fbb_fwrite (void *ptr, size_t taille, size_t n, FILE * fp)
{
	int retour;

	deb_io ();
	retour = fwrite (ptr, taille, n, fp);
	fin_io ();
	return (retour);
}


int fbb_findfirst (char *chemin, struct ffblk *ffblk, int attribut)
{
	int retour;

	deb_io ();
	retour = findfirst (chemin, ffblk, attribut);
	fin_io ();
	return (retour);
}


int fbb_findnext (struct ffblk *ffblk)
{
	int retour;

	deb_io ();
	retour = findnext (ffblk);
	fin_io ();
	return (retour);
}


int fbb_unlink (char *filename)
{
	int retour;

	deb_io ();
#ifdef __linux__
	retour = unlink (back2slash (filename));
#else
	retour = unlink (filename);
#endif
	fin_io ();
	return (retour);
}

long fbb_filelength (int fd)
{
	long retour;

	deb_io ();
	retour = filelength (fd);
	fin_io ();
	return (retour);
}

FILE *ouvre_stats (void)
{
	FILE *fptr;

	while (TRUE)
	{
		if ((fptr = fbb_fopen (d_disque ("statis.dat"), "r+b")) == NULL)
		{
			if (err_ouvert (d_disque ("statis.dat")))
				cree_stat ();
			else
				fbb_error (ERR_CREATE, d_disque ("statis.dat"), 0);
		}
		else
			break;
	}
	return (fptr);
}


FILE *ouvre_sat (void)
{
	FILE *fptr;

	while (TRUE)
	{
		if ((fptr = fbb_fopen (d_disque ("sat\\satel.dat"), "r+b")) == NULL)
		{
			if (err_ouvert (d_disque ("sat\\satel.dat")))
				cree_sat ();
			else
				fbb_error (ERR_CREATE, d_disque ("sat\\satel.dat"), 0);
		}
		else
			break;
	}
	return (fptr);
}


int compress_mess (bullist * pbul)
{
	FILE *fptr;
	unsigned short crc = 0;
	char temp[128];
	unsigned int textsize = 0;

	strcpy (pvoie->sr_fic, mess_name (MBINDIR, pbul->numero, temp));
	if ((fptr = fbb_fopen (pvoie->sr_fic, "rb")) != NULL)
	{
		if (fread (&crc, sizeof (crc), 1, fptr) == 0)
			textsize = 0;
		if (fread (&textsize, sizeof (textsize), 1, fptr) == 0)
			textsize = 0;
		fclose (fptr);
		if (moto)
		{
			crc = xendien (crc);
			textsize = xendienl (textsize);
		}
		if (textsize > 1000000)
		{
			cprintf ("%s :\r\nCRC = %04x  Size = %u ... ", pvoie->sr_fic, crc, textsize);
			cprintf ("Error bin file. Recompressed !\r\n");
			sleep_ (2);
			textsize = 0;
		}
		if (textsize == 0)
		{
			fbb_unlink (pvoie->sr_fic);
		}
	}
	if (textsize == 0)
	{
		dde_huf (voiecur, pbul, ENCODE);
#ifdef HUFF_TASK
		return (0);
#else
		pvoie->ask = 0;
		return (1);
#endif
	}
	return (1);
}


/* #pragma warn -par */
FILE *ouvre_mess (unsigned mode, long nmess, char status)
{
	char strmode[3];
	char chaine[256];
	FILE *fptr;

	strmode[0] = 'r';
	strmode[1] = (mode == O_BINARY) ? 'b' : 't';
	strmode[2] = '\0';

	strcpy (pvoie->sr_fic, mess_name ((mode == O_BINARY) ? MBINDIR : MESSDIR, nmess, chaine));

	if ((fptr = fbb_fopen (pvoie->sr_fic, strmode)) == NULL)
	{
		if ((voiecur != CONSOLE) || (!inexport))
		{
			sprintf (chaine, "\rMessage file %s missing in %s\r", pvoie->sr_fic, mycall);
			outln (chaine, strlen (chaine));
			dde_warning ((mode == O_BINARY) ? W_BIN : W_ASC);
		}
	}
	return (fptr);
}


/* #pragma warn .par */

FILE *ouvre_dirmes (void)
{
	FILE *fptr;
	int erreur;

	while (TRUE)
	{
		if ((fptr = fbb_fopen (d_disque ("dirmes.sys"), "r+b")) == NULL)
		{
			erreur = errno;
			if (err_ouvert (d_disque ("dirmes.sys")))
				cree_dir (erreur);
			else
				fbb_error (ERR_CREATE, d_disque ("DIRMES.SYS"), 0);
		}
		else
			break;
	}
	return (fptr);
}


void ferme (FILE * fptr, int err)
{
	if (fbb_fclose (fptr))
		fbb_warning (ERR_CLOSE, "fclose", err);
}


FILE *ouvre_nomenc ()
{
	FILE *fptr;

	while (TRUE)
	{
		if ((fptr = fbb_fopen (d_disque ("inf.sys"), "r+b")) == NULL)
		{
			if (err_ouvert (d_disque ("INF.SYS")))
				cree_info ();
			else
				fbb_error (ERR_CREATE, d_disque ("inf.sys"), 0);
		}
		else
			break;
	}
	return (fptr);
}

char *mess_name (char *path, long numero, char *nom)
{
	unsigned int final;

	final = (unsigned int) (numero % 10);
	sprintf (nom, "%smail%u\\m_%06ld.mes", path, final, numero);
#ifdef __linux__
	strcpy (nom, back2slash (nom));
#endif
	return (nom);
}

char *temp_name (int voie, char *tempname)
{
	sprintf (tempname, "%stemp.%02d", MBINDIR, voie);
#ifdef __linux__
	strcpy (tempname, back2slash (tempname));
#endif
	return (tempname);
}

char *copy_name (int voie, char *tempname)
{
	sprintf (tempname, "%scopy.%02d", MBINDIR, voie);
#ifdef __linux__
	strcpy (tempname, back2slash (tempname));
#endif
	return (tempname);
}

char *xfwd_name (int voie, char *tempname)
{
	sprintf (tempname, "%sxfwd.%02d", MBINDIR, voie);
#ifdef __linux__
	strcpy (tempname, back2slash (tempname));
#endif
	return (tempname);
}

char *hold_name (long val, char *tempname)
{
	sprintf (tempname, "%s%08lx.hld", MESSDIR, val);
#ifdef __linux__
	strcpy (tempname, back2slash (tempname));
#endif
	return (tempname);
}

void del_temp (int voie)
{
	char temp[128];

	deb_io ();
	fbb_unlink (temp_name (voie, temp));
	fin_io ();
}

void del_copy (int voie)
{
	char temp[128];

	deb_io ();
	fbb_unlink (copy_name (voie, temp));
	fin_io ();
}

#define TAIBUF 5000
static char buffer[TAIBUF];

long copy_fic (int fd_orig, int fd_dest, char *lc)
{

	int nb_lus;
	int ret;
	long nb_oct = 0L;

	if (lc)
		*lc = '\0';

	while (1)
	{
		nb_lus = fbb_read (fd_orig, buffer, TAIBUF);
		nb_oct += (long) nb_lus;
		if (nb_lus > 0)
		{
			ret = fbb_write (fd_dest, buffer, nb_lus);
			if (lc)
				*lc = buffer[nb_lus - 1];
			if (ret == -1)
				break;
		}
		else
		{
			break;
		}
	}
	return (nb_oct);
}


int rename_temp (int voie, char *newname)
{
	int retour;
	int orig, dest;
	char temp[128];

	deb_io ();
	temp_name (voie, temp);
	fbb_unlink (newname);
	if ((0) && (*newname == *temp))
	{
		retour = rename (temp, newname);
	}
	else
	{
		retour = 0;
		orig = fbb_open (temp, O_RDONLY | O_BINARY, 0);
		if (orig < 0)
		{
			fin_io ();
			return (-1);
		}
		dest = fbb_open (newname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		if (dest < 0)
		{
			close (orig);
			fin_io ();
			return (-1);
		}
		copy_fic (orig, dest, NULL);
		fbb_close (orig);
		fbb_close (dest);
		
		fbb_unlink (temp);
	}
	fin_io ();
	return (retour);
}

int hold_temp (int voie, char *tempfile, char *newname, int erase)
{
	int retour = 1;
	int orig, dest;
	char header[256];

	deb_io ();

	orig = fbb_open (tempfile, O_RDONLY | O_BINARY, 0);
	if (orig < 0)
	{
		fin_io ();
		return (-1);
	}

	dest = fbb_open (newname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
	if (dest < 0)
	{
		close (orig);
		fin_io ();
		return (-1);
	}

	/* Reserve la place du header dans le fichier destination */
	memset (header, 0, sizeof (header));
	fbb_write (dest, header, sizeof (header));

	/* Copie le fichier temporaire */
	copy_fic (orig, dest, NULL);

	fbb_close (orig);

	if (*(svoie[voie]->appendf))
	{
		/* Ajoute le fichier d'append s'il existe */
		orig = fbb_open (svoie[voie]->appendf, O_RDONLY | O_BINARY, 0);
		if (orig < 0)
		{
			fin_io ();
			return (-1);
		}

		svoie[voie]->entmes.taille += copy_fic (orig, dest, NULL);
		fbb_close (orig);
		*(svoie[voie]->appendf) = '\0';
	}


	/* Mise a jour du header */
	lseek (dest, 0L, SEEK_SET);
	fbb_write (dest, &(svoie[voie]->entmes), sizeof (bullist));
	fbb_write (dest, svoie[voie]->mess_home, sizeof (svoie[voie]->mess_home));
	fbb_close (dest);
	if (erase)
		fbb_unlink (tempfile);
	fin_io ();
	return (retour);
}

FILE *fappend (char *filename, char *mode)
{
	FILE *fptr;

#if defined(__FBBDOS__) || defined(__WINDOWS__)
	char fullmode[5];

#ifdef __WIN32__
	int c;
	fullmode[0] = 'r';
	fullmode[1] = '+';
	fullmode[2] = *mode;
	fullmode[3] = '\0';
	if ((fptr = fbb_fopen (filename, fullmode)) != NULL)
	{
		fseek (fptr, -1L, SEEK_END);
		c = getc (fptr);
		if (c == CTRL_Z)
			fseek (fptr, -1L, SEEK_END);
		else
			fseek (fptr, 0L, SEEK_END);
		return (fptr);
	}
	else
	{
		fullmode[0] = 'w';
		fullmode[1] = *mode;
		fullmode[2] = '\0';
		return (fbb_fopen (filename, fullmode));
	}
#else
	if ((fptr = fbb_fopen (filename, "r+b")) != NULL)
	{
		fseek (fptr, -1L, SEEK_END);
		c = getc (fptr);
		if (c == CTRL_Z)
			fseek (fptr, -1L, SEEK_END);
		else
			fseek (fptr, 0L, SEEK_END);
		if (*mode == 't')
		{
			_openfd[fileno (fptr)] &= (~O_BINARY);
			_openfd[fileno (fptr)] |= O_TEXT;
		}
		return (fptr);
	}
	else
	{
		fullmode[0] = 'w';
		fullmode[1] = *mode;
		fullmode[2] = '\0';
		return (fbb_fopen (filename, fullmode));
	}
#endif
#endif
#ifdef __linux__
	if ((fptr = fbb_fopen (back2slash (filename), "r+")) != NULL)
	{
		fseek (fptr, 0L, SEEK_END);
		return (fptr);
	}
	else
		return (fbb_fopen (back2slash (filename), "w"));
#endif
}

int fbb_fcloseall (void)
{

#ifdef __linux__
	return 0;
#else
#if defined(__DPMI16__) || defined(__WINDOWS__)
	int cnt;
	cnt = fcloseall ();
	return (cnt);
#else
	int i;
	FILE *fp;

	for (i = 5, fp = _streams + 5, cnt = 0; i < FOPEN_MAX; fp++, i++)
	{
		if (fp->fd != 0xff)
		{
			if (fclose (fp))
				cnt = -9999;
			else
				cnt++;
		}
	}
	return (cnt < 0 ? EOF : cnt);
#endif /* DPMI16 / WINDWOS */
#endif /* LINUX */
}

void fbb_clrscr (void)
{
#ifdef __FBBDOS__
	clrscr ();
#endif
}

/* Returns true if there is more than 1000K available in disks */
int is_room (void)
{
	return ((sys_disk > MIN_DISK) && (tmp_disk > MIN_DISK));
}
