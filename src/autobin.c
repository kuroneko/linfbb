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

/*
 * Module transfert de fichiers binaires. protocole AUTOBIN.
 */

#if defined(__WINDOWS__) || defined(__linux__)
char *abin_str (int voie, char *s)
{
#define XMODLEN 44
	static char stdesc[8][11] =
	{
		"SendInit  ",
		"SendInit  ",
		"SendData  ",
		"SendEof   ",
		"          ",
		"WaitRecv  ",
		"RecvData  ",
		"RecvEof   "
	};

	char taille[40];
	int niv = svoie[voie]->niv3;

	*s = '\0';
	if ((niv >= 0) && (niv < 10))
	{
		if (svoie[voie]->tailm)
		{
			sprintf (taille, "/%ld", svoie[voie]->tailm);
		}
		else
		{
			*taille = '\0';
		}
		sprintf (s, "ABin:%s %s %ld%s",
				 stdesc[niv], svoie[voie]->appendf,
				 svoie[voie]->enrcur, taille);
	}
	return s;
}

static void aff_bin (int ok)
{
}

#else

static void aff_bin (int ok)
{
#define XMODLEN 44
	static char stdesc[8][11] =
	{
		"SendInit  ",
		"SendInit  ",
		"SendData  ",
		"SendEof   ",
		"          ",
		"WaitRecv  ",
		"RecvData  ",
		"RecvEof   "
	};

	char s[80];
	char taille[40];
	int n;
	int niv = pvoie->niv3;

	*s = '\0';
	if (ok)
	{
		if ((niv >= 0) && (niv < 10))
		{
			if (pvoie->tailm)
			{
				sprintf (taille, "/%ld", pvoie->tailm);
			}
			else
			{
				*taille = '\0';
			}
			sprintf (s, "ABin:%s %s %ld%s",
					 stdesc[niv], pvoie->appendf,
					 pvoie->enrcur, taille);
		}
	}
	for (n = strlen (s); n < XMODLEN; n++)
		s[n] = ' ';
	s[XMODLEN] = '\0';
	aff_chaine (W_DEFL, 17, 3, s);
}
#endif

static void compute_CRC (short ch, short *crc)
{
	short hibit;
	short shift;

	for (shift = 0x80; shift; shift >>= 1)
	{
		hibit = *crc & 0x8000;
		*crc <<= 1;
		*crc |= (ch & shift ? 1 : 0);
		if (hibit)
			*crc ^= 0x1021;
	}
}

static void wrbuf (void)
{
	int i;
	int ncars;
	int nbcar = nb_trait;
	uchar *uptr;
	obuf *msgtemp;
	char *ptcur;
	char *ptr;

	uptr = data;
	for (i = 0; i < nbcar; i++)
	{
		compute_CRC ((short) *uptr, (short *) &pvoie->checksum);
		uptr++;
	}

	pvoie->enrcur += (long) nbcar;
	pvoie->size_trans += (long) nbcar;
	if ((msgtemp = pvoie->msgtete) != NULL)
	{
		while (msgtemp->suiv)
			msgtemp = msgtemp->suiv;
	}
	else
	{
		msgtemp = (obuf *) m_alloue (sizeof (obuf));
		pvoie->msgtete = msgtemp;
		msgtemp->nb_car = msgtemp->no_car = 0;
		msgtemp->suiv = NULL;
	}
	ncars = msgtemp->nb_car;
	ptcur = msgtemp->buffer + ncars;
	ptr = data;
	while (nbcar--)
	{
		++pvoie->memoc;
		*ptcur++ = *ptr++;
		if (++ncars == 250)
		{
			msgtemp->nb_car = ncars;
			msgtemp->suiv = (obuf *) m_alloue (sizeof (obuf));
			msgtemp = msgtemp->suiv;
			msgtemp->nb_car = msgtemp->no_car = ncars = 0;
			msgtemp->suiv = NULL;
			ptcur = msgtemp->buffer;
		}
	}
	msgtemp->nb_car = ncars;
	if (pvoie->memoc > MAXMEM)
	{
		write_mess_temp (O_BINARY, voiecur);
	}
}

void bin_transfer (void)
{
	char s[80];
	struct stat bufstat;

	var_cpy (0, "AUTOBIN");

	switch (pvoie->niv3)
	{
	case 0:
		strtok (indd, " \r");
		if ((indd = strtok (NULL, " \r")) == NULL)
		{
			yapp_message (T_ERR + 20);
			retour_appel ();
			break;
		}

		if ((voiecur == CONSOLE) || ((!P_YAPP (voiecur)) && (!SYS (pvoie->finf.flags))))
		{
			yapp_message (T_YAP + 2);
			retour_appel ();
			break;
		}
		if ((tst_point (indd)) &&
			(stat (nom_yapp (), &bufstat) != -1) &&
			((bufstat.st_mode & S_IFREG) != 0))
		{
			if (pvoie->kiss != -2)
				texte (T_YAP + 0);
			pvoie->tailm = pvoie->enrcur = 0L;
			pvoie->size_trans = 0L;
			pvoie->xferok = 1;
			pvoie->type_yapp = 4;
			pvoie->tailm = file_size (pvoie->sr_fic);
			sprintf (s, "#BIN#%ld", pvoie->tailm);
			outln (s, strlen (s));
			aff_bin (1);
			ch_niv3 (1);
		}
		else
		{
			/* ok = 0; */
			yapp_message (T_ERR + 11);
			retour_appel ();
		}
		break;

	case 1:
		if (strnicmp (indd, "#OK#", 4) == 0)
		{
			ch_niv3 (2);
			pvoie->size_trans = 0L;
			pvoie->time_trans = 0L;
			pvoie->checksum = 0;
			set_binary (voiecur, 1);
			bin_transfer ();
		}
		else
		{
			aff_bin (0);
			retour_appel ();
		}
		break;

	case 2:
		pvoie->lignes = -1;
		if (strnicmp (indd, "#ABORT#", 7) == 0)
		{
			set_binary (voiecur, 0);
			clear_outbuf (voiecur);
			retour_dos ();
			aff_bin (0);
			break;
		}
		if (senddata (1) == 1)
		{
			aff_etat ('E');
			send_buf (voiecur);
			set_binary (voiecur, 0);
			ch_niv3 (3);
		}
		aff_bin (1);
		break;

	case 3:
		sprintf (s, "BIN-TX OK #%u  ", 0xffff & pvoie->checksum);
		outln (s, strlen (s));
		pvoie->finf.download += (int) (pvoie->size_trans / 1024L);
		retour_dos ();
		aff_bin (0);
		break;

	case 4:
		strtok (indd, " \r");
		if ((indd = strtok (NULL, " \r")) == NULL)
		{
			yapp_message (T_ERR + 20);
			retour_appel ();
			break;
		}

		if ((voiecur == CONSOLE) || ((!P_YAPP (voiecur)) && (!SYS (pvoie->finf.flags))))
		{
			yapp_message (T_YAP + 2);
			/* ok = 0; */
			retour_appel ();
			break;
		}

		if (read_only ())
			retour_appel ();
		else
		{
			int fd;

			if ((tst_point (indd)) && (stat (nom_yapp (), &bufstat) == -1))
			{
				/*fd = creat (pvoie->sr_fic, S_IREAD | S_IWRITE); */
				fd = open (pvoie->sr_fic, O_CREAT | O_WRONLY | O_TRUNC, S_IREAD | S_IWRITE);
				if (fd > 0)
				{
					close (fd);
					unlink (pvoie->sr_fic);
					pvoie->tailm = pvoie->enrcur = 0L;
					texte (T_YAP + 1);
					ch_niv3 (5);
					aff_bin (1);
				}
				else
				{
					/* ok = 0; */
					yapp_message (T_ERR + 30);
					retour_appel ();
				}
			}
			else
			{
				/* ok = 0; */
				yapp_message (T_ERR + 23);
				retour_appel ();
			}
		}
		break;

	case 5:
		if (strnicmp (indd, "#BIN#", 5) == 0)
		{
			outln ("#OK#", 4);
			aff_etat ('E');
			send_buf (voiecur);
			del_temp (voiecur);
			new_label ();

			pvoie->tailm = atol (indd + 5);
			pvoie->enrcur = 0L;
			pvoie->checksum = 0;
			pvoie->size_trans = 0L;
			pvoie->time_trans = time (NULL);
			set_binary (voiecur, 1);
			pvoie->xferok = 0;
			pvoie->type_yapp = 4;

			ch_niv3 (6);
			aff_bin (1);
		}
		else
		{
			retour_appel ();
			aff_bin (0);
		}
		break;

	case 6:
		wrbuf ();
		aff_bin (1);

		if (pvoie->enrcur == pvoie->tailm)
		{
			pvoie->xferok = 2;
			write_mess_temp (O_BINARY, voiecur);
			if (test_temp (voiecur))
			{
				/* Le fichier est mis en place */
				rename_temp (voiecur, pvoie->sr_fic);
				wr_dir (pvoie->sr_fic, pvoie->sta.indicatif.call);
			}
			set_binary (voiecur, 0);
			sprintf (s, "BIN-RX OK #%u  ", 0xffff & pvoie->checksum);
			outln (s, strlen (s));
			ch_niv3 (7);
			aff_bin (1);
		}
		break;

	case 7:
		retour_appel ();
		aff_bin (0);
		break;
	}

}
