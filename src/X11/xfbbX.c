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



#define __MAIN__

#include <xfbbX.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include <stdint.h>

#include <Xm/Protocols.h>

#include <fbb.xbm>

#define TOT_VOIES 256

#define SEPARATOR 0
#define BUTTON 1
#define TOGGLE 2
#define RADIO 3
#define TOOLBUTTON 4

static void HelpAct (Widget, XEvent *, String *, Cardinal *);
static void ConsoleAct (Widget, XEvent *, String *, Cardinal *);
static void DisconnAct (Widget, XEvent *, String *, Cardinal *);
static void PendingAct (Widget, XEvent *, String *, Cardinal *);
static void MonitorAct (Widget, XEvent *, String *, Cardinal *);
static void SetCallAct (Widget, XEvent *, String *, Cardinal *);
static void ProgTncAct (Widget, XEvent *, String *, Cardinal *);
static void GatewayAct (Widget, XEvent *, String *, Cardinal *);
static void TalkToAct (Widget, XEvent *, String *, Cardinal *);
static void MsgScanAct (Widget, XEvent *, String *, Cardinal *);
static void EditorAct (Widget, XEvent *, String *, Cardinal *);
static void ListCnxAct (Widget, XEvent *, String *, Cardinal *);
static void SndTextAct (Widget, XEvent *, String *, Cardinal *);
static void ConnectItem (int, char *);
static void makekey (char *, char *, char *);
static void end_wait (void);
static void cursor_wait (void);

static XtActionsRec actionsTable[] =
{
	{"Help", HelpAct},
	{"Console", ConsoleAct},
	{"Disconnect", DisconnAct},
	{"PendingFwd", PendingAct},
	{"Monitor", MonitorAct},
	{"SetCall", SetCallAct},
	{"ProgTnc", ProgTncAct},
	{"Gateway", GatewayAct},
	{"TalkTo", TalkToAct},
	{"MsgScan", MsgScanAct},
	{"Editor", EditorAct},
	{"ListCnx", ListCnxAct},
	{"SndText", SndTextAct}
};

#define NB_PITEM 4
#define NB_ICON 16
#define NB_INIT_B 11

static int prints = 0;
static int comm_ok = 1;
static int filters;
static int fbb_sock = -1;
static int first = 1;

static XtInputId InputId = 0;

static Widget MCons;
static Widget MAllc;
static Widget MMon;

static FILE *p_fptr;
static int r_index;

static XtAppContext app_context;

static Pixel df_pixel;
static Pixel rf_pixel;
static Pixel vf_pixel;
static Pixel bf_pixel;
static Pixel no_pixel;
static Pixel rc_pixel;
static Pixel vc_pixel;
static Pixel bc_pixel;

static XmRendition r_rend[10];

static Widget BIcon[NB_ICON];
static Widget form;

static Widget Footer;
static int foothelp;

static Widget ConnectLabel;
static Widget ConnectList;
static Widget MsgsToggle;
static Widget StatusToggle;
static Widget MenuBar;
static Widget ToolBar;
static Widget MenuWindow;
static Widget MenuRemote;
static Widget MenuConfig;
static Widget MenuHelp;
static Widget ListForm;
static Widget StatForm;
static Widget ConnectString;
static Widget StatList;
static Widget MenuFile;

/* Liste des widgets de status */
static Widget TxtUsed;
static Widget TxtGMem;
static Widget TxtDisk1;
static Widget TxtDisk2;
static Widget TxtMsgs;
static Widget TxtResync;
static Widget TxtState;
static Widget TxtHold;
static Widget TxtPriv;

static Widget Used;
static Widget GMem;
static Widget Disk1;
static Widget Disk2;
static Widget Msgs;
static Widget Resync;
static Widget State;
static Widget Hold;
static Widget Priv;
static Widget Popup;
static Widget PItem[NB_PITEM];

void TalkToCB (Widget w, XtPointer client_data, XtPointer call_data);
void OneChanCB (Widget w, XtPointer client_data, XtPointer call_data);
void DisconnectCB (Widget w, XtPointer client_data, XtPointer call_data);

int is_connected (void)
{
	return (fbb_sock > 0);
}

int close_connection (void)
{
	if (fbb_sock > 0)
	{
		HideFbbWindow (ALLCHANN, toplevel);
		HideFbbWindow (CONSOLE, toplevel);
		HideFbbWindow (MONITOR, toplevel);
		close (fbb_sock);
		fbb_sock = -1;
		if (InputId)
			XtRemoveInput (InputId);
		InputId = 0;
	}
	Caption (1);
	return 1;
}

static int open_connection (char *tcp_addr, int tcp_port, int mask)
{
	int sock;
	struct sockaddr_in sock_addr;
	struct hostent *phe;

	if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror ("socket_r");
		return (0);
	}

	if ((phe = gethostbyname (tcp_addr)) == NULL)
	{
		perror ("gethostbyname");
		return (-1);
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons (tcp_port);
	memcpy ((char *) &sock_addr.sin_addr, phe->h_addr, phe->h_length);

	if (connect (sock, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) == -1)
	{
		perror ("connect");
		close_connection ();
		return (-1);
	}

	return (sock);
}

static int do_filter (char *ptr, int len)
{
	char *scan = ptr;
	int lg = 0;

	while (len)
	{
		if ((*ptr == '\n') || isprint (*ptr))
			scan[lg++] = *ptr;
		++ptr;
		--len;
	}
	return (lg);
}

void Received (int channel, char *text, int len)
{
	window_write (CONSOLE, text, len, 0, 0);
}

void ReceiveMonitor (int channel, char *text, int len, int color, int header)
{
	if (channel == 255)
		channel = MONITOR;
	else
		channel = ALLCHANN;
	window_write (channel, text, len, color, header);
}

void ReceiveList (int channel, char *text)
{
	ConnectItem (channel, text);
}

void ReceiveNbCh (int nb)
{
	char buffer[80];

	if (nb == 0)
		strcpy (buffer, "No connected station");
	else
		sprintf (buffer, "%d connected station%c", nb, (nb > 1) ? 's' : '\0');
	LabelSetString (ConnectLabel, buffer, NULL);
}

void ReceiveNbMsg (int priv, int hold, int nbmess)
{
	static int old_nbmess = -1L;
	static int old_priv = -1L;
	static int old_hold = -1L;
	char texte[80];

	if (!(conf[curconf].mask & FBB_MSGS))
		priv = hold = nbmess = 0;

	if (old_nbmess != nbmess)
	{
		sprintf (texte, ": %d", nbmess);
		LabelSetString (Msgs, texte, NULL);
		old_nbmess = nbmess;
	}

	if (priv != old_priv)
	{
		sprintf (texte, ": %d", priv);
		LabelSetString (TxtPriv, "Priv msgs", (priv > 0) ? "RC" : NULL);
		LabelSetString (Priv, texte, (priv > 0) ? "RC" : NULL);
		old_priv = priv;
	}

	if (hold != old_hold)
	{
		sprintf (texte, ": %d", hold);
		LabelSetString (TxtHold, "Hold msgs", (hold > 0) ? "RC" : NULL);
		LabelSetString (Hold, texte, (hold > 0) ? "RC" : NULL);
		old_hold = hold;
	}
}

void ReceiveStatus (int lmem, int gmem, int disk1, int disk2)
{
	static int old_lmem = -1L;
	static int old_gmem = -1L;
	static int old_disk1 = -1L;
	static int old_disk2 = 0L;
	char texte[80];

	if (!(conf[curconf].mask & FBB_STATUS))
		disk1 = disk2 = 9999;

	if (old_lmem != lmem)
	{
		sprintf (texte, ": %d", lmem);
		LabelSetString (Used, texte, NULL);
		old_lmem = lmem;
	}

	if (old_gmem != gmem)
	{
		sprintf (texte, ": %d K", gmem << 4);
		LabelSetString (GMem, texte, NULL);
		old_gmem = gmem;
	}

	if (disk1 != old_disk1)
	{
		sprintf (texte, ": %d K", disk1);
		LabelSetString (TxtDisk1, "Disk#1 free", (disk1 < 1000) ? "RC" : NULL);
		LabelSetString (Disk1, texte, (disk1 < 1000) ? "RC" : NULL);
		old_disk1 = disk1;
	}

	if (disk2 != old_disk2)
	{
		if (disk2 == -1L)
		{
			LabelSetString (TxtDisk2, "", NULL);
			LabelSetString (Disk2, "", NULL);
		}
		else
		{
			sprintf (texte, ": %d K", disk2);
			LabelSetString (TxtDisk2, "Disk#2 free", (disk2 < 1000) ? "RC" : NULL);
			LabelSetString (Disk2, texte, (disk2 < 1000) ? "RC" : NULL);
		}
		old_disk2 = disk2;
	}
}

static int orb_input (Widget w, int *sock, XtInputId * id)
{
	int nb;
	char buffer[4096];
	char save;
	unsigned int service;
	unsigned int len;
	char *scan;
	static int in_input = 0;
	static int pos = 0;

	if (in_input)
		return 0;

	++in_input;

	nb = read (*sock, buffer + pos, sizeof (buffer) - pos);
	if (nb == -1)
	{
		perror ("read");
		--in_input;
		close_connection ();
		return 0;
	}

	if (nb == 0)
	{
		MessageBox (0, "Connection closed", "End", MB_OK | MB_ICONEXCLAMATION);
		--in_input;
		close_connection ();
		return 0;
	}

	scan = buffer;
	nb += pos;

	while (nb >= 4)
	{
		/* Read header first. Be sure the 4 bytes are read */
		service = (unsigned int) scan[0];
		len = ((unsigned int) scan[3] << 8) + (unsigned int) scan[2];

		if (nb < len + 4)
			break;

		scan += 4;
		nb -= 4;

		/* Read the data following the header. Be sure all bytes are read */
		save = scan[len];

		/* decodes and displays the services */
		switch (service)
		{
		case FBB_CONSOLE:
			if (len > 3)
				Received (scan[0], scan + 3, len - 3);
			scan[len - 3] = '\0';
			if (first && len == 30)
				MessageBox (0, scan + 3, "status", MB_OK | MB_ICONEXCLAMATION);
			first = 0;
			break;
		case FBB_MONITOR:
			if (len > 3)
			{
				int n = len;

				if (filters)
					n = do_filter (scan + 3, len - 3) + 3;
				ReceiveMonitor (scan[0], scan + 3, n - 3, scan[1], scan[2]);
			}
			break;
		case FBB_CHANNEL:
			if (len > 3)
				ReceiveMonitor (scan[0], scan + 3, len - 3, scan[1], scan[2]);
			break;
		case FBB_MSGS:
			{
				int nbPriv, nbHeld, nbTotal;

				scan[len] = '\0';
				sscanf (scan, "%d %d %d",
						&nbPriv, &nbHeld, &nbTotal);
				ReceiveNbMsg (nbPriv, nbHeld, nbTotal);
			}
			break;
		case FBB_STATUS:
			{
				int MemUsed, MemAvail, Disk1, Disk2;

				scan[len] = '\0';
				sscanf (scan, "%d %d %d %d",
						&MemUsed, &MemAvail, &Disk1, &Disk2);
				ReceiveStatus (MemUsed, MemAvail, Disk1, Disk2);
			}
			break;
		case FBB_NBCNX:
			scan[len] = '\0';
			ReceiveNbCh (atoi (scan));
			break;
		case FBB_LISTCNX:
			scan[len] = '\0';
			ReceiveList (atoi (scan + 1), scan);
			break;
		case FBB_XFBBX:
			switch (scan[0])
			{
			case 0:
				/* Deconnecte */
				HideFbbWindow (CONSOLE, toplevel);
				conf[curconf].mask &= ~FBB_CONSOLE;
				break;
			case 1:
				/* Connecte */
				conf[curconf].mask |= FBB_CONSOLE;
				ShowFbbWindow (CONSOLE, toplevel);
				break;
			case 2:
				/* Console busy */
				conf[curconf].mask &= ~FBB_CONSOLE;
				MessageBox (0,
							"Remote console is already connected", "Connect",
							MB_OK | MB_ICONEXCLAMATION);
				break;
			}
			end_wait ();
			break;
		}

		scan[len] = save;

		scan += len;
		nb -= len;
	}

	if (nb > 0)
	{
		int i;

		/* All data was not received. */
		/* Copy the rest to the beginning of the buffer */
		for (i = 0; i < nb; i++)
			buffer[i] = scan[i];
	}

	pos = nb;

	--in_input;

	return 0;
}

int init_orb (char *msg)
{
	char buffer[300];
	int sock;
	int console;
	int channel;
	int nb;
	char key[256];

	XmListDeleteAllItems (ConnectList);
	cursor_wait ();

	/* End the current connection */
	close_connection ();

	console = 0;
	channel = 0;
	filters = 0;

	sock = open_connection (conf[curconf].host, conf[curconf].port, conf[curconf].mask);

	if (sock == -1)
	{
		sprintf (msg, "Cannot connect xfbbd at %s:%d", conf[curconf].host, conf[curconf].port);
		Caption (1);
		end_wait ();
		return 0;
	}

	sprintf (buffer, "%d %d %s\n", conf[curconf].mask, channel, conf[curconf].mycall);
	write (sock, buffer, strlen (buffer));

	nb = read (sock, buffer, 20);
	if (nb <= 0)
	{
		sprintf (msg, "Connection closed");
		close (sock);
		Caption (1);
		end_wait ();
		return (0);
	}

	buffer[nb] = '\0';
	sscanf (buffer, "%s", key);

	makekey (key, conf[curconf].pass, buffer);
	strcat (buffer, "\n");
	write (sock, buffer, strlen (buffer));

	InputId = XtAppAddInput (app_context, sock, (XtPointer) XtInputReadMask, (XtInputCallbackProc) orb_input, NULL);

	fbb_sock = sock;
	first = 1;

	XmToggleButtonSetState (MsgsToggle, ((conf[curconf].mask & FBB_MSGS) != 0), True);
	XmToggleButtonSetState (StatusToggle, ((conf[curconf].mask & FBB_STATUS) != 0), True);
	Caption (1);

	/* Open windows depending of the mask */
	if (conf[curconf].mask & FBB_MONITOR)
		ShowFbbWindow (MONITOR, toplevel);
	else
		HideFbbWindow (MONITOR, toplevel);
	if (conf[curconf].mask & FBB_CHANNEL)
		ShowFbbWindow (ALLCHANN, toplevel);
	else
		HideFbbWindow (ALLCHANN, toplevel);

	end_wait ();

	return (1);
}

int console_input (char *buffer, int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (buffer[i] == '\r')
			buffer[i] = '\n';
	write (fbb_sock, buffer, len);
	return (1);
}

#include <X11/cursorfont.h>
void cursor_wait (void)
{
	static Cursor lcursor = 0;

	if (lcursor == 0)
	{
		lcursor = XCreateFontCursor (XtDisplay (toplevel), XC_watch);
	}

	XDefineCursor (XtDisplay (toplevel), XtWindow (toplevel), lcursor);
	XFlush (XtDisplay (toplevel));
}

void end_wait (void)
{
	XUndefineCursor (XtDisplay (toplevel), XtWindow (toplevel));
}

void DialogHelpCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	*((int *) client_data) = 2;
}

void DialogOkCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	*((int *) client_data) = 1;
}

void DialogCancelCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	*((int *) client_data) = 0;
}

void RaiseEV (Widget w, XtPointer client_data, XEvent * event)
{
	if (event->type != VisibilityNotify)
		return;
	XRaiseWindow (XtDisplay (w), XtWindow (XtParent (w)));
}

int MessageBox (int sec, char *texte, char *titre, int flags)
{
	Arg args[20];
	Cardinal n;
	Widget dialog;
	XEvent event;
	XmString string;
	int type;
	time_t temps;

	int retour = -1;

	n = 0;
	XtSetArg (args[n], XmNautoUnmanage, FALSE);
	n++;
	XtSetArg (args[n], XmNtitle, titre);
	n++;
	dialog = XmCreateMessageDialog (toplevel, "Warning", args, n);
	XtAddCallback (dialog, XmNokCallback, DialogOkCB, (XtPointer) & retour);
	XtAddCallback (dialog, XmNcancelCallback, DialogCancelCB, (XtPointer) & retour);
	XtAddCallback (dialog, XmNhelpCallback, DialogHelpCB, (XtPointer) & retour);
	XtAddEventHandler (dialog, VisibilityChangeMask, FALSE,
					   (XtEventHandler) RaiseEV, NULL);

	switch (flags & 0xf0)
	{
	case 0x10:
		type = XmDIALOG_ERROR;
		break;
	case 0x20:
		type = XmDIALOG_QUESTION;
		break;
	case 0x30:
		type = XmDIALOG_WARNING;
		break;
	case 0x40:
		type = XmDIALOG_INFORMATION;
		break;
	default:
		type = XmDIALOG_MESSAGE;
		break;
	}

	string = XmStringCreateSimple ("No");

	n = 0;
	XtSetArg (args[n], XmNdialogType, type);
	n++;
	XtSetArg (args[n], XmNhelpLabelString, string);
	n++;
	XtSetValues (dialog, args, n);

	XmStringFree (string);

	flags &= 0xf;
	if (flags == MB_OK)
	{
		XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
	}
	else if (flags == MB_OKCANCEL)
	{
		XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
	}
	else
	{
		/* YESNO et YESNOCANCEL */
		string = XmStringCreateSimple ("Yes");
		n = 0;
		XtSetArg (args[n], XmNokLabelString, string);
		n++;
		XtSetValues (dialog, args, n);
		XmStringFree (string);
		if (flags == MB_YESNO)
		{
			XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
		}
	}

	string = XmStringCreateSimple (texte);
	n = 0;
	XtSetArg (args[n], XmNmessageString, string);
	n++;
	XtSetValues (dialog, args, n);
	XmStringFree (string);

	XtManageChild (dialog);

	XtAddGrab (dialog, TRUE, TRUE);

	if (sec > 0)
	{
		temps = time (NULL) + (time_t) sec;
	}
	else
	{
		temps = 0;
	}

	while (retour == -1)
	{
		if (XtAppPending (app_context))
		{
			XtAppNextEvent (app_context, &event);
			XtDispatchEvent (&event);
		}
		else
			usleep (100000);

		if ((temps) && (temps < time (NULL)))
		{
			retour = 1;
			break;
		}
	}

	XtRemoveGrab (dialog);
	XtUnmanageChild (dialog);
	XtDestroyWidget (XtParent (dialog));

	return (retour);
}

static void sig_fct (int sig)
{
	int pid, pstatus;

	sig &= 0xff;
	printf ("Signal received = %d\n", sig);
	pid = wait (&pstatus);
	signal (sig, sig_fct);

	switch (sig)
	{
	case SIGHUP:
		/* reload system files */
		break;
	case SIGTERM:
		/* end of session */
		exit (0);
		break;
	case SIGBUS:
		/* end of session */
		fprintf (stderr, "xfbbd : Bus error\n");
		exit (5);
		break;
	case SIGSEGV:
		/* end of session */
		fprintf (stderr, "xfbb : Segmentation violation\n");
		exit (5);
		break;
	}
}

int SetChList (int clean)
{
	return 1;
}

int GetChList (void)
{
	return 1;
}

static int cherche_pos (int ch, int *items)
{
	int i;

	for (i = 1; i < TOT_VOIES + 1; i++)
	{
		if (items[i] == ch)
			return (i);
	}
	return (0);
}

static int insere_pos (int ch, int *items)
{
	int i;
	int pos = 0;

	for (i = 1; i < TOT_VOIES + 1; i++)
	{
		if ((items[i] > ch) || (items[i] == 0))
		{
			pos = i;
			break;
		}
	}

	for (i = TOT_VOIES; i > pos; i--)
	{
		items[i] = items[i - 1];
	}

	items[pos] = ch;

	return (pos);
}

static void delete_pos (int position, int *items)
{
	int i;

	for (i = position; i < TOT_VOIES + 1; i++)
	{
		items[i] = items[i + 1];
	}
}

void ConnectItem (int ch, char *texte)
{
	static int items[TOT_VOIES + 1];
	int pos;
	XmString str;

	if (ch <= 0)
		return;

	if (*texte == '<')
		str = XmStringGenerate (texte + 1, NULL, XmCHARSET_TEXT, "RC");
	else if (*texte == '>')
		str = XmStringGenerate (texte + 1, NULL, XmCHARSET_TEXT, "BC");
	else
		str = XmStringGenerate (texte + 1, NULL, XmCHARSET_TEXT, "NO");

	pos = cherche_pos (ch, items);

	if (texte[3])
	{
		if (pos)
		{
			/* Mise a jour */
			XmListReplaceItemsPos (ConnectList, &str, 1, pos);
		}
		else
		{
			/* Creation */
			pos = insere_pos (ch, items);
			XmListAddItemUnselected (ConnectList, str, pos);
		}
	}
	else if (pos)
	{
		/* supression */
		XmListDeletePos (ConnectList, pos);
		delete_pos (pos, items);
	}

	XmStringFree (str);

}

static Pixel CreeCouleur (Widget w, int couleur)
{
	XColor tcolor;

	tcolor.pixel = 0;
	tcolor.red = ((couleur >> 16) & 0xff) << 8;
	tcolor.green = ((couleur >> 8) & 0xff) << 8;
	tcolor.blue = (couleur & 0xff) << 8;
	tcolor.flags = DoRed | DoGreen | DoBlue;
	if (XAllocColor (XtDisplay(w), DefaultColormapOfScreen(XtScreen(w)), &tcolor))
		return (tcolor.pixel);
	return couleur;
}

static XmRendition CreeCouleurRendition (Widget w, Pixel couleur, char *nom)
{
	Arg args[10];
	Cardinal n;

	n = 0;
	XtSetArg (args[n], XmNrenditionForeground, couleur);
	n++;
	return XmRenditionCreate (w, nom, args, n);
}

void CreateRendition (Widget w)
{
	r_index = 0;

	XtVaGetValues (form, XmNbackground, &df_pixel, NULL);

	rc_pixel = CreeCouleur (w, 0xff0000);
	r_rend[r_index++] = CreeCouleurRendition (w, rc_pixel, "RC");
	vc_pixel = CreeCouleur (w, 0x00ff00);
	r_rend[r_index++] = CreeCouleurRendition (w, vc_pixel, "VC");
	bc_pixel = CreeCouleur (w, 0x0000ff);
	r_rend[r_index++] = CreeCouleurRendition (w, bc_pixel, "BC");
	no_pixel = CreeCouleur (w, 0x000000);
	r_rend[r_index++] = CreeCouleurRendition (w, no_pixel, "NO");
	rf_pixel = CreeCouleur (w, 0x800000);
	r_rend[r_index++] = CreeCouleurRendition (w, rf_pixel, "RF");
	vf_pixel = CreeCouleur (w, 0x008000);
	r_rend[r_index++] = CreeCouleurRendition (w, vf_pixel, "VF");
	bf_pixel = CreeCouleur (w, 0x000080);
	r_rend[r_index++] = CreeCouleurRendition (w, bf_pixel, "BF");
}

void ToolIcons (void)
{
	int i;
	int connected = is_connected ();
	static int OldIconState[NB_ICON];
	int IconState[NB_ICON];

	static int conn = 0;
	static int first = 1;
	static Pixel TopShadow;
	static Pixel BottomShadow;

	if (!comm_ok)
		return;

	if (first)
	{
		XtVaGetValues (BIcon[0],
					   XmNtopShadowColor, &TopShadow,
					   XmNbottomShadowColor, &BottomShadow,
					   NULL);
		XtSetSensitive (BIcon[15], TRUE);
		first = 0;
	}

	if (connected != conn)
	{
		conn = connected;
		XtSetSensitive (BIcon[0], connected);
		XtSetSensitive (BIcon[2], connected);
		XtSetSensitive (BIcon[3], connected);

		XtSetSensitive (MCons, connected);
		XtSetSensitive (MMon, connected);
		XtSetSensitive (MAllc, connected);
	}

	for (i = 0; i < NB_ICON; i++)
		IconState[i] = FALSE;

	IconState[0] = fenetre[CONSOLE];
	IconState[2] = fenetre[MONITOR];
	IconState[3] = fenetre[ALLCHANN];
	IconState[15] = TRUE;

	for (i = 0; i < NB_ICON; i++)
	{
		if (BIcon[i] == NULL)
			continue;

		if (OldIconState[i] != IconState[i])
		{
			if (i < 4)
			{
				if (IconState[i])
				{
					XtVaSetValues (BIcon[i],
								   XmNtopShadowColor, BottomShadow,
								   XmNbottomShadowColor, TopShadow,
								   NULL);
				}
				else
				{
					XtVaSetValues (BIcon[i],
								   XmNtopShadowColor, TopShadow,
								   XmNbottomShadowColor, BottomShadow,
								   NULL);
				}
			}

			OldIconState[i] = IconState[i];
		}
	}
}

void Caption (int update)
{
	time_t temps;
	struct tm tmg;
	struct tm tml;
	char buf[80];
	char conn[80];
	static int lmin = -1;

	if (update)
		lmin = -1;

	temps = time (NULL);
	tmg = *(gmtime (&temps));
	tml = *(localtime (&temps));
	if (tmg.tm_min != lmin)
	{
		if (is_connected ())
			sprintf (conn, "%s (%s:%d)", conf[curconf].name, conf[curconf].host, conf[curconf].port);
		else
			sprintf (conn, "%s : not connected", conf[curconf].name);
		lmin = tmg.tm_min;
		sprintf (buf, "XFBB - %s - %s - %02d:%02d UTC",
				 conf[curconf].mycall, conn,
				 tmg.tm_hour, tmg.tm_min);
		XtVaSetValues (toplevel, XmNtitle, buf, NULL);
	}
}


void ItemCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void ActiveListCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	CurrentSelection = GetChList ();
}

void SelectListCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	CurrentSelection = GetChList ();
	if (CurrentSelection != -1)
	{
		/* Selection d'un canal */
		/* ShowFbbWindow(CurrentSelection, toplevel); */
	}
}

void ConsoleCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	char buffer[3];

	/* Connexion en console */
	cursor_wait ();

	/* Demande de connexion */
	if (conf[curconf].mask & FBB_CONSOLE)
		conf[curconf].mask &= ~FBB_CONSOLE;
	else
		conf[curconf].mask |= FBB_CONSOLE;
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = (char) conf[curconf].mask;
	write (fbb_sock, buffer, 3);
}

void GatewayCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	/* ShowFbbWindow(CONSOLE, toplevel); */
}

void AllChanCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	char buffer[3];

	if (!fenetre[ALLCHANN])
	{
		conf[curconf].mask |= FBB_CHANNEL;
		ShowFbbWindow (ALLCHANN, toplevel);
	}
	else
	{
		conf[curconf].mask &= ~FBB_CHANNEL;
		HideFbbWindow (ALLCHANN, toplevel);
	}
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = (char) conf[curconf].mask;
	write (fbb_sock, buffer, 3);
}

void OneChanCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void DisconnectCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void DisconnectConsole (void)
{
	HideFbbWindow (CONSOLE, toplevel);
}

void MonitorCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	char buffer[3];

	if (!fenetre[MONITOR])
	{
		conf[curconf].mask |= FBB_MONITOR;
		ShowFbbWindow (MONITOR, toplevel);
	}
	else
	{
		conf[curconf].mask &= ~FBB_MONITOR;
		HideFbbWindow (MONITOR, toplevel);
	}
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = (char) conf[curconf].mask;
	write (fbb_sock, buffer, 3);
}

void ScanSysCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	cursor_wait ();
	end_wait ();
}

void ScanMsgCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	printf ("ScanMsg = %d\n",
			((XmToggleButtonCallbackStruct *) call_data)->set);
}

void EditFileCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void ListCnxCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void EditUsrCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void EditMsgCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void PendingCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void StatMemCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Infos status */
	int flag = (((XmToggleButtonCallbackStruct *) call_data)->set != 0);
	char buffer[4];

	if (flag)
	{
		conf[curconf].mask |= FBB_STATUS;
		XtMapWidget (TxtUsed);
		XtMapWidget (TxtGMem);
		XtMapWidget (TxtDisk1);
		XtMapWidget (TxtDisk2);
		XtMapWidget (Used);
		XtMapWidget (GMem);
		XtMapWidget (Disk1);
		XtMapWidget (Disk2);
	}
	else
	{
		conf[curconf].mask &= ~FBB_STATUS;
		XtUnmapWidget (TxtUsed);
		XtUnmapWidget (TxtGMem);
		XtUnmapWidget (TxtDisk1);
		XtUnmapWidget (TxtDisk2);
		XtUnmapWidget (Used);
		XtUnmapWidget (GMem);
		XtUnmapWidget (Disk1);
		XtUnmapWidget (Disk2);
	}
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = (char) conf[curconf].mask;
	write (fbb_sock, buffer, 3);

	ReceiveStatus (0, 0, 0, 0);
	PutConfig ();
}

void StatMsgCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	/* Infos Messages */
	int flag = (((XmToggleButtonCallbackStruct *) call_data)->set != 0);
	char buffer[4];

	if (flag)
	{
		conf[curconf].mask |= FBB_MSGS;

		XtMapWidget (TxtMsgs);
		XtMapWidget (TxtHold);
		XtMapWidget (TxtPriv);
		XtMapWidget (Msgs);
		XtMapWidget (Hold);
		XtMapWidget (Priv);
	}
	else
	{
		conf[curconf].mask &= ~FBB_MSGS;

		XtUnmapWidget (TxtMsgs);
		XtUnmapWidget (TxtHold);
		XtUnmapWidget (TxtPriv);
		XtUnmapWidget (Msgs);
		XtUnmapWidget (Hold);
		XtUnmapWidget (Priv);
	}
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = (char) conf[curconf].mask;
	write (fbb_sock, buffer, 3);

	ReceiveNbMsg (0, 0, 0);
	PutConfig ();
}

void InfoDialog (Widget w, XtPointer client_data, XtPointer call_data)
{
}


void RemoteCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	char msg[256];
	int val = (uintptr_t) client_data;
	int flag = (((XmToggleButtonCallbackStruct *) call_data)->set != 0);

	if (val == curconf)
		return;

	if (flag == 0)
		return;

	curconf = (uintptr_t) client_data;
	PutConfig ();
	init_orb (msg);
}

void TalkToCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void MaintCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void RerunCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void ExitCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	exit (0);
}

void StopCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void ConnectCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

void PrintCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	prints = !prints;

	printf ("print = %d\n", prints);
}

void LabelSetString (Widget label, char *text, char *attr)
{
	Arg args[10];
	Cardinal n = 0;
	XmString string;

	if (text == NULL)
		return;

	if (attr)
		string = XmStringGenerate (text, NULL, XmCHARSET_TEXT, attr);
	else
		string = XmStringCreateSimple (text);
	n = 0;
	XtSetArg (args[n], XmNlabelString, string);
	n++;
	XtSetValues (label, args, n);
	XmStringFree (string);
}

void FHelpCB (Widget w, XtPointer client_data, XEvent * event)
{
	static XmString prev = NULL;

	if (client_data)
	{
		XmString string;

		if (prev)
			XmStringFree (prev);
		XtVaGetValues (Footer, XmNlabelString, &prev, NULL);

		string = XmStringCreateSimple ((char *) client_data);
		XtVaSetValues (Footer, XmNlabelString, string, NULL);
		XmStringFree (string);
		foothelp = TRUE;
	}
	else
	{
		if (prev)
			XtVaSetValues (Footer, XmNlabelString, prev, NULL);
		foothelp = FALSE;
	}
}

Widget add_item (int type, Widget menu_pane, char *nom,
				 XtCallbackProc callback, XtPointer data,
				 char *help, XtEventHandler HelpCB)
{
	Arg args[10];
	Cardinal n = 0;
	Widget bouton = NULL;

	if ((callback == NULL) && (type != SEPARATOR) && (type != TOOLBUTTON))
	{
		XtSetArg (args[n], XmNsensitive, False);
		n++;
	}

	switch (type)
	{
	case SEPARATOR:
		bouton = XmCreateSeparator (menu_pane, nom, args, n);
		break;
	case BUTTON:
		bouton = XmCreatePushButton (menu_pane, nom, args, n);
		if (callback)
			XtAddCallback (bouton, XmNactivateCallback, callback, data);
		break;
	case TOGGLE:
		XtSetArg (args[n], XmNvisibleWhenOff, True);
		n++;
		bouton = XmCreateToggleButton (menu_pane, nom, args, n);
		if (callback)
			XtAddCallback (bouton, XmNvalueChangedCallback, callback, data);
		break;
	case RADIO:
		XtSetArg (args[n], XmNvisibleWhenOff, False);
		n++;
		bouton = XmCreateToggleButton (menu_pane, nom, args, n);
		if (callback)
			XtAddCallback (bouton, XmNvalueChangedCallback, callback, data);
		break;
	case TOOLBUTTON:
		if (nom[0] != ' ')
		{
			XtSetArg (args[n], XmNlabelType, XmPIXMAP);
			n++;
		}
		XtSetArg (args[n], XmNhighlightThickness, 0);
		n++;
		XtSetArg (args[n], XmNsensitive, FALSE);
		n++;
		bouton = XmCreatePushButton (menu_pane, nom, args, n);
		if (callback)
			XtAddCallback (bouton, XmNactivateCallback, callback, data);
		break;
	}

	if (help)
	{
		XtAddEventHandler (bouton, EnterWindowMask, FALSE,
						   HelpCB, (XtPointer) help);
		XtAddEventHandler (bouton, LeaveWindowMask, FALSE,
						   HelpCB, NULL);
	}
	XtManageChild (bouton);
	return (bouton);
}

void AddRT (Widget w)
{
	XmRenderTable rt;

	XtVaGetValues (w, XmNrenderTable, &rt, NULL, NULL);

	/* Make a copy so that setvalues will work correctly */
	rt = XmRenderTableCopy (rt, NULL, 0);
	rt = XmRenderTableAddRenditions (rt, r_rend, r_index, XmMERGE_NEW);
	XtVaSetValues (w, XmNrenderTable, rt, NULL, NULL);
	XmRenderTableFree (rt);
}

/* Translations */
static void HelpAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	AboutDialog (w, NULL, NULL);
}

static void ConsoleAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	ConsoleCB (w, NULL, NULL);
}

static void DisconnAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	DisconnectCB (w, NULL, NULL);
}

static void PendingAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
}

static void MonitorAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	MonitorCB (w, NULL, NULL);
}

static void SetCallAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	CallsignDialog (w, NULL, NULL);
}

static void ProgTncAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
}

static void GatewayAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	GatewayCB (w, NULL, NULL);
}

static void TalkToAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	TalkToCB (w, NULL, NULL);
}

static void MsgScanAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	TalkToCB (w, NULL, NULL);
}

static void EditorAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
}

static void ListCnxAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
}

static void SndTextAct (Widget w, XEvent * event, String * parms, Cardinal * num)
{
	printf ("SndText <%s>\n", parms[0]);
}

Boolean KernelWorkProc (XtPointer data)
{
	ToolIcons ();
	Caption (0);
	usleep (10);
	return (FALSE);
}

int main (int ac, char **av)
{
	Pixmap pixmap;
	Widget bouton;
	Arg args[20];
	Cardinal n;
	XmString string;
	int i;
	Atom DelWindow;
	Widget FormRC;
	char msg[256];
	int res;

	CurrentSelection = -1;
	p_fptr = NULL;

	for (i = 1; i < NSIG; i++)
	{
		signal (i, sig_fct);
	}


	XtToolkitInitialize ();

	app_context = XtCreateApplicationContext ();

	display = XtOpenDisplay (app_context, NULL, av[0], "xfbbX",
							 NULL, 0, &ac, av);
	if (display == NULL)
	{
		XtWarning ("xfbb : cannot open display, exiting...");
		exit (0);
	}

	XtAppAddActions (app_context, actionsTable, XtNumber (actionsTable));

	toplevel = XtAppCreateShell (av[0], "xfbbX",
								 applicationShellWidgetClass,
								 display, NULL, 0);

	fenetre[CONSOLE] = fenetre[MONITOR] = fenetre[ALLCHANN] = 0;

	n = 0;
	XtSetArg (args[n], XmNallowShellResize, True);
	n++;
	XtSetArg (args[n], XmNdeleteResponse, XmDO_NOTHING);
	++n;
	XtSetValues (toplevel, args, n);

	DelWindow = XInternAtom (XtDisplay (toplevel), "WM_DELETE_WINDOW", FALSE);
	XmAddWMProtocolCallback (toplevel, DelWindow, ExitCB, NULL);

	n = 0;
	XtSetArg (args[n], XmNmarginHeight, 5);
	n++;
	XtSetArg (args[n], XmNmarginWidth, 5);
	n++;
	form = XmCreateForm (toplevel, "form", args, n);

	CreateRendition (toplevel);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	MenuBar = XmCreateMenuBar (form, "menu_bar", args, n);
	XtManageChild (MenuBar);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNtopWidget, MenuBar);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNorientation, XmHORIZONTAL);
	n++;

	ToolBar = XmCreateRowColumn (form, "tool_bar", args, n);

	/* Initialisation des boutons du menubar */
	bouton = 0;

	n = 0;
	XtSetArg (args[0], XmNradioBehavior, True);
	MenuFile = XmCreatePulldownMenu (MenuBar, "file", args, 0);
	MenuWindow = XmCreatePulldownMenu (MenuBar, "window", args, 0);
	MenuRemote = XmCreatePulldownMenu (MenuBar, "remote", args, 1);
	MenuConfig = XmCreatePulldownMenu (MenuBar, "config", args, 0);
	MenuHelp = XmCreatePulldownMenu (MenuBar, "help", args, 0);

	XtSetArg (args[0], XmNsubMenuId, MenuFile);
	bouton = XmCreateCascadeButton (MenuBar, "file", args, 1);
	XtManageChild (bouton);

	XtSetArg (args[0], XmNsubMenuId, MenuWindow);
	bouton = XmCreateCascadeButton (MenuBar, "window", args, 1);
	XtManageChild (bouton);

	XtSetArg (args[0], XmNsubMenuId, MenuRemote);
	bouton = XmCreateCascadeButton (MenuBar, "remote", args, 1);
	XtManageChild (bouton);

	XtSetArg (args[0], XmNsubMenuId, MenuConfig);
	bouton = XmCreateCascadeButton (MenuBar, "config", args, 1);
	XtManageChild (bouton);

	XtSetArg (args[0], XmNsubMenuId, MenuHelp);
	bouton = XmCreateCascadeButton (MenuBar, "help", args, 1);
	XtManageChild (bouton);

	/* Bouton help a droite */
	XtVaSetValues (MenuBar, XmNmenuHelpWidget, bouton, NULL);

	XtManageChild (MenuFile);
	XtManageChild (MenuWindow);
	XtManageChild (MenuRemote);
	XtManageChild (MenuConfig);

	add_item (BUTTON, MenuFile, "exit", ExitCB, NULL,
			  "End of the session", (XtEventHandler) FHelpCB);
	MCons = add_item (BUTTON, MenuWindow, "console", ConsoleCB, NULL,
					  "Console connection", (XtEventHandler) FHelpCB);
	MMon = add_item (BUTTON, MenuWindow, "monitoring", MonitorCB, NULL,
					 "Shows monitoring", (XtEventHandler) FHelpCB);
	MAllc = add_item (BUTTON, MenuWindow, "all_channels", AllChanCB, NULL,
				"Shows the traffic of all users", (XtEventHandler) FHelpCB);
	for (i = 0; i < MAX_CONF; i++)
	{
		char name[20];

		sprintf (name, "xfbb %d", i + 1);
		Rmt[i] = add_item (RADIO, MenuRemote, name,
						   RemoteCB, (XtPointer)(intptr_t) i,
						"Select remote xfbb BBS", (XtEventHandler) FHelpCB);
	}

	add_item (BUTTON, MenuConfig, "main_parameters", SetupDialog, NULL,
			"Main configuration of the software", (XtEventHandler) FHelpCB);
	add_item (BUTTON, MenuHelp, "copyright", CopyDialog, NULL,
			  "Displays copyright informations", (XtEventHandler) FHelpCB);
	add_item (SEPARATOR, MenuHelp, "", NULL, NULL, NULL, NULL);
	add_item (BUTTON, MenuHelp, "about", AboutDialog, NULL,
			  "Informations on WinFBB software", (XtEventHandler) FHelpCB);

	BIcon[0] = add_item (TOOLBUTTON, ToolBar, "B1", ConsoleCB, NULL,
						 "Console connection", (XtEventHandler) FHelpCB);
	BIcon[2] = add_item (TOOLBUTTON, ToolBar, "B3", MonitorCB, NULL,
						 "Monitoring window", (XtEventHandler) FHelpCB);
	BIcon[3] = add_item (TOOLBUTTON, ToolBar, "B4", AllChanCB, NULL,
				"Shows the traffic of all users", (XtEventHandler) FHelpCB);
	add_item (TOOLBUTTON, ToolBar, "                                                    ", NULL, NULL, 
				" ", NULL);
	BIcon[15] = add_item (TOOLBUTTON, ToolBar, "B16", AboutDialog, NULL,
						  "On-line help", (XtEventHandler) FHelpCB);

	XtVaSetValues (ToolBar, XmNmenuHelpWidget, BIcon[15], NULL);

	string = XmStringCreateSimple ("  ");
	n = 0;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNlabelString, string);
	n++;
	Footer = XmCreateLabel (form, "footer", args, n);
	XtManageChild (Footer);
	XmStringFree (string);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNtopWidget, ToolBar);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	ConnectLabel = XmCreateLabel (form, "list_label", args, n);
	AddRT (ConnectLabel);
	XtManageChild (ConnectLabel);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNtopWidget, ConnectLabel);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNbottomWidget, Footer);
	n++;
	StatForm = XmCreateForm (form, "stat_form", args, n);
	XtManageChild (StatForm);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNtopWidget, ConnectLabel);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNrightWidget, StatForm);
	n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNbottomWidget, Footer);
	n++;
	ListForm = XmCreateForm (form, "list_form", args, n);
	XtManageChild (ListForm);

	string = XmStringCreateSimple ("Ch Callsign  Start Time  Rt Buf C/Fwd");
	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNlabelString, string);
	n++;
	ConnectString = XmCreateLabel (ListForm, "list_label", args, n);
	XmStringFree (string);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNtopWidget, ConnectString);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNhighlightThickness, 0);
	n++;
	XtSetArg (args[n], XmNscrollBarDisplayPolicy, XmSTATIC);
	n++;
	ConnectList = XmCreateScrolledList (ListForm, "connect_list", args, n);
	XtAddCallback (ConnectList, XmNdefaultActionCallback, SelectListCB, NULL);
	XtAddCallback (ConnectList, XmNbrowseSelectionCallback, ActiveListCB, NULL);
	AddRT (ConnectList);

	XtManageChild (ConnectLabel);
	XtManageChild (ConnectString);
	XtManageChild (ConnectList);

	Popup = XmCreatePopupMenu (ConnectList, "popup", NULL, 0);

	PItem[0] = XmCreatePushButtonGadget (Popup, "Talk", NULL, 0);
	PItem[1] = XmCreatePushButtonGadget (Popup, "Show", NULL, 0);
	PItem[2] = XmCreatePushButtonGadget (Popup, "Infos", NULL, 0);
	PItem[3] = XmCreatePushButtonGadget (Popup, "Disconnect", NULL, 0);
	for (i = 0; i < NB_PITEM; i++)
		XtAddCallback (PItem[i], XmNactivateCallback, ItemCB, (XtPointer)(intptr_t) i);
	XtManageChildren (PItem, NB_PITEM);

	n = 0;
	XtSetArg (args[n], XmNborderWidth, 0);
	n++;
	XtSetArg (args[n], XmNshadowThickness, 0);
	n++;
	XtSetArg (args[n], XmNhighlightThickness, 0);
	n++;
	XtSetArg (args[n], XmNmarginHeight, 1);
	n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNindicatorOn, XmINDICATOR_CHECK_BOX);
	n++;
	XtSetArg (args[n], XmNvisibleWhenOff, True);
	n++;
	StatusToggle = XmCreateToggleButton (StatForm, "status_toggle", args, n);
	XtAddCallback (StatusToggle, XmNvalueChangedCallback, StatMemCB, NULL);
	XtManageChild (StatusToggle);

	n = 0;
	XtSetArg (args[n], XmNborderWidth, 0);
	n++;
	XtSetArg (args[n], XmNshadowThickness, 0);
	n++;
	XtSetArg (args[n], XmNhighlightThickness, 0);
	n++;
	XtSetArg (args[n], XmNmarginHeight, 1);
	n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNleftWidget, StatusToggle);
	n++;
	XtSetArg (args[n], XmNindicatorOn, XmINDICATOR_CHECK_BOX);
	n++;
	XtSetArg (args[n], XmNvisibleWhenOff, True);
	n++;
	MsgsToggle = XmCreateToggleButton (StatForm, "message_toggle", args, n);
	XtAddCallback (MsgsToggle, XmNvalueChangedCallback, StatMsgCB, NULL);
	XtManageChild (MsgsToggle);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNtopWidget, StatusToggle);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftOffset, 2);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightOffset, 2);
	n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNshadowThickness, 1);
	n++;
	XtSetArg (args[n], XmNshadowType, XmSHADOW_IN);
	n++;
	FormRC = XmCreateForm (StatForm, "stat_Flist", args, n);
	XtManageChild (FormRC);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNpacking, XmPACK_COLUMN);
	n++;
	XtSetArg (args[n], XmNnumColumns, 2);
	n++;
	StatList = XmCreateRowColumn (FormRC, "stat_list", args, n);
	XtManageChild (StatList);

	n = 0;
	XtSetArg (args[n], XmNmarginHeight, 1);
	n++;
	XtSetArg (args[n], XmNmappedWhenManaged, 0);
	n++;

	TxtUsed = XmCreateLabel (StatList, "txt_used", args, n);
	AddRT (TxtUsed);
	XtManageChild (TxtUsed);

	TxtGMem = XmCreateLabel (StatList, "txt_gmem", args, n);
	AddRT (TxtGMem);
	XtManageChild (TxtGMem);

	TxtDisk1 = XmCreateLabel (StatList, "txt_disk1", args, n);
	AddRT (TxtDisk1);
	XtManageChild (TxtDisk1);

	TxtDisk2 = XmCreateLabel (StatList, "txt_disk2", args, n);
	AddRT (TxtDisk2);
	XtManageChild (TxtDisk2);

	TxtResync = XmCreateLabel (StatList, "", args, n);
	AddRT (TxtResync);
	XtManageChild (TxtResync);

	TxtState = XmCreateLabel (StatList, "", args, n);
	AddRT (TxtState);
	XtManageChild (TxtState);

	TxtMsgs = XmCreateLabel (StatList, "txt_msgs", args, n);
	AddRT (TxtMsgs);
	XtManageChild (TxtMsgs);

	TxtHold = XmCreateLabel (StatList, "txt_hold", args, n);
	AddRT (TxtHold);
	XtManageChild (TxtHold);

	TxtPriv = XmCreateLabel (StatList, "txt_priv", args, n);
	AddRT (TxtPriv);
	XtManageChild (TxtPriv);

	Used = XmCreateLabel (StatList, "used", args, n);
	AddRT (Used);
	XtManageChild (Used);

	GMem = XmCreateLabel (StatList, "gmem", args, n);
	AddRT (GMem);
	XtManageChild (GMem);

	Disk1 = XmCreateLabel (StatList, "disk1", args, n);
	AddRT (Disk1);
	XtManageChild (Disk1);

	Disk2 = XmCreateLabel (StatList, "disk2", args, n);
	AddRT (Disk2);
	XtManageChild (Disk2);

	Resync = XmCreateLabel (StatList, "", args, n);
	AddRT (Resync);
	XtManageChild (Resync);

	State = XmCreateLabel (StatList, "", args, n);
	AddRT (State);
	XtManageChild (State);

	Msgs = XmCreateLabel (StatList, "msgs", args, n);
	AddRT (Msgs);
	XtManageChild (Msgs);

	Hold = XmCreateLabel (StatList, "hold", args, n);
	AddRT (Hold);
	XtManageChild (Hold);

	Priv = XmCreateLabel (StatList, "private", args, n);
	AddRT (Priv);
	XtManageChild (Priv);

	XtManageChild (ToolBar);
	XtManageChild (form);

	/* Icone de la fenetre */
	pixmap = XCreateBitmapFromData (XtDisplay (toplevel), XtScreen (toplevel)->root,
									fbb_bits, fbb_width, fbb_height);
	XtVaSetValues (toplevel, XmNiconPixmap, pixmap, NULL);

	XtRealizeWidget (toplevel);

	XtSetSensitive (MCons, FALSE);
	XtSetSensitive (MMon, FALSE);
	XtSetSensitive (MAllc, FALSE);

	window_init ();
	set_win_colors ();

	curconf = 0;
	if (!GetConfig ())
		SetupDialog (NULL, NULL, NULL);

	else
	{
		res = init_orb (msg);
		if (res == 0)
		{
			MessageBox (0, msg, "Client connection",
						MB_OK | MB_ICONEXCLAMATION);
			comm_ok = 0;
			/* return (res); */
		}
	}

	/* Initialisation sequences */
	XtAppAddWorkProc (app_context, KernelWorkProc, NULL);

	/* Main Loop */
	XtAppMainLoop (app_context);

	/* Never reached */
	return 0;
}

char *itoa (int val, char *buffer, int base)
{
	sprintf (buffer, "%d", val);
	return buffer;
}

char *ltoa (long lval, char *buffer, int base)
{
	sprintf (buffer, "%ld", lval);
	return buffer;
}


#define PROTOTYPES 1
#include "global.h"
#include "md5.h"

void MD5String (uchar *dest, uchar *source)
{
	int i;
	MD5_CTX context;
	uchar digest[16];
	unsigned int len = strlen (source);

	MD5Init (&context);
	MD5Update (&context, source, len);
	MD5Final (digest, &context);

	*dest = '\0';

	for (i = 0; i < 16; i++)
	{
		char tmp[5];

		sprintf (tmp, "%02X", digest[i]);
		strcat (dest, tmp);
	}
}

static void makekey (char *cle, char *pass, char *buffer)
{
	char source[1024];

	strcpy (source, cle);
	strcat (source, pass);
	MD5String (buffer, source);
}
