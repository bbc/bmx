dnl-----------------------------------------------------------------------------
dnl -- Extract package info
dnl-----------------------------------------------------------------------------

dnl extract package name and version from Version.h

m4_define([m4_BMX_LIBRARY_NAME],
	m4_esyscmd([(sed -n 's/^.*BMX_LIBRARY_NAME[^\"]*\"\([^\"]*\)\"/\1/p' include/bmx/Version.h | tr -d '\n')]))
m4_define([m4_BMX_VERSION_MAJOR],
	m4_esyscmd([(sed -n 's/^.*BMX_VERSION_MAJOR\ *\([0-9][0-9]*\)/\1/p' include/bmx/Version.h | tr -d '\n')]))
m4_define([m4_BMX_VERSION_MINOR],
	m4_esyscmd([(sed -n 's/^.*BMX_VERSION_MINOR\ *\([0-9][0-9]*\)/\1/p' include/bmx/Version.h | tr -d '\n')]))
m4_define([m4_BMX_VERSION_MICRO],
	m4_esyscmd([(sed -n 's/^.*BMX_VERSION_MICRO\ *\([0-9][0-9]*\)/\1/p' include/bmx/Version.h | tr -d '\n')]))
m4_define([m4_BMX_VERSION],
	m4_defn([m4_BMX_VERSION_MAJOR]).m4_defn([m4_BMX_VERSION_MINOR]).m4_defn([m4_BMX_VERSION_MICRO]))


dnl-----------------------------------------------------------------------------
dnl -- Init
dnl-----------------------------------------------------------------------------

AC_INIT(m4_defn([m4_BMX_LIBRARY_NAME]), m4_defn([m4_BMX_VERSION]))
AC_CONFIG_SRCDIR([src/common/BMXException.cpp])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall parallel-tests])
AM_SILENT_RULES([yes])

dnl define for automake 1.12 to avoid warnings
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])


dnl-----------------------------------------------------------------------------
dnl -- Version
dnl-----------------------------------------------------------------------------

dnl CURRENT, REVISION, AGE
dnl - library source changed -> increment REVISION
dnl - interfaces added/removed/changed -> increment CURRENT, REVISION = 0
dnl - interfaces added -> increment AGE
dnl - interfaces removed -> AGE = 0
BMX_LIBVERSION="4:0:0"
AC_SUBST(BMX_LIBVERSION)


BMX_MAJORMINOR=m4_defn([m4_BMX_VERSION_MAJOR]).m4_defn([m4_BMX_VERSION_MINOR])
AC_SUBST(BMX_MAJORMINOR)


dnl-----------------------------------------------------------------------------
dnl -- Checks for programs.
dnl-----------------------------------------------------------------------------

AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL


dnl-----------------------------------------------------------------------------
dnl -- Checks for libraries.
dnl-----------------------------------------------------------------------------

ORIGCFLAGS=$CFLAGS
if test x"$prefix" != x"NONE"; then
	CFLAGS="${CFLAGS} -L${prefix}/lib -I${prefix}/include"
fi

dnl check for clock_gettime
AC_CHECK_LIB(rt, clock_gettime, have_clock_gettime=yes)
if test x$have_clock_gettime = xyes ; then
	AC_DEFINE(HAVE_CLOCK_GETTIME, 1,
			  [Define to 1 if you have librt and the `clock_gettime' function])
	RT_LIB=-lrt
fi

dnl Check for UUID generation library
case "$host" in
	*-*-*mingw*) os=win ;;
	*-*-darwin*) os=mac ;;
esac
if test x"$os" = xwin; then
	AC_CHECK_HEADER([windows.h],
					[UUIDLIB="-lole32"],
					AC_MSG_ERROR(No windows uuid generation function))
elif test x"$os" = xmac; then
	UUIDLIB=""
else
	AC_CHECK_LIB([uuid], [uuid_generate],
			     [UUIDLIB="-luuid"],
			     AC_MSG_ERROR(No uuid generation function))
fi
AC_SUBST(UUIDLIB)

AC_CHECK_LIB(uriparser,uriParseUriA,,
	[AC_MSG_ERROR([liburiparser not found])])
if test x"$prefix" = x"NONE"; then
	LIBURIPARSER_CFLAGS=""
	LIBURIPARSER_LIBS="-luriparser"
else
	LIBURIPARSER_CFLAGS="-I${prefix}/include"
	LIBURIPARSER_LIBS="-L${prefix}/lib -luriparser"
fi
AC_SUBST(LIBURIPARSER_CFLAGS)
AC_SUBST(LIBURIPARSER_LIBS)

dnl version 0.7.2 has been tested
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <uriparser/Uri.h>
#if (defined(URI_VER_MAJOR) && defined(URI_VER_MINOR) && defined(URI_VER_RELEASE) \
&& ((URI_VER_MAJOR > 0) \
|| ((URI_VER_MAJOR == 0) && (URI_VER_MINOR > 7)) \
|| ((URI_VER_MAJOR == 0) && (URI_VER_MINOR == 7) && (URI_VER_RELEASE >= 2)) \
))
/* FINE */
#else
# error uriparser not recent enough
#endif
])],,[AC_MSG_ERROR([uriparser 0.7.2 or later is required, your copy is too old])])


AC_CHECK_LIB(expat, XML_GetFeatureList,
		EXPAT_LIBS="-lexpat",
		AC_MSG_ERROR([Require expat library]))
AC_CHECK_HEADERS(expat.h,
		EXPAT_CFLAGS="",
		AC_MSG_ERROR([Require expat development header files]))
AC_SUBST(EXPAT_CFLAGS)
AC_SUBST(EXPAT_LIBS)


CFLAGS=$ORIGCFLAGS


dnl-----------------------------------------------------------------------------
dnl -- Check for modules
dnl-----------------------------------------------------------------------------

LIBMXF_MAJORMINOR=1.0
LIBMXFPP_MAJORMINOR=${LIBMXF_MAJORMINOR}
AC_SUBST(LIBMXF_MAJORMINOR)
AC_SUBST(LIBMXFPP_MAJORMINOR)

PKG_CHECK_MODULES(LIBMXF, libMXF-${LIBMXF_MAJORMINOR} >= 1.0.3)
AC_SUBST(LIBMXF_CFLAGS)
AC_SUBST(LIBMXF_LIBS)

PKG_CHECK_MODULES(LIBMXFPP, libMXF++-${LIBMXFPP_MAJORMINOR} >= 1.0.3)
AC_SUBST(LIBMXFPP_CFLAGS)
AC_SUBST(LIBMXFPP_LIBS)

LIBCURL_VER="7.22.0"
PKG_CHECK_MODULES(LIBCURL, libcurl >= $LIBCURL_VER, HAVE_LIBCURL=yes, HAVE_LIBCURL=no)
if test "x${HAVE_LIBCURL}" == xyes; then
	AC_DEFINE([HAVE_LIBCURL], [1], [Define to 1 if you have the libcurl library])
else
	AC_MSG_WARN([Disabled HTTP file access because libcurl >= $LIBCURL_VER not found])
fi


dnl-----------------------------------------------------------------------------
dnl -- Checks for header files.
dnl-----------------------------------------------------------------------------

AC_CHECK_HEADERS([inttypes.h sys/time.h sys/timeb.h unistd.h])


dnl-----------------------------------------------------------------------------
dnl -- Checks for typedefs, structures, and compiler characteristics.
dnl-----------------------------------------------------------------------------

AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T


dnl mingw doesn't support the 'z' printf format length modifier for size_t
case "$host" in
	*-*-*mingw32) AC_DEFINE([PRIszt], ["u"], [Define printf size_t format specifier]) ;;
			   *) AC_DEFINE([PRIszt], ["zu"], [Define printf size_t format specifier])
esac


dnl-----------------------------------------------------------------------------
dnl -- Checks for library functions
dnl-----------------------------------------------------------------------------

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO


AC_CHECK_FUNCS([getcwd gettimeofday memmove memset mkdir strerror strerror_r nanosleep gmtime_r])


dnl-----------------------------------------------------------------------------
dnl -- Compiler flags etc.
dnl-----------------------------------------------------------------------------

WARN_CFLAGS=
AC_APPEND_SUPPORTED_CFLAGS(WARN_CFLAGS, [-W -Wall])
AC_SUBST(WARN_CFLAGS)

BMX_CFLAGS="${WARN_CFLAGS} ${LIBMXF_CFLAGS} ${LIBMXFPP_CFLAGS} \
	${LIBURIPARSER_CFLAGS} ${EXPAT_CFLAGS} ${LIBCURL_CFLAGS} -I\$(top_srcdir)/include"
AC_SUBST(BMX_CFLAGS)

BMX_LIBADDLIBS="-lm ${RT_LIB} ${UUIDLIB} ${LIBURIPARSER_LIBS} ${LIBMXF_LIBS} \
	${LIBMXFPP_LIBS} ${EXPAT_LIBS} ${LIBCURL_LIBS}"
AC_SUBST(BMX_LIBADDLIBS)

BMX_LDADDLIBS="${BMX_LIBADDLIBS} \
	\$(top_builddir)/src/libbmx-${BMX_MAJORMINOR}.la"
AC_SUBST(BMX_LDADDLIBS)

BMX_LDFLAGS=
AC_APPEND_SUPPORTED_LDFLAGS(BMX_LDFLAGS, [-Wl,--no-undefined])
AC_SUBST(BMX_LDFLAGS)


dnl-----------------------------------------------------------------------------
dnl -- Package Config
dnl-----------------------------------------------------------------------------

pkgconfigdir="\$(libdir)/pkgconfig"
AC_SUBST(pkgconfigdir)

dnl add libraries to pkg config "Libs:" for static-only builds
if test x"$enable_shared" = xyes; then
	PC_ADD_LIBS=
	PC_ADD_PRIVATE_LIBS="-lm ${RT_LIB} ${UUIDLIB} ${LIBURIPARSER_LIBS} ${EXPAT_LIBS}"
else
	PC_ADD_LIBS="-lm ${RT_LIB} ${UUIDLIB} ${LIBURIPARSER_LIBS} ${EXPAT_LIBS}"
	PC_ADD_PRIVATE_LIBS=
fi
AC_SUBST(PC_ADD_LIBS)
AC_SUBST(PC_ADD_PRIVATE_LIBS)


dnl-----------------------------------------------------------------------------
dnl -- Config files
dnl-----------------------------------------------------------------------------

AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	src/apps/Makefile
	src/common/Makefile
	src/frame/Makefile
	src/essence_parser/Makefile
	src/writer_helper/Makefile
	src/mxf_helper/Makefile
	src/mxf_reader/Makefile
	src/as02/Makefile
	src/mxf_op1a/Makefile
	src/avid_mxf/Makefile
	src/d10_mxf/Makefile
	src/as11/Makefile
	src/rdd9_mxf/Makefile
	src/wave/Makefile
	src/clip_writer/Makefile
	src/st436/Makefile
	test/Makefile
	test/ard_zdf_hdf/Makefile
	test/as02/Makefile
	test/as11/Makefile
	test/mxf_op1a/Makefile
	test/avid_mxf/Makefile
	test/mxf_reader/Makefile
	test/d10_mxf/Makefile
	test/rdd9_mxf/Makefile
	test/wave/Makefile
	test/essence_parser/Makefile
	test/growing_file/Makefile
	test/rdd6/Makefile
	apps/Makefile
	apps/mxf2raw/Makefile
	apps/raw2bmx/Makefile
	apps/bmxtranswrap/Makefile
	tools/Makefile
	msvc_build/Makefile
	msvc_build/vs10/Makefile
	meta/Makefile
	meta/as11/Makefile
	meta/mxf2raw/Makefile
	meta/rdd6/Makefile
	bmx.pc])

AC_OUTPUT

