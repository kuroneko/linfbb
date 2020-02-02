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

#define PUBLIC

#include <xfbb.h>
#include <serv.h>
#include <locale.h>	/* Added Satoshi Yasuda for NLS */
#include <sys/signal.h>
#include <Xm/Protocols.h>

#include <stdint.h>

#include <fbb.xbm>

static void HelpAct   (Widget, XEvent *, String *, Cardinal*);
static void ConsoleAct(Widget, XEvent *, String *, Cardinal*);
static void DisconnAct(Widget, XEvent *, String *, Cardinal*);
static void PendingAct(Widget, XEvent *, String *, Cardinal*);
static void MonitorAct(Widget, XEvent *, String *, Cardinal*);
static void SetCallAct(Widget, XEvent *, String *, Cardinal*);
static void ProgTncAct(Widget, XEvent *, String *, Cardinal*);
static void GatewayAct(Widget, XEvent *, String *, Cardinal*);
static void TalkToAct (Widget, XEvent *, String *, Cardinal*);
static void MsgScanAct(Widget, XEvent *, String *, Cardinal*);
static void EditorAct (Widget, XEvent *, String *, Cardinal*);
static void ListCnxAct(Widget, XEvent *, String *, Cardinal*);
static void SndTextAct(Widget, XEvent *, String *, Cardinal*);
static void ChoCB(Widget, XtPointer, XtPointer);
static void ScanCB(Widget, XtPointer, XtPointer);
static void FHelpCB(Widget, XtPointer, XEvent *);

static XtActionsRec actionsTable[] =
{
  { "Help",       HelpAct    },
  { "Console",    ConsoleAct },
  { "Disconnect", DisconnAct },
  { "PendingFwd", PendingAct },
  { "Monitor",    MonitorAct },
  { "SetCall",    SetCallAct },
  { "ProgTnc",    ProgTncAct },
  { "Gateway",    GatewayAct },
  { "TalkTo",     TalkToAct  },
  { "MsgScan",    MsgScanAct },
  { "Editor",     EditorAct  },
  { "ListCnx",    ListCnxAct },
  { "SndText",    SndTextAct }
};

static int init_phase = 1;
static int StopOn = FALSE;

static Widget MTalkTo;
static Widget MShow;
static Widget MInfos;
static Widget MDiscon;
static Widget MImDisc;
static Widget MEdit;
static Widget MScan;
static Widget MCho;

void TalkToCB(Widget w, XtPointer client_data, XtPointer call_data);
void OneChanCB(Widget w, XtPointer client_data, XtPointer call_data);
void DisconnectCB(Widget w, XtPointer client_data, XtPointer call_data);

static void sig_fct(int sig)
{
	/* int pid, pstatus; */

	sig &= 0xff;
	signal(sig, sig_fct);
	printf("Signal received = %d\n", sig);
	/* pid = wait(&pstatus); */

	switch(sig)
	{
	case SIGHUP :
		/* reload system files */
		printf("Update system files\n");
		init_buf_fwd();
		init_buf_swap();
		init_buf_rej();
		init_bbs();
		break;
	case SIGTERM:
		/* end of session */
		printf("Closing connections\n");
		maintenance() ;
		fbb_quit(1);
		break;
	case SIGBUS:
		/* end of session */
		fprintf(stderr, "xfbbd : Bus error\n");
		exit(5);
		break;
	case SIGSEGV:
		/* end of session */
		fprintf(stderr, "xfbb : Segmentation violation\n");
		exit(5);
		break;
	}
}

int SetChList(int clean)
{
  int i;
  int ok = 0;
  int pos = 0;

  for (i = 0; i < NBVOIES ; i++)
    {
      if ((svoie[i]->sta.connect))
	{
	  ++pos;
	  if (i == CurrentSelection)
	    {
	      XmListSelectPos(ConnectList, pos, TRUE);
	      ok = 1;
	      break;
	    }
	}
    }
  if ((clean) && (!ok))
    XmListDeselectAllItems(ConnectList);
	return 1;
}

int GetChList(void)
{
  int numero = -1;
  int *array;
  int pos;
  int nb;
  int i;

  if ((XmListGetSelectedPos(ConnectList, &array, &nb)) && (nb > 0))
    {
      /* position de la selection */
      nb = array[0];

      /* recherche du numero de canal */
      pos = 1;
      for (i = 0; i < NBVOIES ; i++)
	{
	  if (svoie[i]->sta.connect)
	    {
	      if (pos == nb)
		{
		  numero = i;
		  break;
		}
	      ++pos;
	    }
	}
      free(array);
    }
  return(numero);
}

static Pixel CreeCouleur(Widget w, int couleur)
{
  XColor tcolor;

  tcolor.pixel = 0;
  tcolor.red   = ((couleur >> 16) & 0xff) << 8;
  tcolor.green = ((couleur >> 8 ) & 0xff) << 8;
  tcolor.blue  = (couleur & 0xff) << 8;
  tcolor.flags = DoRed|DoGreen|DoBlue;
  XAllocColor(XtDisplay(w),
	      DefaultColormapOfScreen(XtScreen(w)),
	      &tcolor);
  return(tcolor.pixel);
}

static XmRendition CreeCouleurRendition(Widget w, Pixel couleur, char *nom)
{
  Arg args[10] ;
  Cardinal n;

  n = 0;
  XtSetArg(args[n], XmNrenditionForeground, couleur); n++;
  return XmRenditionCreate(w, nom, args, n); 
}

void CreateRendition(Widget w)
{
  r_index = 0;

  XtVaGetValues(form, XmNbackground, &df_pixel, NULL);

  rc_pixel = CreeCouleur(w, 0xff0000);
  r_rend[r_index++] = CreeCouleurRendition(w, rc_pixel, "RC"); 
  vc_pixel = CreeCouleur(w, 0x00ff00);
  r_rend[r_index++] = CreeCouleurRendition(w, vc_pixel, "VC"); 
  bc_pixel = CreeCouleur(w, 0x0000ff);
  r_rend[r_index++] = CreeCouleurRendition(w, bc_pixel, "BC"); 
  no_pixel = CreeCouleur(w, 0x000000);
  r_rend[r_index++] = CreeCouleurRendition(w, no_pixel, "NO"); 
  rf_pixel = CreeCouleur(w, 0x800000);
  r_rend[r_index++] = CreeCouleurRendition(w, rf_pixel, "RF"); 
  vf_pixel = CreeCouleur(w, 0x008000);
  r_rend[r_index++] = CreeCouleurRendition(w, vf_pixel, "VF"); 
  bf_pixel = CreeCouleur(w, 0x000080);
  r_rend[r_index++] = CreeCouleurRendition(w, bf_pixel, "BF");
}

void ToolIcons(void)
{
  int i;
  int numero;
  static int OldIconState[NB_ICON];
  int IconState[NB_ICON];

  static int first = 1;
  static int cho = -1;
  static int scanning = -1;
  static Pixel TopShadow;
  static Pixel BottomShadow;

  int port = is_pactor();

  if (first)
    {
      XtVaGetValues(BIcon[0], 
		    XmNtopShadowColor, &TopShadow,
		    XmNbottomShadowColor, &BottomShadow,
		    NULL);
      XtSetSensitive(BIcon[13], TRUE);
    }

  if (MenuPactor)
  {
    if (scanning != pactor_scan[port])
    {
      scanning = pactor_scan[port];
  	  XmToggleButtonSetState(MScan, scanning, FALSE);
    }
	
	if ((ONLINE(port)) && (cho != 1))
	  XtSetSensitive(MCho, TRUE);
	else if (cho != 0);
	  XtSetSensitive(MCho, FALSE);
    cho  = ONLINE(port);;
  }
  
  for (i = 0 ; i < NB_ICON ; i++)
    IconState[i] = FALSE;

  IconState[0] = TRUE;
  IconState[1] = TRUE;
  IconState[2] = TRUE;
  IconState[3] = TRUE;
  IconState[8] = TRUE;
  IconState[9] = TRUE;
  IconState[10] = TRUE;
  IconState[11] = TRUE;
  IconState[12] = !editor_on();
  IconState[13] = print;
  /* IconState[14] = TRUE; */
  IconState[15] = TRUE;

  numero = GetChList();
  if (numero == -1)
    {
      IconState[4] = FALSE;
      IconState[5] = FALSE;
      IconState[6] = FALSE;
      IconState[7] = FALSE;
    }
  else if (numero == CONSOLE)
    {
      IconState[4] = (svoie[CONSOLE]->sta.connect);
      IconState[5] = TRUE;
    }
  else
    {
      IconState[4] = TRUE;
      IconState[5] = TRUE;
      IconState[6] = TRUE;
      IconState[7] = (svoie[CONSOLE]->sta.connect == 0) && (can_talk(numero));
    }

  for (i = 0 ; i < NB_ICON ; i++)
    {
      if (OldIconState[i] != IconState[i])
	{
	  /* Menues associated to icons */
	  Widget MenuW = NULL;
	  Widget MenuX = NULL;
	  switch(i)
	    {
	    case 4:
	      MenuW = MShow;
	      break;
	    case 5:
	      MenuW = MInfos;
	      break;
	    case 6:
	      MenuW = MDiscon;
	      MenuX = MImDisc;
	      break;
	    case 7:
	      MenuW = MTalkTo;
	      break;
	    case 12:
	      MenuW = MEdit;
	      break;
	    default:
	      MenuW = NULL;
	      break;
	    }

	  if (i == 13)
	    {
	      /* Bouton PRINT */
	      if (IconState[i])
		{
		  XtVaSetValues(BIcon[i],
				XmNtopShadowColor, BottomShadow,
				XmNbottomShadowColor, TopShadow,
				NULL);
		}
	      else
		{
		  XtVaSetValues(BIcon[i],
				XmNtopShadowColor, TopShadow,
				XmNbottomShadowColor, BottomShadow,
				NULL);
		}
	    }
	  else
	    {
	      XtSetSensitive(BIcon[i], IconState[i]);
	      if (MenuW)
			XtSetSensitive(MenuW, IconState[i]);
	      if (MenuX)
			XtSetSensitive(MenuX, IconState[i]);
	    }
	  OldIconState[i] = IconState[i];
	}
    }
}

void Caption(void)
{
  time_t temps;
  struct tm tmg;
  struct tm tml;
  char buf[80];
  static int lmin = -1;

  temps = time(NULL);
  tmg = *(gmtime(&temps));
  tml = *(localtime(&temps));
  if (tmg.tm_min != lmin)
    {
      lmin = tmg.tm_min;
      sprintf(buf, "XFBB - %s-%d - %02d:%02d UTC",
	      mycall, myssid,
	      tmg.tm_hour, tmg.tm_min);
      XtVaSetValues(toplevel, XmNtitle, buf, NULL);
    }
}

void InitText(char *text)
{
  static char *initext[NB_INIT_B] = 
  {
    "Reading BBS config file",
    "Reading Texts (%s)",
    "Ports configuration (%s)",
    "TNC configuration (%s)",
    "Servers & PG (%s)",
    "Loading BIDs (%s)",
    "Callsigns set-up (%s)",
    "Messages set-up (%s)",
    "WP set-up (%s)",
    "Forward set-up (%s)",
    "BBS set-up (%s)"
  };
  char str[80];
  Arg args[10] ;
  Cardinal n = 0;

  int rouge = ((DEBUG) && ((init_phase == 2) || (init_phase == 3)));

  if (init_phase > NB_INIT_B)
    return;

  n = 0;
  XtSetArg(args[n], XmNset, TRUE); n++;
  XtSetValues(Tb[init_phase-1], args, n);

  sprintf(str, initext[init_phase-1], text);
  LabelSetString(Tb[init_phase-1], str, (rouge) ? "RC" : NULL);
}

Boolean InitWorkProc(XtPointer data)
{
  if (step_initialisations (init_phase))
    {
      /* Initialisation is finished */
      int i;

      /* Delete init window and map main window */
      for (i = 0 ; i < NB_INIT_B ; i++)
	{
	  XtUnmapWidget(Tb[i]);
	}
      
     
      XtManageChild(ConnectLabel);
      XtManageChild(ConnectString);
      XtManageChild(ConnectList);
      
      if (is_pactor())
	  {
	    Widget bouton;
        Arg args[10] ;
		
        MenuPactor = XmCreatePulldownMenu(MenuBar,"pactor", args, 0);
        XtSetArg(args[0], XmNsubMenuId, MenuPactor);
        bouton = XmCreateCascadeButton(MenuBar,"pactor", args, 1);
        XtManageChild(bouton);
        MScan = add_item(TOGGLE, MenuPactor, "scanning", ScanCB, NULL, 
	       "Start/stop frequency scanning", FHelpCB);
        MCho = add_item(BUTTON, MenuPactor, "changeover", ChoCB, NULL, 
	       "Send a change-over", FHelpCB);
      }

      return (TRUE);
    }

  /* Next step */
  ++init_phase;
  return(FALSE);
}

Boolean KernelWorkProc(XtPointer data)
{
  static int running = 0;

  if (running)
    {
      /*      printf("Kernel %d\n", ++val); */
      ToolIcons();
      Caption();
      kernel();
      FbbMem(0);
	if (is_idle)
	{
		usleep(1);
	}
	else
	{
		is_idle = 1;
	}
    }
  else
    {
      running = InitWorkProc(NULL);
    }
  return(FALSE);
}

/* right click in the list */
void PostIt(Widget w, Widget popup, XButtonEvent *event)
{
  int i;
  int nb;
  int numero;
  int pos;

  xprintf("YPos = %d\n", event->y);
  pos = XmListYToPos(ConnectList, event->y);

  nb = 0;
  for (i = 0 ; i < NBVOIES ; i++)
    {
      if (svoie[i]->sta.connect)
	++nb;
    }
  
  if ((pos == 0) || (pos > nb))
    {
      XmListDeselectAllItems(ConnectList);
      CurrentSelection = -1;
      return;
    }

  if (event->button != Button3)
    return;
  
  XmListSelectPos(ConnectList, pos, TRUE);

  numero = GetChList();
  CurrentSelection = numero;

  if (numero == -1)
    {
      for (i = 0 ; i < NB_PITEM ; i++)
	XtSetSensitive(PItem[i], FALSE);
    }
  else if (numero == CONSOLE)
    {
      XtSetSensitive(PItem[0], FALSE);
      XtSetSensitive(PItem[1], TRUE);
      XtSetSensitive(PItem[2], TRUE);
      XtSetSensitive(PItem[3], FALSE);
      XtSetSensitive(PItem[4], FALSE);
    }
  else
    {
      int val;

      val = (svoie[CONSOLE]->sta.connect == 0) && (can_talk(numero));
      XtSetSensitive(PItem[0], val);
      XtSetSensitive(PItem[1], TRUE);
      XtSetSensitive(PItem[2], TRUE);
      XtSetSensitive(PItem[3], TRUE);
      XtSetSensitive(PItem[4], TRUE);
    }

  XmMenuPosition(popup, event);
  XtManageChild(popup);
}

static void EditFileCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if (EditorOff)
    {
      EditorOff = FALSE;
      CreateEditor(NULL, NULL, "Edit system file", 0, 0);
    }
}

void ItemCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int numero = (uintptr_t ) client_data;

  printf("ItemCB = %d\n", numero);
  switch(numero)
    {
    case 0 : /* Talk */
      TalkToCB(w, client_data, call_data);
      break;
    case 1 : /* Show */
      OneChanCB(w, client_data, call_data);
      break;
    case 2 : /* Infos */
      InfoDialog(w, client_data, call_data);
      break;
    case 3 : /* Disconnect */
      DisconnectCB(w, client_data, call_data);
      break;
    case 4 : /* Immediate Disconnect */
      DisconnectCB(w, client_data, call_data);
      break;
    }
}

void ActiveListCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  CurrentSelection = GetChList();
}

void SelectListCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  CurrentSelection = GetChList();
  if (CurrentSelection != -1)
    {
      /* Selection d'un canal */
      ShowFbbWindow(CurrentSelection, toplevel);
    }
}

void ConsoleCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  /* Connexion en console */
  cursor_wait();
  ShowFbbWindow(CONSOLE, toplevel);
  connect_console();
  end_wait();
}

void GatewayCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  ShowFbbWindow(CONSOLE, toplevel);
  connect_tnc();
  traite_voie(CONSOLE);
}

void AllChanCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  ToggleFbbWindow(ALLCHAN, toplevel);
}

void OneChanCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int canal = GetChList();

  if (canal == 0)
    {
      ShowFbbWindow(canal, toplevel);
    }
  else if (canal > 0)
    {
      ToggleFbbWindow(canal, toplevel);
    }
  else
    {
      MessageBox(60, "No selected callsign", "DISPLAY", MB_ICONEXCLAMATION|MB_OK);
    }
}

void DisconnectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int voie = GetChList();
  int immediate = ((uintptr_t) client_data == 4);

  if (voie > 0)
    {
      int res;
      char texte[80];
	  if (immediate)
	      sprintf(texte, "Immediate disconnect %s", svoie[voie]->sta.indicatif.call);
	  else
	      sprintf(texte, "Disconnect %s", svoie[voie]->sta.indicatif.call);
      res = MessageBox(60, texte, "DISCONNECT", MB_ICONEXCLAMATION|MB_OKCANCEL);
      if (res == IDOK)
	{
	  disconnect_channel(voie, immediate);
	}
    }
  else
    {
      MessageBox(60, "No selected callsign", "DISCONNECT", MB_ICONEXCLAMATION|MB_OK);
    }
}

void MonitorCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  ToggleFbbWindow(MMONITOR, toplevel);
}

static void ScanCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  char cmde[80];
  int port = is_pactor();

  /* Start/Stop scanning */
  sprintf(cmde, "PTCTRX SCAN %d", !pactor_scan[port]);
  ptctrx(0, cmde);
}

static void ChoCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int port = is_pactor();

  if ((port) && (ONLINE(port)))
  {
	/* Tue l'eventuel timer en cours */
	del_timer(p_port[port].t_iss);
	p_port[port].t_iss = NULL;
	if (ISS(port))
	{
		tor_stop(p_port[port].pr_voie);
		printf("CHO\r\n");
	}
	else
	{
		tor_start(p_port[port].pr_voie);
		printf("BRK\r\n");
	}
  }
}

void ScanSysCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  cursor_wait();
  init_buf_fwd();
  init_buf_swap();
  init_buf_rej();
  init_bbs();
  end_wait();
}

void ScanMsgCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("ScanMsg = %d\n", 
	 ((XmToggleButtonCallbackStruct*)call_data)->set );
  scan_fwd(!p_forward); 
}

void OptionsCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("Option CB %d = %d\n", 
	 (uintptr_t)client_data,
	 ((XmToggleButtonCallbackStruct*)call_data)->set );
  set_option((uintptr_t)client_data,((XmToggleButtonCallbackStruct*)call_data)->set);
}

void TalkToCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int	voie;
  char	texte[80];
  char	callsign[80];
  
  voie = GetChList();

  if (voie > 0)
    {
      strcpy(callsign, svoie[voie]->sta.indicatif.call);
      
      if (!talk_to(voie))
	{
	  wsprintf(texte, "Can't talk to %s", callsign);
	  MessageBox(60,texte, "TALK", MB_ICONEXCLAMATION|MB_OK);
	}
    }
  else
    {
      MessageBox(60, "No selected callsign", "TALK", MB_ICONEXCLAMATION|MB_OK);
    }
}

void MaintCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if ((!StopOn) && (fct_arret(3)))
    {
      XtSetSensitive(BIcon[14], TRUE);
      XtMapWidget(BIcon[14]);
      StopOn = TRUE;
    }
}

void RerunCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if ((!StopOn) && (fct_arret(2)))
    {
      XtSetSensitive(BIcon[14], TRUE);
      XtMapWidget(BIcon[14]);
      StopOn = TRUE;
    }
}

void ExitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  printf("ExitCB\n");
  if ((!StopOn) && (fct_arret(1)))
    {
      XtSetSensitive(BIcon[14], TRUE);
      XtMapWidget(BIcon[14]);
      StopOn = TRUE;
    }
}

void StopCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if (operationnel == 1)
    {
      cursor_wait();
      maintenance() ;
      end_wait();
      fbb_quit(type_sortie);
    }
}

void PrintCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  print = !print;
 
  printf("print = %d\n", print);
  if (!print)
    close_print();
}

void LabelSetString(Widget label, char *text, char *attr)
{
  Arg args[10] ;
  Cardinal n = 0;
  XmString string;

  if (text == NULL)
    return;

  if (attr)
    string = XmStringGenerate(text, NULL, XmCHARSET_TEXT, attr);
  else
    string = XmStringCreateSimple(text);
  n = 0;
  XtSetArg(args[n], XmNlabelString, string); n++;
  XtSetValues(label, args, n);
  XmStringFree(string);

  FbbSync();
}

static void FHelpCB(Widget w, XtPointer client_data, XEvent *event)
{
  static XmString prev = NULL;

  if (client_data)
    {
      XmString string;

      if (prev)
	XmStringFree(prev);
      XtVaGetValues(Footer, XmNlabelString, &prev, NULL);

      string = XmStringCreateSimple((char *)client_data);
      XtVaSetValues(Footer, XmNlabelString, string, NULL);
      XmStringFree(string);
      foothelp = TRUE;
    }
  else
    {
      if (prev)
	XtVaSetValues(Footer, XmNlabelString, prev, NULL);
      foothelp = FALSE;
    }
}

Widget add_item(int type, Widget menu_pane, char *nom, 
		XtPointer callback, XtPointer data, 
		char *help, XtPointer HelpCB)
{
  Arg args[10] ;
  Cardinal n = 0;
  Widget bouton = NULL;

  if ((callback == NULL) && (type != SEPARATOR) && (type != TOOLBUTTON))
    {
      XtSetArg(args[n], XmNsensitive, False); n++;
    }

  switch (type)
    {
    case SEPARATOR:
      bouton = XmCreateSeparator(menu_pane, nom, args, n);
      break;
    case BUTTON:
      bouton = XmCreatePushButton(menu_pane, nom, args, n);
      if (callback)
	XtAddCallback(bouton, XmNactivateCallback, callback, data);
      break;
    case TOGGLE:
      XtSetArg(args[n], XmNvisibleWhenOff, True); n++;
      bouton = XmCreateToggleButton(menu_pane, nom, args, n);
      if (callback)
	XtAddCallback(bouton, XmNvalueChangedCallback, callback, data);
      break;
    case TOOLBUTTON:
      XtSetArg(args[n], XmNhighlightThickness, 0);n++;
      XtSetArg(args[n], XmNlabelType, XmPIXMAP);n++;
      XtSetArg(args[n], XmNsensitive, FALSE);n++;
      bouton = XmCreatePushButton(menu_pane, nom, args, n);
      if (callback)
	XtAddCallback(bouton, XmNactivateCallback, callback, data);
      break;
    }
 
  if (help)
    {
      XtAddEventHandler(bouton, EnterWindowMask, FALSE, 
			HelpCB, (XtPointer) help);
      XtAddEventHandler(bouton, LeaveWindowMask, FALSE,
			HelpCB, NULL);
    }
  XtManageChild(bouton);
  return(bouton);
}

void AddRT(Widget w)
{
  XmRenderTable rt;

  XtVaGetValues(w, XmNrenderTable, &rt, NULL, NULL);
    
  /* Make a copy so that setvalues will work correctly */
  rt = XmRenderTableCopy(rt, NULL, 0);
  rt = XmRenderTableAddRenditions(rt, r_rend, r_index, XmMERGE_NEW);
  XtVaSetValues(w, XmNrenderTable, rt, NULL, NULL);
  XmRenderTableFree(rt);
}

/* Translations */
static void HelpAct   (Widget w, XEvent *event, String *parms, Cardinal *num)
{
  AboutDialog(w, NULL, NULL);
}

static void ConsoleAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  ConsoleCB(w, NULL, NULL);
}

static void DisconnAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  DisconnectCB(w, NULL, NULL);
}

static void PendingAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  PendingCB(w, NULL, NULL);
}

static void MonitorAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  MonitorCB(w, NULL, NULL);
}

static void SetCallAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  CallsignDialog(w, NULL, NULL);
}

static void ProgTncAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
}

static void GatewayAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  GatewayCB(w, NULL, NULL);
}

static void TalkToAct (Widget w, XEvent *event, String *parms, Cardinal *num)
{
  TalkToCB(w, NULL, NULL);
}

static void MsgScanAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  TalkToCB(w, NULL, NULL);
}

static void EditorAct (Widget w, XEvent *event, String *parms, Cardinal *num)
{
  if (EditorOff)
    {
      EditorOff = FALSE;
      CreateEditor(NULL, NULL, "Edit system file", 0, 0);
    }
}

static void ListCnxAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  ListCnxCB(w, NULL, NULL);
}

static void SndTextAct(Widget w, XEvent *event, String *parms, Cardinal *num)
{
  printf("SndText <%s>\n", parms[0]);
}

int main(int ac, char **av)
{
  Pixmap pixmap;
  Widget bouton;
  Arg args[20] ;
  Cardinal n;
  XmString string;
  int i;
  Atom DelWindow;
  Display *display;
  Widget FormRC;
  int s;

  daemon_mode = 0;
  CurrentSelection = -1;
  p_fptr = NULL;
  setlocale(LC_CTYPE, "");	/* Added Satoshi Yasuda for NLS */

  for (i = 1 ; i < NSIG ; i++)
  {
    signal(i, sig_fct);
  }

#if 0
  toplevel = XtAppInitialize(&app_context, "xfbb", NULL, 0,
			     &ac, av, NULL,NULL, 0);
 
  XtAppAddActions(app_context, actionsTable, XtNumber(actionsTable));
#else
  XtToolkitInitialize();
  app_context = XtCreateApplicationContext();
  display = XtOpenDisplay(app_context, NULL, av[0], "xfbb", 
			  NULL, 0, &ac, av);
  if (display == NULL)
    {
      XtWarning("xfbb : cannot open display, exiting...");
      exit(0);
    }

  XtAppAddActions(app_context, actionsTable, XtNumber(actionsTable));
  toplevel = XtAppCreateShell(av[0], "xfbb", 
			      applicationShellWidgetClass,
			      display, NULL, 0);
#endif

	all_packets = 0;

	/* Parsing options */
	while ((s = getopt(ac, av, "ad")) != -1) {
		switch (s) {
			case 'a':
				all_packets = 1;
				break;
			case 'd':
				break;
			case '?':
				fprintf(stderr, "Usage: xfbb [-a]\n");
				return 0;
		}
	}

  n = 0;
  XtSetArg(args[n], XmNallowShellResize, True);n++;
  XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); ++n;
  XtSetValues(toplevel, args, n);

  DelWindow = XInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", FALSE);
  XmAddWMProtocolCallback(toplevel, DelWindow, ExitCB, NULL);

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 5);n++;
  XtSetArg(args[n], XmNmarginWidth, 5);n++;
  form = XmCreateForm(toplevel, "form", args, n);
  
  CreateRendition(toplevel);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  MenuBar = XmCreateMenuBar(form, "menu_bar", args, n);
  XtManageChild(MenuBar);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, MenuBar);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  /*
    XtSetArg(args[n], XmNmarginWidth, 0);n++;
    XtSetArg(args[n], XmNmarginHeight, 0);n++;
    XtSetArg(args[n], XmNspacing, 0);n++;
    */
  ToolBar = XmCreateRowColumn(form, "tool_bar", args, n);
  /* ToolBar = XmCreateMenuBar(form, "tool_bar", args, n); */

  /* Initialisation des boutons du menubar */
  bouton = 0;

  n = 0;
  MenuFile   = XmCreatePulldownMenu(MenuBar,"file", args, n);
  MenuUser   = XmCreatePulldownMenu(MenuBar,"user", args, n);
  MenuEdit   = XmCreatePulldownMenu(MenuBar,"edit", args, n);
  MenuWindow = XmCreatePulldownMenu(MenuBar,"window", args, n);
  MenuOptions= XmCreatePulldownMenu(MenuBar,"options", args, n);
  MenuConfig = XmCreatePulldownMenu(MenuBar,"config", args, n);
  MenuPactor = NULL;
  MenuHelp   = XmCreatePulldownMenu(MenuBar,"help", args, n);

  XtSetArg(args[0], XmNsubMenuId, MenuFile);
  bouton = XmCreateCascadeButton(MenuBar,"file", args, 1);
  XtManageChild(bouton);

  XtSetArg(args[0], XmNsubMenuId, MenuUser);
  bouton = XmCreateCascadeButton(MenuBar,"user", args, 1);
  XtManageChild(bouton);

  XtSetArg(args[0], XmNsubMenuId, MenuEdit);
  bouton = XmCreateCascadeButton(MenuBar,"edit", args, 1);
  XtManageChild(bouton);

  XtSetArg(args[0], XmNsubMenuId, MenuWindow);
  bouton = XmCreateCascadeButton(MenuBar,"window", args, 1);
  XtManageChild(bouton);

  XtSetArg(args[0], XmNsubMenuId, MenuOptions);
  bouton = XmCreateCascadeButton(MenuBar,"options", args, 1);
  XtManageChild(bouton);

  XtSetArg(args[0], XmNsubMenuId, MenuConfig);
  bouton = XmCreateCascadeButton(MenuBar,"config", args, 1);
  XtManageChild(bouton);

  XtSetArg(args[0], XmNsubMenuId, MenuHelp);
  bouton = XmCreateCascadeButton(MenuBar,"help", args, 1);
  XtManageChild(bouton);

  /* Bouton help a droite */
  XtVaSetValues(MenuBar, XmNmenuHelpWidget, bouton, NULL);

  XtManageChild(MenuFile);
  XtManageChild(MenuUser);
  XtManageChild(MenuEdit);
  XtManageChild(MenuWindow);
  XtManageChild(MenuFile);
  XtManageChild(MenuConfig);

  ScanSys = add_item(BUTTON, MenuFile, "scan_sys", ScanSysCB, NULL, 
	   "Tests the different system files for updating", FHelpCB);
  ScanMsg = add_item(TOGGLE, MenuFile, "scan_msg", ScanMsgCB, NULL, 
	   "Re-scans the list of messages", FHelpCB);
  add_item(SEPARATOR, MenuFile, "", NULL, NULL, NULL, NULL);
  add_item(BUTTON, MenuFile, "maintenance", MaintCB, NULL, 
	   "Exits and runs house-keeping tasks", FHelpCB);
  add_item(BUTTON, MenuFile, "re_run", RerunCB, NULL, 
	   "Re-Runs XFBB", FHelpCB);
  add_item(BUTTON, MenuFile, "exit", ExitCB, NULL, 
	   "End of the session", FHelpCB);
  MTalkTo = add_item(BUTTON, MenuUser, "talk", TalkToCB, NULL, 
	   "Talks to the selected user", FHelpCB);
  MShow = add_item(BUTTON, MenuUser, "show", OneChanCB, NULL, 
	   "Shows the traffic of the selectedd user", FHelpCB);
  MInfos = add_item(BUTTON, MenuUser, "infos", InfoDialog, NULL, 
	   "Give informations on the selected user", FHelpCB);
  MDiscon = add_item(BUTTON, MenuUser, "disconnect", DisconnectCB, (XtPointer)3, 
	   "Disconnects the selected user", FHelpCB);
  MImDisc = add_item(BUTTON, MenuUser, "immediatedisc", DisconnectCB, (XtPointer)4, 
	   "Immediately disconnects the selected user", FHelpCB);
  add_item(BUTTON, MenuUser, "last_connections", ListCnxCB, NULL, 
	   "Lists the last connections", FHelpCB);
  add_item(BUTTON, MenuEdit, "user", EditUsrCB, NULL, 
	   "Edits user's information", FHelpCB);
  add_item(BUTTON, MenuEdit, "message", EditMsgCB, NULL, 
	   "Edits message information and text", FHelpCB);
  add_item(BUTTON, MenuEdit, "forwarding", NULL, NULL, 
	   "Pending forward", FHelpCB);
  MEdit = add_item(BUTTON, MenuEdit, "system_file", EditFileCB, NULL, 
	   "Edits a system file", FHelpCB);
  add_item(BUTTON, MenuWindow, "console", ConsoleCB, NULL, 
	   "Console connection", FHelpCB);
  add_item(BUTTON, MenuWindow, "gateway", GatewayCB, NULL, 
	   "Gateway connection", FHelpCB);
  add_item(BUTTON, MenuWindow, "monitoring", MonitorCB, NULL, 
	   "Shows monitoring", FHelpCB);
  add_item(BUTTON, MenuWindow, "all_channels", AllChanCB, NULL, 
	   "Shows the traffic of all users", FHelpCB);
  Opt[CM_OPTIONEDIT]    = add_item(TOGGLE, MenuOptions, "edition", 
		 		  OptionsCB, (XtPointer)CM_OPTIONEDIT,
				  "Enable/disable full page editor", FHelpCB);
  Opt[CM_OPTIONJUSTIF]  = add_item(TOGGLE, MenuOptions, "justification", 
		 		  OptionsCB, (XtPointer)CM_OPTIONJUSTIF,
				  "Enable/disable console justification", FHelpCB);
  Opt[CM_OPTIONALARM]   = add_item(TOGGLE, MenuOptions, "connection_bip",
				   OptionsCB, (XtPointer)CM_OPTIONALARM,
				   "Enable/disable connection alarm", FHelpCB);
  Opt[CM_OPTIONCALL]    = add_item(TOGGLE, MenuOptions, "sysop_call", 
				   OptionsCB, (XtPointer)CM_OPTIONCALL, 
				   "Enable/disable sysop call", FHelpCB);
  Opt[CM_OPTIONGATEWAY] = add_item(TOGGLE, MenuOptions, "gateway", 
				   OptionsCB, (XtPointer)CM_OPTIONGATEWAY,
				   "Enable/disable gateway access", FHelpCB);
  Opt[CM_OPTIONAFFICH]  = add_item(TOGGLE, MenuOptions, "banners",
				   OptionsCB, (XtPointer)CM_OPTIONAFFICH,
				   "Enable/disable callsign banners", FHelpCB);
  Opt[CM_OPTIONSOUNDB]  = add_item(TOGGLE, MenuOptions, "soundcard",
				   OptionsCB, (XtPointer)CM_OPTIONSOUNDB,
				   "Enable/disable soundcard", FHelpCB);
  Opt[CM_OPTIONINEXPORT]= add_item(TOGGLE, MenuOptions, "inexport", 
		 		  OptionsCB, (XtPointer)CM_OPTIONINEXPORT,
				  "Enable/disable display of import/export mail", FHelpCB);
  add_item(BUTTON, MenuConfig, "tnc_parameters", NULL, NULL,
	   "Access to the low-level TNC programmation", FHelpCB);
  add_item(BUTTON, MenuConfig, "console_callsign", CallsignDialog, NULL,
	   "Specifies the callsign used in console", FHelpCB);
  add_item(BUTTON, MenuConfig, "main_parameters", NULL, NULL, 
	   "Main configuration of the software", FHelpCB);
  add_item(BUTTON, MenuConfig, "fonts", NULL, NULL, 
	   "Set the font of the text windows", FHelpCB);
  MScan = NULL;
  MCho  = NULL;
  add_item(BUTTON, MenuHelp, "contents", NULL, NULL, 
	   "Access online help", FHelpCB);
  add_item(BUTTON, MenuHelp, "copyright", CopyDialog, NULL, 
	   "Displays copyright informations", FHelpCB);
  add_item(SEPARATOR, MenuHelp, "", NULL, NULL, NULL, NULL);

  add_item(BUTTON, MenuHelp, "about", AboutDialog, NULL, 
	   "Informations on WinFBB software", FHelpCB);

  BIcon[0] = add_item(TOOLBUTTON, ToolBar, "B1",  ConsoleCB, NULL, 
		      "Console connection", FHelpCB);
  BIcon[1] = add_item(TOOLBUTTON, ToolBar, "B2",  GatewayCB, NULL, 
		      "Gateway connection", FHelpCB);
  BIcon[2] = add_item(TOOLBUTTON, ToolBar, "B3",  MonitorCB, NULL, 
		      "Monitoring window", FHelpCB);
  BIcon[3] = add_item(TOOLBUTTON, ToolBar, "B4",  AllChanCB, NULL,
		      "Shows the traffic of all users", FHelpCB);
  BIcon[4] = add_item(TOOLBUTTON, ToolBar, "B5",  OneChanCB, NULL,
		      "Shows the traffic of the selected user", FHelpCB);
  BIcon[5] = add_item(TOOLBUTTON, ToolBar, "B6",  InfoDialog, NULL, 
		      "Give informations on the selected user", FHelpCB);
  BIcon[6] = add_item(TOOLBUTTON, ToolBar, "B7",  DisconnectCB, NULL, 
		      "Disconnects the selected user ", FHelpCB);
  BIcon[7] = add_item(TOOLBUTTON, ToolBar, "B8",  TalkToCB, NULL, 
		      "Talk to the selected user", FHelpCB);
  BIcon[8] = add_item(TOOLBUTTON, ToolBar, "B9",  ListCnxCB, NULL, 
		      "Lists the last connections", FHelpCB);
  BIcon[9] = add_item(TOOLBUTTON, ToolBar, "B10", EditUsrCB, NULL, 
		      "Edits user's information", FHelpCB);
  BIcon[10] = add_item(TOOLBUTTON, ToolBar, "B11", EditMsgCB, NULL, 
		       "Edits message information and text", FHelpCB);
  BIcon[11] = add_item(TOOLBUTTON, ToolBar, "B12", PendingCB, NULL, 
		       "Pending forward", FHelpCB);
  BIcon[12] = add_item(TOOLBUTTON, ToolBar, "B13", EditFileCB, NULL, 
		       "Edits a system file", FHelpCB);
  BIcon[13] = add_item(TOOLBUTTON, ToolBar, "B14", PrintCB, NULL, 
		       "Opens printer", FHelpCB);
  BIcon[14] = add_item(TOOLBUTTON, ToolBar, "B15", StopCB, NULL, 
		       "Immediate stop", FHelpCB);
  BIcon[15] = add_item(TOOLBUTTON, ToolBar, "B16", AboutDialog, NULL, 
		       "On-line help", FHelpCB);

  XtVaSetValues(ToolBar, XmNmenuHelpWidget, BIcon[15], NULL);

  string = XmStringCreateSimple("  ");
  n = 0;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNlabelString, string);n++;
  Footer = XmCreateLabel(form, "footer", args, n);
  XtManageChild(Footer);
  XmStringFree(string);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, ToolBar);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  /* XtSetArg(args[n], XmNmappedWhenManaged, FALSE);n++; */
  ConnectLabel = XmCreateLabel(form, "list_label", args, n);
  AddRT(ConnectLabel);
  XtManageChild(ConnectLabel); 
  
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, ConnectLabel);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, Footer);n++;
  /* XtSetArg(args[n], XmNborderWidth, 1);n++; */
  StatForm = XmCreateForm(form, "stat_form", args, n);
  XtManageChild(StatForm);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, ConnectLabel);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNrightWidget, StatForm);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, Footer);n++;
  /* XtSetArg(args[n], XmNborderWidth, 1);n++; */
  ListForm = XmCreateForm(form, "list_form", args, n);
  XtManageChild(ListForm);

  string = XmStringCreateSimple("  ");
  for (i = 0 ; i < NB_INIT_B ; i++)
    {
      n = 0;
      if (i == 0)
	{
	  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
	}
      else
	{
	  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
	  XtSetArg(args[n], XmNtopWidget, Tb[i-1]);n++;
	}
      XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
      XtSetArg(args[n], XmNhighlightThickness, 0);n++;
      XtSetArg(args[n], XmNmarginHeight, 0);n++;
      XtSetArg(args[n], XmNlabelString, string);n++;
      Tb[i]= XmCreateToggleButton(ListForm, "toggle_b", args, n);
    }
  XtManageChildren(Tb, NB_INIT_B);
  XmStringFree(string);

  string = XmStringCreateSimple("Ch Callsign  Start Time  Rt Buf C/Fwd");
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNlabelString, string);n++;
  ConnectString = XmCreateLabel(ListForm, "list_label", args, n);
  /* XtManageChild(ConnectString);*/
  XmStringFree(string);
  
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, ConnectString);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC);n++;
  /* XtSetArg(args[n], XmNmappedWhenManaged, FALSE);n++; */
  ConnectList = XmCreateScrolledList(ListForm, "connect_list", args, n);
  XtAddCallback(ConnectList, XmNdefaultActionCallback, SelectListCB, NULL);
  XtAddCallback(ConnectList, XmNbrowseSelectionCallback, ActiveListCB, NULL);
  AddRT(ConnectList);

  Popup = XmCreatePopupMenu(ConnectList, "popup", NULL, 0);
  XtAddEventHandler(ConnectList,ButtonPressMask, False, 
		    (XtPointer)PostIt, (XtPointer)Popup);

  PItem[0] = XmCreatePushButtonGadget(Popup, "Talk", NULL, 0);
  PItem[1] = XmCreatePushButtonGadget(Popup, "Show", NULL, 0);
  PItem[2] = XmCreatePushButtonGadget(Popup, "Infos", NULL, 0);
  PItem[3] = XmCreatePushButtonGadget(Popup, "Disconnect", NULL, 0);
  PItem[4] = XmCreatePushButtonGadget(Popup, "Immediate Disc", NULL, 0);
  for (i = 0 ; i < NB_PITEM ; i++)
    XtAddCallback(PItem[i], XmNactivateCallback, ItemCB, (XtPointer)(intptr_t)i);
  XtManageChildren(PItem,NB_PITEM);

  n = 0;
  /* XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++; */
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  StatusLabel = XmCreateLabel(StatForm, "status_label", args, n);
  XtManageChild(StatusLabel);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL);n++;
  XtSetArg(args[n], XmNshowArrows, False);n++;
  Jauge = XmCreateScrollBar(StatForm, "jauge", args, n);
  XtManageChild(Jauge);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, StatusLabel);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftOffset, 2);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightOffset, 2);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, Jauge);n++;
  XtSetArg(args[n], XmNshadowThickness, 1);n++;
  XtSetArg(args[n], XmNshadowType, XmSHADOW_IN);n++;
  FormRC = XmCreateForm(StatForm, "stat_Flist", args, n);
  XtManageChild(FormRC);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);n++;
  XtSetArg(args[n], XmNnumColumns, 2);n++;
  StatList = XmCreateRowColumn(FormRC, "stat_list", args, n);
  XtManageChild(StatList);

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 1);n++;

  TxtUsed = XmCreateLabel(StatList, "txt_used", args, n);
  AddRT(TxtUsed);
  XtManageChild(TxtUsed);

  TxtGMem = XmCreateLabel(StatList, "txt_gmem", args, n);
  AddRT(TxtGMem);
  XtManageChild(TxtGMem);

  TxtDisk1 = XmCreateLabel(StatList, "txt_disk1", args, n);
  AddRT(TxtDisk1);
  XtManageChild(TxtDisk1);
  
  TxtDisk2 = XmCreateLabel(StatList, "txt_disk2", args, n);
  AddRT(TxtDisk2);
  XtManageChild(TxtDisk2);

  TxtMsgs = XmCreateLabel(StatList, "txt_msgs", args, n);
  AddRT(TxtMsgs);
  XtManageChild(TxtMsgs);

  TxtResync = XmCreateLabel(StatList, "txt_resync", args, n);
  AddRT(TxtResync);
  XtManageChild(TxtResync);

  TxtState = XmCreateLabel(StatList, "txt_state", args, n);
  AddRT(TxtState);
  XtManageChild(TxtState);

  TxtHold = XmCreateLabel(StatList, "txt_hold", args, n);
  AddRT(TxtHold);
  XtManageChild(TxtHold);

  TxtPriv = XmCreateLabel(StatList, "txt_priv", args, n);
  AddRT(TxtPriv);
  XtManageChild(TxtPriv);

  Used = XmCreateLabel(StatList, "used", args, n);
  AddRT(Used);
  XtManageChild(Used);

  GMem = XmCreateLabel(StatList, "gmem", args, n);
  AddRT(GMem);
  XtManageChild(GMem);

  Disk1 = XmCreateLabel(StatList, "disk1", args, n);
  AddRT(Disk1);
  XtManageChild(Disk1);
  
  Disk2 = XmCreateLabel(StatList, "disk2", args, n);
  AddRT(Disk2);
  XtManageChild(Disk2);

  Msgs = XmCreateLabel(StatList, "msgs", args, n);
  AddRT(Msgs);
  XtManageChild(Msgs);

  Resync = XmCreateLabel(StatList, "resync", args, n);
  AddRT(Resync);
  XtManageChild(Resync);

  State = XmCreateLabel(StatList, "state", args, n);
  AddRT(State);
  XtManageChild(State);

  Hold = XmCreateLabel(StatList, "hold", args, n);
  AddRT(Hold);
  XtManageChild(Hold);

  Priv = XmCreateLabel(StatList, "private", args, n);
  AddRT(Priv);
  XtManageChild(Priv);

  XtManageChild(ToolBar);
  XtManageChild(form);

  /* maj_menu_options(); */

  EditMessage(toplevel);
  EditUser(toplevel);
  PendingForward(toplevel);
  ListCnx(toplevel);

  /* Icone de la fenetre */
  pixmap = XCreateBitmapFromData(XtDisplay(toplevel), XtScreen(toplevel)->root,
				     fbb_bits, fbb_width, fbb_height);
  XtVaSetValues(toplevel, XmNiconPixmap, pixmap, NULL);

  XtRealizeWidget(toplevel);

  XtSetSensitive(MShow,   FALSE);
  XtSetSensitive(MTalkTo, FALSE);
  XtSetSensitive(MDiscon, FALSE);
  XtSetSensitive(MImDisc, FALSE);
  XtSetSensitive(MInfos,  FALSE);

  /* Initialisation sequences */
  XtAppAddWorkProc(app_context, KernelWorkProc, NULL);

  /* Main Loop */
  XtAppMainLoop(app_context);
  return(0);
}

/*
char *strupr(char *str)
{
char *tmp = str;
while (*tmp)
{
  if (islower(*tmp))
    *tmp = toupper(*tmp);
  ++tmp;
}
return str;
}
*/
  
char *itoa(int val, char *buffer, int base)
{
  sprintf(buffer, "%d", val);
  return buffer;
}

char *ltoa(long lval, char *buffer, int base)
{
  sprintf(buffer, "%ld", lval);
  return buffer;
}


char *ultoa(unsigned long lval, char *buffer, int base)
{
  sprintf(buffer, "%lu", lval);
  return buffer;
}


