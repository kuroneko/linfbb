SUBDIRS = src

all clean install:
		for i in ${SUBDIRS} ;   do \
			          ( cd $$i && ${MAKE} $@ ) || exit 1; \
				          done

