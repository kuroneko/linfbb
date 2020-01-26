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

#ifndef __MAIN__
#include <xfbb.h>
#include <serv.h>
#endif

#include <stdio.h>
#include <string.h>
#include <values.h>

#include <X11/keysym.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/FileSB.h>
#include <Xm/BulletinB.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/Protocols.h>

/* #include "textedit.h" */

/***********************************************************************
**                                                                    **
**                  G L O B A L   V A R I A B L E S                   **
**                                                                    **
***********************************************************************/

static	Widget	save_alert;		/* Warning Dialog	      */
static	Atom	a_del_win;		/* Atom for WM_DELETE_WINDOW  */

static	Widget	    find_db,		/* The dialog box             */
		    find_lbl,		/* Label: "Find:"             */
		    find_txt,		/* Find-string Entry Field    */
		    frep_lbl,		/* Label: "Repl:"             */
		    frep_txt,		/* Replace-string Entry Field */
		    find_sep,		/* Sep between pres & conf    */
		    find_btn_1,		/* Action pushbutton          */
		    find_btn_2,		/* Go-away pushbutton         */
		    find_btn_3,		/* Replace pushbutton         */
		    cantfind_db;        /* "Can't Find" Alert         */
static	XmString    find_str,		/* XmString for "Find"        */
		    can_str,		/* XmString for "Cancel"      */
		    next_str,		/* XmString for "Next"        */
		    repl_str,		/* XmString for "Replace"     */
		    done_str;		/* XmString for "Done"	      */
static	int	    finding;		/* Flag for FindCB()	      */
static  int	    fd_start,		/* Start of the found text    */
		    fd_end;		/* End of the found text      */
static int 	    curpos;

static int line = 0; /* Ligne dans le texte */
static int col  = 0; /* Colonne dans le texte */
static int pos  = 0; /* Position de l'insertion ds le buffer */

static int wp;

static int wrap; /* Mode wrap valide */
static int justif; /* Mode justification demande */

static int efoothelp = 0;
static Widget mainwin;		/* XmMainWindow		      */
static Widget menubar;		/* MainWindow Menu Bar	      */
static Widget toolbar;		/* MainWindow Menu Bar	      */
static Widget workwin;		/* MainWindow Work Area       */
static Widget editshell;        /* Shell of the editor        */
static Widget textwin;		/* Work Window XmText widget  */
static Widget footer;		/* MainWindow Footer	      */

static Widget MSave;		/* Widget Save */
static Widget MSaveAs;		/* Widget SaveAs */
static Widget MCut;		/* Widget Cut*/
static Widget MCopy;		/* Widget Copy */
static Widget MJustif;		/* Widget Justification */
static Widget MWrap;		/* Widget AutoWrap */

#define E_NB_ICON 10
static Widget EIcon[E_NB_ICON]; /* Icons of the toolbar	      */
static int OldIconState[E_NB_ICON];

static Arg    arglist[16];	/* For programmatic rsrc stuf */
static Boolean	changed;	/* Used for save alert	      */
static int EditorOptions;

static char *curfile = NULL;   	/* The current filename       */
static Widget	stdfile_db;    	/* The Standard File dialog   */
static Widget	find_alert;

static void (*fileproc)(void);	/* The Read/Write function    */


/***********************************************************************
**                                                                    **
**               F O R W A R D   D E F I N I T I O N S                **
**                                                                    **
***********************************************************************/

static	void 	EndEditor(int);
static	void    InitMenuBar(void);			/* menu.c	      */
static	void	InitOther(void);			/* misc.c	      */
static	char	*StrFind(char *, char *);
static	void	InitAlerts(void);			/* alerts.c	      */
static	void	InitFiler(void);			/* filer.c            */
static	void	FileNew(void);
static	void	FileOpen(void);
static	void	FileInsert(void);
static	void	FileSave(void);
static	void	FileSaveAs(void);
static	void	InitFindDB(void);			/* find.c             */
static	void	ManageFindDB(void);
static	void	InitSaveProto(void);		/* saveproto.c	      */
static	void	InitMainWindow(char *);
static	void	InitWorkWindow(void);
static	void	InitFileMenu(void);
static	void	InitEditMenu(void);
static	void	InitViewMenu(void);
static	void	InitOptionMenu(void);
static	void	InitHelpMenu(void);

static  void	FileMenuCB(Widget, char *, caddr_t);
static  void	EditMenuCB(Widget, char *, XmAnyCallbackStruct *);
static  void    ViewMenuCB(Widget, char *, XmAnyCallbackStruct *);

static	void	ManageStdFile(char *, XtCallbackProc, char *);
static	void	UnmanageStdFile(void);

static	void	SFProc(Widget, caddr_t, XmFileSelectionBoxCallbackStruct *);
static	void	ReadProc(void);
static	void	InsertProc(void);
static	void	WriteProc(void);
static	void	PreReadProc(void);

static	void	FindCB(Widget, char *, XmAnyCallbackStruct *);
static	void	ReplaceCB(Widget, char *, XmAnyCallbackStruct *);
static	void	FindCanCB(Widget, char *, XmAnyCallbackStruct *);

static	void	SaveProtoCB(Widget, char *, XmAnyCallbackStruct *);
static	void	InitSaveAlert(void);
static	void	AlertSaveCB(Widget, char *, caddr_t);
static	Boolean	AlertSaveWP(caddr_t);
static	void	InitFindAlert(void);

/* 
void    CreateEditor(char *file, char *preload, char *title, int options, int autow);
*/


#ifdef __MAIN__

static Widget toplevel;		/* Application Shell          */

void PbCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  /*  XtManageChild(mainwin); */
  CreateEditor(NULL, "filer.c", "Editing : filer.c", 0, 0);
}

/***********************************************************************
**                                                                    **
**  main( argc, argv )                                                **
**                                                                    **
**  Program entry point. Creates shell, calls initialization funcs,   **
**  and turns control over to event loop.                             **
**                                                                    **
***********************************************************************/

void main( argc, argv )
    int     argc;
    char    *argv[];
{
  Widget Pb;

  toplevel = XtInitialize( argv[0], "TextEdit", NULL, 0, &argc, argv );
  Pb = XmCreatePushButton(toplevel, "Bouton", NULL, 0 );
  XtManageChild(Pb);
  XtAddCallback(Pb, XmNactivateCallback, PbCB, NULL);
  
  XtRealizeWidget( toplevel );
  XtMainLoop();
}

#endif /* __MAIN__ */

Boolean EditWorkProc(XtPointer data)
{
  int i;
  int IconState[E_NB_ICON];

  static int first = 1;
  static Pixel TopShadow;
  static Pixel BottomShadow;

  if (first)
    {
      XtVaGetValues(EIcon[0], 
		    XmNtopShadowColor, &TopShadow,
		    XmNbottomShadowColor, &BottomShadow,
		    NULL);
      XtSetSensitive( EIcon[6], TRUE );
      XtSetSensitive( EIcon[7], TRUE );
    }

  *((int *)data) = 0;

  for (i = 0 ; i < E_NB_ICON ; i++)
    IconState[i] = FALSE;

  IconState[0] = TRUE;
  IconState[1] = changed;
  IconState[2] = (XmTextGetSelection(textwin) != NULL);
  IconState[3] = (XmTextGetSelection(textwin) != NULL);
  IconState[4] = TRUE;
  IconState[5] = TRUE;
  IconState[6] = justif;
  IconState[7] = wrap;
  IconState[8] = ((EditorOptions & ED_MESSAGE) != 0);

  /* Help */
  IconState[9]= TRUE;

  for (i = 0 ; i < E_NB_ICON ; i++)
    {
      if (OldIconState[i] != IconState[i])
	{
	  /* Menues associated to icons */
	  Widget MenuW;
	  Widget MenuX;

	  MenuW = NULL;
	  MenuX = NULL;

	  switch(i)
	    {
		case 1:
		MenuW = MSave;
		MenuX = MSaveAs;
		break;
		case 2:
		MenuW = MCut;
		break;
		case 3:
		MenuW = MCopy;
		break;
		case 6:
		MenuW = MJustif;
		break;
		case 7:
		MenuW = MWrap;
		break;
	    }

	  OldIconState[i] = IconState[i];
	  if ((i == 6) || (i == 7))
	    {
	      if (IconState[i])
		{
		  XtVaSetValues(EIcon[i],
				XmNtopShadowColor, BottomShadow,
				XmNbottomShadowColor, TopShadow,
				NULL);
		}
	      else
		{
		  XtVaSetValues(EIcon[i],
				XmNtopShadowColor, TopShadow,
				XmNbottomShadowColor, BottomShadow,
				NULL);
		}
	      if (MenuW)
		  {
		/* XmToggleButtonSetValue(MenuW, IconState[i], FALSE); */
		  XtVaSetValues(MenuW,
				XmNset, IconState[i],
				NULL);
		}
	    }
	  else
	    {
	      XtSetSensitive(EIcon[i], IconState[i]);
	      if (MenuW)
		XtSetSensitive(MenuW, IconState[i]);
	      if (MenuX)
		XtSetSensitive(MenuX, IconState[i]);
	    }
	}
    }

  if (!efoothelp)
    {
      XmString string;
      char buf[80];
      static int old_lig = 0;
      static int old_col = 0;

      if ((old_lig != line) || (old_col != col))
	{
	  old_lig = line;
	  old_col = col;
	  sprintf(buf, "Line %-4d   column %-4d", line+1, col+1);
	  string = XmStringCreateSimple(buf);
	  XtVaSetValues(footer, XmNlabelString, string, NULL);
	  XmStringFree(string);
	}
    }
  return(True);
}

static void Update(void)
{
  if (wp == 0)
    {
      wp = 1;
      XtAppAddWorkProc(app_context, EditWorkProc, &wp);
    }
}

void CreateEditor(char *file, char *preload, char *title, 
		  int options, int autow)
{
  int i;

  EditorOptions = options;

  for (i = 0 ; i < E_NB_ICON ; i++)
    OldIconState[i] = -1;

  InitMainWindow(title);
  InitMenuBar();
  InitWorkWindow();
  
  XmMainWindowSetAreas( mainwin, menubar, NULL, NULL, NULL, workwin );
  
  InitOther();

  XtManageChild(mainwin);

  XtManageChild(editshell);

  changed = FALSE;

  if (curfile)
    free(curfile);
  curfile = NULL;

  if (preload)
    {
      curfile = strdup(preload);
      PreReadProc();
      XmTextSetInsertionPosition(textwin, (XmTextPosition) MAXLONG);
    }

  if (file)
    {
      curfile = strdup(file);
      ReadProc();
    }

  /* Initialisation sequences */
  Update();
  EditWorkProc(&wp);
}
  
void JustifyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  justif = !justif;
  Update();
}

void WrapCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  wrap = !wrap;
  Update();
}

void DeliverCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("Deliver\n");
  EndEditor(TRUE);
}

XtEventHandler EHelpCB(Widget w, XtPointer client_data, XEvent *event)
{
  static XmString prev = NULL;

  if (client_data)
    {
      XmString string;

      if (prev)
	XmStringFree(prev);
      XtVaGetValues(footer, XmNlabelString, &prev, NULL);

      string = XmStringCreateSimple((char *)client_data);
      XtVaSetValues(footer, XmNlabelString, string, NULL);
      XmStringFree(string);
      efoothelp = TRUE;
    }
  else
    {
      if (prev)
	XtVaSetValues(footer, XmNlabelString, prev, NULL);
      efoothelp = FALSE;
    }
  return 0;
}

/***********************************************************************
**                                                                    **
**  InitMainWindow()                                                  **
**                                                                    **
**  This function creates the main window widget and its scrollbars.  **
**  The main window is created as a child of the application shell.   **
**  The scrollbars are either created along with the main-window (if  **
**  its "scrollingPolicy" resource contains TRUE) or separately.      **
**                                                                    **
**  This function modifies the global "mainwin", and accesses the     **
**  global "toplevel".                                                **
**                                                                    **
***********************************************************************/

static void InitMainWindow(char *title)
{
  int n = 0;
  XtSetArg(arglist[n], XmNallowShellResize, True);n++;
  XtSetArg(arglist[n], XmNdeleteResponse, XmDO_NOTHING); ++n;
  XtSetArg(arglist[n], XmNtitle, title); ++n;
  editshell = XmCreateDialogShell(toplevel, "cnsl", arglist, n);

  mainwin = XmCreateMainWindow( editshell, "MainWin", NULL, 0 );
}

static void ChangedCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  changed = TRUE;
  Update();
}

#define MAXLINE 78

static int Wrap(Widget w)
{
  int i;
  int debut;
  int graph;
  int space;
  char *pbuf = NULL;

  if (col < MAXLINE)
    return(0);

  if (!justif && !wrap)
    return(0);

  pbuf = XmTextGetString(w);

  printf("pos = %d texte = %02x\n", pos, pbuf[pos]);
  debut = pos - col;
  printf("debut de ligne = %02x %02x\n", pbuf[debut], pbuf[debut+1]);

  /* Recherche le dernier espace de la ligne, sinon tronque */
  graph = 0;
  space = -1;
  for (i = debut ; i < pos ; i++)
    {
      /* Ne tient pas compte des espaces de debut de ligne */
      if (isgraph(pbuf[i]))
	  graph = 1;
      if ((graph) && (isspace(pbuf[i])))
	space = i;
    }

  printf("Espace en %d\n", space);

  if (space != -1)
    {
      /* remplacer l'espace par un return */
      XmTextReplace(w, space, space+1, "\n");
    }
  else
    {
      /* inserer un return */
      XmTextReplace(w, debut+MAXLINE, debut+MAXLINE, "\n");
      debut = pos;
    }

  /* Justification eventuelle */
  if (justif)
    {
      char buffer[1024];

      if (pos != debut)
	{
	  n_cpy(space-debut, buffer, &pbuf[debut]);
	  justifie(buffer);
	  XmTextReplace(w, debut, space ,buffer);
	}
    }
  XtFree(pbuf);

  return(1);
}

static void DisplayLC(Widget w)
{
  Position x, y;
  int xvalue, yvalue;
  Widget scw, hsb, vsb;
  Dimension fh, fw;
  int xbase, ybase;
  int xinc, yinc;

  {
    XmString string;
    XmFontList font;

    xbase = 6;
    XtVaGetValues(w, XmNfontList, &font, NULL);
    ybase = XmTextGetBaseline(w);
    string = XmStringCreateSimple("Wy");
    XmStringExtent(font, string, &fw, &fh);
    XmStringFree(string);
    fw /= 2;
    /*    printf("Font : w = %d h = %d\n", fw, fh); */
  }

  pos = XmTextGetInsertionPosition(w);
  XmTextPosToXY(w, pos, &x, &y);
  /* printf("x=%d, y=%d\n", x, y); */

  scw = XtParent(w);
  XtVaGetValues(scw, XmNverticalScrollBar, &vsb, XmNhorizontalScrollBar, &hsb, NULL);
  if (hsb)
    XtVaGetValues(hsb, XmNvalue, &xvalue, XmNincrement, &xinc, NULL);
  else
    {
      xvalue = 0;
      xinc = 1;
    }
  if (vsb)
    XtVaGetValues(vsb, XmNvalue, &yvalue, XmNincrement, &yinc, NULL);
  else
    {
      yvalue = 0;
      yinc = 1;
    }

  /* printf("xvalue = %d %d yvalue = %d %d\n", xvalue, xinc, yvalue, yinc);*/

  line = (yvalue/1) + ((y - ybase) / fh);
  col = (xvalue/fw) + ((x - xbase) / fw);
  /*  printf("lig = %d col = %d\n", line, col); */
  Update();
}

static void KbHit(Widget w, caddr_t client_data, XEvent *event)
{
  char buf[10];
  KeySym keysym;

  XLookupString(&event->xkey, buf, 1, &keysym, NULL);
  /* printf("buf[0]=%02x keysym=%x b=%d\n", buf[0], keysym, IsKeypadKey(keysym));*/

  if ((keysym < 0xff) && (Wrap(w)))
    DisplayLC(w);
}

static void PosMoved(Widget w, caddr_t client_data, XEvent *event)
{
  DisplayLC(w);
}

/***********************************************************************
**                                                                    **
**  InitWorkWindow()                                                  **
**                                                                    **
**  This function creates the work window and its children. The       **
**  work window is created as the child of the main window.           **
**                                                                    **
**  This function modifies the globals "workwin" and "textwin", and   **
**  accesses the global "mainwin".                                    **
**                                                                    **
***********************************************************************/

static void InitWorkWindow()
{
  int n = 0;
  XmString string;

  workwin = XmCreateForm( mainwin, "WorkWin", arglist, n );

  n = 0;
  XtSetArg(arglist[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(arglist[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(arglist[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(arglist[n], XmNorientation, XmHORIZONTAL);n++;
  toolbar = XmCreateRowColumn( workwin, "tool_bar", arglist, n );
  XtManageChild( toolbar );

  EIcon[0] = add_item(TOOLBUTTON, toolbar, "Open",  FileMenuCB, "Opn", 
		    "Opens a file", EHelpCB);
  EIcon[1] = add_item(TOOLBUTTON, toolbar, "Save",  FileMenuCB, "Sav", 
		    "Saves the edited file", EHelpCB);
  EIcon[2] = add_item(TOOLBUTTON, toolbar, "Cut",  EditMenuCB, "Cut", 
		    "Cuts the selection", EHelpCB);
  EIcon[3] = add_item(TOOLBUTTON, toolbar, "Copy",  EditMenuCB, "Cpy", 
		    "Copies the selection to the clipboard", EHelpCB);
  EIcon[4] = add_item(TOOLBUTTON, toolbar, "Paste",  EditMenuCB, "Pst", 
		    "Pastes the clipboard to the cursor position", EHelpCB);
  EIcon[5] = add_item(TOOLBUTTON, toolbar, "Replace",  EditMenuCB, "Fnd",
		    "Finds and replace the selected text", EHelpCB);
  EIcon[6] = add_item(TOOLBUTTON, toolbar, "Justify",  JustifyCB, NULL, 
		    "Justifies the text", EHelpCB);
  EIcon[7] = add_item(TOOLBUTTON, toolbar, "Wrap",  WrapCB, NULL, 
		    "Auto wraps at 78 characters", EHelpCB);
  EIcon[8] = add_item(TOOLBUTTON, toolbar, "Deliver",  DeliverCB, NULL, 
		    "Delivers the message", EHelpCB);
  EIcon[9] = add_item(TOOLBUTTON, toolbar, "B16", NULL, NULL, 
		    "On-line help", EHelpCB);
  XtVaSetValues(toolbar, XmNmenuHelpWidget, EIcon[9], NULL);
  
  if (EditorOptions & ED_MESSAGE)
    {
      XtUnmanageChild(EIcon[0]);
    }
  else
    {
      XtUnmanageChild(EIcon[8]);
    }

  string = XmStringCreateSimple("  ");
  n = 0;
  XtSetArg(arglist[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(arglist[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(arglist[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(arglist[n], XmNlabelString, string);n++;
  footer = XmCreateLabel(workwin, "footer", arglist, n);
  XtManageChild(footer);
  XmStringFree(string);

  n = 0;
  XtSetArg(arglist[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(arglist[n], XmNtopWidget, toolbar); n++;
  XtSetArg(arglist[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(arglist[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(arglist[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(arglist[n], XmNbottomWidget, footer);n++;
  textwin = XmCreateScrolledText(workwin, "TextWin", arglist, n);
  XtManageChild( textwin );
  XtAddCallback(textwin, XmNvalueChangedCallback, ChangedCB, NULL);
  XtAddEventHandler(textwin, KeyPressMask, FALSE, (XtPointer)KbHit, NULL);
  XtAddEventHandler(textwin, ButtonReleaseMask|KeyReleaseMask, FALSE, (XtPointer)PosMoved, NULL);

  XtManageChild( workwin );
}


/***********************************************************************
**                                                                    **
**  InitMenuBar()                                                     **
**                                                                    **
**  This function creates the menu bar and all pulldown menus. The    **
**  menu bar is created as the child of the main window.              **
**                                                                    **
**  This function modifies the global "menubar", and accesses the     **
**  global "mainwin".                                                 **
**                                                                    **
***********************************************************************/

static void InitMenuBar(void)
{
  menubar = XmCreateMenuBar( mainwin, "menu_bar", NULL, 0 );
  XtManageChild( menubar );

  InitFileMenu();
  InitEditMenu();
  InitViewMenu();
  InitOptionMenu();
  InitHelpMenu();
  
}

/***********************************************************************
**                                                                    **
**  InitFileMenu()                                                    **
**                                                                    **
**  Creates the File menu: cascade button, pull-down menu pane, and   **
**  all menu-pane choices. Attaches callbacks to menu-pane choices.   **
**                                                                    **
***********************************************************************/

static void InitFileMenu(void)
{
    Widget	topic,
		pane,
		choices[8];

    pane = XmCreatePulldownMenu( menubar, "FilePane", NULL, 0 );

    choices[0] = XmCreatePushButton( pane, "File_New", NULL, 0 );
    choices[1] = XmCreatePushButton( pane, "File_Open", NULL, 0 );
    choices[2] = XmCreatePushButton( pane, "File_Insert", NULL, 0 );
    choices[3] = XmCreatePushButton( pane, "File_Save", NULL, 0 );
    choices[4] = XmCreatePushButton( pane, "File_SaveAs", NULL, 0 );
    choices[5] = XmCreateSeparator(  pane, "File_Sep1", NULL, 0 );
    choices[6] = XmCreatePushButton( pane, "File_Deliver", NULL, 0 );
    choices[7] = XmCreatePushButton( pane, "File_Exit", NULL, 0 );
    XtManageChildren( choices, 8 );

    XtSetArg( arglist[0], XmNsubMenuId, pane );
    topic = XmCreateCascadeButton( menubar, "FileTopic", arglist, 1 );
    XtManageChild( topic );

    XtAddCallback( choices[0], XmNactivateCallback, (XtPointer)FileMenuCB, "New" );
    XtAddCallback( choices[1], XmNactivateCallback, (XtPointer)FileMenuCB, "Opn" );
    XtAddCallback( choices[2], XmNactivateCallback, (XtPointer)FileMenuCB, "Ins" );
    XtAddCallback( choices[3], XmNactivateCallback, (XtPointer)FileMenuCB, "Sav" );
    XtAddCallback( choices[4], XmNactivateCallback, (XtPointer)FileMenuCB, "SAs" );
    XtAddCallback( choices[6], XmNactivateCallback, (XtPointer)FileMenuCB, "Del" );
    XtAddCallback( choices[7], XmNactivateCallback, (XtPointer)FileMenuCB, "Ext" );

    if (EditorOptions & ED_MESSAGE)
      {
	XtSetSensitive( choices[0], FALSE );
	XtSetSensitive( choices[1], FALSE );
	XtSetSensitive( choices[3], FALSE );
	wrap = justif = TRUE;
      }
    else
      {
	XtSetSensitive( choices[6], FALSE );
	wrap = justif = FALSE;
      }
    MSave   = choices[3];
    MSaveAs = choices[4];
    printf("wrap=%d justif=%d\n", wrap, justif);
}


/***********************************************************************
**                                                                    **
**  InitEditMenu()                                                    **
**                                                                    **
**  Creates the Edit menu: cascade button, pull-down menu pane, and   **
**  all menu-pane choices. Attaches callbacks to menu-pane choices.   **
**                                                                    **
***********************************************************************/

static void InitEditMenu(void)
{
    Widget	topic,
		pane,
		choices[7];

    pane = XmCreatePulldownMenu( menubar, "Edit_Pane", NULL, 0 );

    choices[0] = XmCreatePushButton( pane, "Edit_Cut", NULL, 0 );
    choices[1] = XmCreatePushButton( pane, "Edit_Copy", NULL, 0 );
    choices[2] = XmCreatePushButton( pane, "Edit_Paste", NULL, 0 );
    choices[3] = XmCreateSeparator(  pane, "Edit_Sep1", NULL, 0 );
    choices[4] = XmCreatePushButton( pane, "Edit_Find", NULL, 0 );
    XtManageChildren( choices, 5 );

    XtSetArg( arglist[0], XmNsubMenuId, pane );
    topic = XmCreateCascadeButton( menubar, "EditTopic", arglist, 1 );
    XtManageChild( topic );

    XtAddCallback( choices[0], XmNactivateCallback, (XtPointer)EditMenuCB, "Cut" );
    XtAddCallback( choices[1], XmNactivateCallback, (XtPointer)EditMenuCB, "Cpy" );
    XtAddCallback( choices[2], XmNactivateCallback, (XtPointer)EditMenuCB, "Pst" );
    XtAddCallback( choices[4], XmNactivateCallback, (XtPointer)EditMenuCB, "Fnd" );

    XtSetSensitive( choices[2], FALSE );

    MCopy = choices[1];
    MCut  = choices[0];
 }


/***********************************************************************
**                                                                    **
**  InitViewMenu()                                                    **
**                                                                    **
**  Creates the View menu: cascade button, pull-down menu pane, and   **
**  all menu-pane choices. Attaches callbacks to menu-pane choices.   **
**                                                                    **
***********************************************************************/

static void InitViewMenu(void)
{
    Widget pane, choices[4];

    pane = XmCreatePulldownMenu( menubar, "View_Pane", NULL, 0 );

    choices[0] = XmCreatePushButton( pane, "View_Top", NULL, 0 );
    choices[1] = XmCreatePushButton( pane, "View_Bot", NULL, 0 );
    /* choices[2] = XmCreateSeparator(  pane, "View_Sep1", NULL, 0 );
       choices[3] = XmCreatePushButton( pane, "View_Page", NULL, 0 ); */
    XtManageChildren( choices, 2 );

    XtAddCallback( choices[0], XmNactivateCallback, (XtPointer)ViewMenuCB, "Top" );
    XtAddCallback( choices[1], XmNactivateCallback, (XtPointer)ViewMenuCB, "Bot" );
    /*
      XtSetArg( arglist[0], XmNsubMenuId, pane );
      topic = XmCreateCascadeButton( menubar, "ViewTopic", arglist, 1 );
      XtManageChild( topic );
    */
}


/***********************************************************************
**                                                                    **
**  InitOptionMenu()                                                  **
**                                                                    **
**  Creates the Option menu: cascade button, pull-down menu pane, and **
**  all menu-pane choices. Attaches callbacks to menu-pane choices.   **
**                                                                    **
***********************************************************************/

static void InitOptionMenu()
{
    Arg		args[1];
    Widget	topic,
		pane,
		choices[1];

    pane = XmCreatePulldownMenu( menubar, "Option_Pane", NULL, 0 );

    XtSetArg(args[0], XmNvisibleWhenOff, True);
    choices[0] = XmCreateToggleButton( pane, "Option_Justif", args, 1 );
    choices[1] = XmCreateToggleButton( pane, "Option_Wrap", args, 1 );
    XtManageChildren( choices, 2 );

    XtAddCallback( choices[0], XmNvalueChangedCallback, JustifyCB, "Top" );
    XtAddCallback( choices[1], XmNvalueChangedCallback, WrapCB, "Bot" );

    XtSetArg( arglist[0], XmNsubMenuId, pane );
    topic = XmCreateCascadeButton( menubar, "OptionTopic", arglist, 1 );
    XtManageChild( topic );

    MJustif = choices[0];
    MWrap   = choices[1];
}


/***********************************************************************
**                                                                    **
**  InitHelpMenu()                                                    **
**                                                                    **
**  Creates the Help menu: cascade button, pull-down menu pane, and   **
**  all menu-pane choices. Attaches callbacks to menu-pane choices.   **
**                                                                    **
***********************************************************************/

static void InitHelpMenu()
{
    Widget	topic,
		pane,
		choices[1];

    pane = XmCreatePulldownMenu( menubar, "Help_Pane", NULL, 0 );

    choices[0] = XmCreateLabel( pane, "Help_Lbl", NULL, 0 );
    XtManageChildren( choices, 1 );

    XtSetArg( arglist[0], XmNsubMenuId, pane );
    topic = XmCreateCascadeButton( menubar, "HelpTopic", arglist, 1 );
    XtManageChild( topic );

    XtSetArg( arglist[0], XmNmenuHelpWidget, topic );
    XtSetValues( menubar, arglist, 1 );
}

static void EndEditor(int send)
{
  /* exit( 0 ); */
  XtUnmanageChild(mainwin);
  XtDestroyWidget(editshell);

#ifndef __MAIN__
  
  if (EditorOptions & ED_MESSAGE) 
    {
      if  (send)
	{
	  char *txtbuf = XmTextGetString( textwin );
	  int size     = strlen(txtbuf);
	  int i;
	  
	  for (i = 0 ; i < size ; i++)
	    {
	      if (txtbuf[i] == '\n')
		txtbuf[i] = '\r';
	    }
	  record_message(txtbuf, size);
	  
	  XtFree( txtbuf );
	}
      else
	{
	  record_message(NULL, 0);
	}
    }

  if ((EditorOptions & ED_MESSAGE) || (EditorOptions & ED_EDITMSG))
    end_edit(FALSE);
  else
    end_edit(TRUE);

#endif
}


/***********************************************************************
**                                                                    **
**  FileMenuCB( w, client_data, call_data )                           **
**                                                                    **
**  Callback procedure for the "File" pull-down. This function is     **
**  called when any of the file menu buttons are activated. The       **
**  particular operation is identified by a string accessed by the    **
**  "client_data" param.                                              **
**                                                                    **
**  Note: This callback is only invoked on Activate, so the call      **
**        data (which describes the reason) is superfluous. It is     **
**        therefore not declared as a specific type in the func hdr.  **
**                                                                    **
***********************************************************************/

static void FileMenuCB( w, client_data, call_data )
    Widget	w;
    char	*client_data;
    caddr_t	call_data;
{
    if (!strcmp(client_data, "New"))
	FileNew();
    else if (!strcmp(client_data, "Opn"))
	FileOpen();
    else if (!strcmp(client_data, "Ins"))
	FileInsert();
    else if (!strcmp(client_data, "Sav"))
	FileSave();
    else if (!strcmp(client_data, "SAs"))
	FileSaveAs();
    else if (!strcmp(client_data, "Del"))
      {
	EndEditor(FALSE);
      }
    else if (!strcmp(client_data, "Ext"))
        {
	  if (changed)
	    XtManageChild( save_alert );
	  else
	    EndEditor(FALSE);
        }
}


/***********************************************************************
**                                                                    **
**  ViewMenuCB( w, client_data, call_data )                           **
**                                                                    **
**  Callback procedure for the "Edit" pull-down. This function is     **
**  called when any of the menu buttons are activated. The choice     **
**  is identified by a string pointed-to by the "client_data" parm.   **
**  "client_data" param.                                              **
**                                                                    **
**  Note: This callback is only invoked on Activate, so the call      **
**        data (which describes the reason) is superfluous. It is     **
**        therefore not declared as a specific type in the func hdr.  **
**                                                                    **
***********************************************************************/

static void ViewMenuCB( w, client_data, call_data )
    Widget		w;
    char		*client_data;
    XmAnyCallbackStruct	*call_data;
{
    if (!strcmp(client_data, "Top"))
      {
	XmTextSetInsertionPosition(textwin, (XmTextPosition) 0);
      }
    else if (!strcmp(client_data, "Bot"))
      {
	XmTextSetInsertionPosition(textwin, (XmTextPosition) MAXLONG);
      }
}

/***********************************************************************
**                                                                    **
**  EditMenuCB( w, client_data, call_data )                           **
**                                                                    **
**  Callback procedure for the "Edit" pull-down. This function is     **
**  called when any of the menu buttons are activated. The choice     **
**  is identified by a string pointed-to by the "client_data" parm.   **
**  "client_data" param.                                              **
**                                                                    **
**  Note: This callback is only invoked on Activate, so the call      **
**        data (which describes the reason) is superfluous. It is     **
**        therefore not declared as a specific type in the func hdr.  **
**                                                                    **
***********************************************************************/

static void EditMenuCB( w, client_data, call_data )
    Widget		w;
    char		*client_data;
    XmAnyCallbackStruct	*call_data;
{
    XButtonEvent	*event = (XButtonEvent *)call_data->event;

    if (!strcmp(client_data, "Cut"))
	XmTextCut( textwin, event->time );
    else if (!strcmp(client_data, "Cpy"))
	XmTextCopy( textwin, event->time );
    else if (!strcmp(client_data, "Pst"))
	XmTextPaste( textwin );
    else if (!strcmp(client_data, "Del"))
        {
        }
    else if (!strcmp(client_data, "Fnd"))
        {
	ManageFindDB();
        }
    else if (!strcmp(client_data, "Rpl"))
        {
        }
}

/***********************************************************************
**                                                                    **
**  InitFiler()                                                       **
**                                                                    **
**  This function initializes the filer module: it clears the current **
**  filename, disables the File/Save menu choice, and creates the     **
**  Standard File dialog.                                             **
**                                                                    **
***********************************************************************/

void InitFiler()
{
    stdfile_db = XmCreateFileSelectionDialog( mainwin, "StdFile", NULL, 0 );

    XtAddCallback( stdfile_db, XmNokCallback, (XtPointer)SFProc, NULL );
    XtAddCallback( stdfile_db, XmNcancelCallback, (XtPointer)UnmanageStdFile, NULL );
    XtAddCallback( stdfile_db, XmNhelpCallback, (XtPointer)UnmanageStdFile, NULL );
}


/***********************************************************************
**                                                                    **
**  ManageStdFile( title, proc, defspec )                             **
**                                                                    **
**  This function manages the Standard File dialog. It sets the       **
**  dialog's title to the string passed as "title", installs the      **
**  function passed as "proc" in the "fileproc" variable (it gets     **
**  called by the StdFile callback handler), and stores the string    **
**  passed in "defspec" in the "dirSpec" resource.                    **
**                                                                    **
***********************************************************************/

static	void ManageStdFile( title, proc, defspec )
    char	    *title;
    XtCallbackProc  proc;
    char	    *defspec;
{
    XmString	    temp1, temp2;

    temp1 = XmStringCreate( title,   XmSTRING_DEFAULT_CHARSET );
    if (defspec == NULL)
        temp2 = XmStringCreate( "", XmSTRING_DEFAULT_CHARSET );
    else
        temp2 = XmStringCreate( defspec, XmSTRING_DEFAULT_CHARSET );

    XtSetArg( arglist[0], XmNdialogTitle, temp1 );
    XtSetArg( arglist[1], XmNdirSpec,     temp2 );
    XtSetValues( stdfile_db, arglist, 2 );

    XmStringFree( temp1 );
    XmStringFree( temp2 );

    XmFileSelectionDoSearch( stdfile_db, NULL );

    XtManageChild( stdfile_db );

    fileproc = (XtPointer)proc;
}


/***********************************************************************
**                                                                    **
**  UnmanageStdFile()                                                 **
**                                                                    **
**  This function unmanages the Standard File dialog. It is attached  **
**  to the "Cancel" and "Help" buttons, because XmFileSelectionBox    **
**  does not handle the "autoUnmanage" resource.                      **
**                                                                    **
***********************************************************************/

static	void UnmanageStdFile()
{
    XtUnmanageChild( stdfile_db );
}


/***********************************************************************
**                                                                    **
**  SFProc( w, client_data, call_data )                               **
**                                                                    **
**  This function is the callback procedure for the Standard File     **
**  dialog. It stores the chosen filename in "curfile", and then      **
**  calls the function pointed-to by "fileproc".                      **
**                                                                    **
***********************************************************************/

static	void SFProc(w, client_data, call_data )
    Widget				w;
    caddr_t				client_data;
    XmFileSelectionBoxCallbackStruct	*call_data;
{
    if (curfile != NULL)
        free( curfile );
    XmStringGetLtoR( call_data->value, XmSTRING_DEFAULT_CHARSET, &curfile );

    UnmanageStdFile();

    (*fileproc)();
}


/***********************************************************************
**                                                                    **
**  ReadProc()                                                        **
**                                                                    **
**  This function reads the file specified by "curfile" into the      **
**  editor's text buffer.                                             **
**                                                                    **
***********************************************************************/

static	void ReadProc()
{
    FILE    *infile = fopen( curfile, "rt" );
    char    *txtbuf;
    char    *ptr;
    long    size;
    int     i;

    if (infile == NULL)
        /* Should display error */
        return;

    fseek( infile, 0L, 2 );
    size = ftell( infile );
    rewind( infile );

    txtbuf = XtMalloc( size+1 );

    fread( txtbuf, sizeof(char), size, infile );

    ptr = txtbuf;
    for (i = 0 ; i < size ; i++)
      {
	if (txtbuf[i] == '\r')
	  continue;
	*ptr++ = txtbuf[i];
      }
    *ptr = '\0';

    XmTextSetString( textwin, txtbuf );

    XtFree( txtbuf );

    changed = FALSE;
  Update();
}


/***********************************************************************
**                                                                    **
**  PreReadProc()                                                     **
**                                                                    **
**  This function reads the file specified by "curfile" into the      **
**  editor's text buffer. Each line is preceeded with ">"             **
**                                                                    **
***********************************************************************/

#define SBUF 65000
static	void PreReadProc()
{
  FILE *infile;
  FILE *tmpfile;
  char *txtbuf;
  long size;
  char tmpname[80];

  sprintf(tmpname, "/tmp/xfbbtmp.%d", getpid());

  infile = fopen( curfile, "rt" );
  if (infile == NULL)
    /* Should display error */
    return;
  
  tmpfile = fopen( tmpname, "w+t");
  if (tmpfile == NULL)
    {
      perror("open");
      /* Should display error */
      return;
    }
  
  txtbuf = XtMalloc( SBUF+1 );

  txtbuf[0] = '>';
  while (fgets(txtbuf+1, SBUF, infile))
    {
      fputs(txtbuf, tmpfile);
    }
  
  XtFree( txtbuf );
  fclose(infile);
  
  fseek( tmpfile, 0L, 2 );
  size = ftell( tmpfile );
  rewind( tmpfile );
  
  txtbuf = XtMalloc( size+1 );
  
  fread( txtbuf, sizeof(char), size, tmpfile );
  XmTextSetString( textwin, txtbuf );
  
  XtFree( txtbuf );

  fclose ( tmpfile );
  unlink( tmpname );
  
  changed = FALSE;
  Update();
}


/***********************************************************************
**                                                                    **
**  InsertProc()                                                      **
**                                                                    **
**  This function reads the file specified by "curfile" into the      **
**  editor's text buffer.                                             **
**                                                                    **
***********************************************************************/

static	void InsertProc()
{
    FILE    *infile = fopen( curfile, "rt" );
    char    *txtbuf;
    char    *ptr;
    long    size;
    int     i;
    XmTextPosition position;

    if (infile == NULL)
        /* Should display error */
        return;

    fseek( infile, 0L, 2 );
    size = ftell( infile );
    rewind( infile );

    txtbuf = XtMalloc( size+1 );

    fread( txtbuf, sizeof(char), size, infile );

    ptr = txtbuf;
    for (i = 0 ; i < size ; i++)
      {
	if (txtbuf[i] == '\r')
	  continue;
	*ptr++ = txtbuf[i];
      }
    *ptr = '\0';

    position = XmTextGetInsertionPosition( textwin);
    XmTextInsert( textwin, position, txtbuf );

    XtFree( txtbuf );

    changed = TRUE;
  Update();
}


/***********************************************************************
**                                                                    **
**  WriteProc()                                                       **
**                                                                    **
**  This function writes the editor's text buffer into a file named   **
**  by "curfile".                                                     **
**                                                                    **
***********************************************************************/

static	void WriteProc()
{
    FILE    *outfile;
    char    *txtbuf;
    long    size;

    outfile = fopen( curfile, "wt" );
    if (outfile == NULL)
        /* Should display error */
        return;

    txtbuf = XmTextGetString( textwin );
    size   = strlen(txtbuf);

    fwrite( txtbuf, sizeof(char), size, outfile );
    fclose( outfile );

    XtFree( txtbuf );
    changed = FALSE;
  Update();
}


/***********************************************************************
**                                                                    **
**  FileNew()                                                         **
**                                                                    **
**  This function is called from the File/New menu choice. It clears  **
**  the text buffer, resets the current file, and disables the File/  **
**  Save menu choice.                                                 **
**                                                                    **
***********************************************************************/

void FileNew()
{
    XmTextSetString( textwin, "" );

    if (curfile != NULL)
        free( curfile );
    curfile = NULL;
    changed = FALSE;
  Update();
}


/***********************************************************************
**                                                                    **
**  FileOpen()                                                        **
**                                                                    **
**  This function is called from the File/Open menu choice. All it    **
**  does is invoke the Standard File dialog -- SFProc and ReadProc    **
**  do the real work.                                                 **
**                                                                    **
***********************************************************************/

void FileOpen()
{
    ManageStdFile( "Open...", (XtPointer)ReadProc, NULL );
}


/***********************************************************************
**                                                                    **
**  FileInsert()                                                      **
**                                                                    **
**  This function is called from the File/Open menu choice. All it    **
**  does is invoke the Standard File dialog -- SFProc and ReadProc    **
**  do the real work.                                                 **
**                                                                    **
***********************************************************************/

void FileInsert()
{
    ManageStdFile( "Insert...", (XtPointer)InsertProc, NULL );
}


/***********************************************************************
**                                                                    **
**  FileSave()                                                        **
**                                                                    **
**  This function is a link to WriteProc(), which writes the text     **
**  buffer into a file using the current filename.                    **
**                                                                    **
***********************************************************************/

void FileSave()
{
    if (curfile == NULL)
        FileSaveAs();
    else
        WriteProc();
}


/***********************************************************************
**                                                                    **
**  FileSaveAs()                                                      **
**                                                                    **
**  This function is called from the File/Save-As menu choice. It     **
**  invokes the Standard File dialog, and links it to WriteProc.      **
**                                                                    **
***********************************************************************/

void FileSaveAs()
{
    ManageStdFile( "Save As...", (XtPointer)WriteProc, curfile );
}


/***********************************************************************
**                                                                    **
**  InitFindDB()                                                      **
**                                                                    **
**  Creates the "Find" dialog box, which is controlled by the         **
**  Edit/Find... pull-down menu choice.                               **
**                                                                    **
**  Modifies the global variable "find_db", and local variables       **
**  "find_txt", "find_ok_btn", and "find_nxt_btn".                    **
**                                                                    **
***********************************************************************/

void InitFindDB()
{
    Widget	temp;

    find_db = XmCreateBulletinBoardDialog( mainwin, "FindDB", NULL, 0 );

    find_lbl = XmCreateLabel( find_db, "Find_Lbl", NULL, 0 );
    XtManageChild( find_lbl );
    find_txt = XmCreateText( find_db, "Find_Txt", NULL,  0 );
    XtManageChild( find_txt );
    frep_lbl = XmCreateLabel( find_db, "Rep_Lbl", NULL, 0 );
    XtManageChild( frep_lbl );
    frep_txt = XmCreateText( find_db, "Rep_Txt", NULL,  0 );
    XtManageChild( frep_txt );


    find_sep = XmCreateSeparator( find_db, "Find_Sep", NULL, 0 );
    XtManageChild( find_sep );

    find_btn_1 = XmCreatePushButton( find_db, "Find_Btn1", NULL, 0 );
    XtManageChild( find_btn_1 );
    find_btn_2 = XmCreatePushButton( find_db, "Find_Btn2", NULL, 0 );
    XtManageChild( find_btn_2 );
    find_btn_3 = XmCreatePushButton( find_db, "Find_Btn3", NULL, 0 );
    XtManageChild( find_btn_3 );

    XtAddCallback( find_btn_1, XmNactivateCallback, (XtPointer)FindCB,    NULL );
    XtAddCallback( find_btn_2, XmNactivateCallback, (XtPointer)FindCanCB, NULL );
    XtAddCallback( find_btn_3, XmNactivateCallback, (XtPointer)ReplaceCB, NULL );

    XtSetArg( arglist[0], XmNdefaultButton, find_btn_1 );
    XtSetValues( find_db, arglist, 1 );

    find_str = XmStringCreate( "Find",   XmSTRING_DEFAULT_CHARSET );
    can_str  = XmStringCreate( "Cancel", XmSTRING_DEFAULT_CHARSET );
    next_str = XmStringCreate( "Next",   XmSTRING_DEFAULT_CHARSET );
    repl_str = XmStringCreate( "Replace",XmSTRING_DEFAULT_CHARSET );
    done_str = XmStringCreate( "Done",   XmSTRING_DEFAULT_CHARSET );

    cantfind_db = XmCreateMessageDialog( mainwin, "CantFind", NULL, 0 );
    temp = XmMessageBoxGetChild( cantfind_db, XmDIALOG_CANCEL_BUTTON );
    XtUnmanageChild( temp );
    temp = XmMessageBoxGetChild( cantfind_db, XmDIALOG_HELP_BUTTON );
    XtUnmanageChild( temp );
}



/***********************************************************************
**                                                                    **
**  ManageFindDB()                                                    **
**                                                                    **
**  Called when the dialog box is first presented, this function      **
**  manages the DB and sets the labels of its buttons to "Find"       **
**  and "Cancel". It also sets the "finding" flag FALSE, for the      **
**  first call to FindCB.                                             **
**                                                                    **
***********************************************************************/

void ManageFindDB()
{
    char		*findstr;

    XtManageChild( find_db );
    _XmGrabTheFocus( find_txt );

    findstr = XmTextGetSelection( textwin );
    if (findstr)
    {
        XtSetArg( arglist[0], XmNvalue, findstr );
        XtSetValues( find_txt, arglist, 1 );
    }
    XtSetArg( arglist[0], XmNlabelString, find_str );
    XtSetValues( find_btn_1, arglist, 1 );
    XtSetArg( arglist[0], XmNlabelString, can_str );
    XtSetValues( find_btn_2, arglist, 1 );
    XtSetArg( arglist[0], XmNlabelString, repl_str );
    XtSetValues( find_btn_3, arglist, 1 );
    XtSetSensitive( find_btn_3, FALSE);
    finding = FALSE;

    fd_start = fd_end = -1;
}


/***********************************************************************
**                                                                    **
**  FindCB( w, client_data, call_data )                               **
**                                                                    **
**  Called from the "Find" or "Next" buttons of the Find DB. This     **
**  function searches for the first/next occurrence of the search     **
**  string. The "client_data" param points to a string -- "Fnd" or    **
**  "Nxt" -- which indicates which button this is called from; it     **
**  must initialize its local variables for the first call.           **
**                                                                    **
**  In operation, this function searches for the next occurrence of   **
**  the target string in the text buffer, and selects it.             **
**                                                                    **
***********************************************************************/

static void FindCB( w, client_data, call_data )
    Widget		w;
    char		*client_data;
    XmAnyCallbackStruct	*call_data;
{
    char		*findstr,
			*txtbuf,
			*txtptr;

    if (!finding)
	{
	finding = TRUE;
	curpos  = 0;
	XtSetArg( arglist[0], XmNlabelString, next_str );
	XtSetValues( find_btn_1, arglist, 1 );
	XtSetArg( arglist[0], XmNlabelString, done_str );
	XtSetValues( find_btn_2, arglist, 1 );
	}

    findstr = XmTextGetString( find_txt );
    txtbuf  = XmTextGetString( textwin );
    txtptr  = StrFind( (txtbuf + curpos), findstr );
    if (txtptr == NULL)
        {
	XtUnmanageChild( find_db );
	XtManageChild( cantfind_db );
        }
    else
	{
	fd_start = txtptr - txtbuf;
	fd_end   = fd_start + strlen(findstr);
        XtSetArg( arglist[0], XmNcursorPosition, fd_end );
        XtSetValues( textwin, arglist, 1 );
	XmTextSetSelection( textwin, fd_start, fd_end, 
				     call_data->event->xbutton.time );

	curpos = fd_start + 1;
        XtSetSensitive( find_btn_3, TRUE);
	}
    XtFree( txtbuf );
    XtFree( findstr );
    DisplayLC(textwin);
}


/***********************************************************************
**                                                                    **
**  ReplaceCB( w, client_data, call_data )                            **
**                                                                    **
**  Called from the "Replace" button of the Find DB. This             **
**  replaces the found text.                                          **
**                                                                    **
***********************************************************************/

static void ReplaceCB( w, client_data, call_data )
    Widget		w;
    char		*client_data;
    XmAnyCallbackStruct	*call_data;
{
    char		*replstr;

    if (fd_start == -1)
	return;

    replstr = XmTextGetString( frep_txt );

    XmTextReplace( textwin, fd_start, fd_end, replstr );

    curpos += (strlen(replstr) - (fd_end - fd_start)) ;

    XtFree( replstr );
    DisplayLC(textwin);
}

/***********************************************************************
**                                                                    **
**  FindCanCB( w, client_data, call_data )                            **
**                                                                    **
**  Called from the "Cancel" or "OK" buttons of the Find DB. This     **
**  simply unmanages the DB.                                          **
**                                                                    **
***********************************************************************/

static void FindCanCB( w, client_data, call_data )
    Widget		w;
    char		*client_data;
    XmAnyCallbackStruct	*call_data;
{
    XtUnmanageChild( find_db );
}

/***********************************************************************
**                                                                    **
**  InitSaveProto()                                                   **
**                                                                    **
**  This function sets up the callback for the WM_DELETE_WINDOW       **
**  protocol. To make this work, it must also modify the shell's      **
**  deleteResponse resource -- otherwise the shell kills the job.     **
**                                                                    **
***********************************************************************/

void InitSaveProto(void)
{
    a_del_win = XInternAtom( XtDisplay(editshell),
                             "WM_DELETE_WINDOW", FALSE );
    XmAddWMProtocolCallback( editshell, a_del_win, (XtPointer)SaveProtoCB, NULL );

    XtSetArg( arglist[0], XmNdeleteResponse, XmDO_NOTHING );
    XtSetValues( editshell, arglist, 1 );

    InitSaveAlert();
}



/***********************************************************************
**                                                                    **
**  InitSaveAlert()                                                   **
**                                                                    **
**  Creates the "Quit without Saving?" message box.                   **
**                                                                    **
**  Note: The "Help" button is changed to "Quit" for this dialog.     **
**                                                                    **
***********************************************************************/

static void InitSaveAlert()
{
  Widget	temp;

  if (EditorOptions & ED_MESSAGE)
    {
      save_alert = XmCreateWarningDialog( mainwin, "SendAlert", NULL, 0 );
      
      temp = XmMessageBoxGetChild( save_alert, XmDIALOG_OK_BUTTON );
      XtUnmanageChild(temp);
      temp = XmMessageBoxGetChild( save_alert, XmDIALOG_HELP_BUTTON );
      XtAddCallback( temp, XmNactivateCallback, (XtPointer)AlertSaveCB, "Quit" );
    }
  else
    {
      save_alert = XmCreateWarningDialog( mainwin, "SaveAlert", NULL, 0 );
      
      temp = XmMessageBoxGetChild( save_alert, XmDIALOG_OK_BUTTON );
      XtAddCallback( temp, XmNactivateCallback, (XtPointer)AlertSaveCB, "Save" );
      temp = XmMessageBoxGetChild( save_alert, XmDIALOG_HELP_BUTTON );
      XtAddCallback( temp, XmNactivateCallback, (XtPointer)AlertSaveCB, "Quit" );
    }
}



/***********************************************************************
**                                                                    **
**  SaveProtoCB( w, client_data, call_data )                          **
**                                                                    **
**  This function handles the callback for the WM_DELETE_WINDOW       **
**  protocol. It displays a dialog, which queries whether or not      **
**  the workspace should be saved.                                    **
**                                                                    **
***********************************************************************/

static void SaveProtoCB( w, client_data, call_data )
    Widget		w;
    caddr_t		client_data;
    XmAnyCallbackStruct	*call_data;
{
  if (changed)
    XtManageChild( save_alert );

  else
    EndEditor(FALSE);
}



/***********************************************************************
**                                                                    **
**  AlertSaveCB( w, client_data, call_data )                          **
**                                                                    **
**  Callback for the "Quit without Saving?" message box.              **
**  This function either quits or saves then quits.                   **
**                                                                    **
***********************************************************************/

static void AlertSaveCB( w, client_data, call_data )
    Widget      w;
    char        *client_data;
    caddr_t     call_data;
{
    if (!strcmp(client_data, "Save"))
	{
        changed = FALSE;
	XtAppAddWorkProc(app_context, (XtPointer)AlertSaveWP, NULL );
        FileSave();
	}
    else
      EndEditor(FALSE);
}



/***********************************************************************
**                                                                    **
**  AlertSaveWP( client_data )                                        **
**                                                                    **
**  Workproc to handle wait while user saves file. This function      **
**  simply waits until the global variable "saved" becomes TRUE.      **
**                                                                    **
***********************************************************************/

static Boolean AlertSaveWP( client_data )
    caddr_t	client_data;
{
    if (!changed)
	{
      EndEditor(FALSE);
	  return(TRUE);
	}
    else
      return( FALSE );
}

/***********************************************************************
**                                                                    **
**  InitOther()                                                       **
**                                                                    **
**  This function performs other program initialization, such as      **
**  loading any default data.                                         **
**                                                                    **
***********************************************************************/

void InitOther(void)
{
    InitAlerts();
    InitFiler();
    InitFindDB();
    InitSaveProto();
}

/***********************************************************************
**                                                                    **
**  StrFind( s1, s2 )                                                 **
**                                                                    **
**  Returns a pointer to the first occurrence of string s2 in         **
**  string s1. Returns NULL is s2 does not occur in s1.               **
**                                                                    **
***********************************************************************/

char *StrFind( s1, s2 )
    char	*s1;
    char	*s2;
{
    char	*tmps1;
    char	*tmps2;

    for ( ; *s1 != '\0'  ; s1++)
	{
		if (*s1 == *s2)
		{
		    tmps1 = s1;
		    tmps2 = s2;
		    for ( ; (*s1 == *tmps2) && (*s1 != '\0') ; s1++, tmps2++)
			;
		    if (*tmps2 == '\0')
				return( tmps1 );
		}
	}
    return( NULL );
}


/***********************************************************************
**                                                                    **
**  InitAlerts()                                                      **
**                                                                    **
**  Creates all message boxes.                                        **
**                                                                    **
***********************************************************************/

void InitAlerts(void)
{
    InitFindAlert();
}



/***********************************************************************
**                                                                    **
**  InitFindAlert()                                                   **
**                                                                    **
**  Creates the "Search Failed" message box for Find.                 **
**                                                                    **
***********************************************************************/

static void InitFindAlert(void)
{
    Widget	temp;

    find_alert = XmCreateMessageDialog( mainwin, "FindAlert", NULL, 0 );
    temp = XmMessageBoxGetChild( find_alert, XmDIALOG_CANCEL_BUTTON );
    XtUnmanageChild( temp );
    temp = XmMessageBoxGetChild( find_alert, XmDIALOG_HELP_BUTTON );
    XtUnmanageChild( temp );
}



/***********************************************************************
**                                                                    **
**  AlertFindFailed()                                                 **
**                                                                    **
**  Manages the "Search Failed" message box.                          **
**                                                                    **
***********************************************************************/

/*
void AlertFindFailed(void)
{
    XtManageChild( find_alert );
}
*/
