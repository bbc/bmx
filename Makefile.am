SUBDIRS = include src apps tools test msvc_build


pkgconfig_DATA = libbmx-$(BMX_MAJORMINOR).pc

libbmx-$(BMX_MAJORMINOR).pc: libbmx.pc
	cp libbmx.pc libbmx-$(BMX_MAJORMINOR).pc

CLEANFILES = libbmx-$(BMX_MAJORMINOR).pc


ACLOCAL_AMFLAGS = -I m4


BUILT_SOURCES = scm_version.h

scm_version.h:
	sh $(top_srcdir)/gen_scm_version.sh

.PHONY: scm_version.h

DISTCLEANFILES = scm_version.h


EXTRA_DIST = \
	autogen.sh \
	gen_scm_version.sh \
	scm_version.h
