SUBDIRS = include src apps tools test msvc_build


pkgconfig_DATA = bmx-$(BMX_MAJORMINOR).pc

bmx-$(BMX_MAJORMINOR).pc: bmx.pc
	cp bmx.pc bmx-$(BMX_MAJORMINOR).pc

CLEANFILES = bmx-$(BMX_MAJORMINOR).pc


ACLOCAL_AMFLAGS = -I m4


BUILT_SOURCES = bmx_scm_version.h

bmx_scm_version.h:
	sh $(top_srcdir)/gen_scm_version.sh

.PHONY: bmx_scm_version.h

DISTCLEANFILES = bmx_scm_version.h


EXTRA_DIST = \
	autogen.sh \
	gen_scm_version.sh \
	bmx_scm_version.h
