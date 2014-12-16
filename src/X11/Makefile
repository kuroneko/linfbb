
AX25LIBPATH = /usr/local/lib
#AX25INCPATH = /usr/include/netax25
AX25INCPATH = /usr/local/include/netax25

XINCLUDE = -I/usr/X11R6/LessTif/Motif2.0/include -I/usr/X11R6/include 
OLD_AX25 := $(shell test -f /usr/include/ax25/axutils.h && echo 1)
ifeq ($(OLD_AX25), 1)
DEFS = -DOLD_AX25 
endif

CC = gcc
CFLAGS = -Wall -Wstrict-prototypes -O2 -g -funsigned-char $(DEFS) -D__LINUX__ -DPROTOTYPES -I../../include $(XINCLUDE) -I$(AX25INCPATH)
#CFLAGS = -Wall -Wstrict-prototypes -O2 -ggdb -funsigned-char $(DEFS) -D__LINUX__ -DPROTOTYPES -I../../include $(XINCLUDE) -I$(AX25INCPATH)
LIBS   = -L$(AX25LIBPATH) -lm -lax25
XLIBS  = -L/usr/X11R6/lib -L/usr/X11R6/LessTif/Motif2.0/lib -lXm -lXt -lXpm -lXext -lX11 

COMMON_OBJECTS = \
   ../arbre.o\
   ../autobin.o\
   ../balise.o\
   ../bidexms.o\
   ../conf.o\
   ../console.o\
   ../date.o\
   ../devio.o\
   ../dos_1.o\
   ../dos_dir.o\
   ../driver.o\
   ../drv_aea.o\
   ../drv_ded.o\
   ../drv_hst.o\
   ../drv_kam.o\
   ../drv_mod.o\
   ../drv_pop.o\
   ../drv_sock.o\
   ../drv_tcp.o\
   ../edit.o\
   ../ems.o\
   ../error.o\
   ../exec_pg.o\
   ../fbb_conf.o\
   ../fortify.o\
   ../forward.o\
   ../fwdovl1.o\
   ../fwdovl2.o\
   ../fwdovl3.o\
   ../fwdovl4.o\
   ../fwdovl5.o\
   ../fwdovl6.o\
   ../fwdovl7.o\
   ../fwdutil.o\
   ../gesfic.o\
   ../ibm.o\
   ../info.o\
   ../init.o\
   ../init_srv.o\
   ../init_tnc.o\
   ../initfwd.o\
   ../initport.o\
   ../k_tasks.o\
   ../kernel.o\
   ../lzhuf.o\
   ../maint_fw.o\
   ../mbl_edit.o\
   ../mbl_expo.o\
   ../mbl_impo.o\
   ../mbl_kill.o\
   ../mbl_lc.o\
   ../mbl_list.o\
   ../mbl_log.o\
   ../mbl_menu.o\
   ../mbl_opt.o\
   ../mbl_prn.o\
   ../mbl_read.o\
   ../mbl_rev.o\
   ../mbl_stat.o\
   ../mbl_sys.o\
   ../mbl_user.o\
   ../mblutil.o\
   ../md5c.o\
   ../modem.o\
   ../nomenc.o\
   ../nouvfwd.o\
   ../pac_crc.o\
   ../pacsat.o\
   ../qraloc.o\
   ../redist.o\
   ../rx25.o\
   ../serv.o\
   ../serveur.o\
   ../statis.o\
   ../themes.o\
   ../tnc.o\
   ../tncio.o\
   ../trait.o\
   ../trajec.o\
   ../trajovl.o\
   ../variable.o\
   ../warning.o\
   ../watchdog.o\
   ../waveplay.o\
   ../wp.o\
   ../wp_mess.o\
   ../wpserv.o\
   ../xfwd.o\
   ../xmodem.o\
   ../yapp.o

X_OBJETS = \
   xfbbabtd.o\
   xfbbcnsl.o\
   xfbbedtm.o\
   xfbbedtu.o\
   xfbblcnx.o\
   xfbbmain.o\
   xfbbpndd.o\
   xeditor.o

XC_OBJETS = \
   ../md5c.o \
   xfbbXabtd.o\
   xfbbXcnsl.o

all : xfbb xfbbX_cl

clean :
	$(RM) *.o *.orig core 
	$(RM) xfbbX_cl xfbb

xfbb: $(COMMON_OBJETS) $(X_OBJETS) xfbb.o
	$(CC) $(CFLAGS) -o xfbb xfbb.o $(COMMON_OBJECTS) $(X_OBJETS) $(LIBS) $(XLIBS)

xfbb.o : xfbb.c
	$(CC) $(CFLAGS) -c -o xfbb.o xfbb.c

xfbbX_cl: $(XC_OBJETS) xfbbX.o
	$(CC) $(CFLAGS) -o xfbbX_cl xfbbX.o $(XC_OBJETS) $(LIBS) $(XLIBS)

xfbbX.o : xfbbX.c
	$(CC) $(CFLAGS) -c -o xfbbX.o xfbbX.c
