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
 *  MODULE TRANSFERT BINAIRE PROTOCOLE YAPP
 */

#include <serv.h>

typedef struct
{
	char nom[FBB_NAMELENGTH+1];
	char attr;
	unsigned date;
	long size;
}
Fichier;


#include "yapp.h"

char *nom_yapp (void);

static int dir_ok (char *);

static void ack1 (void);
static void ack2 (void);
static void ack3 (void);
static void ack4 (void);
static void ack5 (void);
static void bin_hdr (char *, long);
static void canwait (void);
static void canrecd (void);
static void change_label (void);
static void fin_yapp (void);

/*
   static void not_ready(char *);
 */
static void reprise_yapp (void);
static void sendeof (void);
static void sendinit (void);
static void sendinit_retry (void);
static void wrbuf (void);
static void yapp_timer (void);
static void yapp_xfer (void);



static uchar etat[12][17] =
{
	{AB, SH, SD, AB, AB, AB, AB, AB, SA, C, AB, S, DP, S1, AB, AB, AB},
	{AB, SH, SD, AB, AB, AB, AB, AB, SA, C, AB, S, DP, S1, AB, AB, AB},
	{AB, AB, SD, AB, AB, AB, AB, AB, RP, C, AB, AB, DP, AB, AB, AB, AB},
	{AB, AB, AB, AB, AB, AB, AB, AB, AB, C, AB, AB, DP, AB, AB, AB, SD},
	{AB, AB, AB, AB, AB, AB, AB, AB, AB, C, AB, AB, DP, AB, ST, AB, AB},
	{SA, SA, SA, SA, SA, SA, SA, SA, SA, C, SA, SA, DP, SA, SA, SA, AB},
	{AB, AB, AB, E1, AB, AB, AB, AB, AB, C, AB, AB, DP, AB, AB, AB, AB},
	{AB, AB, AB, RH, E2, AB, AB, E4, AB, C, AB, AB, DP, AB, AB, AB, AB},
	{AB, AB, AB, AB, AB, E6, E3, AB, AB, C, AB, AB, DP, AB, AB, AB, AB},
	{SA, CW, CW, CW, CW, CW, CW, CW, CW, C, CW, CW, CW, CW, CW, CW, AB},
	{SA, CW, CW, CW, CW, CW, CW, CW, CW, E5, SA, CW, DP, SA, CW, CW, AB},
	{AB, SA, SA, SA, SA, SA, SA, SA, SA, C, SA, SA, SA, SA, SA, SA, AB}
};


void display_perf (int voie)
{
	int save_voie;

	ltoa (svoie[voie]->size_trans, varx[0], 10);
	if (svoie[voie]->time_trans == 0)
		svoie[voie]->time_trans = 1;

	ltoa (svoie[voie]->size_trans / svoie[voie]->time_trans, varx[1], 10);

	if (pvoie->kiss == -2)
	{
		save_voie = voiecur;
		selvoie (CONSOLE);
		texte (T_DOS + 5);
		selvoie (save_voie);
	}
	else
		texte (T_DOS + 5);
}

static void reprise_yapp (void)
{
	int lg = 0xff & (int) *(indd + 1);

	if ((lg == 0) || (*(indd + 2) != 'R') || (*(indd + 3) != '\0'))
	{
		ch_niv2 (SA);
		fin_yapp ();
	}
	else
	{
		pvoie->type_yapp = 0;
		indd += 4;
		lg -= 2;
		pvoie->enrcur = atol (indd);	/* Position in the file */
		while (*indd)
		{
			--lg;
			++indd;
		}
		if (lg > 1)
		{
			++indd;
			--lg;
			if (*indd == 'C')
				pvoie->type_yapp = 1;	/* Yapp checksum */
			aff_yapp (voiecur);
		}
		time_yapp[voiecur] = -1;
		pvoie->time_trans = time (NULL);
		if (senddata (2))
		{
			sendeof ();
			ch_niv2 (SE);
		}
		else
			ch_niv2 (SD);
	}
}

char *vir_path (char *nomfic)
{
	int i;
	int len;
	char *nptr;
	char fn[256];
	static char filename[256];

	if (nomfic[1] == ':')
	{
		nptr = nomfic;
	}
	else
	{
		fn[0] = getdisk () + 'A';
		fn[1] = ':';
		strcpy (fn + 2, nomfic);
		nptr = slash2back(fn);
	}
	for (i = 0; i < NB_PATH; i++)
	{
		len = strlen (PATH[i]);
		/* Cherche l'unite virtuelle */
		if ((len) && (strncmpi (PATH[i], nptr, len) == 0))
		{
			/* Transforme le nom physique en nom virtuel */
			filename[0] = i + 'A';
			filename[1] = ':';
			filename[2] = '\\';
			strcpy (filename + 3, nptr + len);
			return (filename);
		}
	}
	/* Pas trouve d'unite virtuelle */
	return (NULL);
}

void t_label (void)
{
	FILE *fptr;
	Rlabel rlabel;
	long record_old = 0L;
	long record_new = 0L;
	struct stat st;
	char filename[82];

	if ((fptr = fopen (d_disque ("YAPPLBL.DAT"), "r+b")) == NULL)
		return;

	while (fread (&rlabel, sizeof (Rlabel), 1, fptr))
	{
		if (*rlabel.nomfic)
		{
			int vdisk;

			vdisk = rlabel.nomfic[0] - 'A';
			strcpy (filename, PATH[vdisk]);
			strcat (filename, rlabel.nomfic + 3);

			if (stat (long_filename(NULL, filename), &st) == 0)
			{
				if (record_old != record_new)
				{
					fseek (fptr, record_new, 0);
					fwrite (&rlabel, sizeof (Rlabel), 1, fptr);
				}
				record_new += sizeof (Rlabel);
			}
		}
		record_old += sizeof (Rlabel);
		fseek (fptr, record_old, 0);
	}

	/* Cleans the rest of the file */
	memset (&rlabel, 0, sizeof (Rlabel));
	while (record_new < record_old)
	{
		fseek (fptr, record_new, 0);
		fwrite (&rlabel, sizeof (Rlabel), 1, fptr);
		record_new += sizeof (Rlabel);
	}

	fclose (fptr);
}

void w_label (char *nomfic, char *label)
{
	int nouveau;
	long record = 0L;
	long record_free = -1L;
	FILE *fptr;
	Rlabel rlabel;
	bullist rmess;
	char *ptr = vir_path (nomfic);

	/* Pas de nom virtuel ... Pas de mise a jour ! */
	if (ptr == NULL)
		return;

	if ((fptr = fopen (d_disque ("YAPPLBL.DAT"), "r+b")) == NULL)
	{
		if ((fptr = fopen (d_disque ("YAPPLBL.DAT"), "wb")) == NULL)
			return;
		fclose (fptr);

		if ((fptr = fopen (d_disque ("YAPPLBL.DAT"), "r+b")) == NULL)
		{
			return;
		}
	}

	nouveau = 1;

	while (fread (&rlabel, sizeof (Rlabel), 1, fptr))
	{
		if (*(rlabel.nomfic) == '\0')
			record_free = record;
		else if (strncmp (ptr, rlabel.nomfic, LABEL_FIC - 1) == 0)
		{
			record_free = record;
			nouveau = 0;
			break;
		}
		record += (long) sizeof (Rlabel);
	}
	if (record_free >= 0L)
		fseek (fptr, record_free, 0);

	if (nouveau)
	{
		memset (&rlabel, 0, sizeof (Rlabel));
		n_cpy (LABEL_NOM - 1, rlabel.label, label);
		n_cpy (LABEL_FIC - 1, rlabel.nomfic, ptr);
		n_cpy (LABEL_OWN - 1, rlabel.owner, pvoie->sta.indicatif.call);
		rlabel.date_creation = time (NULL);

		/* Next message number */
		rlabel.index = ++nomess;

		/* Update the message number */
		memset (&rmess, 0, sizeof (bullist));
		rmess.numero = nomess;
		ouvre_dir ();
		write_dir (0, &rmess);
		ferme_dir ();

	}
	else
	{
		n_cpy (LABEL_NOM - 1, rlabel.label, label);
	}

	fwrite (&rlabel, sizeof (Rlabel), 1, fptr);
	fclose (fptr);
}


static void fin_yapp (void)
#define DELAY_PROMPT 3			/* environ 3 secondes */
{
	pvoie->time_trans = time (NULL) - pvoie->time_trans;
	clear_inbuf (voiecur);
	time_yapp[voiecur] = DELAY_PROMPT;
	ch_niv2 (50);
}


void retour_niveau (void)
{
	if (pvoie->temp1 == N_DOS)
	{
		maj_niv (9, 0, 0);
	}
	else
	{
		maj_niv (N_MBL, 0, 0);
		pvoie->mbl = 1;
	}
}


void retour_appel (void)
{
	int save_voie;

	if (pvoie->kiss == -2)
	{
		pvoie->kiss = CONSOLE;
		save_voie = voiecur;

		/* Retour au gateway */
		selvoie (pvoie->kiss);
		texte (T_GAT + 5);
		maj_niv (N_TELL, 0, 3);

		/* Remet la voie I/O en attente */
		selvoie (save_voie);
		svoie[voiecur]->sta.connect = 17;
		maj_niv (0, 0, 0);

#ifdef __FBBDOS__
		trait (0, " ");
#endif
	}
	else
	{
		retour_niveau ();
		retour_menu (pvoie->niv1);
	}
}


void yapp_message (int numero)
{
	int save_voie;

	if (pvoie->kiss == -2)
	{
		save_voie = voiecur;
		selvoie (CONSOLE);
		texte (numero);
		aff_etat ('E');
		send_buf (CONSOLE);
		selvoie (save_voie);
	}
	else
	{
		texte (numero);
	}
}


static void yapp_timer (void)
{
	if ((pvoie->niv2 == S) || (pvoie->niv2 == S1))
		time_yapp[voiecur] = 20;	/*  20 s */
	else
		time_yapp[voiecur] = 120;	/* 120 s */
}


static void sendinit (void)
{
	int nb;
	uchar buf[20];

	/*  cprintf("Sendinit\r\n") ; */
	pvoie->send_count = 0;
	buf[0] = ENQ;
	buf[1] = 1;
	nb = 2;
	outs (buf, nb);

}


void cancel (char *texte)
{
	int nb = strlen (texte);
	uchar buf[257];

	buf[0] = CAN;
	buf[1] = nb;
	strcpy (buf + 2, texte);
	outs (buf, nb + 2);
	ch_niv2 (CW);
	fin_yapp ();
}


/*
   static void not_ready(char *texte)
   {
   int  nb = strlen(texte) ;
   uchar    buf[257] ;

   buf[0] = NAK ;
   buf[1] = nb ;
   strcpy(buf + 2, texte) ;
   outs(buf, nb + 2) ;
   }
 */

static void sendeof (void)
{
	uchar buf[2];

	buf[0] = ETX;
	buf[1] = 1;
	outs (buf, 2);
}


void sendeot (uchar chck)
{
	uchar buf[2];

	buf[0] = EOT;
	buf[1] = chck;
	outs (buf, 2);
}


static void sendinit_retry (void)
{
	uchar buf[2];

	if (++pvoie->send_count > 6)
	{
		cancel ("Retry count excessive\r");
	}
	else
	{
		buf[0] = ENQ;
		buf[1] = 1;
		outs (buf, 2);
	}
}


long file_size (char *fichier)
{
	struct stat statbuf;

	if (stat (fichier, &statbuf) != -1)
		return (statbuf.st_size);
	return (0L);
}


static void bin_hdr (char *filename, long filesize)
{
	int nb = 0;
	int fd;
	uchar buf[257];
	uchar *ptr = buf;

	union
	{
		unsigned long ytime;
		struct ftime ftime;
	}
	fdir;


	*ptr++ = SOH;
	++ptr;

	strcpy (ptr, filename);
	while (*ptr++)
		++nb;
	++nb;

	sprintf (ptr, "%7ld", filesize);
	while (*ptr++)
		++nb;
	++nb;

	if (pvoie->type_yapp)
	{							/* Yapp Checksum ? */
		if ((fd = open (pvoie->sr_fic, O_RDONLY)) != -1)
		{
			getftime (fd, &(fdir.ftime));
			sprintf (ptr, "%08lX", fdir.ytime);
			while (*ptr++)
				++nb;
			++nb;
			close (fd);
		}
	}

	buf[1] = nb;
	outs (buf, nb + 2);
}


static void sendhdr (char *header)
{
	pvoie->tailm = file_size (pvoie->sr_fic);
	bin_hdr (header, pvoie->tailm);
}

static void compute_CRC (int ch, int *crc)
{
	int hibit;
	int shift;

	for (shift = 0x80; shift; shift >>= 1)
	{
		hibit = *crc & 0x8000;
		*crc <<= 1;
		*crc |= (ch & shift ? 1 : 0);
		if (hibit)
			*crc ^= 0x1021;
	}
}

int senddata (unsigned int type)
{
	int fd, mode, retour = 0, nb = 0;
	int i;
	int lg_buf = 250;
	uchar checksum;
	uchar *uptr;
	uchar buf[300];

	/*
	 * Type = 0 : Ascii
	 *        1 : Binaire
	 *        2 : YAPP
	 */

	if (type == 0)
		mode = O_TEXT;
	else
		mode = O_BINARY;

	if ((fd = open (pvoie->sr_fic, mode | O_RDONLY)) != -1)
	{
		pvoie->sr_mem = 1;
		lseek (fd, pvoie->enrcur, 0);
		if (type == 2)
			aff_yapp (voiecur);
		while (1)
		{
			nb = read (fd, buf + 2, lg_buf);
			if (nb > 0)
			{
				pvoie->size_trans += (long) nb;
				if (type == 2)
				{
					buf[0] = STX;
					buf[1] = (nb == 256) ? 0 : nb;
					if (pvoie->type_yapp)
					{
						uptr = buf + 2;
						checksum = 0;
						for (i = 0; i < nb; i++)
							checksum += *uptr++;
						*uptr = checksum;
						outs (buf, nb + 3);
					}
					else
					{
						outs (buf, nb + 2);
					}

				}
				else
				{
					uptr = buf + 2;
					if (pvoie->type_yapp == 4)
					{
						for (i = 0; i < nb; i++)
						{
							compute_CRC ((int) *uptr, (int *) &pvoie->checksum);
							++uptr;
						}
					}
					outs (buf + 2, nb);
				}
				if (pvoie->memoc >= MAXMEM)
					break;
			}
			/*
			   if (eof (fd))
			   {
			   pvoie->sr_mem = 0;
			   retour = 1;
			   break;
			   }
			 */
			else
				/* EOF or error */
			{
				pvoie->sr_mem = 0;
				retour = 1;
				break;
			}
		}
		pvoie->enrcur = tell (fd);
		close (fd);
	}
	else
		retour = 1;
	return (retour);
}


static void ack1 (void)
{
	uchar buf[2];

	buf[0] = ACK;
	buf[1] = 1;
	outs (buf, 2);
	pvoie->time_trans = time (NULL);
	ch_niv2 (RH);
}


static void ack2 (void)
{
	int i;

	int nbcar = 0xff & (int) data[1];

	uchar buf[2];

	/* decoder le header */
	while (*indd++)
		;
	pvoie->tailm = atol (indd);

	while (*indd++)
		--nbcar;
	--nbcar;

	if (nbcar)
	{
		/* teste Yapp Chck */
		for (i = 0; i < 4; i++)
		{
			if (!isxdigit (*indd))
				break;
			++indd;
		}
		pvoie->type_yapp = (i == 4);
		aff_yapp (voiecur);
	}


	buf[0] = ACK;
	if (pvoie->type_yapp)
	{
		buf[1] = ACK;
	}
	else
	{
		buf[1] = 2;
	}
	outs (buf, 2);
	pvoie->time_trans = time (NULL);
	ch_niv2 (RD);
}


static void ack3 (void)
{
	uchar buf[2];

	/* le fichier est valide */

	buf[0] = ACK;
	buf[1] = 3;
	pvoie->xferok = 2;
/*
   pvoie->time_trans = time(NULL) - pvoie->time_trans;
 */
	write_mess_temp (O_BINARY, voiecur);
	if (test_temp (voiecur))
	{
		rename_temp (voiecur, pvoie->sr_fic);	/* Le fichier est mis en place */
		wr_dir (pvoie->sr_fic, pvoie->sta.indicatif.call);
	}
	outs (buf, 2);
	ch_niv2 (RH);

}


static void ack4 (void)
{
	uchar buf[2];

	buf[0] = ACK;
	buf[1] = 4;
	outs (buf, 2);
	fin_yapp ();
}


static void ack5 (void)
{
	uchar buf[2];

	buf[0] = ACK;
	buf[1] = 5;
	outs (buf, 2);
	ch_niv2 (CW);
}


/*
   static int write_fich(char *fichier, char *texte, int nbcar)
   {
   FILE * fd ;

   if (fd = fopen(fichier, "ab")) {
   fwrite(texte, nbcar, 1, fd) ;
   fclose(fd) ;
   return(1) ;
   }
   return(0) ;
   }
 */

/*
   static int write_temp(char *texte, int nbcar)
   {
   FILE * fd ;
   char temp[128];

   if (fd = fopen(temp_name(voiecur, temp), "ab")) {
   fwrite(texte, nbcar, 1, fd) ;
   fclose(fd) ;
   return(1) ;
   }
   return(0) ;
   }
 */

void wrbuf (void)
{
	uchar checksum = 0;
	int i;
	int ncars;
	int ok = 1;
	int nbcar = (data[1]) ? data[1] & 0xff : 256;
	uchar *uptr;
	obuf *msgtemp;
	char *ptcur;
	char *ptr;

	if (pvoie->type_yapp)
	{
		uptr = data + 2;
		for (i = 0; i < nbcar; i++)
			checksum += *uptr++;
		if (*uptr != checksum)
		{
			cancel ("Checksum error\r");
			return;
		}
	}

#if 0
	/* if (write_fich(pvoie->sr_fic, data + 2, nbcar)) { */
	if (write_temp (data + 2, nbcar))
	{
		pvoie->enrcur += (long) nbcar;
		/*  cprintf("Recoit paquet de %d octets - Nbtrait = %d\r\n", (int) data[1] & 0xff, nb_trait) ; */
		ch_niv2 (RD);
	}
	else
	{
		cancel ("File write error\r");
	}
#else
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
	ptr = data + 2;
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
		ok = write_mess_temp (O_BINARY, voiecur);
	}
	if (ok)
	{
		/*  cprintf("Recoit paquet de %d octets - Nbtrait = %d\r\n", (int) data[1] & 0xff, nb_trait) ; */
		ch_niv2 (RD);
	}
	else
	{
		cancel ("File write error\r");
	}
#endif
}


static void canwait (void)
{
}


static void canrecd (void)
{
	uchar buf[2];

	clear_outbuf (voiecur);
	buf[0] = ACK;
	buf[1] = 5;
	outs (buf, 2);
	fin_yapp ();
}


void out_txt (void)
{
	int lg = 0xff & (int) data[1];

	if (lg && !svoie[CONSOLE]->sta.connect)
	{
		aff_header (voiecur);
		aff_bas (voiecur, W_RCVT, data + 2, lg);
	}
}


static void yapp_xfer (void)
{
	int old_niv2 = pvoie->niv2;

	pvoie->lignes = -1;			/* Pas de pagination */
	ch_niv2 (etat[pvoie->niv2][ptype]);
	yapp_timer ();

	switch (pvoie->niv2)
	{
	case SA:
		pvoie->finf.download += (int) (pvoie->size_trans / 1024L);
		fin_yapp ();
		break;
	case S:
		sendinit ();
		break;
	case S1:
		sendinit_retry ();
		break;
	case SH:
		sendhdr (pvoie->appendf);
		break;
	case SD:
		time_yapp[voiecur] = -1;
		if (pvoie->kiss == -2)
			init_timout (CONSOLE);
		if (senddata (2))
		{
			sendeof ();
			ch_niv2 (SE);
		}
		break;
	case SE:
		sendeof ();
		break;
	case ST:
		sendeot (1);
		break;
	case CW:
		canwait ();
		break;
	case C:
		canrecd ();
		break;
	case E1:
		ack1 ();
		break;
	case E2:
		ack2 ();
		break;
	case E3:
		ack3 ();
		break;
	case E4:
		ack4 ();
		break;
	case E5:
		ack5 ();
		break;
	case E6:
		wrbuf ();
		break;
	case AB:
		cancel ("Abort\r");
		break;
	case RP:
		reprise_yapp ();
		break;
	case DP:
		out_txt ();
		ch_niv2 (old_niv2);
		break;
	case RH:
		break;
	default:
		cancel ("Protocol error\r");
		break;
	}
	aff_yapp (voiecur);
}


int dir_ok (char *masque)
{
	int n;
	char temp[80];

#ifdef __LINUX__
	static char chaine[] = "\\*";

#else
	static char chaine[] = "\\*.*";

#endif

	strcpy (temp, tot_path (ch_slash (masque), pvoie->dos_path));
	n = strlen (temp);
	if ((n > 3) && (temp[n - 1] == '\\'))
		temp[n - 1] = '\0';
	if (is_dir (temp))
	{
		if (temp[n - 1] == '\\')
			temp[n - 1] = '\0';
		strcat (temp, chaine);
	}
	if (findfirst (temp, &(pvoie->dirblk), FA_DIREC))
	{
		texte (T_DOS + 2);
		return (FALSE);
	}
	if (*pvoie->dirblk.ff_name == '.')
	{
		findnext (&(pvoie->dirblk));
		if (findnext (&(pvoie->dirblk)))
		{
			texte (T_DOS + 2);
			return (FALSE);
		}
	}
	return (TRUE);
}


static int dir_new_suite (void)
{
	Rlabel rlabel;
	FILE *fptr;
	int aff = 0;
	int lg;
	char *ptr;
	char *scan;
	char cur_dir[80];

	pvoie->sr_mem = pvoie->seq = FALSE;

	/* sprintf(cur_dir, "%c:%s", pvoie->vdisk, pvoie->dos_path); */

	if ((fptr = fopen (d_disque ("YAPPLBL.DAT"), "rb")) == NULL)
		return (2);

	for (; pvoie->vdisk < 9; ++pvoie->vdisk)
	{
		if (*PATH[(int)pvoie->vdisk] == '\0')
			continue;

		if ((pvoie->vdisk == 8) && (*pvoie->finf.priv == '\0'))
			continue;

		if ((pvoie->temp1 == N_YAPP) &&
			(strncmp (YAPPDIR, PATH[(int)pvoie->vdisk],
					  strlen (PATH[(int)pvoie->vdisk])) != 0))
		if (pvoie->temp1 == N_YAPP)
		{
			ptr = back2slash(PATH[(int)pvoie->vdisk]);
			if (strncmp (YAPPDIR, ptr, strlen(ptr)) != 0)
				continue;
		}

		sprintf (cur_dir, "%c:%s", pvoie->vdisk + 'A', pvoie->dos_path);
		lg = strlen (cur_dir);

		fseek (fptr, pvoie->enrcur, 0);

		while (fread (&rlabel, sizeof (Rlabel), 1, fptr))
		{

			int ok = 1;

			if (rlabel.date_creation < pvoie->noenr_menu)
				ok = 0;
			else if (strncmp (rlabel.nomfic, cur_dir, lg) != 0)
				ok = 0;

			if ((ok) && (findfirst (tot_path (rlabel.nomfic, pvoie->dos_path), &(pvoie->dirblk), 0) == 0))
			{

				ptr = strrchr (rlabel.nomfic, '\\');
				if (ptr == NULL)
					continue;
				*ptr = '\0';

				scan = rlabel.nomfic;
				ptr = scan + lg - 1;

				*scan++ = (pvoie->vdisk == 8) ? 'P' : pvoie->vdisk + 'A';
				++scan;

				if (strlen (ptr) > 1)
				{
					while ((*scan++ = *ptr++) != '\0');
				}
				else
					*(scan + 1) = '\0';

				if (strcmp (rlabel.nomfic, pvoie->appendf) != 0)
				{
					strcpy (pvoie->appendf, rlabel.nomfic);
					cr ();
					if (pvoie->temp1 == N_YAPP)
					{
						out ("YAPP:", 5);
						out (pvoie->appendf + 2, strlen (pvoie->appendf) - 2);
					}
					else
					{
						out (pvoie->appendf, strlen (pvoie->appendf));
					}
					cr ();
					/* outln (":", 1); */
				}

				sprintf (varx[0], "%-20s", pvoie->dirblk.ff_name);
				if ((pvoie->dirblk.ff_attrib & FA_DIREC) != 0)
				{
					var_cpy (1, "<DIR>  ");
				}
				else
				{
					sprintf (varx[1], "%7ld", pvoie->dirblk.ff_fsize);
				}
				var_cpy (2, dir_date (pvoie->dirblk.ff_fdate));
				*varx[3] = *varx[4] = '\0';
				n_cpy (LABEL_NOM, varx[4], rlabel.label);
				out ("  ", 2);
				texte (T_YAP + 4);
				aff = 1;
			}

			if (pvoie->memoc >= MAXMEM)
			{
				pvoie->sr_mem = TRUE;
				pvoie->enrcur = ftell (fptr);
				fclose (fptr);
				return (0);
			}
		}

		strcpy (pvoie->dos_path, "\\");
		pvoie->enrcur = 0;
	}
	fclose (fptr);
	if (!aff)
		return (2);
	return (1);
}

int dir_new (void)
{
	int fin = 0;

	switch (pvoie->niv3)
	{
	case 0:
		pvoie->temp3 = pvoie->vdisk;
		pvoie->cmd_new = 1;
		pvoie->vdisk = 0;
		pvoie->noenr_menu = pvoie->finf.lastyap;
		pvoie->enrcur = 0;
		*pvoie->appendf = '\0';
		strcpy (pvoie->dos_path, "\\");

		switch (dir_new_suite ())
		{
		case 2:
			texte (T_DOS + 2);
		case 1:
			fin = 1;
			break;
		case 0:
			ch_niv3 (1);
			break;
		}
		break;
	case 1:
		if (dir_new_suite () != 0)
			fin = 1;
		break;
	}

	if (fin)
	{
		pvoie->vdisk = pvoie->temp3;
		retour_dos ();
		pvoie->temp3 = 0;
		pvoie->cmd_new = 0;
	}

	pvoie->l_yapp = time (NULL);
	return (1);
}

static void aff_liste_labels (Fichier * fichier, char *path, int n)
{
	char *ptr;
	char *lptr;
	FILE *fptr;
	Rlabel rlabel;
	int i;
	char chemin[80];
	char *path_ptr = vir_path (path);

	/* for (i = 0; i < n; i++)
		printf("Fichier={%s} Path={%s} n=%d\n", fichier[i].nom, path, n); */
	if (path_ptr)
	{
		if ((fptr = fopen (d_disque ("YAPPLBL.DAT"), "rb")) != NULL)
		{
			while (fread (&rlabel, sizeof (Rlabel), 1, fptr))
			{

				ptr = rlabel.nomfic;

				n_cpy (79, chemin, ptr);
				lptr = strrchr (chemin, '\\');
				if (lptr == NULL)
					continue;
				*(lptr + 1) = '\0';

				lptr = strrchr (ptr, '\\');
				++lptr;

				if (strcmpi (path_ptr, chemin) != 0)
					continue;

				for (i = 0; i < n; i++)
				{
					if (strcmpi (fichier[i].nom, lptr) == 0)
					{
						sprintf (varx[0], "%-20s", fichier[i].nom);
						if ((fichier[i].attr & FA_DIREC) != 0)
						{
							var_cpy (1, "<DIR>  ");
						}
						else
						{
							sprintf (varx[1], "%7ld", fichier[i].size);
						}
						var_cpy (2, dir_date (fichier[i].date));
						*varx[3] = *varx[4] = '\0';
						n_cpy (LABEL_NOM, varx[4], rlabel.label);
						texte (T_YAP + 4);
						fichier[i].nom[0] = '\0';
						break;
					}
				}
			}
			fclose (fptr);
		}
	}

	/* Affiche les fichiers non labelises */

	for (i = 0; i < n; i++)
	{
		if (*fichier[i].nom)
		{
			sprintf (varx[0], "%-20s", fichier[i].nom);
			if ((fichier[i].attr & FA_DIREC) != 0)
			{
				var_cpy (1, "<DIR>  ");
			}
			else
			{
				sprintf (varx[1], "%7ld", fichier[i].size);
			}
			var_cpy (2, dir_date (fichier[i].date));
			*varx[3] = *varx[4] = '\0';
			texte (T_YAP + 4);
		}
	}
}

#define MAX_FILES 1000

int dir_yapp (char *masque)
{
	int aff = 0;
	int n;
	char temp[128];
	char *ptr;
	Fichier *fichier;

	fichier = (Fichier *) m_alloue (MAX_FILES * sizeof (Fichier));

	strcpy (temp, tot_path (ch_slash (masque), pvoie->dos_path));

	n = strlen (temp);
	if ((n > 3) && (temp[n - 1] == '\\'))
		temp[n - 1] = '\0';
	if (is_dir (temp))
	{
		strcat (temp, "\\");
	}
	else
	{
		ptr = strrchr (temp, '\\');
		if (ptr)
			*++ptr = '\0';
	}

	if ((*masque == '\0') || (dir_ok (masque)))
	{

		n = 0;
		while (1)
		{
			strcpy (fichier[n].nom, pvoie->dirblk.ff_name);
			fichier[n].size = pvoie->dirblk.ff_fsize;
			fichier[n].date = pvoie->dirblk.ff_fdate;
			fichier[n].attr = pvoie->dirblk.ff_attrib;
			aff = 1;

			if (++n == MAX_FILES)
				break;

			if (findnext (&(pvoie->dirblk)))
				break;
		}

#ifdef __LINUX__
		ptr = slash2back (temp);
#else
		ptr = temp;
#endif
		aff_liste_labels (fichier, ptr, n);

		if (!aff)
			texte (T_DOS + 2);
	}

	m_libere (fichier, MAX_FILES * sizeof (Fichier));

	return (0);
}

void yapp (void)
{
	char ptr[256];
	char *scan;

	switch (pvoie->niv2)
	{
	case 40:
		scan = get_nextparam();
		if (scan)
			strcpy (ptr, scan);
		else
#ifdef __LINUX__
			strcpy (ptr, "*");
#else
			strcpy (ptr, "*.*");
#endif
		dir_suite (ptr);
		retour_dir ((YAPPDIR[1] == ':') ? (YAPPDIR[0] - 'A') : getdisk ());
		break;

	case 42:
		scan = get_nextparam();
		if (scan)
			strcpy (ptr, scan);
		else
#ifdef __LINUX__
			strcpy (ptr, "*");
#else
			strcpy (ptr, "*.*");
#endif
		dir_yapp (ptr);

		retour_dir ((YAPPDIR[1] == ':') ? (YAPPDIR[0] - 'A') : getdisk ());
		break;

	case 43:
		maj_niv (9, 12, 0);
		dir_new ();
		break;

	case 50:
		set_binary (voiecur, 0);
		if (!pvoie->xferok)
			del_temp (voiecur);
		else if (pvoie->xferok == 2)
		{
			/*
			   ltoa(pvoie->enrcur, varx[0], 10) ;
			   texte(T_DOS + 5) ;
			 */
			display_perf (voiecur);
		}
		aff_yapp (voiecur);
		pvoie->xferok = 1;
		aff_header (voiecur);
		retour_appel ();
		break;

	case 51:
		var_cpy (0, "YAPP");
		new_label ();
		pvoie->tailm = pvoie->enrcur = 0L;
		pvoie->size_trans = 0L;
		ch_niv2 (R);
		texte (T_YAP + 1);
		set_binary (voiecur, 1);
		pvoie->xferok = 0;
		del_temp (voiecur);
		yapp_timer ();
		aff_yapp (voiecur);
		break;

	case 52:
		change_label ();
		retour_appel ();
		break;

	default:
		yapp_xfer ();
		break;
	}
}


char *nom_yapp (void)
{
	char fichier[MAXPATH], dr[MAXDIR], di[MAXDIR], fi[FBB_NAMELENGTH], ex[MAXEXT];
	
	*fichier = '\0';
	*dr      = '\0';
	*di      = '\0';
	*fi      = '\0';
	*ex      = '\0';

	teste_espace ();
	n_cpy (MAXPATH, fichier, indd);
	if (!tst_point (fichier))
		*pvoie->sr_fic = '\0';
	else
	{
#ifdef __LINUX__
		strcpy (pvoie->sr_fic, slash2back (tot_path (ch_slash (fichier), pvoie->dos_path)));
#else
		strcpy (pvoie->sr_fic, tot_path (ch_slash (fichier), pvoie->dos_path));
#endif
		fnsplit (fichier, dr, di, fi, ex);
		strcpy (pvoie->appendf, fi);
		strcat (pvoie->appendf, ex);
	}
	return (pvoie->sr_fic);
}


static void change_label (void)
{
	char *ptr;
	
	ptr = get_nextparam();
	if (ptr)
	{
		w_label (pvoie->sr_fic, ptr);
		cr();
		dir_yapp(pvoie->appendf);
	}
}


void new_label (void)
{
	while ((*indd) && (!ISPRINT (*indd)))
		++indd;
	w_label (pvoie->sr_fic, sup_ln (indd));
	n_cpy (LABEL_NOM - 1, pvoie->label, sup_ln (indd));
}


int menu_yapp (void)
{
	struct stat bufstat;
	char buf[80];
	int ok = 1;
	int c, fd;

	if (ISGRAPH (*(indd + 1)))
		return (0);

	if (pvoie->temp1 != N_DOS)
		strcpy (pvoie->dos_path, "\\");

	var_cpy (0, "YAPP");

	limite_commande ();
	// strupr (sup_ln (indd - 1));
	n_cpy (78, buf, indd);

	c = *indd++;

	switch (toupper(c))
	{
	case 'D':
		if ((voiecur == CONSOLE) || ((!P_YAPP (voiecur)) && (!SYS (pvoie->finf.flags))))
		{
			/* ok = 0; */
			yapp_message (T_YAP + 2);
			retour_appel ();
			break;
		}
#ifdef MULTI
		if (ymodem_files ())
		{
			ch_niv2 (S);
			if (pvoie->kiss != -2)
				texte (T_YAP + 0);
			pvoie->tailm = pvoie->enrcur = 0L;
			pvoie->size_trans = 0L;
			set_binary (voiecur, 1);
			pvoie->xferok = 1;
			pvoie->type_yapp = 1;
			sendinit ();
			yapp_timer ();
			aff_yapp (voiecur);
		}
#else
		if ((tst_point (indd)) &&
			(stat (nom_yapp (), &bufstat) != -1) &&
			((bufstat.st_mode & S_IFREG) != 0))
		{
			ch_niv2 (S);
			if (pvoie->kiss != -2)
				texte (T_YAP + 0);
			pvoie->tailm = pvoie->enrcur = 0L;
			pvoie->size_trans = 0L;
			set_binary (voiecur, 1);
			pvoie->xferok = 1;
			pvoie->type_yapp = 1;
			sendinit ();
			yapp_timer ();
			aff_yapp (voiecur);
		}
		else
		{
			/* ok = 0; */
			yapp_message (T_ERR + 11);
			retour_appel ();
		}
#endif
		break;

	case 'U':
		if (!is_room ())
		{
			outln ("*** Disk full !", 15);
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
		{
			retour_appel ();
		}
		else if (!isspace (*indd))
		{
			/* ok = 0; */
			yapp_message (T_ERR + 20);
			retour_appel ();
		}
		else
		{
			if ((tst_point (indd)) && (stat (nom_yapp (), &bufstat) == -1))
			{
				/* fd = creat (pvoie->sr_fic, S_IREAD | S_IWRITE); */
				fd = open (pvoie->sr_fic, O_CREAT | O_WRONLY | O_TRUNC, S_IREAD | S_IWRITE);
				if (fd > 0)
				{
					close (fd);
					unlink (pvoie->sr_fic);
					if (pvoie->kiss != -2)
					{
						yapp_message (T_YAP + 3);
						ch_niv2 (51);
					}
					else
					{
						pvoie->tailm = pvoie->enrcur = 0L;
						pvoie->size_trans = 0L;
						ch_niv2 (R);
						set_binary (voiecur, 1);
						pvoie->xferok = 0;
						pvoie->type_yapp = 0;
						yapp_timer ();
						aff_yapp (voiecur);
					}
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

	case 'W':
		ch_niv2 (40);
		pvoie->noenr_menu = 0L;
		yapp ();
		break;

	case 'N':
		ch_niv2 (43);
		pvoie->noenr_menu = pvoie->finf.lastyap;
		yapp ();
		pvoie->l_yapp = time (NULL);
		break;

	case 'I':
		ch_niv2 (42);
		pvoie->noenr_menu = 0L;
		yapp ();
		break;

	case 'L':
		teste_espace();
		if (droits (MODLABEL))
		{
			if (*indd == '\0')
			{
				texte (T_ERR + 20);
				retour_appel ();
			}
			else if (!aut_ecr (indd, 0))
			{
				retour_appel ();
			}
			else
			{
				if (stat (nom_yapp (), &bufstat) != 0)
				{
					texte (T_ERR + 11);
					retour_appel ();
				}
				else
				{
					texte (T_YAP + 3);
					ch_niv2 (52);
				}
			}
		}
		else
		{
			/* retour_niveau();
			   cmd_err(indd - 2) ; */
			ok = 0;
		}
		break;

	case 'Z':
		strcpy (pvoie->appendf, indd);
		nom_yapp ();
		if (aut_dir (pvoie->sr_fic, pvoie->sta.indicatif.call))
		{
			if (stat (pvoie->sr_fic, &bufstat) != -1)
			{
				if (unlink (pvoie->sr_fic) == 0)
					texte (T_DOS + 10);
				else
					texte (T_ERR + 23);
				retour_appel ();
			}
			else
			{
				texte (T_ERR + 11);
				retour_appel ();
			}
		}
		else
		{
			texte (T_ERR + 23);
			/* ok = 0; */
			retour_niveau ();
		}
		break;

	default:
		/*
		   retour_niveau();
		   cmd_err(indd - 2) ;
		 */
		--indd;
		ok = 0;
		break;
	}
	if (ok)
		fbb_log (voiecur, 'Y', buf);
	return (ok);
}
