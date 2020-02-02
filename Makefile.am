installconf:
	(cd $(top_srcdir)/etc && $(MAKE) installconf)

SUBDIRS = man doc etc scripts include src 

# Build the folder tree for fbb and remind user to run make installconf
install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/filter
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/pg
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/server
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/tool
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script/error_return
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script/end_session
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script/rerun
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script/maintenance
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script/stop_system
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script/sig_segv
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)/script/reset
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/log
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/oldmail
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/sat
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/wp
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/fbbdos
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/fbbdos/yapp
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/fbbdos/readonly
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/mail
	for n in 0 1 2 3 4 5 6 7 8 9; do \
	  $(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/mail/mail$$n; \
	done
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/binmail
	for n in 0 1 2 3 4 5 6 7 8 9; do \
	  $(mkinstalldirs) $(DESTDIR)$(localstatedir)/ax25/fbb/binmail/mail$$n; \
	done
	@echo
	@echo " *** Read the \'README' if you haven't already done so!"
	@echo " *** "
	@echo " *** Remember to run \'make installconf' if this is a new installation."
	@echo " *** This will install the default configuration files, BUT it WILL" 
	@echo " *** overwrite any existing config files you have."
	@echo " ***"
	@echo " *** If you are upgrading an existing configuration, you do not need"
	@echo " *** to run \'make installconf'."
	@echo

#EXTRA_DIST = ax25apps.spec SVNREV

ACLOCAL_AMFLAGS = -I m4

