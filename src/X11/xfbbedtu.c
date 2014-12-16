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

#include <xfbb.h>
#include <serv.h>

#define LG_TBLIST 100
static int LgList;
static char *TbList;
static int NbLang;

static Widget EditUsrDialog;
static Widget Callsign;
static Widget CallList;
static Widget LangList;
static Widget Add;
static Widget Del;
static Widget Home;
static Widget Name;
static Widget Pass;
static Widget Prvt;
static Widget Zip;
static Widget Stat[11];

static int AddMode;
static info user;

void EditUsrCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int i;
  XmString string;

  if (XtIsManaged(EditUsrDialog))
    {
      /* Passer la fenetre au 1er plan */
      XRaiseWindow(XtDisplay(EditUsrDialog),XtWindow(XtParent(EditUsrDialog)));
      return;
    }

  AddMode = FALSE;

  cursor_wait();

  NbLang = 0;
  XmListDeleteAllItems(CallList);
  XmListDeleteAllItems(LangList);

  FbbRequestUserList();

  /* Tri de la liste */
  qsort(TbList, LgList, 7, (int (*)(const void *, const void *))strcmp);

  for (i = 0 ; i < LgList ; i++)
    {
      string = XmStringCreateSimple(TbList + (i * 7));
      XmListAddItem(CallList, string, 0);
      XmStringFree(string);
    }

  /* Liberation de la liste */
  free(TbList);
  LgList = 0;
  TbList = NULL;

  XmListSelectPos(CallList, 1, TRUE);
  XtManageChild(EditUsrDialog);
  end_wait();
}

void TextFieldGetString(Widget w, char *str, int len, int upcase)
{
  char *text;

  text = XmTextFieldGetString(w);
  if (upcase)
    {
      strn_cpy(len, str, text);
    }
  else
    {
      n_cpy(len, str, text);
    }
  XtFree(text);
}

void RefreshUser(void)
{
  int i;
  XmString string;

  cursor_wait();

  XmListDeleteAllItems(CallList);
  XmListDeleteAllItems(LangList);

  NbLang = 0;
  FbbRequestUserList();

  /* Tri de la liste */
  qsort(TbList, LgList, 7, (int (*)(const void *, const void *))strcmp);

  for (i = 0 ; i < LgList ; i++)
    {
      string = XmStringCreateSimple(TbList + (i * 7));
      XmListAddItem(CallList, string, 0);
      XmStringFree(string);
    }

  /* Liberation de la liste */
  free(TbList);
  LgList = 0;
  TbList = NULL;

  string = XmStringCreateSimple(user.indic.call);
  XmListSelectItem(CallList, string, TRUE);
  XmListSetItem(CallList, string);
  XmStringFree(string);
  end_wait();
}

static void ApplyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int count;
  int *list;

  if (AddMode)
    {
      Widget scroll = NULL;

      /* Inserer le nouvel indicatif */
      char Call[8];
      indicat indic;
      unsigned num_indic;
      FILE *fptr ;
      info frec ;

      /* Lire l'indicatif */
      TextFieldGetString(Callsign, Call, 7, TRUE);
      if (!find(Call))
	{
	  char text[80];

	  /* Erreur call */
	  sprintf(text, "Invalid callsign \"%s\"", Call);
	  MessageBox(20, text, "Add User", MB_ICONEXCLAMATION|MB_OK);
	  return;
	}

      indic.num = 0;
      extind(Call, indic.call);
      pvoie->emis = insnoeud(indic.call, &num_indic) ;

      if (pvoie->emis->coord == 0xffff)
	{
	  /* L'indicatif n'existe pas ... Creer le record */
	  pvoie->emis->coord = rinfo++;
	  pvoie->emis->val = 1;
	  strcpy(pvoie->emis->indic, Call);
	  
	  init_info(&frec, &indic) ;
	  
	  fptr = ouvre_nomenc() ;
	  fseek(fptr, (long)pvoie->emis->coord * ((long) sizeof(info)), 0) ;
	  fwrite((char *) & frec, (int) sizeof(info), 1, fptr) ;
	  ferme(fptr, 40) ;
	}
      GetUserInfos(indic.call, &user);

      /* Devalider l'indicatif */
      XtUnmapWidget(Callsign);

      /* Revalider la liste ... */
      XtMapWidget(CallList);
      XtVaGetValues(CallList, XmNverticalScrollBar, &scroll, NULL);
      if (scroll)
	XtMapWidget(scroll);
      XtSetSensitive(Add, TRUE);
      XtSetSensitive(Del, TRUE);
      AddMode = FALSE;
    }

  /* Modification de l'utilisateur */
  
  XmListGetSelectedPos(LangList, &list, &count);
  if (count)
    {
      user.lang = list[0]-1;
      free(list);
    }
  
  TextFieldGetString(Name, user.prenom, 12, FALSE);
  TextFieldGetString(Pass, user.pass,   12, TRUE);
  TextFieldGetString(Zip,  user.zip,    8,  TRUE);
  TextFieldGetString(Prvt, user.priv,   12, TRUE);
  TextFieldGetString(Home, user.home,   40, TRUE);
  
  user.flags = 0;
  if (XmToggleButtonGetState(Stat[0]))  user.flags |= F_PRV;
  if (XmToggleButtonGetState(Stat[1]))  user.flags |= F_PAG;
  if (XmToggleButtonGetState(Stat[2]))  user.flags |= F_BBS;
  if (XmToggleButtonGetState(Stat[3]))  user.flags |= F_PMS;
  if (XmToggleButtonGetState(Stat[4]))  user.flags |= F_SYS;
  if (XmToggleButtonGetState(Stat[5]))  user.flags |= F_EXP;
  if (XmToggleButtonGetState(Stat[6]))  user.flags |= F_LOC;
  if (XmToggleButtonGetState(Stat[7]))  user.flags |= F_EXC;
  if (XmToggleButtonGetState(Stat[8]))  user.flags |= F_MOD;
  if (XmToggleButtonGetState(Stat[9]))  user.flags |= F_UNP;
  if (XmToggleButtonGetState(Stat[10])) user.flags |= F_NEW;
  
  SetUserInfos(user.indic.call, &user);
  RefreshUser();
}

static void CloseCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if (AddMode)
    {
      Widget scroll = NULL;

      /* Devalider l'indicatif */
      XtUnmapWidget(Callsign);

      XtMapWidget(CallList);
      XtVaGetValues(CallList, XmNverticalScrollBar, &scroll, NULL);
      if (scroll)
	XtMapWidget(scroll);
      XtSetSensitive(Add, TRUE);
      XtSetSensitive(Del, TRUE);
      AddMode = FALSE;
    }
  XtUnmanageChild((Widget)client_data);
  XmListDeleteAllItems(CallList);
  XmListDeleteAllItems(LangList);
}

static void AddUserCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget scroll = NULL;

  XtVaGetValues(CallList, XmNverticalScrollBar, &scroll, NULL);
  XtUnmapWidget(CallList);
  if (scroll)
    XtUnmapWidget(scroll);

  /* Valider l'indicatif */
  XtMapWidget(Callsign);
  XmTextFieldSetString(Callsign, "CALLSIGN");

  XtSetSensitive(Add, FALSE);
  XtSetSensitive(Del, FALSE);
  AddMode = TRUE;

  XmListDeselectAllItems(LangList);
  XmTextFieldSetString(Name, "");
  XmTextFieldSetString(Pass, "");
  XmTextFieldSetString(Zip,  "");
  XmTextFieldSetString(Prvt, "");
  XmTextFieldSetString(Home, "");
  XmToggleButtonSetState(Stat[0], (PRV(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[1], (PAG(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[2], (BBS(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[3], (PMS(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[4], (SYS(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[5], (EXP(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[6], (LOC(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[7], (EXC(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[8], (MOD(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[9], (UNP(def_mask) != 0), FALSE);
  XmToggleButtonSetState(Stat[10],(NEW(def_mask) != 0), FALSE);
}

static void DelUserCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  XmString string;
  ind_noeud *noeud;
  unsigned num_indic;
  FILE *fptr ;
  info frec ;
  char text[80];

  /* Message box ... */

  sprintf(text, "Delete %s. Are you sure ?", user.indic.call);
  if (!MessageBox(20, text, "Delete user", MB_ICONQUESTION|MB_OKCANCEL))
    return;
   
  noeud = insnoeud(user.indic.call, &num_indic) ;
  if (noeud->coord == 0xffff)
    return;

  string = XmStringCreateSimple(user.indic.call);
  XmListDeleteItem(CallList, string);
  XmStringFree(string);

  fptr = ouvre_nomenc() ;
  fseek(fptr, (long)noeud->coord * sizeof(info) , 0) ;
  fread((char *) & frec, sizeof(info), 1, fptr) ;
  *(frec.indic.call) = '\0' ;
  fseek(fptr, (long)noeud->coord * sizeof(info) , 0) ;
  fwrite((char *) & frec, (int) sizeof(info), 1, fptr) ;
  ferme(fptr, 41) ;
  noeud->coord = 0xffff ;

  XmListSelectPos(CallList, 1, TRUE);
}

static void SelectCallCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  XmListCallbackStruct *cd = (XmListCallbackStruct *)call_data;
  char *callsign;

  XmStringGetLtoR(cd->item, XmSTRING_DEFAULT_CHARSET, &callsign);

  if (!GetUserInfos(callsign, &user))
    return;

  if (user.lang < NbLang)
    XmListSelectPos(LangList, user.lang+1, FALSE);
  else
    XmListDeselectAllItems(LangList);
  XmTextFieldSetString(Name, user.prenom);
  XmTextFieldSetString(Pass, user.pass);
  XmTextFieldSetString(Zip, user.zip);
  XmTextFieldSetString(Prvt, user.priv);
  XmTextFieldSetString(Home, user.home);
  XmToggleButtonSetState(Stat[0], (PRV(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[1], (PAG(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[2], (BBS(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[3], (PMS(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[4], (SYS(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[5], (EXP(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[6], (LOC(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[7], (EXC(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[8], (MOD(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[9], (UNP(user.flags) != 0), FALSE);
  XmToggleButtonSetState(Stat[10],(NEW(user.flags) != 0), FALSE);
}

static void SelectLangCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  XmListCallbackStruct *cd = (XmListCallbackStruct *)call_data;
  char *text;

  XmStringGetLtoR(cd->item, XmSTRING_DEFAULT_CHARSET, &text);
}

void EditUser(Widget toplevel)
{
  Arg args[20] ;
  Cardinal n;
  Widget w;
  Widget rcHome;
  Widget rcPrvt;
  Widget rcZip;
  Widget rcPass;
  Widget rcName;
  Widget rcStat;

  LgList = 0;
  TbList = NULL;

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 5);n++;
  XtSetArg(args[n], XmNmarginWidth, 5);n++;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  EditUsrDialog = XmCreateFormDialog(toplevel, "edit_user", args, n);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNmappedWhenManaged, FALSE);n++;
  XtSetArg(args[n], XmNcolumns, 8);n++;
  Callsign = XmCreateTextField(EditUsrDialog, "callsign", args, n);
  XtManageChild(Callsign);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 2);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 22);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditUsrDialog, "apply", args, n);
  XtAddCallback(w, XmNactivateCallback, ApplyCB, 0);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 27);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 47);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  Add = XmCreatePushButton(EditUsrDialog, "add_user", args, n);
  XtAddCallback(Add, XmNactivateCallback, AddUserCB, 0);
  XtManageChild(Add);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 52);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 72);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  Del = XmCreatePushButton(EditUsrDialog, "del_user", args, n);
  XtAddCallback(Del, XmNactivateCallback, DelUserCB, 0);
  XtManageChild(Del);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 77);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 97);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditUsrDialog, "close", args, n);
  XtAddCallback(w, XmNactivateCallback, CloseCB, (XtPointer)EditUsrDialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, w);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rcHome = XmCreateRowColumn(EditUsrDialog, "rcHome", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 70);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rcHome, "home_bbs", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 40);n++;
  Home = XmCreateTextField(rcHome, "TextF", args, n);
  XtManageChild(Home);

  XtManageChild(rcHome);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcHome);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rcPrvt = XmCreateRowColumn(EditUsrDialog, "rcPriv", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 70);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rcPrvt, "private_dir", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 40);n++;
  Prvt = XmCreateTextField(rcPrvt, "TextF", args, n);
  XtManageChild(Prvt);

  XtManageChild(rcPrvt);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcPrvt);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rcZip = XmCreateRowColumn(EditUsrDialog, "rcZip", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 70);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rcZip, "zip_code", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 20);n++;
  Zip = XmCreateTextField(rcZip, "TextF", args, n);
  XtManageChild(Zip);

  XtManageChild(rcZip);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcZip);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rcPass = XmCreateRowColumn(EditUsrDialog, "rcPass", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 70);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rcPass, "password", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 20);n++;
  Pass = XmCreateTextField(rcPass, "TextF", args, n);
  XtManageChild(Pass);

  XtManageChild(rcPass);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcPass);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rcName = XmCreateRowColumn(EditUsrDialog, "rcName", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 70);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rcName, "name", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 20);n++;
  Name = XmCreateTextField(rcName, "TextF", args, n);
  XtManageChild(Name);

  XtManageChild(rcName);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNtopOffset, 5);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftOffset, 5);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcName);n++;
  XtSetArg(args[n], XmNbottomOffset, 5);n++;
  XtSetArg(args[n], XmNvisibleItemCount, 10);n++;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  CallList = XmCreateScrolledList(EditUsrDialog, "callsign_list", args, n);
  XtAddCallback(CallList, XmNbrowseSelectionCallback, SelectCallCB, NULL);
  XtManageChild(CallList);
  
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNtopOffset, 5);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNleftOffset, 5);n++;
  XtSetArg(args[n], XmNleftWidget, CallList);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcName);n++;
  XtSetArg(args[n], XmNbottomOffset, 5);n++;
  XtSetArg(args[n], XmNvisibleItemCount, 10);n++;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  LangList = XmCreateScrolledList(EditUsrDialog, "language_list", args, n);
  XtAddCallback(LangList, XmNbrowseSelectionCallback, SelectLangCB, NULL);
  XtManageChild(LangList);
  
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNleftOffset, 5);n++;
  XtSetArg(args[n], XmNleftWidget, rcName);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcPrvt);n++;
  XtSetArg(args[n], XmNbottomOffset, 5);n++;
  XtSetArg(args[n], XmNradioBehavior, FALSE);n++;
  rcStat = XmCreateRadioBox(EditUsrDialog, "rcStat", args, n);

  n = 0;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  Stat[0] = XmCreateToggleButton(rcStat, "read_all", args, n);
  Stat[1] = XmCreateToggleButton(rcStat, "paging", args, n);
  Stat[2] = XmCreateToggleButton(rcStat, "bbs", args, n);
  Stat[3] = XmCreateToggleButton(rcStat, "pms", args, n);
  Stat[4] = XmCreateToggleButton(rcStat, "sysop", args, n);
  Stat[5] = XmCreateToggleButton(rcStat, "expert", args, n);
  Stat[6] = XmCreateToggleButton(rcStat, "local", args, n);
  Stat[7] = XmCreateToggleButton(rcStat, "excluded", args, n);
  Stat[8] = XmCreateToggleButton(rcStat, "modem", args, n);
  Stat[9] = XmCreateToggleButton(rcStat, "unproto", args, n);
  Stat[10]= XmCreateToggleButton(rcStat, "private_msg", args, n);

  XtManageChildren(Stat, 11);
  XtManageChild(rcStat);
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

void AddUserList(char *callsign)
{
  if ((LgList % LG_TBLIST) == 0)
    {
      if (LgList)
	TbList = realloc(TbList, (LgList + LG_TBLIST) * 7);
      else
	TbList = malloc(LG_TBLIST * 7);
    }

  strn_cpy(6, TbList + (LgList * 7), callsign);
  ++LgList;
}

void AddUserLang(char *langue)
{
  XmString string;

  string = XmStringCreateSimple(langue);
  XmListAddItem(LangList, string, 0);
  XmStringFree(string);
  ++NbLang;
}
