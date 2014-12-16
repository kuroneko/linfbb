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

static int data_ok(recfwd *prec, int data_mode)
{
	if ((data_mode == 0) && (prec->bin))
		return 0;
	if ((data_mode == 3) && (!prec->bin))
		return 0;
	if ((data_mode == 2) && (prec->bin) && (!PRIVATE (prec->type)))
		return 0;
	return 1;
}

int print_fwd (int nobbs, uchar max, uchar old, uchar typ, int data_mode)
{
	int pos;
	int noctet;
	int ok = 0;
	int pass = 2;
	int aff = 0;
	char cmpmsk;
	recfwd *prec;
	lfwd *ptr_fwd;
	rd_list *ptemp = NULL;
	time_t date = time(NULL) - 3600L * (long)old;

	unsigned offset;
	bloc_mess *bptr;
	bullist bul;

	libere_tlist (voiecur);

	noctet = (nobbs - 1) / 8;
	cmpmsk = 1 << ((nobbs - 1) % 8);

	ouvre_dir ();

	if (fast_fwd)
	{
		while (pass)
		{
			pos = 0;
			ptr_fwd = tete_fwd;
			while (1)
			{
				aff = 0;
				if (pos == NBFWD)
				{
					ptr_fwd = ptr_fwd->suite;
					if (ptr_fwd == NULL)
						break;
					pos = 0;
				}
				prec = &ptr_fwd->fwd[pos];
				if (prec->type)
				{
					if (data_ok(prec, data_mode))
					{
 						if ((prec->fbbs[noctet] & cmpmsk) && (prec->date <= date) && (prec->kb <= max))
						{
							if ((prec->type == 'P') || (prec->type == 'A'))
							{
								if (pass == 2)
									aff = 1;
							}
							else
							{
								if (pass == 1)
									aff = 1;
							}
							if (aff)
							{
								if (ptemp)
								{
									ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
									ptemp = ptemp->suite;
								}
								else
								{
									pvoie->t_list = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
								}
								ptemp->suite = NULL;
								ptemp->nmess = prec->nomess;
								ptemp->verb = 1;
								ok = 1;
							}
						}
					}
				}
				pos++;
			}
			if (typ)
				break;
			--pass;
		}
	}
	else
	{
		while (pass)
		{

			offset = 0;
			bptr = tete_dir;

			while (bptr)
			{
				if (bptr->st_mess[offset].noenr)
				{
					read_dir (bptr->st_mess[offset].noenr, &bul);

					if (bul.type)
					{
						int kb = (int) (bul.taille >> 10);

						if ((bul.fbbs[noctet] & cmpmsk) && (bul.date <= date) && (kb <= max))
						{
							if ((bul.type == 'P') || (bul.type == 'A'))
							{
								if (pass == 2)
									aff = 1;
							}
							else
							{
								if (pass == 1)
									aff = 1;
							}
							if (aff)
							{
								if (ptemp)
								{
									ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
									ptemp = ptemp->suite;
								}
								else
								{
									pvoie->t_list = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
								}
								ptemp->suite = NULL;
								ptemp->nmess = bul.numero;
								ptemp->verb = 1;
								ok = 1;
							}
						}
					}
				}
				if (++offset == T_BLOC_MESS)
				{
					bptr = bptr->suiv;
					offset = 0;
				}
			}
			if (typ)
				break;
			--pass;
		}
	}
	ferme_dir ();
	maj_niv (N_MBL, 16, 0);
	mess_liste (1);
	return (ok);
}

#if 0

int print_fwd (int nobbs, uchar max, uchar typ)
{
	int pos;
	int noctet;
	int ok = 0;
	int pass = 2;
	int aff;
	char cmpmsk;
	rd_list *ptemp = NULL;

	unsigned offset;
	bloc_mess *bptr;
	bullist bul;

	libere_tlist (voiecur);

	pos = 0;
	noctet = (nobbs - 1) / 8;
	cmpmsk = 1 << ((nobbs - 1) % 8);

	ouvre_dir ();

	while (pass)
	{

		offset = 0;
		bptr = tete_dir;

		while (bptr)
		{
			if (bptr->st_mess[offset].noenr)
			{
				read_dir (bptr->st_mess[offset].noenr, &bul);

				if (bul.type)
				{
					int kb = (int) (bul.taille >> 10);

					if ((bul.fbbs[noctet] & cmpmsk) && (kb <= max))
					{
						if ((bul.type == 'P') || (bul.type == 'A'))
						{
							if (pass == 2)
								aff = 1;
						}
						else
						{
							if (pass == 1)
								aff = 1;
						}
						if (aff)
						{
							if (ptemp)
							{
								ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
								ptemp = ptemp->suite;
							}
							else
							{
								pvoie->t_list = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
							}
							ptemp->suite = NULL;
							ptemp->nmess = bul.numero;
							ptemp->verb = 1;
							ok = 1;
						}
					}
				}
			}
			if (++offset == T_BLOC_MESS)
			{
				bptr = bptr->suiv;
				offset = 0;
			}
		}
		if (typ)
			break;
		--pass;
	}

	ferme_dir ();
	maj_niv (N_MBL, 16, 0);
	mess_liste (1);
	return (ok);
}
#endif

void maj_fwd (void)
{
	int i, nb = 5;
	char temp[NBMASK];

	df ("maj_fwd", 0);

	ouvre_dir ();

	selvoie (CONSOLE);
	vlang = 0;
	pvoie->mode |= F_FOR;
	while (nb)
	{
		if (read_dir (p_forward, ptmes) == 0)
		{
			p_forward = 0;
			maj_options ();
			aff_etat ('A');
			aff_nbsta ();
			break;
		}
		if ((*ptmes->bbsv) &&
			(ptmes->status != 'H') &&
			((ptmes->status == 'N') ||
			 (ptmes->status == 'Y') ||
			 (ptmes->status == '$')
			))
		{
			aff_etat ('O');
			nb = 0;
			for (i = 0; i < NBMASK; i++)
			{
				temp[i] = ptmes->fbbs[i];
				ptmes->fbbs[i] = '\0';
			}
			test_forward (2);
			if (memcmp (ptmes->fbbs, temp, NBMASK) != 0)
			{
				clear_fwd (ptmes->numero);
				ins_fwd (ptmes);
				write_dir (p_forward, ptmes);
			}
		}
		else
			--nb;
		++p_forward;
	}
	ferme_dir ();
	ff ();
}
