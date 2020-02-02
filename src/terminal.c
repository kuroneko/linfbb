/************************************************************************
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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Parts of code have been taken from many other softwares.
    Thanks for the help.
************************************************************************/


 /**********************************************
 *                                             *
 * xfbbC : Client for xfbbd BBS daemon version *
 *                                             *
 **********************************************/
#include <sys/types.h>
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>

#define uchar char
#define	MAX_PACKETLEN	512
#define	MAX_BUFLEN	2*MAX_PACKETLEN

typedef struct {
    WINDOW *ptr;
    int max_y;
    int max_x;
    char string[MAX_BUFLEN];
    int bytes;
    int curs_pos;
} t_win;

t_win win_in;
t_win win_out;

struct wint_s
{
	WINDOW* ptr;
	int fline;
	int lline;
	struct wint_s* next;
};
typedef struct wint_s wint;

wint wintab;

#define TALKMODE	001	/* two windows (outgoing and incoming) with menu */
#define SLAVEMODE	002	/* Menu mode */
#define RAWMODE		004	/* mode used by earlier versions */

int mode = TALKMODE;

WINDOW* winopen(wint *wtab, int nlines, int ncols, int begin_y, int begin_x, int border)
{

	while (wtab->next != NULL)
		wtab = wtab->next;

	wtab->next = (wint *)malloc(sizeof(wint));
	wtab       = wtab->next;

	wtab->next = NULL;

	wtab->ptr  = newwin(nlines, ncols, begin_y, begin_x);

	if (wtab->ptr == NULL)
		return NULL;

 	wtab->fline = begin_y;
	wtab->lline = begin_y + nlines;

	if (border)
		wborder(wtab->ptr, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
			ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	
	return wtab->ptr;
}

int start_talk_mode(wint * wintab, t_win * win_in, t_win * win_out)
{
    int cnt;
    WINDOW *win;

    win_out->max_y = 4;		/* TXLINES */
    win_out->max_x = COLS;
    win_in->max_y = (LINES - 4) - win_out->max_y;
    win_in->max_x = COLS;

    win_out->ptr = winopen(wintab, win_out->max_y + 1, win_out->max_x, (win_in->max_y + 3), 0, FALSE);
    win_in->ptr = winopen(wintab, win_in->max_y + 1, win_in->max_x, 1, 0, FALSE);
    win = winopen(wintab, 1, win_out->max_x, win_in->max_y + 2, 0, FALSE);

    for (cnt = 0; cnt < COLS; cnt++)
	waddch(win, '-');
    wrefresh(win);

    scrollok(win_in->ptr, TRUE);
    scrollok(win_out->ptr, TRUE);

    wclear(win_out->ptr);
    wrefresh(win_out->ptr);
    wclear(win_in->ptr);
    wrefresh(win_in->ptr);

    win_out->bytes = 0;
    win_out->curs_pos = 0;
    win_in->bytes = 0;
    win_out->curs_pos = 0;

    return 0;
}

static int start_screen(char *call)
{
    int cnt;

    if (initscr() == NULL)
		return -1;

    attron(A_REVERSE);
    move(0, 0);
    addstr(call);

    for (cnt = strlen(call); cnt <= 80; cnt++)
		addch(' ');

    attroff(A_REVERSE);

    noecho();
    raw();
    refresh();

    return 0;
}

static void writeincom(int mode, t_win *win_in, char *buf, int bytes)
{
    int cnt;

	if (mode & RAWMODE) 
	{
		if ((write(STDOUT_FILENO, buf, bytes)) != bytes)
			fprintf (stderr, "Error writing to STDOUT file %d\n", STDOUT_FILENO);
		return;
    }

    for (cnt = 0; cnt < bytes; cnt++) 
	{
		switch (buf[cnt]) 
		{
		case 201:
		case 218:
	    	waddch(win_in->ptr, ACS_ULCORNER);
	    	break;
		case 187:
		case 191:
	    	waddch(win_in->ptr, ACS_URCORNER);
	    	break;
		case 200:
		case 192:
	    	waddch(win_in->ptr, ACS_LLCORNER);
	    	break;
		case 188:
		case 217:
	    	waddch(win_in->ptr, ACS_LRCORNER);
	    	break;
		case 204:
		case 195:
	    	waddch(win_in->ptr, ACS_LTEE);
	    	break;
		case 185:
		case 180:
	    	waddch(win_in->ptr, ACS_RTEE);
	    	break;
		case 203:
		case 194:
	    	waddch(win_in->ptr, ACS_TTEE);
	    	break;
		case 202:
		case 193:
	    	waddch(win_in->ptr, ACS_BTEE);
	    	break;
		case 205:
		case 196:
	    	waddch(win_in->ptr, ACS_HLINE);
	    	break;
		case 186:
		case 179:
	    	waddch(win_in->ptr, ACS_VLINE);
	    	break;
		case 129:
	    	waddch(win_in->ptr, 252);	/*u umlaut */
	    	break;
		case 132:
	    	waddch(win_in->ptr, 228);	/*a umlaut */
	    	break;
		case 142:
	    	waddch(win_in->ptr, 196);	/*A umlaut */
	    	break;
		case 148:
	    	waddch(win_in->ptr, 246);	/*o umlaut */
	    	break;
		case 153:
	    	waddch(win_in->ptr, 214);	/*O umlaut */
	    	break;
		case 154:
	    	waddch(win_in->ptr, 220);	/*U umlaut */
	    	break;
		case 225:
	    	waddch(win_in->ptr, 223);	/*sz */
	    	break;
		default:
	    	{
			if (buf[cnt] > 127)
		    	waddch(win_in->ptr, '.');
			else
		    	waddch(win_in->ptr, buf[cnt]);
	    	}
		}
    }

    wrefresh(win_in->ptr);

    return;
}

int readoutg(t_win *win_out, wint *wintab, char *buf)
{
    int out_cnt;
    int c;
    int ypos = 0, xpos = 0;

    c = getch();
    if (c == ERR)
		return 0;

    switch (c) {
    case KEY_BACKSPACE:
    case 127:
		{
	    	getyx(win_out->ptr, ypos, xpos);
	    	if (win_out->bytes > 0) 
			{
				if (win_out->curs_pos < win_out->bytes) 
				{
		    		mvwaddnstr(win_out->ptr, ypos, --xpos, &win_out->string[win_out->curs_pos], win_out->bytes - win_out->curs_pos);
		    		waddch(win_out->ptr, ' ');
		    		memmove(&win_out->string[win_out->curs_pos - 1], &win_out->string[win_out->curs_pos], win_out->bytes - win_out->curs_pos);
				} 
				else
		    		mvwaddch(win_out->ptr, ypos, --xpos, ' ');

				wmove(win_out->ptr, ypos, xpos);
				win_out->bytes--;
				win_out->curs_pos--;
	    	}
		}
		break;
    case KEY_LEFT:
		if (win_out->curs_pos > 0) 
		{
	    	win_out->curs_pos--;
	    	getyx(win_out->ptr, ypos, xpos);
	    	wmove(win_out->ptr, ypos, xpos - 1);
		}
	break;
    case KEY_RIGHT:
		if (win_out->curs_pos < win_out->bytes) 
		{
	    	win_out->curs_pos++;
	    	getyx(win_out->ptr, ypos, xpos);
	    	wmove(win_out->ptr, ypos, xpos + 1);
		}
		break;
    case KEY_ENTER:
    case (int) '\n':
    case (int) '\r':
		{
	    	if (win_out->curs_pos < win_out->bytes) {
			getyx(win_out->ptr, ypos, xpos);
			wmove(win_out->ptr, ypos, xpos + win_out->bytes - win_out->curs_pos);
	    	}
	    	waddch(win_out->ptr, '\n');
	    	win_out->string[win_out->bytes++] = (char) '\n';
	    	wrefresh(win_out->ptr);
	    	strncpy(buf, win_out->string, win_out->bytes);
	    	wrefresh(win_out->ptr);
	    	out_cnt = win_out->bytes;
	    	win_out->bytes = 0;
	    	win_out->curs_pos = 0;
	    	return out_cnt;
		}
		break;
    default:
		{
	    	waddch(win_out->ptr, (char) c);
	    	if (win_out->curs_pos < win_out->bytes) 
			{
				getyx(win_out->ptr, ypos, xpos);
				waddnstr(win_out->ptr, &win_out->string[win_out->curs_pos], win_out->bytes - win_out->curs_pos);
				memmove(&win_out->string[win_out->curs_pos + 1], &win_out->string[win_out->curs_pos], win_out->bytes - win_out->curs_pos);
				win_out->string[win_out->curs_pos] = (char) c;
				wmove(win_out->ptr, ypos, xpos);
	    	} 
			else
				win_out->string[win_out->bytes] = (char) c;

	    	win_out->bytes++;
	    	win_out->curs_pos++;
		}
    }
    wrefresh(win_out->ptr);
    return 0;
}

void init_terminal(int termmode, char *call)
{
	if (termmode)
	{
		mode = TALKMODE;
		start_screen(call);
		start_talk_mode(&wintab, &win_in, &win_out);
	}
	else
	{
		mode = RAWMODE;
	}
}

void end_terminal(void)
{
	endwin();
}

int read_terminal(char *buf, int len)
{
    int bytes;

	if ((mode & RAWMODE) == RAWMODE)
		bytes = read(STDIN_FILENO, buf, len);
	else 
	{
		bytes = readoutg(&win_out, &wintab, buf);
		if (bytes == -1) 
		{
			wclear(win_in.ptr);
			wrefresh(win_in.ptr);
			wclear(win_out.ptr);
			wrefresh(win_out.ptr);
			bytes = 0;
		}
		writeincom(mode, &win_in, buf, bytes);
	}
	
	return bytes;
}

void write_terminal(char *buf, int bytes)
{
	writeincom(mode, &win_in, buf, bytes);
}
