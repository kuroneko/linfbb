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

#include <xfbb.h>
#include <serv.h>
#include <sys/vfs.h>

void WinMessage (int temps, char *text);
void WinDebug (char *fmt,...);
int can_talk (int);

static int orig_percent;
static int orig_pitch;
static int orig_duration;


 /*
    char *strlwr(char *txt)
    {
    char *ptr = txt;

    while (*ptr)
    {
    if (isupper(*ptr))
    *ptr = tolower(*ptr);
    ++ptr;
    }
    return(txt);
    }
  */

void FbbSync (void)
{
	/*
	   XEvent event;

	   while (XtAppPending(app_context))
	   {
	   printf("Deb\n");
	   XtAppNextEvent(app_context, &event);
	   XtDispatchEvent(&event);
	   printf("Fin\n");
	   }  
	 */
	XmUpdateDisplay (toplevel);
	XmUpdateDisplay (toplevel);
}

void WinMSleep (unsigned milliseconds)
{
	usleep (milliseconds * 1000);
}

void WinSleep (unsigned seconds)
{
	sleep (seconds);
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

/*
   static void MessageBoxTimeOutCB(XtPointer client_data, XtIntervalId id)
   {
   printf("MessageBoxTimeOutCB\n");
   *((int *)client_data) = 1;
   }
 */

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
	/*   XtSetArg(args[n], XmNdialogType, XmDIALOG_QUESTION);n++; */
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

int editor_on (void)
{
	return (EditorOff != TRUE);
}

int xfbb_edit (void)
{
	int error = 0;
	char tmp[1024];
	char fn[128];
	char bbsv[40];

	printf ("xfbb_edit, reply = %d\n", reply);
	*tmp = '\0';

	if (reply)
	{
		char ftmp[80];

		if (pvoie->enrcur)
			strcpy (fn, mess_name (MESSDIR, pvoie->enrcur, ftmp));
		else
			*fn = '\0';
	}

	if (*ptmes->bbsv)
	{
		sprintf (bbsv, "@ %s", ptmes->bbsv);
	}
	else
		*bbsv = '\0';

	if (reply == 1)
	{
		sprintf (tmp, "Reply: %s %s", ptmes->desti, bbsv);
	}
	else if (reply == 3)
	{
		sprintf (tmp, "Mail: %s %s", ptmes->desti, bbsv);
	}
	else if (reply == 4)
	{
		sprintf (tmp, "Msg: %ld", pvoie->enrcur);
	}

	tmp[50] = '\0';

	if (reply == 4)
		CreateEditor (fn, NULL, tmp, ED_EDITMSG, TRUE);
	else
		CreateEditor (NULL, fn, tmp, ED_MESSAGE, TRUE);

	return (error);
}

int end_xfbb_edit (void)
{
	return 1;
}

void end_edit (int check)
{
	if (check)
	{
		cursor_wait ();
		init_bbs ();
		test_buf_fwd ();
		init_buf_swap ();
		init_buf_rej ();
		load_themes ();
		end_wait ();
	}
	EditorOff = TRUE;
}

int record_message (char *ptr, int len)
{
	selvoie (CONSOLE);
	if (ptr)
	{
		if (!get_mess_fwd ('\0', ptr, len, 2))
			get_mess_fwd ('\0', "\032", 1, 2);
	}
	retour_mbl ();
	aff_etat ('E');
	send_buf (voiecur);
	reply = 0;
	return (1);
}

int fbb_exec (char *commande)
{
	return (False);
}

void fbb_quit (unsigned retour)
{
	/*    closecom(); */
	sortie_prg ();
	exit (retour);
}

int fbb_list (int update)
{
	static int nb_prec = -1;
	int nb = 0;
	int i;
	XmString string;


	for (i = 0; i < NBVOIES; i++)
	{
		if (svoie[i]->sta.connect)
			++nb;
	}

	if ((nb >= 0) && (nb != nb_prec))
	{
		char buffer[80];

		nb_prec = nb;
		if (DEBUG)
		{
			strcpy (buffer, "TEST Mode");
		}
		else
		{
			if (nb == 0)
				strcpy (buffer, "No connected station");
			else
				sprintf (buffer, "%d connected station%c", nb, (nb > 1) ? 's' : '\0');
		}
		LabelSetString (ConnectLabel, buffer, (DEBUG) ? "RC" : NULL);
	}

	XmListDeleteAllItems (ConnectList);

	for (i = 0; i < NBVOIES; i++)
	{
		if (svoie[i]->sta.connect)
		{
			int ok = 0;
			int nobbs;
			int ch;
			struct tm *sdate;
			unsigned t_cnx;
			Forward *pfwd;
			char bbs[10];
			char buffer[80];
			char call[20];
			int choix = 0;
			int fwd = 0;

			/* Test du forward */
			ok = 0;
			*bbs = '\0';
			pfwd = p_port[no_port (i)].listfwd;
			while (pfwd)
			{
				if ((svoie[i]->curfwd) && (pfwd->forward == i))
				{
					nobbs = svoie[i]->bbsfwd;
					choix = (int) svoie[i]->cur_choix;
					strn_cpy (6, bbs, bbs_ptr + (nobbs - 1) * 7);
					ok = 1;
					fwd = 1;
				}
				pfwd = pfwd->suite;
			}

			if ((!ok) && (svoie[i]->mode & F_FOR) && (i != CONSOLE))
			{
				fwd = 2;
				strcpy (bbs, svoie[i]->sta.indicatif.call);
			}

			strlwr (bbs);

			if (P_TOR (i))
			{
				int port = no_port (i);

				if (p_port[port].t_wait)
				{
					fwd = 3;
					strcpy (bbs, "F-Wait");
				}
				else if (p_port[port].t_busy)
				{
					fwd = 3;
					strcpy (bbs, "F-Chck");
				}
			}

			/* Affichage */
			sdate = localtime (&(svoie[i]->debut));
			t_cnx = (unsigned) (time (NULL) - svoie[i]->debut);
			if (i == 1)
				ch = 99;
			else
				ch = (i > 0) ? i - 1 : i;
			if (svoie[i]->sta.indicatif.num)
				sprintf (call, "%s-%d", svoie[i]->sta.indicatif.call, svoie[i]->sta.indicatif.num);
			else
				sprintf (call, "%s", svoie[i]->sta.indicatif.call);
			sprintf (buffer, "%02d %-9s %02d:%02d %02d:%02d %2d %3d %c%c%s",
					 ch,
					 call,
					 sdate->tm_hour,
					 sdate->tm_min,
					 t_cnx / 3600,
					 (t_cnx / 60) % 60,
					 svoie[i]->sta.ret,
					 svoie[i]->sta.ack,
					 (choix < 2) ? ' ' : '0' + choix,
					 (choix < 2) ? ' ' : '/',
					 bbs
				);

			if (fwd == 1)
				string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "RC");
			else if (fwd == 2)
				string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "BC");
			else if (fwd == 3)
				string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "BF");
			else
				string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "NO");

			XmListAddItem (ConnectList, string, 0);
			XmStringFree (string);
		}
	}

	SetChList (FALSE);
	return (nb);
}

int call_dll (char *cmd, int mode, char *buffer, int len, char *data)
{
	/*
	   int    i;
	   int    point = 0;
	   char   dll_name[80];
	   char   *ptr;
	   int    retour = -1;

	   // Looking for DLL in the command
	   ptr = cmd;
	   i = 0;
	   while (*ptr)
	   {
	   if (!isgraph(*ptr))
	   break;
	   if (*ptr == '.')
	   point = 1;
	   dll_name[i++] = *ptr;
	   ++ptr;
	   }
	   dll_name[i] = '\0';

	   if (!point)
	   strcat(dll_name, ".DLL");

	   {
	   // Appel DLL
	   HINSTANCE  hinstFilter;

	   SetErrorMode(DEFAULT_ERROR_MODE|SEM_NOOPENFILEERRORBOX);
	   hinstFilter = LoadLibrary(dll_name);
	   if (hinstFilter > HINSTANCE_ERROR)
	   {
	   int    ac = 0;
	   char   *av[30];

	   av[ac] = strtok(cmd, " ");
	   while (av[ac])
	   {
	   ++ac;
	   av[ac] = strtok(NULL, " ");
	   }

	   if (mode == REPORT_MODE)
	   {
	   int    (FAR    PASCAL *DllProc)(int ac, char FAR **av, char FAR *, int len);
	   (FARPROC) DllProc = GetProcAddress(hinstFilter, "svc_main");
	   if (DllProc)
	   retour = (*DllProc)(ac, (char FAR **)av, (char FAR *)buffer, len);
	   }
	   else
	   {
	   int    (FAR    PASCAL *DllProc)(int ac, char FAR **av);
	   (FARPROC) DllProc = GetProcAddress(hinstFilter, "dll_main");
	   if (DllProc)
	   retour = (*DllProc)(ac, (char FAR **)av);
	   }
	   FreeLibrary(hinstFilter);
	   }
	   SetErrorMode(DEFAULT_ERROR_MODE);
	   }
	   return(retour);
	 */
	return (-1);
}

/*
   cmd : tableau des commandes a executer
   nb_cmd : nombre de commandes a executer
   mode : REPORT_MODE : attend un fichier log en retour 
   NO_REPORT_MODE : pas de fichier log en retour
   log   : nom du fichier de retour
   xdir  : repertoire dans lequel doivent s'executer les commandes
 */
int call_nbdos (char **cmd, int nb_cmd, int mode, char *log, char *xdir, char *data)
{

	/* Appel DOS */
	int i;
	int ExitCode = 0;

	char *ptr;
	char file[256];
	char buf[256];
	char dir[256];
	char arg[256];

	if (log)
		sprintf (file, " </dev/null >%s 2>&1", back2slash (log));
	else
		sprintf (file, " </dev/null");

	if (xdir)
	{
		ptr = strchr(xdir, ';');
		if (ptr)
			*ptr = '\0';
		sprintf (dir, "cd %s ; ", back2slash (xdir));
	}
	else
		*dir = '\0';

	if (data)
	{
		ptr = strchr(data, ';');
		if (ptr)
			*ptr = '\0';
		sprintf (arg, " \"%s\"", data);
	}
	else
		*arg = '\0';

	for (i = 0; i < nb_cmd; i++)
	{
		int retour;
		char *ptr;

		/* semi-column is forbidden for security reasons */
		ptr = strchr(cmd[i], ';');
		if (ptr)
			*ptr = '\0';

		sprintf (buf, "%s%s%s%s", dir, cmd[i], arg, file);
		printf ("Commande = {%s}\n", buf);

		retour = system (buf);
		ExitCode = retour >> 8;
		printf ("retour   = %x\n", retour);
		if (ExitCode == 127)
			ExitCode = -1;
		printf ("ExitCode = %d\n", ExitCode);
	}
	return (ExitCode);
}

int filter (char *ligne, char *buffer, int len, char *data, char *xdir)
{
	char deroute[80];
	int retour;

	sprintf (deroute, "%sEXECUTE.xxx", MBINDIR);
	retour = call_nbdos (&ligne, 1, REPORT_MODE, deroute, xdir, data);
	/* retour = wait_dos(ligne, deroute); */
	if (retour != -1)
	{
		outfichs (deroute);
	}
	unlink (deroute);
	return (retour);
}

void CompressPosition (int mode, int val, long numero)
{
	Arg args[10];
	Cardinal n;
	Pixel color;

	if (val == 0)
		val = 1;

	switch (mode)
	{
	case 0:
		color = df_pixel;
		val = 100;
		break;
	case 1:
		color = rc_pixel;
		break;
	case 2:
		color = bc_pixel;
		break;
	default:
		color = bc_pixel;
		break;
	}

	n = 0;
	XtSetArg (args[n], XmNsliderSize, val);
	n++;
	XtSetArg (args[n], XmNbackground, color);
	n++;
	XtSetValues (Jauge, args, n);

	FbbSync ();
}

void FbbStatus (char *callsign, char *texte)
{
	if (!foothelp)
	{
		char buffer[80];

		sprintf (buffer, "%-15s %s", callsign, texte);
		LabelSetString (Footer, buffer, NULL);
	}
}

void FbbMem (int update)
{
	static long old_us = 0xffffffffL;
	static long old_avail = 0;
	static time_t old_time = 0L;
	static int old_getd = 0;
	static int old_gMem = -1;
	static long old_nbmess = -1L;
	static long old_temp = 0;

	int gMem = nb_ems_pages ();
	long us = mem_alloue;
	char texte[80];

	time_t new_time = time (NULL);

	/* positionne tot_mem a la taille de memoire dispo
	   tot_mem = 1000000L; */

	if (operationnel == -1)
		return;

	/* Mise a jour toutes les secondes */
	if (old_time == new_time)
		return;

	old_time = new_time;

	if (us < 0L)
		us = 0L;

	if (us != old_us)
	{
		sprintf (texte, ": %ld", us);
		LabelSetString (Used, texte, NULL);
		old_us = us;
	}

	if (gMem != old_gMem)
	{
		sprintf (texte, ": %d K", gMem << 4);
		LabelSetString (GMem, texte, NULL);
		old_gMem = gMem;
	}

	/* Test disque toutes les 10 secondes */
	if (old_getd == 0)
	{
		struct statfs dfree;
		fsid_t fsid;

		if (statfs (DATADIR, &dfree) == 0)
		{
			if (dfree.f_bavail != old_avail)
			{
				sys_disk = dfree.f_bavail * (dfree.f_bsize / 1024L);

				LabelSetString (TxtDisk1, "Disk#1 free", (sys_disk < 1000) ? "RC" : NULL);

				/* FbbApp->FbbDiskTxt->IntVal = (sys_disk < 100); */
				sprintf (texte, ": %ld K", sys_disk);
				/* FbbApp->FbbDisk->IntVal = (sys_disk < 100); */
				/* FbbApp->FbbDisk->SetText(texte); */
				LabelSetString (Disk1, texte, (sys_disk < 1000) ? "RC" : NULL);

				old_avail = dfree.f_bavail;
				memcpy (&fsid, &dfree.f_fsid, sizeof (fsid));
			}
		}

		if ((statfs (MBINDIR, &dfree) == 0) &&
			(dfree.f_bavail != old_avail))
			/*      (memcmp(&fsid, &dfree.f_fsid,sizeof(fsid_t)) != 0)) */
		{
			if (dfree.f_bavail != old_temp)
			{
				tmp_disk = dfree.f_bavail * (dfree.f_bsize / 1024L);

				LabelSetString (TxtDisk2, "Disk#2 free", (tmp_disk < 1000) ? "RC" : NULL);
				/* FbbApp->FbbDiskTxt->IntVal = (sys_disk < 100); */
				sprintf (texte, ": %ld K", tmp_disk);
				/* FbbApp->FbbDisk->IntVal = (sys_disk < 100); */
				/* FbbApp->FbbDisk->SetText(texte); */
				LabelSetString (Disk2, texte, (tmp_disk < 1000) ? "RC" : NULL);

				XtManageChild (Disk2);
				XtManageChild (TxtDisk2);
				old_temp = dfree.f_bavail;
			}
		}
		else
		{
			tmp_disk = sys_disk;
			XtUnmanageChild (Disk2);
			XtUnmanageChild (TxtDisk2);
		}
		old_getd = 10;
	}
	else
		--old_getd;

	if (nbmess != old_nbmess)
	{
		sprintf (texte, ": %ld", nbmess);
		LabelSetString (Msgs, texte, NULL);
		old_nbmess = nbmess;
	}

	/*
	   val = GetFreeSystemResources(GFSR_SYSTEMRESOURCES);
	   if (val != old_system)
	   {
	   sprintf(texte, "Syst: %d%%", val);
	   FbbApp->FbbSystem->SetText(texte);
	   FbbApp->FbbSystem->UpdateWindow();
	   old_system = val;
	   }
	   val = GetFreeSystemResources(GFSR_USERRESOURCES);
	   if (val != old_user)
	   {
	   sprintf(texte, ": %d %%", val);
	   FbbApp->FbbUser->SetText(texte);
	   FbbApp->FbbUser->UpdateWindow();
	   old_user = val;
	   }

	   val = GetFreeSystemResources(GFSR_GDIRESOURCES);
	   if (val != old_gdi)
	   {
	   sprintf(texte, ": %d %%", val);
	   FbbApp->FbbGdi->SetText(texte);
	   FbbApp->FbbGdi->UpdateWindow();
	   old_gdi = val;
	   }
	   if (nbmess != old_nbmess)
	   {
	   sprintf(texte, ": %ld", nbmess);
	   FbbApp->FbbMsg->SetText(texte);
	   FbbApp->FbbMsg->UpdateWindow();
	   old_nbmess = nbmess;
	   }
	 */
}

void DisplayResync (int port, int nb)
{
	static int tot_resync = 0;
	char texte[80];

	if (nb)
	{
		if (nb == 1)
		{
			LabelSetString (TxtResync, "Resynchro", "RC");
			++tot_resync;
		}
		sprintf (texte, ": (%d) %d", port, nb);
		LabelSetString (Resync, texte, "RC");
	}
	else
	{
		LabelSetString (TxtResync, "Resynchro", NULL);
		sprintf (texte, ": %d       ", tot_resync);
		LabelSetString (Resync, texte, NULL);
	}
}

/*
   void AddListFwd(int mode, char *bbs)
   {

   switch (mode)
   {
   case 0 :
   FbbApp->ForwardList->SetRedraw(FALSE);
   FbbApp->ForwardList->ClearList();
   break;
   case 1 :
   FbbApp->ForwardList->AddString(bbs);
   break;
   case 2 :
   FbbApp->ForwardList->SetRedraw(TRUE);
   FbbApp->ForwardList->Invalidate();
   break;
   }

   }
 */

void ShowError (char *titre, char *info, int lig)
{
	printf ("%s : %s %d\n", titre, info, lig);
	/*
	   char   texte[80];

	   sndPlaySound("SystemExclamation", SND_ASYNC);
	   if(lig > 0)
	   wsprintf(texte, "%s %d", info, lig);
	   else
	   strcpy(texte, info);
	   FbbApp->Box(60, texte, titre, IDI_EXCLAMATION);
	 */
}

void WinMessage (int temps, char *text)
{
	MessageBox (temps, text, "Message", MB_ICONEXCLAMATION | MB_OK);
}

void WMessage (int temps, char *text, char *message)
{
	MessageBox (temps, text, message, MB_ICONEXCLAMATION | MB_OK);
}

static void InfoMessageTimeOutCB (XtPointer client_data, XtIntervalId id)
{
	printf ("XtAddTimeOutCB\n");
	InfoMessage (-1, NULL, NULL);
}

void InfoMessage (int temps, char *texte, char *titre)
{
	static Widget InfoW = NULL;
	Arg args[10];
	Cardinal n;

	if (InfoW == NULL)
	{
		n = 0;
		XtSetArg (args[n], XmNautoUnmanage, FALSE);
		n++;
		InfoW = XmCreateMessageDialog (toplevel, "info_msg", args, n);

		XtUnmanageChild (XmMessageBoxGetChild (InfoW, XmDIALOG_OK_BUTTON));
		XtUnmanageChild (XmMessageBoxGetChild (InfoW, XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild (XmMessageBoxGetChild (InfoW, XmDIALOG_HELP_BUTTON));
		XtUnmanageChild (XmMessageBoxGetChild (InfoW, XmDIALOG_SEPARATOR));
	}

	if (texte == NULL)
	{
		if (InfoW)
		{
			sleep (1);
			XtUnmanageChild (InfoW);
		}
	}
	else
	{
		XmString string;

		n = 0;
		if (titre)
		{
			XtSetArg (args[n], XmNtitle, titre);
			n++;
		}
		else
			sleep (1);

		/*      XtSetArg(args[n], XmNwidth, 300);n++; */
		XtSetValues (XtParent (InfoW), args, n);

		string = XmStringCreateSimple (texte);
		n = 0;
		XtSetArg (args[n], XmNdialogType, XmDIALOG_INFORMATION);
		n++;
		XtSetArg (args[n], XmNmessageString, string);
		n++;
		XtSetValues (InfoW, args, n);
		XmStringFree (string);

		if (titre)
			XtManageChild (InfoW);

		if (temps > 0)
		{
			printf ("XtAddTimeOut %d\n", temps);
			XtAppAddTimeOut (app_context, temps * 1000,
						  (XtTimerCallbackProc) InfoMessageTimeOutCB, NULL);
		}

		FbbSync ();
	}
}

void CloseFbbWindow (int numero)
{
	if (numero == 0)
		sysop_end ();
	HideFbbWindow (numero, toplevel);
}

int sel_option (char *texte, int *val)
{

	int res = MessageBox (0, texte, NULL, MB_ICONQUESTION | MB_OKCANCEL);

	if (res == IDOK)
		*val = 'y';

	return (res == IDOK);
}

int WindowService (void)
{
	/* return (FbbApp->SvcList != NULL); */
	return (False);
}

void GetBell (Display * dpy)
{
	XKeyboardState stateValues;

	XGetKeyboardControl (dpy, &stateValues);

	orig_percent = stateValues.bell_percent;
	orig_pitch = stateValues.bell_pitch;
	orig_duration = stateValues.bell_duration;
}



/*--------------------------------------------------------------------*
 |                             SetBell                                |
 *--------------------------------------------------------------------*/
void SetBell (Display * dpy, int pitch, int duration)
{
	XKeyboardControl controlValues;
	unsigned long valueMask = KBBellPercent | KBBellPitch | KBBellDuration;

	controlValues.bell_percent = orig_percent;
	controlValues.bell_pitch = pitch;
	controlValues.bell_duration = duration;

	XChangeKeyboardControl (dpy, valueMask, &controlValues);
}


#if 0
/*--------------------------------------------------------------------*
 |                            Pitch                                   |
 *--------------------------------------------------------------------*/
int Pitch (int note)
{
	double x, m, n, f;

	/* notes are calculated from the base frequency. */
	/* This is the first note on the keyboard.       */
	/* The frequency of a note = 2^(index / 12).     */

	x = (double) 2.0;
	m = (double) note;
	n = (double) 12.0;

	f = (double) appData->baseFrequency * pow (x, (m / n));

	return ((int) f);
}
#endif

/*--------------------------------------------------------------------*
 |                             PlayNote                               |
 *--------------------------------------------------------------------*/
void PlayNote (int Pitch, int Duration)
{
	if (orig_duration == 0)
		GetBell (XtDisplay (toplevel));

	if (Pitch)
	{
		SetBell (XtDisplay (toplevel), Pitch, Duration);
		XBell (XtDisplay (toplevel), 100);
		SetBell (XtDisplay (toplevel), orig_pitch, orig_duration);
		FbbSync ();
	}
	WinMSleep (Duration);
}

void bipper (void)
{
	if (!bip)
		return;

	if (doub_fen)
	{
		if (!play ("connect.wav"))
		{
			WinMessage (5, "Error : Cannot play file \"connect.wav\"");
		}
	}
	else
	{
		/* 
		   XBell(XtDisplay(toplevel), 100);
		 */
		PlayNote (2000, 40);
		PlayNote (1000, 40);
#if 0
		/* Haut parleur */
		/*
		   outportb(0x43, 0xb6);
		   outportb(0x42, 0xffff);
		   asm nop
		   asm nop
		   asm nop
		   outportb(0x42,0xffff);

		   OpenSound();
		   SetVoiceSound(1, MAKELONG(0, 2000), 20);
		   SetVoiceSound(1, MAKELONG(0, 1000), 20);
		   StartSound();
		   WaitSoundState(S_QUEUEEMPTY);
		   CloseSound();
		 */
#endif
	}

}

void music (int stat)
{

	static int note[10] =
	{
	/* 440, 0, 0, 440, 440, 388, 499, 388, 499, 0, 0, 499 */
	/*    44, 0, 44, 44, 42, 45, 42, 45, 0, 45 */
		40, 0, 40, 36, 40, 36, 38, 38, 0, 38
	};

	/*

	   int res = MessageBox(texte, NULL, MB_ICONQUESTION|MB_OKCANCEL);

	   if (res == IDOK)
	   {
	 */
	if (doub_fen)
	{
		if (stat)
		{
			t_tell = 1000;
			if (!play ("syscall.wav"))
			{
				WinMessage (5, "Error : Cannot play file \"syscall.wav\"");
			}
		}
		else
		{
			t_tell = -1;
		}
	}
	else
	{
		int i;

		for (i = 0; i < 10; i++)
		{
			double x, m, n, f;

			/* notes are calculated from the base frequency. */
			/* This is the first note on the keyboard.       */
			/* The frequency of a note = 2^(index / 12).     */

			if (note[i])
			{
				x = (double) 2.0;
				m = (double) note[i];
				n = (double) 12.0;

				f = (double) 55 *pow (x, (m / n));
			}
			else
				f = 0.0;


			PlayNote ((int) f, 200);
			WinMSleep (100);
		}
		/*

		   outportb(0x43, 0xb6);
		   outportb(0x42, 0xffff);
		   asm nop
		   asm nop
		   asm nop
		   outportb(0x42,0xffff);

		   if (stat)
		   {
		   t_tell = 1000 ;
		   OpenSound();
		   SetVoiceAccent(1, 120, 255, S_STACCATO, 0);
		   SetVoiceNote(1, 0, 8, 0);
		   for (i = 0 ; i < 10 ; i++)
		   {
		   //             if (note[i] == note[i+1])
		   //                 SetVoiceSound(1, MAKELONG(0, 0), 5);
		   //             SetVoiceSound(1, MAKELONG(0, note[i]), 50);
		   SetVoiceNote(1, note[i], 8, 0);
		   }
		   StartSound();
		   }
		   else
		   {
		   t_tell = -1 ;
		   }
		   WaitSoundState(S_QUEUEEMPTY);
		   CloseSound();
		 */
	}
}

void maj_menu_options (void)
{
	XmToggleButtonSetState (Opt[CM_OPTIONEDIT], sed, FALSE);
	XmToggleButtonSetState (Opt[CM_OPTIONCALL], ok_tell, FALSE);
	XmToggleButtonSetState (Opt[CM_OPTIONALARM], bip, FALSE);
	XmToggleButtonSetState (Opt[CM_OPTIONGATEWAY], gate, FALSE);
	XmToggleButtonSetState (Opt[CM_OPTIONJUSTIF], just, FALSE);
	XmToggleButtonSetState (Opt[CM_OPTIONAFFICH], ok_aff, FALSE);
	XmToggleButtonSetState (Opt[CM_OPTIONSOUNDB], doub_fen, FALSE);
	XmToggleButtonSetState (Opt[CM_OPTIONINEXPORT], aff_inexport, FALSE);
	XmToggleButtonSetState (ScanMsg, p_forward, FALSE);
}

void set_option (int id, int val)
{
	switch (id)
	{
	case CM_OPTIONEDIT:
		sed = val;
		break;
	case CM_OPTIONCALL:
		ok_tell = val;
		break;
	case CM_OPTIONJUSTIF:
		just = val;
		break;
	case CM_OPTIONALARM:
		bip = val;
		break;
	case CM_OPTIONGATEWAY:
		gate = val;
		break;
	case CM_OPTIONAFFICH:
		ok_aff = val;
		break;
	case CM_OPTIONSOUNDB:
		doub_fen = val;
		break;
	case CM_OPTIONINEXPORT:
		aff_inexport = val;
		break;
	}
	maj_options ();
}

int set_callsign (char *buf)
{
	FILE *fptr;

	if (!ind_console (0, buf))
		return (0);

	aff_msg_cons ();
	if ((fptr = fopen (d_disque ("ETAT.SYS"), "r+t")) == NULL)
		fbb_error (ERR_OPEN, d_disque ("ETAT.SYS"), 0);
	fprintf (fptr, "%-6s-%X\n", cons_call.call, cons_call.num);
	ferme (fptr, 74);
	return (1);
}

void get_callsign (char *buf)
{
	sprintf (buf, "%s-%d", cons_call.call, cons_call.num);
}

void win_msg_cons (int priv, int hold)
{
	char texte[80];

	if (priv >= 0)
	{
		sprintf (texte, ": %d", priv);
		LabelSetString (TxtPriv, "Priv msgs", (priv > 0) ? "RC" : NULL);
		LabelSetString (Priv, texte, (priv > 0) ? "RC" : NULL);
		/* FbbApp->FbbPriv->IntVal = priv; */
	}

	if (hold >= 0)
	{
		sprintf (texte, ": %d", hold);
		LabelSetString (TxtHold, "Hold msgs", (hold > 0) ? "RC" : NULL);
		LabelSetString (Hold, texte, (hold > 0) ? "RC" : NULL);
		/* FbbApp->FbbHoldTxt->IntVal = hold; */
	}
}

void disconnect_channel (int channel, int immediate)
{
	if ((svoie[channel]->sta.connect) && (channel) && (channel < NBVOIES))
	{
		if (immediate)
			force_deconnexion (channel, 1);
		else
			deconnexion (channel, 1);
	}
}

int can_talk (int channel)
{
	if ((channel < 0) || (channel >= NBVOIES))
		return (FALSE);
	return ((svoie[channel]->sta.connect) && (channel) && (channel < NBVOIES) && (svoie[channel]->niv3 == 0));
}

int talk_to (int channel)
{
	if ((svoie[channel]->sta.connect) && (channel) && (channel < NBVOIES))
	{
		if (svoie[channel]->niv3 == 0)
		{
			v_tell = channel;
			selvoie (CONSOLE);
			strn_cpy (6, pvoie->sta.indicatif.call, cons_call.call);
			pvoie->sta.indicatif.num = cons_call.num;
			pvoie->sta.connect = 16;
			pvoie->deconnect = FALSE;
			pvoie->pack = 0;
			pvoie->read_only = 0;
			pvoie->vdisk = 2;
			pvoie->xferok = 1;
			pvoie->mess_recu = 1;
			pvoie->mbl = 0;
			init_timout (CONSOLE);
			pvoie->temp3 = 0;
			pvoie->nb_err = 0;
			pvoie->finf.lang = langue[0]->numlang;
			init_langue (voiecur);
			maj_niv (N_MBL, 9, 2);
			selvoie (v_tell);
			pvoie->sniv1 = pvoie->niv1;
			pvoie->sniv2 = pvoie->niv2;
			pvoie->sniv3 = pvoie->niv3;
			pvoie->nb_err = 0;
			init_langue (voiecur);
			maj_niv (N_MBL, 9, 2);
			texte (T_MBL + 16);
			t_tell = -1;
			ShowFbbWindow (0, toplevel);
			return (1);
		}
	}
	return (0);
}

int fct_arret (int type)
{

	long caltemps;

#ifdef ENGLISH
	static char *strtype[4] =
	{"Error ", "End", "Re-Run ", "Maintenance"};

#else
	static char *strtype[4] =
	{"Erreur", "Fin", "Relance", "Maintenance"};

#endif
	int c;
	int nb_connect;
	int res;

	c = 1;
	nb_connect = actif (2);
	if (nb_connect)
	{
		res = MessageBox (0, "Immediate", strtype[type],
						  MB_ICONQUESTION | MB_YESNOCANCEL);
		if (res == IDCANCEL)
			return (0);
		if (res == IDNO)
			c = 2;
	}

	type_sortie = type;

	switch (c)
	{
	case 1:
		if (MessageBox (0, "Are you sure ?", "Quit XFBB",
						MB_ICONQUESTION | MB_OKCANCEL) == IDOK)
		{
			house_keeping ();
			maintenance ();
			fbb_quit (type);
		}
		else
			return (0);
		break;
	case 2:
		save_fic = 1;
		set_busy ();
		time (&caltemps);
		stop_min = minute (caltemps);
		break;
	case 3:
		house_keeping ();
		break;

	}

	return (1);
}

void scan_fwd (int val)
{
	XmToggleButtonSetState (ScanMsg, val, FALSE);

	if (val)
	{
		if (p_forward == 0)
		{
			p_forward = 1;
			init_buf_fwd ();
			init_buf_swap ();
			init_buf_rej ();
			init_bbs ();
		}
	}

	else
	{
		if (p_forward)
			p_forward = 0;
	}

	maj_options ();
}

void win_status (char *txt)
{
	char texte[80];

	sprintf (texte, ": %s", txt);

	LabelSetString (State, texte, NULL);
}

char *win_memo (int val)
{
	FILE *fptr = fopen (d_disque ("MEMO.SYS"), "rt");
	char buffer[257];
	char *ptr = NULL;

	if (fptr)
	{
		while (fgets (buffer, 256, fptr))
		{
			if (val-- == 0)
			{
				ptr = var_crlf (sup_ln (buffer));
				break;
			}
		}
		ferme (fptr, 81);
	}
	return (ptr);
}

void fin_tache (int voie)
{
	selvoie (voie);
	traite_voie (voie);
}

void win_execute (char *buffer)
{
	/*
	   WORD               diagInst;
	   BOOL               bResult;
	   TASKENTRY      te;
	   HANDLE         hInst;

	   // Attend la reception eventuelle de trames RS232
	   WinSleep(1);

	   if ((FbbApp->ddeCB == 0) && (FbbApp->pThunk == NULL))
	   {
	   // Si la liste est vide, lancer la notify Callback
	   //AppWindow  = FbbApp->MainWindow->HWindow;
	   //FbbApp->pThunk = MakeProcInstance((FARPROC)Callback, FbbApp->ghInstance);
	   //NotifyRegister(0, (LPFNNOTIFYCALLBACK)FbbApp->pThunk, NF_NORMAL);
	   }
	   ++FbbApp->ddeCB;

	   // diagInst = WinExec(buffer, SW_SHOWNORMAL);
	   diagInst = WinExec(buffer, SW_SHOWMINNOACTIVE);
	   if (diagInst < 32)
	   {
	   FbbApp->MainWindow->MessageBox("WinExec Failed", "Error", MB_OK);
	   }
	   else
	   {
	   hInst = (HANDLE)diagInst;
	   te.dwSize = sizeof(te);
	   bResult = TaskFirst(&te);
	   while (bResult)
	   {
	   if (te.hInst == hInst)
	   {
	   pvoie->task = te.hTask;
	   break;
	   }
	   bResult = TaskNext(&te);
	   }
	   }
	 */
}

#if 0
void init_socket (int port)
{
	/*
	   int adresse = p_com[p_port[port].ccom].cbase;
	   FbbApp->Socket = new MySocket(FbbApp->Client->HWindow, adresse, port, p_port[port].nb_voies);
	 */
}

void dec_socket (int voie)
{
	/*
	   int    canal = no_canal(voie);
	   SOCKET sock;

	   if (canal == -1)
	   return;

	   sock = FbbApp->Socket->SockCan[canal-1].socket;
	   FbbApp->Socket->Server_DestroyConnection(sock);
	   FbbApp->Socket->Disconnection(sock, 0);
	 */
}

void free_socket (int port)
{
	/*
	   if (FbbApp->Socket)
	   delete FbbApp->Socket;
	   FbbApp->Socket = NULL;
	 */
}

int snd_tcp (int port, int canal, int cmd, char *buffer, int len, Beacon * ptr)
{
	/*
	   SOCKET sock;

	   if (cmd == UNPROTO)
	   {
	   // Unprotos ? ...
	   return(0);
	   }

	   sock = FbbApp->Socket->SockCan[canal-1].socket;
	   if (sock == 0xffff)
	   return(-1);

	   FbbApp->Socket->Send(sock, buffer, len);
	   return(len);
	 */
}

int tcp_busy (int voie)
{
	/*
	   int    canal = no_canal(voie);

	   if (canal == -1)
	   return(0);

	   SOCKET sock = FbbApp->Socket->SockCan[canal-1].socket;

	   return((FbbApp->Socket->Full(sock)) ? svoie[voie]->maxbuf * 2 : 0);
	 */
}

void connect_tcp (int voie, indicat * call, char *address, int tcport)
{
	/*
	   int    canal = no_canal(voie);
	   if (canal == -1)
	   return;

	   MySocket *Socket = FbbApp->Socket;
	   SocketCanal *Canal = &Socket->SockCan[canal-1];

	   wsprintf(Canal->call, "%s-%d", call->call, call->num);
	   Canal->socket = Socket->ConnectPort((LPSTR)address, tcport);
	 */
}
#endif

void user_status (int voie)
{
	int i;
	int trouve = FALSE;
	int pos = 0;
	XmString string;

	DisplayInfoDialog (voie);

	/* Chercher la voie dans la liste */
	for (i = 0; i < NBVOIES; i++)
	{
		if (svoie[i]->sta.connect)
		{
			++pos;
			if (i == voie)
			{
				trouve = TRUE;
				break;
			}
		}
	}

	if (trouve)
	{
		int ok = 0;
		int nobbs;
		int ch;
		struct tm *sdate;
		unsigned t_cnx;
		Forward *pfwd;
		char bbs[10];
		char buffer[80];
		char call[20];
		int choix = 0;
		int fwd = 0;

		/* Test du forward */
		ok = 0;
		*bbs = '\0';
		pfwd = p_port[no_port (i)].listfwd;
		while (pfwd)
		{
			if ((svoie[i]->curfwd) && (pfwd->forward == i))
			{
				nobbs = svoie[i]->bbsfwd;
				choix = (int) svoie[i]->cur_choix;
				strn_cpy (6, bbs, bbs_ptr + (nobbs - 1) * 7);
				ok = 1;
				fwd = 1;
			}
			pfwd = pfwd->suite;
		}

		if ((!ok) && (svoie[i]->mode & F_FOR) && (i != CONSOLE))
		{
			fwd = 2;
			strcpy (bbs, svoie[i]->sta.indicatif.call);
		}

		strlwr (bbs);

		if (P_TOR (i))
		{
			int port = no_port (i);

			if (p_port[port].t_wait)
			{
				fwd = 3;
				strcpy (bbs, "F-Wait");
			}
			else if (p_port[port].t_busy)
			{
				fwd = 3;
				strcpy (bbs, "F-Chck");
			}
		}

		/* Affichage */
		sdate = localtime (&(svoie[i]->debut));
		t_cnx = (unsigned) (time (NULL) - svoie[i]->debut);
		if (i == 1)
			ch = 99;
		else
			ch = (i > 0) ? i - 1 : i;
		if (svoie[i]->sta.indicatif.num)
			sprintf (call, "%s-%d", svoie[i]->sta.indicatif.call, svoie[i]->sta.indicatif.num);
		else
			sprintf (call, "%s", svoie[i]->sta.indicatif.call);
		sprintf (buffer, "%02d %-9s %02d:%02d %02d:%02d %2d %3d %c%c%s",
				 ch,
				 call,
				 sdate->tm_hour,
				 sdate->tm_min,
				 t_cnx / 3600,
				 (t_cnx / 60) % 60,
				 svoie[i]->sta.ret,
				 svoie[i]->sta.ack,
				 (choix < 2) ? ' ' : '0' + choix,
				 (choix < 2) ? ' ' : '/',
				 bbs
			);

		if (fwd == 1)
			string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "RC");
		else if (fwd == 2)
			string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "BC");
		else if (fwd == 3)
			string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "BF");
		else
			string = XmStringGenerate (buffer, NULL, XmCHARSET_TEXT, "NO");

		/* Remplacer la ligne par la nouvelle ligne */
		XmListReplaceItemsPos (ConnectList, &string, 1, pos);
		XmStringFree (string);
		SetChList (FALSE);
	}
}

void aff_traite (int voie, int val)
{
	/*
	   FbbApp->ConnectList->SetChannelProcess(voie, val);
	   if (1)
	   // if (val)
	   {
	   char buf[40];
	   char call[30];
	   Svoie *ptvoie = svoie[voie];

	   sprintf (buf, "Buf:%03d Task:%02d-%02d-%02d",
	   ptvoie->sta.mem, ptvoie->niv1, ptvoie->niv2, ptvoie->niv3);

	   #ifdef ENGLISH
	   sprintf (call, "Ch:%02d %s-%d",
	   #else
	   sprintf (call, "Vo:%02d %s-%d",
	   #endif
	   virt_canal (voie), ptvoie->sta.indicatif.call, ptvoie->sta.indicatif.num);
	   FbbStatus (call, buf);
	   }
	 */
}

void select_con (int voie)
{
	/*
	   char   select[10];

	   wsprintf(select, "%02d", voie);
	   // FbbApp->ConnectList->SetSelString(select, -1);
	 */
}

void SpoolLine (int voie, int attr, char *data, int lg)
{
	if (data)
	{
		char *ptr;
		char *buf;
		int i;

		if (p_fptr == NULL)
		{
			char *ptr;

			/* Ouvrir le spooler */
			ptr = getenv ("XFBB_PRN");
			if (ptr)
				p_fptr = popen (ptr, "w");
			else
				p_fptr = popen ("lpr", "w");
		}

		printf ("spool %d carac ok\n", lg);
		ptr = buf = malloc (lg);

		if (buf == NULL)
			return;

		for (i = 0; i < lg; i++)
		{
			if (data[i] == '\r')
				*ptr++ = '\n';
			else
				*ptr++ = data[i];
		}

		fwrite (buf, lg, 1, p_fptr);

		free (buf);
	}
	else
	{
		/* Fermer l'imprimante */
		if (p_fptr)
		{
			pclose (p_fptr);
			p_fptr = NULL;
		}
		return;
	}
}

#include <stdarg.h>
void WinDebug (char *fmt,...)
{
	va_list argptr;
	int cnt;

	va_start (argptr, fmt);
	cnt = vprintf (fmt, argptr);
	va_end (argptr);
}

void SendEchoCmd (char *buf, int lg)
{
	/*
	   int    i;
	   int    debut;
	   char   *ptr;
	   char   sauve;

	   if (lg == 0)
	   return;

	   if (!FbbApp->ProgTnc->IsWindow())
	   return;

	   ptr= buf;
	   debut = 1;
	   for (i = 0 ; i < lg; i++)
	   {
	   // Saute les LF
	   if ((buf[i] == '\r') || (buf[i] == '\n'))
	   {
	   if (debut)
	   {
	   ptr= &buf[i+1];
	   }
	   else
	   {
	   sauve = buf[i];
	   buf[i]= '\0';
	   FbbApp->ProgTnc->ResLine->Insert(ptr);
	   FbbApp->ProgTnc->ResLine->Insert("\r\n");
	   buf[i] = sauve;
	   ptr= &buf[i+1];
	   debut = 1;
	   }
	   }
	   else
	   debut = 0;
	   }

	   if (!debut)
	   {
	   FbbApp->ProgTnc->ResLine->Insert(ptr);
	   FbbApp->ProgTnc->ResLine->Insert("\r\n");
	   }
	 */
}

/*
   void window_write(int numero, char *data, int len, int color, int header)
   {
   char buf[300];
   char *ptr;

   memcpy(buf, data, len);
   buf[len] = '\0';

   ptr = buf;
   while (*ptr)
   {
   if (*ptr == '\r')
   *ptr = '\n';
   ++ptr;
   }

   printf("%02d:%s", numero, buf);
   }
 */

static void FSOkCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	*((int *) client_data) = 1;
}

static void FSCancelCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	*((int *) client_data) = 0;
}

int GetFileNameDialog (char *filename)
{
	Arg args[20];
	Cardinal n;
	XmString string;
	Widget fs;
	int retour;
	XEvent event;

	n = 0;
	string = XmStringCreateSimple ("*.imp");
	XtSetArg (args[n], XmNpattern, string);
	n++;
	fs = XmCreateFileSelectionDialog (toplevel, "file_selection", args, n);
	XmStringFree (string);

	XtAddCallback (fs, XmNokCallback, FSOkCB, &retour);
	XtAddCallback (fs, XmNcancelCallback, FSCancelCB, &retour);

	XtUnmanageChild (XmFileSelectionBoxGetChild (fs, XmDIALOG_HELP_BUTTON));

	XtManageChild (fs);

	retour = -1;

	XtAddGrab (fs, TRUE, TRUE);

	while (retour == -1)
	{
		XtAppNextEvent (app_context, &event);
		XtDispatchEvent (&event);
	}

	/*
	   n = 0;
	   XtSetArg(args[n], XmNfileListItemCount, &nb);n++;
	   XtSetArg(args[n], XmNfileListItems, &stable);n++;
	   XtGetValues(fs, args, n);
	 */

	if (retour)
	{
		char *text;

		n = 0;
		XtSetArg (args[n], XmNtextString, &string);
		n++;
		XtGetValues (fs, args, n);

		if (!XmStringGetLtoR (string, XmSTRING_DEFAULT_CHARSET, &text))
			retour = 0;
		else
			strcpy (filename, text);
	}

	XtRemoveGrab (fs);
	XtUnmanageChild (fs);
	XtDestroyWidget (XtParent (fs));

	return (retour);
}
