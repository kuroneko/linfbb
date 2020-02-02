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

static int LgList;
static int filtre_port;

static Widget CnxDialog;
static Widget CnxList;
static Widget ListPorts;
static Widget ToggleFilter;
static Widget TextFilter;

static void AfficheListe(void)
{
  long record ;
  FILE *fptr ;
  char *ptr ;
  int		valvoie;
  uchar		valport;
  char		valcall[8];
  char		buffer[80];
  statis	buffstat;
  int sub_mask = 0;
  int affich = 0;
  XmString string;
  char call_mask[10];


  XmListDeleteAllItems(CnxList);

  fptr = ouvre_stats() ;
  record = filelength(fileno(fptr)) ;

  record -= (long) (sizeof(statis) * 500) ;

  if (record < 0L)
    record = 0L;
  
  sub_mask = XmToggleButtonGetState(ToggleFilter);
  ptr = XmTextFieldGetString(TextFilter);
  strn_cpy(6, call_mask, ptr);
  XtFree(ptr);

  fseek(fptr, record, 0) ;
  
  for (;;)
    {
      if (fread((char *) &buffstat, sizeof(statis), 1, fptr) == 0)
	break;
      valport = buffstat.port + 'A' ;
      if ((filtre_port == 0) || (filtre_port == (buffstat.port + 1)))
	{
	  valvoie = buffstat.voie ;
	  n_cpy(6, valcall, buffstat.indcnx);
	  
	  if (*call_mask == '\0')
	    affich =1;
	  else if (sub_mask)
	    affich = (strstr(valcall, call_mask) != 0);
	  else
	    affich = (strcmp(valcall, call_mask) == 0);
	  
	  if (affich)
	    {
	      sprintf(buffer, "%c %-2d %6ld %-6s %s %3d'%02d",
		      valport, valvoie,
		      record / (long) sizeof(statis),
		      valcall, strdt(buffstat.datcnx),
		      buffstat.tpscnx / 60, buffstat.tpscnx % 60 ) ;
	      string = XmStringCreateSimple(buffer);
	      XmListAddItem(CnxList, string, 0);
	      XmStringFree(string);
	    }
	}
      record += (long) sizeof(statis);
    }

  XmListSetBottomPos(CnxList, 0);
  ferme(fptr, 43) ;
}

void ListeConnexions(void)
{
  XmString tstring[NBPORT+1];
  int i;
  int nb;
  Arg args[20] ;
  Cardinal n;

  tstring[0] = XmStringCreateSimple("All ports");
  nb = 1;
  for (i = 1 ; i < NBPORT ; i++)
    {
      if (p_port[i].pvalid)
	{
	  printf("Port %d = %s\n", i, p_port[i].freq);
	  tstring[nb] = XmStringCreateSimple(p_port[i].freq);
	  ++nb;
	}
    }

  n = 0;
  XtSetArg(args[n], XmNitems, tstring); n++;
  XtSetArg(args[n], XmNitemCount, nb); n++;
  XtSetArg(args[n], XmNvisibleItemCount, nb); n++;
  XtSetValues(XtNameToWidget(ListPorts,"*List"), args, n);  
}

void ListCnxCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if (XtIsManaged(CnxDialog))
    {
      /* Passer la fenetre au 1er plan */
      XRaiseWindow(XtDisplay(CnxDialog),XtWindow(XtParent(CnxDialog)));
      return;
    }

  cursor_wait();
  printf("PendingCB\n");
  LgList = 0;

  ListeConnexions();
  AfficheListe();

  XtManageChild(CnxDialog);
  end_wait();
}


static void CloseCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  XtUnmanageChild(CnxDialog);
}

static void PortCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  XmComboBoxCallbackStruct *cb = (XmComboBoxCallbackStruct *)call_data;

  filtre_port = cb->item_position - 1;
  AfficheListe();
}

static void ToggleFilterCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  AfficheListe();
}

static void TextFilterCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  AfficheListe();
}

void ListCnx(Widget toplevel)
{
  Arg args[20] ;
  Cardinal n;
  XmString string;
  Widget w;

  filtre_port = 0;

  n = 0;
  XtSetArg(args[n], XmNmarginHeight, 5);n++;
  XtSetArg(args[n], XmNmarginWidth, 5);n++;
  XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
  CnxDialog = XmCreateFormDialog(toplevel, "cnx_dialog", args, n);

  string = XmStringCreateSimple("All ports");

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  ToggleFilter = XmCreateToggleButton(CnxDialog, "toggle_filter", args, n);
  XtAddCallback (ToggleFilter, XmNvalueChangedCallback, ToggleFilterCB, NULL);
  XtManageChild(ToggleFilter);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftWidget, ToggleFilter);n++;
  TextFilter = XmCreateTextField(CnxDialog, "text_filter", args, n);
  XtAddCallback (TextFilter, XmNvalueChangedCallback, TextFilterCB, NULL);
  XtManageChild(TextFilter);

  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftWidget, TextFilter);n++;
  XtSetArg(args[n], XmNitems, &string); n++;
  XtSetArg(args[n], XmNitemCount, 1); n++;
  XtSetArg(args[n], XmNeditable, FALSE); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, FALSE);n++;
  ListPorts = XmCreateDropDownComboBox(CnxDialog, "combo_ports", args, n);
  XtAddCallback (ListPorts, XmNselectionCallback, PortCB, NULL);
  XtManageChild(ListPorts);

  n= 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNbottomWidget, ListPorts);n++;
  XtSetArg(args[n], XmNbottomOffset, 5);n++;
  XtSetArg(args[n], XmNhighlightThickness, 0);n++;
  XtSetArg(args[n], XmNvisibleItemCount, 20);n++;
  CnxList = XmCreateScrolledList(CnxDialog, "cnx_list", args, n);
  XtManageChild(CnxList);

  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);n++;
  XtSetArg(args[n], XmNtopWidget, CnxList);n++;
  XtSetArg(args[n], XmNleftWidget, ListPorts);n++;
  XtSetArg(args[n], XmNtopOffset, 5);n++;
  w = XmCreatePushButton(CnxDialog, "close", args, n);
  XtAddCallback(w, XmNactivateCallback, CloseCB, (XtPointer)CnxDialog);
  XtManageChild(w);
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
