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

#define RECSIZ 256

static char *fic (char *);

static int cmd_edit (void);
static int edit_find (char *);
static int edit_replace (char *, char *);
static int ouvre_temp (void);

static void cree_temp (void);
static void edit_insert (char *);
static void edit_append (char *);
static void edit_kill (int);
static void edit_aff (int);
static void kill_temp (void);
static void prompt_edit (void);
static void sauve_temp (void);


int cmd_edit (void)
{
	int c, nb, moins;
	char chaine1[300];
	char chaine2[300];
	char *pbuf;

	sup_ln (indd);

	while (*indd)
	{

		moins = 0;
		if (*indd == '+')
			++indd;
		else if (*indd == '-')
		{
			moins = 1;
			++indd;
		}

		if (isdigit (*indd))
		{
			nb = 0;
			while (isdigit (*indd))
			{
				nb *= 10;
				nb += (*indd++ - '0');
			}
		}
		else
			nb = 1;
		if (moins)
			nb = -nb;

		c = *indd++;
		switch (toupper (c))
		{

		case ' ':
			break;

		case '?':
			outln ("A,B,E,F,I,K,L,N,P,R,S,Q,?", 25);
			break;

		case 'A':
			pbuf = chaine1;
			while ((c = *indd++) != 0)
			{
				if (c == '\\')
					c = *indd++;
				else if (c == '/')
					break;
				*pbuf++ = c;
			}
			*pbuf = '\0';
			if (pvoie->tete_edit.max == 0)
				edit_insert (chaine1);
			else
				edit_append (chaine1);
			break;

		case 'B':
			pvoie->tete_edit.ligne = 1;
			break;

		case 'E':
			pvoie->tete_edit.ligne = pvoie->tete_edit.max;
			if (pvoie->tete_edit.ligne < 1)
				pvoie->tete_edit.ligne = 1;
			break;

		case 'F':
			pbuf = chaine1;
			while ((c = *indd++) != 0)
			{
				if (c == '\\')
					c = *indd++;
				else if (c == '/')
					break;
				*pbuf++ = c;
			}
			*pbuf = '\0';
			if (edit_find (chaine1) == 0)
			{
				texte (T_ERR + 0);
				return (1);
			}
			break;

		case 'I':
			pbuf = chaine1;
			while ((c = *indd++) != 0)
			{
				if (c == '\\')
					c = *indd++;
				else if (c == '/')
					break;
				*pbuf++ = c;
			}
			*pbuf = '\0';
			edit_insert (chaine1);
			break;

		case 'K':
			edit_kill (nb);
			break;

		case 'L':
			pvoie->tete_edit.ligne += nb;
			if (pvoie->tete_edit.ligne < 1)
				pvoie->tete_edit.ligne = 1;
			if (pvoie->tete_edit.ligne > pvoie->tete_edit.max)
				pvoie->tete_edit.ligne = pvoie->tete_edit.max;
			break;

		case 'N':
			pvoie->tete_edit.numero = !pvoie->tete_edit.numero;
			break;

		case 'P':
			edit_aff (nb);
			break;

		case 'R':
			pbuf = chaine1;
			while ((c = *indd++) != 0)
			{
				if (c == '\\')
					c = *indd++;
				else if (c == '/')
					break;
				*pbuf++ = c;
			}
			*pbuf = '\0';
			pbuf = chaine2;
			while ((c = *indd++) != 0)
			{
				if (c == '\\')
					c = *indd++;
				else if (c == '/')
					break;
				*pbuf++ = c;
			}
			*pbuf = '\0';
			if (edit_replace (chaine1, chaine2) == 0)
			{
				texte (T_ERR + 0);
				return (1);
			}
			break;

		case 'S':
			sauve_temp ();
			break;

		case 'Q':
			libere_edit (voiecur);
			return (0);

		default:
			varx[0][0] = c;
			varx[0][1] = '\0';
			texte (T_ERR + 1);
			return (1);

		}

	}
	return (1);
}


void prompt_edit (void)
{
	outs ("EDIT>", 5);
}


int ouvre_temp (void)
{
	int ftemp;
	char nom_temp[80];

	sprintf (nom_temp, "TEMP_%d.$$$", voiecur);
	if ((ftemp = open (nom_temp, O_RDWR | O_BINARY)) == -1)
		fbb_error (ERR_OPEN, nom_temp, 1);
	return (ftemp);
}


void kill_temp (void)
{
	char nom_temp[80];

	sprintf (nom_temp, "TEMP_%d.$$$", voiecur);
	unlink (nom_temp);
}


static char *fic (char *nomfic)
{
	char *ptr;

	if ((ptr = strrchr (nomfic, '\\')) != NULL)
		return (ptr + 1);
	else
		return (nomfic);
}


void libere_edit (int voie)
{
	edit_ch *edit_temp, *ch_ptr;

	edit_temp = svoie[voie]->tete_edit.liste;

	while ((ch_ptr = edit_temp) != NULL)
	{
		edit_temp = edit_temp->suite;
		m_libere (ch_ptr, sizeof (edit_ch));
	}
	svoie[voie]->tete_edit.liste = NULL;
}


void edit_aff (int nblig)
{
	int ftemp, pos;
	edit_ch *edit_temp;
	char buffer[RECSIZ + 2];
	char chaine[300];

	ftemp = ouvre_temp ();

	edit_temp = pvoie->tete_edit.liste;
	pos = 1;

	while (edit_temp)
	{
		if (pos >= pvoie->tete_edit.ligne)
		{
			if (nblig-- <= 0)
				break;
			lseek (ftemp, RECSIZ * (long) edit_temp->record, 0);
			read (ftemp, buffer, RECSIZ);
			if (pvoie->tete_edit.numero)
				sprintf (chaine, "%03d %s", pos, buffer);
			else
				strcpy (chaine, buffer);
			outs (chaine, strlen (chaine));
		}
		edit_temp = edit_temp->suite;
		++pos;
	}

	close (ftemp);
}


void edit_kill (int nblig)
{
	int ftemp, pos;
	edit_ch *edit_temp, *ch_deb = NULL, *ch_ptr;
	char buffer[RECSIZ + 2];

	ftemp = ouvre_temp ();

	edit_temp = pvoie->tete_edit.liste;
	pos = 1;
	while (edit_temp)
	{
		if (pos == pvoie->tete_edit.ligne)
			break;
		ch_deb = edit_temp;
		edit_temp = edit_temp->suite;
		++pos;
	}
	while (edit_temp)
	{
		if (nblig-- <= 0)
			break;
		lseek (ftemp, (long) RECSIZ * (long) edit_temp->record, 0);
		read (ftemp, buffer, RECSIZ);
		buffer[0] = '\0';
		lseek (ftemp, (long) RECSIZ * (long) edit_temp->record, 0);
		write (ftemp, buffer, RECSIZ);
		ch_ptr = edit_temp;
		edit_temp = edit_temp->suite;
		m_libere (ch_ptr, sizeof (edit_ch));
		--pvoie->tete_edit.max;
	}
	if (pvoie->tete_edit.ligne == 1)
		pvoie->tete_edit.liste = edit_temp;
	else
		ch_deb->suite = edit_temp;
	close (ftemp);
}


void edit_insert (char *chaine)
{
	int ftemp, pos;
	char *ptr;
	edit_ch *edit_temp, *ch_deb;

	ftemp = ouvre_temp ();

	pos = 1;
	if (pvoie->tete_edit.ligne == 1)
	{
		ch_deb = pvoie->tete_edit.liste;
		edit_temp = pvoie->tete_edit.liste = (edit_ch *) m_alloue (sizeof (edit_ch));
	}
	else
	{
		edit_temp = pvoie->tete_edit.liste;
		while (edit_temp)
		{
			if (pos == pvoie->tete_edit.ligne - 1)
				break;
			edit_temp = edit_temp->suite;
			++pos;
		}
		ch_deb = edit_temp->suite;
		edit_temp->suite = (edit_ch *) m_alloue (sizeof (edit_ch));
		edit_temp = edit_temp->suite;
	}
	edit_temp->suite = ch_deb;
	ptr = chaine;
	while (*ptr)
		++ptr;
	*ptr++ = '\n';
	*ptr = '\0';
	lseek (ftemp, 0L, 2);
	edit_temp->record = (int) (tell (ftemp) / RECSIZ);
	write (ftemp, chaine, RECSIZ);
	close (ftemp);
	++pvoie->tete_edit.max;
}


void edit_append (char *chaine)
{
	int ftemp, pos;
	char *ptr;
	edit_ch *edit_temp, *ch_deb;

	ftemp = ouvre_temp ();

	pos = 1;
	edit_temp = pvoie->tete_edit.liste;
	while (edit_temp)
	{
		if (pos == pvoie->tete_edit.ligne)
			break;
		edit_temp = edit_temp->suite;
		++pos;
	}
	if (edit_temp)
	{
		ch_deb = edit_temp->suite;
		edit_temp->suite = (edit_ch *) m_alloue (sizeof (edit_ch));
		edit_temp = edit_temp->suite;
	}
	else
	{
		edit_temp = (edit_ch *) m_alloue (sizeof (edit_ch));
		ch_deb = NULL;
	}
	edit_temp->suite = ch_deb;
	ptr = chaine;
	while (*ptr)
		++ptr;
	*ptr++ = '\n';
	*ptr = '\0';
	lseek (ftemp, 0L, 2);
	edit_temp->record = (int) (tell (ftemp) / RECSIZ);
	write (ftemp, chaine, RECSIZ);
	close (ftemp);
	++pvoie->tete_edit.max;
	++pvoie->tete_edit.ligne;
}


int edit_find (char *chaine)
{
	int ftemp, pos, ok = 0;
	edit_ch *edit_temp;
	char buffer[RECSIZ + 2];

	ftemp = ouvre_temp ();

	edit_temp = pvoie->tete_edit.liste;
	pos = 1;

	while (edit_temp)
	{
		if (pos >= pvoie->tete_edit.ligne)
		{
			lseek (ftemp, RECSIZ * (long) edit_temp->record, 0);
			read (ftemp, buffer, RECSIZ);
			if (strstr (buffer, chaine))
			{
				pvoie->tete_edit.ligne = pos;
				ok = 1;
				break;
			}
		}
		edit_temp = edit_temp->suite;
		++pos;
	}

	close (ftemp);

	return (ok);
}


int edit_replace (char *avant, char *apres)
{
	int ftemp, pos, ok = 0;
	edit_ch *edit_temp;
	char buffer[RECSIZ + 2];
	char change[300];
	char *ptr, *nptr, *cptr;

	ftemp = ouvre_temp ();

	edit_temp = pvoie->tete_edit.liste;
	pos = 1;

	while (edit_temp)
	{
		if (pos >= pvoie->tete_edit.ligne)
		{
			lseek (ftemp, RECSIZ * (long) edit_temp->record, 0);
			read (ftemp, buffer, RECSIZ);
			ptr = buffer;
			if ((cptr = strstr (buffer, avant)) != NULL)
			{
				pvoie->tete_edit.ligne = pos;
				nptr = change;
				while (ptr != cptr)
					*nptr++ = *ptr++;
				while (*apres)
					*nptr++ = *apres++;
				while (*avant)
				{
					++avant;
					++ptr;
				}
				while ((*nptr++ = *ptr++) != '\0');
				if (strlen (change) > 256)
				{
					/* printf ("<>\n"); */
					outln ("<>", 2);
				}
				else
				{
					lseek (ftemp, RECSIZ * (long) edit_temp->record, 0);
					write (ftemp, change, RECSIZ);
				}
				ok = 1;
				break;
			}
		}
		edit_temp = edit_temp->suite;
		++pos;
	}

	close (ftemp);

	return (ok);
}


void cree_temp (void)
{
	FILE *fptr;
	long nbcar = 0L;
	int ftemp, lg, pos;
	edit_ch *edit_temp;
	char buffer[RECSIZ + 2];
	char nom_temp[80];

	sprintf (nom_temp, "TEMP_%d.$$$", voiecur);
	if ((ftemp = open (nom_temp, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)
		) == -1)
		fbb_error (ERR_CREATE, nom_temp, 2);

	pos = 0;
	pvoie->tete_edit.ligne = 1;
	pvoie->tete_edit.carac = 0;
	pvoie->tete_edit.liste = NULL;
	edit_temp = pvoie->tete_edit.liste;

	if ((fptr = fopen (pvoie->sr_fic, "rt")) != NULL)
	{
		while (fgets (buffer, RECSIZ + 2, fptr))
		{
			if ((lg = strlen (buffer)) > RECSIZ)
			{
#ifdef ENGLISH
				outln ("Line too long !    ", 19);
#else
				outln ("Ligne trop longue !", 19);
#endif
				break;
			}
			if ((lg) && (buffer[lg - 1] != '\n'))
			{
				buffer[lg] = '\n';
				buffer[lg + 1] = '\0';
				++lg;
			}
			write (ftemp, buffer, RECSIZ);
			if (edit_temp)
			{
				edit_temp->suite = (edit_ch *) m_alloue (sizeof (edit_ch));
				edit_temp = edit_temp->suite;
			}
			else
			{
				edit_temp = pvoie->tete_edit.liste = (edit_ch *) m_alloue (sizeof (edit_ch));
			}
			edit_temp->suite = NULL;
			edit_temp->record = pos++;
			nbcar += (long) lg + 1;
		}
		fclose (fptr);
		pvoie->tete_edit.new_t = 0;
	}
	else
		pvoie->tete_edit.new_t = 1;
	pvoie->tete_edit.max = pos;

	close (ftemp);

	sprintf (buffer, "%s : %d/%ld", fic (pvoie->sr_fic), pos, nbcar + 1);
	outsln (buffer, strlen (buffer));
}


void sauve_temp (void)
{
	FILE *fptr;
	int ftemp, pos = 0;
	long nbcar = 0L;
	edit_ch *edit_temp;
	char buffer[RECSIZ + 2];

	if (pvoie->tete_edit.new_t)
		pvoie->tete_edit.new_t = 2;

	if ((fptr = fopen (pvoie->sr_fic, "wt")) == NULL)
		fbb_error (ERR_CREATE, pvoie->sr_fic, 3);

	ftemp = ouvre_temp ();

	edit_temp = pvoie->tete_edit.liste;

	while (edit_temp)
	{
		lseek (ftemp, RECSIZ * (long) edit_temp->record, 0);
		read (ftemp, buffer, RECSIZ);
		fputs (buffer, fptr);
		nbcar += (long) strlen (buffer) + 1;
		++pos;
		edit_temp = edit_temp->suite;
	}
	/* fputc ('\032', fptr); */

	fclose (fptr);
	close (ftemp);
	wr_dir (pvoie->sr_fic, pvoie->sta.indicatif.call);

	sprintf (buffer, "%s : %d/%ld", fic (pvoie->sr_fic), pos, nbcar + 1);
	outsln (buffer, strlen (buffer));
}


void edit (void)
{
	char *ptr;

	pvoie->lignes = -1;
	switch (pvoie->niv3)
	{
	case 0:
		strtok (indd, " \r");
		/* cprintf("Commande : <%s>\r\n", indd) ; */
		if ((ptr = strtok (NULL, " \r")) == NULL)
		{
			texte (T_ERR + 20);
		}
		else
		{
			if (tst_point (ptr))
			{
				if (aut_ecr (ch_slash (ptr), 1))
				{
					pvoie->tete_edit.numero = 0;
					strcpy (pvoie->sr_fic, tot_path (ch_slash (ptr), pvoie->dos_path));
					cree_temp ();
					prompt_edit ();
					ch_niv3 (1);
					break;
				}
			}
		}
		maj_niv (9, 0, 0);
		prompt_dos ();
		break;
	case 1:
		if (!cmd_edit ())
		{
			kill_temp ();
			if (pvoie->tete_edit.new_t == 2)
			{
				ch_niv3 (2);
				texte (T_YAP + 3);
				break;
			}
			else
			{
				maj_niv (9, 0, 0);
				prompt_dos ();
				break;
			}
		}
		prompt_edit ();
		break;

	case 2:
		new_label ();
		maj_niv (9, 0, 0);
		prompt_dos ();
		break;

	}
}
