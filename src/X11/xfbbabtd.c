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

#include <serv.h>
#include <xfbb.h>

static Widget about_dialog = NULL;
static Widget copy_dialog = NULL;
static Widget info_dialog = NULL;
static Widget call_dialog = NULL;

static int info_canal;

static struct
{
  char		str[256];
  char		prenom[13];
  char		home[41];
  stat_ch	sta;
  int		niv1, niv2, niv3, canal, nbmess, nbnew, paclen, memoc;
  unsigned	flags;
} prev;

static Widget IYapp;
static Widget ICall;
static Widget IDigis;
static Widget IName;
static Widget IHome;
static Widget IChannel;
static Widget IPort;
static Widget IN1N2N3;
static Widget IFlags;
static Widget IPaclen;
static Widget IStatus;
static Widget IMem;
static Widget IBuf;
static Widget IRet;
static Widget IPerso;
static Widget IUnread;

static XmString StringCreate(char *text)
{
  XmString xmstr;
  char *deb;

  if ((text == NULL) || (text[0] == '\0'))
    {
      return(XmStringCreateSimple(""));
    }

  xmstr = (XmString)NULL;

  deb = text;
  while (*text)
    {
      if (*text == '\n')
	{
	  *text = '\0';
	  xmstr = XmStringConcat(xmstr, XmStringCreateSimple(deb));
	  xmstr = XmStringConcat(xmstr, XmStringSeparatorCreate());
	  *text++ = '\n';
	  deb = text;
	}
      else
	++text;
    }

  if (*deb)
    {
      xmstr = XmStringConcat(xmstr, XmStringCreateSimple(deb));
    }
  return(xmstr);
}

static char *XVersion(int dat)
{
  static char	prodVersion[80];
  char	sdate[30];
  
  if (dat)
    sprintf(sdate, " (%s)", date());
  else
    *sdate = '\0';
  
  sprintf(prodVersion, "%s%s", version(), sdate) ;
  
  return(prodVersion);
}

static void OkCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget *pw;

  pw = (Widget *)client_data;

  XtUnmanageChild(*pw);
  XtDestroyWidget(XtParent(*pw));
  *pw = NULL;;
}

static void ApplyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget *pw;
  Widget wt;
  char *ptr;
  pw = (Widget *)client_data;

  /*  wt = XmSelectionBoxGetChild(call_dialog, XmDIALOG_TEXT); */
  wt = XmSelectionBoxGetChild(*pw, XmDIALOG_TEXT);

  ptr = XmTextGetString(wt);

  if (!set_callsign(ptr))
    {
      char texte[80];
      sprintf(texte, "%s is not a valid callsign !", ptr);
      MessageBox(60, texte, "Change callsign", MB_ICONEXCLAMATION|MB_OK);
      get_callsign(texte);
      XmTextSetString(wt, texte);
    }
  else
    {
      XtUnmanageChild(*pw);
      XtDestroyWidget(XtParent(*pw));
      *pw = NULL;;
    }
  XtFree(ptr);
}

void CallsignDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
  char buffer[512];
  Arg args[20] ;
  Cardinal n;

  if (call_dialog)
    {
      /* Dialog deja ouvert, on le passe devant */
      XRaiseWindow(XtDisplay(call_dialog),XtWindow(XtParent(call_dialog)));
      return;
    }

  n = 0;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  XtSetArg(args[n], XmNmessageAlignment, XmALIGNMENT_CENTER);n++;
  call_dialog = XmCreatePromptDialog(toplevel, "callsign", args, n);

  XtManageChild(XmSelectionBoxGetChild(call_dialog, XmDIALOG_APPLY_BUTTON));
  XtUnmanageChild(XmSelectionBoxGetChild(call_dialog, XmDIALOG_OK_BUTTON));
  XtUnmanageChild(XmSelectionBoxGetChild(call_dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback(call_dialog, XmNapplyCallback, ApplyCB, (XtPointer)&call_dialog);
  XtAddCallback(call_dialog, XmNcancelCallback, OkCB, (XtPointer)&call_dialog);

  get_callsign(buffer);
  XmTextSetString(XmSelectionBoxGetChild(call_dialog, XmDIALOG_TEXT), buffer);
  XtManageChild(call_dialog);
}

void AboutDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
  char buffer[512];
  Arg args[20] ;
  Cardinal n;
  XmString string;

  if (about_dialog)
    {
      /* Dialog deja ouvert, on le passe devant */
      XRaiseWindow(XtDisplay(about_dialog),XtWindow(XtParent(about_dialog)));
      return;
    }

  sprintf(buffer, 
	  "XFBB (Linux version)\n\nVersion %s\n\n"
	  "Copyright 1986-1996. All rights reserved."
	  "\n",
	  XVersion(TRUE));

  string = StringCreate(buffer);

  n = 0;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  XtSetArg(args[n], XmNmessageAlignment, XmALIGNMENT_CENTER);n++;
  XtSetArg(args[n], XmNmessageString, string);n++;
  about_dialog = XmCreateMessageDialog(toplevel, "about", args, n);

  XmStringFree(string);

  XtUnmanageChild(XmMessageBoxGetChild(about_dialog, XmDIALOG_CANCEL_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(about_dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback(about_dialog, XmNokCallback, OkCB, (XtPointer)&about_dialog);

  XtManageChild(about_dialog);
}

void CopyDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
  char buffer[1024];
  Arg args[20] ;
  Cardinal n;
  XmString string;
 
  if (copy_dialog)
    {
      /* Dialog deja ouvert, on le passe devant */
      XRaiseWindow(XtDisplay(copy_dialog),XtWindow(XtParent(copy_dialog)));
      return;
    }

  sprintf(buffer,
	  "\n"
	  "         AX25 BBS software  -  XFBB version %s\n"
	  "         (C) F6FBB 1986-1996       (%s)\n\n"
	  "This software is in the public domain. It can be copied or\n"
	  "installed only for amateur use abiding by the laws.\n\n"
	  "All commercial or professional use is prohibited.\n\n"
	  "F6FBB (Jean-Paul ROUBELAT) declines any responsibilty\n"
	  "in the use of XFBB software.\n\n"
	  "This software is free of charge, but a 100 FF or 20 US $\n"
	  "(or more) contribution will be appreciated.\n\n",
	  XVersion(0), date()
	  ) ;
  
  string = StringCreate(buffer);
  
n = 0;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  XtSetArg(args[n], XmNmessageString, string);n++;
  copy_dialog = XmCreateMessageDialog(toplevel, "copyright", args, n);

  XmStringFree(string);

  XtUnmanageChild(XmMessageBoxGetChild(copy_dialog, XmDIALOG_CANCEL_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(copy_dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback(copy_dialog, XmNokCallback, OkCB, (XtPointer)&copy_dialog);

  XtManageChild(copy_dialog);
}

static void ShowCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("ShowCB\n");
  if (info_canal != -1)
    ShowFbbWindow(info_canal, toplevel);
}

static Widget CreateInfoLabel(Widget parent, char *name, char *val, uchar alig)
{
  Arg args[10];
  Cardinal n;
  /*   XmString string; */
  Widget w;
  
  n = 0;
  /*  string = XmStringCreateSimple(val);
  XtSetArg(args[n], XmNlabelString, string);n++; */
  XtSetArg(args[n], XmNalignment, alig);n++;
  XtSetArg(args[n], XmNmarginHeight, 0);n++;
  w = XmCreateLabel(parent, name, args, n);
  /*   XmStringFree(string); */
  XtManageChild(w);

  return(w);
}

void InfoDialog(Widget w, XtPointer client_data, XtPointer call_data)
{
  Arg args[20] ;
  Cardinal n;
  Widget rc;
  int nb;

  /* Lecture du canal courant */
  nb = GetChList();

  if (nb != -1)
    info_canal = nb;

  if (info_dialog)
    {
      /* Dialog deja ouvert, on le passe devant */
      XRaiseWindow(XtDisplay(info_dialog),XtWindow(XtParent(info_dialog)));
      return;
    }

  if (nb == -1)
    return;

  memset(&prev, 0xff, sizeof(prev));

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 5);n++;
  XtSetArg(args[n], XmNmarginWidth, 5);n++;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  info_dialog = XmCreateFormDialog(toplevel, "infos", args, n);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 20);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 40);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(info_dialog, "show", args, n);
  XtAddCallback(w, XmNactivateCallback, ShowCB  , 0);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 60);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 80);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(info_dialog, "close", args, n);
  XtAddCallback(w, XmNactivateCallback, OkCB, (XtPointer)&info_dialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, w);n++;
  XtSetArg(args[n], XmNbottomOffset, 10);n++;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING);n++;
  IYapp = XmCreateLabel(info_dialog, "info_label", args, n);
  XtManageChild(IYapp);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, IYapp);n++;
  XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);n++;
  XtSetArg(args[n], XmNnumColumns, 2);n++;
  XtSetArg(args[n], XmNmarginHeight, 0);n++;
  XtSetArg(args[n], XmNisAligned, FALSE);n++;
  rc = XmCreateRowColumn(info_dialog, "rc_info", args, n);

  CreateInfoLabel(rc, "callsign", "Callsign :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "digis", "Digis :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "name", "Name :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "home_bbs", "Home BBS :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "l_separator", "", XmALIGNMENT_END);
  CreateInfoLabel(rc, "channel", "Channel :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "port", "Port :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "n1_n2_n3", "N1,N2,N3 :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "flags", "Flags :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "paclen", "Paclen :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "status", "Status :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "l_separator", "", XmALIGNMENT_END);
  CreateInfoLabel(rc, "mem_used", "Memory used :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "buffers", "Buffers :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "retries", "Retries :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "l_separator", "", XmALIGNMENT_END);
  CreateInfoLabel(rc, "priv_msg", "Personnal messages :", XmALIGNMENT_END);
  CreateInfoLabel(rc, "unread_msg", "Unread messages :", XmALIGNMENT_END);

  ICall    = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IDigis   = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IName    = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IHome    = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
             CreateInfoLabel(rc, "r_separator", "", XmALIGNMENT_BEGINNING);
  IChannel = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IPort    = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IN1N2N3  = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IFlags   = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IPaclen  = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IStatus  = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
             CreateInfoLabel(rc, "r_separator", "", XmALIGNMENT_BEGINNING);
  IMem     = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IBuf     = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IRet     = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
             CreateInfoLabel(rc, "r_separator", "", XmALIGNMENT_BEGINNING);
  IPerso   = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);
  IUnread  = CreateInfoLabel(rc, "info_data", "", XmALIGNMENT_BEGINNING);

  XtManageChild(rc);

  DisplayInfoDialog(nb);

  XtManageChild(info_dialog);
}

static void InfoLabel(Widget w, char *txt)
{
  XmString  string;

  string = XmStringCreateSimple(txt);
  XtVaSetValues(w, XmNlabelString, string, NULL);
  XmStringFree(string);
}

void DisplayInfoDialog(int voie)
{
  int		i;
  char		str[256];
  char		call[80];
  indicat	*ind;
  
  if ((info_dialog == NULL) || (voie != info_canal))
    return;

  ind = &svoie[info_canal]->sta.indicatif;
  if (memcmp(ind, &prev.sta.indicatif, sizeof(indicat)) != 0)
    {
      prev.sta.indicatif = *ind;
      if (ind->num)
	sprintf(call, "%s-%d", ind->call, ind->num);
      else
	sprintf(call, "%s", ind->call);
      InfoLabel(ICall, call);
      
      *str = '\0';
      for (i = 0 ; i < 2 ; i++)
	{
	  ind = &svoie[info_canal]->sta.relais[i];
	  if (*(ind->call))
	    
	    {
	      if (ind->num)
		sprintf(call, "%s-%d", ind->call, ind->num);
	      else
		sprintf(call, "%s", ind->call);
	      if (i > 0)
		strcat(str, ",");
	      strcat(str, call);
	    }
	}
      ind = &svoie[info_canal]->sta.relais[i];
      if (*(ind->call))
	strcat(str, ",...");
      InfoLabel(IDigis, str);
    }
  
  if (strcmp(prev.prenom, svoie[info_canal]->finf.prenom) != 0)
    {
      strcpy(prev.prenom, svoie[info_canal]->finf.prenom);
      InfoLabel(IName, svoie[info_canal]->finf.prenom);
    }
  
  if (strcmp(prev.home, svoie[info_canal]->finf.home) != 0)
    {
      strcpy(prev.home, svoie[info_canal]->finf.home);
      InfoLabel(IHome, svoie[info_canal]->finf.home);
    }
  
  if (info_canal != prev.canal)
    {
      prev.canal = info_canal;
      InfoLabel(IChannel, itoa((info_canal > 0) ? info_canal-1 : info_canal, str, 10));
      InfoLabel(IPort, itoa(no_port(info_canal), str, 10));
    }
  
  if ((svoie[info_canal]->niv1 != prev.niv1) ||
      (svoie[info_canal]->niv2 != prev.niv2) ||
      (svoie[info_canal]->niv3 != prev.niv3))
    {
      prev.niv1 = svoie[info_canal]->niv1;
      prev.niv2 = svoie[info_canal]->niv2;
      prev.niv3 = svoie[info_canal]->niv3;
      sprintf(str, "%02d %02d %02d",
	       svoie[info_canal]->niv1, svoie[info_canal]->niv2, svoie[info_canal]->niv3);
      InfoLabel(IN1N2N3, str);
    }
  
  if (svoie[info_canal]->finf.flags != prev.flags)
    {
      prev.flags = svoie[info_canal]->finf.flags;
      InfoLabel(IFlags, strflags(&svoie[info_canal]->finf));
    }
  
  if (svoie[info_canal]->paclen != prev.paclen)
    {
      prev.paclen = svoie[info_canal]->paclen;
      InfoLabel(IPaclen, itoa(svoie[info_canal]->paclen, str, 10));
    }
  
  if (svoie[info_canal]->sta.stat != prev.sta.stat)
    {
      prev.sta.stat = svoie[info_canal]->sta.stat;
      InfoLabel(IStatus, stat_voie(info_canal));
    }
  
  if (svoie[info_canal]->memoc != prev.memoc)
    {
      prev.memoc = svoie[info_canal]->memoc;
      InfoLabel(IMem, itoa(svoie[info_canal]->memoc, str, 10));
    }
  
  if (svoie[info_canal]->sta.ack != prev.sta.ack)
    {
      prev.sta.ack = svoie[info_canal]->sta.ack;
      InfoLabel(IBuf, itoa(svoie[info_canal]->sta.ack, str, 10));
    }
  
  if (svoie[info_canal]->sta.ret != prev.sta.ret)
    {
      prev.sta.ret = svoie[info_canal]->sta.ret;
      InfoLabel(IRet, itoa(svoie[info_canal]->sta.ret, str, 10));
    }
  
  if (svoie[info_canal]->ncur)
    {
      if (svoie[info_canal]->ncur->nbmess != prev.nbmess)
	{
	  prev.nbmess = svoie[info_canal]->ncur->nbmess;
	  InfoLabel(IPerso, itoa(svoie[info_canal]->ncur->nbmess, str, 10));
	}
      if (svoie[info_canal]->ncur->nbnew != prev.nbnew)
	{
	  prev.nbnew = svoie[info_canal]->ncur->nbnew;
	  InfoLabel(IUnread, itoa(svoie[info_canal]->ncur->nbnew, str, 10));
	}
    }
  else
    {
      InfoLabel(IPerso, "");
      InfoLabel(IUnread, "");
    }
  
  if (svoie[info_canal]->niv1 == N_YAPP)
    yapp_str(info_canal, str);
#ifndef __linux__
  else if (svoie[info_canal]->niv1 == N_MOD)
    xmodem_str(info_canal, str);
#endif
  else if (svoie[info_canal]->niv1 == N_BIN)
    abin_str(info_canal, str);
  else if (svoie[info_canal]->niv1 == N_XFWD)
    xfwd_str(info_canal, str);
  else
    *str = '\0';
  
  if (strcmp(str, prev.str) != 0)
    {
      strcpy(prev.str, str);
      InfoLabel(IYapp, str);
    }
}
