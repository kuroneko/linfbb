docdir = $(datadir)/doc/@PACKAGE@

doc_DATA = \
	satdoc-e.doc\
	satdoc-f.doc\
	satupdat.doc\
	fbb.conf.min.sample\
	fbb.conf.sample\
	fbb.sh

edit = sed \
	-e 's|@sbindir[@]|$(sbindir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g' \
	-e 's|@prefix[@]|$(prefix)|g' \
	-e 's|@libdir[@]|$(libdir)|g' \
	-e 's|@docdir[@]|$(docdir)|g' \
	-e 's|@package_string[@]|$(PACKAGE_NAME)$(PACKAGE_VERSION)|g' \
	-e 's|@package_version[@]|$(PACKAGE_VERSION)|g'

fbb.sh: fbb.sh.in Makefile
	$(edit) $(srcdir)/$@.in > $@

fbb.conf.min.sample: fbb.conf.min.sample.in Makefile
	$(edit) $(srcdir)/$@.in > $@

fbb.conf.sample: fbb.conf.sample.in Makefile
	$(edit) $(srcdir)/$@.in > $@

CLEANFILES = fbb.sh fbb.conf.min.sample fbb.conf.sample

SUBDIRS = html

EXTRA_DIST = \
	satdoc-e.doc\
	satdoc-f.doc\
	satupdat.doc\
	fbb.conf.min.sample.in\
	fbb.conf.sample.in\
	fbb.sh.in 
