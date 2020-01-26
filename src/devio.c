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

/*
 * Routines d'entrees-Sorties
 *
 */


#include <serv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

static int car_tnc_linux(int port);
static int closecom_linux(int com);
int initcom_linux (int com);
static int rec_tnc_linux (int port);
static void send_tnc_linux (int port, int carac);

int drsi_port (int port, int canal)
{
	int i;

	df ("drsi_port", 2);

	for (i = 1; i < NBPORT; i++)
	{
		if (DRSI (i))
		{
			if (p_port[i].ccanal == canal)
			{
				ff ();
				return (i);
			}
		}
	}
	ff ();
	return (port);
}

int hst_port (int port, int canal)
{
	int i;

	for (i = 1; i < NBPORT; i++)
	{
		if (HST (i))
		{
			if (p_port[i].ccanal == canal)
			{
				ff ();
				return (i);
			}
		}
	}
	return (port);
}

int bpq_port (int port, int canal)
{
	int i;

	df ("bpq_port", 2);

	for (i = 1; i < NBPORT; i++)
	{
		if (BPQ (i))
		{
			if (p_port[i].ccanal == canal)
			{
				ff ();
				return (i);
			}
		}
	}
	ff ();
	return (port);
}

int linux_port (int port, int canal)
{
	return (canal);
}

int initcom (void)
{
  int com, port, valid;
  
  df ("initcom", 0);
  
#ifdef ENGLISH
  if (DEBUG)
    cprintf ("Debug valid \r\n");
#else
  if (DEBUG)
    cprintf ("Debug valide\r\n");
#endif
  else
    {
      for (com = 1; com < NBPORT; com++)
	{
	  valid = 0;
	  p_com[com].comfd = -1;
	  for (port = 1; port < NBPORT; port++)
	    {
	      if ((p_port[port].pvalid) && (p_port[port].ccom == com))
		{
		  switch (p_port[port].typort)
		    {
		    case TYP_MOD :
		    case TYP_DED :
		    case TYP_HST :
		      valid = 1;
		      break;
		    default :
		      valid = 0;
		      break;
		    }
		  break;
		}
	    }
	  if (valid)
	    {
	      int	ret = 0;
	      
	      switch (p_com[com].combios)
		{
		case P_LINUX:
		  ret = initcom_linux (com);
		  break;
		}
	      if (!ret)
		return(FALSE);
	    }
	}
    }
  sleep_ (1);
  ff ();
  return(TRUE);
}


void closecom (void)
{
	int com, port, valid;

#ifdef ENGLISH
	if (DEBUG)
		cprintf ("Debug valid \r\n");
#else
	if (DEBUG)
		cprintf ("Debug valide\r\n");
#endif
	else
	{
	  if (p_port == NULL)
	    return;
	  
	  for (com = 1; com < NBPORT; com++)
	    {
	      valid = 0;
	      for (port = 1; port < NBPORT; port++)
		{
#ifdef __WINDOWS__
		  if ((p_port[port].pvalid) && (ETHER (port)))
		    {
		      free_socket (port);
		    }
#endif
		  if ((p_port[port].pvalid) && (p_port[port].ccom == com))
		    {
		      switch (p_port[port].typort)
			{
			case TYP_MOD :
			case TYP_DED :
			case TYP_HST :
			  valid = 1;
			  break;
			default :
			  valid = 0;
			  break;
			}
		      break;
		    }
		}
	      if (valid)
		{
		  switch (p_com[com].combios)
		    {
		      
		    case P_LINUX:
		      closecom_linux(com);
		      break;
		    }
		}
	    }
	}
	sleep_ (1);
	return;
}


int car_tnc (int port)
{
	int val;

	df ("car_tnc", 1);

	switch (BIOS (port))
	{
	case P_LINUX:
	  val = car_tnc_linux (port);
	  return(val);
	}
	ff ();
	return (1);
}


int rec_tnc (int port)
{
	int c = -1;

	df ("rec_tnc", 1);

	switch (BIOS (port))
	{

	case P_LINUX:
		c = rec_tnc_linux (port);
		break;
	default:
		fbb_error (ERR_TNC, "RECEIVE: WRONG INTERFACE", BIOS (port));
		break;
	}
	ff ();
	return (c);
}

void send_tnc (int port, int carac)
{

	df ("send_tnc", 2);


	switch (BIOS (port))
	{

	case P_LINUX:
		send_tnc_linux (port, carac);
		break;
	default:
		fbb_error (ERR_TNC, "SEND: WRONG INTERFACE", BIOS (port));
	}
	ff ();
	return;
}


int car_tx (int port)
{
	return (0);
}


#undef inportb
#undef outportb

void selcanal (int port)
{
	return;
}

/*

 * Routines Entrees-Sortie  ... Interface avec LINUX
 *
 */



static struct termios def_tty;

int default_tty(int com)
{
  return tcsetattr(p_com[com].comfd, TCSANOW, &def_tty);
}

#undef open
#undef close
#undef read
#undef write

int initcom_linux (int com)
{
  int spd;
  int comfd;
  int newbaud;
  struct termios tty;

  /* Ferme le port si deja ouvert */
  closecom_linux (com);

  /* sprintf(buf, "/dev/cua%d", com-1); */
  printf("Init %s\n", p_com[com].name);

  comfd = open(p_com[com].name, O_RDWR);
  if (comfd == -1)
  {
    fprintf(stderr, "com%d : cannot open the device %s\n", com, p_com[com].name);
    return(0);
  }

  p_com[com].comfd = comfd;

  tcgetattr(comfd, &def_tty);
  tcgetattr(comfd, &tty);

  newbaud = (int)(p_com[com].baud >> 5);
  if (p_com[com].options & 0x20)
    newbaud += 10;
  
  printf("newbaud = %d\n", newbaud);
  switch(newbaud) 
    {
    case 2:     spd = B300;	break;
    case 3:	spd = B600;	break;
    case 4:	spd = B1200;	break;
    case 5:	spd = B2400;	break;
    case 6:	spd = B4800;	break;
    case 7:	spd = B9600;	break;
    case 12:	spd = B19200;	break;
    case 14:	spd = B38400;	break;
    case 15:	spd = B57600;	break;
    case 16:	spd = B115200;	break;
    default:    spd = -1;       break;
  }

  if (spd != -1) 
    {
      cfsetospeed(&tty, (speed_t)spd);
      cfsetispeed(&tty, (speed_t)spd);
    }


  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;

  /* Set into raw, no echo mode */

  tty.c_iflag &= ~(IGNBRK | IGNCR | INLCR | ICRNL | IUCLC | 
  	IXANY | IXON | IXOFF | INPCK | ISTRIP);
  tty.c_iflag |= (BRKINT | IGNPAR);
  tty.c_oflag &= ~OPOST;
  tty.c_lflag = ~(ICANON | ISIG | ECHO | ECHONL | ECHOE | ECHOK);
  tty.c_cflag |= CREAD | CRTSCTS;
 
  /*
    #else Okay, this is better. XXX - Fix the above.
    tty.c_iflag =  IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cflag |= CLOCAL | CREAD;
    #endif
  */
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 5;

  /* Flow control. */
  /*
  if (!hwf) tty.c_cflag &= ~CRTSCTS;
  if (swf) tty.c_iflag |= IXON;
  */

  tty.c_cflag &= ~(PARENB | PARODD);
  /*
  if (par[0] == 'E')
	tty.c_cflag |= PARENB;
  else if (par[0] == 'O')
	tty.c_cflag |= PARODD;
	*/

  tcsetattr(comfd, TCSANOW, &tty);
  tcflow(comfd, TCOON);

   /*
  {
    / Set RTS /
    int mcs;

    ioctl(comfd, TIOCMGET, &mcs);
    mcs |= TIOCM_RTS;
    ioctl(comfd, TIOCMSET, &mcs);
  }
  */

  return (TRUE);
}


static int rec_tnc_linux (int port)
{
  int chr;

  if (!car_tnc_linux(port))
    return(-1);

  read(p_com[(int)p_port[port].ccom].comfd, &chr, 1);
  return (chr & 0xff);
}


static void send_tnc_linux (int port, int carac)
{
  write(p_com[(int)p_port[port].ccom].comfd, &carac, 1);
}

static int car_tnc_linux(int port)
{
  long i = 0;

	(void) ioctl(p_com[(int)p_port[port].ccom].comfd, FIONREAD, &i);
  return((int)i);
}

static int closecom_linux(int com)
{

  if (p_com[com].comfd == -1)
    return(0) ;

  sleep(2);
xprintf("Close com !\n");
  close(p_com[com].comfd);
  p_com[com].comfd = -1;
  return(1);
}

/*

 * end of devio.c
 *
 */
