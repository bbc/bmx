SUBDIRS = include src apps tools test msvc_build


pkgconfig_DATA = libbmx-$(BMX_MAJORMINOR).pc

libbmx-$(BMX_MAJORMINOR).pc: libbmx.pc
	cp libbmx.pc libbmx-$(BMX_MAJORMINOR).pc

CLEANFILES = libbmx-$(BMX_MAJORMINOR).pc


ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = autogen.sh
