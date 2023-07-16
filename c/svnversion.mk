## Do we have svnversion?
ifeq ($(shell sh -c 'which svnversion 1> /dev/null 2>&1 && echo y'),y)
  ## Is this a working copy?
  ifeq ($(shell sh -c 'LC_ALL=C svnversion -n . | grep -q ^[0-9] && echo y'),y)
    $(shell sh -c 'svnversion -n . > svn_version')
  endif
endif
## Set version information:
REVISION = $(shell sh -c 'cat svn_version 2> /dev/null')
