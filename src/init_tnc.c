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

int inittnc (void)
{
	FILE *fpinit;
	char s[80];
	int port, port_ok, reset;
	int d_init;
	int lig, col, i, ok;
	int ok_init[NBPORT];

#ifdef ENGLISH
#ifdef __linux__
	cprintf ("TNC ports set-up              \n");
#else
	cprintf ("TNC ports set-up              \r\n");
#endif
#else
#ifdef __linux__
	cprintf ("Initialisation des ports TNC\n");
#else
	cprintf ("Initialisation des ports TNC\r\n");
#endif
#endif

#ifdef __WINDOWS__
	DisplayResync (port, 0);
#endif

	if (DEBUG)
	{
#if defined(__WINDOWS__) || defined(__linux__)
		InitText ("TEST Mode");
#else
		cprintf ("Debug valide\r\n");
#endif
		return (1);
	}
	for (port = 1; port < NBPORT; port++)
		ok_init[port] = -1;
	deb_io ();
	col = 10;
	lig = 2;
	port_ok = 1;
	for (port = 1; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			selcanal (port);
#ifdef __WINDOWS__
			if (ETHER (port))
			{
				init_socket (port);
			}
			else
#endif
#ifdef __linux__
				/* if (ETHER (port)) */
			if ((LINUX (port)) && (p_port[port].typort == TYP_ETH))
			{
				if (!opn_drv (port, p_port[port].nb_voies))
				{
					char str[256];

					sprintf (str, "Error init TCP port %d", port);
					WinMessage (5, str);
					continue;
				}
			}
			else if ((LINUX (port)) && (p_port[port].typort == TYP_TCP))
			{
				if (!opn_drv (port, p_port[port].nb_voies))
				{
					char str[256];

					sprintf (str, "Error init TELNET port %d", port);
					WinMessage (5, str);
					continue;
				}
			}
			else if ((LINUX (port)) && (p_port[port].typort == TYP_POP))
			{
				if (!opn_drv (port, p_port[port].nb_voies))
				{
					char str[256];

					sprintf (str, "Error init POP port %d", port);
					WinMessage (5, str);
					continue;
				}
			}
			else if ((LINUX (port)) && (p_port[port].typort == TYP_SCK))
			{
				if (!opn_drv (port, p_port[port].nb_voies))
				{
					char str[256];

					sprintf (str, "Error init LINUX port %d", port);
					WinMessage (5, str);
					continue;
				}
			}
			else
#endif
			if (BPQ (port))
			{
				switch (p_port[port].typort)
				{
#if defined(__WINDOWS__) || defined(__FBBDOS__)
				case TYP_FLX:	/* Mode FLEX */
					cprintf ("Interface FLEX\r\n");
					if (!opn_drv (port, p_port[port].nb_voies))
					{
						char str[256];

						sprintf (str, "Error init FLEX port %d", port);
						WinMessage (5, str);
						continue;
					}
					break;
#endif

				case TYP_DED:
					sprintf (s, "UR%d", p_port[port].nb_voies);
					break;
				case TYP_BPQ:
					cprintf ("Interface BPQNODE\r\n");
					bpq_deconnect = 1;
					break;
				}
			}
			else
			{
				cprintf ("switch\r\n");
				switch (p_port[port].typort)
				{
#if defined(__WIN32__)
				case TYP_FLX:	/* Mode FLEX */
					cprintf ("Interface FLEX\r\n");
					if (!opn_drv (port, p_port[port].nb_voies))
					{
						char str[256];

						sprintf (str, "Error init FLEX port %d", port);
						WinMessage (5, str);
						continue;
					}
					break;
#endif
				case TYP_DED:	/* MODE DED */
					cprintf ("DED HostMode.\r\n");
					d_init = 1;
					for (i = 1; i < NBPORT; i++)
					{
						if (ok_init[i] == p_port[port].ccom)
							d_init = 0;
					}
					if (d_init)
					{
						if (DRSI (port))
							ok_init[port] = p_port[port].ccom;
						if (!opn_drv (port, p_port[port].nb_voies))
						{
							char str[256];

							sprintf (str, "Error init DED port %d", port);
							WinMessage (5, str);
							continue;
						}
					}
					break;

				case TYP_HST:	/* MODE DED */
					cprintf ("PTC HostMode.\r\n");
					d_init = 1;
					for (i = 1; i < NBPORT; i++)
					{
						if (ok_init[i] == p_port[port].ccom)
							d_init = 0;
					}
					if (d_init)
					{
						if (DRSI (port))
							ok_init[port] = p_port[port].ccom;
						if (!opn_drv (port, p_port[port].nb_voies))
						{
							char str[256];

							sprintf (str, "Error init PTC-II port %d", port);
							WinMessage (5, str);
							continue;
						}
					}
					break;

				case TYP_PK:	/* Mode PK232 */
					cprintf ("PK232 HostMode.\r\n");
#ifdef __WINDOWS__
					if (BIOS (port) == P_WINDOWS)
						initcom_windows (p_port[port].ccom, 4096, 4096, CTS | DSR);
#endif
					while (1)
					{
						reset = 4;
						tncstr (port, "*", 0);
						sleep_ (1);
						tncstr (port, "\021\030\003", 0);
						sleep_ (1);
						tncstr (port, "MO 0\r", 0);
						tncstr (port, "HOST ON\r", 0);
						vide (port, 0);
						tncstr (port, "\001\001OGG\027", 0);
						sleep_ (1);
						if (rec_tnc (port) >= 0)
							break;
						/* #pragma warn -rch */
						vide (port, 0);
						sleep_ (2);
						tncstr (port, "\003\003\003", 0);
						sleep_ (2);
						tncstr (port, "\033\030RESTART\r", 0);
#ifdef ENGLISH
						cprintf ("Reset sent ... Please wait.     \r\n");
#else
						cprintf ("Reset envoy‚... Patientez S.V.P.\r\n");
#endif
						i = 20;
						ok = 0;
						while (i--)
						{
							if ((ok = (rcv_tnc (port) >= 0)) != 0)
								break;
							sleep_ (1);
						}
						vide (port, 0);
					}
					vide (port, 0);

					/* Parametres par defaut */
					tnc_commande (port, "HPN", PORTCMD);
					tnc_commande (port, "CETRANS", PORTCMD);
					tnc_commande (port, "AIN", PORTCMD);
					tnc_commande (port, "HDN", PORTCMD);
					tnc_commande (port, "PL0", PORTCMD);
					tnc_commande (port, "MI0", PORTCMD);
					sprintf (s, "ML%s-%d", mycall, myssid);
					tnc_commande (port, s, PORTCMD);
					sprintf (s, "UR%d", p_port[port].nb_voies);
					tnc_commande (port, s, PORTCMD);
					sprintf (s, "MX%d", p_port[port].frame);
					tnc_commande (port, s, PORTCMD);
					break;

				case TYP_MOD:	/* Mode MODEM */
					cprintf ("MODEM Mode\r\n");
					init_modem (port);
					modem_no_echo (port);
					break;

				case TYP_KAM:	/* Mode KAM */
#ifdef __WINDOWS__
					if (BIOS (port) == P_WINDOWS)
						initcom_windows (p_port[port].ccom, 4096, 4096, CTS | DSR);
#endif
					cprintf ("Kantronics HostMode\r\n");
					/* Parametres par defaut */
					tnc_commande (port, "HEADERLN OFF", PORTCMD);
					tnc_commande (port, "HEADERLN OFF", PORTCMD);
					tnc_commande (port, "STATSHRT ON", PORTCMD);
					tnc_commande (port, "PACLEN 0", PORTCMD);
					sprintf (s, "MY %s-%d", mycall, myssid);
					tnc_commande (port, s, PORTCMD);
					if (p_port[port].ccanal == 1)
					{
						sprintf (s, "MAXFRAME /%d", p_port[port].frame);
					}
					else
					{
						sprintf (s, "MAXFRAME %d/", p_port[port].frame);
					}
					tnc_commande (port, s, PORTCMD);
					if (p_port[port].ccanal == 1)
						sprintf (s, "USERS /%d", p_port[port].nb_voies);
					else
						sprintf (s, "USERS %d/", p_port[port].nb_voies);
					tnc_commande (port, s, PORTCMD);
					ok_init[port] = p_port[port].ccom;
					break;

				case TYP_TCP:	/* Mode ETHERNET */
					cprintf ("ETHERNET Mode\r\n");
					break;

#ifdef __WINDOWS__
				case TYP_AGW:	/* Mode ETHERNET */
					if (!opn_drv (port, p_port[port].nb_voies))
					{
						char str[256];

						sprintf (str, "Error init AGW port %d", port);
						WinMessage (5, str);
						continue;
					}
					break;
#endif
				}
			}

			/* Programmed parameters */
			
			/* First, try the "port_name.ini" file */
			sprintf (s, "%s.prt", p_port[port].freq);
			fpinit = fopen (c_disque (s), "rb");

			/* Then try the "inittnc" file */
			if (fpinit == NULL)
			{
				sprintf (s, "inittnc%d.sys", port);
				fpinit = fopen (c_disque (s), "rb");
			}
			if (fpinit)
			{
				while (fgets (s, 80, fpinit))
				{
					sup_ln (s);
					if ((*s) && (*s != '#'))
					{
						tnc_commande (port, s, PORTCMD);
					}
				}
				ferme (fpinit, 2);
			}
#if !defined(__WIN32__) && (defined(__FBBDOS__) || defined(__WINDOWS__))
			if (p_port[port].typort == TYP_BPQ)
			{
				ini_bpq (p_port[port].nb_voies);
			}
#endif

#ifdef ENGLISH
#ifdef __linux__
			cprintf ("End TNC set-up         \n");
#else
			cprintf ("End TNC set-up         \r\n");
#endif
#else
#ifdef __linux__
			cprintf ("Initialisation termin‚e\n");
#else
			cprintf ("Initialisation termin‚e\r\n");
#endif
#endif
			sprintf (s, "OK PORT %d COM%d-%d",
					 port, p_port[port].ccom, p_port[port].ccanal);
#if defined(__WINDOWS__) || defined(__linux__)
			InitText (s);
#endif
			++port_ok;
			lig++;
			col += 2;
		}
	}
	for (port = port_ok - 1; port > 0; port--)
	{
		lig--;
		col -= 2;
	}
	fin_io ();
	return (1);
}
