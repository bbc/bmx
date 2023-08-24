1) install libMXF (ingex/lib) and libMXFReader (ingex/examples/reader) in /usr/local

2) add mxf plugin to vlc/configure.ac 

a) add the following lines:

dnl
dnl  mxf plugin
dnl
AC_ARG_ENABLE(mxf,
  [  --enable-mxf            experimental mxf demux support (default disabled)])
if test "${enable_mxf}" = "yes"
then
  	VLC_ADD_PLUGINS([mxf])
    VLC_ADD_LDFLAGS([mxf],[-lMXFReader -lMXF -luuid])
fi


b) add to AC_CONFIG_FILES
  modules/demux/mxf/Makefile



3) create vlc/modules/demux/mxf directory and copy mxf_vlc.c and Modules.am

4) run vlc/bootstrap

5) ./configure --enable-dv --enable-mxf

6) make

7) sudo make install

