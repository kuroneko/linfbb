# this is only if "old" ax25 for kernel 2.0.x are used
AX25OLDPATH = /usr/src/ax25

# the rest should not be changed
AX25LIBPATH = /usr/local/lib
#AX25INCPATH = /usr/src/linux/include/netax25
AX25INCPATH = /usr/local/include/netax25

.EXPORT_ALL_VARIABLES:

USE_NCURSES := $(shell test -f /usr/include/ncurses.h && echo 1)
OLD_AX25    := $(shell test -f $(AX25OLDPATH)/lib/axutils.h && echo 1)
NEW_AX25    := $(shell test -f $(AX25INCPATH)/axlib.h && echo 1)

ifneq ($(NEW_AX25), 1)
  ifeq ($(OLD_AX25), 1)
    DEFS = -DOLD_AX25
    AX25INCPATH = $(AX25OLDPATH)/lib
  endif
endif

CC     = gcc
CFLAGS = -g -O2 -Wstrict-prototypes -funsigned-char $(DEFS) -D__LINUX__ -DPROTOTYPES -I../include -fstack-check
#CFLAGS = -Wall -Wstrict-prototypes -ggdb2 -funsigned-char $(DEFS) -D__LINUX__ -Wformat-security -D_FORTIFY_SOURCE=2 -DPROTOTYPES -I../include -fstack-check -fstack-protector-all
LIBS   = -lm -lax25

COMMON_OBJECTS = \
   arbre.o\
   autobin.o\
   balise.o\
   bidexms.o\
   conf.o\
   console.o\
   date.o\
   devio.o\
   dos_1.o\
   dos_dir.o\
   driver.o\
   drv_aea.o\
   drv_ded.o\
   drv_hst.o\
   drv_kam.o\
   drv_mod.o\
   drv_pop.o\
   drv_sock.o\
   drv_tcp.o\
   edit.o\
   ems.o\
   error.o\
   exec_pg.o\
   fbb_conf.o\
   fortify.o\
   forward.o\
   fwdovl1.o\
   fwdovl2.o\
   fwdovl3.o\
   fwdovl4.o\
   fwdovl5.o\
   fwdovl6.o\
   fwdovl7.o\
   fwdutil.o\
   gesfic.o\
   ibm.o\
   info.o\
   init.o\
   init_srv.o\
   init_tnc.o\
   initfwd.o\
   initport.o\
   k_tasks.o\
   kernel.o\
   lzhuf.o\
   maint_fw.o\
   mbl_edit.o\
   mbl_expo.o\
   mbl_impo.o\
   mbl_kill.o\
   mbl_lc.o\
   mbl_list.o\
   mbl_log.o\
   mbl_menu.o\
   mbl_opt.o\
   mbl_prn.o\
   mbl_read.o\
   mbl_rev.o\
   mbl_stat.o\
   mbl_sys.o\
   mbl_user.o\
   mblutil.o\
   md5c.o\
   modem.o\
   nomenc.o\
   nouvfwd.o\
   pac_crc.o\
   pacsat.o\
   qraloc.o\
   redist.o\
   rx25.o\
   serv.o\
   serveur.o\
   statis.o\
   themes.o\
   tnc.o\
   tncio.o\
   trait.o\
   trajec.o\
   trajovl.o\
   variable.o\
   warning.o\
   watchdog.o\
   waveplay.o\
   wp.o\
   wp_mess.o\
   wpserv.o\
   xfwd.o\
   xmodem.o\
   yapp.o

all : xfbbd fbbgetconf epurmess epurwp reqdir clr_user maintinf xfbbC satupdat ajoursat satdoc

install: all
	@cd ..;./install_sh

clean :
	$(RM) *.o *.orig ../include/*.orig core
	$(RM) xfbbd xfbbC epurmess epurwp reqdir clr_user maintinf fbbgetconf
	$(RM) satdoc satupdat ajoursat
	$(RM) X11/*.o X11/xfbb X11/*.orig X11/core X11/xfbbX_cl

xfbbd: $(COMMON_OBJECTS) xfbbd.o fbb_orb.o
	$(CC) $(CFLAGS) -o xfbbd xfbbd.o fbb_orb.o $(COMMON_OBJECTS) $(LIBS)

xfbbd.o : xfbbd.c
	$(CC) $(CFLAGS) -D__ORB__ -c -o xfbbd.o xfbbd.c

fbbgetconf : fbbgetconf.o fbb_conf.o
	$(CC) $(CFLAGS) -o fbbgetconf fbbgetconf.o fbb_conf.o

epurmess : epurmess.o fbb_conf.o
	$(CC) $(CFLAGS) -o epurmess epurmess.o fbb_conf.o

epurwp : epurwp.o fbb_conf.o
	$(CC) $(CFLAGS) -o epurwp epurwp.o fbb_conf.o

clr_user : clr_user.o fbb_conf.o
	$(CC) $(CFLAGS) -o clr_user clr_user.o fbb_conf.o

maintinf : maintinf.o fbb_conf.o
	$(CC) $(CFLAGS) -o maintinf maintinf.o fbb_conf.o

reqdir : reqdir.o fbb_conf.o
	$(CC) $(CFLAGS) -o reqdir reqdir.o fbb_conf.o

satupdat : satupdat.c
	$(CC) $(CFLAGS) satupdat.c -o satupdat

ajoursat : ajoursat.c
	$(CC) $(CFLAGS) ajoursat.c -o ajoursat

satdoc : satdoc.c
	$(CC) $(CFLAGS) -o satdoc satdoc.c

ifeq ($(USE_NCURSES),1)
CFLAGS += -DUSE_NCURSES
xfbbC : xfbbC.o md5c.o terminal.o
	$(CC) $(CFLAGS) -o xfbbC xfbbC.o md5c.o terminal.o -lncurses
else
xfbbC : xfbbC.o md5c.o
	@echo "***"
	@echo "*** ncurses is not installed. xfbbC client will use terminal mode"
	@echo "***"
	$(CC) $(CFLAGS) -o xfbbC xfbbC.o md5c.o
endif
