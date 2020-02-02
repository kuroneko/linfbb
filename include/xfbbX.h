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


/*
 * Fichier des variables locales
 */

#define ENGLISH

#ifdef __MAIN__
#define PUBLIC
#else
#define PUBLIC extern
#endif

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

#define MB_OK               0x0000
#define MB_OKCANCEL         0x0001
#define MB_YESNOCANCEL      0x0003
#define MB_YESNO            0x0004

#define MB_ICONHAND         0x0010
#define MB_ICONQUESTION     0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040
#define MB_ICONINFORMATION  MB_ICONASTERISK

#define FBB_MSGS        1
#define FBB_STATUS      2
#define FBB_NBCNX       4
#define FBB_LISTCNX     8
#define FBB_MONITOR     16
#define FBB_CONSOLE     32
#define FBB_CHANNEL     64
#define FBB_XFBBX      128

#define CONSOLE 0
#define MONITOR 1
#define ALLCHANN 2

#define MAX_CONF 4
#define MAXFEN 3

typedef struct
{
	char name[80];
	char mycall[80];
	char host[80];
	char pass[256];
	int port;
	int mask;
}
conf_t;

PUBLIC Display *display;

PUBLIC conf_t conf[MAX_CONF];
PUBLIC unsigned curconf;

PUBLIC Widget Rmt[MAX_CONF];
PUBLIC Widget toplevel;

PUBLIC int fenetre[MAXFEN];
PUBLIC int CurrentSelection;

#ifdef __cplusplus
extern "C"
{
#endif

	/* xfbbX.c */
	extern int close_connection (void);
	extern int console_input (char *, int);
	extern int init_orb (char *);
	extern int MessageBox (int, char *, char *, int);
	extern void Caption (int);
	extern void LabelSetString (Widget, char *, char *);

	/* xfbbXcnsl.c */
	extern void AllChanCB (Widget, XtPointer, XtPointer);
	extern void ConsoleCB (Widget, XtPointer, XtPointer);
	extern void HideFbbWindow (int numero, Widget parent);
	extern void MonitorCB (Widget, XtPointer, XtPointer);
	extern void set_win_colors (void);
	extern void ShowFbbWindow (int, Widget);
	extern void sysop_end (void);
	extern void window_init (void);
	extern void window_write (int, char *, int, int, int);

	/* xfbbXabtd.c */
	extern int GetConfig (void);
	extern int PutConfig (void);
	extern void AboutDialog (Widget, XtPointer, XtPointer);
	extern void CallsignDialog (Widget, XtPointer, XtPointer);
	extern void CopyDialog (Widget, XtPointer, XtPointer);
	extern void SetupDialog (Widget, XtPointer, XtPointer);

#ifdef __cplusplus
}
#endif
