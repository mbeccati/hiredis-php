PHP_ARG_ENABLE(hiredis, whether to enable hiredis,
[ --enable-hiredis   Enable hiredis])

if test "$PHP_HIREDIS" = "yes"; then
  AC_DEFINE(HAVE_HIREDIS, 1, [Whether you have my extension])
  PHP_NEW_EXTENSION(hiredis, hiredis.c hiredis_reader.c, $ext_shared)
fi

AC_CHECK_LIB(hiredis, redisConnectWithTimeout, [],
[AC_MSG_ERROR([library 'libhiredis', version 0.10.0 or newer, is required])])

EXTRA_LDFLAGS="-lhiredis"
