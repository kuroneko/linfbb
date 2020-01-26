# We need to install some of these binaries in custom locations
tooldir = $(pkglibdir)/tool

serverdir = $(pkglibdir)/server

tool_PROGRAMS = clr_user epurmess epurwp maintinf

server_PROGRAMS = reqdir

# The rest of the binaries go in sbin
sbin_PROGRAMS = ajoursat satdoc satupdat fbbgetconf xfbbd xfbbC

ajoursat_SOURCES = ajoursat.c

clr_user_SOURCES = clr_user.c fbb_conf.c

satdoc_SOURCES = satdoc.c

satupdat_SOURCES = satupdat.c

reqdir_SOURCES = reqdir.c fbb_conf.c

epurwp_SOURCES = epurwp.c fbb_conf.c

epurmess_SOURCES = epurmess.c fbb_conf.c

fbbgetconf_SOURCES = fbbgetconf.c fbb_conf.c

maintinf_SOURCES = maintinf.c fbb_conf.c

# Build a convenience library, since these are all used by a couple different
# programs
noinst_LIBRARIES = libxfbbd.a

libxfbbd_a_SOURCES = arbre.c autobin.c balise.c bidexms.c conf.c\
                console.c date.c devio.c dos_1.c dos_dir.c\
		driver.c drv_aea.c drv_ded.c drv_hst.c drv_kam.c\
		drv_mod.c drv_pop.c drv_sock.c drv_tcp.c edit.c\
		ems.c error.c exec_pg.c fbb_conf.c fbb_orb.c\
                fortify.c forward.c fwdovl1.c fwdovl2.c fwdovl3.c\
		fwdovl4.c fwdovl5.c fwdovl6.c fwdovl7.c fwdutil.c\
		gesfic.c ibm.c info.c init.c init_srv.c\
		init_tnc.c initfwd.c initport.c k_tasks.c kernel.c\
		lzhuf.c maint_fw.c mbl_edit.c mbl_expo.c mbl_impo.c\
		mbl_kill.c mbl_lc.c mbl_list.c mbl_log.c mbl_menu.c\
		mbl_opt.c mbl_prn.c mbl_read.c mbl_rev.c mbl_stat.c\
		mbl_sys.c mbl_user.c mblutil.c md5c.c modem.c\
		nomenc.c nouvfwd.c pac_crc.c pacsat.c qraloc.c\
                redist.c rx25.c serv.c serveur.c statis.c themes.c\
		tnc.c tncio.c trait.c trajec.c trajovl.c variable.c\
                warning.c watchdog.c waveplay.c wp.c wp_mess.c\
                wpserv.c xfwd.c xmodem.c yapp.c

# We want to build fbb_conf.c on the fly to substitute in default paths
nodist_libxfbbd_a_SOURCES = fbb_conf.c

BUILT_SOURCES = fbb_conf.c

CLEANFILES = fbb_conf.c

sourcefiles = fbb_conf.c

edit = sed \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@docdir[@]|$(docdir)|g' \
	-e 's|@libdir[@]|$(libdir)|g'

fbb_conf.c: Makefile
	@list='$(sourcefiles)'; for p in $$list; do \
	$(edit) $(srcdir)/$$p.in >$$p; \
	done

# Need to include fbb_conf.c.in manually, as well as aff_stat.c
EXTRA_DIST = fbb_conf.c.in aff_stat.c

xfbbd_SOURCES = xfbbd.c

xfbbd_CFLAGS = -D__ORB__ $(AM_CFLAGS)

xfbbd_LDADD = libxfbbd.a $(AX25_LIB)

if HAVE_NCURSES
xfbbC_SOURCES = md5c.c terminal.c xfbbC.c
xfbbC_LDADD = @CURSES_LIB@
else
xfbbC_SOURCES = md5c.c xfbbC.c
endif

#AM_CPPFLAGS = -I$(top_srcdir)/include 

AM_CPPFLAGS = -I$(top_srcdir)/include -Wstrict-prototypes -funsigned-char -DPROTOTYPES -fstack-check

# Need to build stuff in this folder first, before X11, as there are deps in here
# Check and see if we are going to build the X binaries at all first
if HAVE_X_LIBS
EXTRADIRS = X11
endif
SUBDIRS = . $(EXTRADIRS)
