# Define all the paths we want to use to install the files in
FBB_SYSCONFDIR=$(sysconfdir)/ax25/fbb

etcfilesdir = $(FBB_SYSCONFDIR)

sysfilesdir = $(FBB_SYSCONFDIR)

langfilesdir = $(FBB_SYSCONFDIR)/lang

fwdfilesdir = $(FBB_SYSCONFDIR)/fwd

# Gather all our file names and sort them by target
# etcfiles and sysfile are going to be manually installed 
# by make installconf
etcfiles = \
	fbbopt.conf\
	port.sys.sample\
	protect.sys

sysfiles = \
	bbs.sys\
	cron.sys\
	forward.sys\
	langue.sys\
	passwd.sys\
	redist.sys\
	reject.sys\
	swapp.sys\
	themes.sys

# langfiles and fwdfiles will be installed by make install
langfiles = \
	lang/english.ent.sample\
	lang/english.hlp\
	lang/english.inf\
	lang/english.txt\
	lang/francais.ent.sample\
	lang/francais.hlp\
	lang/francais.inf\
	lang/francais.txt

fwdfiles = amsat.fwd

# This is a trigger for automake to run the do_subst routine
# which will run sed to update the paths in the input files
noinst_SCRIPTS = do_subst amsat.fwd

# This subroutine uses sed to do find and replace in a file 
# to dynamically update the paths based on how configure was called
edit = sed \
	-e 's|@sbindir[@]|$(sbindir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@prefix[@]|$(prefix)|g' \
	-e 's|@libdir[@]|$(libdir)|g' \
	-e 's|@docdir[@]|$(docdir)|g' \
	-e 's|@package_string[@]|$(PACKAGE_NAME)$(PACKAGE_VERSION)|g'

# Normally (like with amsat.fwd), we would use per-file make targets 
# to do the substitution in the files. For etcfiles, we use the automake 
# noinst_SCRIPTS target to run this subroutine and process them all
do_subst: Makefile
	@list='$(etcfiles)'; for p in $$list; do \
	$(edit) $(srcdir)/$$p.in >$$p; \
	done

amsat.fwd: amsat.fwd.in Makefile
	$(edit) $(srcdir)/$@.in > $@

# We only want to install the etcfiles and sysfiles if the user 
# specifically runs make installconf, as they will clobber 
# any existing files on a working system. This target still lets 
# us build and isntall them for new installs and packaging.
installconf:
	$(mkinstalldirs) $(DESTDIR)$(etcfilesdir)
	@list='$(etcfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(etcfilesdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(etcfilesdir)/$$p; \
	done
	$(mkinstalldirs) $(DESTDIR)$(sysfilesdir)
	@list='$(sysfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(sysfilesdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(sysfilesdir)/$$p; \
	done
	$(mkinstalldirs) $(DESTDIR)$(langfilesdir)
	@list='$(langfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(etcfilesdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(etcfilesdir)/$$p; \
	done
	$(mkinstalldirs) $(DESTDIR)$(fwdfilesdir)
	@list='$(fwdfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(fwdfilesdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(fwdfilesdir)/$$p; \
	done


# Remove any files processed with sed when we make clean
CLEANFILES = $(etcfiles) $(fwdfiles)

EXTRA_DIST = \
	$(sysfiles)\
	$(langfiles)\
	fbbopt.conf.in\
	port.sys.sample.in\
	protect.sys.in\
	amsat.fwd.in

