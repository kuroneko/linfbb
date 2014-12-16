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
#include <fbb_drv.h>

typedef int (*sta_)(int, int, int, void *);
typedef int (*snd_)(int, int, int, char *, int, Beacon *);
typedef int (*rcv_)(int *, int *, int *, char *, int *, ui_header *);
typedef int (*opn_)(int, int);
typedef int (*cls_)(int);


typedef struct {
	sta_ sta;
	snd_ snd;
	rcv_ rcv;
	opn_ opn;
	cls_ cls;
} DrvFct;

/* I don't know how to avoid this warning... */
static DrvFct drv_fct[NB_TYP] =
{
	/*  STA_DRV     SND_DRV     RCV_DRV */
	{
		sta_ded, snd_ded, rcv_ded, opn_ded, cls_ded
	}
	,							/* TYP_DED */
	{
		sta_aea, snd_aea, rcv_aea, NULL, NULL
	}
	,							/* TYP_PK */
	{
		sta_mod, snd_mod, NULL, NULL, NULL
	}
	,							/* TYP_MOD */
	{
		sta_kam, snd_kam, rcv_kam, NULL, NULL
	}
	,							/* TYP_KAM */
#if !defined(__WIN32__) && (defined(__WINDOWS__) || defined(__FBBDOS__))
	{
		sta_bpq, snd_bpq, rcv_bpq, NULL, NULL
	}
	,							/* TYP_BPQ */
#else
	{
		NULL, NULL, NULL, NULL, NULL
	}
	,							/* TYP_BPQ */
#endif
#if defined(__WINDOWS__)
	{
		sta_tcp, snd_tcp, NULL, NULL, NULL
	}
	,							/* TYP_TCP */
#elif defined(__LINUX__)
	{
		sta_tcp, snd_tcp, rcv_tcp, opn_tcp, cls_tcp
	}
	,							/* TYP_TCP */
#else
	{
		NULL, NULL, NULL, NULL, NULL
	}
	,							/* TYP_TCP */
#endif
#if defined(__LINUX__)
	{
		sta_sck, snd_sck, rcv_sck, opn_sck, cls_sck
	}
	,							/* TYP_SOCK */
#else
	{
		NULL, NULL, NULL, NULL, NULL
	}
	,							/* TYP_SOCK */
#endif
#if defined(__WINDOWS__)
	{
		sta_agw, snd_agw, rcv_agw, opn_agw, cls_agw
	}
	,							/* TYP_AGW */
#else
	{
		NULL, NULL, NULL, NULL, NULL
	}
	,							/* TYP_AGW */
#endif
#if defined(__WINDOWS__)
	{
		sta_tcp, snd_tcp, rcv_tcp, NULL, NULL
	}
	,							/* TYP_ETH */
#elif defined(__LINUX__)
	{
		sta_tcp, snd_tcp, rcv_tcp, opn_tcp, cls_tcp
	}
	,							/* TYP_ETH */
#else
	{
		NULL, NULL, NULL, NULL, NULL
	}
	,							/* TYP_ETH */
#endif
	{
		sta_hst, snd_hst, rcv_hst, opn_hst, cls_hst
	}
	,							/* TYP_HST */
#if defined(__WINDOWS__) || defined(__FBBDOS__)
	{
		sta_flx, snd_flx, rcv_flx, opn_flx, cls_flx
	}
	,							/* TYP_FLX */
#else
	{
		NULL, NULL, NULL, NULL, NULL
	}
	,							/* TYP_FLX */
#endif
#if defined(__LINUX__)
	{
		sta_pop, snd_pop, rcv_pop, opn_pop, cls_pop
	}
	,							/* TYP_POP */
#else
	{
		NULL, NULL, NULL, NULL, NULL
	}
	,							/* TYP_POP */
#endif
};

void clear_queue (int voie)
{
	if (P_TOR (voie))
	{
		sta_drv (voie, TOR, "%C");
	}
}

void tor_end (int voie)
{
}

void tor_start (int voie)
{
	if (P_TOR (voie))
	{
		sta_drv (voie, TOR, "%I");
	}
}

void tor_stop (int voie)
{
	if (P_TOR (voie))
	{
		sta_drv (voie, TOR, "%O");
	}
}

void tor_disc (int voie)
{
}

/*******************************************
 *
 * Status d'une voie
 *
 * Retourne le status demande dans *ptr
 *
 *******************************************/
int sta_drv (int voie, int cmd, void *ptr)
{
	int canal;
	int val = 0;

	int port = no_port (voie);
	int p;

	if (DEBUG)
		return (0);

	df ("sta_drv", 4);

	if ((voie == CONSOLE) || (voie == INEXPORT))
	{
		ff ();
		return (0);
	}

	p = p_port[port].typort;

	if (drv_fct[p].sta)
	{
		canal = svoie[voie]->affport.canal;
		val = (drv_fct[p].sta) (port, canal, cmd, ptr);
	}

	ff ();
	return (val);
}

/*******************************************
/
/  Lecture d'une voie
/
/  cmd retourne le type de donnees lues
/  buf retourne les donnees lues
/  len retourne longueur des donnees lues
/  ptr retourne l'info monitoring (ou NULL)
/
********************************************/
int rcv_drv (int *port, int *voie, int *cmd, char *buf, int *len, ui_header * ptr)
{
	int val = 0;
	int canal = 0;
	int p;

	if (DEBUG)
		return (0);

	df ("rcv_drv", 12);

	if ((*voie == CONSOLE) || (*voie == INEXPORT))
	{
		ff ();
		return (0);
	}

	p = p_port[*port].typort;

	if (drv_fct[p].rcv)
	{
		val = (drv_fct[p].rcv) (port, &canal, cmd, buf, len, ptr);

		if (val)
		{
			if (val == 1)
			{
				/* Active le flag de processing */
				is_idle = 0;
			}

			val = 1;

			/* Reponse a une commande */
			if ((*cmd == ECHOCMD) || (*cmd == ERRCMD))
			{
#ifdef __WINDOWS__
				SendEchoCmd (buf, *len);
#endif
				*len = 0;
				*cmd = NOCMD;
				ff ();
				return (0);
			}

			/* Recherche la voie correspondant au canal */
			if ((*cmd != UNPROTO) && (*cmd != NBBUF))
			{
				*voie = no_voie (*port, canal);
				if (*voie == -1)
				{
					ff ();
					return (0);
				}

#ifdef __LINUX__
				if ((!HST (*port)) && (!DRSI (*port)) && (!BPQ (*port)) && (!S_LINUX (*port)))
#else
				if ((!HST (*port)) && (!DRSI (*port)) && (!BPQ (*port)) && (!AGW (*port)))
#endif

				{
					svoie[*voie]->affport.port = *port;
				}
			}
		}
	}

	ff ();
	return (val);
}


/*******************************************
/
/  Ecriture d'une voie
/
/  cmd contient le type de donnees lues
/  buf contient les donnees envoyees
/  len contient la longueur des donnees lues
/  ptr contient l'info monitoring (ou NULL)
/
********************************************/
int snd_drv (int voie, int cmd, char *buffer, int len, Beacon * ptr)
{
	int val = 0;
	int canal;
	int port;
	int p;

	/* Recherche du port */
	if (cmd == UNPROTO)
		port = voie;
	else
		port = no_port (voie);

	p = p_port[port].typort;

	if ((DEBUG) || (!p_port[port].pvalid))
	{
		return (1);
	}

	if ((cmd != UNPROTO) && ((voie == CONSOLE) || (voie == INEXPORT)))
		return (0);

	df ("snd_drv", 7);

	if (drv_fct[p].snd)
	{
		if (cmd == UNPROTO)
		{
			{
				val = (drv_fct[p].snd) (port, 0, cmd, buffer, len, ptr);
				ptr = ptr->next;
			}

		}
		else
		{
			canal = svoie[voie]->affport.canal;
			val = (drv_fct[p].snd) (port, canal, cmd, buffer, len, ptr);

			if ((cmd == DATA) && (svoie[voie]->sta.mem))
				--svoie[voie]->sta.mem;
		}
	}

	if (val)
	{
		is_idle = 0;
	}
	ff ();
	return (val);
}

/*******************************************
/
/  Ouverture d'un port
/
********************************************/
int opn_drv (int port, int nb)
{
	int p;
	int val = 0;

	if (DEBUG)
		return (0);

	p = p_port[port].typort;

	if (drv_fct[p].opn)
	{
		val = (drv_fct[p].opn) (port, nb);
	}

	return (val);
}

/*******************************************
/
/  Fermeture d'un port
/
********************************************/
int cls_drv (int port)
{
	int p;
	int val = 0;

	if (DEBUG)
		return (0);

	p = p_port[port].typort;

	if (drv_fct[p].cls)
	{
		val = (drv_fct[p].cls) (port);
	}

	return (val);
}

/*
 * Fonctions diverses
 */

int no_voie (int port, int canal)
{
	int i;

	df ("no_voie", 2);

#ifdef __LINUX__
	if (S_LINUX (port))
	{
		for (i = 1; i < NBVOIES; i++)
		{
			if ((S_LINUX ((int)svoie[i]->affport.port)) && (canal == svoie[i]->affport.canal))
			{
				ff ();
				return (i);
			}
		}
	}
	else
#endif

	if (HST (port))
	{
		for (i = 1; i < NBVOIES; i++)
		{
			if ((port == svoie[i]->affport.port) && (canal == 1) && (P_TOR (i)))
			{
				/* Port pactor */
				ff ();
				return (i);
			}
			if ((HST ((int)svoie[i]->affport.port)) && (canal == svoie[i]->affport.canal))
			{
				ff ();
				return (i);
			}
		}
	}

	else if (BPQ (port))
	{
		for (i = 1; i < NBVOIES; i++)
		{
			if ((BPQ ((int)svoie[i]->affport.port)) && (canal == svoie[i]->affport.canal))
			{
				ff ();
				return (i);
			}
		}
	}

	else if (DRSI (port))
	{
		for (i = 1; i < NBVOIES; i++)
		{
			if ((DRSI ((int)svoie[i]->affport.port)) && (canal == svoie[i]->affport.canal))
			{
				ff ();
				return (i);
			}
		}
	}

	else if (AGW (port))
	{
		for (i = 1; i < NBVOIES; i++)
		{
			if ((AGW ((int)svoie[i]->affport.port)) && (canal == svoie[i]->affport.canal))
			{
				ff ();
				return (i);
			}
		}
	}

	else
	{
		for (i = 1; i < NBVOIES; i++)
		{
			if ((port == svoie[i]->affport.port) && (canal == svoie[i]->affport.canal))
			{
				ff ();
				return (i);
			}
		}
	}
	ff ();
	return (-1);
}


int is_pactor (void)
{
	int port;

	for (port = 1; port < NBPORT; port++)
		if (IS_PACTOR (port))
			return (port);
	return (0);
}


void not_allowed (char *buffer)
{
#ifdef ENGLISH
	sprintf (buffer, "Not an allowed command !");
#else
	sprintf (buffer, "Commande impossible !   ");
#endif
}


void paclen_change (int port, int canal, char *buffer)
{
	int voie = 0;
	int pac;
	int *p_ptr;
	char *ptr = buffer;

	if (canal)
		voie = no_voie (port, canal);

	do
	{
		++ptr;
	}

	while (isspace (*ptr));

	if (canal)
	{
		p_ptr = &(svoie[voie]->paclen);
	}
	else
	{
		p_ptr = &(p_port[port].pk_t);
	}

	if (isdigit (*ptr))
	{
		pac = atoi (ptr);
		*buffer = '\0';
		if ((pac >= 30) && (pac <= 250))
			*p_ptr = pac;
		else
			sprintf (buffer, "INVALID VALUE : %d\r\n", pac);
	}
	else
		sprintf (buffer, "%d\r\n", *p_ptr);
}


char *mot (char *chaine)
{
	static char ch_retour[20];
	char *ptr = ch_retour;

	while (isalnum (*chaine))
		*ptr++ = *chaine++;
	*ptr = '\0';
	return (ch_retour);
}



void env_date (void)
{
	struct tm *sdate;
	int port;
	long temps = time (NULL);
	char buffer[300];

	sdate = localtime (&temps);

	for (port = 1; port < NBPORT; port++)
	{
		if (p_port[port].pvalid)
		{
			deb_io ();
			switch (p_port[port].typort)
			{
			case TYP_DED:
				sprintf (buffer, "K %02d:%02d:%02d",
						 sdate->tm_hour, sdate->tm_min, sdate->tm_sec);
				tnc_commande (port, buffer, PORTCMD);
				sprintf (buffer, "K %02d/%02d/%02d",
				   sdate->tm_mon + 1, sdate->tm_mday, sdate->tm_year % 100);
				tnc_commande (port, buffer, PORTCMD);
				break;
			case TYP_PK:
				sprintf (buffer, "DA%02d%02d%02d%02d%02d",
					sdate->tm_year % 100, sdate->tm_mon + 1, sdate->tm_mday,
						 sdate->tm_hour, sdate->tm_min);
				selcanal (port);
				tnc_commande (port, buffer, PORTCMD);
				break;
			}
			fin_io ();
		}
	}
}

#define CESC 0x1f

void set_binary (int voie, int val)
{
	svoie[voie]->binary = val;
	sta_drv (voie, BINCMD, &val);
}

void set_bs (int voie, int val)
{
	sta_drv (voie, BSCMD, &val);
}

#ifdef __LINUX__

#ifdef OLD_AX25
#include <ax25/axconfig.h>
#include <ax25/nrconfig.h>
#include <ax25/rsconfig.h>
#else
#include <netax25/axconfig.h>
#include <netax25/nrconfig.h>
#include <netax25/rsconfig.h>
#endif

/* Une seule initialisation quelque soit le driver */
int fbb_ax25_config_load_ports (void)
{
	static int init_ok = 0;

	if (init_ok)
		return (1);

	init_ok = 1;
	return (ax25_config_load_ports ());
}

/* Une seule initialisation quelque soit le driver */
int fbb_nr_config_load_ports (void)
{
	static int init_ok = 0;

	if (init_ok)
		return (1);

	init_ok = 1;
	return (nr_config_load_ports ());
}

/* Une seule initialisation quelque soit le driver */
int fbb_rs_config_load_ports (void)
{
	static int init_ok = 0;

	if (init_ok)
		return (1);

	init_ok = 1;
	return (rs_config_load_ports ());
}
#endif
