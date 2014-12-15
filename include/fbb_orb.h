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
 * Packet sent protocol :
 * buf[0] = service
 * buf[1] = command
 * buf[2] = data length LSB
 * buf[3] = data length MSB
 *
 * Service :
 *	ORB_CONSOLE
 *	ORB_MONITOR
 *	ORB_CHANNEL
 *		data[0] = channel (CONSOLE=0, MONITOR=0xff, channel [1..254]);
 *		data[1] = color;
 *		data[2] = header;
 *		data[3..] = data	
 *	ORB_XFBBX
 *		data[0] = 0: disconnected
 *	 			  1: connected
 *				  2: connexion refused
 *				  3: ask editor for Sx
 *				  4: ask editor for SR
 *	ORB_LISTCNX
 *		(data = connection line)
 *	ORB_MSGS
 *		(data = private and bulletin number)
 *	ORB_STATUS
 *		(data = status)
 *	ORB_NBCNX
 *		(data = connection number)
 *	ORB_DATA
 *		Command = 0 : list of services
 *			(data = list of services (one per byte))
 *		Command = 1 : Dir
 *			(1st packet = filename/dir-mask, next packets = data, length=0 = end)
 *		Command = 2 : File sent to client
 *			(1st packet = filename+size, next packets = data, length=0 = end)
 *		Command = 3 : File received accepted (length=0)
 *			(1st packet = filename+size, next packets = data, length=0 = end)
 *		Command = 4 : Forward management to client
 *			(1st packet = filename+size, next packets = data, length=0 = end)
 *		Command = 5 : Disconnection of users
 *			(1st packet = Channel(1..50) callsign immediate(0/1))
 */

/*
 * Packet received protocol :
 * buf[0] = service
 * buf[1] = command
 *
 * Service :
 *	ORB_REQUEST
 *	Command :
 *		0 : mask configuration
 *			buf[2] = mask
 *		1 : Directory (mask follows)
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = directory mask
 *		2 : get file (filename follows)
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = filename
 *		3 : put file (1st packet = filename, next packets = data, length=0 = end)
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = filename or data
 *		4 : forward management
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = command
 *		5 : disconnect request
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = command
 *		6 : user management
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = command
 *		7 : message management
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = command
 *		8 : message request
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = command
 *		9 : options management
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = command
 *		10: pactor management
 *			buf[2] = data length LSB
 *			buf[3] = data length MSB
 *			buf[4..] = command
 */

#define ORB_REQUEST	0
#define ORB_MSGS	1
#define ORB_STATUS	2
#define ORB_NBCNX	4
#define ORB_LISTCNX	8
#define ORB_MONITOR	16
#define ORB_CONSOLE	32
#define ORB_CHANNEL	64
#define ORB_XFBBX	128
#define ORB_DATA	0

#define SVC_LIST	0
#define SVC_DIR		1
#define SVC_RECV	2
#define SVC_SEND	3
#define SVC_FWD		4
#define SVC_DISC	5
#define SVC_USER	6
#define SVC_MSG		7
#define SVC_MREQ	8
#define SVC_OPT		9
#define SVC_PACTOR	10
#define SVC_INFO	11
#define SVC_MAX		12

#define IYapp	1
#define ICall	2
#define IDigis	3
#define IName	4
#define IHome	5
#define IChan	6
#define IPort	7
#define IN1N2N3	8
#define IFlags	9
#define IPaclen	10
#define IStatus	11
#define IMem	12
#define IBuf	13
#define IRet	14
#define IPerso	15
#define IUnread	16
