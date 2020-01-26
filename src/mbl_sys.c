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

#if defined(__FBBDOS__) || defined(__WINDOWS__)
#define random_nb(val) random(val)
#endif

/* Teste si l'utilisateur a ou aura les droits apres la cmde SYS */
int droits_2 (unsigned int droit)
{
	FILE *fptr;
	char indic[80];
	char buffer[300];
	unsigned ds, dss;

	if (droits (droit))
		return (TRUE);

	if ((SYS (pvoie->finf.flags)) && (droit & ds_droits))
		return (TRUE);

	/* Acces a la cmde SYS ? */
	if (SYS (pvoie->finf.flags))
	{
		if ((fptr = fopen (c_disque ("passwd.sys"), "rt")) != NULL)
		{
			if (fgets (buffer, 300, fptr))
			{
				while (fgets (buffer, 300, fptr))
				{
					sup_ln (buffer);
					if (*buffer == '#')
						continue;
					*indic = '\0';
					ds = dss = 0;

					sscanf (buffer, "%s %u %u", indic, &ds, &dss);
					if (strcmp (pvoie->sta.indicatif.call, strupr (indic)) == 0)
					{
						fclose (fptr);
						return ((droit & dss) && (ds & CMDSYS));
					}
				}
			}
			fclose (fptr);
		}
		return ((droit & dss_droits) && (ds_droits & CMDSYS));
	}

	return (FALSE);
}

void change_droits (int voie)
{
	FILE *fptr;
	char indic[80];
	char buffer[300];
	unsigned ds, dss;

	if (SYS (svoie[voie]->finf.flags))
	{
		if ((fptr = fopen (c_disque ("passwd.sys"), "rt")) != NULL)
		{
			if (fgets (buffer, 300, fptr))
			{
				while (fgets (buffer, 300, fptr))
				{
					sup_ln (buffer);
					if (*buffer == '#')
						continue;
					*indic = '\0';
					ds = dss = 0;
					sscanf (buffer, "%s %u %u", indic, &ds, &dss);
					if (strcmp (svoie[voie]->sta.indicatif.call, strupr (indic)) == 0)
					{
						fclose (fptr);
						if (*svoie[voie]->passwd)
							svoie[voie]->droits = dss;
						else
							svoie[voie]->droits = ds;
						return;
					}
				}
			}
			fclose (fptr);
		}
		if (*svoie[voie]->passwd)
			svoie[voie]->droits = dss_droits;
		else
			svoie[voie]->droits = ds_droits;
	}
	else
		svoie[voie]->droits = d_droits;
}


int comp_passwd (char *call, char *chaine, time_t pass_time)
{
	int ok = 0;
	unsigned ds, dss;
	char indic[80];
	char buffer[300];
	char password[300];
	char *ptr = NULL;
	FILE *fptr;
	int first = 1;

	if (*chaine == '!')
	{
		++chaine;
		while (!ISGRAPH (*chaine))
			++chaine;
	}

	if (strlen (chaine) > 256)
		chaine[256] = '\0';

	if ((fptr = fopen (c_disque ("passwd.sys"), "rt")) != NULL)
	{
		while (fgets (buffer, 300, fptr))
		{
			sup_ln (buffer);
			ptr = buffer;

			if ((!isgraph (*buffer)) || (*buffer == '#'))
				continue;

			if (first)
			{
				/* Skip the first line */
				first = 0;
				continue;
			}

			*indic = '\0';
			ds = dss = 0;

			sscanf (buffer, "%s %u %u %[^\n]", indic, &ds, &dss, password);
			ptr = password;
			if (strcmpi (call, indic) == 0)
			{
				while (isspace (*ptr))
					++ptr;
				ok = 1;
				break;
			}
		}

		if (!ok)
		{
			rewind (fptr);
			while (fgets (buffer, 300, fptr))
			{
				sup_ln (buffer);

				if ((!isgraph (*buffer)) || (*buffer == '#'))
					continue;

				ptr = buffer;

				/* Get global password */
				/* sscanf (buffer, "%[^\n]", ptr); */
				break;
			}
		}

		ok = (strcmpi (chaine, ptr) == 0);

		fclose (fptr);
	}

	if (!ok)
	{
		/* Checks MD5 */
		uchar source[300];
		uchar dest[80];

		sprintf (source, "%010ld%s", pass_time, ptr);
		MD5String (dest, source);
		ok = (stricmp (dest, chaine) == 0);
	}

	return (ok);
}

char *mk_passwd (char *chaine)
{

	static char str[80];
	char temp[20];
	int lg, i, rd;
	char *ptr = chaine;

	randomize ();
	lg = strlen (chaine);
	i = 0;

	sprintf (str, "%s-%d> ", mycall, myssid);

	while (*ptr)
	{
		if (ISGRAPH (*ptr))
			++i;
		++ptr;
	}

	if (i > 7)
	{
		for (i = 0; i < 5; i++)
		{
			while (1)
			{
				rd = random_nb (lg);
				if (ISGRAPH (chaine[rd]))
					break;
			}
			pvoie->passwd[i] = toupper (chaine[rd]);
			chaine[rd] = '\0';
			sprintf (temp, " %d", rd + 1);
			strcat (str, temp);
		}
	}
	else
	{
		strcat (str, " 0 0 0 0 0");
	}

	/* MD5 password implementation */
	pvoie->pass_time = time (NULL);
	sprintf (temp, " [%010ld]", pvoie->pass_time);
	strcat (str, temp);

	return (str);
}

char *snd_passwd (char *chaine)
{
	int ok = 0;
	unsigned ds, dss;
	char indic[80];
	char buffer[300];
	char *ptr, *ind;
	FILE *fptr;
	int first = 1;

	sprintf (chaine, "%s-%d> ", mycall, myssid);
	*pvoie->passwd = 1;
	if ((fptr = fopen (c_disque ("passwd.sys"), "rt")) != NULL)
	{
		while (fgets (buffer, 300, fptr))
		{
			ind = indic;
			ptr = buffer;
			if ((!isgraph (*buffer)) || (*buffer == '#'))
				continue;

			if (first)
			{
				/* Skip the first line */
				first = 0;
				continue;
			}

			*indic = '\0';
			ds = dss = 0;

			sscanf (buffer, "%s %u %u %[^\n]", indic, &ds, &dss, ptr);
			if (strcmp (pvoie->sta.indicatif.call, strupr (indic)) == 0)
			{
				while (isspace (*ptr))
					++ptr;
				ok = 1;
				break;
			}
		}

		if (!ok)
		{
			rewind (fptr);

			while (fgets (buffer, 300, fptr))
			{
				if ((!isgraph (*buffer)) || (*buffer == '#'))
					continue;

				ptr = buffer;

				/* Get global password */
				sscanf (buffer, "%[^\n]", ptr);
				break;
			}
		}

		strcpy (chaine, mk_passwd (buffer));

		fclose (fptr);

	}
	return (chaine);
}


int tst_passwd (char *chaine)
{
	int ok;

	sup_ln (chaine);			/* strupr(chaine) */

	ok = ((*chaine != '!') && (!strncmpi (chaine, pvoie->passwd, 5)));

	if (!ok)
	{
		ok = (comp_passwd (pvoie->sta.indicatif.call, chaine, pvoie->pass_time));
	}

	return (ok);
}

#define PROTOTYPES 1
#include "global.h"
#include "md5.h"

void MD5String (uchar *dest, uchar *source)
{
	int i;
	MD5_CTX context;
	uchar digest[16];
	unsigned int len = strlen (source);

	MD5Init (&context);
	MD5Update (&context, source, len);
	MD5Final (digest, &context);

	*dest = '\0';

	for (i = 0; i < 16; i++)
	{
		char tmp[5];

		sprintf (tmp, "%02X", digest[i]);
		strcat (dest, tmp);
	}
}

