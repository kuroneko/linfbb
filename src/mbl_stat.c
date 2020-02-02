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
 * Module status BBS
 */

extern Desc *desc;

static char *intf (int port)
{
	static char interf[10];

	switch (p_com[(int)p_port[port].ccom].combios)
	{
	case 1:
		strcpy (interf, "ESS/COM.");
		break;
	case 2:
		strcpy (interf, "BPQ-HOST");
		break;
	case 3:
		strcpy (interf, "FBBIOS  ");
		break;
	case 4:
		strcpy (interf, "DRSI    ");
		break;
	case 5:
		strcpy (interf, "TFPCR   ");
		break;
	case 6:
		strcpy (interf, "WINDOWS ");
		break;
	case 7:
		strcpy (interf, "ETHERNET");
		break;
	case 8:
		strcpy (interf, "TFWIN   ");
		break;
	case 9:
		strcpy (interf, "LINUX   ");
		break;
	default:
		strcpy (interf, "???     ");
		break;
	}
	return (interf);
}

static char *mode_port (int port)
{
	static char mode[10];

	int val;
	int i;
	char *ptr = mode;

	for (i = 0; i < 5; i++)
		*ptr++ = ' ';
	*ptr = '\0';

	val = p_port[port].moport;
	ptr = mode;

	if (val & 0x01)
		*ptr++ = 'G';
	else if (val & 0x02)
		*ptr++ = 'B';
	else
		*ptr++ = 'U';

	if (val & 0x04)
		*ptr++ = 'Y';
	if (val & 0x08)
		*ptr++ = 'M';
	if (val & 0x10)
		*ptr++ = 'W';
	if (val & 0x20)
		*ptr++ = 'L';

	return (mode);
}

static char *typort (int port)
{
	static char protocole[10];

	if (p_com[(int)p_port[port].ccom].combios == 3)
	{							/* Modem */
		strcpy (protocole, "FBBIOS");
	}

	else
	{
		switch (p_port[port].typort)
		{
		case TYP_DED:
			strcpy (protocole, "WA8DED");
			break;
		case TYP_PK:
			strcpy (protocole, "PK232 ");
			break;
		case TYP_KAM:
			strcpy (protocole, "KAM   ");
			break;
		case TYP_BPQ:
			strcpy (protocole, "BPQ   ");
			break;
		case TYP_MOD:
			strcpy (protocole, "MODEM ");
			break;
		case TYP_TCP:
			strcpy (protocole, "TCP-IP");
			break;
		case TYP_SCK:
			strcpy (protocole, "S_AX25");
			break;
		case TYP_AGW:
			strcpy (protocole, "AGW-PE");
			break;
		case TYP_ETH:
			strcpy (protocole, "ETHER ");
			break;
		case TYP_HST:
			strcpy (protocole, "PTC-II");
			break;
		case TYP_FLX:
			strcpy (protocole, "FLEX  ");
			break;
		default:
			strcpy (protocole, "???   ");
			break;
		}
	}

	return (protocole);
}

#if 0
#ifndef __WINDOWS__

typedef struct
{
	unsigned size;
	unsigned nb;
}
bloc;

static int sort_function (const void *a, const void *b)
{
	bloc *ab = (bloc *) a;
	bloc *bb = (bloc *) b;

	if (ab->size > bb->size)
		return (1);

	if (ab->size < bb->size)
		return (-1);

	return (0);
}

#endif
#endif

void mbl_stat (void)
{
	int i;
	int ok;
	int voie;
	char call[20];
	char freq[20];
	char ligne[200];
	int occ;
	long tot;
	int reste;
	int machine;
	long temps = time (NULL);

	cr ();
	sprintf (ligne, "Software FBB Version %s (%s) compiled on %s",
			 version (), os (), date ());
	outln (ligne, strlen (ligne));
	
	sprintf (ligne,
			 "Mem Us:%ld  Mem Ok:%ld  Bid:%d  Lang:%d  Ports:%d  Ch:%d  FBB %s  BIN %s",
			 mem_alloue, tot_mem, maxbbid, maxlang, nbport (), NBVOIES - 2,
			 (fbb_fwd) ? "Ok" : "No",
			 (bin_fwd) ? "Ok" : "No"			 
		);
	outln (ligne, strlen (ligne));

/*	
#ifndef __DPMI16__
	if (high_memory_type () == XMS)
	{
		sprintf (ligne, "Free XMS memory = %u KB", xms_free ());
		outln (ligne, strlen (ligne));
	}
#endif
*/
	cr ();
	occ = 0;
	tot = 0L;

	sprintf (ligne, "Available disks : ");
	out (ligne, strlen (ligne));
	for (i = 0; i < 8; i++)
	{
		if (*PATH[i])
		{
			sprintf (ligne, "%c: ", i + 'A');
			out (ligne, strlen (ligne));
		}
	}
	cr ();
	cr ();

	sprintf (ligne, "%s", typ_exms ());
	outln (ligne, strlen (ligne));
	cr ();
	if (*ligne != 'N')
	{
		for (i = 0; i < NB_EMS; i++)
		{
			if (t_ems[i].flag == 0)
				continue;
			ok = 0;
			if (desc[i].tot_bloc)
			{
				sprintf (ligne, "     %s : %2d page(s) (%3ld KB)",
					  t_ems[i].ctype, desc[i].tot_bloc, desc[i].size >> 10);
				occ += desc[i].tot_bloc;
				tot += desc[i].size;
				ok = 1;
			}
			else
			{
				if ((i != HROUTE) || ((i == HROUTE) && (!EMS_WPG_OK ())))
				{
					sprintf (ligne, "     %s : Unused", t_ems[i].ctype);
					ok = 1;
				}
			}
			if (ok)
				outln (ligne, strlen (ligne));
		}
		sprintf (ligne, "   Total :%3d page(s) (%3ld KB)", occ, tot >> 10);
		outln (ligne, strlen (ligne));
		cr ();
	}

	sprintf (ligne,
			 "Ch Callsign  N1 N2 N3   Cnect Cmput T-Out  Used Status     Buf Freq"
		);
	outln (ligne, strlen (ligne));
	for (voie = 2; voie < NBVOIES; voie++)
	{
		occ = 0;
		reste = 0;
		machine = 0;

		if (svoie[voie]->sta.connect)
		{
			sprintf (call, "%-6s-%2d", svoie[voie]->sta.indicatif.call, svoie[voie]->sta.indicatif.num);
			occ = (int) (temps - svoie[voie]->debut);
			reste = (int) time_att[voie];
			machine = (int) svoie[voie]->tmach;
		}
		else
		{
			continue;
		}
		if ((!svoie[voie]->sta.connect) && (DRSI (no_port (voie))))
			strcpy (freq, "DRSI");
		else if ((!svoie[voie]->sta.connect) && (BPQ (no_port (voie))))
			strcpy (freq, "BPQ");
		else
			strcpy (freq, p_port[no_port (voie)].freq);
		sprintf (ligne,
		  "%02d %s %02d %02d %02d %4d:%02d %2d:%02d %2d:%02d %5d %s %4d %s",
				 voie - 1, call, svoie[voie]->niv1, svoie[voie]->niv2, svoie[voie]->niv3,
				 occ / 60, occ % 60, machine / 60, machine % 60, reste / 60, reste % 60,
				 svoie[voie]->memoc + svoie[voie]->inbuf.nbcar, stat_voie (voie), svoie[voie]->sta.ack, freq
			);
		outln (ligne, strlen (ligne));
	}
	cr ();

}

void wreq_cfg (FILE * fptr)
{
	int i;
	int occ;
	long tot;
	serlist *lptr;
	char buffer[256];

	fprintf (fptr, "ReqCfg %s BBS\r\n", mycall);

	fprintf (fptr, "\r\nReqCfg V 1.2 (C) F6FBB 1992 - BBS %s\r\n\r\n", mycall);

	fprintf (fptr, "\r\nSoftware FBB Version %s (%s) compiled on %s\r\n\r\n",
			 version (), os (), date ());
	fprintf (fptr,
			 "\r\nMem Us:%ld  Mem Ok:%ld  Bid:%d  Ports:%d  Ch:%d  FBB %s  BIN %s\r\n\r\n",

			 mem_alloue, tot_mem, maxbbid, nbport (), NBVOIES - 2,
			 (fbb_fwd) ? "Ok" : "No",
			 (bin_fwd) ? "Ok" : "No"
		);


	fprintf (fptr, "Available volumes : ");
	for (i = 0; i < 8; i++)
	{
		if (*PATH[i])
		{
			fprintf (fptr, "%c: ", i + 'A');
		}
	}
	fprintf (fptr, "\r\n\r\n");

	occ = 0;
	tot = 0L;
	sprintf (buffer, "%s\r\n\r\n", typ_exms ());
	fprintf (fptr, "%s", buffer);

	if (*buffer != 'N')
	{
		for (i = 0; i < NB_EMS; i++)
		{
			if (t_ems[i].flag == 0)
				continue;
			if (desc[i].tot_bloc)
			{
				fprintf (fptr, "     %s : %2d page(s) (%3ld KB)\r\n",
					  t_ems[i].ctype, desc[i].tot_bloc, desc[i].size >> 10);
				occ += desc[i].tot_bloc;
				tot += desc[i].size;
			}
			else if ((i != HROUTE) || ((i == HROUTE) && (!EMS_WPG_OK ())))
			{
				fprintf (fptr, "     %s : Unused\r\n", t_ems[i].ctype);
			}
		}
		fprintf (fptr, "   Total :%3d page(s) (%3ld KB)\r\n", occ, tot >> 10);
	}

	fprintf (fptr, "\r\nLanguages\r\n\r\n");
	for (i = 0; i < maxlang; i++)
	{
		fprintf (fptr, "      %2d : %-10s\n", i + 1, nomlang + i * LG_LANG);
	}

	fprintf (fptr, "\r\nServers\r\n\r\n");
	lptr = tete_serv;
	while (lptr)
	{
		fprintf (fptr, "  %6s : %s\r\n", lptr->nom_serveur, lptr->com_serveur);
		lptr = lptr->suiv;
	}

	fprintf (fptr, "\r\nPort Interface Emulat. Ch Mode  Frequency\r\n");
	for (port = 1; port < NBPORT; port++)
	{

		if (!p_port[port].pvalid)
			continue;

		fprintf (fptr, "%d    %s  %s  %-2d %s %s\r\n",
				 port, intf (port), typort (port), p_port[port].nb_voies,
				 mode_port (port), p_port[port].freq);
	}
}

int req_cfg (char *filename)
{
	FILE *fptr;
	char buffer[256];
	char sender[80];
	char route[80];

	fptr = fopen (filename, "rt");	/* Open the received message */
	if (fptr == NULL)
		return (1);

	fgets (buffer, 80, fptr);	/* Read the command line */
	sscanf (buffer, "%*s %*s %*s %s\n", sender);

	*route = '\0';
	fgets (buffer, 80, fptr);	/* Read the subject */
	strupr (buffer);			/* Capitalize */
	sscanf (buffer, "%*[^@\n]%[^\n]", route);	/* Scan route */

	fclose (fptr);				/* All needed is read */

	fptr = fappend (MAILIN, "b");
	if (fptr == NULL)
		return (1);

	fprintf (fptr, "#\r\n");	/* Tell that this is a message from this BBS */

	fprintf (fptr, "SP %s %s < %s\r\n", sender, route, mycall);

	wreq_cfg (fptr);

	fprintf (fptr, "\r\n/EX\r\n");

	fclose (fptr);

	return (0);
}
