
dnl Like AC_TRY_EVAL but also errors out if the compiler generates
dnl _any_ output. Some compilers might issue warnings which we want
dnl to catch.
AC_DEFUN([AC_TRY_EVAL2],
[{ (eval echo configure:__oline__: \"[$]$1\") 1>&AC_FD_CC; dnl
(eval [$]$1) 2>&AC_FD_CC; _out=`eval [$]$1 2>&1` && test "x$_out" = x; }])


dnl Like AC_TRY_COMPILE but calls AC_TRY_EVAL2 instead of AC_TRY_EVAL
AC_DEFUN([AC_TRY_COMPILE2],
[cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
[$1]
int main(void) {
[$2]
; return 0; }
EOF
if AC_TRY_EVAL2(ac_compile); then
  ifelse([$3], , :, [rm -rf conftest*
  $3])
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
ifelse([$4], , , [  rm -rf conftest*
  $4
])dnl
fi
rm -f conftest*])

dnl Determine what socket length (socklen_t) data type is 
AC_DEFUN([AC_SOCKLEN_T],
[
AC_MSG_CHECKING([for type of socket length (socklen_t)])
AC_TRY_COMPILE2([
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>],[
(void)getsockopt (1, 1, 1, NULL, (socklen_t *)NULL)],[
  AC_MSG_RESULT(socklen_t *)
  SOCKLEN_T=socklen_t],[
  AC_TRY_COMPILE2([
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>],[
(void)getsockopt (1, 1, 1, NULL, (size_t *)NULL)],[
    AC_MSG_RESULT(size_t *)
    SOCKLEN_T=size_t],[
    AC_TRY_COMPILE2([
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>],[
(void)getsockopt (1, 1, 1, NULL, (int *)NULL)],[
      AC_MSG_RESULT(int *)
      SOCKLEN_T=int],[
      AC_MSG_WARN(could not determine)
      SOCKLEN_T="unsigned int"])])])

if test "$SOCKLEN_T" = socklen_t; then
  AC_DEFINE(HAVE_SOCKLEN_T, 1, socklen_t defined in sys/socket.h)
fi

AC_DEFINE_UNQUOTED(SOCKLEN_T, $SOCKLEN_T, [Determine what socket length (socklen_t) data type is])

])
