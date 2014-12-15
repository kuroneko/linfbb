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
 * Fichier des variables locales
 */

#define ENGLISH

#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/ScrollBar.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/MessageB.h>
#include <Xm/FileSB.h>
#include <Xm/ComboBox.h>

#define IDOK                1
#define IDCANCEL            0
#define IDYES               1
#define IDNO                2

#define MB_OK               0x0000
#define MB_OKCANCEL         0x0001
#define MB_YESNOCANCEL      0x0003
#define MB_YESNO            0x0004

#define MB_ICONHAND         0x0010
#define MB_ICONQUESTION     0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040
#define MB_ICONINFORMATION  MB_ICONASTERISK

#define ED_MESSAGE 1
#define ED_EDITMSG 2

#define SEPARATOR 0
#define BUTTON 1
#define TOGGLE 2
#define TOOLBUTTON 3

#ifndef PUBLIC
#define PUBLIC extern
#endif

PUBLIC XtAppContext app_context;

PUBLIC XmRendition r_rend[10];
PUBLIC int r_index;

PUBLIC FILE *p_fptr;

PUBLIC Pixel df_pixel;
PUBLIC Pixel rf_pixel;
PUBLIC Pixel vf_pixel;
PUBLIC Pixel bf_pixel;
PUBLIC Pixel no_pixel;
PUBLIC Pixel rc_pixel;
PUBLIC Pixel vc_pixel;
PUBLIC Pixel bc_pixel;

PUBLIC Widget toplevel;
PUBLIC Widget form;
PUBLIC Widget pb;
PUBLIC Widget MenuBar;
PUBLIC Widget MenuFile;
PUBLIC Widget MenuUser;
PUBLIC Widget MenuEdit;
PUBLIC Widget MenuWindow;
PUBLIC Widget MenuOptions;
PUBLIC Widget MenuConfig;
PUBLIC Widget MenuPactor;
PUBLIC Widget MenuHelp;
PUBLIC Widget ToolBar;
PUBLIC Widget Jauge;

PUBLIC Widget ListForm;
PUBLIC Widget ConnectLabel;
PUBLIC Widget StatForm;
PUBLIC Widget StatusLabel;
PUBLIC Widget ConnectList;
PUBLIC Widget ConnectString;
PUBLIC Widget StatList;

PUBLIC Widget Footer;
PUBLIC int foothelp;

PUBLIC int editoron;

#define NB_ICON 16
PUBLIC Widget BIcon[NB_ICON];

#define NB_INIT_B 11
PUBLIC Widget Tb[NB_INIT_B];

PUBLIC Widget ScanSys;
PUBLIC Widget ScanMsg;

/* Liste des widgets de status */
PUBLIC Widget TxtUsed;
PUBLIC Widget TxtGMem;
PUBLIC Widget TxtDisk1;
PUBLIC Widget TxtDisk2;
PUBLIC Widget TxtMsgs;
PUBLIC Widget TxtResync;
PUBLIC Widget TxtState;
PUBLIC Widget TxtHold;
PUBLIC Widget TxtPriv;

PUBLIC Widget Used;
PUBLIC Widget GMem;
PUBLIC Widget Disk1;
PUBLIC Widget Disk2;
PUBLIC Widget Msgs;
PUBLIC Widget Resync;
PUBLIC Widget State;
PUBLIC Widget Hold;
PUBLIC Widget Priv;

PUBLIC Widget Popup;

PUBLIC Widget Opt[10];

#define NB_PITEM 5
PUBLIC Widget PItem[NB_PITEM];

PUBLIC int CurrentSelection;

int can_talk (int);
int editor_on (void);
int GetChList(void);
int MessageBox(int, char *, char *, int);
int record_message (char *ptr, int len);
int set_callsign (char *buf);
int SetChList(int clean);
int talk_to (int channel);

void CreateEditor(char *, char *, char *, int, int);
void cursor_wait(void);
void disconnect_channel(int, int);
void end_edit (int);
void end_wait(void);
void FbbRequestUserList (void);
void FbbSync (void);
void EditMsgCB(Widget, XtPointer, XtPointer);
void EditUsrCB(Widget, XtPointer, XtPointer);
void get_callsign (char *buf);
void ListCnxCB(Widget, XtPointer, XtPointer);
void PendingCB(Widget, XtPointer, XtPointer);
void set_option (int, int);
void TextFieldGetString(Widget w, char *str, int len, int upcase);
void HideFbbWindow(int numero, Widget parent);
void AboutDialog(Widget, XtPointer, XtPointer);
void CopyDialog(Widget, XtPointer, XtPointer);
void InfoDialog(Widget, XtPointer, XtPointer);
void CallsignDialog(Widget, XtPointer, XtPointer);

void EditMessage(Widget);
void EditUser(Widget);
void ListCnx(Widget);
void OptionsCB(Widget w, XtPointer client_data, XtPointer call_data);
void PendingForward(Widget);

void LabelSetString(Widget, char *, char *);
void ScanMsgCB (Widget w, XtPointer client_data, XtPointer call_data);
void ShowFbbWindow(int, Widget);
void sysop_end(void);
void TalkToCB (Widget w, XtPointer client_data, XtPointer call_data);
void ToggleFbbWindow(int numero, Widget parent);

void DisplayInfoDialog(int);

void CreateEditor(char *file, char *preload, char *title, 
		  int options, int autow);

Widget add_item(int type, Widget menu_pane, char *nom, 
		XtPointer callback, XtPointer data, 
		char *help, XtPointer HelpCB);
