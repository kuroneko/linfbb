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


#include <stdlib.h>
#include <xfbbX.h>
#include <unistd.h>
#include <ctype.h>

#include <config.h>

static Widget about_dialog = NULL;
static Widget copy_dialog = NULL;
static Widget call_dialog = NULL;

static XmString StringCreate (char *text)
{
	XmString xmstr;
	char *deb;

	if ((text == NULL) || (text[0] == '\0'))
	{
		return (XmStringCreateSimple (""));
	}

	xmstr = (XmString) NULL;

	deb = text;
	while (*text)
	{
		if (*text == '\n')
		{
			*text = '\0';
			xmstr = XmStringConcat (xmstr, XmStringCreateSimple (deb));
			xmstr = XmStringConcat (xmstr, XmStringSeparatorCreate ());
			*text++ = '\n';
			deb = text;
		}
		else
			++text;
	}

	if (*deb)
	{
		xmstr = XmStringConcat (xmstr, XmStringCreateSimple (deb));
	}
	return (xmstr);
}

char *date (void)
{
	return (__DATE__);
}

char *version (void) 
{
	static char buffer[20];

	sprintf (buffer, "%s", VERSION);
	
	return (buffer);
}

static char *XVersion (int dat)
{
	static char prodVersion[80];
	char sdate[30];

	if (dat)
		sprintf (sdate, " (%s)", date ());
	else
		*sdate = '\0';

	sprintf (prodVersion, "%s%s", version (), sdate);

	return (prodVersion);
}

static void CancelCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget *pw;

	pw = (Widget *) client_data;

	XtUnmanageChild (*pw);
	XtDestroyWidget (XtParent (*pw));
	*pw = NULL;;
}

static int is_call (char *call)
{
	int nb;
	char *trait;
	char *ptr;

	trait = strchr (call, '-');
	if (trait)
		*trait = '\0';

	nb = 0;
	ptr = call;
	while (*ptr)
	{
		if (!isalnum (*ptr))
			return (0);
		++ptr;
		++nb;
	}
	if ((nb < 4) || (nb > 6))
		return (0);

	if (trait == NULL)
		return (1);

	*trait = '-';

	nb = 0;
	ptr = trait + 1;
	while (*ptr)
	{
		if (!isdigit (*ptr))
			return (0);
		++ptr;
	}
	nb = atoi (trait + 1);
	if ((nb < 0) || (nb > 15))
		return (0);

	return (1);
}

static void OkCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget *pw;
	Widget wt;
	char *ptr;

	pw = (Widget *) client_data;

	wt = XmSelectionBoxGetChild (*pw, XmDIALOG_TEXT);

	ptr = XmTextGetString (wt);

	if (!is_call (ptr))
	{
		char texte[80];

		sprintf (texte, "%s is not a valid callsign !", ptr);
		MessageBox (60, texte, "Change callsign", MB_ICONEXCLAMATION | MB_OK);
	}
	else
	{
		strcpy (conf[curconf].mycall, ptr);
		XtUnmanageChild (*pw);
		XtDestroyWidget (XtParent (*pw));
		*pw = NULL;;
		Caption (1);
	}
	XtFree (ptr);
}

void CallsignDialog (Widget w, XtPointer client_data, XtPointer call_data)
{
	Arg args[20];
	Cardinal n;

	printf ("calldialog\n");
	if (call_dialog)
	{
		/* Dialog deja ouvert, on le passe devant */
		XRaiseWindow (XtDisplay (call_dialog), XtWindow (XtParent (call_dialog)));
		printf ("calldialog up\n");
		return;
	}

	n = 0;
	XtSetArg (args[n], XmNautoUnmanage, FALSE);
	n++;
	XtSetArg (args[n], XmNmessageAlignment, XmALIGNMENT_CENTER);
	n++;
	call_dialog = XmCreatePromptDialog (toplevel, "callsign", args, n);

	XtUnmanageChild (XmSelectionBoxGetChild (call_dialog, XmDIALOG_HELP_BUTTON));
	XtAddCallback (call_dialog, XmNokCallback, OkCB, (XtPointer) & call_dialog);
	XtAddCallback (call_dialog, XmNcancelCallback, CancelCB, (XtPointer) & call_dialog);

	/* get_callsign(buffer); */
	XmTextSetString (XmSelectionBoxGetChild (call_dialog, XmDIALOG_TEXT), conf[curconf].mycall);
	XtManageChild (call_dialog);
}

typedef struct
{
	Widget wdial;
	Widget wname;
	Widget whost;
	Widget wport;
	Widget wcall;
	Widget wpass;
}
wid_t;

static void S_CancelCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	wid_t *pw;

	pw = (wid_t *) client_data;

	XtUnmanageChild (pw->wdial);
	XtDestroyWidget (XtParent (pw->wdial));
	pw->wdial = NULL;
}

int GetConfig (void)
{
	int i;
	int ret = 0;
	char *home = getenv ("HOME");
	char filename[256];

	memset(conf, 0, sizeof(conf));
	for (i = 0 ; i < MAX_CONF ; i++)
	{
		sprintf(conf[i].name, "Remote %d", i);
		strcpy(conf[i].host, "localhost");
		conf[i].port = 3286;
		conf[i].mask = FBB_NBCNX | FBB_LISTCNX | FBB_XFBBX;
	}
	
	if (home)
	{
		FILE *fptr;
		char buf[256];

		sprintf (filename, "%s/.xfbbX", home);
		fptr = fopen (filename, "r");
		if (fptr)
		{
			for (i = 0 ; i < MAX_CONF ; i++)
			{
				if (fgets (buf, sizeof(buf), fptr) != NULL)
				sscanf (buf, "%*s %[^\n]", conf[i].name);
				if (fgets (buf, sizeof(buf), fptr) != NULL)
				sscanf (buf, "%*s %s\n", conf[i].host);
				if (fgets (buf, sizeof(buf), fptr) != NULL)
				sscanf (buf, "%*s %d\n", &conf[i].port);
				if (fgets (buf, sizeof(buf), fptr) != NULL)
				sscanf (buf, "%*s %d\n", &conf[i].mask);
				if (fgets (buf, sizeof(buf), fptr) != NULL)
				sscanf (buf, "%*s %[^\n]", conf[i].pass);
				if (fgets (buf, sizeof(buf), fptr) != NULL)
				sscanf (buf, "%*s %s\n", conf[i].mycall);
				LabelSetString (Rmt[i], conf[i].name, NULL);
			}
			if (fscanf (fptr, "%*s %d\n", &curconf) != EOF)
			if (curconf >= MAX_CONF)
				curconf = 0;
				
			fclose (fptr);
			ret = 1;
		}
		else {
			sprintf (buf, "Cannot read configuration file '%s' !", filename);
			MessageBox (60, buf, "Configuration", MB_ICONEXCLAMATION | MB_OK);
		}
	}

	for (i = 0 ; i < MAX_CONF ; i++)
		LabelSetString (Rmt[i], conf[i].name, NULL);

	/* Set the current config */
	XmToggleButtonSetState(Rmt[curconf], True, False);

	return ret;
}

int PutConfig (void)
{
	int i;
	char *home = getenv ("HOME");

	if (home)
	{
		FILE *fptr;
		char buf[256], filename[256];

		sprintf (filename, "%s/.xfbbX", home);
		fptr = fopen (filename, "w");
		if (fptr)
		{
			for (i = 0 ; i < MAX_CONF ; i++)
			{
				int mask = conf[i].mask;
				
				mask &= ~(FBB_CONSOLE);
				
				fprintf (fptr, "name_%d: %s\n", i, conf[i].name);
				fprintf (fptr, "host_%d: %s\n", i, conf[i].host);
				fprintf (fptr, "port_%d: %d\n", i, conf[i].port);
				fprintf (fptr, "mask_%d: %d\n", i, mask);
				fprintf (fptr, "pass_%d: %s\n", i, conf[i].pass);
				fprintf (fptr, "call_%d: %s\n", i, conf[i].mycall);
			}
			fprintf (fptr, "curr_c: %d\n", curconf);
			fclose (fptr);
			return 1;
		}
		else {
			sprintf (buf, "Cannot write configuration file '%s' !", filename);
			MessageBox (60, buf, "Configuration", MB_ICONEXCLAMATION | MB_OK);
		}
	}
	return 0;
}

static void S_OkCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	wid_t *pw;
	char *name;
	char *host;
	char *port;
	char *pass;
	char *call;
	char msg[256];

	  pw = (wid_t *) client_data;

	/* confname */
	name = XmTextFieldGetString (pw->wname);

	/* hostname */
	host = XmTextFieldGetString (pw->whost);

	/* port */
	port = XmTextFieldGetString (pw->wport);

	/* password */
	pass = XmTextFieldGetString (pw->wpass);

	/* callsign */
	call = XmTextFieldGetString (pw->wcall);
	
	if (!is_call (call))
	{
		char texte[80];

		sprintf (texte, "%s is not a valid callsign !", call);
		MessageBox (60, texte, "Change callsign", MB_ICONEXCLAMATION | MB_OK);
	}
	else
	{
		strcpy (conf[curconf].name, name);
		strcpy (conf[curconf].host, host);
		strcpy (conf[curconf].pass, pass);
		strcpy (conf[curconf].mycall, call);
		conf[curconf].port = atoi (port);

		LabelSetString (Rmt[curconf], conf[curconf].name, NULL);

		PutConfig ();

		XtUnmanageChild (pw->wdial);
		XtDestroyWidget (XtParent (pw->wdial));
		pw->wdial = NULL;

		close_connection ();
		sleep(2);
		if (!init_orb (msg))
		{
			MessageBox (0, msg, "Client connection", MB_OK | MB_ICONEXCLAMATION);
		}
	}
}

void SetupDialog (Widget w, XtPointer client_data, XtPointer call_data)
{
	Arg args[20];
	Cardinal n;
	Widget work_area;
	int conf_ok;

	static wid_t wid =
	{NULL, NULL, NULL, NULL, NULL};

	if (wid.wdial)
	{
		/* Dialog deja ouvert, on le passe devant */
		XRaiseWindow (XtDisplay (wid.wdial), XtWindow (XtParent (wid.wdial)));
		return;
	}

	conf_ok = GetConfig ();

	n = 0;
	XtSetArg (args[n], XmNautoUnmanage, FALSE);
	n++;
	XtSetArg (args[n], XmNmessageAlignment, XmALIGNMENT_CENTER);
	n++;
	wid.wdial = XmCreatePromptDialog (toplevel, "setup", args, n);

	XtUnmanageChild (XmSelectionBoxGetChild (wid.wdial, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmSelectionBoxGetChild (wid.wdial, XmDIALOG_TEXT));
	XtUnmanageChild (XmSelectionBoxGetChild (wid.wdial, XmDIALOG_SELECTION_LABEL));

	XtAddCallback (wid.wdial, XmNokCallback, S_OkCB, (XtPointer) & wid);
	XtAddCallback (wid.wdial, XmNcancelCallback, S_CancelCB, (XtPointer) & wid);

	n = 0;
	work_area = XmCreateWorkArea (wid.wdial, "workarea", args, n);

	n = 0;
	XtManageChild (XmCreateLabel (work_area, "confname", NULL, 0));
	wid.wname = XmCreateTextField (work_area, "txt_host", NULL, 0);
		XmTextFieldSetString (wid.wname, conf[curconf].name);
	XtManageChild (wid.wname);

	XtManageChild (XmCreateLabel (work_area, "hostname", NULL, 0));
	wid.whost = XmCreateTextField (work_area, "txt_host", NULL, 0);
		XmTextFieldSetString (wid.whost, conf[curconf].host);
	XtManageChild (wid.whost);

	XtManageChild (XmCreateLabel (work_area, "portnb", NULL, 0));
	wid.wport = XmCreateTextField (work_area, "txt_port", NULL, 0);
	{
		char txt[80];

		sprintf (txt, "%d", conf[curconf].port);
		XmTextFieldSetString (wid.wport, txt);
	}
	XtManageChild (wid.wport);

	XtManageChild (XmCreateLabel (work_area, "callsign", NULL, 0));
	wid.wcall = XmCreateTextField (work_area, "txt_call", NULL, 0);
		XmTextFieldSetString (wid.wcall, conf[curconf].mycall);
	XtManageChild (wid.wcall);

	XtManageChild (XmCreateLabel (work_area, "password", NULL, 0));
	wid.wpass = XmCreateTextField (work_area, "txt_pass", NULL, 0);
		XmTextFieldSetString (wid.wpass, conf[curconf].pass);
	XtManageChild (wid.wpass);

	XtManageChild (work_area);
	XtManageChild (wid.wdial);
}

void AboutDialog (Widget w, XtPointer client_data, XtPointer call_data)
{
	char buffer[512];
	Arg args[20];
	Cardinal n;
	XmString string;

	if (about_dialog)
	{
		/* Dialog deja ouvert, on le passe devant */
		XRaiseWindow (XtDisplay (about_dialog), XtWindow (XtParent (about_dialog)));
		return;
	}

	sprintf (buffer,
			 "xfbbX (Linux version)\n\nVersion %s\n\n"
			 "Copyright 1986-1998. All rights reserved."
			 "\n",
			 XVersion (TRUE));

	string = StringCreate (buffer);

	n = 0;
	XtSetArg (args[n], XmNautoUnmanage, FALSE);
	n++;
	XtSetArg (args[n], XmNmessageAlignment, XmALIGNMENT_CENTER);
	n++;
	XtSetArg (args[n], XmNmessageString, string);
	n++;
	about_dialog = XmCreateMessageDialog (toplevel, "about", args, n);

	XmStringFree (string);

	XtUnmanageChild (XmMessageBoxGetChild (about_dialog, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (about_dialog, XmDIALOG_HELP_BUTTON));
	XtAddCallback (about_dialog, XmNokCallback, CancelCB, (XtPointer) & about_dialog);

	XtManageChild (about_dialog);
}

void CopyDialog (Widget w, XtPointer client_data, XtPointer call_data)
{
	char buffer[1024];
	Arg args[20];
	Cardinal n;
	XmString string;

	if (copy_dialog)
	{
		/* Dialog deja ouvert, on le passe devant */
		XRaiseWindow (XtDisplay (copy_dialog), XtWindow (XtParent (copy_dialog)));
		return;
	}

	sprintf (buffer,
			 "\n"
			 "         AX25 BBS software  -  XFBB version %s\n"
			 "         (C) F6FBB 1986-1998       (%s)\n\n"
			 "This software is in the public domain. It can be copied or\n"
			 "installed only for amateur use abiding by the laws.\n\n"
			 "All commercial or professional use is prohibited.\n\n"
			 "F6FBB (Jean-Paul ROUBELAT) declines any responsibilty\n"
			 "in the use of XFBB software.\n\n"
/*			 "This software is free of charge, but a 100 FF or 20 US $\n"
			 "(or more) contribution will be appreciated.\n\n",		*/
			 , XVersion (0), date ()
		);

	string = StringCreate (buffer);

	n = 0;
	XtSetArg (args[n], XmNautoUnmanage, FALSE);
	n++;
	XtSetArg (args[n], XmNmessageString, string);
	n++;
	copy_dialog = XmCreateMessageDialog (toplevel, "copyright", args, n);

	XmStringFree (string);

	XtUnmanageChild (XmMessageBoxGetChild (copy_dialog, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (copy_dialog, XmDIALOG_HELP_BUTTON));
	XtAddCallback (copy_dialog, XmNokCallback, CancelCB, (XtPointer) & copy_dialog);

	XtManageChild (copy_dialog);
}

