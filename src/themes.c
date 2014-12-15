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
 * THEMES.C
 *
 * Gestion des themes de bulletins
 */

#include <serv.h>

#define READ_OK(st)	((st.status != 'K') && (st.status != 'A') && (st.status != 'H'))

#define iswcard(c) ((c) == '*' || (c) == '?' || (c) == '&' || (c) == '#' || (c) == '@' || (c) == '=')
#define isword(c) ((c) == '@' || iswcard(c) || isalnum(c))
#define isope(c) ((c) == '&' || (c) == '|')

#define OPE_OR  255
#define OPE_AND 254
#define OPE_NOT 253
#define OPE     252

#define TYP_TO  0
#define TYP_VIA 1

#define MAX_THEMES	250

static int themes_read (int verbose);

static void error_file (void);
static void libere_theme (void);
static void prompt_themes (void);
static void trans_num (void);
static void group_select(char *ptr);
static void group_read(char *ptr);
static void group_previous(void);
static void group_next(void);
static void group_xhdr(char *ptr);
static void group_xover(char *ptr);

typedef char NomTh[7];

#define LG_THEME 32

typedef struct type_Fields {
	char typ_field;
	char *val_field;
} Fields;

typedef struct type_Equate {
	Fields fields[256];
	int nb_fields;
	char equ[512];
	int nb_equ;
} Equate;

typedef struct
{
	char nom[LG_THEME+1];
//	char nb_themes;
//	NomTh *lthemes;
	Equate equate;
	int nb_bull;
	long first_bull;
	long last_bull;
} Theme;

typedef struct
{
	long msg_num;
	int grp_num;
} grplu;

#define NBGRPLU 100
typedef struct typ_grpbloc
{
	grplu *grpel;
	int nb_tot;
	int nb_cur;
} grpbloc;

typedef struct typ_th_date
{
	char nom[LG_THEME+1];
	time_t date;
	int trouve;
} th_date;

static grpbloc *group_index[MAX_THEMES];
static th_date theme_date[MAX_THEMES*2];

static int tot_themes;
static int nolig;
static int nb_dates;

int scan_pos;
char *scan_line;

static Theme *theme_liste = NULL;

void end_themes (void)
{
	if (theme_liste)
		libere_theme ();
}

#define CADENCE 5;

void scan_themes (int add)
{
#ifdef __FBBDOS__
	fen *fen_ptr;

#endif
	unsigned offset = 0;
	int nb = 0;
	long last_num = 0L;
	bloc_mess *bptr = tete_dir;
	bullist ligne;


	deb_io ();
#ifdef __FBBDOS__
#ifdef ENGLISH
	fen_ptr = open_win (10, 5, 50, 8, INIT, "Themas");
#else
	fen_ptr = open_win (10, 5, 50, 8, INIT, "Themes");
#endif
#endif

	ouvre_dir ();

	while (bptr)
	{
		if (bptr->st_mess[offset].noenr)
		{
			++nb;
			read_dir (bptr->st_mess[offset].noenr, &ligne);

			if ((ligne.type == 'B') && (READ_OK (ligne)))
			{
				valide_themes (bptr->st_mess[offset].noenr, add, &ligne);
			}
			if ((nb % 100) == 0)
			{
#if defined(__WINDOWS__) || defined(__LINUX__)
				char buf[80];

				sprintf (buf, "%ld", bptr->st_mess[offset].nmess);
				InitText (buf);
#endif
#ifdef __FBBDOS__
				cprintf ("%ld\r", bptr->st_mess[offset].nmess);
#endif
			}
			last_num = bptr->st_mess[offset].nmess;
		}

		if (++offset == T_BLOC_MESS)
		{
			bptr = bptr->suiv;
			offset = 0;
		}
	}

#if defined(__WINDOWS__) || defined(__LINUX__)
	{
		char buf[80];

		sprintf (buf, "%ld", last_num);
		InitText (buf);
	}
#else
	cprintf ("%ld\r", last_num);
#endif

	ferme_dir ();

#ifdef __FBBDOS__
	attend_caractere (2);
	close_win (fen_ptr);
#endif
	fin_io ();
}

int theme_existe (char *theme, char liste[MAX_THEMES][LG_THEME+1])
{
	int i;

	for (i = 0; i < tot_themes; i++)
	{
		if (strcmp (theme, liste[i]) == 0)
			return (1);
	}
	return (0);
}

static void read_dates (void)
{
	int nb;
	FILE *fptr;

	memset(theme_date, sizeof(th_date) * MAX_THEMES * 2, 0);
	
	fptr = fopen (d_disque ("THEMES.DAT"), "rt");
	if (fptr == NULL)
		return;

	for (nb_dates = 0 ; nb_dates < MAX_THEMES ; nb_dates++)
	{
		nb = fscanf(fptr, "%s %ld\n", theme_date[nb_dates].nom, &theme_date[nb_dates].date);
		if (nb != 2)
			break;
		theme_date[nb_dates].trouve = 0;
	}
	
	fclose(fptr);
	
	return;
}

char *check_dates (time_t date)
{
	int i;
	int nb = 0;
	static char buf[4 + (LG_THEME+2) * MAX_THEMES];

	for (i = 0 ; i < nb_dates ; i++)
	{
		if ((theme_date[i].trouve) && (date < theme_date[i].date))
		{
			nb += sprintf(buf+nb, "%s\r\n", theme_date[i].nom);
		}
	}
	sprintf(buf+nb, ".\r\n");

	return buf;
}

static void write_dates (void)
{
	int i;

	FILE *fptr = fopen (d_disque ("THEMES.DAT"), "wt");
	if (fptr == NULL)
		return;

	for (i = 0 ; i < nb_dates ; i++)
	{
		if (theme_date[i].trouve)
			fprintf(fptr, "%s %ld\n", theme_date[i].nom, theme_date[i].date);
	}
	
	fclose(fptr);
	
	return;
}

static void add_date (char *nom)
{
	int i;

	for (i = 0 ; i < nb_dates ; i++)
	{
		if (strcmpi(theme_date[i].nom, nom) == 0)
		{
			theme_date[i].trouve = 1;
			return;
		}
	}
	
	n_cpy (LG_THEME, theme_date[nb_dates].nom, nom);
	theme_date[nb_dates].date = time(NULL);
	theme_date[nb_dates].trouve = 1;
	
	printf("%s", ctime(&theme_date[nb_dates].date));
	nb_dates++;
}

static int new_val(Equate *st, int type, char *value)
{
	st->fields[st->nb_fields].val_field = value;
	st->fields[st->nb_fields].typ_field = type;
	st->equ[st->nb_equ++] = st->nb_fields++;

	return 1;
}

static int new_ope(Equate *st, int operation)
{
	st->equ[st->nb_equ++] = operation;

	return 1;
}

static char *next_word(void)
{
	int nb = 0;

	while (isword(scan_line[scan_pos+nb]))
		++nb;

	if (nb > 0)
	{
		char *word = malloc(nb+1);
		memcpy(word, scan_line+scan_pos, nb);
		word[nb] = '\0';

		scan_pos += nb;

		return word;
	}

	return NULL;
}

/*
return values
0 : OK
1 : unattended character
2 : ')' missing
3 : No memory
*/

static int expression(Equate *st)
{
	int operation;
	int type;

	while (isspace(scan_line[scan_pos]))
		++scan_pos;
	
	/* 1st word */
	if (isope(scan_line[scan_pos]))
	{
		return 1;
	}

	if (scan_line[scan_pos] ==')')
	{
		return 0;
	}
	
	if (scan_line[scan_pos] =='!')
	{
		int ret;
		
		++scan_pos;
		ret = expression(st);
		if (ret != 0)
			return ret;
		new_ope(st, OPE_NOT);
	}
	
	else if (scan_line[scan_pos] == '(')
	{
		int ret;
		
		++scan_pos;
		ret = expression(st);
		if (ret != 0)
			return ret;
		
		if (scan_line[scan_pos] != ')')
			return 2;

		++scan_pos;
	}
	
	else if (isword(scan_line[scan_pos]))
	{
		type = TYP_TO;
		if (scan_line[scan_pos] == '@')
		{
			type = TYP_VIA;
			++scan_pos;
		}
		if (!new_val(st, type, next_word()))
			return 3;
	}
	
	else
	{
		return 1;
	}
	
	operation = OPE_OR;

	/* next words */
	for (;;)
	{
		while (isspace(scan_line[scan_pos]))
			++scan_pos;

		if (scan_line[scan_pos] == '\0')
			return 0;
		
		else if (scan_line[scan_pos] == '&')
		{
			++scan_pos;
			operation = OPE_AND;
			continue;
		}

		else if (scan_line[scan_pos] == '|')
		{
			++scan_pos;
			operation = OPE_OR;
			continue;
		}

		else if (scan_line[scan_pos] =='!')
		{
			int ret;
			
			++scan_pos;
			ret = expression(st);
			if (ret != 0)
				return ret;
			new_ope(st, OPE_NOT);
			continue;
		}
	
		else if (scan_line[scan_pos] == '(')
		{
			int ret;
			
			++scan_pos;
			ret = expression(st);
			if (ret != 0)
				return ret;
			
			if (scan_line[scan_pos] != ')')
				return 2;

			++scan_pos;
		}
		
		else if (scan_line[scan_pos] ==')')
		{
			return 0;
		}
		
		else if (isword(scan_line[scan_pos]))
		{
			type = TYP_TO;
			if (scan_line[scan_pos] == '@')
			{
				type = TYP_VIA;
				++scan_pos;
			}
			if (!new_val(st, type, next_word()))
				return 3;
		}
		
		else
		{
			return 10;
		}

		new_ope(st, operation);

		operation = OPE_OR;
	}
}

void load_themes (void)
{
	int i;
//	int nb;
	FILE *fptr;
	char *ptr;
	char buffer[258];
//	NomTh tmp_nom[100];
//	NomTh *pnom;
	struct stat st;
	char nom_themes[MAX_THEMES][LG_THEME+1];

	if ((stat (c_disque ("THEMES.SYS"), &st) == 0) && (st.st_mtime == t_thm))
		return;

	t_thm = st.st_mtime;

	if (theme_liste)
	{
		libere_theme ();
		theme_liste = NULL;
	}

	for (i = 0 ; i < MAX_THEMES ; i++)
		group_index[i] = NULL;

	nolig = 0;
	tot_themes = 0;

	fptr = fopen (c_disque ("THEMES.SYS"), "rt");
	if (fptr == NULL)
	{
		return;
	}

	while (fgets (buffer, 257, fptr))
	{
		++nolig;
		ptr = strtok (buffer, " \t\n");
		if ((ptr == NULL) || (*ptr == '#'))
			continue;

		if (theme_existe (ptr, nom_themes))
		{
			fclose (fptr);
			error_file ();
			tot_themes = 0;
			return;
		}

		n_cpy (10, nom_themes[tot_themes], ptr);

		++tot_themes;

		if (tot_themes == MAX_THEMES)
			break;
	}

	if (tot_themes == 0)
	{
		fclose (fptr);
		return;
	}

	theme_liste = (Theme *) m_alloue (tot_themes * sizeof (Theme));
	
	rewind (fptr);

	read_dates ();
	
	tot_themes = 0;

	while (fgets (buffer, 257, fptr))
	{
		int ret;

		sup_ln (buffer);
		ptr = strtok (buffer, " \t");
		if ((ptr == NULL) || (*ptr == '#'))
			continue;

		n_cpy (LG_THEME, theme_liste[tot_themes].nom, ptr);
		add_date(ptr);
		
		scan_line = strtok (NULL, "\n");
		scan_pos  = 0;
		
		if (scan_line)
		{
			ret = expression(&theme_liste[tot_themes].equate);
			if (ret != 0)
			{
				printf("error %d - theme \"%s\" (col %d)\n", 
						ret, theme_liste[tot_themes].nom, (int)strlen(ptr)+scan_pos+1);
				printf("%s\n", scan_line);
				for (i = 0 ; i < scan_pos ; i++)
					putchar(' ');
				printf("^\n");
			}
		}
/*
		nb = 0;
		while ((ptr = strtok (NULL, " \t")) != NULL)
		{
			n_cpy (6, tmp_nom[nb], ptr);
			++nb;
		}
		if (nb)
			pnom = (NomTh *) m_alloue (nb * sizeof (NomTh));
		else
			pnom = NULL;
		for (i = 0; i < nb; i++)
			strcpy (pnom[i], tmp_nom[i]);
	
		theme_liste[tot_themes].lthemes = pnom;
		theme_liste[tot_themes].nb_themes = nb;
*/

		theme_liste[tot_themes].nb_bull = 0;
		theme_liste[tot_themes].first_bull = 1L;
		theme_liste[tot_themes].last_bull = 0L;

		++tot_themes;
		if (tot_themes == MAX_THEMES)
			break;
	}

	write_dates ();

	fclose (fptr);

	scan_themes (0);
	scan_themes (1);
}

static int add_to_group(int add, int group, bullist *pbul)
{
	if (group_index[group] == NULL)
	{
		group_index[group] = (grpbloc *)calloc(sizeof(grpbloc), 1);
		if (group_index[group] == NULL)
			return 0;
		group_index[group]->nb_tot = NBGRPLU;
		group_index[group]->grpel = (grplu *)calloc(sizeof(grplu), group_index[group]->nb_tot);
		if (group_index[group]->grpel == NULL)
			return 0;
	}

	++theme_liste[group].nb_bull;

	if (add || pbul->grpnum <= theme_liste[group].last_bull)
	{
		pbul->grpnum = ++theme_liste[group].last_bull;
		pbul->theme = 0L;
	}

	if (theme_liste[group].nb_bull == 1)
		theme_liste[group].first_bull = pbul->grpnum;

	theme_liste[group].last_bull = pbul->grpnum;
		
	group_index[group]->grpel[group_index[group]->nb_cur].msg_num = pbul->numero;
	group_index[group]->grpel[group_index[group]->nb_cur].grp_num = pbul->grpnum;
	
	++group_index[group]->nb_cur;

	if (group_index[group]->nb_cur == group_index[group]->nb_tot)
	{
		group_index[group]->nb_tot += NBGRPLU;
		group_index[group]->grpel = (grplu *)realloc(group_index[group]->grpel, sizeof(grplu) * group_index[group]->nb_tot);
		if (group_index[group]->grpel == NULL)
			return 0;
	}

	return 1;
}

static long grp_to_bull(int group, int grp_num)
{
	int i;

	if (group_index[group] == NULL){
		return 0l;
	}

	
	for (i = 0 ; i < group_index[group]->nb_cur ; i++)
	{
		if (group_index[group]->grpel[i].grp_num == grp_num)
			return group_index[group]->grpel[i].msg_num;
	}
	
	return 0L;
}

int th_check(Equate *st, bullist * pbul)
{
#define TH_STACK 256

	char *to = pbul->desti;
	char *via = pbul->bbsv;
	char stack[TH_STACK+1];
	int stpos = -1;
//	int res = 0;
	int i;
	int val;

	stack[0] = 0;

	for (i = 0 ; i < st->nb_equ ; i++)
	{
		int x = st->equ[i];

		if (x > OPE)
		{
			/*
			char *op[] = {"ENTER", "OR", "AND" };
			int t = -x - 1;
			printf(" %s ", op[t]);
			*/

			if (stpos <= 0)
				return -1;

			switch (x)
			{
			case OPE_NOT:
				val = stack[stpos];
				stack[stpos] = !val;
				break;
			case OPE_AND:
				val = stack[stpos--];
				stack[stpos] &= val;
				break;
			case OPE_OR:
				val = stack[stpos--];
				stack[stpos] |= val;
				break;
			}
		}
		else
		{
			int cur = 0;

			if (stpos == TH_STACK)
				return -1;

			/*	printf("%d %s", st->fields[x].typ_field, st->fields[x].val_field); */
			if (st->fields[x].typ_field == TYP_TO)
				cur = strmatch(to, st->fields[x].val_field);
			else if (st->fields[x].typ_field == TYP_VIA)
				cur = strmatch(via, st->fields[x].val_field);
			stack[++stpos] = cur;
		}

		/* printf("\n"); */
	}

	/* printf("stack = %d, result = %d\n", stpos, stack[stpos]); */

	return (stack[stpos]);
}

/*
 *
 * First pass (add = 0) : Validate all already well classed bulletins
 * Second pass (add = 1): Validate unclassed or new bulletins 
 *
 */

void valide_themes (unsigned noenr, int add, bullist * pbul)
{
	int i;
//	, j;
	int update = 0;
/*	long val = 1L;*/
	long masque = -1L;

	if (theme_liste == NULL)
		return;
		
	for (i = 1; i < tot_themes; i++)
	{
		/*
		for (j = 0; j < theme_liste[i].nb_themes; j++)
		{
			if (strmatch (pbul->desti, theme_liste[i].lthemes[j]))
			{
				masque = i;
				break;
			}
		}

		if (masque != -1L)
			break;
		*/

		if (th_check(&theme_liste[i].equate, pbul))
		{
			/* No duplicate */
			masque = i;
			break;
		}
	}

	if (add == 0)
	{
		/* masque must be equal to the theme */
		if (pbul->theme == masque)
			update = 1;
	}
	else
	{
		/* unclassed or new bulletins */
		if (pbul->theme != masque)
			update = 1;
	}

	if (update)
	{
		int group = (masque == -1L) ? 0 : (int)masque;
		
		add_to_group(add, group, pbul);

		if (pbul->theme != masque)
		{
			pbul->theme = masque;
			if (noenr != 0)
				write_dir (noenr, pbul);
		}
	}
}

static void libere_theme (void)
{
	int i;

	if (theme_liste == NULL)
		return;

	for (i = 0; i < NBVOIES; i++)
	{
		if ((svoie[i]->niv1 == N_THEMES) && (svoie[i]->niv2 != 0))
		{
			selvoie (i);
			pvoie->finf.theme = 0;
			prompt_themes ();
		}
	}

	for (i = 0; i < tot_themes; i++)
	{
		int j;
		
		Equate *st = &theme_liste[i].equate;
		for (j = 0 ; j < st->nb_fields; j++)
		{
			free(st->fields[j].val_field);
		}
/*
		if (theme_liste[i].nb_themes)
		{
			m_libere (theme_liste[i].lthemes,
					  theme_liste[i].nb_themes * sizeof (NomTh));
		}
*/
	}
	m_libere (theme_liste, tot_themes * sizeof (Theme));
	theme_liste = NULL;
	tot_themes = 0;
}

char *cur_theme (int voie)
{
	return (theme_liste[(int)svoie[voie]->finf.theme].nom);
}

int nbull_theme (int voie)
{
	return (theme_liste[(int)svoie[voie]->finf.theme].nb_bull);
}

static void prompt_themes (void)
{
	maj_niv (N_THEMES, 0, 0);

	pvoie->sr_mem = 0;

	texte (T_THE + 2);
}

static void list_topics (void)
{
/*
	int i;
	char s[100];
	int numero = pvoie->finf.theme;

	for (i = 0; i < theme_liste[numero].nb_themes; i++)
	{
		sprintf (s, " %-6s", theme_liste[numero].lthemes[i]);
		out (s, strlen (s));

		if ((i + 1) % 8 == 0)
			cr ();
	}
	if (i % 8)
		cr ();
*/
}

static void display_themes (void)
{
	int i;
	char s[100];

	for (i = 0; i < tot_themes; i++)
	{
		sprintf (s, "%2d:%-25s %-4d", i, theme_liste[i].nom, theme_liste[i].nb_bull);
		out (s, strlen (s));
		if ((i + 1) % 2 == 0)
			cr ();
		else
			out ("     ", 5);
	}
	if (i % 2)
		cr ();

	prompt_themes ();
}

static char *ltitre (int mode, bullist * pbul)	/* Mode = 1 pour LS */
{
	/*  int lg = (mode) ? 80 : 36 ; */
	int lg = 80;
	static char buf[100];

	if (pvoie->typlist)
	{
		sprintf (buf, "%-12s ", pbul->bid);
		if (mode)
			strn_cpy (lg, buf + 13, pbul->titre);
		else
			n_cpy (lg, buf + 13, pbul->titre);
	}
	else
	{
		if (mode)
			strn_cpy (lg, buf, pbul->titre);
		else
			n_cpy (lg, buf, pbul->titre);
	}
	return (buf);
}

static void alloue_list_numeros (void)
{
	if (pvoie->ptemp)
	{
		m_libere ((char *) pvoie->ptemp, pvoie->psiz);
		pvoie->ptemp = NULL;
		pvoie->psiz = 0;
	}

	pvoie->psiz = pvoie->temp2 * sizeof (long);

	pvoie->ptemp = m_alloue (pvoie->psiz);
}

static void aff_ligne (int numero, bullist * ligne)
{
	char *ptr;

	*ptmes = *ligne;

	if (*(ligne->bbsv))
		sprintf (varx[0], "@%-6s", bbs_via (ligne->bbsv));
	else
		strcpy (varx[0], "       ");
	var_cpy (1, ltitre (0, ligne));
	ptmes->numero = (long) numero;

	ptr = var_txt (langue[vlang]->plang[T_THE + 1 - 1]);
	if (strlen (ptr) > 80)
	{
		ptr[79] = '\r';
		ptr[80] = '\0';
	}
	outs (ptr, strlen (ptr));
}

static int entete_theme (void)
{
	return (texte (T_THE + 0));
}

static int themes_bloc_liste (void)
{
	long *num_list;
	int retour = 1;
	int num_lig = 0;
	int max_lig;
	int group;
	unsigned offset = pvoie->recliste.offset;
	bloc_mess *bptr = pvoie->recliste.ptemp;
	bullist ligne;

	num_list = (long *) pvoie->ptemp;
	if ((num_list == NULL) || (pvoie->psiz == 0))
		return (retour);

	pvoie->sr_mem = pvoie->seq = FALSE;

	group = (pvoie->finf.theme == 0) ? -1 : pvoie->finf.theme;

	max_lig = pvoie->psiz / sizeof (long);

	num_lig = max_lig - pvoie->temp2;

	ouvre_dir ();

	while (bptr)
	{

		/* Pb debordement du tableau ! */
		if (num_lig >= max_lig)
		{
			retour = 2;
			break;
		}

		if (!pvoie->reverse)
			--offset;

		if (bptr->st_mess[offset].noenr)
		{
			read_dir (bptr->st_mess[offset].noenr, &ligne);

			if ((ligne.type == 'B') && (READ_OK (ligne)))
			{
				if (ligne.theme == group)
				{
					if (pvoie->temp1)
					{
						pvoie->temp2 -= entete_theme ();
						pvoie->temp1 = 0;
					}
					num_list[num_lig++] = ligne.numero;
					aff_ligne (num_lig, &ligne);
					--pvoie->temp2;
				}
			}
			if (pvoie->temp2 == 0)
			{
				retour = 2;
				break;
			}
		}

		if (pvoie->reverse)
		{
			if (++offset == T_BLOC_MESS)
			{
				bptr = bptr->suiv;
				offset = 0;
			}
		}
		else
		{
			if (offset == 0)
			{
				bptr = prec_dir (bptr);
				offset = T_BLOC_MESS;
			}
		}

		if (pvoie->memoc >= MAXMEM)
		{
			pvoie->sr_mem = TRUE;
			retour = 0;
			break;
		}
		if (trait_time > MAXTACHE)
		{
			pvoie->seq = TRUE;
			retour = 0;
			break;
		}

	}

	ferme_dir ();

	pvoie->recliste.offset = offset;
	pvoie->recliste.ptemp = bptr;

	return (retour);
}

int themes_lx (void)
{
	pvoie->temp1 = 1;
	if (pvoie->reverse)
	{
		pvoie->recliste.ptemp = tete_dir;
		pvoie->recliste.offset = 0;
	}
	else
	{
		pvoie->recliste.ptemp = last_dir ();
		pvoie->recliste.offset = T_BLOC_MESS;
	}
	return (1);
}

int themes_rx (int verbose)
{
	int error = 0;
	long no;
	int c, ok = TRUE;
	bullist *pbul;
	rd_list *ptemp = NULL;

	df ("themes_rx", 1);
/*
   print_fonction(stdout); print_history(stdout); sleep_(10);
   for (;;);
 */
	sup_ln (indd);
	c = toupper (*indd);
	++indd;

	if ((c != ' ') && (*indd != ' ') && (*indd != '\0'))
	{
		ff ();
		return (1);
	}

	pvoie->aut_nc = 1;

	libere_tread (voiecur);
	init_recliste (voiecur);
	pvoie->recliste.l = verbose;

	switch (c)
	{

		/*
		   case 'A' :
		   pvoie->recliste.status = 'A' ;
		   break ;
		   case 'L' :
		   if (teste_espace()) {
		   if (isdigit(*indd))
		   pvoie->recliste.last = lit_chiffre(0) ;
		   else {
		   texte(T_ERR + 3) ;
		   ok = 0 ;
		   }
		   } else {
		   --indd;
		   error = 1;
		   ok = 4 ;
		   }
		   break ;
		   case 'M' :
		   case 'N' :
		   if (read_mine(c))
		   ok = 2 ;
		   else
		   ok = 0 ;
		   break ;
		   case 'S' :
		   if (teste_espace())
		   strn_cpy(19, pvoie->recliste.find, indd) ;
		   else {
		   --indd;
		   error = 1;
		   ok = 4 ;
		   }
		   break ;
		   case '<' :
		   if (teste_espace()) {
		   strn_cpy(6, pvoie->recliste.exp, indd) ;
		   } else {
		   texte(T_ERR + 2) ;
		   ok = 0 ;
		   }
		   break ;
		   case '>' :
		   if (teste_espace()) {
		   strn_cpy(6, pvoie->recliste.dest, indd) ;
		   } else {
		   texte(T_ERR + 2) ;
		   ok = 0 ;
		   }
		   break ;
	 */ case ' ':
		trans_num ();
		/*if (strchr(indd, '-')) {
		   if (isdigit(*indd))
		   pvoie->recliste.debut = lit_chiffre(1) ;
		   else {
		   texte(T_ERR + 3) ;
		   ok = 0 ;
		   break ;
		   }
		   ++indd ;
		   if (isdigit(*indd))
		   pvoie->recliste.fin = lit_chiffre(1) ;
		   if (pvoie->recliste.fin <= pvoie->recliste.debut)
		   ok = 0 ;
		   } else {         */
		ok = 0;
		while ((no = lit_chiffre (1)) != 0L)
		{
			if ((pbul = ch_record (NULL, no, 'Y')) != NULL)
			{
				if (droit_ok (pbul, 1))
				{
					if (ptemp)
					{
						ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
						ptemp = ptemp->suite;
					}
					else
					{
						pvoie->t_read = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
					}
					ptemp->suite = NULL;
					ptemp->nmess = no;
					ptemp->verb = verbose;
					ok = 2;
				}
				else
					texte (T_ERR + 10);
			}
			else
				texte (T_ERR + 10);
		}
		/* }                  */
		break;
	default:
		if ((c == '\0') && (verbose))
		{
			texte (T_MBL + 8);
			ok = 0;
		}
		else
		{
			error = 1;
			--indd;
			ok = 4;
		}
		break;
	}
	switch (ok)
	{
	case 0:
		prompt_themes ();
		break;
	case 1:
		pvoie->recliste.ptemp = last_dir ();
		pvoie->recliste.offset = T_BLOC_MESS;
		pvoie->temp1 = 1;
		pvoie->sr_mem = 1;
		ch_niv3 (1);
		themes_read (verbose);
		break;
	case 2:
		pvoie->sr_mem = 1;
		ch_niv3 (2);
		themes_read (verbose);
		break;
	}
	ff ();
	return (error);
}

static int themes_read (int verbose)
{
	int error = 0;

	df ("themes_read", 1);

	switch (pvoie->niv3)
	{
	case 0:
		++indd;
		error = themes_rx (verbose);
		break;
	case 1:
		switch (mbl_bloc_list ())
		{
		case 0:				/* Pas de message */
			texte (T_MBL + 3);
			prompt_themes ();
		case 1:				/* Pas fini */
			break;
		case 2:				/* Termine */
			ch_niv3 (2);
			mbl_read (verbose);
			break;
		}
		break;
	case 2:
		if (mbl_mess_read () == 0)
		{
			prompt_themes ();
		}
		break;
	case 3:
		if (read_mess (1) == 0)
			ch_niv3 (2);
		break;
	default:
		fbb_error (ERR_NIVEAU, "MSG-READ", pvoie->niv3);
	}
	ff ();
	return (error);
}


static void trans_num (void)
{
	char s[256];
	char buf[80];
	char *ptr;
	int num;
	int i;

	n_cpy (255, s, indd);
	*indd = '\0';

	ptr = s;
	while (*ptr)
	{
		if (isdigit (*ptr))
		{
			i = 0;
			while (isdigit (*ptr))
			{
				buf[i++] = *ptr++;
			}
			buf[i] = '\0';

			num = atoi (buf) - 1;
			if ((num >= 0) && (num < (pvoie->psiz / sizeof (long))))
			{
				long lnum;

				lnum = ((long *) pvoie->ptemp)[num];
				strcat (indd, ltoa (lnum, buf, 10));
			}
			else
			{
				/* Erreur */
			}
		}
		else
		{
			buf[0] = *ptr++;
			buf[1] = '\0';
			strcat (indd, buf);
		}
	}
}

int themes_list (void)
{
	int mode_list = 1;
	int verbose = 0;
	int error = 0;

	switch (pvoie->niv3)
	{
	case 0:
		if (toupper (*indd) == 'C')
		{
			return (list_lc ());
		}

		switch (themes_lx ())
		{
		case 0:
			prompt_themes ();
		case 2:
			mode_list = 0;
			break;
		case 1:
			pvoie->temp2 = nbl_page (voiecur);
			alloue_list_numeros ();
			ch_niv3 (1);
			break;
		case 3:
			mode_list = 0;
			error = 1;
			--indd;
			break;
		}
		break;
	case 1:
		break;
	case 2:
		while_space ();
		switch (toupper (*indd))
		{
		case 'A':
			mode_list = 0;
			prompt_themes ();
			break;
		case 'V':
			verbose = 1;
		case 'R':
			mode_list = 0;
			incindd ();
			if (isdigit (*indd))
			{
				trans_num ();
				pvoie->aut_nc = 1;
				list_read (verbose);
				pvoie->sr_mem = 1;
				if (mbl_mess_read () == 0)
				{
					pvoie->sr_mem = 0;
					texte (T_QST + 6);
				}
			}
			else
			{
				texte (T_ERR + 3);
				texte (T_QST + 6);
			}
			break;
		default:
			pvoie->temp2 = nbl_page (voiecur);
			alloue_list_numeros ();
			ch_niv3 (1);
			break;
		}
		break;
	case 3:
		mode_list = 0;
		pvoie->aut_nc = 1;
		if (read_mess (1) == 0)
			ch_niv3 (4);
		break;
	case 4:
		mode_list = 0;
		pvoie->aut_nc = 1;
		pvoie->sr_mem = 1;
		if (mbl_mess_read () == 0)
		{
			pvoie->sr_mem = 0;
			texte (T_QST + 6);
			ch_niv3 (2);
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "MSG-LIST", pvoie->niv3);
	}

	if (mode_list)
	{
		pvoie->lignes = -1;
		switch (themes_bloc_liste ())
		{
		case 0:
			break;
		case 1:
			prompt_themes ();
			break;
		case 2:
			texte (T_QST + 6);
			ch_niv3 (2);
			break;
		}
	}
	return (error);
}

int nom_theme (char *nom)
{
	int i;

	strupr (sup_ln (nom));

	if (strlen(nom) > 2)
	{
		for (i = 0; i < tot_themes; i++)
		{
			if (strncmpi (theme_liste[i].nom, nom, strlen(nom)) == 0)
			{
				pvoie->finf.theme = i;
				list_topics ();
				prompt_themes ();
				return (1);
			}
		}
	}
	return (0);
}

void theme_err (char *ptri)
{
	int i = 0;
	char *ptr = varx[0];

	while (ISGRAPH (*ptri))
	{
		if (++i == 40)
			break;
		else
			*ptr++ = *ptri++;
	}
	*ptr = '\0';
	texte (T_ERR + 1);
	*varx[0] = '\0';
	if ((FOR (pvoie->mode)) || (++pvoie->nb_err == MAX_ERR))
		pvoie->deconnect = 6;
	else
		prompt_themes ();
}


int menu_themes (void)
{
	int error = 0;
	int verbose = 0;
	char *com = indd;


	if (tot_themes == 0)
	{
		texte (T_DOS + 2);
		retour_mbl ();
		return (error);
	}

	if (nom_theme (indd))
		return (error);

	switch (toupper (*indd))
	{
	case 'H':
		display_themes ();
		break;
	case 'Q':
	case 'F':
		retour_mbl ();
		break;
	case 'L':
		ch_niv2 (1);
		error = themes_list ();
		break;
	case 'V':
		verbose = 1;
	case 'R':
		ch_niv2 (2);
		error = themes_read (verbose);
		break;
	case 'B':
		maj_niv (N_MENU, 0, 0);
		sortie ();
		break;
	default:
		if (isdigit (*indd))
		{
			int select = atoi (indd);

			if ((select >= 0) && (select < tot_themes) && (theme_liste[select].nb_bull))
			{
				pvoie->finf.theme = select;
				list_topics ();
			}
			else
			{
				texte (T_DOS + 2);
			}
			prompt_themes ();
		}
		else if (!ISGRAPH (*indd))
		{
			display_themes ();
		}
		else
		{
			error = 1;
		}
		break;
	}

	if (error)
		theme_err (com);

	return (error);
}

int themes (void)
{
	int ret = 0;

	if (POP (no_port(voiecur)))
	{
		incindd ();
	
		switch (*indd)
		{
		case 'G':
			/* Select group "G name" */
			incindd();
			group_select(indd);
			break;
		case 'P':
			/* Select previous bulletin in the theme */
			group_previous();
			break;
		case 'N':
			/* Select next bulletin in the theme */
			group_next();
			break;
		case 'R':
			/* Read a bulletin number "R mode #nb" */
			incindd();
			group_read(indd);
			break;
		case 'H':
			/* Read a bulletin number "R mode #nb" */
			incindd();
			group_xhdr(indd);
			break;
		case 'O':
			/* Read a bulletin number "R mode #nb" */
			incindd();
			group_xover(indd);
			break;
		default :
			ret = 1;
			break;
		}
		retour_mbl();
		return ret;
	}
	
	switch (pvoie->niv2)
	{
	case 0:
		ret = menu_themes ();
		break;
	case 1:
		ret = themes_list ();
		break;
	case 2:
		ret = themes_read (0);
		break;
	default :
		ret = 0;
		break;
	}

	return (ret);
}


static void error_file (void)
{
	char wtexte[200];

	deb_io ();
#ifdef ENGLISH
	if (operationnel)
	{
		sprintf (wtexte, "\r\nError in file THEMES.SYS line %d  \r\n\a", nolig);
		if (w_mask & W_FILE)
			mess_warning (admin, "*** FILE ERROR ***    ", wtexte);
	}
	sprintf (wtexte, "Error in file THEMES.SYS line %d  ", nolig);
#else
	if (operationnel)
	{
		sprintf (wtexte, "\r\nErreur fichier THEMES.SYS ligne %d\r\n\a", nolig);
		if (w_mask & W_FILE)
			mess_warning (admin, "*** ERREUR FICHIER ***", wtexte);
	}
	sprintf (wtexte, "Erreur fichier THEMES.SYS ligne %d", nolig);
#endif
	fin_io ();
	win_message (5, wtexte);
}

/* Functions for the NNTP server */

static int n_theme;

static int pos_theme(char *nom)
{
	int i;

	strupr (sup_ln (nom));

	for (i = 0; i < tot_themes; i++)
	{
		if (strcmpi (theme_liste[i].nom, nom) == 0)
			return i;
	}
	return -1;
}

char *next_group (void)
{
	static char cur_buf[80];
	
	if (n_theme >= tot_themes)
		return NULL;

	sprintf(cur_buf, "%s %ld %ld %c", 
			theme_liste[n_theme].nom, 
			theme_liste[n_theme].last_bull,
			theme_liste[n_theme].first_bull,
			'n');

	++n_theme;
	return cur_buf;
}

char *first_group (void)
{
	load_themes();
	n_theme = 0;
	return next_group();
}

char *get_group_info(char *nom, char *buffer)
{
	int val = pos_theme (nom);
	
	if (val == -1)
		return NULL;
		
	sprintf(buffer, "%d %ld %ld %s",
			theme_liste[val].nb_bull,
			theme_liste[val].first_bull,
			theme_liste[val].last_bull,
			theme_liste[val].nom);
	return buffer;
}

static int group_st(char *buffer, long nbul, int mode)
{
	char *strm[4] = {"text follows", "head follows", "body follows", "statistics"};
	int intm[4] = {220, 221, 222, 223};
	bullist *pbul = NULL;
	int cur_theme = pvoie->groupe;
	int ret = 0;
	
	if (cur_theme == -1)
	{
		strcpy(buffer, "412 No group selected");
	}
	else if (nbul == -1L)
	{
		strcpy(buffer, "420 No current bulletin");
	}
	else
	{
		long numero;
		int group = (cur_theme == 0) ? -1 : cur_theme;
		
		numero = grp_to_bull(cur_theme, nbul);
		pbul = ch_record (NULL, numero, ' ');
		
		if (pbul == NULL)
			strcpy(buffer, "430 No such bulletin");
		else if (pbul->theme != group)
			strcpy(buffer, "423 No such bulletin in the current group");
		else
		{
			sprintf(buffer, "%d %ld <%ld@%s> - %s",
					intm[mode],
					nbul,
					numero,
					mycall, 
					strm[mode]);

			pvoie->cur_bull = nbul;

			ret = 1;
		}
	}
	return (ret);
}

static int get_range(char *ptr, long *first, long *last)
{
	char *scan;

	*first = 0;
	*last = 0x7fffffff;
	
	if (*ptr)
	{
		*first = atol(ptr);
		scan = strchr(ptr, '-');
		if (scan)
		{
			++scan;
			if (isdigit(*scan))
				*last = atol(scan);
		}
		else
		{
			*last = *first;
		}
	}
	return 1;
}

static void send_overview(long numero, bullist *pbul)
{
	char buffer[256];
	int nb;
	/* I simplified the Subject line just to remove a little clutter */ 
	nb = sprintf(buffer, "%ld\t%s\t%s\t%s\t<%ld@%s>\t\t%ld\t%d",
		numero,
		pbul->titre,
		pbul->exped,
		pop_date(pbul->datesd),
		pbul->numero, mycall,
		/* reference, ? */
		pbul->taille,
		1);
	outsln(buffer, nb);
}

static void group_xover(char *ptr)
{
	char buffer[80];
	long nbul;
	long numero;
	bullist *pbul;
	long first, last;
	int cur_theme;
	
	cur_theme = (int)pvoie->groupe;
	if (cur_theme == -1)
	{
		strcpy(buffer, "412 No group selected\r\032");
	}
	else
	{		
		if (get_range(ptr, &first, &last))
		{
			if (last > theme_liste[cur_theme].last_bull)
				last = theme_liste[cur_theme].last_bull;

			strcpy(buffer, "224 Overview information");
			outsln(buffer, strlen(buffer));

			ouvre_dir ();
				
			for (nbul = first ; nbul <= last ; nbul++)
			{
				if (nbul < theme_liste[cur_theme].first_bull || nbul > theme_liste[cur_theme].last_bull)
					continue;
					
				numero = grp_to_bull(cur_theme, nbul);
				if (numero > 0L)
				{
					pbul = ch_record (NULL, numero, ' ');
					send_overview(nbul, pbul);
				}
			}
			ferme_dir ();
			outsln("\033\r", 2);
			
			return;

		}
		else
		{
			strcpy(buffer, "420 No bulletin selected\r\032");
		}
	}
	outsln(buffer, strlen(buffer));
}

static void group_select(char *ptr)
{
	int val = pos_theme (ptr);
	int nb;
	char buf[80];
	
	if (val == -1)
	{
		nb = sprintf(buf, "411 No such group");		
	}
	else
	{
		pvoie->groupe = val;
		nb = sprintf(buf, "211 %d %ld %ld %s",
			theme_liste[val].nb_bull,
			theme_liste[val].first_bull,
			theme_liste[val].last_bull,
			theme_liste[val].nom);
	}
	outsln(buf, nb);
}

static void group_read(char *ptr)
{
	char buffer[80];
	char *scan;
	int mode;
	int ret;
	long numero = pvoie->cur_bull;
	
	scan = strchr(ptr, '<');
	if (scan)
	{
		/* Get by ID */
		sscanf(ptr, "%d", &mode);
		++scan;
		numero = atol(scan);
	}
	else
	{
		/* Get by number */
		sscanf(ptr, "%d %ld", &mode, &numero);
	}
	
	ret = group_st(buffer, numero, mode);
	outsln(buffer, strlen(buffer));
	
	if (ret)
	{
		if (!scan)
		{
			/* update the current bulletin number */
			pvoie->cur_bull = numero;
		}

		if (mode != 3)
		{
			int cur_theme = pvoie->groupe;
			int group = (cur_theme == -1L) ? 0 : cur_theme;
			long nbul = grp_to_bull(group, numero);

			sprintf(indd, " %ld\r", nbul);
			mbl_read (0);
		}
	}
	else
	{
		outsln("\032\r", 2);
	}
}

static void group_next(void)
{
	char buffer[80];
	int trouve = 0;
	int cur_theme = pvoie->groupe;
	long nbul = pvoie->cur_bull;
	
	if (cur_theme == -1)
	{
		strcpy(buffer, "412 No group selected");
	}
	else if (nbul == -1L)
	{
		strcpy(buffer, "420 No current bulletin");
	}
	else
	{
		while (nbul < theme_liste[cur_theme].last_bull)
		{
			++nbul;
			if (grp_to_bull(cur_theme, nbul) != 0L)
			{
				trouve = 1;
				break;
			}
		}
		if (trouve)
		{
				group_st(buffer, nbul, 3);	
		}
		else
		{
				strcpy(buffer, "422 No next bulletin in group");
		}
	}
	outsln(buffer, strlen(buffer));
}

static void group_previous(void)
{
	char buffer[80];
	int trouve = 0;
	int cur_theme = pvoie->groupe;
	long nbul = pvoie->cur_bull;
	
	if (cur_theme == -1)
	{
		strcpy(buffer, "412 No group selected");
	}
	else if (nbul == -1L)
	{
		strcpy(buffer, "420 No current bulletin");
	}
	else
	{
		while (nbul > theme_liste[cur_theme].first_bull)
		{
			--nbul;
			if (grp_to_bull(cur_theme, nbul) != 0L)
			{
				trouve = 1;
				break;
			}
		}
			
		if (trouve)
		{
			group_st(buffer, nbul, 3);	
		}
		else
		{
			strcpy(buffer, "422 No previous bulletin in group");
		}
	}
	outsln(buffer, strlen(buffer));
}

static void group_xhdr(char *ptr)
{
/*
	char head[80];
	char range[80];
	
	sscanf(ptr, "%s %s", head, range);
	if (strcmpi(head, "subject) != 0)
*/
}
