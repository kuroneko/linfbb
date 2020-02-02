maintscriptsdir = $(pkglibdir)/script/maintenance

maintscripts_SCRIPTS = \
	20_epurmess\
	20_epursat\
	20_epurwp

# Check and see if we are building with X11 support, if we are then 
# we also need to install the xfbbX script
if HAVE_X_LIBS
EXTRASCRIPTS = xfbbX
endif
sbin_SCRIPTS = fbb $(EXTRASCRIPTS)

edit = sed \
	-e 's|@sbindir[@]|$(sbindir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@prefix[@]|$(prefix)|g' \
	-e 's|@libdir[@]|$(libdir)|g' \
	-e 's|@docdir[@]|$(docdir)|g' \
	-e 's|@package_string[@]|$(PACKAGE_NAME)$(PACKAGE_VERSION)|g'\
	-e 's|@package_version[@]|$(PACKAGE_VERSION)|g'

20_epurmess: 20_epurmess.in Makefile
	$(edit) $(srcdir)/$@.in > $@

20_epursat: 20_epursat.in Makefile
	$(edit) $(srcdir)/$@.in > $@

20_epurwp: 20_epurwp.in Makefile
	$(edit) $(srcdir)/$@.in > $@

fbb: fbb.in Makefile
	$(edit) $(srcdir)/$@.in > $@

CLEANFILES = $(maintscripts_SCRIPTS) fbb

EXTRA_DIST = \
	20_epurmess.in\
	20_epursat.in\
	20_epurwp.in\
	fbb.in\
	xfbbX
	
