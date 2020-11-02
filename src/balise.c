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

/*
 *  BALISE.C Gestion de la balise
 *
 */

#include <serv.h>

#define  INT14   0x14
#define  INT7F   0x7f

typedef struct typ_ddes
{
	char indic[8];
	long numero;
	int port;
	struct typ_ddes *suiv;
}
Ddes;

static Beacon *tete_beacon[NBPORT];

static long current_bcl[NBPORT];	/* Message BCL en cours */

static Ddes *t_ddes[NBPORT];

static char *broadcast_bloc (int);
static char *broadcast_line (long);
static char *via (char *);

static void broadcast_port (int, char *);
static void mode_binaire (int);
static void send_broadcast (int, char *, int);
static void set_beacon (char *, Beacon *);
static void set_bcall (char *, indicat *);

static void free_beacon (int port)
{
	Beacon *ptemp;
	Beacon *pb = tete_beacon[port];

	while (pb)
	{
		ptemp = pb;
		pb = pb->next;
		m_libere (ptemp, sizeof (Beacon));
	}
	tete_beacon[port] = NULL;
}

void end_beacon (void)
{
	int port;

	for (port = 0; port < NBPORT; port++)
		free_beacon (port);
}

static void add_beacon (int port, char *texte)
{
	Beacon *pb = tete_beacon[port];

	if (pb)
	{
		while (pb->next)
		{
			pb = pb->next;
		}
		pb->next = (Beacon *) m_alloue (sizeof (Beacon));
		pb = pb->next;
	}
	else
	{
		pb = tete_beacon[port] = (Beacon *) m_alloue (sizeof (Beacon));
	}

	set_beacon (texte, pb);
	pb->next = NULL;
}


static void set_bcall (char *callsign, indicat * call_str)
{
	int i = 0;
	char *ptr = call_str->call;

	df ("set_bcall", 4);
	call_str->num = 0;
	if (callsign == NULL)
	{
		*ptr = '\0';
		ff ();
		return;
	}

	strupr (callsign);
	while ((*callsign) && (!isalnum (*callsign)))
		++callsign;
	while (isalnum (*callsign))
		if (i++ < 6)
			*ptr++ = *callsign++;
	*ptr = '\0';
	while ((*callsign) && (!isdigit (*callsign)))
		++callsign;
	i = 0;
	while (isdigit (*callsign))
	{
		i *= 10;
		i += (*callsign++ - '0');
	}
	if ((i >= 0) && (i <= 15))        
		call_str->num = i;
	ff ();
}


static void set_beacon (char *path, Beacon * beacon)
{
	int i = 0;
	char *ptr;

	beacon->nb_digi = 0;
	ptr = strtok (path, " ,\r\n");
	set_bcall (ptr, &beacon->desti);

	ptr = strtok (NULL, " ,\r\n");
	if (ptr)
	{
		strupr (ptr);
		if (((strlen (ptr) == 1) && (*ptr == 'V')) ||
			((strlen (ptr) == 3) && (strcmp (ptr, "VIA") == 0)))
		{
			ptr = strtok (NULL, " ,\r\n");
		}
	}

	for (i = 0; i < 8; i++)
	{
		if (ptr)
			++beacon->nb_digi;
		set_bcall (ptr, &beacon->digi[i]);
		ptr = strtok (NULL, " ,\r\n");
	}
}

static void aff_ui (int port, char *line)
{
	int nbcar;

	nbcar = strlen (line);
	put_ui (line, W_CNST, nbcar);
}

void dde_synchro (char *indic, long num, int port)
{
	Ddes *ptemp;
	char line[80];

	sprintf (line, "[%d] Broadcast SYNC #%ld asked fm %s", port, num, indic);
	aff_ui (port, line);

	if (p_port[port].moport & 0x20)
	{
		if (t_ddes[port])
		{
			ptemp = t_ddes[port];
			while (ptemp->suiv)
				ptemp = ptemp->suiv;
			ptemp->suiv = (Ddes *) m_alloue (sizeof (Ddes));
			ptemp = ptemp->suiv;
		}
		else
		{
			ptemp = t_ddes[port] = (Ddes *) m_alloue (sizeof (Ddes));
		}
		ptemp->suiv = NULL;
		strn_cpy (6, ptemp->indic, indic);
		ptemp->numero = num;
		ptemp->port = port;
	}
	else
	{
		sprintf (line, "[%d] Port %d not allowed for unproto lists !", port, port);
		aff_ui (port, line);
	}
}

void send_balise (int port)
{
	static int first = 1;
	FILE *fptr;
	char *ptr;
	char sbuffer[600];
	int c, var, nb, pos = 0;
	int maxcar;
	int debut = 1;
	Beacon *pb;

	df ("send_balise", 1);

	aff_etat ('B');

	if (first)
	{
		first = 0;
		for (nb = 0; nb < NBPORT; nb++)
		{
			t_ddes[nb] = NULL;
			tete_beacon[nb] = NULL;
			current_bcl[nb] = nomess;
		}
	}

	free_beacon (port);

	if (p_port[port].pvalid)
	{
		sprintf (sbuffer, "BEACON%d.SYS", port);
		if ((fptr = fopen (c_disque (sbuffer), "rt")) != NULL)
		{
			maxcar = p_port[port].beacon_paclen;
			if (maxcar == 0)
				maxcar = 128;
			nb = 0;
			vlang = 0;			/* langue primaire */
			var = FALSE;
			ptr = NULL;

			while (1)
			{
				if (var)
				{
					if (*ptr)
					{
						sbuffer[nb] = *ptr++;
						nb++;
					}
					else
						var = FALSE;
				}
				else
				{
					if ((c = fgetc (fptr)) == EOF)
						break;
					if ((pos == 0) && (c == '%'))
					{
						if (fscanf (fptr, "%d", &t_balise[port])) {
							t_balise[port] *= 60;
							fgetc (fptr);
						} else {
							perror ("send_balise() error reading port");
						}
					}
					else if ((pos == 0) && (c == '!'))
					{
						fgets (sbuffer, 80, fptr);
						add_beacon (port, sbuffer);
						nb = 0;
					}
					else
					{
						if (tete_beacon[port] == NULL)
						{
							strcpy(sbuffer, "MAIL");
							add_beacon (port, sbuffer);
						}
						if ((debut) && (p_port[port].moport & 0x20))
						{
							if (current_bcl[port] == nomess)
							{
								sprintf (sbuffer, "%-6ld !!\r", nomess);
								send_broadcast (port, sbuffer, strlen (sbuffer));
							}
							debut = 0;
						}
						++pos;
						if (c == '$')
						{
							if ((c = fgetc (fptr)) == EOF)
								break;
							ptr = variable (c);
							var = TRUE;
						}
						else if (c == '%')
						{
							if ((c = fgetc (fptr)) == EOF)
								break;
							ptr = alt_variable (c);
							var = TRUE;
						}
						else
						{
							if (c == '\n')
							{
								sbuffer[nb] = '\r';
								++nb;
								pos = 0;
							}
							else if (c != '\r')
							{
								sbuffer[nb] = c;
								nb++;
							}
						}
					}
				}
				if (nb == maxcar)
				{
					for (pb = tete_beacon[port]; pb ; pb = pb->next)
					{
						snd_drv (port, UNPROTO, sbuffer, nb, pb);
					}
					nb = 0;
				}
			}
			ferme (fptr, 12);
			if (nb)
			{
				for (pb = tete_beacon[port]; pb ; pb = pb->next)
				{
					snd_drv (port, UNPROTO, sbuffer, nb, pb);
				}
			}
		}
	}
	ff ();
}


static char *via (char *bbsv)
{
	int lg = 0;
	static char buffer[40];
	char *ptr = buffer;

	if (*bbsv)
	{
		*ptr++ = '@';
		++lg;
	}

	while (isalnum (*bbsv))
	{
		*ptr++ = *bbsv++;
		if (++lg == 7)
			break;
	}

	while (lg++ < 7)
		*ptr++ = ' ';
	*ptr = '\0';

	return (buffer);
}

static void send_broadcast (int port, char *buffer, int len)
{
	indicat sav_beacon;
	Beacon *pb = tete_beacon[port];


	if ((len == 0) || (*buffer == '\0'))
		return;

	while (pb)
	{
		sav_beacon = pb->desti;
		strcpy (pb->desti.call, "FBB");
		pb->desti.num = 0;
		snd_drv (port, UNPROTO, buffer, len, pb);
		pb->desti = sav_beacon;
		pb = pb->next;
	}
}

void broadcast_port (int port, char *chaine)
{
	int len;
	int lt;
	char bloc[256];
	char tampon[90];
	char *optr;
	int maxcar = p_port[port].pk_t;

	if (maxcar == 0)
		maxcar = 128;

	*bloc = '\0';
	len = 0;

	while (*chaine)
	{
		optr = tampon;
		lt = 0;
		for (;;)
		{
			*optr++ = *chaine;
			++lt;
			if ((*chaine++ == '\r') || (lt == 89))
			{
				break;
			}
		}
		*optr = '\0';
		if ((len + lt) > maxcar)
		{
			if (*bloc)
			{
				send_broadcast (port, bloc, len);
				*bloc = '\0';
				len = 0;
			}
			else
			{
				send_broadcast (port, tampon, lt);
				*tampon = '\0';
				lt = 0;
			}
		}
		strcat (bloc, tampon);
		len += lt;
	}
	send_broadcast (port, bloc, len);
}

static char *broadcast_line (long numero)
{
	static char sbuffer[128];
	char titre[40];
	bullist bull;
	bullist *sav;

	int ok = 0;


	sav = ptmes;
	ptmes = &bull;

	if (ch_record (NULL, numero, '\0'))
	{
		ok = 1;
	}

	if (ok && (strcmp (ptmes->desti, "KILL") == 0))
		ok = 0;

	if (ok && (strcmp (ptmes->desti, "WP") == 0))
		ok = 0;

	if (ok && (ptmes->type == 'A') && (!ack_unproto))
		ok = 0;

	if (ok && (ptmes->type != 'B'))
	{
		if ((*ptmes->bbsv == '\0') && (!priv_unproto))
			ok = 0;
	}

	if (ok && (ptmes->type != 'B'))
	{
		if ((*ptmes->bbsv) && (!via_unproto))
			ok = 0;
	}

	if (ok && ((ptmes->status == 'A') || (ptmes->status == 'K')))
		ok = 0;

	if (ok)
	{
		if ((mute_unproto) && (ptmes->type == 'P'))
			strcpy (titre, "***");
		else
			n_cpy (34, titre, ptmes->titre);
		sprintf (sbuffer,
				 "%-6ld %c %6ld %-6s%s %-6s %s %s\r",
				 ptmes->numero, ptmes->type,
		 (ptmes->taille > 1000000L) ? 999999L : ptmes->taille, ptmes->desti,
			via (ptmes->bbsv), ptmes->exped, date_mbl (ptmes->date), titre);
	}
	else
		sprintf (sbuffer, "%-6ld #\r", numero);
	ptmes = sav;
	return (sbuffer);
}

static char *broadcast_bloc (int port)
{
	FILE *fptr;
	info frec;
	int back_mess = 0;
	int offset;
	long dde_bcl;
	char *ptr;
	Ddes *ptemp;
	ind_noeud *exp;
	bloc_mess *bptr;
	mess_noeud *mptr;
	unsigned num_indic;
	bullist bul;
	static char beacon_buffer[256];
	char line[80];

	*beacon_buffer = '\0';
	*line = '\0';

	while (t_ddes[port])
	{
		/* Demande de synchro */
		dde_bcl = t_ddes[port]->numero;
		if ((nomess - dde_bcl) > nb_unproto)
			dde_bcl = nomess - nb_unproto;
		else
			back_mess = 1;

		/* cherche les prives eventuels et envoie le premier ... */
		exp = insnoeud (t_ddes[port]->indic, &num_indic);
		if (exp->coord != 0xffff)
		{
			fptr = ouvre_nomenc ();
			fseek (fptr, (long) exp->coord * sizeof (frec), 0);
			fread (&frec, sizeof (info), 1, fptr);
			ferme (fptr, 79);

			if (!UNP (frec.flags))
			{
				sprintf (beacon_buffer, "%-6ld / %s\r", current_bcl[port], t_ddes[port]->indic);
				sprintf (line, "[%d] U Flag not set for %s, stopped !", port, t_ddes[port]->indic);
				aff_ui (port, line);
				back_mess = 1;
			}
			else
			{
				if (nomess <= dde_bcl)
				{
					sprintf (beacon_buffer, "%-6ld !!\r", nomess);
					sprintf (line, "[%d] %s asked %ld, end of list %ld !", port, t_ddes[port]->indic, dde_bcl, nomess);
					aff_ui (port, line);
					back_mess = 1;
				}
				else
				{
					ouvre_dir ();
					bptr = tete_dir;
					offset = 0;

					if (dde_bcl < current_bcl[port])
					{
						current_bcl[port] = dde_bcl;
					}
					sprintf (line, "[%d] %s allowed, resync to %ld", port, t_ddes[port]->indic, current_bcl[port]);
					aff_ui (port, line);

					while (bptr)
					{
						mptr = &(bptr->st_mess[offset]);
						if ((mptr->noenr) && (mptr->no_indic == num_indic))
						{
							read_dir (mptr->noenr, &bul);
							/* if ((bul.status != 'K') && (bul.status != 'H') && (bul.status != 'A')) { */
							if (bul.status == 'N')
							{
								if ((bul.numero > t_ddes[port]->numero) && (bul.numero <= current_bcl[port]))
								{
									sprintf (line, "[%d] %s unread private %ld", port, t_ddes[port]->indic, bul.numero - 1);
									aff_ui (port, line);
									sprintf (beacon_buffer, "%-6ld ! %s\r", bul.numero - 1, t_ddes[port]->indic);
									ptr = broadcast_line (bul.numero);
									strcat (beacon_buffer, ptr);
									back_mess = 1;
									break;
								}
							}
						}
						if (++offset == T_BLOC_MESS)
						{
							bptr = bptr->suiv;
							offset = 0;
						}
					}
					ferme_dir ();
				}

			}
		}

		else
		{
			sprintf (line, "[%d] Unknown callsign %s, stopped !", port, t_ddes[port]->indic);
			aff_ui (port, line);
			sprintf (beacon_buffer, "%-6ld / %s\r", current_bcl[port], t_ddes[port]->indic);
			back_mess = 1;
		}

		if (back_mess == 0)
		{
			sprintf (line, "[%d] %s, end of list %ld", port, t_ddes[port]->indic, current_bcl[port]);
			aff_ui (port, line);
			sprintf (beacon_buffer, "%-6ld ! %s\r", current_bcl[port], t_ddes[port]->indic);
		}

		ptemp = t_ddes[port];
		t_ddes[port] = t_ddes[port]->suiv;
		m_libere (ptemp, sizeof (Ddes));

	}

	if (back_mess == 0)
	{
		for (;;)
		{
			if (current_bcl[port] == nomess)
				break;
			ptr = broadcast_line (current_bcl[port] + 1);
			if ((strlen (beacon_buffer) + strlen (ptr)) > 250)
			{
				break;
			}
			strcat (beacon_buffer, ptr);
			++current_bcl[port];
		}
	}
	return (beacon_buffer);
}

void broadcast_list (void)
{
	int port;
	char *ptr;

	df ("broadcast_list", 0);

	for (port = 1; port < NBPORT; port++)
	{
		if ((p_port[port].pvalid) && (p_port[port].moport & 0x20))
		{
			if ((t_ddes[port] == NULL) && (current_bcl[port] == nomess))
				continue;

			ptr = broadcast_bloc (port);

			if (*ptr == '\0')
			{
				ff ();
				return;
			}

			broadcast_port (port, ptr);
		}
	}
	ff ();
}

static void mode_binaire (int voie)
{
	if (svoie[voie]->binary == 0)
	{
		clear_inbuf (voie);
		time_yapp[voie] = -1;
		set_binary(voie, 1);
	}
}


void send_bin_message (void)
{
	switch (pvoie->niv3)
	{

	case 5:
		env_message ();
		break;

	case 6:
		if (bin_message (pvoie->t_read) == 0)
		{
			ch_niv3 (5);
			env_message ();
		}
		break;
	}

	if (pvoie->niv3 == 2)
	{
		retour_mbl ();
		pvoie->temp1 = -2;
	}
}

void send_binary_mess (void)
{
	long no;
	long seek_offset;
	int nb = 0;
	bullist *pbul;
	rd_list *ptemp = NULL;

	init_fb_mess (voiecur);
	incindd ();
	pvoie->fbb = 1;

	if ((no = lit_chiffre (0)) != 0)
	{
		seek_offset = 0L;
		if (*indd == '!')
		{
			++indd;
			pvoie->fbb = 2;
			seek_offset = lit_chiffre (0);
		}
		if (ptemp)
		{
			ptemp->suite = (rd_list *) m_alloue (sizeof (rd_list));
			ptemp = ptemp->suite;
		}
		else
		{
			pvoie->t_read = ptemp = (rd_list *) m_alloue (sizeof (rd_list));
		}
		ptemp->pmess = &(pvoie->fb_mess[nb]);
		ptemp->suite = NULL;
		ptemp->nmess = no;
		ptemp->verb = 1;
		if ((pbul = ch_record (NULL, no, 'Y')) != NULL)
		{
			if (droit_ok (pbul, 1))
			{
				pvoie->fb_mess[nb] = *pbul;
				pvoie->fb_mess[nb].taille = seek_offset;
			}
			else
			{
				ptemp->verb = 0;
			}
		}
		else
		{
			ptemp->verb = 0;
		}

	}

	mode_binaire (voiecur);
	pvoie->enrcur = 0L;
	maj_niv (N_RBIN, 0, 5);
	send_bin_message ();
	pvoie->temp1 = -2;			/* Demande la suppression des messages apres envoi */
}
