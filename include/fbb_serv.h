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
 *        SERV.H
 */


#ifndef	_fbb_serv
#define	_fbb_serv

/* Define THIRTYTWOBITDATA if you are bringing in data from 32-bit versions.
 This creates datafiles that match the origin documentation for FBB.  
 They are not compatible with files created with 64-bit versions.  */
/*#define THIRTYTWOBITDATA 1 */
#ifdef THIRTYTWOBITDATA
#define fbb_long int
#else
#define fbb_long long
#endif
/* #define FORTIFY */

#ifdef __LINUX__

#include <sys/vfs.h>
#include <sys/socket.h>
#ifdef OLD_AX25
#include <linux/ax25.h>
#include <linux/netrom.h>
#include <linux/rose.h>
#include <ax25/axutils.h>
#else
#include <netax25/ax25.h>
#include <netrom/netrom.h>
#include <netrose/rose.h>
#include <netax25/axlib.h>
#endif
/* Bug dans libc ????? <================== */
#define __NO_CTYPE

#define FAR

#define __a2__ __attribute__ ((packed, aligned(2)))
#else
#define __a2__
#endif

/* Numero du canal pactor HST */
#define PACTOR_CH 31

/* Minimum place disque (KB) */
#define MIN_DISK 1000

/* #define FBB_DEBUG 1 */
#define NO_EMS_CHN

/* Genere une exception .... */
/* #define dump_core()  {char *coreptr = NULL ; *coreptr = 0x55 ; } */

/* Index des couleurs */
#define W_SNDT	0				/* Envoie data */
#define W_RCVT	1				/* Recoit data */
#define W_CHNI	2				/* Canal Information */
#define W_MONH	3				/* Monitoring header */
#define W_MOND 4				/* Monitoring data */
#define W_CNST 5				/* Console text */
#define W_BACK 6				/* Background */
#define W_STAT 7				/* Fenetre de status */
#define W_DEFL 8				/* Couleur Fenetre haute */
#define W_VOIE 9				/* Couleur voie */
#define W_NCOL	10				/* Nombre de couleurs definies */

#define ENCODE 0
#define DECODE 1

#define NO_REPORT_MODE	0
#define REPORT_MODE		1

/* Programmation du port */
#define	XON	1
#define	CTS	2
#define	DSR	4

/* Constantes specifiques aux I/O */
#define COMMAND 11
#define DATA    12
#define UNPROTO 13
#define DISPLAY 14
#define TOR     15
#define STATS   16
#define NBBUF   17
#define NBCHR   18

/* Protocoles de forwarding */
#define	FWD_MBL	1
#define	FWD_FBB	2
#define	FWD_BIN	4
#define	FWD_BIN1	8
#define	FWD_XPRO	16

/* Constantes de status des TNC */
#define	INVCMD	0xff
#define	NOCMD		0
#define	TNCSTAT	1
#define	PACLEN	2
#define	CMDE		3
#define	SETFLG	4
#define	SNDCMD	5
#define	ECHOCMD	6
#define	PORTCMD	7
#define	ERRCMD	8
#define	BINCMD	9
#define	BSCMD		10
#define	SUSPCMD	11
#define	SETBUSY	12

/* Nb d'unites virtuelles ds FBBDOS */
#define NB_PATH	8

#define NEWIDNT

#define LINT_ARGS

#define uchar char
#define lcall unsigned long

#define ISGRAPH(c)	(!iscntrl(c) && !isspace(c))
#define ISPRINT(c)	(!iscntrl(c))

#define PRIVATE(type) ((type == 'P') || (type == 'A') || (type == 'T'))

#include "version.h"

#if defined(__FBBDOS__) || defined(__WINDOWS__)
#include <process.h>
#include <alloc.h>
#include <io.h>
#include <dos.h>
#include <conio.h>
#include <dir.h>
#include <bios.h>
#include <mem.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define	ERR_OPEN	0
#define	ERR_CREATE	1
#define	ERR_CLOSE	2
#define	ERR_SYNCHRO	3
#define	ERR_MEMORY	4
#define	ERR_CANAL	5
#define	ERR_NIVEAU	6
#define	ERR_WRITE	7
#define	ERR_SYNTAX	8
#define	ERR_TNC		9
#define	ERR_PTR		10
#define	ERR_EXMS	11
#define	ERR_DIV0	12
#define	ERR_EXCEPTION	13

#ifndef FBB_IO

#define	USR_CALL	0
#define BBS_CALL	1

#define read		fbb_read
#define write		fbb_write
#define open		fbb_open
#define close		fbb_close
#define unlink		fbb_unlink

#define fread		fbb_fread
#define	fwrite		fbb_fwrite
#define	fopen		fbb_fopen
#define	fclose		fbb_fclose
#define fgetc		fbb_fgetc
#define fputc		fbb_fputc
#define fgets		fbb_fgets
#define fputs		fbb_fputs
#define mkdir           fbb_mkdir
#define rmdir           fbb_rmdir

#define statfs(file, buf) fbb_statfs(file, buf)
#define stat(file, buf)   fbb_stat(file, buf)
#define access(file, mode) fbb_access(file, mode)

#define findfirst	fbb_findfirst
#define findnext	fbb_findnext

#define filelength	fbb_filelength

#define textattr	fbb_textattr

#define clrscr		fbb_clrscr
#endif /* FBB_IO */

#define RCV_BUFFER_SIZE 1600

#define	NO_MS		0
#define	EMS			1
#define	XMS			2

#define ECART        1000L		/* Chiffre minimum pour extension Nø message */
#define M_LIG        4			/* Separation fenetre haut / bas */
#define MAXMEM       5000		/* Buffer plein  */
#define BIDLEN       12			/* Taille du BID */
#define BIDCOMP      9			/* Taille du BID compresse */
#define MAX_FB       5			/* Nb de lignes ds fwd FBB */
#define	MAX_BCL      3			/* Nb de lignes ds broadcast */

#define MAX_ERR      20			/* Nombre max d'erreurs autorisees */

#define I_COM1       0x04		/* COM1 = IRQ4 */
#define I_COM2       0x03		/* COM2 = IRQ3 */

#define F_EXC        0x0001
#define F_LOC        0x0002
#define F_EXP        0x0004
#define F_SYS        0x0008
#define F_BBS        0x0010
#define F_PAG        0x0020
#define F_GST        0x0040
#define F_MOD        0x0080
#define F_PRV        0x0100
#define F_UNP        0x0200
#define F_NEW        0x0400
#define F_PMS        0x0800
/* #define F_PWD        0x1000 */

#define F_FOR        0x00100
#define F_FBB        0x00200
#define F_HIE        0x00400
#define F_BID        0x00800
#define F_NFW        0x01000
#define F_MID        0x02000
#define F_BIN        0x04000
#define F_ACQ        0x08000

#define	W_DISK       0x0001
#define	W_FILE       0x0002
#define	W_SERVER     0x0004
#define	W_PINGPONG   0x0008
#define	W_NO_ROUTE   0x0010
#define	W_NO_NTS     0x0020
#define	W_MESSAGE    0x0040
#define	W_ERROR      0x0080
#define	W_REJECT     0x0100
#define	W_HOLD       0x0200

#define TYP_DED      0
#define TYP_PK       1
#define TYP_MOD      2
#define TYP_KAM      3
#define TYP_BPQ      4
#define TYP_TCP      5
#define TYP_SCK      6
#define TYP_AGW      7
#define TYP_ETH		8
#define TYP_HST		9
#define TYP_FLX		10
#define TYP_POP		11
#define NB_TYP       12

#define W_PPG        0x01		/* Ping-Pong */
#define W_ROU        0x02		/* Unknown route */
#define W_NTS        0x04		/* Unknown nts */
#define W_ASC        0x08		/* Ascii file not found */
#define W_BIN        0x10		/* Binary file not found */

#define FWD_PRIV     0x01		/* Forward only privates */
#define FWD_SMALL    0x02		/* Forward smallest first */
#define FWD_DUPES    0x04		/* Forward allowing dupes */

#define EXC(flag)    (flag & F_EXC)
#define LOC(flag)    (flag & F_LOC)
#define EXP(flag)    (flag & F_EXP)
#define SYS(flag)    (flag & F_SYS)
#define BBS(flag)    (flag & F_BBS)
#define PAG(flag)    (flag & F_PAG)
#define GST(flag)    (flag & F_GST)
#define MOD(flag)    (flag & F_MOD)
#define PRV(flag)    (flag & F_PRV)
#define UNP(flag)    (flag & F_UNP)
#define NEW(flag)    (flag & F_NEW)
#define PMS(flag)    (flag & F_PMS)

#define FBB(flag)    (flag & F_FBB)
#define FOR(flag)    (flag & F_FOR)
#define HIE(flag)    (flag & F_HIE)
#define BID(flag)    (flag & F_BID)
#define NFW(flag)    (flag & F_NFW)
#define MID(flag)    (flag & F_MID)
#define BIN(flag)    (flag & F_BIN)
#define ACQ(flag)    (flag & F_ACQ)
#define PWD(flag)    (flag & F_PWD)

#define P_GUEST(voie)  ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x01) || (svoie[voie]->localmode & 0x01)) : 0)
#define P_BBS(voie)    ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x02) || (svoie[voie]->localmode & 0x02)) : 0)
#define P_YAPP(voie)   ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x04) || (svoie[voie]->localmode & 0x04)) : 0)
#define P_MODM(voie)   ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x08) || (svoie[voie]->localmode & 0x08)) : 0)
#define P_GATE(voie)   ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x10) || (svoie[voie]->localmode & 0x10)) : 0)
#define P_LIST(voie)   ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x20) || (svoie[voie]->localmode & 0x20)) : 0)
#define P_READ(voie)   ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x40) || (svoie[voie]->localmode & 0x40)) : 0)
#define P_TOR(voie)    ((voie > 0) ? ((p_port[no_port(voie)].moport & 0x80) || (svoie[voie]->localmode & 0x80)) : 0)

#define P_DIRECT	0
#define P_MODEM	3
#define P_COMBIOS	1
#define P_BPQ		2
#define P_MODEM	3
#define P_DRSI		4
#define P_TFPC		5
#define P_WINDOWS	6
#define P_ETHER	7
#define P_TFWIN	8
#define P_LINUX	9

#define BIOS(port)     (p_com[(int)p_port[port].ccom].combios)
#define COMBIOS(port)  (p_com[(int)p_port[port].ccom].combios == P_COMBIOS)
#define BPQ(port)      (p_com[(int)p_port[port].ccom].combios == P_BPQ)
#define DRSI(port)     ((p_com[(int)p_port[port].ccom].combios == P_DRSI) || (p_com[(int)p_port[port].ccom].combios == P_TFWIN))
#define TFPC(port)     (p_com[(int)p_port[port].ccom].combios == P_TFPC)
#define WINDOWS(port)  ((p_com[(int)p_port[port].ccom].combios == P_WINDOWS)
#define ETHER(port)    (p_com[(int)p_port[port].ccom].combios == P_ETHER)
#define LINUX(port)    (p_com[(int)p_port[port].ccom].combios == P_LINUX)
#define AGW(port)      ((p_com[(int)p_port[port].ccom].combios == P_WINDOWS) && (p_port[port].typort == TYP_AGW))
#define S_LINUX(port)  ((LINUX(port)) && (p_port[port].typort == TYP_SCK))
#define HST(port)      (p_port[port].typort == TYP_HST)
#define FLX(port)      (p_port[port].typort == TYP_FLX)
#define POP(port)      (p_port[port].typort == TYP_POP)

/* Pactor */
#define IS_PACTOR(p)	((p_port[p].typort == TYP_HST) && (p_port[p].ccanal == 0))
#define ISS(p)			((p_com[(int)p_port[p].ccom].pactor_st & 0x08) ? 1 : 0)
#define BUSY(p)			((p_com[(int)p_port[p].ccom].pactor_st == 247) ? 1 : 0)
#define ONLINE(p)		(((p_com[(int)p_port[p].ccom].pactor_st & 0x07) != 0x07) ? 1 : 0)

#define PACTOR_ONLINE	1
#define PACTOR_SCAN		2
#define PACTOR_ISS		4

#define EMS_BLOC	16384

/* File number de l'EMS */
#define	DIRMES		0
#define	WBID		1
#define	HROUTE		2
#define FORWARD		3
#define	REJET		4
#define WPG		5
#define	SCREEN		6
#define NB_EMS		7			/* Nombre de flags EMS */

#define	EMS_NUL	0x0000
#define	EMS_MSG	0x0001
#define	EMS_BID	0x0002
#define	EMS_HRT	0x0004
#define	EMS_FWD	0x0008
#define	EMS_REJ	0x0010
#define	EMS_WPG	0x0040
#define	EMS_SCR	0x0020

#define EMS_MSG_OK()    (in_exms & EMS_MSG)
#define EMS_BID_OK()    (in_exms & EMS_BID)
#define EMS_HRT_OK()    (in_exms & EMS_HRT)
#define EMS_FWD_OK()    (in_exms & EMS_FWD)
#define EMS_REJ_OK()    (in_exms & EMS_REJ)
#define EMS_SCR_OK()    (in_exms & EMS_SCR)
#define EMS_WPG_OK()    (in_exms & EMS_WPG)
#define EMS_OVR_OK()    (in_exms & EMS_OVR)
#define EMS_CHN_OK()    (in_exms & EMS_CHN)


/*
   Structure d'int en mode reel a travers DPMI

   Fonction int86real()
 */

typedef struct
{
	unsigned long DI;
	unsigned long SI;
	unsigned long BP;
	unsigned long rs;
	unsigned long BX;
	unsigned long DX;
	unsigned long CX;
	unsigned long AX;
	unsigned int FL;
	unsigned int ES;
	unsigned int DS;
	unsigned int FS;
	unsigned int GS;
	unsigned int IP;
	unsigned int CS;
	unsigned int SP;
	unsigned int SS;
}
RSEGS;

typedef struct
{
	char ctype[4];
	unsigned flag;
}
tp_ems;

typedef struct					/* Informations pour appel XMS */
{
	unsigned AX,				/* Seuls les registres AX, BX, DX et */
	  BX,						/* SI sont requis selon la fonction */
	  DX,						/* appel‚e, il faut donc une autre */
	  SI,						/* adresse de segment */
	  DS;
}
XMSRegs;

#define TRUE    1
#define FALSE   0

#define DEGRAD  57.2957795

#define LG_LANG 10				/* Taille du nom fichier langue */
#define MAXLIGNES 20

#define MAXTRAIT     180		/* Temps maximal de traitement (# 3 mn) */
#define MAXTASK      3			/* Temps maximum dans une tache longue  */

#define N_MENU       0
#define N_MESS       1
#define N_QRA        2
#define N_STAT       3
#define N_INFO       4
#define N_NOMC       5
#define N_TRAJ       6
#define N_ECH        7
#define N_RBIN       8
#define N_DOS        9
#define N_GATE       10
#define N_MOD        11
#define N_BIN        12
#define N_XFWD       13
#define N_MBL        14
#define N_FORW       15
#define N_TELL       16
#define N_YAPP       17
#define N_CONF       18
#define N_MINI_EDIT  19
#define N_THEMES     20
#define NBRUB        21			/* Nb de niv1 maximum  */

#define OUI          3			/* Offset du 'OUI' */
#define NON          4			/* Offset du 'NON' */
#define JOUR         5			/* Offset du jour */
#define MOIS         6			/* Offset du mois */
#define T_DEB        1
#define T_MES        (T_DEB+10)	/* Offset des textes dans le fichier */
#define T_QST        (T_MES+14)	/* Questions et messages standards */
#define T_ERR        (T_QST+7)	/* Messages d'erreur */
#define T_MBL        (T_ERR+32)	/* Rubrique emulation MBL */
#define T_TRT        (T_MBL+60)	/* Traitement des donnees */
#define T_MEN        (T_TRT+14)	/* Rubrique menu serveur */
#define T_STA        (T_MEN+3)	/* Rubrique statistiques */
#define T_NOM        (T_STA+24)	/* Rubrique Nomenclature */
#define T_TRJ        (T_NOM+15)	/* Rubrique Trajectographie */
#define T_QRA        (T_TRJ+41)	/* Rubrique Qra-Locator */
#define T_DOS        (T_QRA+19)	/* Rubrique FBBDOS */
#define T_INF        (T_DOS+12)	/* Rubrique Informations */
#define T_GAT        (T_INF+4)	/* Gateway */
#define T_YAP        (T_GAT+13)	/* Yapp transfert */
#define T_CNF        (T_YAP+5)	/* Conference */

#define T_THE        (T_CNF+11)	/* Themes */
#define NBTEXT       (T_THE+2)	/* Nombre de lignes de texte */

#define TNCVOIES     9
#define NBLIG        10
#define CONSOLE      0
/* #define MAXVOIES     50			/ Nb de voies maximum */
#define MAXVOIES     96			/* Nb de voies maximum */
#define INEXPORT     1			/* Voie reservee a l'import/export */
#define MMONITOR     (MAXVOIES)	/* Affiche le monitoring */
#define ALLCHAN		(MAXVOIES+1)	/* Affiche tous les canaux */
#define TOTVOIES     (MAXVOIES+3)	/* Nb de voies total */


#define NBCOM        20			/* Nb de COMs maximum en interne */
#define NBPORT       (16+1)		/* Nb de ports maximum (+ console) */
#define SECONDE      19			/* Nb de ticks par seconde */
#define MAXSTAT      5000L		/* echantillonnage pour les stats */
#define max_ack      4

#define PASS     0x1E
#define STREAMSW 0x1D
#define SENDPAC  0x1F
#define TEXTE    0xFF
#define CTRLZ    0x1A

/* Droits sysop */

#define CONSMES      1			/* Consultation de tous les messages     */
#define SUPMES       2			/* Suppression de tous les messages      */
#define CMDSYS       4			/* Acces a la commande SYS               */
#define COSYSOP      8			/* Fonctions cosysop : edit, forward     */
#define MODLABEL     16			/* Edition des labels DOS, YAP, DOC      */
#define SUPFIC       32			/* Suppression de fichiers FBBDOS, YAPP  */
#define ACCGATE      64			/* Acces a tous les gateways, sauf modem */
#define EXEDOS       128		/* Execute des programmes DOS            */
#define ACCESDOS     256		/* Acces a tout le repertoire du disque  */
#define CMDRESET     512		/* Acces aux commandes '/R' et '/A'      */

#define DOS    (YELLOW    + (BLACK << 4))

typedef struct FbbTimerStruct
{
	void *userdata;
	void FAR (*fct) (int, void *);
	int port;
	time_t temps;
	struct FbbTimerStruct *next;
}
FbbTimer;

/* #define AIDE    1 */
#define ABREG   0
#define NBMEN   10				/* nb. de rubriques par menu infos */

typedef struct THroute
{
	char route[42];
	struct THroute *suiv;
}
Hroute;

typedef struct TWp
{
/*
   char     callsign[7];
   char     home[7];
 */
	lcall callsign;
	lcall home;
}
Wp;

typedef struct
{
	int port;
	char from[12];
	char to[12];
	char via[100];
	char ctl[12];
	char txt[12];
	int pid;
	int ui;
}
ui_header;

#define MAX_BROUTE 2000

typedef struct TBloc_route
{
	char b_route[MAX_BROUTE];
	struct TBloc_route *suiv;
}
Broute;

typedef struct
{
	int numlang;
	char *plang[NBTEXT];
}
tlang;

typedef struct tport
{
	char port;
	char canal;
}
sport;

/* Structure de EMS/XMS */

typedef struct
{
	long size;					/* Taille du fichier memoire */
	long pos;					/* Position du pointeur dans le fichier */
	unsigned nb_records;		/* Nombre de records */
	int nb;						/* Nb de caracteres restants dans la page */
	unsigned max_bloc;			/* Nb de blocs maximum */
	unsigned tot_bloc;			/* Nb de blocs alloues */
	unsigned num_bloc;			/* Numero du bloc courant */
	char *ptr;					/* Pointeur de caractere */
	char **alloc;				/* Blocs alloues */
}
Desc;

/* Structure de REJET/HOLD */

typedef struct
{
	char mode;
	char type;
	char exped[7];
	char via[7];
	char desti[7];
	char bid[13];
	int size;
}
Rej_rec;

/* Structures fichiers WP */

typedef struct
{								/* 108 bytes */
	long last;
	short local;
	char source;
	char callsign[7];
	char homebbs[41];
	char zip[9];
	char name[13];
	char qth[31];
}
Wpr;

typedef struct
{								/* 194 bytes */
	char callsign[7];
	char name[13];
	uchar free;
	uchar changed;
	ushort seen;
	fbb_long last_modif __a2__;
	fbb_long last_seen __a2__;
	char first_homebbs[41];
	char secnd_homebbs[41];
	char first_zip[9];
	char secnd_zip[9];
	char first_qth[31];
	char secnd_qth[31];
}
Wps;

/*
 * Structures et blocs de messages
 */
typedef struct
{
	long nmess;
	unsigned noenr;
	unsigned no_indic;
}
mess_noeud;

#define T_BLOC_MESS	100

typedef struct st_bloc_mess
{
	mess_noeud st_mess[T_BLOC_MESS];
	struct st_bloc_mess *suiv;
}
bloc_mess;

typedef struct typ_serlist
{
	int num_serv;
	char nom_serveur[7];
	char com_serveur[80];
	char nom_pg[80];
	struct typ_serlist *suiv;
}
serlist;

typedef struct typ_pglist
{
	char nom_pg[10];
	struct typ_pglist *suiv;
}
pglist;

/*
 * Structures et blocs d'indicatifs
 */

typedef struct
{
	char indic[7];
	/* char       lettre; */
	uchar val;
	short nbmess;
	short nbnew;
	unsigned coord;
}
ind_noeud;

#define T_BLOC_INFO	50

typedef struct st_bloc_indic
{
	ind_noeud st_ind[T_BLOC_INFO];
	struct st_bloc_indic *suiv;
}
bloc_indic;

typedef struct typstat
{
	char indcnx[6];
	uchar port;
	uchar voie;
	fbb_long datcnx __a2__;
	short tpscnx;
}
statis;

typedef struct typMsysop
{
	char call[22];
	struct typMsysop *next;
}
Msysop;

typedef struct typindic
{
	char call[7];
	char num;
}
indicat;

typedef struct typenrg
{
	char indic[7];
	char exped[7];
	long date;
	long suite;
	char texte[81];
}
enrg;

typedef struct
{

	indicat indic;				/* 8  Callsign */
	indicat relai[8];			/* 64 Digis path */
	fbb_long lastmes __a2__;		/* 4  Last L number */
	fbb_long nbcon __a2__;			/* 4  Number of connexions */
	fbb_long hcon __a2__;			/* 4  Last connexion date */
	fbb_long lastyap __a2__;		/* 4  Last YN date */
	ushort flags;				/* 2  Flags */
	ushort on_base;				/* 2  ON Base number */

	uchar nbl;					/* 1  Lines paging */
	uchar lang;					/* 1  Language */

	fbb_long newbanner __a2__;		/* 4  Last Banner date */
	ushort download;			/* 2  download size (KB) = 100 */
	char free[20];				/* 20 Reserved */
	char theme;					/* 1  Current topic */

	char nom[18];				/* 18 1st Name */
	char prenom[13];			/* 13 Christian name */
	char adres[61];				/* 61 Address */
	char ville[31];				/* 31 City */
	char teld[13];				/* 13 home phone */
	char telp[13];				/* 13 modem phone */
	char home[41];				/* 41 home BBS */
	char qra[7];				/* 7  Qth Locator */
	char priv[13];				/* 13 PRIV directory */
	char filtre[7];				/* 7  LC choice filter */
	char pass[13];				/* 13 Password */
	char zip[9];				/* 9  Zipcode */

}
info;							/* Total : 360 bytes */

typedef struct
{
	char mode;
	char fbid[13];
	fbb_long numero __a2__;
}
bidfwd;

typedef struct CmdList
{
	char cmd[10];
	char *action;
	struct CmdList *next;
}
cmdlist;

typedef struct
{
	indicat indic;
	long first __a2__;
	long last __a2__;
	ushort nb;
}
Heard;

typedef struct typlist
{
	bloc_mess *ptemp;
	unsigned offset;
	int l;
	long last;
	long debut;
	long fin;
	long avant;
	long apres;
	char type;
	char status;
	char route;
	char exp[7];
	char dest[7];
	char bbs[7];
	char find[20];
}
tlist;

#define NBBBS 80
#define NBMASK NBBBS/8

typedef struct
{								/* Longueur = 194 octets */
	char type;
	char status;
	fbb_long numero __a2__;
	fbb_long taille __a2__;
	fbb_long date __a2__;
	char bbsf[7];
	char bbsv[41];
	char exped[7];
	char desti[7];
	char bid[13];
	char titre[61];
	char bin;
	char free[5];
	fbb_long grpnum __a2__;
	ushort nblu;
	fbb_long theme __a2__;
	fbb_long datesd __a2__;
	fbb_long datech __a2__;
	char fbbs[NBMASK];
	char forw[NBMASK];
}
bullist;

typedef struct
{
	int nbpriv, nbbul, nbkb;
}
atfwd;

typedef struct
{
	char type;
	char bin;
	uchar kb;
	char free;
	long nomess;
	long date;
	char fbbs[NBMASK];
	char bbsv[6];
}
recfwd;

#define NBFWD 100
typedef struct typ_lfwd
{
	recfwd fwd[NBFWD];
	struct typ_lfwd *suite;
}
lfwd;

typedef struct typrd_list
{
	long nmess;
	int verb;
	bullist *pmess;
	struct typrd_list *suite;
}
rd_list;

typedef struct typ_satel
{
	char dd[18];
	short y3;
	double d3 __a2__;
	short n3, h3, m3, s3;
	double i0 __a2__;
	double o0 __a2__;
	double e0 __a2__;
	double w0 __a2__;
	double m0 __a2__;
	double a0 __a2__;
	double n0 __a2__;
	double q3 __a2__;
	fbb_long k0 __a2__;
	double f1 __a2__;
	double v1 __a2__;
	short pas;
	fbb_long maj __a2__;
	fbb_long cat __a2__;			/* Catalog Number - anciennement vide  */
	short libre[4];
}
satel;

typedef struct typ_date_t
{
	double jour;
	int mois, annee;
	int heure, mn, sec;
}
date_t;

#define DIM_IBUF 90
typedef struct typ_buflig
{
	struct typ_buflig *suite;	/* pointeur du buffer suivant */
	int lgbuf;					/* longueur du buffer */
	char *buffer;				/* buffer de caracteres */
}
lbuf;

typedef struct
{
	int nblig;					/* nb de lignes en buffer */
	int nbcar;					/* nb de caracteres en buffer */
	int nocar;					/* nb de caracteres deja lus dans la ligne */
	char *ptr;					/* pointeur du dernier caractere */
	lbuf *tete;					/* pointeur du premier buffer */
	lbuf *curr;					/* pointeur du buffer courrant */
}
ibuf;

#define TAILBUF 300
#define DIMBUF  300
#define DATABUF 300

typedef struct typ_cbuf
{
	int ptr_r;					/* Pointeur caracteres recus */
	int ptr_l;					/* Pointeur caracteres lus */
	int ptr_a;					/* Pointeur des caracteres affiches */
	int nblig;					/* nb de lignes dans le buffer */
	int nbcar;					/* nb de caracteres dans buffer */
	int buf_vide;				/* Buffer de reception vide */
	int flush;					/* Demande d'envoi du buffer */
	char buf[DIMBUF];			/* Buffer de reception */
}
cbuf;

#define NB_MARQUES 10
typedef struct typ_obuf
{
	int nb_car;
	int no_car;
	int marque[NB_MARQUES];
	struct typ_obuf *suiv;
	char buffer[TAILBUF];
}
obuf;

typedef struct typ_iliste
{
	char indic[8];
	struct typ_iliste *suiv;
}
iliste;

typedef struct
{
	char ind[7];
	unsigned pos;
}
tri;

typedef struct typ_Forward
{
	char reverse;				/* Demande de reverse */
	unsigned fwdpos;			/* Index dans le fichier de forward */
	unsigned lastpos;			/* Index precedent dans le fichier de forward */
	int fwdlig;					/* Numero de ligne dans le fichier de forward */
	int cptif;					/* Nombre des imbrications de IF */
	int forward;				/* Indicateur de forward en cours et voie de forward */
	int no_con;					/* Numero de la connexion forward */
	int no_bbs;					/* No de BBS en cours de forward */
	int fin_fwd;				/* Fin de forward sur le canal */
	char con_lig[8][80];		/* Liste des commandes de connexion */
	char mesnode[4][3][20];		/* Identificateurs de connexion */
	char txt_con[40];			/* Texte envoye a la connexion */
	char fwdbbs[8];				/* Nom de la BBS a forwarder */
	struct typ_Forward *suite;	/* Voie forward suivante */
}
Forward;

typedef struct beacon
{
	indicat desti;
	indicat digi[8];
	int nb_digi;
	struct beacon *next;
}
Beacon;

typedef struct list_freq
{
	int val;
	double freq;
	char cmde[41];
	struct list_freq *next;
}
ListFreq;

#define NBHEARD 20

typedef struct port_data
{
	char canal;
	char compteur;
	char buf[257];
	int len;
	int cmd;
	struct port_data *next;
}
PortData;

typedef struct
{
	int pk_t;					/* Taille du paquet */
	int min_fwd;				/* Minute de forward du port */
	int per_fwd;				/* Periode de forward du port */
	int maxbloc;				/* Taille maximum du bloc forward */
	int mem;					/* Taille de memoire dispo dans le TNC */
	int beacon_paclen;			/* Paclen de la balise du port */
	int synchro;				/* Mode resync */
	int fd;						/* Descripteur du port */
	int type;					/* Type de connexion AX25/NR/ROSE */
	int wait[MAXVOIES];			/* Packets a lire */
	long cur;					/* Nb d'octets envoyes sur le port */
	int nbc;					/* Vitesse du port */
	char stop;					/* Delai d'interruption du port */
	char polling;				/* Polling en cours */
	char idem;					/* Ne change pas de canal */
	char frame;					/* Nombre de frames */
	char ccom;					/* Numero du COM (0 a 7) */
	char ccanal;				/* Numero du canal dans le COM (1 a 4) */
	char cur_can;				/* Numero du canal en cours de polling */
	char last_cmde;				/* Derniere commande envoyee */
	char pr_voie;				/* No de la 1ere voie du port */
	char nb_voies;				/* Nombre de voies affectees dans le port */
	char tt_can;				/* Dernier canal TNC du port (DRSI/BPQ) */
	char pvalid;				/* validation du port */
	char typort;				/* Type de TNC sur le port 0=DED 1=PK232 */
	char moport;				/* mode d'acces du port */
	char transmit;				/* Port en emission (TOR) */
	char echo;					/* Echo sur le port (Modem) */
	char portbuf[300];			/* Buffer des caracteres en cours de reception */
	int portind;				/* Index dans le buffer */
	FbbTimer *t_delay;			/* Id du timer delay */
	FbbTimer *t_wait;			/* Id du timer wait (Pactor) */
	FbbTimer *t_busy;			/* If du timer busy (Pactor) */
	FbbTimer *t_iss;			/* If du timer iss (Pactor) */
	PortData *cmd;				/* Commandes/Datas a envoyer au TNC */
	PortData *last;				/* Derniere Commandes/Datas envoyee au TNC */
	Forward *listfwd;			/* liste des voies forward du port */
	char freq[10];				/* frequence du port */
	char name[20];				/* Name of the logical port LINUX */
	char fwd[NBMASK];			/* Liste des BBS forwardees sur le port */
	Heard heard[NBHEARD];		/* Jheard */
	ListFreq *lfreq;			/* Tete de la liste des frequences */
}
defport;

typedef struct
{
	int cbase;					/* Adresse de base du COM */
	int combios;				/* Type d'interface logiciel */
	int port;					/* Numero du port telnet */
	long baud;					/* Vitesse du port */
#ifdef __WIN32__
	HANDLE comfd;				/* Ident du com port (Windows) */
#else
	int comfd;					/* Ident du com port (Windows) */
#endif
	int irq;					/* IRQ du port (Windows) */
	int delai;					/* Delai avant resync */
	int ovr;					/* Nb d'overruns */
	char mult_sel;				/* Selection courante du mux */
	char options;				/* Options : Deuxieme set de vitesses */
	char multi[8];				/* Gestion du multiplexeur */
	char compteur;				/* compteur pour PTC */
	char name[20];				/* Name of the device LINUX */
	char pactor_st;				/* Status of the pactor channel */
	/*  int rxptr_8      ; * Pointeur caracteres recus */
	/*  int rxptr_h      ; * Pointeur caracteres lus */
	/*  int txptr_8      ; * Pointeur caracteres envoyes */
	/*  int txptr_h      ; * Pointeur caracteres ecris */
	/*  char rxbuf_vide  ; * Buffer de reception vide */
	/*  char txbuf_vide  ; * Buffer d'emission vide */
	/*  char rxbuf[DIMBUF] ; * Buffer de reception */
	/*  char txbuf[DIMBUF] ; * Buffer d'emission */
}
defcom;

#ifdef __FBBDOS__
typedef struct
{
	char *ptr;
	struct text_info sav_mod;
	unsigned int taille;
	unsigned char cg;
	unsigned char lh;
	unsigned char cd;
	unsigned char lb;
}
fen;

#endif

typedef struct param_fwd
{
	char chaine[80];
	int type;
	struct param_fwd *suiv;
}
typ_pfwd;

typedef struct ymodem_list
{
	struct ymodem_list *next;
	char filename[256];
	long size_trans;
	long time_trans;
	int ok;
}
Ylist;

typedef struct
{
	uchar voie;
	uchar attr;
	char buf[160];
}
Ligne;

typedef struct
{
	int first;					/* 1ere ligne du buffer allouee a l'ecran */
	int totlig;					/* nb de lignes du buffer */
	int curlig;					/* Ligne de l'affichage */
	int deblig;					/* Ligne debut du buffer circulaire */
	int carpos;					/* Pointeur du caractere courant dans la ligne */
	int scrlig;					/* Pointeur de la ligne scrollee */
	int voie;					/* Voie du buffer de ligne */
	int color;					/* Couleur de la ligne courrante */
	char cur_buf[160];			/* Buffer de la ligne en cours */
	Ligne *ligne;
}
FScreen;

typedef struct typ_edit_ch
{
	int record;
	struct typ_edit_ch *suite;
}
edit_ch;

typedef struct
{
	int ligne;
	int carac;
	int max;
	int numero;
	int new_t;
	edit_ch *liste;
}
typedit;

#define LABEL_FIC 80
#define LABEL_NOM 40
#define LABEL_OWN 8

typedef struct typYl
{
	int record;
	long date_creation;
	char nomfic[LABEL_FIC];
	struct typYl *suiv;
}
Ylabel;

/*
   typedef struct {
   char nomfic[LABEL_FIC] ;
   char label[LABEL_NOM] ;
   long date_creation;
   long free;
   } Rlabel ;
 */

typedef struct
{
	char nomfic[LABEL_FIC];
	char label[LABEL_NOM];
	char owner[LABEL_OWN];
	fbb_long index;
	fbb_long date_creation;
	char free[24];
}
Rlabel;							/* 160 bytes */

typedef struct dde_huf_struct
{
	int voie;
	int mode;
	bullist *bull;
	char header[160];
	struct dde_huf_struct *next;
}
desc_huf;

#define MAX_X 25
typedef struct
{
	int nb_bid;
	int ls_bid;
	int r_bid;
	int ok_chck;
	unsigned chck;
	long numero[MAX_X];
	char bid[MAX_X][13];
	char ok_bid[MAX_X];
}
XInfo;

#define NBROUTE 10
typedef struct rt
{
	char call[NBROUTE][7];
	struct rt *suite;
}
Route;

#ifdef __WINDOWS__
typedef struct
{
	HWND hWnd;
	HINSTANCE hInst;			/* hInstance of application */
	int valid;
	/* int      xChar, yChar, yCharnl;        character size */
	int xClient, yClient;		/* client window size */
	int nVscrollMax, nHscrollMax;	/* scroll ranges */
	int nVscrollPos, nHscrollPos;	/* current scroll positions */
	int numlines, numcolumns;	/* number of lines/column in buffer */
	int nVscrollInc, nHscrollInc;	/* scroll increments */
	int nPageMaxLines;			/* max lines on screen */
	int xPos, yPos;
	int val;
	HANDLE hbuff;
	LPSTR lpbuff;
	/* HFONT     hnewsfont;                   * handle of new fixed font */
}
WINF;

typedef struct scrollkeys
{
	WORD wVirtkey;
	int iMessage;
	WORD wRequest;
}
SCROLLKEYS;

#endif

typedef struct
{
	int connect;				/* voie connectee */
	int ack;					/* Ack de la voie */
	int ret;					/* Retry de la voie */
	int stat;					/* Etat du TNC */
	int mem;					/* Taille buffer dispo */
	indicat callsign;			/* Indicatif de la voie */
	indicat indicatif;			/* Indicatif de l'OM connecte */
	indicat relais[8];			/* Indicatif des relais */
}
stat_ch;

#define NB_P_ROUTE 8
#define NB_DEL     50

typedef struct
{
	sport affport;				/* affectation des voies */
	stat_ch sta;				/* Etat de la voie */
	char ch_status;				/* Demande d'affichage du status */
	int sid;					/* Un SID a ete recu */
	int aut_linked;				/* Commande "Linked to" autorisee */
	int nb_prompt;				/* Nombre de prompts a   attendre */
	int private_dir;			/* Acces au repertoire prive */
	int aut_nc;					/* Autorisation du (N)ext et (C)ontinue */
	int rev_mode;				/* Reverse Forward en mode MBL */
	int data_mode;				/* Forward des data */
	int maj_ok;					/* Autorise la mise a jour carnet de trafic */
	int fbb;					/* Autorise l'utilisation du protocole FBB */
	int paclen;					/* Longueur du paquet */
	int mess_recu;				/* Message entierement recu */
	int stop;					/* Arret de pagination */
	int memoc;					/* Memoire occupee par les buffers */
	int conf;					/* Indication de conference */
	int xferok;					/* Resultat du transfert */
	int binary;					/* Mode de fonctionnement des entrees */
	int nb_err;					/* nbre d'erreurs de la voie */
	int dde_int;				/* Demande d'interruption */
	int lignes;					/* Nbre de lignes dans la page en cours */
	int niv1;					/* Niveau 1 de procedures */
	int niv2;					/* Niveau 2 de procedures */
	int niv3;					/* Niveau 3 de procedures */
	int sniv1;					/* Sauve niveau 1 de procedures */
	int sniv2;					/* Sauve niveau 2 de procedures */
	int sniv3;					/* Sauve niveau 3 de procedures */
	unsigned long mode;			/* Aide : TRUE */
	unsigned droits;			/* Droits d'acces du sysop */
	unsigned short checksum;	/* Checksum transfert */
	unsigned prot_fwd;			/* Protocoles de forward supportes */
	int timout;					/* Temps de time-out de la voie */
	int deconnect;				/* Demande de deconnexion */
	int cross_connect;			/* Indicateur de cross_connexion */
	int send_count;				/* Time out yapp */
	int type_yapp;				/* Type de YAPP utilise (1 = Checksum) */
	int dde_marche;				/* Demande de mise en marche */
	int mbl;					/* Mode de fonctionnement */
	int kiss;					/* Mode kiss : get-away */
	int groupe;					/* current group */
	long cur_bull;				/* current bulletin */
	int ch_mon;					/* Mode monitoring du gateway */
	int temp1;
	int temp2;
	int temp3;
	int ind_mess;				/* Indice du tableau de messages */
	int t_tr;					/* Indique un traitement en cours */
	int seq;					/* Interruption traitements longs */
	int sr_mem;					/* gestion de la memoire de sortie */
	int log;					/* autorisation de log */
	unsigned no_indic;			/* Numero de l'indicatif dans la liste */
	unsigned warning;			/* Demande de warnings */
	date_t tdeb;
	double t_trajec;
	double r4;
	double r6;
	uchar msg_held;				/* Messages de la voie a passer en HOLD */
	uchar pack;					/* Packets en cours d'emission */
	uchar mbl_ext;				/* Extension du forward MBL (LATER/REJECT) */
	/*    uchar xfwd           ;  Protocole XFWD identifie */
	uchar ret;				/* Return character seen */
	uchar read_only;			/* Mode d'acces read-only */
	uchar vdisk;				/* Numero du disque virtuel de FbbDos */
	uchar reverse;				/* Sens de balayage de la liste des messages */
	uchar entete;				/* Position dans la reception d'un message */
	uchar header;				/* Lignes R: recues */
	uchar bbsfwd;				/* numero de la bbs forwardee */
	uchar maxfwd;				/* taille max des messages forwardes */
	uchar oldfwd;				/* delai max des messages forwardes */
	uchar typfwd;				/* Type de messages forwardes (0 = tous) */
	uchar nb_choix;				/* Nombre de choix du link courant */
	uchar cur_choix;			/* Choix surlequel se fait le link */
	uchar nb_egal;				/* Nombre de messages '=' en instance */
	/* uchar nb_dupes       ;  Nombre de messages dupliques dans bloc */
	uchar m_ack;				/* Demande un message d'ack */
	uchar clock;				/* Mise a l'heure PMS */
	uchar rev_param;			/* Mise a jour des parametres en reverse */
	uchar cmd_new;				/* Commande "new" running */
	int maxbuf;
	/* uchar wp             ; * Demande de mise a jour des "White Pages" */
	double w;					/* Sauvegarde longitude */
	double l;					/* Sauvegarde latitude  */
	double cumul_dist;			/* Cumul de distances */
	long debut;					/* Heure de debut de connexion */
	long tstat;					/* Temps d'occupation des rubrique */
	long stemps[NBRUB];			/* Temps d'occupation par rubrique */
	long tmach;					/* Temps machine cumule par voie */
	long messdate;				/* Date de creation du message */
	int typlist;				/* type de liste (avec / sans BID) */
	int localmode;				/* Mode de fonctionnement local a la voie */
	long ask;					/* Demande de traitement apres huffman */
	long nmess;					/* No du message en cours de forward */
	long tailm;					/* Taille du message en cours */
	long size_trans;			/* Taille du transfert */
	long time_trans;			/* Temps de transfert */
	long l_mess;				/* Dernier message liste par L */
	long l_hold;				/* Dernier message liste par RE */
	long l_yapp;				/* Derniere consultation du YN */
	long mess_num;				/* Numero original du message */
	long mess_egal[NB_DEL];		/* Numero des messages reportes */
	long pass_time;				/* Time pour calculer le passwd MD2 */
	char ch_temp[MAXPATH];		/* Stockage temporaire */
	char label[MAXPATH];		/* Label du fichier en cours de reception */
	char dos_path[41];			/* Path actuel du DOS */
	char mess_home[41];			/* Home BBS du message */
	char passwd[5];				/* Password */
	char sr_fic[MAXPATH];		/* nom du fichier en cours d'emission */
	char mess_bid[13];			/* nom du fichier en cours d'emission */
	Ylabel *llabel;				/* Liste des label YAPP */
	rd_list *t_read;			/* tete de la liste des messages a lire */
	rd_list *t_list;			/* liste des messages a lister */
	tlist recliste;				/* Sauvegarde contexte trait. long */
	Forward *curfwd;			/* Bloc courant de forward */
	long noenr_menu;
	obuf *outptr;				/* pointeur du buffer de sortie */
	ibuf inbuf;					/* structures de buffers reception */
	char appendf[MAXPATH];		/* nom du fichier append */
	char chck;					/* Checksum transfert binaire */
	bullist entmes;				/* pointeur de la structure message */
	bullist fb_mess[MAX_FB];	/* Messages en attente de forward */
	char ok_mess[MAX_FB];		/* Messages en attente de com/decompression */
	ind_noeud *emis;			/* noeud de travail */
	ind_noeud *ncur;			/* noeud de la pers. connectee */
	info finf;					/* struct enr. fichier info */
	obuf *msgtete;				/* tete de message en cours d'ecriture */
	long enrcur;				/* enrg. du msg. en cours */
	struct ffblk dirblk;		/* structures DTA des directories */
	typedit tete_edit;			/* structure de l'editeur */
	Route *r_tete;				/* Tete des routes en reception message */
	Route *r_curr;				/* Curr des routes en reception message */
	int r_pos;					/* position dans le tableau */
	char p_route[NB_P_ROUTE][7];	/* routes prioritaires */
	Ylist *ytete;				/* Tete de la liste de fichiers en batch */
	typ_pfwd *ctnc;				/* Init du TNC apres la connection */
	void *ptemp;				/* Poiteur pour applis temporaires */
	unsigned psiz;				/* Taille de l'alloc temporaire */
	XInfo *Xfwd;				/* Structure pour XForwarding */
#ifdef __LINUX__
	int to_rzsz[2];				/* Pipe pour la communication avec RZ/SZ */
	int to_xfbb[2];				/* Pipe pour la communication avec RZ/SZ */
	int rzsz_pid;
#endif
#ifdef __WINDOWS__
	WINF Winh;					/* Informations fenetre */
#ifdef __WIN32__
	HANDLE task;				/* Tache en cours */
#else
	HTASK task;					/* Tache en cours */
#endif
#endif
}
Svoie;

/* #if !defined(MAIN) */
#include "fbb_var.h"
#include "fbb_dec.h"
/* #endif */

#if  FBB_DEBUG

void debut_fonction (char *, int, char *);
void fin_fonction (void);
void print_fonction (FILE *);
void print_history (FILE *);

#define df(str,lg)	debut_fonction(str, lg, MK_FP(_SS, _BP))
#define ff()		fin_fonction()

#else

#define df(str,lg)
#define ff()

#endif

#include <fortify.h>

#endif /* _fbb_serv */
