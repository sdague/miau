AC_INIT([miau], [0.5.4])
AC_CONFIG_SRCDIR([src/miau.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])
AH_TEMPLATE([VERSIONNAME], [long version name])
AC_DEFINE([VERSIONNAME], ["catnap"])

# option, explanation, define
AC_DEFUN([MY_ARG_ENABLE], [
	AH_TEMPLATE([$3], [$2])
	AC_ARG_ENABLE([$1], AC_HELP_STRING([--enable-$1], [$2]),
		[USE_$3=yes], [USE_$3=no])
	AC_CACHE_CHECK([whether to use $1], [USE_$3], [USE_$3=no])
	test "x${USE_$3}" = "xyes" && AC_DEFINE([$3], [yes])
])

# option, explanation, define
AC_DEFUN([MY_ARG_DISABLE], [
	AH_TEMPLATE([$3], [$2])
	AC_ARG_ENABLE([$1], AC_HELP_STRING([--disable-$1], [$2]),
		[USE_$3=no], [USE_$3=yes])
	AC_CACHE_CHECK([whether to use $1], [USE_$3], [USE_$3=yes])
	test "x${USE_$3}" = "xyes" && AC_DEFINE([$3], [yes])
])

AH_TEMPLATE([VSNPRINTF_WORKAROUND], [quick hack to vsnprintf])
AC_ARG_WITH([vsnprintf-workaround],
	AC_HELP_STRING([--with-vsnprintf-workaround],
		[use a quick hack to ignore missing vsnprintf]),
	[vsnprintf_workaround=yes], [vsnprintf_workaround=no])
AC_ARG_WITH([dont-check-select],
	AC_HELP_STRING([--with-dont-check-select],
		[don't check existence of "select()" since we can't find it on HP-UX]),
	[dont_check_select=yes], [dont_check_select=no])

AC_ARG_ENABLE([local], AC_HELP_STRING([--enable-local],
	[install miau locally, not system-wide]),
	[ac_local=yes], [ac_local=no])

AH_TEMPLATE([IPV6], [IPV6 available?])
AC_ARG_ENABLE([ipv6], AC_HELP_STRING([--enable-ipv6],
	[enable ipv6-support]))

MY_ARG_ENABLE([dccbounce], [enable DCC-bouncing capability], [DCCBOUNCE])
MY_ARG_ENABLE([automode], [enable automode -capability], [AUTOMODE])
MY_ARG_ENABLE([releasenick], [enable release nick -feature], [RELEASENICK])

MY_ARG_ENABLE([ctcp-replies], [enable ctcp-replies], [CTCPREPLIES])
MY_ARG_ENABLE([mkpasswd], [enable mkpasswd-capability], [MKPASSWD])
MY_ARG_ENABLE([uptime], [enable uptime-command], [UPTIME])
MY_ARG_ENABLE([chanlog], [enable channel logging], [CHANLOG])
MY_ARG_ENABLE([privlog], [enable private message logging], [PRIVLOG])
MY_ARG_ENABLE([onconnect], [enable on-connect actions], [ONCONNECT])
MY_ARG_ENABLE([empty-awaymsg], [allow empty AWAY messages], [EMPTYAWAY])

MY_ARG_DISABLE([ascii-art], [disable fancy ascii-art], [ASCIIART])
MY_ARG_DISABLE([quicklog], [disable quicklog], [QUICKLOG])
MY_ARG_DISABLE([qlogstamp], [disable quicklog timestamps], [QLOGSTAMP])
MY_ARG_DISABLE([inbox], [disable message inbox], [INBOX])
# MY_ARG_DISABLE([qlogtomsglog], [disable quicklog to messagelog -feature],
#	[QLOGTOMSGLOG])


MY_ARG_ENABLE([enduserdebug], [enable debugging feature for end-user (debug)],
	[ENDUSERDEBUG])
MY_ARG_ENABLE([pingstat], [enable ping-statistics (debug)], [PINGSTAT])
MY_ARG_ENABLE([dumpstatus], [enable command to dump miau's status (debug)],
		[DUMPSTATUS])

CFLAGS=""		# Possible -O2 in following if-then-else -statement
AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug],
		[produce debugging information (for gdb etc.) (debug)]),
	[ac_debug=yes], [ac_debug=no])
if test "x$ac_debug" = "xyes"; then
	CFLAGS="${CFLAGS} -g"
else
	CFLAGS="${CFLAGS} -O2"
fi
#AC_ARG_ENABLE([strict-compile], AC_HELP_STRING([--enable-strict-compile],
#	[make all warnings into errors (debug)]),
#[ac_werrors=yes], [ac_werrors=no])
#if test "x$ac_w_errors" = "xyes"; then
#	CFLAGS="${CFLAGS} -Werror"
#fi

#AH_TEMPLATE([LOGGING], [need logging-related features])
#if test "x$USE_chanlog" = "xyes" -o "x$msglog" = "xyes"; then
#	AC_DEFINE([LOGGING], [yes])
#fi

AC_PROG_CC

AC_PROG_MAKE_SET
AC_PROG_INSTALL

if test "$ac_local" = yes; then
	AC_MSG_RESULT(configuring for local installation)
	bindir=$HOME/miau
	sbindir=$bindir
	datadir=$HOME/.miau
	infodir=$HOME/miau
	mandir=$infodir
else
	datadir="$datadir/doc/miau/examples"
fi
AC_SUBST([ac_local bindir sbindir datadir infodir mandir exampledir])

if test "x$GCC" = "xyes"; then
	CFLAGS="${CFLAGS} -Wall"
fi

if test "x$enable_ipv6" = "xyes"; then
	AC_CHECK_FUNC(gethostbyname2, AC_DEFINE(IPV6))
fi

dnl Checks for libraries.
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(gethostbyaddr, nsl)
AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(hstrerror, resolv)
AC_SEARCH_LIBS(crypt, crypt)


dnl Checks for header files.
AC_HEADER_STDC

if test "$ac_cv_header_stdc" = no; then
	AC_MSG_ERROR(ansi-c headers are required!)
fi

AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h errno.h pwd.h crypt.h)
AC_CHECK_HEADERS(sys/types.h sys/socket.h sys/stat.h netinet/in.h netdb.h arpa/inet.h signal.h ctype.h strings.h)


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM


dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(socket gethostbyname gethostbyaddr inet_ntoa sigemptyset, ,
		AC_MSG_ERROR(required function not found))
if test "x$dont_check_select" != "xyes"; then
	AC_CHECK_FUNCS([select], [found_select=yes], [found_select=no])
	if test "x$found_select" != "xyes"; then
		cat <<EOF

configure couldn't find select() on your system - which is most unlikely. This
is a know issue when compiling miau on HP-UX. If you want to ignore this missing
select, run configure with --with-dont-check-select and keep your fingers
crossed.
EOF
		AC_MSG_ERROR([required function not found])
	fi
fi
AC_CHECK_FUNCS(strchr strdup strerror strncasecmp strcasecmp bzero strtoul, ,
		AC_MSG_ERROR(required function not found))
AC_CHECK_FUNCS(crypt getpass gethostname, ,
		AC_MSG_ERROR(required function not found))
AC_CHECK_FUNCS(sigaction random hstrerror)
AC_CHECK_FUNCS([vsnprintf], [have_vsnprintf=yes], [have_vsnprintf=no])

if test "x$have_vsnprintf" != "xyes"; then
	if test "x$vsnprintf_workaround" = "xyes"; then
		AC_DEFINE([VSNPRINTF_WORKAROUND])
	else
		cat <<EOF

Your system doesn't have function vsnprintf. miau can use vsprintf instead,
but this may result in illegal memory accesses, which will most definetly
cause segfaults. If you are sure you want to do this, use
--with-vsnprintf-workaround and watch miau going down, hard. :-)
EOF
		AC_MSG_ERROR([required function not found])
	fi
fi

AC_CHECK_FUNCS([backtrace backtrace_symbols])

AC_OUTPUT([Makefile src/Makefile doc/Makefile misc/Makefile misc/miaurc miau.spec])

if test "x$have_vsnprintf" != "xyes" -a \
		"x$vsnprintf_workaround" = "xyes"; then
	echo "- WARNING: Using segfault-prone vsnprintf-workaround."
fi
if test "x$dont_check_select" = "xtrue"; then
	echo "- WARNING: Never checked for select."
fi


AC_MSG_RESULT([Now type 'make' to compile.])

