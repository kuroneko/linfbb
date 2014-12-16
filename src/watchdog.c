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
 * WATCHDOG.C
 *
 */

#include <serv.h>

static int watch_option = 0;

#ifdef __WINDOWS__
#ifdef __WIN32__
static HANDLE watchfd;

#else
static int watchfd;

#endif
#endif /* WINDOWS */

 /***************************************
 * Specifie un port pour le watchdog :
 * 0 = rien
 * 1 = lpt1
 * 2 = lpt2
 * etc...
 * 81 = com1
 * 82 = coim2
 * etc...
 ****************************************/
void init_watchdog (int val)
{
#ifdef __WINDOWS__
	char str[80];
	DCB dcb;
	int error;

	if (val == 0)
		return;

	if (val > 80)
		wsprintf (str, "LPT%d", val - 80);
	else
		wsprintf (str, "COM%d", val);

#ifdef __WIN32__
	watchfd = CreateFile (str, GENERIC_READ | GENERIC_WRITE,
						  0,	// exclusive access
						   NULL,	// no security attrs
						   OPEN_EXISTING,
						  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
						  NULL);

	if (watchfd == INVALID_HANDLE_VALUE)
		return;

#else
	watchfd = OpenComm (str, 64, 64);
	if (watchfd < 0)
	{
		/* Erreur ouverture COM */
		char title[80];

		wsprintf (title, "Error Watchdog %d", val);
		ShowError (title, "OpenComm :", 0);
		return;
	}
#endif

	if (val < 80)
	{
		memset ((char *) &dcb, '\0', sizeof (DCB));
		wsprintf (str, "COM%d:9600,n,8,1", val);
		error = BuildCommDCB (str, &dcb);

#ifdef __WIN32__
		error = SetCommState (watchfd, &dcb);
#else
		error = SetCommState (&dcb);
#endif
	}

	watch_option = val;

#endif
}

void end_watchdog (void)
{
#ifdef __WINDOWS__
	int ret;

	if (!watch_option)
	{
		return;
	}

#ifdef __WIN32__
	if (watchfd != INVALID_HANDLE_VALUE)
	{
//      EscapeCommFunction (ptrcom->comfd, SETXON);
		if (!CloseHandle (watchfd))
		{
			char title[80];

			wsprintf (title, "Error watchdog %d", watch_option);
			ShowError (title, "CloseComm :", ret);
		}
		watchfd = INVALID_HANDLE_VALUE;
	}
#else
	if (watchfd != -1)
	{
		if ((ret = CloseComm (watchfd)) < 0)
		{
			char title[80];

			wsprintf (title, "Error watchdog %d", watch_option);
			ShowError (title, "CloseComm :", ret);
		}
		watchfd = -1;
	}
#endif
#endif
	watch_option = 0;
}

void watchdog (void)
{
#ifdef __WINDOWS__
	time_t temps;
	static int car = 0;
	static time_t prev = 0;

	int nb;
	int err;

	if (!watch_option)
	{
		return;
	}

	temps = time (NULL);
	if (temps == prev)
		return;

	prev = temps;
	car = !car;

#ifdef __WIN32__

	{
		DWORD lpErrors = 0L;
		char title[80];

		/* Get the first spurious error */
		ClearCommError (watchfd, &lpErrors, NULL);

		ClearCommError (watchfd, &lpErrors, NULL);
		if (lpErrors & (CE_PTO | CE_IOE | CE_DNS))
		{
			wsprintf (title, "Error watchdog %d", watch_option);
			ShowError (title, "Device not ready, watchdog disabled :", lpErrors);
			end_watchdog ();
			return;
		}
	}

	if (!WriteFile (watchfd, &car, 1, &nb, NULL))
	{
		char title[80];

		wsprintf (title, "Error watchdog %d", watch_option);
		ShowError (title, "WriteComm :", nb);
		return;
	}

#else

	nb = WriteComm (watchfd, &car, 1);
	if (nb < 0)
	{
		char title[80];

		while (err = GetCommError (watchfd, NULL))
		{
			fbb_warning (9, "Send error : ", err);
		}
		wsprintf (title, "Error watchdog %d", watch_option);
		ShowError (title, "WriteComm :", nb);
		return;
	}
#endif

#endif
}
