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
#include <modem.h>

/*
 *  Module FBBdos
 */

static int execute_dos(void);
static int rx_file(char *, int);

static void copy_file(void);
static void dos_copy(void);
static void edit_label(void);
static void help_appel(char **);
static void menu_dos(void);
static void put_file(void);
static void view(void);

static int where_loop;
/*
 * F_FILTER feature
 *
 * Parameters :
 *    Callsign-SSID
 *    Temp filename
 *    Number of the calling process (Xmodem=11, YAPP=17)
 *    Number of the user's record in INF.SYS
 *
 * Return value
 *    0 : File OK, can be recorded.
 *    1 : File not OK, will be discarded.
 *
 * Comments
 *    The datas sent to stdout by F_FILTER
 *    will be redirected to the user.
 */


/* return the next argument from indd and updates the indd pointer */
char *get_nextparam(void)
{
	char *ptr;

	while (*indd && isspace(*indd))
		++indd;

	if (*indd == '"')
	{
		ptr = ++indd;
		while (*indd && (*indd != '"'))
			++indd;
		if (*indd)
			*indd++ = '\0';
	}
	else
	{
		ptr = indd;
		while (*indd && isgraph(*indd))
			++indd;
		if (*indd)
			*indd++ = '\0';
	}

	return (*ptr) ? ptr : NULL;
}

int test_temp(int voie)
{
	static int test = 1;
	int retour = 1;
	int ret;
	char temp[128];
	char s[256];


	temp_name(voie, temp);
	indd[80] = '\0';

	if (test)
	{
		char dir[80];
		char file[80];
		FILE *fptr;

#ifdef __linux__
		strcpy(dir, "X:/");	/* fill string with form of response: X:/ */
#else
		strcpy(dir, "X:\\");	/* fill string with form of response: X:\ */
#endif
		dir[0] = 'A' + getdisk();	/* replace X with current drive letter */
		getcurdir(0, dir + 3);	/* fill rest of string with current directory */
#ifdef __linux__
		sprintf(file, "%s/f_filter.%02d", dir, voiecur);
#else
		sprintf(file, "%s\\f_filter.%02d", dir, voiecur);
#endif

#ifdef __linux__
		strcpy(file, back2slash(file));
#endif
		if ((fptr = fopen(file, "wt")) == NULL)
			return (-1);

		fprintf(fptr, "#\n# Downloaded File information\n#\n");
#ifdef __linux__
		fprintf(fptr, "TempName = %s\n", back2slash(temp));
		fprintf(fptr, "FileName = %s\n", back2slash(pvoie->sr_fic));
#else
		fprintf(fptr, "TempName = %s\n", temp);
		fprintf(fptr, "FileName = %s\n", pvoie->sr_fic);
#endif
		fprintf(fptr, "Label = %s\n", pvoie->label);
		fprintf(fptr, "#\n");

		fclose(fptr);

		{
			char buffer[1024];

			*buffer = '\0';

#ifdef __linux__
/*			sprintf(s, "%sf_filter %s-%d %d %u %s",
					FILTDIR,*/
			sprintf(s, "./f_filter %s-%d %d %u %s",
					pvoie->sta.indicatif.call, pvoie->sta.indicatif.num,
					pvoie->niv1, pvoie->ncur->coord, file);
			ret = filter(s, buffer, sizeof(buffer), NULL, FILTDIR);
#else
			sprintf(s, "f_filter %s-%d %d %u %s",
					pvoie->sta.indicatif.call, pvoie->sta.indicatif.num,
					pvoie->niv1, pvoie->ncur->coord, file);
			ret = filter(s, buffer, sizeof(buffer), NULL, NULL);
#endif

			buffer[1023] = '\0';
			if (*buffer)
				out(buffer, strlen(buffer));
		}

		unlink(file);

		switch (ret)
		{
		case -1:
			test = 0;
			clear_outbuf(voiecur);
			break;
		case 1:
			retour = 0;
			break;
		default:
			break;
		}
	}

	return (retour);
}

int user_ok(void)
{
	if (droits(MODLABEL | SUPFIC | ACCESDOS))
		return (1);

	if (*pvoie->finf.priv)
		return (1);

	if (P_MODM(voiecur))
	{
		if (max_mod == 0)
			return (1);
		if (pvoie->finf.download > max_mod)
			return (0);
	}
	else
	{
		if (max_yapp == 0)
		{
			return (1);
		}
		if (pvoie->finf.download > max_yapp)
		{
			return (0);
		}
	}
	return (1);
}
void retour_dos(void)
{
	retour_niveau();
	prompt(pvoie->finf.flags, pvoie->niv1);
}


char *local_path(char *chaine)
{
	int size, lg;
	static char temp[256];

	strcpy(temp, chaine);
	size = lg = strlen(temp);
	if ((lg > 2) && (temp[1] == ':'))
		size = lg - 2;
	if (size > 1)
		if (temp[lg - 1] == '\\')
			temp[lg - 1] = '\0';
	return (temp);
}


int tst_point(char *chaine)
{
	char *ptr;
	char vdisk;

	if ((voiecur != CONSOLE) &&
		(pvoie->kiss != -2) && ((ptr = strchr(chaine, ':')) != NULL))
	{
		--ptr;
		vdisk = toupper(*ptr) - 'A';
		if (vdisk == 15)
		{
			if (*pvoie->finf.priv == '\0')
			{
				texte(T_ERR + 29);
				return (FALSE);
			}
		}
		else if ((vdisk > 7) || (PATH[(int) vdisk][0] == '\0'))
		{
			texte(T_ERR + 29);
			return (FALSE);
		}
	}

	if (!droits(ACCESDOS))
	{
		if (strstr(chaine, "..") || strstr(chaine, "~"))
		{
			texte(T_ERR + 15);
			return (FALSE);
		}
	}
	return (TRUE);
}


#if 0
/*
 * Attend une structure tm en entree
 * Retourne le nombre de secondes depuis 1-1-1970
 */
static long date_to_second(struct tm *dat)
{
	static char Days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	long x;
	register int i;
	register int days;
	int hours;

	x = 24L * 60L * 60L * 3652L + timezone;	/* Convertit de 1980 a 1970 */
	i = dat->tm_year;
	if (i > 1900)
		i -= 1900;
	x += (i >> 2) * (1461L * 24L * 60L * 60L);
	x += (i & 3) * (24L * 60L * 60L * 365L);
	if (i & 3)
		x += 24L * 3600L;
	days = 0;
	i = dat->tm_mon - 1;
	while (i > 0)
	{
		i--;
		days += Days[i];
	}
	days += dat->tm_mday - 1;
	if (dat->tm_mon > 2 && (dat->tm_year & 3) == 0)
		days++;					/* bissextile */
	hours = days * 24 + dat->tm_hour;	/* Heures */
	x += hours * 3600L;
	x += 60L * dat->tm_min + dat->tm_sec;
	return (x);
}
#endif

void send_file(int type)
{
	int fin, retour;
	char *ptr;
	struct stat bufstat;

	switch (pvoie->niv3)
	{
	case 0:
		if (!user_ok())
		{
			texte(T_ERR + 18);
			retour_dos();
			break;
		}

		fin = 0;
		if (type)
			pvoie->lignes = -1;
		get_nextparam();
		if ((ptr = get_nextparam()) == NULL)
			fin = T_ERR + 20;
		else if (tst_point(ptr))
		{
			strcpy(pvoie->sr_fic, tot_path(ch_slash(ptr), pvoie->dos_path));
			retour = stat(pvoie->sr_fic, &bufstat);
			strcpy(pvoie->appendf, ptr);
			if ((retour == -1) || ((bufstat.st_mode & S_IFREG) == 0))
			{
				fin = T_ERR + 11;
			}
			else
			{
				pvoie->enrcur = 0L;
				pvoie->size_trans = 0L;
				pvoie->temp2 = type;
				if (senddata(0))
				{
					fin = -1;
					pvoie->finf.download += (int) (pvoie->size_trans / 1024L);
				}
				else
					ch_niv3(1);
			}
		}
		if (fin)
		{
			if (type)
				ctrl_z();
			if (fin > 0)
				texte(fin);
			retour_dos();
		}
		break;
	case 1:
		if (senddata(0))
		{
			if (pvoie->temp2)
				ctrl_z();
			pvoie->finf.download += (int) (pvoie->size_trans / 1024L);
			retour_dos();
		}
		break;
	}

}


void put_file(void)
{
	FILE *fptr;
	obuf *msgtemp;
	char temp[128];

	if ((fptr = fopen(temp_name(voiecur, temp), "at")) != NULL)
	{
		while ((msgtemp = pvoie->msgtete) != NULL)
		{
			fwrite(msgtemp->buffer, msgtemp->nb_car, 1, fptr);
			pvoie->memoc -= msgtemp->nb_car;
			pvoie->msgtete = msgtemp->suiv;
			m_libere((char *) msgtemp, sizeof(*msgtemp));
		}
		fclose(fptr);
	}
	libere(voiecur);
}


static int rx_file(char *ptr, int nbcar)
{
	int ncars;
	obuf *msgtemp;
	char *ptcur;

	if ((msgtemp = pvoie->msgtete) != NULL)
	{
		while (msgtemp->suiv)
			msgtemp = msgtemp->suiv;
	}
	else
	{
		msgtemp = (obuf *) m_alloue(sizeof(obuf));
		pvoie->msgtete = msgtemp;
		msgtemp->nb_car = msgtemp->no_car = 0;
		msgtemp->suiv = NULL;
	}
	ncars = msgtemp->nb_car;
	ptcur = msgtemp->buffer + ncars;
	while (nbcar--)
	{
		if (*ptr == '\32')
		{
			msgtemp->nb_car = ncars;
			put_file();
			ltoa(pvoie->tailm, varx[0], 10);
			texte(T_DOS + 5);
			return (FALSE);
		}
		++pvoie->tailm;
		if (*ptr == '\r')
		{
			++pvoie->tailm;
			*ptr = '\n';
		}
		++pvoie->memoc;
		*ptcur++ = *ptr++;
		if (++ncars == 250)
		{
			msgtemp->nb_car = ncars;
			msgtemp->suiv = (obuf *) m_alloue(sizeof(obuf));
			msgtemp = msgtemp->suiv;
			msgtemp->nb_car = msgtemp->no_car = ncars = 0;
			msgtemp->suiv = NULL;
			ptcur = msgtemp->buffer;
		}
	}
	msgtemp->nb_car = ncars;
	if (pvoie->memoc > MAXMEM)
		put_file();
	return (TRUE);
}


void receive_file(void)
{
	char *ptr;
	struct stat bufstat;

	switch (pvoie->niv3)
	{
	case 0:
		if (read_only())
		{
			retour_dos();
			break;
		}

		get_nextparam();
		if ((ptr = get_nextparam()) == NULL)
		{
			texte(T_ERR + 20);
			retour_dos();
			break;
		}
		if ((!tst_point(ptr)) || (!aut_ecr(ch_slash(ptr), 1)))
		{
			retour_dos();
			break;
		}
		entete_saisie();
		pvoie->tailm = 0L;
		n_cpy(40, pvoie->appendf, ptr);
		strcpy(pvoie->sr_fic,
			   tot_path(ch_slash(pvoie->appendf), pvoie->dos_path));
		if (stat(pvoie->sr_fic, &bufstat) == -1)
		{
			del_temp(voiecur);
			/* unlink(pvoie->sr_fic) ; */
			pvoie->xferok = 0;
			texte(T_YAP + 3);
			maj_niv(9, 3, 2);
		}
		else
		{
			texte(T_ERR + 23);
			retour_dos();
		}
		break;
	case 1:
		if (rx_file(indd, nb_trait) == 0)
		{
			pvoie->xferok = 1;
			if (test_temp(voiecur))
			{
				rename_temp(voiecur, pvoie->sr_fic);
				wr_dir(pvoie->sr_fic, pvoie->sta.indicatif.call);
			}
			retour_dos();
		}
		break;
	case 2:
		new_label();
		texte(T_DOS + 6);
		maj_niv(9, 3, 1);
		break;
	}
}


void dos_copy(void)
{
	struct stat bufstat;
	int fd_orig, fd_dest;
	long nb_oct;
	Rlabel rlabel;
	FILE *fptr;
	char *ptri, *ptro, *ptr;
	char orig[80];
	int r;

	get_nextparam();

	if ((ptri = get_nextparam()) == NULL)
	{
		texte(T_ERR + 20);
		return;
	}

	if ((ptro = get_nextparam()) == NULL)
	{
		texte(T_ERR + 21);
		return;
	}

	if (strcmpi(ptri, ptro) == 0)
	{
		texte(T_ERR + 23);
		return;
	}

	if (!tst_point(ptri))
		return;
	if (!tst_point(ptro))
		return;

	strcpy(orig, tot_path(ch_slash(ptri), pvoie->dos_path));

	stat(orig, &bufstat);
	strcpy(pvoie->appendf, ptri);

	if ((bufstat.st_mode & S_IFREG) == 0)
	{
		texte(T_ERR + 11);
		return;
	}

	if ((fd_orig = open(orig, O_RDONLY | O_BINARY)) == EOF)
	{
		texte(T_ERR + 11);
		return;
	}

	if (!aut_ecr(ch_slash(ptro), 1))
	{
		close(fd_orig);
		return;
	}

	strcpy(pvoie->appendf, ptro);
	strcpy(pvoie->sr_fic, tot_path(ch_slash(ptro), pvoie->dos_path));

	r = stat(pvoie->sr_fic, &bufstat);
	if ((r == 0 && (bufstat.st_mode & S_IFREG) == 0) || errno != ENOENT)
	{
		texte(T_ERR + 11);
		return;
	}

	if ((fd_dest =
		 open(pvoie->sr_fic, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
			  S_IREAD | S_IWRITE)) == EOF)
	{
		texte(T_ERR + 30);
		close(fd_orig);
		return;
	}

	nb_oct = copy_fic(fd_orig, fd_dest, NULL);

	close(fd_orig);
	close(fd_dest);
	wr_dir(pvoie->sr_fic, pvoie->sta.indicatif.call);

	*rlabel.label = '\0';

	ptr = vir_path(orig);
	fptr = NULL;

	if ((ptr) && ((fptr = fopen(d_disque("YAPPLBL.DAT"), "rb")) != NULL))
	{
		while (fread(&rlabel, sizeof(Rlabel), 1, fptr))
		{
			if (strcmp(ptr, rlabel.nomfic) == 0)
			{
				fclose(fptr);
				fptr = NULL;
				w_label(pvoie->sr_fic, rlabel.label);
				break;
			}
		}
	}

	if (fptr)
		fclose(fptr);

	ltoa(nb_oct, varx[0], 10);
	texte(T_DOS + 7);
	return;
}


void copy_file(void)
{
	if (!read_only())
		dos_copy();
	prompt_dos();
}


void del_file(void)
{
	char *ptr;

	if (!read_only())
	{
		get_nextparam();
		if ((ptr = get_nextparam()) == NULL)
		{
			texte(T_ERR + 20);
		}
		else
		{
			if (tst_point(ptr))
			{
				if (aut_ecr(ch_slash(ptr), 1))
				{
					strcpy(pvoie->appendf, ptr);
					if (unlink(tot_path(ptr, pvoie->dos_path)) == 0)
						texte(T_DOS + 10);
					else
						texte(T_ERR + 11);
				}
			}
		}
	}
	retour_dos();
}


static void edit_label(void)
{
	char *ptr;

	switch (pvoie->niv3)
	{
	case 0:
		if (droits(MODLABEL))
		{
			struct stat st;

			get_nextparam();
			if ((ptr = get_nextparam()) == NULL)
			{
				texte(T_ERR + 20);
				retour_dos();
				break;
			}

			if (!tst_point(ptr) || !aut_ecr(ptr, 0))
			{
				retour_dos();
				break;
			}

			n_cpy(40, pvoie->appendf, ptr);
			strcpy(pvoie->sr_fic,
				   tot_path(ch_slash(pvoie->appendf), pvoie->dos_path));

			if (stat(pvoie->sr_fic, &st) == -1)
			{
				texte(T_ERR + 11);
				retour_dos();
				break;
			}

			texte(T_YAP + 3);
			ch_niv3(1);
		}
		else
		{
			texte(T_ERR + 1);
			retour_dos();
		}
		break;

	case 1:
		while ((*indd) && (!ISPRINT(*indd)))
			++indd;
		if (ISGRAPH(*indd))
		{
			w_label(pvoie->sr_fic, sup_ln(indd));
			cr();
			dir_yapp(pvoie->appendf);
		}
		retour_dos();
		break;
	}
}


void help_appel(char *appel[])
{
	char s[80];
	int i = 0;

	texte(T_DOS + 8);
	texte(T_DOS + 9);
	while (1)
	{
		if (*appel[i] == '\0')
			break;
		sprintf(s, "%-10s", appel[i]);
		outs(s, strlen(s));
		if ((++i % 6) == 0)
			outs("\n", 1);
	}
	outs("\n\n", 2);
}


#ifdef __WINDOWS__
static int execute_dos(void)
{
	char buf[80];
	char cmd[256];
	int disk;
	int i;
	int ret;
	char cur_dir[MAXPATH];
	char fbbdos_dir[MAXPATH];

	while (isalnum(*indd))
		++indd;
	while_space();

	if (*indd)
	{
		/* recupere disque et repertoire courants */
		disk = getdisk();
		strcpy(cur_dir, "X:\\");
		cur_dir[0] = 'A' + disk;
		getcurdir(0, cur_dir + 3);

		strcpy(fbbdos_dir, tot_path("", pvoie->dos_path));

		if ((i = strlen(fbbdos_dir)) > 3)
			fbbdos_dir[i - 1] = '\0';

		operationnel = 2;

		{
			char deroute[80];

			wsprintf(deroute, "%sexecute.$$$", MBINDIR);
			ret =
				call_nbdos(&indd, 1, REPORT_MODE, deroute, fbbdos_dir, NULL);
			if (ret != -1)
				outfichs(deroute);
			unlink(deroute);
		}

		operationnel = 1;

		if (ret == -1)
			texte(T_ERR + 8);
		else if (ret != 0)
		{
			texte(T_ERR + 8);
			wsprintf(buf, "Errorlevel = %d", ret);
			outln(buf, strlen(buf));
		}
	}
	else
	{
		texte(T_ERR + 20);
	}

	prompt_dos();
	return (1);
}

#endif

#ifdef __linux__
static int execute_dos(void)
{
	char buf[256];
	int ret;

	/* run a task under linux. This task is blocking ... */

	/* Skip DOS command */
	while (isalnum(*indd))
		++indd;
	while_space();

	operationnel = 2;

	{
		char deroute[80];
		char fbbdos_dir[256];
		char *ptr;
		int i;

		strcpy(fbbdos_dir, tot_path("", pvoie->dos_path));
		if ((i = strlen(fbbdos_dir)) > 3)
			fbbdos_dir[i - 1] = '\0';

		sprintf(deroute, "%sexecute.xxx", MBINDIR);

		ptr = indd;

		/* Look for a semi-column or pipe command */

		while (*ptr)
		{
			if (*ptr == ';' || *ptr == '|')
			{
				*ptr = '\0';
				break;
			}
			ptr++;
		}

		ret = call_nbdos(&indd, 1, REPORT_MODE, deroute, fbbdos_dir, NULL);

		if (ret != -1)
			outfichs(deroute);
		unlink(deroute);

	}

	operationnel = 1;

	if (ret == -1)
		texte(T_ERR + 8);
	else if (ret != 0)
	{
		texte(T_ERR + 8);
		sprintf(buf, "Errorlevel = %d", ret);
		outln(buf, strlen(buf));
	}

	prompt_dos();
	return (1);
}
#endif

#ifdef __FBBDOS__
static int execute_dos(void)
{
	int retour = 1;
	static char slash_c[] = "/C";
	char *arg[20];
	char *ptr;
	char log[80];
	int i = 0;
	int disk;
	int ofst;
	int duplic, oldstdout, oldstderr;
	char deroute[80];
	char cur_dir[MAXPATH];
	char fbbdos_dir[MAXPATH];

	sprintf(deroute, "%s\\EXECUTE.$$$", getcwd(log, 80));

	strtok(indd, " ");

	arg[i++] = getenv("COMSPEC");
	arg[i++] = slash_c;
	while ((arg[i] = strtok(NULL, " ")) != NULL)
		++i;

	if (i == 2)
	{
		retour = -1;
		texte(T_ERR + 20);
	}

	else
	{
		ofst = 2;
		ptr = strrchr(arg[2], '.');
		if (ptr)
		{
			if (strcmp(strupr(ptr + 1), "BAT") == 0)
				ofst = 0;
		}

		deb_io();

		/* redirige stdout et stderr sur le fichier EXECUTE.$$$ */
		duplic = open(deroute, O_CREAT | O_RDWR, S_IWRITE | S_IREAD);
		oldstdout = dup(1);
		oldstderr = dup(2);
		dup2(duplic, 1);
		dup2(duplic, 2);
		close(duplic);

		/* recupere disque et repertoire courants */
		disk = getdisk();
		strcpy(cur_dir, "X:\\");
		cur_dir[0] = 'A' + disk;
		getcurdir(0, cur_dir + 3);

		strcpy(fbbdos_dir, tot_path("", pvoie->dos_path));
		setdisk(fbbdos_dir[0] - 'A');

		if ((i = strlen(fbbdos_dir)) > 3)
			fbbdos_dir[i - 1] = '\0';
		chdir(fbbdos_dir);

		operationnel = 2;

		break_ok();
		retour = spawnvp(P_WAIT, arg[ofst], arg + ofst);
		if (retour == -1)
		{
			ofst = 0;
			retour = spawnvp(P_WAIT, arg[ofst], arg + ofst);
		}
		break_stop();
		operationnel = 1;

		/* remet disque et repertoire courants */
		setdisk(disk);
		chdir(cur_dir);

		/* Supprime les redirections */
		dup2(oldstdout, 1);
		dup2(oldstderr, 2);
		close(oldstdout);
		close(oldstderr);

		fin_io();
		outfich(deroute);
		unlink(deroute);
		if (retour == -1)
			texte(T_ERR + 8);
		else if (retour != 0)
		{
			texte(T_ERR + 8);
			sprintf(log, "Errorlevel = %d", retour);
			outln(log, strlen(log));
		}
	}

	prompt_dos();
	return (retour);
}

#endif

static void view(void)
{
	char *ptr;
	char temp[256];

	get_nextparam();
	ptr = get_nextparam();
	if ((ptr) && (strchr(ptr, '/') == NULL))
	{
#ifdef __linux__
		{
			static char *fbb_view = "fbb_view";
			char *vptr = getenv("FBB_VIEW");

			if (vptr == NULL)
				vptr = fbb_view;
			sprintf(temp, "DOS %s %s", vptr, back2slash(ptr));
		}
#else
		sprintf(temp, "DOS FV %s", ptr);
#endif
		indd = temp;
		maj_niv(9, 99, 0);
		execute_dos();
		cr();
	}
	else
	{
		texte(T_ERR + 20);
		prompt_dos();
	}
}


static int where(int lg, char *path, char *pattern)
{
	struct ffblk ffblk;
	char rech[80];
	char temp[128];
	int done = 1;
	int premier = 1;
	int retour = 0;

	strcpy(rech, path);
#ifdef __linux__
	strcat(rech, "*");
#else
	strcat(rech, "*.*");
#endif

	++where_loop;

	done = findfirst(rech, &ffblk, FA_DIREC);
	while (!done)
	{
		if (*ffblk.ff_name != '.')
		{
			if (ffblk.ff_attrib & FA_DIREC)
			{
				strcpy(temp, path);
				strcat(temp, ffblk.ff_name);
				strcat(temp, "\\");
				if ((strncmp(temp + 1, ":\\PROC\\", 7) != 0)
					&& (where_loop < 16))
				{
					if (where(lg, temp, pattern))
						retour = 1;
				}
				premier = 1;
			}
			else
			{
				if (strmatch(ffblk.ff_name, pattern))
				{
					if (premier)
					{
						char v;

						if (pvoie->vdisk == 8)
							v = 'P';
						else
							v = 'A' + pvoie->vdisk;
						sprintf(temp, "%c:%s :", v, path + lg);
						outln(temp, strlen(temp));
						premier = 0;
					}
					*varx[4] = *varx[5] = *varx[6] = *varx[7] = '\0';
					sprintf(varx[0], "      %-13s", ffblk.ff_name);
					var_cpy(2, dir_date(ffblk.ff_fdate));
					*varx[3] = '\0';
					if ((ffblk.ff_attrib & FA_DIREC) != 0)
					{
						var_cpy(1, "<DIR> ");
					}
					else
					{
						sprintf(varx[1], "%6ld", ffblk.ff_fsize);
					}
					texte(T_DOS + 3);
					retour = 1;
					if (pvoie->memoc >= MAXMEM)
						break;
				}
			}
		}
		done = findnext(&ffblk);
	}

	--where_loop;

	return (retour);
}


static void where_file(void)
{
	int lg;
	int retour = 0;
	char *ptr;

	get_nextparam();
	pvoie->noenr_menu = 0L;

	ptr = get_nextparam();

	if (ptr == NULL)
	{
		texte(T_ERR + 20);
		retour_dos();
		return;
	}

	if (tst_point(ptr))
	{
		char cur_dir[80];
		int sav_vdisk = pvoie->vdisk;

		for (pvoie->vdisk = 0; pvoie->vdisk < 9; ++pvoie->vdisk)
		{
			if (*PATH[(int)pvoie->vdisk] == '\0')
				continue;

			if ((pvoie->vdisk == 8) && (*pvoie->finf.priv == '\0'))
				continue;

			/* strcpy (cur_dir, tot_path ("\0", "\\")); */
			strcpy(cur_dir, PATH[(int)pvoie->vdisk]);
			lg = strlen(cur_dir);

			where_loop = 0;
			if (where(lg - 1, cur_dir, ptr))
				retour = 1;
		}
		pvoie->vdisk = sav_vdisk;
	}
	if (pvoie->memoc >= MAXMEM)
		outln("....", 4);
	else if (!retour)
		texte(T_DOS + 2);
	retour_dos();
}


void menu_dos(void)
{
	int i, error = 0;
	int vdisk;

	char *iptr, *optr, commande[80];
	static char *appel[] = {
		"HELP", "?", "O",
		"DIR", "EDIT", "GET", "PUT", "CD", "MD", "MKDIR",
		"COPY", "DEL", "RD", "RMDIR", "TYPE", "DU", "YGET", "YPUT",
		"XGET", "XPUT", "LIST", "VIEW", "NEW", "LABEL", "WHERE",
		"X1GET", "ZGET", "ZPUT", "BGET", "BPUT",
		"EXIT", "QUIT", "F", "B", "\0"
	};
	char com[80];
	char temp[80];

	limite_commande();
	while (*indd && (!ISGRAPH(*indd)))
		indd++;
	strn_cpy(70, com, indd);
	sup_ln(indd);

	if (*indd == '\0')
	{
		prompt_dos();
		return;
	}

	if (strncmp(com, "PRIV", 4) == 0)
	{
		fbb_log(voiecur, 'D', com);
		if (*pvoie->finf.priv)
		{
			pvoie->vdisk = 8;
			strcpy(pvoie->dos_path, "\\");
		}
		else
			texte(T_ERR + 29);
		prompt_dos();
		return;
	}

	if ((droits(EXEDOS)) && (strncmp(com, "DOS", 3) == 0))
	{
		fbb_log(voiecur, 'D', com);
		maj_niv(9, 99, 0);
		execute_dos();
		return;
	}

	if ((com[1] == ':') && (!ISGRAPH(com[2])))
	{
#ifdef __linux__
		if (voiecur == CONSOLE)
		{
			static char *txt = "Cannot select a virtual disk in console mode";
			outln(txt, strlen(txt));
			prompt_dos();
			return;
		}
#endif
		vdisk = toupper(com[0]) - 'A';
		if (vdisk == 15)
		{
			vdisk = 8;
		}
		strcat(com, "\\");
		if ((voiecur == CONSOLE) && (is_dir(com)))
		{
			pvoie->vdisk = vdisk;
			strcpy(pvoie->dos_path, "\\");
		}
		else if ((voiecur != CONSOLE) && (vdisk < 8) && (*PATH[vdisk]))
		{
			pvoie->vdisk = vdisk;
			strcpy(pvoie->dos_path, "\\");
		}
		else if ((vdisk == 8) && (*pvoie->finf.priv))
		{
			pvoie->vdisk = vdisk;
			strcpy(pvoie->dos_path, "\\");
		}
		else
			texte(T_ERR + 29);
		prompt_dos();
		return;
	}

	if ((strncmp(com, "HELP", 4) == 0) || (*com == '?') || (*com == 'H'))
	{
		if (*com == '?')
			++indd;
		else
		{
			while (ISGRAPH(*indd))
				++indd;
		}
		while_space();
		if (*indd == '\0')
			*--indd = '?';
		if (!out_help(indd))
			help_appel(appel);
		*indd = '\0';
	}

	if ((strncmp(com, "CD", 2) == 0) && (ISGRAPH(*(com + 2))))
	{
		sprintf(temp, "CD %s", com + 2);
		indd = temp;
	}

	if (*indd)
	{
		iptr = indd;
		optr = commande;
		while (ISGRAPH(*iptr))
		{
			*optr = (islower(*iptr)) ? toupper(*iptr) : *iptr;
			iptr++;
			optr++;
		}
		*optr = '\0';
		if (*commande == 'O')
			commande[1] = '\0';

		i = 0;
		while (1)
		{
			optr = commande;
			var_cpy(0, optr);
			if (*appel[i] == '\0')
			{
				texte(T_ERR + 1);
				prompt_dos();
				return;
			}
			if (strncmp(appel[i], optr, 3) == 0)
				break;
			++i;
		}

		pvoie->temp1 = N_DOS;

		switch (i)
		{
		case 0:
		case 1:				/* help_appel(appel) ; */
			break;
		case 2:
			indd = com + 1;
			mbl_options();
			retour_dos();
			break;
		case 3:
			maj_niv(9, 1, 0);
			dir();
			break;
		case 4:
			maj_niv(9, 9, 0);
			edit();
			break;
		case 5:
			maj_niv(9, 2, 0);
			send_file(1);
			break;
		case 6:
			if (!is_room())
			{
				outln("*** Disk full !", 15);
				error = 4;
				break;
			}
			maj_niv(9, 3, 0);
			receive_file();
			break;
		case 7:
			maj_niv(9, 4, 0);
			change_dir();
			break;
		case 8:
		case 9:
			maj_niv(9, 5, 0);
			make_dir();
			break;
		case 10:
			if (!is_room())
			{
				outln("*** Disk full !", 15);
				error = 4;
				break;
			}
			maj_niv(9, 6, 0);
			copy_file();
			break;
		case 11:
			maj_niv(9, 7, 0);
			del_file();
			break;
		case 12:
		case 13:
			maj_niv(9, 8, 0);
			remove_dir();
			break;
		case 14:
			maj_niv(9, 2, 0);
			send_file(0);
			break;
		case 15:
			du();
			break;
		case 16:
			if (!user_ok())
			{
				error = 3;
			}
			else
			{
				if (P_MODM(voiecur))
				{
					var_cpy(0, "YMODEM");
					maj_niv(N_MOD, 4, XS_INIT);
					pvoie->type_yapp = 2;
					xmodem();
				}
				else
				{
					indd += 3;
					*indd = 'D';
					maj_niv(N_YAPP, 0, 0);
					menu_yapp();
				}
			}
			break;
		case 17:
			if (!is_room())
			{
				outln("*** Disk full !", 15);
				error = 4;
				break;
			}
			if (P_MODM(voiecur))
			{
#if defined(__WINDOWS__) || defined(__linux__)
				var_cpy(0, "YMODEM");
				maj_niv(N_MOD, 4, XR_INIT);
				pvoie->type_yapp = 2;
				xmodem();
#else
				var_cpy(0, "YMODEM");
				pvoie->type_yapp = 2;
				error = 2;
#endif
			}
			else
			{
				indd += 3;
				*indd = 'U';
				maj_niv(N_YAPP, 0, 0);
				menu_yapp();
			}
			break;
		case 18:
			var_cpy(0, "XMODEM");
			if (P_MODM(voiecur))
			{
				if (!user_ok())
					error = 3;
				else
				{
					maj_niv(N_MOD, 4, XS_INIT);
					pvoie->type_yapp = 0;
					xmodem();
				}
			}
			else
				error = 2;
			break;
		case 19:
			if (!is_room())
			{
				outln("*** Disk full !", 15);
				error = 4;
				break;
			}
			var_cpy(0, "XMODEM");
			if (P_MODM(voiecur))
			{
				maj_niv(N_MOD, 4, XR_INIT);
				pvoie->type_yapp = 0;
				xmodem();
			}
			else
				error = 2;
			break;
		case 20:
			maj_niv(9, 10, 0);
			list();
			break;
		case 21:
			maj_niv(9, 11, 0);
			view();
			break;
		case 22:
			maj_niv(9, 12, 0);
			dir_new();
			break;

		case 23:
			maj_niv(9, 14, 0);
			edit_label();
			break;

		case 24:
			maj_niv(9, 16, 0);
			where_file();
			break;

		case 25:
			var_cpy(0, "1K-XMODEM");
			if (P_MODM(voiecur))
			{
				if (!user_ok())
					error = 3;
				else
				{
					maj_niv(N_MOD, 4, XS_INIT);
					pvoie->type_yapp = 1;
					xmodem();
				}
			}
			else
				error = 2;
			break;
		case 26:
			var_cpy(0, "ZMODEM");
			if (P_MODM(voiecur))
			{
				if (!user_ok())
					error = 3;
				else
				{
					maj_niv(N_MOD, 4, XS_INIT);
					pvoie->type_yapp = 3;
					xmodem();
				}
			}
			else
				error = 2;
			break;
		case 27:
			if (!is_room())
			{
				outln("*** Disk full !", 15);
				error = 4;
				break;
			}
			var_cpy(0, "ZMODEM");
			if ((P_MODM(voiecur))
				&& ((BIOS(no_port(voiecur)) == P_WINDOWS)
					|| (BIOS(no_port(voiecur)) == P_LINUX)))
			{
				if (!user_ok())
					error = 3;
				else
				{
					maj_niv(N_MOD, 4, XR_INIT);
					pvoie->type_yapp = 3;
					xmodem();
				}
			}
			else
				error = 2;
			break;
		case 28:
			if (!user_ok())
			{
				error = 3;
			}
			else
			{
				maj_niv(N_BIN, 0, 0);
				bin_transfer();
			}
			break;
		case 29:
			if (!is_room())
			{
				outln("*** Disk full !", 15);
				error = 4;
				break;
			}
			maj_niv(N_BIN, 0, 4);
			bin_transfer();
			break;
		case 30:
		case 31:
		case 32:
			error = 1;
			retour_mbl();
			break;
		case 33:
			indd = com + 1;
			if (bye())
			{
				error = 1;
				maj_niv(N_MENU, 0, 0);
				sortie();
			}
			break;
		default:
			error = 1;
			retour_mbl();
			break;
		}

		switch (error)
		{

		case 2:
			texte(T_YAP + 2);
			prompt_dos();
			break;

		case 3:
			texte(T_ERR + 18);
			prompt_dos();
			break;

		case 4:
			prompt_dos();
			break;

		case 0:
			fbb_log(voiecur, 'D', com);
			break;
		}
	}
	else
		prompt_dos();
}


void dos(void)
{
	switch (pvoie->niv2)
	{
	case 0:
		menu_dos();
		break;
	case 1:
		dir();
		break;
	case 2:
		send_file(0);
		break;
	case 3:
		receive_file();
		break;
	case 4:
		change_dir();
		break;
	case 5:
		make_dir();
		break;
	case 6:
		copy_file();
		break;
	case 7:
		del_file();
		break;
	case 8:
		remove_dir();
		break;
	case 9:
		edit();
		break;
	case 10:
		list();
		break;
	case 11:
		view();
		break;
	case 12:
		dir_new();
		break;
	case 14:
		edit_label();
		break;
	case 15:
		bin_transfer();
		break;
	case 16:
		where_file();
		break;
	case 99:
		execute_dos();
		break;
	default:
		fbb_error(ERR_NIVEAU, "FBBDOS", pvoie->niv2);
		break;
	}
}
