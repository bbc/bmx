dnl AC_APPEND_SUPPORTED_LDFLAGS(LDFLAGS_VARIABLE, [flags to check])
dnl Example: AC_APPEND_SUPPORTED_LDFLAGS(JSONSPIRIT_LDFLAGS, [-Wl,--no-undefined])
dnl If flags are supported, they are appended to the variable passed as the first argument

AC_DEFUN([AC_APPEND_SUPPORTED_LDFLAGS], [
  AC_MSG_CHECKING([whether $CC supports $2])
  orig_ldflags="$LDFLAGS"
  LDFLAGS="$2"
  AC_LINK_IFELSE([AC_LANG_PROGRAM()],
                          [support_ld_flag=yes],
                          [support_ld_flag=no])
  if test "x$support_ld_flag" = "xyes"; then
    AC_MSG_RESULT(yes)
    $1="$$1 $2"
  else
    AC_MSG_RESULT(no)
  fi
  LDFLAGS="$orig_ldflags"
])
