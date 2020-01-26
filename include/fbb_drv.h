/***********************************************************************
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
    along with this program. If not, see <http://www.gnu.org/licenses/>.

    Parts of code have been taken from many other softwares.
    Thanks for the help.
***********************************************************************/


/*
 * Declaration des fonctions drivers
 */


#define STA_DRV	0
#define SND_DRV	1
#define RCV_DRV	2
#define OPN_DRV	3
#define CLS_DRV	4
#define TYP_DRV	5

/*
 * Drivers WA8DED
 */

int sta_ded (int, int, int, void *);
int snd_ded (int, int, int, char *, int, Beacon *);
int rcv_ded (int *, int *, int *, char *, int *, ui_header *);
int opn_ded (int, int);
int cls_ded (int);

/*
 * Drivers PK232
 */

int sta_aea (int, int, int, void *);
int snd_aea (int, int, int, char *, int, Beacon *);
int rcv_aea (int *, int *, int *, char *, int *, ui_header *);

/*
 * Drivers BPQ
 */

int sta_bpq (int, int, int, void *);
int snd_bpq (int, int, int, char *, int, Beacon *);
int rcv_bpq (int *, int *, int *, char *, int *, ui_header *);

/*
 * Drivers KAM
 */

int sta_kam (int, int, int, void *);
int snd_kam (int, int, int, char *, int, Beacon *);
int rcv_kam (int *, int *, int *, char *, int *, ui_header *);

/*
 * Drivers MODEM
 */

int sta_mod (int, int, int, void *);
int snd_mod (int, int, int, char *, int, Beacon *);


/*
 * Drivers TCP
 */

#if defined(__WINDOWS__)
int sta_tcp (int, int, int, void *);
int snd_tcp (int, int, int, char *, int, Beacon *);
int rcv_tcp (int *, int *, int *, char *, int *, ui_header *);

#elif defined(__linux__)
int sta_tcp (int, int, int, void *);
int snd_tcp (int, int, int, char *, int, Beacon *);
int rcv_tcp (int *, int *, int *, char *, int *, ui_header *);
int opn_tcp (int, int);
int cls_tcp (int);

#endif


/*
 * Drivers SOCKET
 */

#ifdef __linux__
int sta_sck (int, int, int, void *);
int snd_sck (int, int, int, char *, int, Beacon *);
int rcv_sck (int *, int *, int *, char *, int *, ui_header *);
int opn_sck (int, int);
int cls_sck (int);

#endif


/*
 * Drivers AGW
 */

#ifdef __WINDOWS__
int sta_agw (int, int, int, void *);
int snd_agw (int, int, int, char *, int, Beacon *);
int rcv_agw (int *, int *, int *, char *, int *, ui_header *);
int opn_agw (int, int);
int cls_agw (int);

#endif

/*
 * Drivers HostMode PTC-II
 */

int sta_hst (int, int, int, void *);
int snd_hst (int, int, int, char *, int, Beacon *);
int rcv_hst (int *, int *, int *, char *, int *, ui_header *);
int opn_hst (int, int);
int cls_hst (int);

/*
 * Drivers Flexnet
 */

#if defined(__WINDOWS__) || defined(__FBBDOS__)
int sta_flx (int, int, int, void *);
int snd_flx (int, int, int, char *, int, Beacon *);
int rcv_flx (int *, int *, int *, char *, int *, ui_header *);
int opn_flx (int, int);
int cls_flx (int);

#endif

/*
 * Drivers POP
 */

#if defined(__linux__)
int sta_pop (int, int, int, void *);
int snd_pop (int, int, int, char *, int, Beacon *);
int rcv_pop (int *, int *, int *, char *, int *, ui_header *);
int opn_pop (int, int);
int cls_pop (int);

#endif


