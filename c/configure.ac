## Process this file with autoconf to produce a configure script.
##
## Configure.ac for RcppGSL
##
## Copyright (C) 2010         Romain Francois and Dirk Eddelbuettel
## Copyright (C) 2014 - 2019  Dirk Eddelbuettel
##
## Licensed under GNU GPL 2 or later

# The version set here will propagate to other files from here
AC_INIT([moocore],[0.9])

PKG_CONFIG_NAME="gsl-config"
PKG_DEB_NAME="libgsl-dev"
PKG_RPM_NAME="libgsl-dev"
PKG_BREW_NAME="gsl"

gsl_cflags=
AC_ARG_WITH([gsl-cflags],
            AS_HELP_STRING([--with-gsl-cflags=GSL_CFLAGS],[GNU GSL CFLAGS]),
            [gsl_cflags=$withval])
if test -n "${gsl_cflags}" ; then
   GSL_CFLAGS="${gsl_cflags}"
else
  if test -n "${GSL_CFLAGS}" ; then
     GSL_CFLAGS="${GSL_CFLAGS}"
  fi
fi

gsl_libs=
AC_ARG_WITH([gsl-libs],
            AS_HELP_STRING([--with-gsl-libs=GSL_LIBS],[GNU GSL libs]),
            [gsl_libs=$withval])
if test -n "${gsl_libs}" ; then
   GSL_LIBS="${gsl_libs}"
else
  if test -n "${GSL_LIBS}" ; then
     GSL_LIBS="${GSL_LIBS}"
  else
    ## Use gsl-config to find arguments for compiler and linker flags
    ## Check for non-standard programs: gsl-config(1)
    AC_PATH_PROG([GSL_CONFIG], [$PKG_CONFIG_NAME])
    ## If gsl-config was found, let's use it
    if test -n "${GSL_CONFIG}"; then
        # Use $PKG_CONFIG_NAME for header and linker arguments
        GSL_CFLAGS=`${GSL_CONFIG} --cflags`
        GSL_LIBS=`${GSL_CONFIG} --libs`
    else
        # Customize the error
        AC_MSG_ERROR([
      --------------------------- [MOOCORE] --------------------------------
      Configuration failed because $PKG_CONFIG_NAME was not found. Try installing:
      * Debian, Ubuntu, etc: sudo apt-get install $PKG_DEB_NAME
      * Fedora, CentOS, RHEL: sudo yum install $PKG_RPM_NAME
      * Mac OSX: brew install $PKG_BREW_NAME
      If $PKG_CONFIG_NAME is already installed, check that '$PKG_CONFIG_NAME' is in your
      PATH and PKG_CONFIG_PATH contains a $PKG_CONFIG_NAME.pc file. If $PKG_CONFIG_NAME
      is unavailable you can set GSL_CFLAGS and GSL_LIBS manually via (you will
      need to adjust paths and library names):
      make GSL_CFLAGS=-I/usr/include GSL_LIBS=-L/usr/lib/x86_64-linux-gnu -lgsl -lgslcblas -lm
      -------------------------- [ERROR MESSAGE] ---------------------------
      cat configure.log
      --------------------------------------------------------------------
      ])
    fi
  fi
fi

AC_MSG_NOTICE([Creating Makevars with:
GSL_CFLAGS=$GSL_CFLAGS
GSL_LIBS=$GSL_LIBS])

# Now substitute these variables in Makevars.in to create Makevars
AC_SUBST(GSL_CFLAGS)
AC_SUBST(GSL_LIBS)

AC_CONFIG_FILES([Makevars])
AC_OUTPUT
