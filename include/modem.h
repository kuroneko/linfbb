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


/* Extensions au driver WINDOWS comm.drv */
#define	SETECHO	12
#define	CLRECHO	13
#define	SETSTAR	14


#define	INQUE		16384
#define	OUTQUE	16384

#define  INT14   0x14

/* Emission */

#define XS_INIT  0
#define XS_SEND  1
#define XS_WAIT  2
#define XS_END   3
#define XS_QUEUE 10
#define XS_EXTERN 11

/* Reception */

#define XR_INIT  4
#define XR_LABL  5
#define XR_RECV  6
#define XR_END   7
#define XR_EXTERN 12

/* Communes */

#define XM_ABORT 8

/* Zmodem */

#define ZS_FILE  9

/* Constantes */

#define NUL 0
#define SOH 1
#define EOT 4
#define ACK 6
#define NAK 21
#define CAN 24

/* Macros */

#define xmodem_off(voie)   xmodem_mode(0x00, voie)
#define xmodem_tx_on(voie) xmodem_mode(0x01, voie)
#define xmodem_rx_on(voie) xmodem_mode(0x02, voie)
#define xmodem_fin(voie)   xmodem_mode(0x04, voie)
#define xmodem_tx_1k(voie) xmodem_mode(0x05, voie)
#define ymodem_tx_on(voie, type) xmodem_mode((type) ? 0x07 : 0x06, voie)

/* Constantes ZModem */

#define ZRQINIT	0
#define ZRINIT	1
#define ZSINIT	2
#define ZACK	3
#define ZFILE	4
#define ZSKIP	5
#define ZNAK	6
#define ZABORT	7
#define ZFIN	8
#define ZRPOS	9
#define ZDATA	10
#define ZEOF	11
#define ZFERR	12
#define ZCRC	13
#define ZCHALL	14
#define ZCOMPL	15
#define ZCAN	16
#define ZFREEC	17
#define ZCOMM	18
#define ZSTDERR	19
