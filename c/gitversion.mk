## Do we have git?
ifeq ($(shell sh -c 'which git 1> /dev/null 2>&1 && echo y'),y)
  ## Is this a working copy?
  ifeq ($(shell sh -c "LC_ALL=C git rev-parse --is-inside-work-tree 2>&1 | grep -F true"),true)
     $(shell sh -c "git describe --dirty --first-parent --always --exclude '*' > git_version")
  endif
endif
## Set version information:
REVISION=.$(shell sh -c 'cat git_version 2> /dev/null')
