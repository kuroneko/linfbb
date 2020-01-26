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

static Widget MsgList;
static Widget EditMsgDialog;
static Widget From;
static Widget Bid;
static Widget To;
static Widget Via;
static Widget Title;
static Widget Sent;
static Widget Recv;
static Widget Size;
static Widget EditFwdDialog;
static Widget tog_w;
static Widget tog_d;
static Widget fwd_toggle[NBBBS];
static int fwd_t_val[NBBBS];

static Widget Type[4];
static Widget Stat[7];
static Widget Data;

static int on_w;
static int on_d;
static Pixel color_w;
static Pixel color_d;

static bullist bullig;

static void EditFwdStatus(void);

void EditMsgCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if (XtIsManaged(EditMsgDialog))
    {
      /* Passer la fenetre au 1er plan */
      XRaiseWindow(XtDisplay(EditMsgDialog),XtWindow(XtParent(EditMsgDialog)));
      return;
    }

  cursor_wait();
  printf("EditMsgCB\n");

  FbbRequestMessageList();

  XmListSelectPos(MsgList, 1, TRUE);
  /* XmListSetBottomPos(MsgList, 0); */
  XtManageChild(EditMsgDialog);
  end_wait();
}


static void ApplyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  TextFieldGetString(From, bullig.exped,  6, TRUE);
  TextFieldGetString(Bid,  bullig.bid,   12, TRUE);
  TextFieldGetString(To,   bullig.desti,  6, TRUE);
  TextFieldGetString(Via,  bullig.bbsv,  40, TRUE);
  TextFieldGetString(Title,bullig.titre, 60, FALSE);
  
  if (XmToggleButtonGetState(Type[0])) bullig.type   = 'A';
  if (XmToggleButtonGetState(Type[1])) bullig.type   = 'B';
  if (XmToggleButtonGetState(Type[2])) bullig.type   = 'P';
  if (XmToggleButtonGetState(Type[3])) bullig.type   = 'T';
  if (XmToggleButtonGetState(Stat[0])) bullig.status = '$';
  if (XmToggleButtonGetState(Stat[1])) bullig.status = 'N';
  if (XmToggleButtonGetState(Stat[2])) bullig.status = 'Y';
  if (XmToggleButtonGetState(Stat[3])) bullig.status = 'F';
  if (XmToggleButtonGetState(Stat[4])) bullig.status = 'X';
  if (XmToggleButtonGetState(Stat[5])) bullig.status = 'K';
  if (XmToggleButtonGetState(Stat[6])) bullig.status = 'A';
  
  SetMsgInfo(&bullig, bullig.numero);
}

static void CloseCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("CloseCB\n");
  XtUnmanageChild((Widget)client_data);
  XmListDeleteAllItems(MsgList);
}

static void EditFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("EditFwdCB\n");
  EditFwdStatus();
}

static void EditTxtCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  char nom[80];
  char numero[40];
  char msgname[256];

  printf("EditTxtCB\n");

  sprintf(numero, "Msg #%ld", bullig.numero);
  strcpy(msgname, mess_name(MessPath(), bullig.numero, nom));
  EditorOff = FALSE;
  CreateEditor(msgname, NULL, numero, 0, TRUE);
}

static void SelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  XmListCallbackStruct *cd = (XmListCallbackStruct *)call_data;
  char *text;
  long nb;
  char buf[80];

  XmStringGetLtoR(cd->item, XmSTRING_DEFAULT_CHARSET, &text);
  sscanf(text, "%ld", &nb);
  printf("SelectCB %ld\n", nb);

  if (!GetMsgInfos(&bullig, nb))
    return;

  XmTextFieldSetString(From, bullig.exped);
  XmTextFieldSetString(To, bullig.desti);
  XmTextFieldSetString(Via, bullig.bbsv);
  XmTextFieldSetString(Title, bullig.titre);
  XmTextFieldSetString(Bid, bullig.bid);
  sprintf(buf,"%ld", bullig.taille);
  XmTextFieldSetString(Size, buf);
  XmTextFieldSetString(Recv, strdt(bullig.date));
  XmTextFieldSetString(Sent, strdt(bullig.datesd));
  XmToggleButtonSetState(Type[0], (bullig.type   == 'A'), FALSE);
  XmToggleButtonSetState(Type[1], (bullig.type   == 'B'), FALSE);
  XmToggleButtonSetState(Type[2], (bullig.type   == 'P'), FALSE);
  XmToggleButtonSetState(Type[3], (bullig.type   == 'T'), FALSE);
  XmToggleButtonSetState(Stat[0], (bullig.status == '$'), FALSE);
  XmToggleButtonSetState(Stat[1], (bullig.status == 'N'), FALSE);
  XmToggleButtonSetState(Stat[2], (bullig.status == 'Y'), FALSE);
  XmToggleButtonSetState(Stat[3], (bullig.status == 'F'), FALSE);
  XmToggleButtonSetState(Stat[4], (bullig.status == 'X'), FALSE);
  XmToggleButtonSetState(Stat[5], (bullig.status == 'K'), FALSE);
  XmToggleButtonSetState(Stat[6], (bullig.status == 'A'), FALSE);
  XmToggleButtonSetState(Data, (bullig.bin), FALSE);
}

void EditMessage(Widget toplevel)
{
  Arg args[20] ;
  Cardinal n;
  Widget rcl;
  Widget rcr;
  Widget rc2;
  Widget rc3;
  Widget rc4;
  Widget w;
  Widget f;
  Widget rcTitle;
  Widget rcVia;
  Widget rcType;
  Widget rcData;
  Widget rcStat;

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 5);n++;
  XtSetArg(args[n], XmNmarginWidth, 5);n++;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  EditMsgDialog = XmCreateFormDialog(toplevel, "edit_msg", args, n);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 2);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 22);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditMsgDialog, "apply", args, n);
  XtAddCallback(w, XmNactivateCallback, ApplyCB, 0);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 27);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 47);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditMsgDialog, "edit_fwd", args, n);
  XtAddCallback(w, XmNactivateCallback, EditFwdCB, 0);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 52);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 72);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditMsgDialog, "edit_text", args, n);
  XtAddCallback(w, XmNactivateCallback, EditTxtCB, 0);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 77);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 97);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditMsgDialog, "close", args, n);
  XtAddCallback(w, XmNactivateCallback, CloseCB, (XtPointer)EditMsgDialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, w);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rcTitle = XmCreateRowColumn(EditMsgDialog, "rcTitle", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rcTitle, "title", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 40);n++;
  Title = XmCreateTextField(rcTitle, "TextF", args, n);
  XtManageChild(Title);

  XtManageChild(rcTitle);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcTitle);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rcVia = XmCreateRowColumn(EditMsgDialog, "rcVia", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rcVia, "via", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 40);n++;
  Via = XmCreateTextField(rcVia, "TextF", args, n);
  XtManageChild(Via);

  XtManageChild(rcVia);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNrightPosition, 50);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcVia);n++;
  rcl = XmCreateForm(EditMsgDialog, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
  XtSetArg(args[n], XmNleftPosition, 50);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcVia);n++;
  rcr = XmCreateForm(EditMsgDialog, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rc2 = XmCreateRowColumn(rcl, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rc2, "to", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 15);n++;
  To = XmCreateTextField(rc2, "TextF", args, n);
  XtManageChild(To);

  XtManageChild(rc2);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rc2);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rc3 = XmCreateRowColumn(rcl, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rc3, "bid", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 15);n++;
  Bid = XmCreateTextField(rc3, "TextF", args, n);
  XtManageChild(Bid);

  XtManageChild(rc3);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rc3);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rc4 = XmCreateRowColumn(rcl, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rc4, "from", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 15);n++;
  From = XmCreateTextField(rc4, "TextF", args, n);
  XtManageChild(From);

  XtManageChild(rc4);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END);n++;
  w = XmCreateLabel(rcl, "msg", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNtopOffset, 5);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNleftOffset, 5);n++;
  XtSetArg(args[n], XmNleftWidget, w);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rc4);n++;
  XtSetArg(args[n], XmNbottomOffset, 5);n++;
  XtSetArg(args[n], XmNvisibleItemCount, 15);n++;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  MsgList = XmCreateScrolledList(rcl, "message_list", args, n);
  XtAddCallback(MsgList, XmNbrowseSelectionCallback, SelectCB, NULL);
  XtManageChild(MsgList);
  
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rc2 = XmCreateRowColumn(rcr, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rc2, "size", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 15);n++;
  XtSetArg(args[n], XmNeditable, FALSE);n++;
  XtSetArg(args[n], XmNcursorPositionVisible, FALSE);n++;
  Size = XmCreateTextField(rc2, "TextF", args, n);
  XtManageChild(Size);

  XtManageChild(rc2);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rc2);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rc3 = XmCreateRowColumn(rcr, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rc3, "recv", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 15);n++;
  XtSetArg(args[n], XmNeditable, FALSE);n++;
  XtSetArg(args[n], XmNcursorPositionVisible, FALSE);n++;
  Recv = XmCreateTextField(rc3, "TextF", args, n);
  XtManageChild(Recv);

  XtManageChild(rc3);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rc3);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNmarginWidth, 0);n++;
  rc4 = XmCreateRowColumn(rcr, "rc", args, n);

  n = 0;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(rc4, "sent", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNcolumns, 15);n++;
  XtSetArg(args[n], XmNeditable, FALSE);n++;
  XtSetArg(args[n], XmNcursorPositionVisible, FALSE);n++;
  Sent = XmCreateTextField(rc4, "TextF", args, n);
  XtManageChild(Sent);

  XtManageChild(rc4);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  f = XmCreateForm(rcr, "form", args, n);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  rcStat = XmCreateRadioBox(f, "rcStat", args, n);

  n = 0;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  Stat[0] = XmCreateToggleButton(rcStat, "$", args, n);
  Stat[1] = XmCreateToggleButton(rcStat, "N", args, n);
  Stat[2] = XmCreateToggleButton(rcStat, "Y", args, n);
  Stat[3] = XmCreateToggleButton(rcStat, "F", args, n);
  Stat[4] = XmCreateToggleButton(rcStat, "X", args, n);
  Stat[5] = XmCreateToggleButton(rcStat, "K", args, n);
  Stat[6] = XmCreateToggleButton(rcStat, "A", args, n);

  XtManageChildren(Stat, 7);
  XtManageChild(rcStat);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNrightWidget, rcStat);n++;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END);n++;
  w = XmCreateLabel(f, "status", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNrightWidget, rcStat);n++;
  XtSetArg(args[n], XmNrightOffset, 30);n++;
  rcType = XmCreateRadioBox(f, "rcType", args, n);

  n = 0;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  Type[0] = XmCreateToggleButton(rcType, "A", args, n);
  Type[1] = XmCreateToggleButton(rcType, "B", args, n);
  Type[2] = XmCreateToggleButton(rcType, "P", args, n);
  Type[3] = XmCreateToggleButton(rcType, "T", args, n);

  XtManageChildren(Type, 4);

  XtManageChild(rcType);

  n = 0;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, rcType);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNrightWidget, rcStat);n++;
  XtSetArg(args[n], XmNrightOffset, 30);n++;
  rcData = XmCreateRadioBox(f, "rcData", args, n);

  n = 0;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  XtSetArg(args[n], XmNshadowThickness, 0);n++;
  Data = XmCreateToggleButton(rcData, " ", args, n);
  XtManageChild(Data);
  XtSetSensitive(Data, FALSE);

  XtManageChild(rcData);


  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, rcType);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNrightWidget, rcType);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(f, "type", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, rcData);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNrightWidget, rcData);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNwidth, 40);n++;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END);n++;
  XtSetArg(args[n], XmNrecomputeSize, FALSE);n++;
  w = XmCreateLabel(f, "data", args, n);
  XtManageChild(w);

  XtManageChild(f);
  XtManageChild(rcl);
  XtManageChild(rcr); 
}

#if 0
main(int ac, char **av)
{
  Arg args[20] ;
  Cardinal n;
  Widget toplevel;
  Widget rc;
  Widget bp;
  Widget EditMsgDialog;
  XtAppContext app_context;

  toplevel = XtAppInitialize(&app_context, "TM", NULL, 0,
			     &ac, av, NULL,NULL, 0);
  n = 0;
  XtSetArg(args[n], XmNallowShellResize, True);n++;
  XtSetValues(toplevel, args, n);

  EditMessage(toplevel);

  rc = XmCreateRowColumn(toplevel, "rc", NULL, 0);

  bp = XmCreatePushButton(rc, "Edit Message", NULL, 0);
  XtAddCallback(bp, XmNactivateCallback, EditMsgCB, NULL);
  XtManageChild(bp);
  XtManageChild(rc);

  XtRealizeWidget(toplevel);

  /* Main Loop */
  XtAppMainLoop(app_context);
}
#endif

void AddMessageList(char *number)
{
  XmString string;

  string = XmStringCreateSimple(number);
  XmListAddItem(MsgList, string, 0);
  XmStringFree(string);
}

static void ApplyFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int	nobbs;
  int	noctet;
  int	cmpmsk;

  for (nobbs = 0 ; nobbs < NBBBS ; nobbs++)
  {
    noctet = nobbs / 8 ;
    cmpmsk = 1 << (nobbs % 8) ;

    switch (fwd_t_val[nobbs])
    {
      case 0 :
        clr_bit_fwd(bullig.fbbs, nobbs+1);
        clr_bit_fwd(bullig.forw, nobbs+1);
        break;
      case 1 :
        clr_bit_fwd(bullig.fbbs, nobbs+1);
        set_bit_fwd(bullig.forw, nobbs+1);
        break;
      case 2 :
        set_bit_fwd(bullig.fbbs, nobbs+1);
        clr_bit_fwd(bullig.forw, nobbs+1);
        break;
    }
  }
  SetMsgInfo(&bullig, bullig.numero);
  clear_fwd(bullig.numero) ;
  ins_fwd(&bullig) ;
}

static void CloseFwdCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("CloseCB\n");
  XtUnmanageChild((Widget)client_data);
  XtDestroyWidget(XtParent((Widget)client_data));
}

static void DisplayToggle(Widget w, int val)
{
  Cardinal n;
  Arg args[20] ;

  n = 0;

  switch (val)
  {
    case 0 :
      XtSetArg(args[n], XmNset, False); n++;
      XtSetArg(args[n], XmNselectColor, color_w); n++;
      XtSetArg(args[n], XmNindicatorOn, on_w); n++;
      break;
    case 1 :
      XtSetArg(args[n], XmNset, True); n++;
      XtSetArg(args[n], XmNselectColor, color_w); n++;
      XtSetArg(args[n], XmNindicatorOn, on_w); n++;
      break;
    case 2 :
      XtSetArg(args[n], XmNset, True); n++;
      XtSetArg(args[n], XmNselectColor, color_d); n++;
      XtSetArg(args[n], XmNindicatorOn, on_d); n++;
      break;
  }
  XtSetValues(w, args, n);
}

static void EditToggleCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int num = (uintptr_t)client_data;

  if (fwd_t_val[num] == -1)
  {
    DisplayToggle(w, 0);

  }
  else
  {
    if (++fwd_t_val[num] == 3)
      fwd_t_val[num] = 0;

    DisplayToggle(w, fwd_t_val[num]);
  }
}

static void UpdateFwdChecks(Widget *toggle)
{
  int i;
  int val;
  int noctet;
  int cmpmsk;

  XtVaGetValues(tog_w, XmNselectColor, &color_w, XmNindicatorOn, &on_w, NULL);
  XtVaGetValues(tog_d, XmNselectColor, &color_d, XmNindicatorOn, &on_d, NULL);

  for (i = 0 ; i < NBBBS ; i++)
  {
    if (fwd_t_val[i] == -1)
    {
      DisplayToggle(toggle[i], 0);
      XtSetSensitive(toggle[i], False);
      continue;
    }
    noctet = i / 8 ;
    cmpmsk = 1 << (i % 8) ;

    val = ((bullig.forw[noctet]) & (cmpmsk)) ? 1 : 0;
    if (val == 0)
      val = ((bullig.fbbs[noctet]) & (cmpmsk)) ? 2 : 0;

    fwd_t_val[i] = val;

    DisplayToggle(toggle[i], val);
  }
}

static void EditFwdStatus(void)
{
  Cardinal n;
  Arg args[20] ;
  Widget w;
  Widget fwd_rc;
  int i;
  XmString string;
  char ifwd[NBBBS][7];

  ch_bbs(1, ifwd);

  printf("Msg #%ld\n", bullig.numero);

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 5);n++;
  XtSetArg(args[n], XmNmarginWidth, 5);n++;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  EditFwdDialog = XmCreateFormDialog(toplevel, "edit_fwd", args, n);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditFwdDialog, "apply", args, n);
  XtAddCallback(w, XmNactivateCallback, ApplyFwdCB, 0);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNleftWidget, w);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreatePushButton(EditFwdDialog, "close", args, n);
  XtAddCallback(w, XmNactivateCallback, CloseFwdCB, (XtPointer)EditFwdDialog);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  w = XmCreateLabel(EditFwdDialog, "fwd_label_exd", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightWidget, w);n++;
  tog_d = XmCreateToggleButton(EditFwdDialog, "fwd_toggle_exd", args, n);
  XtManageChild(tog_d);

  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, tog_d);n++;
  w = XmCreateLabel(EditFwdDialog, "fwd_label_exw", args, n);
  XtManageChild(w);

  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, tog_d);n++;
  XtSetArg(args[n], XmNrightWidget, w);n++;
  tog_w = XmCreateToggleButton(EditFwdDialog, "fwd_toggle_exw", args, n);
  XtManageChild(tog_w);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, tog_w);n++;
  XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);n++;
  XtSetArg(args[n], XmNnumColumns, 4);n++;
  fwd_rc = XmCreateRowColumn(EditFwdDialog, "fwd_rc", args, n);
  XtManageChild(fwd_rc);

  for (i = 0 ; i < NBBBS ; i++)
  {
    n = 0;

    if (*ifwd[i])
    {
      string = XmStringCreateSimple(ifwd[i]);
      XtSetArg(args[n], XmNfillOnSelect, True); n++;
    }
    else
    {
      string = XmStringCreateSimple("      ");
      XtSetArg(args[n], XmNfillOnSelect, False); n++;
      fwd_t_val[i] = -1;
    }

    XtSetArg(args[n], XmNlabelString, string);n++;
    XtSetArg(args[n], XmNvisibleWhenOff, True); n++;
    fwd_toggle[i] = XmCreateToggleButton(fwd_rc, "fwd_toggle", args, n);
    XtAddCallback(fwd_toggle[i], XmNvalueChangedCallback, EditToggleCB, (XtPointer)(intptr_t)i);

    XmStringFree(string);
  }
  XtManageChildren(fwd_toggle, NBBBS);

  UpdateFwdChecks(fwd_toggle);

  XtManageChild(EditFwdDialog);
}
