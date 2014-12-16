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

#include <serv.h>
#include <xfbb.h>

#include <stdio.h>
#include <X11/X.h>
#include <Xm/Xm.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawingA.h>
#include <Xm/DialogS.h>
#include <Xm/TextF.h>
#include <Xm/Protocols.h>

typedef struct {
  char text[82];
  int pos_bis;
  long color;
  long color_bis;
} Line;

typedef struct {
  Widget drawing;
  Widget scroll;
  Widget frame;
  Widget edit;
  Widget line1;
  Widget line2;
  Widget line3;
  int premier;
  int nblignes;
  int scrollpos;
  int totlignes;
  int curligne;
  int curcol;
  Line *winbuf;
} WinInfo;

void HideFbbWindow(int numero, Widget parent);

static WinInfo *cnsl[TOTVOIES];
static Display *display = NULL;
static XFontStruct *fontinfo = NULL;
static GC dgc = NULL;
static long ColorVal[W_NCOL];
static int sysop_on = FALSE;
static Widget CallDialog;

#define NB_HISTO 20
static int  histo_pos;
static char history[NB_HISTO][82];

static void free_buffer(int numero);
static void alloc_buffer(int numero, int nblig);

#include <X11/cursorfont.h>
void cursor_wait(void)
{
  static Cursor lcursor = 0;

  if (lcursor == 0)
    {
      lcursor = XCreateFontCursor(display, XC_watch);
    }

  XDefineCursor(display, XtWindow(toplevel), lcursor);
}

void end_wait(void)
{
  XUndefineCursor(display, XtWindow(toplevel));
}

void resizeCB(Widget w, XtPointer data, XtPointer call)
{
  Arg args[20] ;
  Cardinal n;
  Dimension val;
  int pos;
  WinInfo *info = (WinInfo *)data;

  n = 0;
  XtSetArg(args[n], XmNheight, &val);n++;
  XtGetValues(info->drawing, args, n);

  info->nblignes = val / (fontinfo->ascent + fontinfo->descent);

  if (info->premier == info->curligne)
    {
      pos = info->totlignes - info->nblignes;
    }
  else
    {
      int offset;

      offset = info->curligne - info->premier;
      if (offset < 0)
	offset += info->totlignes;

      if (offset > (info->totlignes - info->nblignes))
	{
	  offset = info->totlignes - info->nblignes;
	  info->premier = info->curligne - offset;
	  if (info->premier < 0)
	    info->premier += info->totlignes;
	}

      pos = info->totlignes - info->nblignes - offset;
      if (pos < 0)
	pos += info->totlignes;
    }

  n = 0;
  XtSetArg(args[n], XmNpageIncrement, info->nblignes); n++;
  XtSetArg(args[n], XmNsliderSize, info->nblignes); n++;
  XtSetArg(args[n], XmNmaximum, info->totlignes); n++;
  XtSetArg(args[n], XmNvalue, pos); n++;
  XtSetValues(info->scroll, args, n);

  XClearArea(display, XtWindow(info->drawing),0, 0, 0, 0, TRUE);
}

void refreshCB(Widget w, XtPointer data, XtPointer call)
{
  int height;
  int offset;
  int i;
  int pos;
  WinInfo *info = (WinInfo *)data;

  offset = fontinfo->ascent;
  height = fontinfo->ascent + fontinfo->descent;

  pos = info->premier - info->nblignes + 1;
  if (pos < 0)
    pos += info->totlignes;

  for (i = 0 ; i < info->nblignes ; i++)
    {
      char *ptr = info->winbuf[pos].text;
      if (info->winbuf[pos].pos_bis == 0)
	{
	  XSetForeground(display, dgc, info->winbuf[pos].color);
	  XDrawImageString(display, XtWindow(w), 
			   dgc, 5,offset + i * height, 
			   ptr, 80);
	}
      else
	{
	  int largeur;

	  /* 1ere couleur */
	  XSetForeground(display, dgc,
			 info->winbuf[pos].color);
	  XDrawImageString(display, XtWindow(w), 
			   dgc, 5,offset + i * height, 
			   ptr, info->winbuf[pos].pos_bis);
	  largeur = XTextWidth(fontinfo, ptr, info->winbuf[pos].pos_bis);

	  /* 2eme couleur */
	  ptr = info->winbuf[pos].text + info->winbuf[pos].pos_bis;
	  XSetForeground(display, dgc, 
			 info->winbuf[pos].color_bis);
	  XDrawImageString(display, XtWindow(w), 
			   dgc, 5 + largeur,
			   offset + i * height, 
			   ptr, 80 - info->winbuf[pos].pos_bis);
	}
      ++pos;
      if (pos >= info->totlignes)
	pos = 0;
    }
}

void scrollCB(Widget w, XtPointer data, XtPointer call)
{
  Arg args[20] ;
  Cardinal n;
  int val;
  int offset;
  WinInfo *info = (WinInfo *)data;

  n = 0;
  XtSetArg(args[n], XmNvalue, &val);n++;
  XtGetValues(w, args, n);

  offset = info->totlignes - (val + info->nblignes);

  info->premier = info->curligne - offset ;
  if (info->premier < 0)
    info->premier += info->totlignes;
       
  XClearArea(display, XtWindow(info->drawing),0, 0, 1, 1, TRUE);
}

void quitCB(Widget w, XtPointer data, XtPointer call)
{
}

void scroll_window(WinInfo *info)
{
  Arg args[20] ;
  Cardinal n;
  int pos;
  int modscroll = 1;

  if (info->premier == info->curligne)
    {
      /* debut de buffer (ligne courante) */
      ++info->premier;
      if (info->premier == info->totlignes)
	info->premier = 0;
      modscroll = 0;
    }

  ++info->curligne;
  if (info->curligne == info->totlignes)
    info->curligne = 0;

  pos = info->curligne + info->nblignes - 1;
  if (pos > info->totlignes)
    pos -= info->totlignes;

  if (info->premier == pos)
    {
      /* fin de buffer */
      ++info->premier;
      if (info->premier == info->totlignes)
	info->premier = 0;
      modscroll = 2;
    }

  if (modscroll != 1)
    {
      if (info->frame)
	{
	  refreshCB(info->drawing, (XtPointer)info, NULL);
	  /* XClearArea(display, XtWindow(info->drawing),0, 0, 1, 1, TRUE); */
	  /* XFlush(display); */
	  /* XSync(display,0); */
	}
    }
  else if (info->frame)
    {
      /* mettre le scrollbar a jour */
      int pos;
      int offset;

      offset = info->curligne - info->premier;
      if (offset < 0)
	offset += info->totlignes;

      pos = info->totlignes - info->nblignes - offset;
      if (pos < 0)
	pos += info->totlignes;
       
      n = 0;
      XtSetArg(args[n], XmNvalue, pos); n++;
      XtSetValues(info->scroll, args, n);
    }
}

static void x_write(char *data, int len, int color, int numero)
{
  int pos;
  int cr;
  int i;
  int c;
  int reste = 1;
  char *ptr;
  long xcolor = ColorVal[color];
  WinInfo *info = cnsl[numero];

  if (info == NULL)
    return;

  pos = info->curcol;
  ptr = info->winbuf[info->curligne].text;

  cr = 0;

  if (info->frame)
    XSetForeground(display, dgc, xcolor);

  if ((pos) && (xcolor != info->winbuf[info->curligne].color))
    {
      info->winbuf[info->curligne].color_bis = xcolor;
      info->winbuf[info->curligne].pos_bis = pos;
    }
  else
    info->winbuf[info->curligne].color = xcolor;

  for(i = 0 ; i < len ; i++)
    {
      c = *data++;
      if (c == '\n')
	continue;
      if (c == '\r')
	{
	  int cpos;

	  if (*data == '\n')
	    {
	      --len;
	      ++data;
	    }

	  cpos = info->curligne + 1;
	  if (cpos >= info->totlignes)
	    cpos = 0;
	  ptr = info->winbuf[cpos].text;
	  ptr[80] = '\0';
	  memset(ptr, 0x20, 80);
	  info->winbuf[cpos].color = xcolor;
	  info->winbuf[cpos].pos_bis = 0;

	  reste = 0;
	  scroll_window(info);

	  pos = 0;
	}
      else
	{
	  reste = 1;
	  ptr[pos] = c;
	  ++pos;
	  if (pos == 80)
	    {
	      int cpos;

	      cpos = info->curligne + 1;
	      if (cpos >= info->totlignes)
		cpos = 0;
	      ptr = info->winbuf[cpos].text;
	      ptr[80] = '\0';
	      memset(ptr, 0x20, 80);
	      info->winbuf[cpos].color = xcolor;
	      info->winbuf[cpos].pos_bis = 0;

	      reste = 0;
	      scroll_window(info);

	      pos = 0;
	    }
	}
    }
  info->curcol = pos;

  if ((info->frame) && (info->premier == info->curligne) && reste)
    {
      char *cptr = info->winbuf[info->curligne].text;
      int offset = fontinfo->ascent;
      int height = fontinfo->ascent + fontinfo->descent;
      
      XDrawImageString(display, XtWindow(info->drawing), 
		       dgc, 5,
		       offset + (info->nblignes - 1) * height, 
		       cptr, 80);
      /* sleep(1); */
    }
}

void window_write(int numero, char *data, int len, int color, int header)
{
  if ((numero > 0) && (numero != MMONITOR) && ((numero != INEXPORT) || (aff_inexport)))
    {
      x_write(data, len, color, ALLCHAN);
    }
  if (!header)
    {
      x_write(data, len, color, numero);
    }
}

long dos2pixel(int couleur)
{
  static int xcolor[16];
 
  couleur &= 0xf;

  if (xcolor[couleur] == 0)
    {
      Colormap cmap;
      XColor color;
      unsigned long mask;
      int r, g, b;
      int val;
      unsigned long pixel;

      val = (couleur & 0x8) ? 0xff : 0x80;

      if (couleur == 7)
	r = g = b = 0xc0;
      else if (couleur == 8)
	r = g = b = 0x80;
      else
	{
	  r = g = b = 0;
	  if (couleur & 1)
	    r = val;
	  if (couleur & 2)
	    g = val;
	  if (couleur & 4)
	    b = val;
	}

      cmap = DefaultColormap(display, DefaultScreen(display));
      if (XAllocColorCells(display, cmap, FALSE, &mask, 0, &pixel, 1))
	{
	  color.pixel = pixel;
	  color.red = r << 8;
	  color.green = g << 8;
	  color.blue = b << 8;
	  color.flags = DoRed|DoGreen|DoBlue;
	  XStoreColor(display, cmap, &color);
	  xcolor[couleur] = pixel;
	}
      else
	printf("Cannot alloc color %x in colormap\n", couleur);
    }

  return(xcolor[couleur]);
}

void set_win_colors(void)
{
  ColorVal[W_SNDT] = dos2pixel(SEND);
  ColorVal[W_RCVT] = dos2pixel(RECV);
  ColorVal[W_CHNI] = dos2pixel(INDIC);
  ColorVal[W_MONH] = dos2pixel(HEADER);
  ColorVal[W_MOND] = dos2pixel(UI);
  ColorVal[W_CNST] = dos2pixel(CONS);
  ColorVal[W_BACK] = dos2pixel(FOND_VOIE);
  ColorVal[W_STAT] = dos2pixel(STA);		/* Couleur status */
  ColorVal[W_DEFL] = dos2pixel(DEF);		/* Couleur status */
  /* ColorVal[W_VOIE] = dos2pixel(VOIE); 	 Couleur voies */
  ColorVal[W_VOIE] = dos2pixel(INDIC); 	/* Couleur voies */
 
  /* Raffraichit les fenetres existantes 
  for (int i = 0 ; i < TOTVOIES ; i++)
    {
      if (Visu->Display[i])
	{
	  Visu->Display[i]->SetBkgndColor(FbbApp->ColorVal[W_BACK]);
	  Visu->Display[i]->Invalidate();
	}
    }
    */
}

void window_connect(int numero)
{
  char	buf[80];
  WinInfo *info;
  
  if (cnsl[numero] == NULL)
    alloc_buffer(numero, 100);
  info= cnsl[numero];

  if ((numero > 0) && (numero < MMONITOR))
    {
      char	ssid[20];
      int	num;
      
      num = (numero == INEXPORT) ? 99 : numero-1;
      
      if (svoie[numero]->sta.indicatif.num)
	sprintf(ssid, "-%d", svoie[numero]->sta.indicatif.num);
      else
	*ssid = '\0';
      sprintf(buf, "Channel %d : %s%s",
	      num, svoie[numero]->sta.indicatif.call, ssid);
      if (info->frame)
	{
	  Arg args[1] ;
	  XtSetArg(args[0], XmNtitle, buf);
	  XtSetValues(info->frame, args, 1);
	}
      if (numero > 0)
	{
	  sprintf(buf, "%d:%s%s", num, svoie[numero]->sta.indicatif.call, ssid);
	  /* FbbApp->UpdateWindowMenu(numero, buf, 2); */
	}
    }
}

void window_disconnect(int numero)
{
  WinInfo *info= cnsl[numero];

  if (info == NULL)
    return;

  if ((numero > 0) && (numero != MMONITOR))
    {
      char	buf[80];
      int	num;
      
      num = (numero == INEXPORT) ? 99 : numero-1;
      
      sprintf(buf, "Channel %d : None", num);
      if (info->frame)
	{
	  Arg args[1];
	  XtSetArg(args[0], XmNtitle, buf);
	  XtSetValues(info->frame, args, 1);
	}
      if (numero > 0)
	{
	  sprintf(buf, "%d:None", num);
	  /* FbbApp->UpdateWindowMenu(numero, buf, 2); */
	}
      if (!info->frame)
	free_buffer(numero);
    }

  if (numero == CurrentSelection)
    CurrentSelection = -1;
}

void window_init(void)
{
  int i;

  for (i = 0 ; i < TOTVOIES ; i++)
    {
      cnsl[i] = NULL;
    }

  display = XtDisplay(toplevel);
  /*
    Visu = new TVisus();
    InitBuffers();
    */
}

int get_win_lig(int numero)
{
  WinInfo *info = cnsl[numero];

  if (info)
    return (info->nblignes);
  return(25);
}

static void free_buffer(int numero)
{
  if (cnsl[numero])
    {
      free(cnsl[numero]->winbuf);
      free(cnsl[numero]);
      cnsl[numero] = NULL;
    }
}

static void alloc_buffer(int numero, int nblig)
{
  int i;

  if (cnsl[numero] == NULL)
    {
      WinInfo *info = (WinInfo *) calloc(sizeof(WinInfo), 1);
      if (info == NULL)
	{
	  printf("cannot allocate buffer for channel %d\n", numero);
	  exit(2);
	}
 
      info->winbuf = (Line *)calloc(sizeof(Line), nblig);
      info->totlignes = nblig;
      info->nblignes = 25;

      for (i = 0; i < nblig ; i++)
	{
	  info->winbuf[i].text[80] = '\0';
	  memset(info->winbuf[i].text, 0x20, 80);
	  info->winbuf[i].color = 0;
	  info->winbuf[i].color_bis = 0;
	  info->winbuf[i].pos_bis = 0;
	}

      cnsl[numero] = info;
    }
}

void sysop_end_chat(void)
{
  if (v_tell)
    {
      selvoie(CONSOLE) ;
      maj_niv(0, 0, 0) ;
      pvoie->sta.connect = FALSE ;
      selvoie(v_tell) ;
      maj_niv(pvoie->sniv1, pvoie->sniv2, pvoie->sniv3) ;
      prompt(pvoie->finf.flags, pvoie->niv1) ;
      pvoie->seq = v_tell = 0 ;
    }
}

int sysop_chat(void)
{
  if ((!svoie[CONSOLE]->sta.connect) && (svoie[v_tell]->sta.connect))
    {
      ShowFbbWindow(CONSOLE, toplevel);
      selvoie(CONSOLE) ;
      pvoie->sta.connect = 16 ;
      pvoie->deconnect = FALSE ;
      pvoie->pack = 0;
      pvoie->read_only = 0;
      pvoie->vdisk = 2;
      pvoie->xferok = 1;
      pvoie->mess_recu = 1 ;
      pvoie->mbl = 0;
      init_timout(CONSOLE) ;
      pvoie->temp3 = 0 ;
      pvoie->nb_err = 0;
      pvoie->finf.lang = langue[0]->numlang ;
      init_langue(voiecur) ;
      maj_niv(N_MBL, 9, 2) ;
      selvoie(v_tell) ;
      init_langue(voiecur) ;
      maj_niv(N_MBL, 9, 2) ;
      texte(T_MBL + 15) ;
      return(1);
    }
  return(0);
}
static void CallOkCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  sysop_chat();
  sysop_end();
}

static void CallCancelCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  sysop_end();
}

void sysop_end(void)
{
  if (sysop_on)
    {
      XtUnmanageChild(CallDialog);
      XtDestroyWidget(CallDialog);
      sysop_on = FALSE;
    }
}

void sysop_call(char *texte)
{
  Arg args[20] ;
  Cardinal n;
  XmString string;

  if (!sysop_on)
    {
      sysop_on = TRUE;

      string = XmStringCreateSimple(texte);

      /* Cree la fenetre de l'appel */
      n = 0;
      XtSetArg(args[n], XmNautoUnmanage, FALSE);n++;
      XtSetArg(args[n], XmNtitle, "Sysop call");n++;
      XtSetArg(args[n], XmNmessageString, string);n++;
      CallDialog = XmCreateMessageDialog(toplevel, "call_dialog", args, n);
      XtAddCallback(CallDialog, XmNokCallback, CallOkCB, NULL);
      XtAddCallback(CallDialog, XmNcancelCallback, CallCancelCB, NULL);
      XtUnmanageChild(XmMessageBoxGetChild(CallDialog, XmDIALOG_HELP_BUTTON));
      XtManageChild(CallDialog);

      XmStringFree(string);
    }
}

static void deleteCB(Widget w, XtPointer data, XtPointer call)
{
  int numero = (int)data;

  HideFbbWindow(numero, NULL);
  if (numero == CONSOLE)
    {
      sysop_end_chat();
      /* Deconnecter la console si connectee */
      if (svoie[CONSOLE]->sta.connect)
	deconnexion(CONSOLE, 0);
    }
  else if ((numero != ALLCHAN) && (numero != MMONITOR) && (!svoie[numero]->sta.connect))
    {
      free_buffer(numero);
    }
  if (numero > 0)
    {
      /* FbbApp->UpdateWindowMenu(Numero, NULL, 0); */
    }
}

/*
static void focusCB(Widget w, XtPointer data, XtPointer call)
{
  WinInfo *info = (WinInfo *)data;

  XtSetKeyboardFocus(w, info->edit);
}
*/
/*
  static void modifCB(Widget w, XtPointer data, XtPointer call)
  { 
  printf("ModifCB\n");
  }
*/

static void editCB(Widget w, XtPointer data, XtPointer call)
{
  char *ptr;
  char buffer[82];
  WinInfo *info = (WinInfo *)data;

  ptr = XmTextFieldGetString(info->line2);
  XmTextFieldSetString(info->line3, ptr);
  ptr = XmTextFieldGetString(info->line1);
  XmTextFieldSetString(info->line2, ptr);
  ptr = XmTextFieldGetString(info->edit);
  XmTextFieldSetString(info->line1, ptr);
  XmTextFieldSetString(info->edit,"");

  strcpy(buffer, ptr);
  if (++histo_pos == NB_HISTO)
    histo_pos = 0;
  strcpy(history[histo_pos], buffer);

  strcat(buffer, "\r");
  window_write(CONSOLE, buffer, strlen(buffer), W_CNST, 0);
  console_inbuf(buffer, strlen(buffer));
}

static void KeyEV(Widget w, XtPointer data, XKeyEvent *event)
{
  int i;
  int lg;
  int code;
  char *ptr;
  XmTextPosition pos;
  WinInfo *info = (WinInfo *)data;

  code = event->keycode;
  if (event->state & 1)
    code |= 0x100; /* shift */
  if (event->state & 4)
    code |= 0x400; /* control */
  if (event->state & 8)
    code |= 0x800; /* alt */

  switch(event->keycode)
    {
    case 0x009 : /* escape */
      ptr = XmTextFieldGetString(info->edit);
      if (strlen(ptr) == 0)
	{
	  /* commande Esc */
	  console_inbuf("\033\r", 2);
	}
      else
	{
	  /* Efface la commande en cours */
	  XmTextFieldSetString(info->edit, "");
	}
      XtFree(ptr);
      break;
    case 0x062 : /* UP */
    case 0x162 : /* Shift-UP */
      /* ligne precedente */
      for (i = 0 ; i < NB_HISTO ; i++)
	{
	  char *ptr = history[histo_pos];
	  if (histo_pos == 0)
	    histo_pos = NB_HISTO;
	  --histo_pos;
	  if (*ptr)
	    {
	      XmTextFieldSetString(info->edit, ptr);
	      XmTextFieldSetInsertionPosition(info->edit, 80);
	      break;
	    }
	}
      break;
    case 0x068 : /* DW */
    case 0x168 : /* Shift-DW */
      /* ligne suivante */
      for (i = 0 ; i < NB_HISTO ; i++)
	{
	  if (++histo_pos == NB_HISTO)
	    histo_pos = 0;
	  if (*history[histo_pos])
	    {
	      XmTextFieldSetString(info->edit,history[histo_pos]);
	      XmTextFieldSetInsertionPosition(info->edit, 80);
	      break;
	    }
	}
      break;
    case 0x462 : /* Ctrl-UP */
      /* Scroll UP */
      break;
    case 0x468 : /* Ctrl-DW */
      /* scroll DW */
      break;
    case 0x063 : /* PUP */
    case 0x163 : /* Shift-PUP */
      break;
    case 0x069 : /* PDW */
    case 0x169 : /* Shift-PDW */
      break;
    case 0x463 : /* Ctrl-PUP */
      break;
    case 0x469 : /* Ctrl-PDW */
      break;
    }

  /* Teste la fin de ligne */
  pos = XmTextFieldGetInsertionPosition(info->edit);
  lg = XmTextFieldGetMaxLength(info->edit);
  if (pos == lg)
    {
      char	*scan;

      ptr = XmTextFieldGetString(info->edit);
      scan = strrchr(ptr, ' ');
      if (scan)
	{
	  *scan++ = '\0';
	}
      XmTextFieldSetString(info->edit, ptr);
      editCB(w, info, NULL);

      if ((scan) && (*scan))
	{
	  XmTextFieldSetString(info->edit, scan);
	  XmTextFieldSetInsertionPosition(info->edit, 80);
	}

      XtFree(ptr);
    }
}

WinInfo *xcnsl(int numero, Widget pere, int nblig, Arg *argp, Cardinal np)
{
  Arg args[20] ;
  Cardinal n;
  Pixel foreground;
  Pixel background;
  Widget form;
  WinInfo *info;
  int mask;
  XGCValues values;
  Dimension hg;
  Widget separe;
  int console = (numero == CONSOLE) ;

  if (cnsl[numero] == NULL)
    alloc_buffer(numero, nblig);

  info = cnsl[numero];

  form = XmCreateForm(pere, "form", argp, np);

  /* XtAddCallback(form, XmNfocusCallback, focusCB, info); */

  if (display == NULL)
    display = XtDisplay(form);
  if (fontinfo == NULL)
    fontinfo = XLoadQueryFont(display, "6x10");

  if (console)
    {

      XmFontList fontlist;

      memset(history, 0, NB_HISTO * 82);

      fontlist = XmFontListCreate(fontinfo, XmSTRING_DEFAULT_CHARSET);

      n = 0;
      XtSetArg(args[n], XmNleftAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNmarginHeight,0); n++;
      XtSetArg(args[n], XmNmaxLength,78); n++;
      XtSetArg(args[n], XmNshadowThickness,0); n++;
      XtSetArg(args[n], XmNhighlightThickness,0); n++;
      XtSetArg(args[n], XmNfontList,fontlist); n++;      
      info->edit = XmCreateTextField(form, "edit", args, n);
      XtManageChild(info->edit);

      n = 0;
      XtSetArg(args[n], XmNleftAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_WIDGET); n++;
      XtSetArg(args[n], XmNbottomWidget,info->edit); n++;
      XtSetArg(args[n], XmNeditable,FALSE); n++;
      XtSetArg(args[n], XmNmarginHeight,0); n++;
      /* XtSetArg(args[n], XmNmarginWidth,0); n++;*/
      XtSetArg(args[n], XmNshadowThickness,0); n++;
      XtSetArg(args[n], XmNhighlightThickness,0); n++;
      XtSetArg(args[n], XmNcursorPositionVisible,FALSE); n++;
      XtSetArg(args[n], XmNfontList,fontlist); n++;      
      info->line1 = XmCreateTextField(form, "line1", args, n);
      XtManageChild(info->line1);
      XtSetKeyboardFocus(info->line1, info->edit);

      n = 0;
      XtSetArg(args[n], XmNleftAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_WIDGET); n++;
      XtSetArg(args[n], XmNbottomWidget,info->line1); n++;
      XtSetArg(args[n], XmNeditable,FALSE); n++;
      XtSetArg(args[n], XmNmarginHeight,0); n++;
      /* XtSetArg(args[n], XmNmarginWidth,0); n++; */
      XtSetArg(args[n], XmNshadowThickness,0); n++;
      XtSetArg(args[n], XmNhighlightThickness,0); n++;
      XtSetArg(args[n], XmNcursorPositionVisible,FALSE); n++;
      XtSetArg(args[n], XmNfontList,fontlist); n++;      
      info->line2 = XmCreateTextField(form, "line2", args, n);
      XtManageChild(info->line2);
      XtSetKeyboardFocus(info->line2, info->edit);

      n = 0;
      XtSetArg(args[n], XmNleftAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_WIDGET); n++;
      XtSetArg(args[n], XmNbottomWidget,info->line2); n++;
      XtSetArg(args[n], XmNeditable,FALSE); n++;
      XtSetArg(args[n], XmNmarginHeight,0); n++;
      /* XtSetArg(args[n], XmNmarginWidth,0); n++; */
      XtSetArg(args[n], XmNshadowThickness,0); n++;
      XtSetArg(args[n], XmNhighlightThickness,0); n++;
      XtSetArg(args[n], XmNcursorPositionVisible,FALSE); n++;
      XtSetArg(args[n], XmNfontList,fontlist); n++;      
      info->line3 = XmCreateTextField(form, "line3", args, n);
      XtManageChild(info->line3);
      XtSetKeyboardFocus(info->line3, info->edit);

      n = 0;
      XtSetArg(args[n], XmNleftAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_WIDGET); n++;
      XtSetArg(args[n], XmNbottomWidget,info->line3); n++;
      separe = XmCreateSeparator(form, "separator", args, n);
      XtManageChild(separe);

      XtAddCallback(info->edit, XmNactivateCallback, editCB, (XtPointer)info);
      /* XtAddCallback(info->edit, XmNmodifyVerifyCallback, modifCB, info); */
      XtAddEventHandler(info->edit, KeyPressMask, FALSE, (XtEventHandler)KeyEV, (XtPointer)info);
      XmFontListFree(fontlist);

      XtSetKeyboardFocus(form, info->edit);

	  n = 0;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_WIDGET); n++;
      XtSetArg(args[n], XmNbottomWidget,separe); n++;
	  XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM); n++;
	  XtSetArg(args[n], XmNtopAttachment,XmATTACH_FORM); n++;
	  XtSetArg(args[n], XmNmaximum,nblig); n++;
	  info->scroll = XmCreateScrollBar(form, "scroll", args, n);
	  XtManageChild(info->scroll);

	  n = 0;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_WIDGET); n++;
      XtSetArg(args[n], XmNbottomWidget,separe); n++;
	}
  else
    {
	  n = 0;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM); n++;
	  XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM); n++;
	  XtSetArg(args[n], XmNtopAttachment,XmATTACH_FORM); n++;
	  XtSetArg(args[n], XmNmaximum,nblig); n++;
	  info->scroll = XmCreateScrollBar(form, "scroll", args, n);
	  XtManageChild(info->scroll);
 
	  n = 0;
      XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM); n++;
    }

  XtSetArg(args[n], XmNtopAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment,XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget,info->scroll); n++;
  XtSetArg(args[n], XmNwidth,500); n++;
  XtSetArg(args[n], XmNheight,200); n++;
  info->drawing = XmCreateDrawingArea(form, "drawing", args, n);

  XtManageChild(info->drawing);
  XtManageChild(form);

  XtSetKeyboardFocus(info->drawing, info->edit);

  n = 0;
  XtSetArg(args[n], XmNforeground, &foreground); n++;
  XtSetArg(args[n], XmNbackground, &background); n++;
  XtSetArg(args[n], XmNheight, &hg);n++;
  XtGetValues(info->drawing, args, n);

  /* info->premier = 0; */

  mask = GCForeground | GCBackground | GCFont;
  values.background = background;
  values.foreground = foreground;
  values.font = fontinfo->fid;

  if (dgc == NULL)
    dgc = XtGetGC(info->drawing, mask, &values);

  XtAddCallback(info->scroll, XmNvalueChangedCallback, scrollCB, info);
  XtAddCallback(info->scroll, XmNdragCallback, scrollCB, info);
  XtAddCallback(info->drawing, XmNexposeCallback, refreshCB, info);
  XtAddCallback(info->drawing, XmNresizeCallback, resizeCB, info);

  info->nblignes = hg / (fontinfo->ascent + fontinfo->descent);

  n = 0;
  XtSetArg(args[n], XmNpageIncrement, info->nblignes); n++;
  XtSetArg(args[n], XmNsliderSize, info->nblignes); n++;
  XtSetArg(args[n], XmNmaximum, info->totlignes); n++;
  XtSetArg(args[n], XmNvalue, info->totlignes - info->nblignes); n++;
  XtSetValues(info->scroll, args, n);

  return(info);
}

void ShowFbbWindow(int numero, Widget parent)
{
  WinInfo*info;

  if (cnsl[numero] == NULL)
    alloc_buffer(numero, 100);
  info = cnsl[numero];

  if (info->frame)
    {
      if (XtIsManaged(info->frame))
	{
	  XRaiseWindow(XtDisplay(info->frame),XtWindow(info->frame));
	}
      else
	XtManageChild(info->frame);
    }
  else
    {
      Arg args[20] ;
      Cardinal n;
      Atom DelWindow;
      char buf[80];

      if (numero == CONSOLE)
	{
	  sprintf(buf, "Console");
	}
      else if (numero == MMONITOR)
	{
	  sprintf(buf, "Monitoring");
	}
      else if (numero == ALLCHAN)
	{
	  sprintf(buf, "All channels");
	}
      else
	{
	  char	ssid[20];
	  int	num;
	  
	  num = (numero == INEXPORT) ? 99 : numero-1;
	  
	  /* Fenetres canaux ...*/
	  if (svoie[numero]->sta.indicatif.num)
	    sprintf(ssid, "-%d", svoie[numero]->sta.indicatif.num);
	  else
	    *ssid = '\0';
	  
	  sprintf(buf, "Channel %d : %s%s",
		   num, svoie[numero]->sta.indicatif.call, ssid);
	}

      n = 0;
      XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); ++n;
      XtSetArg(args[n], XmNtitle, buf); ++n;
      info->frame = XmCreateDialogShell(parent, "cnsl", args, n);
      xcnsl(numero, info->frame, 100, NULL, 0);
      XtManageChild(info->frame);
      
      DelWindow = XInternAtom(display, "WM_DELETE_WINDOW", FALSE);
      XmAddWMProtocolCallback(info->frame, DelWindow, deleteCB, (XtPointer)numero);
    }
}

void HideFbbWindow(int numero, Widget parent)
{
  if ((cnsl[numero]) && (cnsl[numero]->frame) &&
      (XtIsManaged(cnsl[numero]->frame)))
    {
      XtUnmanageChild(cnsl[numero]->frame);
    }
}

void ToggleFbbWindow(int numero, Widget parent)
{
  if ((cnsl[numero]) && (cnsl[numero]->frame) &&
      (XtIsManaged(cnsl[numero]->frame)))
    {
      HideFbbWindow(numero, parent);
    }
  else
    {
      ShowFbbWindow(numero, parent);
    }
}

#ifdef __TEST__

XtAppContext app_context;
int connected;

static void TimeoutEvent(XtPointer data, XtIntervalId *Id)
{
  static int i=0;
  char text[82];

  sprintf(text, "Bonjour numero %d. Je suis une ligne ! * ", i++);
  window_write(1, text, strlen(text), i%16, 0);

  if (connected)
    XtAppAddTimeOut(app_context, 500L, TimeoutEvent, NULL);
}

void ConnectCB(Widget w, XtPointer data, XtPointer call)
{
  if (!connected)
    {
      window_connect(1);
      XtAppAddTimeOut(app_context, 500L, TimeoutEvent, 0);
    }
  connected = TRUE;
}

void DisconnectCB(Widget w, XtPointer data, XtPointer call)
{
  window_disconnect(1);
  connected = FALSE;
}

void ShowCB(Widget w, XtPointer data, XtPointer call)
{
  ShowFbbWindow(1, (Widget)data);
}

void HideCB(Widget w, XtPointer data, XtPointer call)
{
  HideFbbWindow(1, (Widget)data);
}

void ToggleCB(Widget w, XtPointer data, XtPointer call)
{
  ToggleFbbWindow(1, (Widget)data);
}

main(int ac, char **av)
{
  Arg args[20] ;
  Cardinal n;
  Widget toplevel;
  Widget rc;
  Widget bp;

  svoie[1] = (Svoie *)calloc(sizeof(Svoie), 1);
  strcpy(svoie[1]->sta.indicatif.call, "F6FBB");
  svoie[1]->sta.indicatif.num = 1;

  SEND = 0;
  RECV = 2;
  INDIC = 3;
  UI = 4;
  CONS = 5;
  FOND_VOIE = 6;
  HEADER = 7;

  toplevel = XtAppInitialize(&app_context, "TM", NULL, 0,
			     &ac, av, NULL,NULL, 0);
  n = 0;
  XtSetArg(args[n], XmNallowShellResize, True);n++;
  XtSetValues(toplevel, args, n);

  rc = XmCreateRowColumn(toplevel, "rc", NULL, 0);

  bp = XmCreatePushButton(rc, "Connect", NULL, 0);
  XtAddCallback(bp, XmNactivateCallback, ConnectCB, toplevel);
  XtManageChild(bp);

  bp = XmCreatePushButton(rc, "Disconnect", NULL, 0);
  XtAddCallback(bp, XmNactivateCallback, DisconnectCB, toplevel);
  XtManageChild(bp);

  bp = XmCreatePushButton(rc, "Show", NULL, 0);
  XtAddCallback(bp, XmNactivateCallback, ShowCB, toplevel);
  XtManageChild(bp);

  bp = XmCreatePushButton(rc, "Hide", NULL, 0);
  XtAddCallback(bp, XmNactivateCallback, HideCB, toplevel);
  XtManageChild(bp);

  bp = XmCreatePushButton(rc, "Toggle", NULL, 0);
  XtAddCallback(bp, XmNactivateCallback, ToggleCB, toplevel);
  XtManageChild(bp);

  XtManageChild(rc);

  /*
    alloc_buffer(0);
    cnsl[0] = xcnsl(toplevel,100, NULL, 0);
    */

  XtRealizeWidget(toplevel);

  display = XtDisplay(toplevel);
  set_win_colors();

  /* Main Loop */
  XtAppMainLoop(app_context);
}

#endif

