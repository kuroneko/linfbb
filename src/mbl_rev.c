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

/*
 * Module MBL_REV.C
 */

#include "aff_stat.c"

#define REV_HOLD 100
typedef struct rev_list
{
	long hold[REV_HOLD];
	struct rev_list *next;
}
RevList;

static RevList *hold_list = NULL;
static int off_hlist = REV_HOLD;

static int longcmp (const void *a, const void *b)
{
	long la = *((long *) a);
	long lb = *((long *) b);

	if (la == lb)
		return (0);
	return (la < lb) ? -1 : 1;
}

static int exist_hold (long numero)
{
	char holdname[130];
	struct stat stat;

	hold_name (numero, holdname);
	return (stat (holdname, &stat) == 0);
}

void end_hold (void)
{
	int i;
	RevList *rptr = hold_list;
	RevList *prev = NULL;

	while (rptr)
	{
		for (i = 0; i < REV_HOLD; i++)
		{
			if (!exist_hold (rptr->hold[i]))
				rptr->hold[i] = 0L;
			if (rptr->hold[i] != 0L)
				break;
		}

		if (i == REV_HOLD)
		{
			/* Le bloc est vide, on relie les chainons valides et on le libere */
			if (prev)
			{
				/* Il y a un chainon avant */
				prev->next = rptr->next;
			}
			else
			{
				/* C'etait la tete de liste */
				hold_list = rptr->next;
			}

			/* Si le dernier bloc est libere, on prepare l'allocation d'un nouveau bloc */
			if (rptr->next == NULL)
				off_hlist = REV_HOLD;

			m_libere (rptr, sizeof (RevList));

			if (prev)
				rptr = prev->next;
			else
				rptr = hold_list;

		}
		else
		{
			prev = rptr;
			rptr = rptr->next;
		}
	}
}

int already_held (long numero)
{
	int i;
	RevList *rptr = hold_list;

	while (rptr)
	{
		for (i = 0; i < REV_HOLD; i++)
			if (rptr->hold[i] == numero)
				return 1;

		rptr = rptr->next;
	}
	return 0;
}

int delete_hold (long numero)
{
	int i;
	RevList *rptr = hold_list;

	while (rptr)
	{
		for (i = 0; i < REV_HOLD; i++)
		{
			if (rptr->hold[i] == numero)
			{
				rptr->hold[i] = 0L;
				return 1;
			}
		}
		rptr = rptr->next;
	}
	return 0;
}

void ajoute_hold (long numero)
{
	RevList *rptr = hold_list;

	if (already_held (numero))
		return;

	if (rptr)
	{
		/* Avance jusqu'au dernier maillon */
		while (rptr->next)
		{
			rptr = rptr->next;
		}
	}

	if (off_hlist == REV_HOLD)
	{
		RevList *ptmp;

		/* Ajoute un maillon a la liste */
		ptmp = m_alloue (sizeof (RevList));
		memset (ptmp, 0, sizeof (RevList));
		off_hlist = 0;
		if (rptr)
		{
			rptr->next = ptmp;
		}
		else
			hold_list = rptr = ptmp;
	}

	rptr->hold[off_hlist++] = numero;
}

void init_hold (void)
{
	int done;
	long num;
	int i;
	int nb;
	long *hold_nb;
	char path[130];
	struct ffblk ffblk;

	/* Cree ou remet la liste a jour. Les entrees non valides sont a 0 */
	end_hold ();

	sprintf (path, "%s*.HLD", MESSDIR);

	/* Chercher le nombre de messages held */
	nb = 0;
	done = findfirst (path, &ffblk, 0);
	while (!done)
	{
		++nb;
		done = findnext (&ffblk);
	}

	if (nb)
	{
		hold_nb = (long *) malloc (nb * sizeof (long));

		i = 0;

		/* Lire les noms de messages held */
		done = findfirst (path, &ffblk, 0);
		while (!done)
		{
			sscanf (ffblk.ff_name, "%lx", &num);
			hold_nb[i] = num;
			if (++i > nb)
				break;
			done = findnext (&ffblk);
		}

		/* Tri de la liste */
		qsort (hold_nb, nb, sizeof (long), longcmp);

		/* Insere les numeros tries dans la liste */
		for (i = 0; i < nb; i++)
			ajoute_hold (hold_nb[i]);

		free (hold_nb);
	}

	nb_hold = nb;
}

static int next_hold (void)
{
	int fd;
	int retour = 1;
	bullist lbul;
	char holdname[130];

	int i=0;
	RevList *rptr = hold_list;

	while (rptr)
	{
		for (i = 0; i < REV_HOLD; i++)
		{
			if (rptr->hold[i] > pvoie->l_hold)
			{
				pvoie->l_hold = rptr->hold[i];
				break;
			}
		}

		if (i != REV_HOLD)
			break;

		rptr = rptr->next;
	}

	if ((rptr == NULL) || (i == REV_HOLD))
	{
		/* Pas de message suivant */
		texte (T_MBL + 3);
		return retour;
	}

	hold_name (pvoie->l_hold, holdname);

	fd = open (holdname, O_RDONLY | O_BINARY);
	read (fd, &lbul, sizeof (bullist));
	close (fd);

	cr ();
	*ptmes = lbul;
	ptmes->numero = ++pvoie->temp1;

	entete_liste ();
	aff_status (ptmes);
	retour = 2;
	pvoie->messdate = ptmes->datesd;

	return (retour);
}


static int read_hold (int verbose)
{
	int nb;
	FILE *fptr;
	char chaine[256];
	char holdname[130];

	nb = 0;

	hold_name (pvoie->l_hold, holdname);

	if ((fptr = fopen (holdname, "rt")) != NULL)
	{
		fseek (fptr, pvoie->enrcur, 0);
		if (!verbose)
		{
			pvoie->enrcur = supp_header (fptr, 1);
			fseek (fptr, pvoie->enrcur, 0);
		}
		fflush (fptr);
		while ((nb = read (fileno (fptr), chaine, 250)) > 0)
		{
			outs (chaine, nb);
			if (pvoie->memoc >= MAXMEM)
			{
				/*        if (!getvoie(CONSOLE]->connect) cprintf("Max atteint\r\n") ; */
				pvoie->enrcur = ftell (fptr);
				break;
			}
		}
		ferme (fptr, 45);
	}
	else
		return (-1);

	return (nb);
}

void list_held (void)
{
	char holdname[130];
	int fd;
	bullist lbul;
	int premier = 1;
	int i;
	RevList *rptr;

	init_hold ();

	rptr = hold_list;

	while (rptr)
	{
		for (i = 0; i < REV_HOLD; i++)
		{
			if (rptr->hold[i])
			{
				hold_name (rptr->hold[i], holdname);

				fd = open (holdname, O_RDONLY | O_BINARY);
				read (fd, &lbul, sizeof (bullist));
				close (fd);

				*ptmes = lbul;
				ptmes->numero = pvoie->temp1 + 1;

				if (premier)
				{
					premier = 0;
					entete_liste ();
				}
				aff_status (ptmes);

				++pvoie->temp1;
			}
		}
		rptr = rptr->next;
	}

	if (premier)
		texte (T_MBL + 3);
}

static void hold_question (void)
{
	texte (T_MBL + 58);
}

int review (void)
{
	int orig;
	int dest;
	int local = 0;
	int verbose = 0;

	int error = 0;
	long numess;
	char holdname[130];
	char messname[130];

	switch (pvoie->niv3)
	{
	case 0:
		init_hold ();
		pvoie->l_hold = 0L;
		pvoie->temp1 = 0;
		switch (next_hold ())
		{
		case 0:
			ch_niv3 (1);
			break;
		case 1:
			retour_mbl ();
			break;
		case 2:
			hold_question ();
			ch_niv3 (2);
			break;
		}
		break;
	case 1:
		switch (next_hold ())
		{
		case 0:
			break;
		case 1:
			retour_mbl ();
			break;
		case 2:
			hold_question ();
			ch_niv3 (2);
			break;
		}
		break;
	case 2:
		while_space ();

		switch (toupper (*indd))
		{
		case 'V':
			verbose = 1;
		case 'R':
			/* Lire le message held */
			pvoie->sr_mem = 0;
			pvoie->enrcur = 256L;

			switch (read_hold (verbose))
			{
			case 0:
				pvoie->sr_mem = 0;
				hold_question ();
				ch_niv3 (2);
				break;
			case -1:
				retour_mbl ();
				break;
			default:
				ch_niv3 (3);
				break;
			}

			/*
			   indd = data;
			   sprintf(data, "%ld", ptmes->numero);
			   list_read(verbose);
			   if (mbl_mess_read() == 0) {
			   pvoie->sr_mem = 0;
			   hold_question();
			   ch_niv3(2);
			   }
			   else
			   ch_niv3(3);
			 */
			break;
		case 'A':
		case 'K':
		case 'D':
			unlink (hold_name (pvoie->l_hold, holdname));
			delete_hold (pvoie->l_hold);
			--nb_hold;
			aff_msg_cons ();
			switch (next_hold ())
			{
			case 0:
				ch_niv3 (1);
				break;
			case 1:
				retour_mbl ();
				break;
			case 2:
				hold_question ();
				ch_niv3 (2);
				break;
			}
			break;
		case 'L':
			local = 1;
		case 'U':
			/* Unhold */

			*(pvoie->appendf) = '\0';
			*(pvoie->mess_bid) = '\0';

			ptmes->numero = 0L;
			ptmes->theme = 0;

			pvoie->chck = 0;
			pvoie->m_ack = 0;
			pvoie->messdate = time (NULL);
			pvoie->mess_num = -1;

			if ((ptmes->type == 'B') || ((ptmes->type == 'P') && (strcmp (ptmes->desti, "SYSOP") == 0)))
			{
				if (*(ptmes->bbsv))
					ptmes->status = '$';
				else
					ptmes->status = 'N';
			}
			else
				ptmes->status = 'N';

			/* Copier le fichier avec un nouveau No de message */
			hold_name (pvoie->l_hold, holdname);

			orig = open (holdname, O_RDONLY | O_BINARY);
			if (orig < 0)
			{
				retour_mbl ();
				break;
			}

			/* Saute le header */
			lseek (orig, (long) sizeof (bullist), SEEK_SET);

			/* Lit le home-bbs */
			fbb_read (orig, pvoie->mess_home, sizeof (pvoie->mess_home));
			pvoie->mess_home[sizeof (pvoie->mess_home) - 1] = '\0';

			/* Debut du texte */
			lseek (orig, 256L, SEEK_SET);

			/* Cree un nouveau message */
			numess = ptmes->numero = next_num ();
			mess_name (MESSDIR, ptmes->numero, messname);
			dest = open (messname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
			if (dest < 0)
			{
				close (orig);
				retour_mbl ();
				break;
			}

			copy_fic (orig, dest, NULL);

			close (orig);
			close (dest);

			unlink (holdname);
			delete_hold (pvoie->l_hold);
			--nb_hold;

			put_mess_fwd ('H');
			if (local)
			{
				/* Pas de forwarding ... */
				int i;

				for (i = 0; i < NBMASK; i++)
				{
					ptmes->forw[i] = 0xff;
					ptmes->fbbs[i] = '\0';
				}
				maj_rec (ptmes->numero, ptmes);
				clear_fwd (ptmes->numero);
				/* Supprimer le via */
				ptmes->bbsv[0] = '\0';
			}

			tst_sysop (ptmes->desti, numess);
			tst_serveur (ptmes);
			tst_ack (ptmes);
			tst_warning (ptmes);


			aff_msg_cons ();
			cr ();


			switch (next_hold ())
			{
			case 0:
				ch_niv3 (1);
				break;
			case 1:
				retour_mbl ();
				break;
			case 2:
				hold_question ();
				ch_niv3 (2);
				break;
			default:
				break;
			}
			break;
		case '\0':
			switch (next_hold ())
			{
			case 0:
				ch_niv3 (1);
				break;
			case 1:
				retour_mbl ();
				break;
			case 2:
				hold_question ();
				ch_niv3 (2);
				break;
			}
			break;
		case 'Q':
			retour_mbl ();
			break;
		default:
			hold_question ();
			break;
		}
		break;
	case 3:
		switch (read_hold (1))
		{
		case 0:
			pvoie->sr_mem = 0;
			hold_question ();
			ch_niv3 (2);
			break;
		case -1:
			retour_mbl ();
			break;
		}
		break;
	default:
		fbb_error (ERR_NIVEAU, "REVIEW", pvoie->niv3);
	}

	return (error);
}
