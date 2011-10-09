dnl FreeBSD -pthread check - Jonathan McDowell <<EMAIL: PROTECTED>>
AC_DEFUN(AC_PTHREAD_FREEBSD,
[AC_MSG_CHECKING([if we need -pthread for threads])
AC_CACHE_VAL(ac_ldflag_pthread,
[ac_save_LDFLAGS="$LDFLAGS"
LDFLAGS="-pthread $LDFLAGS"
AC_TRY_LINK(
[
char pthread_create();
],
pthread_create();,
eval "ac_ldflag_pthread=yes",
eval "ac_ldflag_pthread=no"),
LIBS="$ac_save_LDFLAGS"
])
if eval "test \"`echo $ac_ldflag_pthread`\" = yes"; then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi
])

