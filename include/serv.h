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
 * Fichier des variables locales
 */

#ifndef _XFBB_SERV_H
#define _XFBB_SERV_H

#define ENGLISH

#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>

#define _timezone timezone
#define xprintf printf
#define cprintf printf
#define stricmp strcasecmp
#define strcmpi strcasecmp
#define strnicmp strncasecmp
#define strncmpi strncasecmp
#define _read read
#define _write write
#define wsprintf sprintf

#define tell(fd) lseek((fd), 0L, SEEK_CUR)

#define MAXPATH 512
#define O_BINARY 0
#define O_TEXT   0100000        /* ne doit pas gener ds fcntl.h */

struct  ftime   {
    unsigned    ft_tsec  : 5;   /* Two second interval */
    unsigned    ft_min   : 6;   /* Minutes */
    unsigned    ft_hour  : 5;   /* Hours */
    unsigned    ft_day   : 5;   /* Days */
    unsigned    ft_month : 4;   /* Months */
    unsigned    ft_year  : 7;   /* Year */
};

#define FA_NORMAL   0x00        /* Normal file, no attributes */
#define FA_RDONLY   0x01        /* Read only attribute */
#define FA_HIDDEN   0x02        /* Hidden file */
#define FA_SYSTEM   0x04        /* System file */
#define FA_LABEL    0x08        /* Volume label */
#define FA_DIREC    0x10        /* Directory */
#define FA_ARCH     0x20        /* Archive */
#define FA_LINK     0x80        /* Lien */
 
#define FBB_NAMELENGTH 128
#define FBB_MASKLENGTH 64
#define FBB_BASELENGTH 256

struct  ffblk   {
  DIR        *ff_dir;
  char        ff_mask[FBB_MASKLENGTH+1];
  char        ff_base[FBB_BASELENGTH+1];
  char        ff_attrib;
  unsigned    ff_ftime;
  unsigned    ff_fdate;
  long        ff_fsize;
  char        ff_name[FBB_NAMELENGTH+1];
};

/* #define MAXPATH   80 */
#define MAXDIR    66
#define MAXFILE   9
#define MAXEXT    5

#define P_WAIT    0

#include "fbb_serv.h"
/*
#ifdef __cplusplus
extern "C" {
#endif

char *strupr (char *);
char *itoa(int, char*, int);
char *ltoa(long, char*, int);
char *ultoa(unsigned long, char*, int);
char *strupr(char *);

int GetFileNameDialog(char *);

void InfoMessage(int, char *, char *);
void FbbMem(int);
int fbb_list(int);

int fbb_ax25_config_load_ports(void);
int fbb_nr_config_load_ports(void);
int fbb_rs_config_load_ports(void);

void deb_io (void);
void fin_io (void);
void randomize(void);
int random_nb(int num);
long filelength(int fd);
int getdisk(void);
char *getcurdir(int drive, char *str);
int is_cdir(int chr);
int fnsplit(char *name, char *drive, char *rep, char *base, char *ext);
int getftime(int fd, struct ftime *ft);
void format_ffblk(struct ffblk *blk, struct dirent *dir);
int findfirst (char *rech, struct ffblk *blk, int mask);
int findnext (struct ffblk *blk);
unsigned long free_disk (int disk);
int filter (char *ligne, char *buffer, int len, char *data);

int fbb_statfs(char *filename, struct statfs *buf);

#ifdef __cplusplus
}
#endif
*/

extern int filter (char *, char *, int , char * , char *);

#define CM_OPTIONJUSTIF   0
#define CM_OPTIONALARM    1
#define CM_OPTIONCALL     2
#define CM_OPTIONGATEWAY  3
#define CM_OPTIONAFFICH   4
#define CM_OPTIONSOUNDB   5
#define CM_OPTIONEDIT     6
#define CM_OPTIONINEXPORT 7

#define CM_FILESCANMSG  10

#endif





