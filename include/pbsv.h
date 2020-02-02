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

/* pbsv.h 1993.8.6 */

#define DEBUG 1

#define PBSV_VER "0.04"
#define PBSV_ID  "[930806]"

typedef int VOID;
typedef int BOOL;
typedef int TINY;

#define OK  1
#define NG  0

#define ON  1
#define OFF 0

#define NOT_DEFINE (-1)

#define MAXUSER	    20
#define MAXHOLE	    100
#define MAXPFHDIR   16384

#define MAXBLKSIZE  244

#define ADRSIZE  7	    /* adrs length	*/
#define CALLSIZE 10 
#define HDRSIZE  (1+7+7+1+1)
#define FRMSIZE	    2048

#define F_DIR	0x0001

extern unsigned short	calc_crc(unsigned char c, unsigned short crc);

struct stqcell {
    struct stqcell *next;
};

struct stqueue {
    struct stqcell *head;
    struct stqcell *tail;
};

struct sthole {
    struct sthole *next;
    long offset;
    ushort length;
    time_t start;
    time_t end;
};

struct stuser {
    struct stuser *next;
    time_t entry_t;
    ushort flags;
    char call[CALLSIZE];
    int  file_type;
    long file_id;
    ushort block_size;
    long file_size;
    struct stqueue hole;
};

struct stpfhdir {
    time_t t_old;
    time_t t_new;
    long file_id;
    int pfh_type;
    /* int pfh_size; */
};

struct stpfh {
    u_long file_number;
    char   file_name[8];
    char   file_ext[3];
    u_long file_size;
    u_long create_time;
    u_long last_modied_time;
    u_char seu_flag;
    u_char file_type;
    ushort  body_checksum;
    ushort  header_cecksum;
    ushort  body_offset;

    ushort  pfh_size;
};

/* pbsv.h */
