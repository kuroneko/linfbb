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

#include <stdint.h>

static int LgList;

static Widget PendingDialog;
static Widget FwdList[4];
static Widget imp_dialog;


static void import_bbs(Widget w, char *call);

void PendingCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int i;

  if (XtIsManaged(PendingDialog))
    {
      /* Passer la fenetre au 1er plan */
      XRaiseWindow(XtDisplay(PendingDialog),XtWindow(XtParent(PendingDialog)));
      return;
    }

  cursor_wait();

  LgList = 0;

  for (i = 0 ; i< 4 ; i++)
    {
      XmListDeleteAllItems(FwdList[i]);
    }

  fwd_encours();

  XtManageChild(PendingDialog);
  end_wait();
}

static void CloseCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  XtUnmanageChild(PendingDialog);
}

static void StopFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int *tab;
  int nb;
  int	nb_bbs;
  int		i;
  char		str[80];
  char		bbs[8];
  int		bbs_list[80];
  int		port_fwd;
  char ifwd[NBBBS][7];

  ch_bbs (1, ifwd);
  
  nb_bbs = 0;

  for (i = 0 ; i< 4 ; i++)
    {
      if (XmListGetSelectedPos(FwdList[i], &tab, &nb))
	{
	  nb_bbs = 1;
	  bbs_list[0] = (i*20) + tab[0] - 1;
	  free(tab);
	  break;
	}
    }

  if (nb_bbs == 0)
    {
      strcpy(str, "Select a BBS");
      MessageBox(60, str, "STOP", MB_OK | MB_ICONEXCLAMATION);
      return;
    }

  for (i = 0 ; i < nb_bbs ; i++)
    {
      if (!isgraph(ifwd[bbs_list[i]][0]))
	continue;

      strn_cpy(6, bbs, ifwd[bbs_list[i]]);
      sprintf(str, "Stops forwarding to BBS %s", bbs);
      /*
	ret = MessageBox(60, str, "STOP", MB_OKCANCEL | MB_ICONQUESTION);
	if (ret == IDCANCEL)
	continue;
	*/
      
      *str= '\0';
      switch (port_fwd = dec_fwd(bbs))
	{
	case -1 :
	  sprintf(str, "BBS %s is not forwarding", bbs);
	  break;
	case -2 :
	  sprintf(str, "Unknown BBS %s", bbs);
	  break;
	default :
	  break;
	}
      if (*str)
	{
	  MessageBox(60, str, "STOP", MB_OK | MB_ICONEXCLAMATION);
	  return;
	}
    }
  CloseCB(w, NULL, NULL);
}

static void SelectFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int i;

  for (i = 0 ; i< 4 ; i++)
    {
      if (i == (uintptr_t)client_data)
	continue;
      XmListDeselectAllItems(FwdList[i]);
    }
}

static void StartFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int nb_bbs;
  char str[80];
  int bbs_list[80];
  int i;
  int nb;
  int *tab;
  char ifwd[NBBBS][7];

  ch_bbs (1, ifwd);

  nb_bbs = 0;

  for (i = 0 ; i< 4 ; i++)
    {
      if (XmListGetSelectedPos(FwdList[i], &tab, &nb))
	{
	  nb_bbs = 1;
	  bbs_list[0] = (i*20) + tab[0] - 1;
	  free(tab);
	  break;
	}
    }

  if (nb_bbs == 0)
    {
      strcpy(str, "Select a BBS");
      MessageBox(60, str, "START", MB_OK | MB_ICONEXCLAMATION);
      return;
    }

  for (i = 0 ; i < nb_bbs ; i++)
    {
      int	retour;
      int	port_fwd;
      int	reverse = 1;
      char	bbs[8];
      
      *str = '\0';
      if (!isgraph(ifwd[bbs_list[i]][0]))
	continue;

      strn_cpy(6, bbs, ifwd[bbs_list[i]]);
      
      retour = val_fwd(bbs, &port_fwd, reverse);
      if (retour < 0)
	{
	  switch (retour)
	    {
	    case -1 :
	      sprintf(str, "No forwarding channel on port %d", port_fwd);
	      break;
	    case -2:
	      sprintf(str, "No port affected to %s", bbs);
	      break;
	    case -3 :
	      sprintf(str, "Unknown BBS %s", bbs);
	      break;
	    case -4 :
	      sprintf(str, "BBS %s already connected", bbs);
	      break;
	    }
	  if (*str)
	    {
	      MessageBox(60, str, "START", MB_OK | MB_ICONEXCLAMATION);
	      return;
	    }
	}
    }
  CloseCB(w, NULL, NULL);
}

static void StartAllCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  char		str[80];
  int		port_fwd;
  int		reverse =0;
  int		retour;
  char		bbs[8];
  
  *str = '\0';
  strcpy(bbs, "9");
  
  retour = val_fwd(bbs, &port_fwd, reverse);
  if (retour < 0)
    {
      switch (retour)
	{
	case -1 :
	  sprintf(str, "No forwarding channel on port %d", port_fwd);
	  break;
	case -2:
	  sprintf(str, "No port affected to %s", bbs);
	  break;
	case -3 :
	  sprintf(str, "Unknown BBS %s", bbs);
	  break;
	case -4 :
	  sprintf(str, "BBS %s already connected", bbs);
	  break;
	}
      if (*str)
	{
	  MessageBox(60, str, "START", MB_OK | MB_ICONEXCLAMATION);
	  return;
	}
    }
  if (*str == '\0')
    CloseCB(w, NULL, NULL);
}

static void OkCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget *pw;

  pw = (Widget *)client_data;

  XtUnmanageChild(imp_dialog);
  XtDestroyWidget(XtParent(imp_dialog));
  imp_dialog = NULL;
}

static void ApplyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget wt;
  Widget wf;
  char *ptr;

  wf = (Widget)client_data;

  /*  wt = XmSelectionBoxGetChild(call_dialog, XmDIALOG_TEXT); */
  wt = XmSelectionBoxGetChild(w, XmDIALOG_TEXT);

  ptr = XmTextGetString(wt);

  XtUnmanageChild(imp_dialog);
  XtDestroyWidget(XtParent(imp_dialog));
  imp_dialog = NULL;

  if (find(ptr))
    {
      import_bbs(wf, ptr);
    }
  else
    {
      char str[80];

      sprintf(str, "Invalid callsign %s", ptr);
      MessageBox(60, str, "Import", MB_OK | MB_ICONEXCLAMATION);
    }
  
  XtFree(ptr);
  
}

static void ImportCallsign(Widget w)
{
  Arg args[20] ;
  Cardinal n;

  if (imp_dialog)
    {
      /* Dialog deja ouvert, on le passe devant */
      XRaiseWindow(XtDisplay(imp_dialog),XtWindow(XtParent(imp_dialog)));
      return;
    }

  n = 0;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  XtSetArg(args[n], XmNmessageAlignment, XmALIGNMENT_CENTER);n++;
  imp_dialog = XmCreatePromptDialog(toplevel, "callsign", args, n);

  XtManageChild(XmSelectionBoxGetChild(imp_dialog, XmDIALOG_APPLY_BUTTON));
  XtUnmanageChild(XmSelectionBoxGetChild(imp_dialog, XmDIALOG_OK_BUTTON));
  XtUnmanageChild(XmSelectionBoxGetChild(imp_dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback(imp_dialog, XmNapplyCallback, ApplyCB, (XtPointer)w);
  XtAddCallback(imp_dialog, XmNcancelCallback, OkCB, (XtPointer)w);

  XtManageChild(imp_dialog);
}

static void import_bbs(Widget w, char *call)
{
  int fd;
  int retour;
  char filename[512];
  char str[80];

  retour = GetFileNameDialog(filename);

  if (retour == 0)
    return;

  if ((fd = open(filename, S_IREAD)) != -1)
    close(fd);
  else 
    {
      sprintf(str, "Cannot find %s", filename);
      MessageBox(60, str, "Import", MB_OK | MB_ICONEXCLAMATION);
      return;
    }

  n_cpy(6, svoie[INEXPORT]->sta.indicatif.call, call) ;
  svoie[INEXPORT]->sta.indicatif.num = 0 ;
  strcpy(io_fich,filename);
  
  mail_ch = INEXPORT;
  selvoie(mail_ch) ;
  pvoie->sta.connect = inexport = 4 ;
  pvoie->enrcur = 0L ;
  pvoie->debut = time(NULL);
  pvoie->mode = F_FOR | F_HIE | F_BID | F_MID ;
  pvoie->finf.lang = langue[0]->numlang ;
  aff_event(voiecur, 1);
  maj_niv(N_MBL, 99, 0) ;
  
  CloseCB(w, NULL, NULL);
}

static void ImportFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int nb_bbs;
  int bbs_list[80];
  char callsign[10];
  char str[80];
  int i;
  int nb;
  int *tab;
  char ifwd[NBBBS][7];

  ch_bbs (1, ifwd);

  if (svoie[INEXPORT]->sta.connect)
    {
      sprintf(str, "Channel busy, cannot import");
      MessageBox(60, str, "Import", MB_OK | MB_ICONEXCLAMATION);
      return;
    }

  nb_bbs = 0;
  for (i = 0 ; i< 4 ; i++)
    {
      if (XmListGetSelectedPos(FwdList[i], &tab, &nb))
	{
	  nb_bbs = 1;
	  bbs_list[0] = (i*20) + tab[0] - 1;
	  free(tab);
	  n_cpy(6, callsign, ifwd[bbs_list[0]]) ;
	  if (!isgraph(callsign[0]))
	    {
	      nb_bbs = 0;
	    }
	  break;
	}
    }

  if (nb_bbs == 0)
    {
      /*
	sprintf(str, "Select a BBS");
	MessageBox(60, str, "Import", MB_OK | MB_ICONEXCLAMATION);
	*/
      ImportCallsign(w);
      return;
    }
  if (nb_bbs > 1)
    {
      sprintf(str, "Only one selection allowed");
      MessageBox(60, str, "Import", MB_OK | MB_ICONEXCLAMATION);
      return;
    }

  import_bbs(w, callsign);
}

static void ExportFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int fd;
  int nb_bbs;
  char str[80];
  int bbs_list[80];
  int i;
  int *tab;
  int nb;
  int retour;
  char filename[512];
  char ifwd[NBBBS][7];
  char callsign[10];

  ch_bbs (1, ifwd);

  if (svoie[INEXPORT]->sta.connect)
    {
      sprintf(str, "Channel busy, cannot export");
      MessageBox(60, str, "Export", MB_OK | MB_ICONEXCLAMATION);
      return;
    }

  nb_bbs = 0;
  for (i = 0 ; i< 4 ; i++)
    {
      if (XmListGetSelectedPos(FwdList[i], &tab, &nb))
	{
	  nb_bbs = 1;
	  bbs_list[0] = (i*20) + tab[0] - 1;
	  free(tab);
	  n_cpy(6, callsign, ifwd[bbs_list[0]]) ;
	  if (!isgraph(callsign[0]))
	    {
	      nb_bbs = 0;
	    }
	  break;
	}
    }

  if (nb_bbs == 0)
    {
      sprintf(str, "Select a BBS");
      MessageBox(60, str, "Import", MB_OK | MB_ICONEXCLAMATION);
      return;
    }
  if (nb_bbs > 1)
    {
      sprintf(str, "Only one selection allowed");
      MessageBox(60, str, "Import", MB_OK | MB_ICONEXCLAMATION);
      return;
    }

  n_cpy(6, svoie[INEXPORT]->sta.indicatif.call, callsign) ;
  svoie[INEXPORT]->sta.indicatif.num = 0 ;

  selvoie(INEXPORT) ;
  pvoie->mode = F_FOR ;
  if (!appel_rev_fwd(1))
    {
      sprintf(str, "No mail for %s", svoie[INEXPORT]->sta.indicatif.call);
      MessageBox(60, str, "Export", MB_OK | MB_ICONEXCLAMATION);
      return;
    }
  
  retour = GetFileNameDialog(filename);

  if (retour == 0)
    return;

  strcpy(io_fich, filename);
  
  if ((fd = creat(io_fich, S_IREAD | S_IWRITE)) != -1)
    close(fd);
  else 
    {
      sprintf(str, "Cannot create %s", io_fich);
      MessageBox(60, str, "Export", MB_OK | MB_ICONEXCLAMATION);
      return;
    }
  
  unlink(io_fich) ; /* supprime un eventuel fichier */
  mail_ch = INEXPORT;
  selvoie(mail_ch) ;
  pvoie->sta.connect = inexport = 4 ;
  pvoie->debut = time(NULL);
  pvoie->finf.lang = langue[0]->numlang ;
  aff_event(voiecur, 1);
  maj_niv(N_MBL, 98, 0) ;

  CloseCB(w, NULL, NULL);
}

void PendingForward(Widget toplevel)
{
  int i;
  Arg args[20] ;
  Cardinal n;
  Widget w;
  XmRenderTable rt;

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 5);n++;
  XtSetArg(args[n], XmNmarginWidth, 5);n++;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  PendingDialog = XmCreateFormDialog(toplevel, "pending_fwd", args, n);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 2);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 17);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(PendingDialog, "stop_fwd", args, n);
  XtAddCallback(w, XmNactivateCallback, StopFwdCB, 0);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 18);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 33);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(PendingDialog, "start_fwd", args, n);
  XtAddCallback(w, XmNactivateCallback, StartFwdCB, (XtPointer)PendingDialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 34);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 49);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(PendingDialog, "start_all", args, n);
  XtAddCallback(w, XmNactivateCallback, StartAllCB, (XtPointer)PendingDialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 50);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 65);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(PendingDialog, "import_fwd", args, n);
  XtAddCallback(w, XmNactivateCallback, ImportFwdCB, (XtPointer)PendingDialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 66);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 81);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(PendingDialog, "export_fwd", args, n);
  XtAddCallback(w, XmNactivateCallback, ExportFwdCB, (XtPointer)PendingDialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 82);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 97);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(PendingDialog, "close", args, n);
  XtAddCallback(w, XmNactivateCallback, CloseCB, (XtPointer)PendingDialog);
  XtManageChild(w);

  for (i = 0 ; i < 4 ; i++)
    {
      n= 0;
      XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
      XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
      XtSetArg(args[n], XmNleftPosition, i * 25);n++;
      XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
      XtSetArg(args[n], XmNrightPosition, i* 25 + 24);n++;
      XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
      XtSetArg(args[n], XmNbottomWidget, w);n++;
      XtSetArg(args[n], XmNhighlightThickness, 0);n++;
      XtSetArg(args[n], XmNvisibleItemCount, 20);n++;
      FwdList[i] = XmCreateList(PendingDialog, "fwd_list", args, n);
      XtAddCallback(FwdList[i], XmNbrowseSelectionCallback, 
		    SelectFwdCB, (XtPointer)(intptr_t)i);
      XtAddCallback(FwdList[i], XmNdefaultActionCallback, 
		    StartFwdCB, (XtPointer)(intptr_t)i);

      XtVaGetValues(FwdList[i], XmNrenderTable, &rt, NULL, NULL);
    
      /* Make a copy so that setvalues will work correctly */
      rt = XmRenderTableCopy(rt, NULL, 0);
      rt = XmRenderTableAddRenditions(rt, r_rend, r_index, XmMERGE_NEW);
      XtVaSetValues(FwdList[i], XmNrenderTable, rt, NULL, NULL);
      XmRenderTableFree(rt);
    }

  XtManageChildren(FwdList, 4);
}

#if 0
main(int ac, char **av)
{
  Arg args[20] ;
  Cardinal n;
  Widget toplevel;
  Widget rc;
  Widget bp;
  Widget EditUsrDialog;
  XtAppContext app_context;

  toplevel = XtAppInitialize(&app_context, "TM", NULL, 0,
			     &ac, av, NULL,NULL, 0);
  n = 0;
  XtSetArg(args[n], XmNallowShellResize, True);n++;
  XtSetValues(toplevel, args, n);

  EditUsrDialog = EditUser(toplevel);

  rc = XmCreateRowColumn(toplevel, "rc", NULL, 0);

  bp = XmCreatePushButton(rc, "Edit User", NULL, 0);
  XtAddCallback(bp, XmNactivateCallback, EditUsrCB, (XtPointer)EditUsrDialog);
  XtManageChild(bp);
  XtManageChild(rc);

  XtRealizeWidget(toplevel);

  /* Main Loop */
  XtAppMainLoop(app_context);
}
#endif

void AddPendingLine(char *call, int priv, int bull, int kb)
{
  char str[80];
  XmString string;

  if (*call)
    {
      sprintf(str, "%-6s:%d/%d-%dk", call, priv, bull, kb);
      
      if (priv)
	string = XmStringGenerate(str, NULL, XmCHARSET_TEXT, "RC");
      else if (bull)
	string = XmStringGenerate(str, NULL, XmCHARSET_TEXT, "BC");
      else
	string = XmStringGenerate(str, NULL, XmCHARSET_TEXT, "NO");
    }
  else
    {
      str[0] = '.';
      str[1] = '\0';
      string = XmStringCreateSimple(str);
    }

  XmListAddItem(FwdList[LgList/20], string, 0);
  XmStringFree(string);     

  ++LgList;
}
