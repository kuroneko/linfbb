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


#include <xfbbX.h>

#include <stdio.h>
#include <stdlib.h>
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

#include <stdint.h>

/* Index des couleurs */
#define W_SNDT	0				/* Envoie data */
#define W_RCVT	1				/* Recoit data */
#define W_CHNI	2				/* Canal Information */
#define W_MONH	3				/* Monitoring header */
#define W_MOND 4				/* Monitoring data */
#define W_CNST 5				/* Console text */
#define W_BACK 6				/* Background */
#define W_STAT 7				/* Fenetre de status */
#define W_DEFL 8				/* Couleur Fenetre haute */
#define W_VOIE 9				/* Couleur voie */
#define W_NCOL	10				/* Nombre de couleurs definies */

typedef struct
{
	char text[82];
	int pos_bis;
	long color;
	long color_bis;
}
Line;

typedef struct
{
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
}
WinInfo;


static WinInfo *cnsl[MAXFEN];
static XFontStruct *fontinfo = NULL;
static GC dgc = NULL;
static long ColorVal[W_NCOL];
static int sysop_on = FALSE;
static Widget CallDialog;

#define NB_HISTO 20
static int histo_pos;
static char history[NB_HISTO][82];

int SEND = 0;
int RECV = 2;
int INDIC = 3;
int UI = 4;
int CONS = 5;
int FOND_VOIE = 6;
int HEADER = 2;

void alloc_buffer (int numero, int nblig);

void resizeCB (Widget w, XtPointer data, XtPointer call)
{
	Arg args[20];
	Cardinal n;
	Dimension val;
	int pos;
	WinInfo *info = (WinInfo *) data;

	n = 0;
	XtSetArg (args[n], XmNheight, &val);
	n++;
	XtGetValues (info->drawing, args, n);

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
	XtSetArg (args[n], XmNpageIncrement, info->nblignes);
	n++;
	XtSetArg (args[n], XmNsliderSize, info->nblignes);
	n++;
	XtSetArg (args[n], XmNmaximum, info->totlignes);
	n++;
	XtSetArg (args[n], XmNvalue, pos);
	n++;
	XtSetValues (info->scroll, args, n);

	XClearArea (display, XtWindow (info->drawing), 0, 0, 0, 0, TRUE);
}

void refreshCB (Widget w, XtPointer data, XtPointer call)
{
	int height;
	int offset;
	int i;
	int pos;
	WinInfo *info = (WinInfo *) data;

	offset = fontinfo->ascent;
	height = fontinfo->ascent + fontinfo->descent;

	pos = info->premier - info->nblignes + 1;
	if (pos < 0)
		pos += info->totlignes;

	for (i = 0; i < info->nblignes; i++)
	{
		char *ptr = info->winbuf[pos].text;

		if (info->winbuf[pos].pos_bis == 0)
		{
			XSetForeground (display, dgc, info->winbuf[pos].color);
			XDrawImageString (display, XtWindow (w),
							  dgc, 5, offset + i * height,
							  ptr, 80);
		}
		else
		{
			int largeur;

			/* 1ere couleur */
			XSetForeground (display, dgc,
							info->winbuf[pos].color);
			XDrawImageString (display, XtWindow (w),
							  dgc, 5, offset + i * height,
							  ptr, info->winbuf[pos].pos_bis);
			largeur = XTextWidth (fontinfo, ptr, info->winbuf[pos].pos_bis);

			/* 2eme couleur */
			ptr = info->winbuf[pos].text + info->winbuf[pos].pos_bis;
			XSetForeground (display, dgc,
							info->winbuf[pos].color_bis);
			XDrawImageString (display, XtWindow (w),
							  dgc, 5 + largeur,
							  offset + i * height,
							  ptr, 80 - info->winbuf[pos].pos_bis);
		}
		++pos;
		if (pos >= info->totlignes)
			pos = 0;
	}
}

void scrollCB (Widget w, XtPointer data, XtPointer call)
{
	Arg args[20];
	Cardinal n;
	int val;
	int offset;
	WinInfo *info = (WinInfo *) data;

	n = 0;
	XtSetArg (args[n], XmNvalue, &val);
	n++;
	XtGetValues (w, args, n);

	offset = info->totlignes - (val + info->nblignes);

	info->premier = info->curligne - offset;
	if (info->premier < 0)
		info->premier += info->totlignes;

	XClearArea (display, XtWindow (info->drawing), 0, 0, 1, 1, TRUE);
}

void quitCB (Widget w, XtPointer data, XtPointer call)
{
}

void scroll_window (WinInfo * info)
{
	Arg args[20];
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
			refreshCB (info->drawing, (XtPointer) info, NULL);
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
		XtSetArg (args[n], XmNvalue, pos);
		n++;
		XtSetValues (info->scroll, args, n);
	}
}

static void x_write (char *data, int len, int color, int numero)
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
	{
		XSetForeground (display, dgc, xcolor);
	}

	if ((pos) && (xcolor != info->winbuf[info->curligne].color))
	{
		info->winbuf[info->curligne].color_bis = xcolor;
		info->winbuf[info->curligne].pos_bis = pos;
	}
	else
		info->winbuf[info->curligne].color = xcolor;

	for (i = 0; i < len; i++)
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
			memset (ptr, 0x20, 80);
			info->winbuf[cpos].color = xcolor;
			info->winbuf[cpos].pos_bis = 0;

			reste = 0;
			scroll_window (info);

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
				memset (ptr, 0x20, 80);
				info->winbuf[cpos].color = xcolor;
				info->winbuf[cpos].pos_bis = 0;

				reste = 0;
				scroll_window (info);

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

		XDrawImageString (display, XtWindow (info->drawing),
						  dgc, 5,
						  offset + (info->nblignes - 1) * height,
						  cptr, 80);
	}
}

void window_write (int numero, char *data, int len, int color, int header)
{
	int i;

	for (i = 0; i < len; i++)
		if (data[i] == '\n')
			data[i] = '\r';
	x_write (data, len, color, numero);
}

long dos2pixel (int couleur)
{
	static int xcolor[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };;

	couleur &= 0xf;

	if (xcolor[couleur] == -1)
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
		
		cmap = DefaultColormap (display, DefaultScreen (display));
		if (XAllocColorCells (display, cmap, FALSE, &mask, 0, &pixel, 1))
		{
			color.pixel = pixel;
			color.red = r << 8;
			color.green = g << 8;
			color.blue = b << 8;
			color.flags = DoRed | DoGreen | DoBlue;
			XStoreColor (display, cmap, &color);
			xcolor[couleur] = pixel;
		}
		else
		{
			xcolor[couleur] =  (r << 16) + (g << 8) + b ;
		}
	}

	return (xcolor[couleur]);
}

void set_win_colors (void)
{
	ColorVal[W_SNDT] = dos2pixel (SEND);
	ColorVal[W_RCVT] = dos2pixel (RECV);
	ColorVal[W_CHNI] = dos2pixel (INDIC);
	ColorVal[W_MONH] = dos2pixel (HEADER);
	ColorVal[W_MOND] = dos2pixel (UI);
	ColorVal[W_CNST] = dos2pixel (CONS);
	ColorVal[W_BACK] = dos2pixel (FOND_VOIE);
	ColorVal[W_VOIE] = dos2pixel (INDIC);	/* Couleur voies */
}

void window_connect (int numero)
{
	char buf[80];
	WinInfo *info;

	if (cnsl[numero] == NULL)
		alloc_buffer (numero, 100);
	info = cnsl[numero];

	switch (numero)
	{
	case 0:
		strcpy (buf, "Console");
		break;
	case 1:
		strcpy (buf, "Monitoring");
		break;
	case 2:
		strcpy (buf, "All channels");
		break;
	default:
		buf[0] = '\0';
		break;
	}

	if (info->frame)
	{
		Arg args[1];

		XtSetArg (args[0], XmNtitle, buf);
		XtSetValues (info->frame, args, 1);
	}
}

void window_disconnect (int numero)
{
	WinInfo *info = cnsl[numero];

	if (info == NULL)
		return;

	if (numero == CurrentSelection)
		CurrentSelection = -1;
}

void window_close (int numero)
{
	switch (numero)
	{
	case 0:
		ConsoleCB (NULL, NULL, NULL);
		break;
	case 1:
		MonitorCB (NULL, NULL, NULL);
		break;
	case 2:
		AllChanCB (NULL, NULL, NULL);
		break;
	}
}

void window_init (void)
{
	int i;

	for (i = 0; i < MAXFEN; i++)
	{
		cnsl[i] = NULL;
	}

	display = XtDisplay (toplevel);
}

int get_win_lig (int numero)
{
	WinInfo *info = cnsl[numero];

	if (info)
		return (info->nblignes);
	return (25);
}

void free_buffer (int numero)
{
	if (cnsl[numero])
	{
		free (cnsl[numero]->winbuf);
		free (cnsl[numero]);
		cnsl[numero] = NULL;
	}
}

void alloc_buffer (int numero, int nblig)
{
	int i;

	if (cnsl[numero] == NULL)
	{
		WinInfo *info = (WinInfo *) calloc (sizeof (WinInfo), 1);

		if (info == NULL)
		{
			printf ("cannot allocate buffer for channel %d\n", numero);
			exit (2);
		}

		info->winbuf = (Line *) calloc (sizeof (Line), nblig);
		info->totlignes = nblig;
		info->nblignes = 25;

		for (i = 0; i < nblig; i++)
		{
			info->winbuf[i].text[80] = '\0';
			memset (info->winbuf[i].text, 0x20, 80);
			info->winbuf[i].color = 0;
			info->winbuf[i].color_bis = 0;
			info->winbuf[i].pos_bis = 0;
		}

		cnsl[numero] = info;
	}
}

static void CallOkCB (Widget w, XtPointer client_data, XtPointer call_data)
{
}

static void CallCancelCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	sysop_end ();
}

void sysop_end (void)
{
	if (sysop_on)
	{
		XtUnmanageChild (CallDialog);
		XtDestroyWidget (CallDialog);
		sysop_on = FALSE;
	}
}

void sysop_call (char *texte)
{
	Arg args[20];
	Cardinal n;
	XmString string;

	if (!sysop_on)
	{
		sysop_on = TRUE;

		string = XmStringCreateSimple (texte);

		/* Cree la fenetre de l'appel */
		n = 0;
		XtSetArg (args[n], XmNautoUnmanage, FALSE);
		n++;
		XtSetArg (args[n], XmNtitle, "Sysop call");
		n++;
		XtSetArg (args[n], XmNmessageString, string);
		n++;
		CallDialog = XmCreateMessageDialog (toplevel, "call_dialog", args, n);
		XtAddCallback (CallDialog, XmNokCallback, CallOkCB, NULL);
		XtAddCallback (CallDialog, XmNcancelCallback, CallCancelCB, NULL);
		XtUnmanageChild (XmMessageBoxGetChild (CallDialog, XmDIALOG_HELP_BUTTON));
		XtManageChild (CallDialog);

		XmStringFree (string);
	}
}

static void deleteCB (Widget w, XtPointer data, XtPointer call)
{
	int numero = (uintptr_t) data;

	window_close (numero);
}

static void editCB (Widget w, XtPointer data, XtPointer call)
{
	char *ptr;
	char buffer[82];
	WinInfo *info = (WinInfo *) data;

	ptr = XmTextFieldGetString (info->line2);
	XmTextFieldSetString (info->line3, ptr);
	ptr = XmTextFieldGetString (info->line1);
	XmTextFieldSetString (info->line2, ptr);
	ptr = XmTextFieldGetString (info->edit);
	XmTextFieldSetString (info->line1, ptr);
	XmTextFieldSetString (info->edit, "");

	strcpy (buffer, ptr);
	if (++histo_pos == NB_HISTO)
		histo_pos = 0;
	strcpy (history[histo_pos], buffer);

	strcat (buffer, "\r");
	window_write (CONSOLE, buffer, strlen (buffer), W_CNST, 0);
	console_input (buffer, strlen (buffer));
}

static void KeyEV (Widget w, XtPointer data, XKeyEvent * event)
{
	int i;
	int lg;
	int code;
	char *ptr;
	XmTextPosition pos;
	WinInfo *info = (WinInfo *) data;

	code = event->keycode;
	if (event->state & 1)
		code |= 0x100;			/* shift */
	if (event->state & 4)
		code |= 0x400;			/* control */
	if (event->state & 8)
		code |= 0x800;			/* alt */

	switch (event->keycode)
	{
	case 0x009:				/* escape */
		ptr = XmTextFieldGetString (info->edit);
		if (strlen (ptr) == 0)
		{
			/* commande Esc */
			/* console_inbuf("\033\r", 2); */
		}
		else
		{
			/* Efface la commande en cours */
			XmTextFieldSetString (info->edit, "");
		}
		XtFree (ptr);
		break;
	case 0x062:				/* UP */
	case 0x162:				/* Shift-UP */
		/* ligne precedente */
		for (i = 0; i < NB_HISTO; i++)
		{
			char *ptr = history[histo_pos];

			if (histo_pos == 0)
				histo_pos = NB_HISTO;
			--histo_pos;
			if (*ptr)
			{
				XmTextFieldSetString (info->edit, ptr);
				XmTextFieldSetInsertionPosition (info->edit, 80);
				break;
			}
		}
		break;
	case 0x068:				/* DW */
	case 0x168:				/* Shift-DW */
		/* ligne suivante */
		for (i = 0; i < NB_HISTO; i++)
		{
			if (++histo_pos == NB_HISTO)
				histo_pos = 0;
			if (*history[histo_pos])
			{
				XmTextFieldSetString (info->edit, history[histo_pos]);
				XmTextFieldSetInsertionPosition (info->edit, 80);
				break;
			}
		}
		break;
	case 0x462:				/* Ctrl-UP */
		/* Scroll UP */
		break;
	case 0x468:				/* Ctrl-DW */
		/* scroll DW */
		break;
	case 0x063:				/* PUP */
	case 0x163:				/* Shift-PUP */
		break;
	case 0x069:				/* PDW */
	case 0x169:				/* Shift-PDW */
		break;
	case 0x463:				/* Ctrl-PUP */
		break;
	case 0x469:				/* Ctrl-PDW */
		break;
	}

	/* Teste la fin de ligne */
	pos = XmTextFieldGetInsertionPosition (info->edit);
	lg = XmTextFieldGetMaxLength (info->edit);
	if (pos == lg)
	{
		char *scan;

		ptr = XmTextFieldGetString (info->edit);
		scan = strrchr (ptr, ' ');
		if (scan)
		{
			*scan++ = '\0';
		}
		XmTextFieldSetString (info->edit, ptr);
		editCB (w, info, NULL);

		if ((scan) && (*scan))
		{
			XmTextFieldSetString (info->edit, scan);
			XmTextFieldSetInsertionPosition (info->edit, 80);
		}

		XtFree (ptr);
	}
}

WinInfo *xcnsl (int numero, Widget pere, int nblig, Arg * argp, Cardinal np)
{
	Arg args[20];
	Cardinal n;
	Pixel foreground;
	Pixel background;
	Widget form;
	WinInfo *info;
	int mask;
	XGCValues values;
	Dimension hg;
	Widget separe = NULL;
	int console = (numero == CONSOLE);

	if (cnsl[numero] == NULL)
		alloc_buffer (numero, nblig);

	info = cnsl[numero];

	form = XmCreateForm (pere, "form", argp, np);

	if (display == NULL)
		display = XtDisplay (form);
	if (fontinfo == NULL)
		fontinfo = XLoadQueryFont (display, "6x10");

	if (console)
	{

		XmFontList fontlist;

		memset (history, 0, NB_HISTO * 82);

		fontlist = XmFontListCreate (fontinfo, XmSTRING_DEFAULT_CHARSET);

		n = 0;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNmarginHeight, 0);
		n++;
		XtSetArg (args[n], XmNmaxLength, 78);
		n++;
		XtSetArg (args[n], XmNshadowThickness, 0);
		n++;
		XtSetArg (args[n], XmNhighlightThickness, 0);
		n++;
		XtSetArg (args[n], XmNfontList, fontlist);
		n++;
		info->edit = XmCreateTextField (form, "edit", args, n);
		XtManageChild (info->edit);

		n = 0;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
		n++;
		XtSetArg (args[n], XmNbottomWidget, info->edit);
		n++;
		XtSetArg (args[n], XmNeditable, FALSE);
		n++;
		XtSetArg (args[n], XmNmarginHeight, 0);
		n++;
		XtSetArg (args[n], XmNshadowThickness, 0);
		n++;
		XtSetArg (args[n], XmNhighlightThickness, 0);
		n++;
		XtSetArg (args[n], XmNcursorPositionVisible, FALSE);
		n++;
		XtSetArg (args[n], XmNfontList, fontlist);
		n++;
		info->line1 = XmCreateTextField (form, "line1", args, n);
		XtManageChild (info->line1);
		XtSetKeyboardFocus (info->line1, info->edit);

		n = 0;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
		n++;
		XtSetArg (args[n], XmNbottomWidget, info->line1);
		n++;
		XtSetArg (args[n], XmNeditable, FALSE);
		n++;
		XtSetArg (args[n], XmNmarginHeight, 0);
		n++;
		XtSetArg (args[n], XmNshadowThickness, 0);
		n++;
		XtSetArg (args[n], XmNhighlightThickness, 0);
		n++;
		XtSetArg (args[n], XmNcursorPositionVisible, FALSE);
		n++;
		XtSetArg (args[n], XmNfontList, fontlist);
		n++;
		info->line2 = XmCreateTextField (form, "line2", args, n);
		XtManageChild (info->line2);
		XtSetKeyboardFocus (info->line2, info->edit);

		n = 0;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
		n++;
		XtSetArg (args[n], XmNbottomWidget, info->line2);
		n++;
		XtSetArg (args[n], XmNeditable, FALSE);
		n++;
		XtSetArg (args[n], XmNmarginHeight, 0);
		n++;
		XtSetArg (args[n], XmNshadowThickness, 0);
		n++;
		XtSetArg (args[n], XmNhighlightThickness, 0);
		n++;
		XtSetArg (args[n], XmNcursorPositionVisible, FALSE);
		n++;
		XtSetArg (args[n], XmNfontList, fontlist);
		n++;
		info->line3 = XmCreateTextField (form, "line3", args, n);
		XtManageChild (info->line3);
		XtSetKeyboardFocus (info->line3, info->edit);

		n = 0;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
		n++;
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
		n++;
		XtSetArg (args[n], XmNbottomWidget, info->line3);
		n++;
		separe = XmCreateSeparator (form, "separator", args, n);
		XtManageChild (separe);

		XtAddCallback (info->edit, XmNactivateCallback, editCB, (XtPointer) info);
		XtAddEventHandler (info->edit, KeyPressMask, FALSE, (XtEventHandler) KeyEV, (XtPointer) info);
		XmFontListFree (fontlist);

		XtSetKeyboardFocus (form, info->edit);
	}

	n = 0;
	if (console)
	{
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
		n++;
		XtSetArg (args[n], XmNbottomWidget, separe);
		n++;
	}
	else
	{
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);
		n++;
	}
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNmaximum, nblig);
	n++;
	info->scroll = XmCreateScrollBar (form, "scroll", args, n);
	XtManageChild (info->scroll);

	n = 0;
	if (console)
	{
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET);
		n++;
		XtSetArg (args[n], XmNbottomWidget, separe);
		n++;
	}
	else
	{
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);
		n++;
	}
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);
	n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET);
	n++;
	XtSetArg (args[n], XmNrightWidget, info->scroll);
	n++;
	XtSetArg (args[n], XmNwidth, 500);
	n++;
	XtSetArg (args[n], XmNheight, 200);
	n++;
	info->drawing = XmCreateDrawingArea (form, "drawing", args, n);

	XtManageChild (info->drawing);
	XtManageChild (form);

	XtSetKeyboardFocus (info->drawing, info->edit);

	n = 0;
	XtSetArg (args[n], XmNforeground, &foreground);
	n++;
	XtSetArg (args[n], XmNbackground, &background);
	n++;
	XtSetArg (args[n], XmNheight, &hg);
	n++;
	XtGetValues (info->drawing, args, n);

	/* info->premier = 0; */

	mask = GCForeground | GCBackground | GCFont;
	values.background = background;
	values.foreground = foreground;
	values.font = fontinfo->fid;

	if (dgc == NULL)
		dgc = XtGetGC (info->drawing, mask, &values);

	XtAddCallback (info->scroll, XmNvalueChangedCallback, scrollCB, info);
	XtAddCallback (info->scroll, XmNdragCallback, scrollCB, info);
	XtAddCallback (info->drawing, XmNexposeCallback, refreshCB, info);
	XtAddCallback (info->drawing, XmNresizeCallback, resizeCB, info);

	info->nblignes = hg / (fontinfo->ascent + fontinfo->descent);

	n = 0;
	XtSetArg (args[n], XmNpageIncrement, info->nblignes);
	n++;
	XtSetArg (args[n], XmNsliderSize, info->nblignes);
	n++;
	XtSetArg (args[n], XmNmaximum, info->totlignes);
	n++;
	XtSetArg (args[n], XmNvalue, info->totlignes - info->nblignes);
	n++;
	XtSetValues (info->scroll, args, n);

	return (info);
}

void ShowFbbWindow (int numero, Widget parent)
{
	WinInfo *info;

	if (cnsl[numero] == NULL)
		alloc_buffer (numero, 100);
	info = cnsl[numero];

	if (info->frame)
	{
		if (XtIsManaged (info->frame))
		{
			XRaiseWindow (XtDisplay (info->frame), XtWindow (info->frame));
		}
		else
			XtManageChild (info->frame);
	}
	else
	{
		Arg args[20];
		Cardinal n;
		Atom DelWindow;
		char buf[80];

		if (numero == CONSOLE)
		{
			sprintf (buf, "Console");
		}
		else if (numero == MONITOR)
		{
			sprintf (buf, "Monitoring");
		}
		else if (numero == ALLCHANN)
		{
			sprintf (buf, "All channels");
		}

		n = 0;
		XtSetArg (args[n], XmNdeleteResponse, XmDO_NOTHING);
		++n;
		XtSetArg (args[n], XmNtitle, buf);
		++n;
		info->frame = XmCreateDialogShell (parent, "cnsl", args, n);
		xcnsl (numero, info->frame, 100, NULL, 0);
		XtManageChild (info->frame);

		DelWindow = XInternAtom (display, "WM_DELETE_WINDOW", FALSE);
		XmAddWMProtocolCallback (info->frame, DelWindow, deleteCB, (XtPointer)(intptr_t) numero);
	}
	fenetre[numero] = 1;
}

void HideFbbWindow (int numero, Widget parent)
{
	if ((cnsl[numero]) && (cnsl[numero]->frame) &&
		(XtIsManaged (cnsl[numero]->frame)))
	{
		XtUnmanageChild (cnsl[numero]->frame);
	}
	fenetre[numero] = 0;
}

void ToggleFbbWindow (int numero, Widget parent)
{
	if ((cnsl[numero]) && (cnsl[numero]->frame) &&
		(XtIsManaged (cnsl[numero]->frame)))
	{
		HideFbbWindow (numero, parent);
	}
	else
	{
		ShowFbbWindow (numero, parent);
	}
}
