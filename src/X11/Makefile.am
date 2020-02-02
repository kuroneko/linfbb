sbin_PROGRAMS = xfbb xfbbX_cl

xfbb_SOURCES = xfbbabtd.c xfbbcnsl.c xfbbedtm.c xfbbedtu.c xfbblcnx.c\
                xfbbmain.c xfbbpndd.c xeditor.c xfbb.c

xfbb_LDADD = ../libxfbbd.a -lXm -lXt -lXpm -lXext -lX11 $(AX25_LIB)

xfbbX_cl_SOURCES =  ../md5c.c xfbbXabtd.c xfbbXcnsl.c xfbbX.c

xfbbX_cl_LDADD = -lXm -lXt -lXpm -lXext -lX11

AM_CPPFLAGS = -I$(top_srcdir)/include -Wstrict-prototypes -funsigned-char -DPROTOTYPES -fstack-check 
