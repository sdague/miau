AC_INIT([miau], [0.6.2])
AC_CONFIG_SRCDIR([src/miau.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])
AH_TEMPLATE([VERSIONNAME], [long version name])
AC_DEFINE([VERSIONNAME], ["All the trouble and twice the bugs"])


# option, explanation, define
AC_DEFUN([MY_ARG_ENABLE], [
	AH_TEMPLATE([$3], [$2])
	AC_ARG_ENABLE([$1],
		AC_HELP_STRING([--enable-$1], [$2]),
		[use_$3=$enableval], [use_$3=no])
	test "x${use_$3}" = "xyes" && AC_DEFINE([$3], [yes])
])

# option, explanation, define
AC_DEFUN([MY_ARG_DISABLE], [
	AH_TEMPLATE([$3], [$2])
	AC_ARG_ENABLE([$1],
		AC_HELP_STRING([--disable-$1], [$2]),
		[use_$3=$enableval], [use_$3=yes])
	test "x${use_$3}" = "xyes" && AC_DEFINE([$3], [yes])
])

AC_ARG_WITH([snprintf-workaround],
	AC_HELP_STRING([--with-snprintf-workaround],
		[use a quick hack to ignore missing (v)snprintf]),
	[snprintf_workaround=yes], [snprintf_workaround=no])

AC_ARG_WITH([dont-check-select],
	AC_HELP_STRING([--with-dont-check-select],
		[don't check existence of "select()" since we can't find it on HP-UX]),
	[dont_check_select=yes], [dont_check_select=no])

AC_ARG_ENABLE([local], AC_HELP_STRING([--enable-local],
	[install miau locally, not system-wide]),
	[ac_local=yes], [ac_local=no])

AH_TEMPLATE([IPV6], [IPV6 available?])
AC_ARG_ENABLE([ipv6],
	AC_HELP_STRING([--enable-ipv6],
	[enable IPv6 ONLY -support (no IPv4!)]),
	[use_IPV6=$enableval], [use_IPV6=no])

MY_ARG_ENABLE([dccbounce], [enable DCC-bouncing capability], [DCCBOUNCE])
MY_ARG_ENABLE([automode], [enable automode -capability], [AUTOMODE])
MY_ARG_ENABLE([releasenick], [enable release nick -feature], [RELEASENICK])

MY_ARG_ENABLE([mkpasswd], [enable mkpasswd-capability], [MKPASSWD])
MY_ARG_ENABLE([uptime], [enable uptime-command], [UPTIME])
MY_ARG_ENABLE([chanlog], [enable channel logging], [CHANLOG])
MY_ARG_ENABLE([privlog], [enable private message logging], [PRIVLOG])
MY_ARG_ENABLE([onconnect], [enable on-connect actions], [ONCONNECT])
MY_ARG_ENABLE([empty-awaymsg], [allow empty AWAY messages], [EMPTYAWAY])

MY_ARG_DISABLE([ctcp-replies], [disable ctcp-replies], [CTCPREPLIES])
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

AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug],
		[produce debugging information (for gdb etc.) (debug)]),
	[use_DEBUG=yes], [use_DEBUG=no])
if test "x$use_DEBUG" = "xyes"; then
	CFLAGS="${CFLAGS} -g"
fi


dnl Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h crypt.h fcntl.h netdb.h netinet/in.h])
AC_CHECK_HEADERS([stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])


dnl Checks for libraries.
AC_SEARCH_LIBS(gethostbyname, [inet nsl])
AC_SEARCH_LIBS(gethostbyaddr, [nsl])
AC_SEARCH_LIBS(socket, [socket])
AC_SEARCH_LIBS(hstrerror, [resolv])
AC_SEARCH_LIBS(crypt, [crypt])


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_HEADER_TIME
AC_STRUCT_TM
AC_HEADER_STDBOOL


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

if test "x$use_IPV6" = "xyes"; then
	AC_CHECK_FUNC(gethostbyname2, AC_DEFINE(IPV6))
fi


# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm atexit dup2])
AC_CHECK_FUNCS([gethostbyaddr gethostbyname gethostname getpass gettimeofday])
AC_CHECK_FUNCS([inet_ntoa memmove mkdir random])
AC_CHECK_FUNCS([select socket strcasecmp strchr strdup strerror strncasecmp])
AC_CHECK_FUNCS([strrchr strtol strtoul])

AC_CHECK_FUNCS([bzero crypt sigaction])


if test "$ac_cv_header_stdc" = no; then
	AC_MSG_ERROR(ansi-c headers are required!)
fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM


dnl Checks for platform specific stuff.
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

AC_CHECK_FUNCS([vsnprintf], [have_snprintf=yes], [have_snprintf=no])
AC_CHECK_FUNCS([snprintf], , [have_snprintf=no])
if test "x$have_snprintf" != "xyes"; then
	if test "x$snprintf_workaround" != "xyes"; then
		cat <<EOF

Your system doesn't have function (v)snprintf. miau can use (v)sprintf instead,
but this may result in illegal memory access, which will most definitely be a
security risk and cause segfaults. If you are sure you want to do this, use
--with-snprintf-workaround and watch miau going down, hard. :-)
EOF
		AC_MSG_ERROR([required function not found])
	fi
fi

# socklen_t stuff (Lars Brinkhoff <lars@nocrew.org>)
AC_CACHE_CHECK([for socklen_t], [ac_cv_type_socklen_t], [
	AC_TRY_COMPILE(
		[#include <sys/types.h>
		#include <sys/socket.h>],
		[socklen_t len = 42; return 0;],
		[ac_cv_type_socklen_t=yes],
		[ac_cv_type_socklen_t=no])
])
if test "x$ac_cv_type_socklen_t" != "xyes"; then
	AC_DEFINE([socklen_t], [int], [Substitute for socklen_t])
fi

# hstrerror on Digital UNIX
_CFLAGS="${CFLAGS}"
CFLGAS="-Werror"
AC_CACHE_CHECK([for hstrerror prototype], [ac_cv_func_hstrerror], [
	AC_TRY_LINK(
		[#include <netdb.h>],
		[const char *ret; ret = hstrerror(1); return 0;],
		[ac_cv_func_hstrerror=yes],
		[ac_cv_func_hstrerror=no])
])
if test "x${ac_cv_func_hstrerror}" = "xyes"; then
	AH_TEMPLATE([HAVE_HSTRERROR_PROTO], [have prototype for hstrerror])
	AC_DEFINE([HAVE_HSTRERROR_PROTO], [yes])
fi
CFLAGS="${_CFLAGS}"

# try to find va_copy
AC_CACHE_CHECK([for va_copy], [ac_cv_macro_va_copy], [
	AC_TRY_LINK(
		[#include <stdarg.h>],
		[va_list a1, a2; va_copy(a1, a2); return 0;],
		[ac_cv_macro_va_copy=yes],
		[ac_cv_macro_va_copy=no])
])
if test "x${ac_cv_macro_va_copy}" = "xyes"; then
	AH_TEMPLATE([HAVE_VA_COPY], [have va_copy macro])
	AC_DEFINE([HAVE_VA_COPY], [yes])
fi
AC_CACHE_CHECK([for __va_copy], [ac_cv_macro___va_copy], [
	AC_TRY_LINK(
		[#include <stdarg.h>],
		[va_list a1, a2; va_copy(a1, a2); return 0;],
		[ac_cv_macro___va_copy=yes],
		[ac_cv_macro___va_copy=no])
])
if test "x${ac_cv_macro___va_copy}" = "xyes"; then
	AH_TEMPLATE([HAVE___VA_COPY], [have __va_copy macro])
	AC_DEFINE([HAVE___VA_COPY], [1])
fi


AC_OUTPUT([Makefile src/Makefile doc/Makefile misc/Makefile])
AC_OUTPUT([misc/miaurc miau.spec])

if test "x$have_snprintf" != "xyes" -a \
		"x$snprintf_workaround" = "xyes"; then
	echo "- WARNING: Using INSECURE and crash-prone snprintf-workaround."
fi
if test "x$dont_check_select" = "xtrue"; then
	echo "- WARNING: Never checked for select."
fi

test "x$use_QLOGSTAMP" && have_QLOGSTAMP=", with timestamps"
cat <<EOF

miau $VERSION

quicklog ...... : $use_QUICKLOG$have_QLOGSTAMP
chanlog ....... : $use_CHANLOG	privlog	....... : $use_PRIVLOG	inbox ......... : $use_INBOX
CTCP-replies .. : $use_CTCPREPLIES	on-connect .... : $use_ONCONNECT	DCC-bounce .... : $use_DCCBOUNCE
automode ...... : $use_AUTOMODE	ascii-art ..... : $use_ASCIIART	mkpasswd ...... : $use_MKPASSWD
releasenick ... : $use_RELEASENICK

empty-awaymsg . : $use_EMPTYAWAY	uptime ........ : $use_UPTIME	pingstat ...... : $use_PINGSTAT
dumpstatus .... : $use_DUMPSTATUS	enduserdebug .. : $use_ENDUSERDEBUG	debug ......... : $use_DEBUG

hstrerror: $ac_cv_func_hstrerror	socklen_t: $ac_cv_type_socklen_t	snprintf: $have_snprintf

EOF
if test "x$use_IPV6" = "xyes"; then
	echo "Uses IPv6, exclusively!"
	echo ""
fi
