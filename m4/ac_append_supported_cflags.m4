dnl AC_APPEND_SUPPORTED_CFLAGS(CFLAGS_VARIABLE, [flags to check])
dnl Example: AC_APPEND_SUPPORTED_CFLAGS(JSONSPIRIT_CFLAGS, [-Wall -Wextra])
dnl If flags are supported, they are appended to the variable passed as the first argument

AC_DEFUN([AC_APPEND_SUPPORTED_CFLAGS], [
  AC_MSG_CHECKING([whether $CC supports $2])
  orig_cflags="$CFLAGS"
  CFLAGS="-Werror $2"
  AC_LINK_IFELSE([AC_LANG_PROGRAM()],
                          [support_c_flag=yes],
                          [support_c_flag=no])
  if test "x$support_c_flag" = "xyes"; then
    AC_MSG_RESULT(yes)
    $1="$$1 $2"
  else
    AC_MSG_RESULT(no)
  fi
  CFLAGS="$orig_cflags"
])
