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

/******************************
 *
 *  DRIVERS pour ligne MODEM
 *
 ******************************/

#include <serv.h>

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#define N_TTY_BUF_SIZE 4096
#endif

#include <fbb_drv.h>
#include <modem.h>

#define ANCIEN 1

extern int GetModemDCD (int port);

/* #if defined (__WINDOWS__) || defined(__linux__) */
#define LOCSIZ 256
static char *locbuf[NBPORT] =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static int locnb[NBPORT] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int echap[NBPORT] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* #endif */


static int bs[NBPORT] =
{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

int echo[NBPORT] =
{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

/*
 * Fonctions g‚n‚riques du driver
 */


int sta_mod (int port, int canal, int cmd, void *ptr)
{
	int ret = 0;
	char command[256];

	switch (cmd)
	{
	case PORTCMD:
		strcpy (command, (char *) ptr);
		strcat (command, "\r");
		ret = snd_mod (port, 0, COMMAND, command, strlen (command), NULL);
		sleep_ (1);
		break;
	case BSCMD:
		bs[port] = *((int *) ptr);
		return (1);
	case ECHOCMD:
		echo[port] = *((int *) ptr);
#ifdef __WIN32__
		SetModemEcho (port, echo[port]);
#endif
		return (1);
	case SUSPCMD:
		return (1);
	}
	return (ret);
}

int snd_mod (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	int ctrl = 0;
	char c;

	switch (cmd)
	{
	case COMMAND:

		while (len--)
		{
			c = *buffer++;

			switch (c)
			{
			case '~':
				deb_io ();
#ifdef __WINDOWS__
				WinMSleep (500);
#else
				tempo = 10;
				while (tempo);
#endif
				break;

			case '^':
				ctrl = 1;
				break;

			default:
				if (ctrl)
				{
					c -= '@';
					ctrl = 0;
				}

#ifdef __linux__
				if (BIOS (port) == P_LINUX)
				{
					defcom *ptrcom = &p_com[(int)p_port[port].ccom];
					int nb;

					if (ptrcom->comfd < 0)
						return (1);

					for (;;)
					{
						int nbcar;

						ioctl (ptrcom->comfd, TIOCOUTQ, &nbcar);
						if ((N_TTY_BUF_SIZE - nbcar) >= 1)
							break;
						sleep_ (1);
					}
					nb = write (ptrcom->comfd, &c, 1);
					nb = N_TTY_BUF_SIZE >> 8;
					p_port[port].frame = (uchar) nb - 2;
				}
#endif
#ifdef __WINDOWS__
				if (BIOS (port) == P_WINDOWS)
				{
#ifdef __WIN32__
					int nb;

					nb = PutModemBuf (port, &c, 1);
#else
					defcom *ptrcom = &p_com[p_port[port].ccom];
					COMSTAT cstat;
					DWORD dwErrorMask;
					int nb;

					if (ptrcom->comfd < 0)
						return (1);

					for (;;)
					{
						GetCommError (ptrcom->comfd, &cstat);
						if ((OUTQUE - cstat.cbOutQue) >= 1)
							break;
						sleep_ (1);
					}
					nb = WriteComm (ptrcom->comfd, &c, 1);
					while (GetCommError (ptrcom->comfd, &cstat));
#endif
					nb = OUTQUE >> 8;
					p_port[port].frame = (uchar) nb - 2;
				}
#endif
#ifdef __FBBDOS__
				{
					RSEGS rsegs;

					BufReel[0] = c;
					memset (&rsegs, 0, sizeof (RSEGS));
					rsegs.CX = 1;
					rsegs.DX = (int) p_port[port].ccom - 1;
					rsegs.DI = 0;
					rsegs.ES = BufSeg;
					rsegs.DS = BufSeg;

					while (rsegs.CX)
					{
						rsegs.AX = 0x0a00;
						int14real (&rsegs);
					}
				}
#endif
				break;
			}
		}
		break;

	case DATA:

#ifdef __linux__
		if (BIOS (port) == P_LINUX)
		{
			defcom *ptrcom = &p_com[(int)p_port[port].ccom];
			int nb;
			int c;
			int i;
			int dbl;

			/* int  binary; */
			char buf[600];

			if (ptrcom->comfd < 0)
				return (1);

			/* binary = svoie[p_port[port].pr_voie]->binary; */

			if (bs[port])
			{
				i = 0;
				for (nb = 0; nb < len; nb++)
				{
					buf[i++] = buffer[nb];
					if (buffer[nb] == '\r')
						buf[i++] = '\n';
				}
			}
			else
			{
				i = 0;
				for (nb = 0; nb < len; nb++)
				{
					c = buffer[nb] & 0xff;
					switch (c)
					{
					case 0x01:	/* Echappement */
					case 0x11:	/* XOn */
					case 0x13:	/* XOff */
					case 0x08:	/* BS */
					case 0x0a:	/* LF */
					case 0x00:	/* Null */
						c += 0x40;
						dbl = 1;
						break;

					default:
						dbl = 0;
						break;
					}
					if (dbl)
					{
						/* Caractere d'echappement */
						buf[i++] = 0x01;
					}

					buf[i++] = c;
				}
			}

			for (;;)
			{
				int nbcar;

				ioctl (ptrcom->comfd, TIOCOUTQ, &nbcar);
				if ((N_TTY_BUF_SIZE - nbcar) >= i)
					break;
				sleep_ (1);
			}
			nb = write (ptrcom->comfd, buf, i);
			nb = N_TTY_BUF_SIZE >> 8;
			p_port[port].frame = (uchar) nb - 2;
		}
#endif
#ifdef __WINDOWS__
		if (BIOS (port) == P_WINDOWS)
		{
#ifdef __WIN32__
			defcom *ptrcom = &p_com[p_port[port].ccom];
			int nb;
			int c;
			int i;
			int dbl;
			char buf[600];

			if (ptrcom->comfd == NULL)
				return (1);

			i = 0;
			if (bs[port])
			{
				for (nb = 0; nb < len; nb++)
				{
					c = buffer[nb] & 0xff;
					buf[i++] = c;

					/* Envoie le LF si pas d'echo ... */
					if (c == '\r')
						buf[i++] = '\n';
				}
			}
			else
			{
				for (nb = 0; nb < len; nb++)
				{
					c = buffer[nb] & 0xff;
					if (!bs[port])
					{
						switch (c)
						{
						case 0x01:		/* Echappement */
						case 0x11:		/* XOn */
						case 0x13:		/* XOff */
						case 0x08:		/* BS */
						case 0x0a:		/* LF */
						case 0x00:		/* Null */
							c += 0x40;
							dbl = 1;
							break;

						default:
							dbl = 0;
							break;
						}
						if (dbl)
						{
							/* Caractere d'echappement */
							buf[i++] = 0x01;
						}
					}
					buf[i++] = c;
				}
			}

			nb = PutModemBuf (port, buf, i);
			nb = OUTQUE >> 8;
			p_port[port].frame = (uchar) nb - 2;
#else
			defcom *ptrcom = &p_com[p_port[port].ccom];
			COMSTAT cstat;
			int nb;
			int c;
			int i;
			int dbl;

/*          int binary; */
			char buf[600];

			if (ptrcom->comfd < 0)
				return (1);

/*          binary = svoie[p_port[port].pr_voie]->binary; */

			i = 0;
			if (bs[port])
			{
				for (nb = 0; nb < len; nb++)
				{
					c = buffer[nb] & 0xff;
					buf[i++] = c;

					/* Envoie le LF si pas d'echo ... */
					if (c == '\r')
						buf[i++] = '\n';
				}
			}
			else
			{
				for (nb = 0; nb < len; nb++)
				{
					c = buffer[nb] & 0xff;
					if (!bs[port])
					{
						switch (c)
						{
						case 0x01:		/* Echappement */
						case 0x11:		/* XOn */
						case 0x13:		/* XOff */
						case 0x08:		/* BS */
						case 0x0a:		/* LF */
						case 0x00:		/* Null */
							c += 0x40;
							dbl = 1;
							break;

						default:
							dbl = 0;
							break;
						}
						if (dbl)
						{
							/* Caractere d'echappement */
							buf[i++] = 0x01;
						}
					}
					buf[i++] = c;
				}
			}
			for (;;)
			{
				GetCommError (ptrcom->comfd, &cstat);
				if ((OUTQUE - cstat.cbOutQue) >= i)
					break;
				sleep_ (1);
			}
			nb = WriteComm (ptrcom->comfd, buf, i);
			while (GetCommError (ptrcom->comfd, &cstat));

			nb = OUTQUE >> 8;
			p_port[port].frame = (uchar) nb - 2;
#endif
		}
#endif
#ifdef __FBBDOS__
		{
			int nb;
			int c;
			int i;
			int dbl;

/*          int binary; */
			char buf[600];

/*          binary = svoie[p_port[port].pr_voie]->binary; */

			i = 0;

			if (bs[port])
			{
				memcpy (buf, buffer, len);
			}
			else
			{
				for (nb = 0; nb < len; nb++)
				{
					c = buffer[nb] & 0xff;
					switch (c)
					{
					case 0x01:	/* Echappement */
					case 0x11:	/* XOn */
					case 0x13:	/* XOff */
					case 0x0a:	/* LF */
					case 0x08:	/* BS */
					case 0x00:	/* Null */
						c += 0x40;
						dbl = 1;
						break;

					default:
						dbl = 0;
						break;
					}
					if (dbl)
					{
						/* Caractere d'echappement */
						buf[i++] = 0x01;
					}
					buf[i++] = c;

					/* Envoie le LF si pas d'echo ...
					   if ((!binary) && (p_port[port].echo) && (c == '\r')) */
				}

				len = i;
			}

			memcpy (BufReel, buf, len);

			memset (&rsegs, 0, sizeof (RSEGS));
			rsegs.CX = len;
			rsegs.DX = (int) p_port[port].ccom - 1;
			rsegs.DI = 0;
			rsegs.ES = BufSeg;
			rsegs.DS = BufSeg;

			while (rsegs.CX)
			{
				rsegs.AX = 0xa00;
				if (rsegs.CX != len)
					sleep_ (1);
				int14real (&rsegs);
			}
			p_port[port].frame = (uchar) (rsegs.BX >> 9) - 2;
		}
#endif
		return (1);

	case UNPROTO:
		break;
	}
	return (1);
}

void end_modem (void)
{
#if defined(__WINDOWS__) || defined(__linux__)
	int i;

	for (i = 0; i < NBPORT; i++)
	{
		if (locbuf[i])
		{
			m_libere (locbuf[i], LOCSIZ);
			locbuf[i] = NULL;
		}
	}
#endif
}

int lit_port_modem (int port)
{
	int fin_rxmodem;
	int fin_txmodem;
	int abort_xmodem; 
	int zr_pos;
	int nb, con;
	int nbtot;
	int voie;
	int ch_stat;
	Forward *pfwd;

	if (DEBUG)
		return (0);

	df ("lit_port_modem", 3);

	voie = p_port[port].pr_voie;
	pfwd = svoie[voie]->curfwd;

	ch_stat = 0;

#ifdef __linux__
	if (BIOS (port) == P_LINUX)	/* LINUX driver */

	{
		defcom *ptrcom = &p_com[(int)p_port[port].ccom];
		char buffer[300];

		if (ptrcom->comfd < 0)
			return 0;

		for (;;)
		{
			int mcs;
			int old;

			if (svoie[voie]->binary == 2)
				nb = 0;
			else
			{
				old = fcntl (ptrcom->comfd, F_GETFL, 0);
				(void) fcntl (ptrcom->comfd, F_SETFL, old | O_NDELAY);
				nb = read (ptrcom->comfd, buffer, 300);
				(void) fcntl (ptrcom->comfd, F_SETFL, old);
			}

			if (nb < 0)
			{
				/* perror("com read"); */
				nb = 0;
			}

			/* Lit l'etat du DCD */
			ioctl (ptrcom->comfd, TIOCMGET, &mcs);
			con = (mcs & TIOCM_CAR) ? 1 : 0;

			fin_rxmodem = 0;
			fin_txmodem = 0;
			abort_xmodem = 0;
			zr_pos = 0;

			if ((svoie[voie]->sta.connect > 1) && (svoie[voie]->sta.connect < 17) && (!con))
			{
				md_no_echo (voie);
				if (svoie[voie]->kiss >= 0)
				{
					init_timout (voie);
					selvoie (svoie[voie]->kiss);
					/* outln("MODEM DISCONNECTION", 20) ; */
					ch_niv3 (2);
					texte (T_GAT + 1);
					selvoie (voie);
					status (voie);
					svoie[voie]->sta.connect = 17;
				}
				else
				{
					dec_voie (voie);
					re_init_modem (voie);
					echap[port] = 0;
					ch_stat = 1;
				}
			}
			if (nb)
			{
				if (con)
				{
					int c;
					int i;
					char *lbuf = locbuf[port];
					int lnb = locnb[port];

					if (lbuf == NULL)
					{
						lbuf = locbuf[port] = malloc (LOCSIZ);
						lnb = locnb[port] = 0;
					}

					if (svoie[voie]->binary == 2)
					{
						/* Envoie les donnees au process du FORK */
						int fd = svoie[voie]->to_rzsz[1];

						nb = write (fd, buffer, nb);
						if (nb < 0)
							nb = 0;
						if (nb)
						{
							/* int i;
							   dprintf("Envoye %d carac a rszs(fd = %d)\n", nb, fd);
							   for (i = 0 ; i < nb ; i++)
							   dprintf("%02x ", buffer[i] &0xff);
							   dprintf("\n"); */
						}
					}
					else
					{
						for (i = 0; i < nb; i++)
						{
							c = buffer[i] & 0xff;

							if (echap[port])
							{
								c -= 0x40;
								echap[port] = 0;
							}
							else if (c == 0x01)
							{
								/* Caractere d'echappement */
								echap[port] = c;
								continue;
							}
							else if ((c == 0x08) /* && (bs[port]) */ )
							{
								if (lnb > 0)
								{
									selvoie (voie);
									outs ("\b \b", 3);
									--lnb;
								}
								continue;
							}
							else if ((c == 0x0d) ||		/* CR */
									 (c == 0x11) ||		/* XOn */
									 (c == 0x13) ||		/* XOff */
									 (c == 0x00))	/* Null */
							{
								/* A jeter !! */
								continue;
							}
							else if (c == '\n')
							{
								/* transformer les LF en CR */
								c = '\r';
							}

							lbuf[lnb++] = c;
							if ((c == '\r') || (lnb == LOCSIZ) ||
								((svoie[voie]->binary) && (nb == i + 1)))
							{
								if (svoie[voie]->kiss >= 0)
								{
									init_timout (voie);
									selvoie (svoie[voie]->kiss);
									outs (lbuf, lnb);
									write_capture (lbuf, lnb);
									selvoie (voie);
								}
								else
								{
									md_inbuf (voie, lbuf, lnb);
									if ((pfwd) && (*pfwd->txt_con))
									{
										md_send (no_port (voie), pfwd->txt_con);
										*pfwd->txt_con = '\0';
									}
								}
								lnb = 0;
							}
						}
						locnb[port] = lnb;
					}
				}
				else if (md_busy (buffer, nb))
				{
					/* Arret de la connexion */
					md_no_echo (voie);
					if (svoie[voie]->sta.connect)
					{
						dec_voie (voie);
						deconnect_modem (voie);
						re_init_modem (voie);
						echap[port] = 0;
						ch_stat = 1;
					}
				}
			}
			else
			{
				if (con)
				{
					if (svoie[voie]->sta.connect <= 1)
					{
						sleep_ (1);
						connect_modem (voie);
						nb_trait = 0;
						status (voie);
						traite_voie (voie);
					}
					else if (svoie[voie]->sta.connect == 17)
					{
						sleep_ (1);
						init_timout (voie);
						selvoie (svoie[voie]->kiss);
						ch_niv3 (3);
						selvoie (voie);
						status (voie);
						svoie[voie]->sta.connect = 2;
						md_defaut (voie);
					}
				}
				break;
			}
		}

		if (ch_stat)
		{
			nb_trait = 0;
			traite_voie (voie);
		}

		/* Lire le nb de buffers dispos en emission */
		{
			int nbcar;

			if (ioctl (ptrcom->comfd, TIOCOUTQ, &nbcar) == -1)
				perror ("ioctl");
			/* printf("buf = %d\n", nbcar); */
			nbtot = (nbcar + 127) >> 7;
			p_port[no_port (voie)].frame = (uchar) (N_TTY_BUF_SIZE >> 8) - 1;
			svoie[voie]->sta.mem = (N_TTY_BUF_SIZE >> 7);
			/*
			   nbtot = (cstat.cbOutQue + 127) >> 7;
			   p_port[no_port (voie)].frame = (uchar) (OUTQUE >> 8) - 1;
			   svoie[voie]->sta.mem = (OUTQUE >> 7);
			 */
		}

		if (nbtot != svoie[voie]->sta.ack)
		{
			svoie[voie]->sta.ack = nbtot;
			if ((nbtot > 20) && (svoie[voie]->ch_mon >= 0))
			{
				svoie[voie]->ch_mon = -1;
			}
			ch_stat = 1;
		}

		if (svoie[voie]->sta.connect)
		{
			ack_suiv (voie);
		}

		if ((!svoie[voie]->stop) && (svoie[voie]->seq))
		{						/* Recharge les bufs de sortie */
			if (tot_mem > 10000L)
				traite_voie (voie);
			ff ();
			return (0);
		}

		if ((v_tell == 0) || (v_tell != voie) ||
			((v_tell) && (v_tell == voie)))
		{
			while ((!svoie[voie]->seq) && (inbuf_ok (voie)))
			{
				traite_voie (voie);
				if (svoie[voie]->seq)
					break;
			}
		}

	}
#endif
#ifdef __WINDOWS__
	if (BIOS (port) == P_WINDOWS)	/* WINDOWS driver */

	{
		int nstat;
		COMSTAT cstat;
		defcom *ptrcom = &p_com[p_port[port].ccom];
		char buffer[300];

#ifdef __WIN32__
		int nbused;

		if (ptrcom->comfd == NULL)
			return;
#else
		if (ptrcom->comfd < 0)
			return;
#endif

		for (;;)
		{
#ifdef __WIN32__
			con = GetModemDCD (port);
			if ((con) && (svoie[voie]->niv1 == N_MOD) && (svoie[voie]->niv2 == 4) && ((svoie[voie]->niv3 == XR_EXTERN) || (svoie[voie]->niv3 == XS_EXTERN)))
			{
				if ((GetTransferStat (port)) == 0)
				{
					traite_voie (voie);
					return (0);
				}
				nb = 0;
			}
			else
			{
				nb = GetModemBuf (port, buffer, 300);
			}
#else
			nb = ReadComm (ptrcom->comfd, buffer, 300);

			if (nb < 0)
				nb = -nb;

			while (GetCommError (ptrcom->comfd, &cstat));

			nstat = GetCommStatus (ptrcom->comfd);
			con = (nstat & 0x80) ? 1 : 0;
#endif

			fin_rxmodem = 0;
			fin_txmodem = 0;
			abort_xmodem = 0;
			zr_pos = 0;

			if ((svoie[voie]->sta.connect > 1) && (svoie[voie]->sta.connect < 17) && (!con))
			{
				md_no_echo (voie);
				if (svoie[voie]->kiss >= 0)
				{
					init_timout (voie);
					selvoie (svoie[voie]->kiss);
					/* outln("MODEM DISCONNECTION", 20) ; */
					ch_niv3 (2);
					texte (T_GAT + 1);
					selvoie (voie);
					status (voie);
					svoie[voie]->sta.connect = 17;
				}
				else
				{
					dec_voie (voie);
					re_init_modem (voie);
					echap[port] = 0;
					ch_stat = 1;
				}
			}
			if (nb)
			{
				if (con)
				{
					int c;
					int i;
					char *lbuf = locbuf[port];
					int lnb = locnb[port];

					if (lbuf == NULL)
					{
						lbuf = locbuf[port] = malloc (LOCSIZ);
						lnb = locnb[port] = 0;
					}

					for (i = 0; i < nb; i++)
					{
						c = buffer[i] & 0xff;

						if (echap[port])
						{
							c -= 0x40;
							echap[port] = 0;
						}
						else if (c == 0x01)
						{
							/* Caractere d'echappement */
							echap[port] = c;
							continue;
						}
						else if (c == 0x08)
						{
							if ((lnb > 0) && (bs[port]))
							{
#ifndef __WIN32__
								selvoie (voie);
								outs ("\b \b", 3);
#endif
								--lnb;
							}
							continue;
						}
						else if ((c == 0x0a) ||		/* LF */
								 (c == 0x11) ||		/* XOn */
								 (c == 0x13) ||		/* XOff */
								 (c == 0x00))	/* Null */
						{
							/* A jeter !! */
							continue;
						}

						lbuf[lnb++] = c;
						if ((c == '\r') || (lnb == LOCSIZ) ||
							((svoie[voie]->binary) && (nb == i + 1)))
						{
							if (svoie[voie]->kiss >= 0)
							{
								init_timout (voie);
								selvoie (svoie[voie]->kiss);
								outs (lbuf, lnb);
								write_capture (lbuf, lnb);
								selvoie (voie);
							}
							else
							{
								md_inbuf (voie, lbuf, lnb);
								if ((pfwd) && (*pfwd->txt_con))
								{
									md_send (no_port (voie), pfwd->txt_con);
									*pfwd->txt_con = '\0';
								}
							}
							lnb = 0;
						}
					}
					locnb[port] = lnb;
				}
				else if (md_busy (buffer, nb))
				{
					/* Arret de la connexion */
					md_no_echo (voie);
					if (svoie[voie]->sta.connect)
					{
						dec_voie (voie);
						deconnect_modem (voie);
#ifndef __WIN32__
						re_init_modem (voie);
#endif
						echap[port] = 0;
						ch_stat = 1;
					}
				}
			}
			else
			{
				if (con)
				{
					if (svoie[voie]->sta.connect <= 1)
					{
						sleep_ (1);
						connect_modem (voie);
						nb_trait = 0;
						status (voie);
						traite_voie (voie);
					}
					else if (svoie[voie]->sta.connect == 17)
					{
						sleep_ (1);
						init_timout (voie);
						selvoie (svoie[voie]->kiss);
						ch_niv3 (3);
						selvoie (voie);
						status (voie);
						svoie[voie]->sta.connect = 2;
						md_defaut (voie);
					}
				}
				break;
			}
		}

		if (ch_stat)
		{
			nb_trait = 0;
			traite_voie (voie);
		}

#ifdef __WIN32__
		GetModemStat (no_port (voie), &nbtot, &nbused);
		svoie[voie]->sta.mem = nbtot >> 8;
		nbtot = (nbused + 249) >> 8;
#else
		nbtot = (cstat.cbOutQue + 127) >> 7;
		p_port[no_port (voie)].frame = (uchar) (OUTQUE >> 8) - 1;
		svoie[voie]->sta.mem = (OUTQUE >> 7);
#endif

		if (nbtot != svoie[voie]->sta.ack)
		{
			svoie[voie]->sta.ack = nbtot;
			if ((nbtot > 20) && (svoie[voie]->ch_mon >= 0))
			{
				svoie[voie]->ch_mon = -1;
			}
			ch_stat = 1;
		}

		if (svoie[voie]->sta.connect)
		{
			ack_suiv (voie);
		}

		if ((!svoie[voie]->stop) && (svoie[voie]->seq))
		{						/* Recharge les bufs de sortie */
			if (tot_mem > 10000L)
				traite_voie (voie);
			ff ();
			return (0);
		}

		if ((v_tell == 0) || (v_tell != voie) ||
			((v_tell) && (v_tell == voie)))
		{
			while ((!svoie[voie]->seq) && (inbuf_ok (voie)))
			{
				traite_voie (voie);
				if (svoie[voie]->seq)
					break;
			}
		}

	}
#endif
#ifdef __FBBDOS__
	{
		RSEGS rsegs;

		for (;;)
		{
			memset (&rsegs, 0, sizeof (RSEGS));
			rsegs.AX = 0xb00;
			rsegs.CX = 256;
			rsegs.DX = (int) p_port[port].ccom - 1;
			rsegs.DI = 0;
			rsegs.ES = BufSeg;
			rsegs.DS = BufSeg;
			int14real (&rsegs);

			nb = (int) rsegs.CX;

			con = (rsegs.AX & 0x80) ? 1 : 0;

			fin_rxmodem = ((nb == 0) && (rsegs.BX & 0x1000));
			fin_txmodem = ((rsegs.BX & 0x0080) == 0);
			abort_xmodem = ((rsegs.BX & 0x4000) != 0);
			zr_pos = ((rsegs.BX & 0x0040) != 0);

			if ((svoie[voie]->type_yapp == 3) && ((rsegs.BX & 0x8000) != 0))
			{
				fin_txmodem = 1;
			}

			if (zr_pos)
				get_zrpos (voie);

			if ((svoie[voie]->time_trans == 0L) && ((rsegs.BX & 0x0100) == 0))
				svoie[voie]->time_trans = time (NULL);

			if ((svoie[voie]->sta.connect > 1) && (svoie[voie]->sta.connect < 17) && (!con))
			{
				md_no_echo (voie);
				if (svoie[voie]->kiss >= 0)
				{
					init_timout (voie);
					selvoie (svoie[voie]->kiss);
					/* outln("MODEM DISCONNECTION", 20) ; */
					ch_niv3 (2);
					texte (T_GAT + 1);
					selvoie (voie);
					status (voie);
					svoie[voie]->sta.connect = 17;
				}
				else
				{
					dec_voie (voie);
					re_init_modem (voie);
					echap[port] = 0;
					ch_stat = 1;
				}
			}
			if (nb)
			{
				if (con)
				{
					if (svoie[voie]->kiss >= 0)
					{
						init_timout (voie);
						selvoie (svoie[voie]->kiss);
						outs (BufReel, nb);
						write_capture (BufReel, nb);
						selvoie (voie);
					}
					else
					{
						int i;
						int c;
						int lnb = nb;

#if 0
						lnb = 0;
						for (i = 0; i < nb; i++)
						{
							c = BufReel[i] & 0xff;

							if (echap[port])
							{
								c -= 0x40;
								echap[port] = 0;
							}
							else if (c == 0x01)
							{
								echap[port] = c;
								continue;
							}
							else if (c == 0x08)
							{
								continue;
							}
							else if ((c == 0x0a) ||		/* LF */
									 (c == 0x11) ||		/* XOn */
									 (c == 0x13) ||		/* XOff */
									 (c == 0x00))	/* Null */
							{
								continue;
							}

							BufReel[lnb++] = c;
						}
#endif
						md_inbuf (voie, BufReel, lnb);
						if ((pfwd) && (*pfwd->txt_con))
						{
							md_send (no_port (voie), pfwd->txt_con);
							*pfwd->txt_con = '\0';
						}
					}
				}
				else if (md_busy (BufReel, nb))
				{
					/* Arret de la connexion */
					md_no_echo (voie);
					if (svoie[voie]->sta.connect)
					{
						dec_voie (voie);
						deconnect_modem (voie);
						re_init_modem (voie);
						echap[port] = 0;
						ch_stat = 1;
					}
				}
			}
			else
			{
				if (con)
				{
					if (svoie[voie]->sta.connect <= 1)
					{
						sleep_ (1);
						connect_modem (voie);
						nb_trait = 0;
						status (voie);
						traite_voie (voie);
					}
					else if (svoie[voie]->sta.connect == 17)
					{
						sleep_ (1);
						init_timout (voie);
						selvoie (svoie[voie]->kiss);
						ch_niv3 (3);
						selvoie (voie);
						status (voie);
						svoie[voie]->sta.connect = 2;
						md_defaut (voie);
					}
				}
				break;
			}
		}

		if (ch_stat)
		{
			nb_trait = 0;
			traite_voie (voie);
		}

		if ((svoie[voie]->binary) && (abort_xmodem))
		{
			xmodem_fin (voie);
			svoie[voie]->niv3 = XM_ABORT;
			traite_voie (voie);
		}

		memset (&rsegs, 0, sizeof (RSEGS));
		rsegs.AX = 0x0a00;
		rsegs.CX = 0;
		rsegs.DX = (int) p_port[port].ccom - 1;
		rsegs.DI = 0;
		rsegs.ES = BufSeg;
		rsegs.DS = BufSeg;
		int14real (&rsegs);

		nbtot = (int) ((rsegs.BX >> 8) - (rsegs.BX & 0xff));
		p_port[no_port (voie)].frame = (uchar) (rsegs.BX >> 9) - 1;
		svoie[voie]->sta.mem = (int) (rsegs.BX >> 8) + 4;

		if (nbtot != svoie[voie]->sta.ack)
		{
			svoie[voie]->sta.ack = nbtot;
			if ((nbtot > 20) && (svoie[voie]->ch_mon >= 0))
			{
				svoie[voie]->ch_mon = -1;
			}
			ch_stat = 1;
		}

		if (svoie[voie]->sta.connect)
		{
			ack_suiv (voie);
		}

		if ((svoie[voie]->binary) && (svoie[voie]->outptr == NULL) && (svoie[voie]->niv3 == XS_QUEUE))
		{
			xmodem_fin (voie);
			svoie[voie]->niv3 = XS_END;
		}

#ifdef ANCIEN
		if ((svoie[voie]->binary) && (fin_txmodem) && (svoie[voie]->niv3 == XS_END))
		{
			traite_voie (voie);
		}
#else
		if ((svoie[voie]->binary) && (fin_txmodem))
		{
			traite_voie (voie);
		}
#endif

		if ((!svoie[voie]->stop) && (svoie[voie]->seq))
		{						/* Recharge les bufs de sortie */
			if (tot_mem > 10000L)
				traite_voie (voie);
			ff ();
			return (0);
		}

		if ((v_tell == 0) || (v_tell != voie) ||
			((v_tell) && (v_tell == voie)))
		{
			while ((!svoie[voie]->seq) && (inbuf_ok (voie)))
			{
				traite_voie (voie);
				if (svoie[voie]->seq)
					break;
			}
		}

		if ((svoie[voie]->binary) && (fin_rxmodem) && (svoie[voie]->niv3 == XR_RECV))
		{
			svoie[voie]->xferok = 1;
			svoie[voie]->niv3 = XR_END;
			traite_voie (voie);
		}

	}
#endif
	if (ch_stat)
		status (voie);

	ff ();
	return (0);
}
