m4_include([version.m4])
AC_PREREQ(2.59)
AC_INIT([iplike], VERSION_NUMBER, [http://www.opennms.org])

RELEASE=1
AC_SUBST([RELEASE])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_ENABLE_STATIC([no])

# don't test c++ and f77 in libtool
m4_ifdef([AC_LIBTOOL_TAGS], [AC_LIBTOOL_TAGS([])])

AC_HEADER_STDC

AC_PROG_CC
AC_PROG_LN_S
AC_PROG_LIBTOOL
AC_PROG_INSTALL
ONMS_CHECK_SUNCC
ONMS_SET_CC_WARNING_CFLAGS

dnl
dnl lovingly stolen from postgis' autoconf macros
dnl

PGCONFIG=
AC_ARG_WITH(pgsql, 
[  --with-pgsql[[=ARG]]      build for a specific pgsql version
                          [[ARG=path to pg_config]]],
	if test "$with_pgsql" != "no" -a "$with_pgsql" != "yes"; then
		if test ! -f "$with_pgsql"; then
			AC_MSG_ERROR([Invalid argument to --with-pgsql]);
		fi
		PGCONFIG="$with_pgsql"
	fi
)
if test -z "$PGCONFIG"; then
	AC_PATH_PROG([PGCONFIG], [pg_config])
fi
if test -z "$PGCONFIG"; then
	AC_MSG_ERROR([PostgreSQL is required]);
fi
PGINCLUDES="-I`${PGCONFIG} --includedir` -I`${PGCONFIG} --includedir-server`"
PGLIBS="-L'`${PGCONFIG} --libdir`'"
PGLIBDIR="`${PGCONFIG} --libdir`"
PSQL_BINDIR=`$PGCONFIG --bindir`
PGPLUGINDIR=`"${PGCONFIG}" --pkglibdir`

PGLONGVER="`${PGCONFIG} --version | sed -e 's,.* ,,' -e 's,^\([[0-9]]*\.[[0-9]]*\).*$,\1,'`"
PGSHORTVER="`${PGCONFIG} --version | sed -e 's,.* ,,' -e 's,^\([[0-9]]*\)\.\([[0-9]]*\).*$,\1\2,'`"

CPPFLAGS="$CPPFLAGS -DWIN32_CLIENT_ONLY"

saved_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS} ${PGINCLUDES}"

PGAC_TYPE_64BIT_INT([long int])

if test x"$HAVE_LONG_INT_64" = x"no" ; then
  PGAC_TYPE_64BIT_INT([long long int])
fi

AC_CHECK_HEADERS([windef.h])
AC_MSG_CHECKING(for postgres.h)
AC_CACHE_VAL(onms_cv_postgres_h_good_without_undef,
  [
    AC_TRY_COMPILE([
      /* These are defined in pg_config.h and in confdefs.h, which is bad */
      #undef PACKAGE_BUGREPORT
      #undef PACKAGE_NAME
      #undef PACKAGE_STRING
      #undef PACKAGE_TARNAME
      #undef PACKAGE_VERSION

      #ifdef HAVE_WINDEF_H
      #include <winsock2.h>
      #include <windef.h>
      #endif

      #include <ctype.h>
      #include <postgres.h>
    ],
    [],
    [onms_cv_postgres_h_good_without_undef=yes],
    [onms_cv_postgres_h_good_without_undef=no])
  ])
AC_MSG_RESULT($onms_cv_postgres_h_good_without_undef)

AS_IF([test "x$onms_cv_postgres_h_good_without_undef" != "xyes"],
  [
    AC_MSG_CHECKING(for postgres.h with _FILE_OFFSET_BITS undefined)
    AC_CACHE_VAL(onms_cv_postgres_h_good_with_undef,
      [
        AC_TRY_COMPILE([
          /* These are defined in pg_config.h and in confdefs.h, which is bad */
          #undef PACKAGE_BUGREPORT
          #undef PACKAGE_NAME
          #undef PACKAGE_STRING
          #undef PACKAGE_TARNAME
          #undef PACKAGE_VERSION
    
          #ifdef HAVE_WINDEF_H
          #include <winsock2.h>
          #include <windef.h>
          #endif

          #include <ctype.h>
    
          #undef _FILE_OFFSET_BITS
    
          #include <postgres.h>
        ],
        [],
        [onms_cv_postgres_h_good_with_undef=yes],
        [onms_cv_postgres_h_good_with_undef=no])
      ])
    AC_MSG_RESULT($onms_cv_postgres_h_good_with_undef)

    AS_IF([test "x$onms_cv_postgres_h_good_with_undef" = "xyes"],
      [
        PGINCLUDES="${PGINCLUDES} -DUNDEF_FILE_OFFSET_BITS=1"
      ],
      [
        AC_MSG_ERROR([could not compile test program with postgres.h])
      ]
    )
  ]
)
CPPFLAGS="${saved_CPPFLAGS}"

PG_BUNDLE_LOADER=""
CLI_LDFLAGS=""

AX_CHECK_LINK_FLAG(["-Wl,-bundle_loader,${PSQL_BINDIR}/postgres"],[
  PG_BUNDLE_LOADER='-Wl,-bundle_loader,$(PSQL_BINDIR)/postgres'
],[],[-Wl,-bundle])
AX_CHECK_LINK_FLAG([-Wl,-U,_pg_detoast_datum_packed],[
  CLI_LDFLAGS="$CLI_LDFLAGS -Wl,-U,_pg_detoast_datum_packed"
])
AX_CHECK_LINK_FLAG([-Wl,--unresolved-symbols=ignore-in-object-files],[
  CLI_LDFLAGS="$CLI_LDFLAGS -Wl,--unresolved-symbols=ignore-in-object-files"
])

AC_SUBST([PGCONFIG])
AC_SUBST([PGINCLUDES])
AC_SUBST([PGLIBS])
AC_SUBST([PGLIBDIR])
AC_SUBST([PSQL_BINDIR])
AC_SUBST([PGPLUGINDIR])
AC_SUBST([PG_BUNDLE_LOADER])
AC_SUBST([CLI_LDFLAGS])

AM_RPM_INIT
AM_CONDITIONAL(MAKE_RPMS, test x$make_rpms = xtrue)

AC_CONFIG_FILES([Makefile iplike.spec install_iplike.sh debian/setver.sh])

AC_OUTPUT

chmod a+x debian/setver.sh >/dev/null 2>&1 || :
debian/setver.sh "$PGLONGVER" "$PACKAGE_VERSION" >/dev/null 2>&1 || :
