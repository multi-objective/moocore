#-----------------------------------------------------------------------

#  Makefile for moocore

VERSION = 0.1.9999$(REVISION)

#-----------------------------------------------------------------------

#                        Copyright (c) 2007-2023
#           Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

#  This program is free software (software libre); you can redistribute
#  it and/or modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2 of the
#  License, or (at your option) any later version.

#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you can obtain a copy of the GNU
#  General Public License at:
#                  http://www.gnu.org/copyleft/gpl.html
#  or by writing to:
#            Free Software Foundation, Inc., 59 Temple Place,
#                  Suite 330, Boston, MA 02111-1307 USA
#
#-----------------------------------------------------------------------
# IMPORTANT NOTE: Please be aware that the fact that this program is
# released as Free Software does not excuse you from scientific
# propriety, which obligates you to give appropriate credit! If you
# write a scientific paper describing research that made substantive
# use of this program, it is your obligation as a scientist to
# acknowledge its use.  Moreover, as a personal note, I would
# appreciate it if you would email manuel.lopez-ibanez@manchester.ac.uk with
# citations of papers referencing this work so I can mention them to
# my funding agent and tenure committee.
#-----------------------------------------------------------------------

S=
## Quiet / verbose output:
ifneq ($(findstring $(MAKEFLAGS),s),s)
  ifdef S
    QUIET_CC   = @echo '   ' CC $@;
    QUIET_AR   = @echo '   ' AR $@;
    QUIET_LINK = @echo '   ' LINK $@;
    QUIET_RM   = @echo '   ' RM $@;
    ECHO       =
  else
    ECHO       = @echo "$(1)"
  endif
endif

# ifeq ($(OS),Windows_NT)
#   uname_S:=Windows
#   ECHO = @echo $(1)
#   RM=del /Q
#   MKDIR= @mkdir $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
#   -include Makevars.win
# else
#   uname_S := $(shell gcc -dumpmachine 2>&1)
#   uname_S := $(subst MINGW,mingw,$(uname_S))
#   ifneq (,$(findstring mingw,$(uname_S)))
#     MKDIR= @mkdir $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
#     RM=del /Q
#     uname_S:=mingw
#     ECHO = @echo $(1)
#   else
#     ## Detect system and machine type:
#     uname_S :=$(shell uname -s || echo unknown)
#     uname_M :=$(shell uname -m || echo unknown)
#     ECHO = @echo "$(1)"
#    include Makevars
#   endif
# endif
MKDIR= @mkdir -p $(1)

-include Makevars

DEBUG?=0
BINDIR?=../bin
BINDIR:=$(abspath $(BINDIR))

## Define source files
SRCS  = igd.c epsilon.c dominatedsets.c nondominated.c io.c ndsort.c hv_contrib.c hv.c pareto.c whv.c whv_hype.c \
	eaf.c eafdiff.c eaf_main.c eaf3d.c avl.c cmdline.c libutil.c main-hv.c timer.c
HEADERS = io.h io_priv.h common.h gcc_attribs.h igd.h epsilon.h nondominated.h hv.h cmdline.h whv.h whv_hype.h \
	eaf.h cvector.h avl.h bit_array.h
# Relative to root folder.
DIST_OTHER_FILES =  git_version *.mk configure.ac configure
DIST_ROOT_FILES = Makefile README.md LICENSE
OBJS  = $(SRCS:.c=.o)

# We do not need $(GSL_LIBS) yet because we don't compile whv_hype.c into any executable.
EXE_LDFLAGS=-lm

include gitversion.mk
include gcc.mk

MEX?=mkoctfile --mex # mex

ifneq ($(uname_S),Cygwin)
CPPFLAGS += -D_GNU_SOURCE
else
CPPFLAGS += -U_GNU_SOURCE
endif

EXE_CFLAGS += $(OPT_CFLAGS) $(GSL_CFLAGS) $(WARN_CFLAGS) \
	-DDEBUG=$(DEBUG) -DVERSION='"$(VERSION)"' -DMARCH='"$(gcc-guess-march)"'

igd = $(BINDIR)/igd$(EXE)
epsilon = $(BINDIR)/epsilon$(EXE)
dominatedsets = $(BINDIR)/dominatedsets$(EXE)
nondominated = $(BINDIR)/nondominated$(EXE)
ndsort = $(BINDIR)/ndsort$(EXE)
eafexe=$(BINDIR)/eaf$(EXE)
hvexe=$(BINDIR)/hv$(EXE)


.PHONY : all clean dist default check test eaf hv

default: $(igd) $(epsilon) $(dominatedsets) $(nondominated) $(ndsort) $(eafexe) $(hvexe)

eaf: $(eafexe)

hv: $(hvexe)

include libhv.mk

$(igd): OBJS=igd.o io.o cmdline.o
$(igd): HEADERS=igd.h nondominated.h cmdline.h

$(epsilon): OBJS=epsilon.o io.o cmdline.o
$(epsilon): HEADERS=epsilon.h nondominated.h cmdline.h

$(dominatedsets): OBJS=dominatedsets.o io.o cmdline.o
$(dominatedsets): HEADERS=nondominated.h cmdline.h

$(nondominated): OBJS=nondominated.o io.o cmdline.o
$(nondominated): HEADERS= epsilon.h nondominated.h cmdline.h

$(eafexe): OBJS=eaf.o io.o eaf_main.o eaf3d.o avl.o cmdline.o
$(eafexe): HEADERS=eaf.h

$(ndsort): OBJS=ndsort.o pareto.o io.o hv_contrib.c $(LIBHV_OBJS) cmdline.o
$(ndsort): HEADERS=nondominated.h cmdline.h


$(hvexe): OBJS=main-hv.o io.o timer.o cmdline.o $(LIBHV_OBJS)
$(hvexe): HEADERS=hv.h timer.h

io.o: io_priv.h io.h

$(igd) $(epsilon) $(nondominated) $(dominatedsets) $(ndsort) $(eafexe) $(hvexe): $(OBJS) $(HEADERS)
	$(call MKDIR, $(BINDIR)/)
	$(call ECHO,--> Building $@ version $(VERSION) <---)
	$(QUIET_LINK)$(CC) -o $@  $(OBJS) $(EXE_CFLAGS) $(CFLAGS) $(EXE_LDFLAGS)

$(OBJS): $(HEADERS) common.h gcc_attribs.h

clean:
	@-$(RM) config.status config.log
	@-$(RM) Hypervolume_MEX.mex

	$(call ECHO,---> Removing $(igd) <---)
	@$(RM) $(igd)

	$(call ECHO,---> Removing $(epsilon) <---)
	@$(RM) $(epsilon)

	$(call ECHO,---> Removing $(dominatedsets) <---)
	@$(RM) $(dominatedsets)

	$(call ECHO,---> Removing $(nondominated) <---)
	@$(RM) $(nondominated)

	$(call ECHO,---> Removing $(ndsort) <---)
	@$(RM) $(ndsort)

	$(call ECHO,---> Removing $(eafexe) <---)
	@$(RM) $(eafexe)

	$(call ECHO,---> Removing $(hvexe) <---)
	@$(RM) $(hvexe)

	$(call ECHO,---> Removing object files: $(OBJS) <---)
	@$(RM) $(OBJS) $(HV_OBJS)

TESTSUITE:="../testsuite"
check: clean
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=1 SANITIZERS=""
	$(MAKE) check-eaf
	$(MAKE) check-hv

check-eaf:
	$(MAKE) eaf DEBUG=1 SANITIZERS=""
	$(call ECHO,---> Checking $(eafexe) for errors (DEBUG=1) <--- )
	@cd $(TESTSUITE)/eaf && ../regtest.py $(eafexe) && cd $(PWD)

check-hv:
	$(MAKE) $(hvexe) DEBUG=1 SANITIZERS=""
	$(call ECHO,---> Checking $(hvexe) for errors (DEBUG=1) <--- )
	@cd $(TESTSUITE)/hv && ../regtest.py $(hvexe) && cd $(PWD)

test: clean
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=0 SANITIZERS=""
	$(MAKE) test-eaf
	$(MAKE) test-hv

test-eaf:
	$(MAKE) eaf DEBUG=0 SANITIZERS=""
	$(call ECHO,---> Testing $(eafexe) for speed (DEBUG=0) <--- )
	@cd $(TESTSUITE)/eaf && ../regtest.py $(eafexe) && cd $(PWD)

test-hv:
	$(MAKE) $(hvexe) DEBUG=0 SANITIZERS=""
	$(call ECHO,---> Testing $(hvexe) for speed (DEBUG=0) <--- )
	@cd $(TESTSUITE)/hv && ../regtest.py $(hvexe) && cd $(PWD)

all: clean
	$(MAKE) default

%.o : %.c
	$(QUIET_CC)$(CC) $(CPPFLAGS) $(EXE_CFLAGS) $(CFLAGS) -c -o $@ $<

mex: MEX?=mex
mex: Hypervolume_MEX.c $(HV_SRCS)
	$(call ECHO,---> You can use 'make mex MEXFLAGS="-O -std=c99"' to pass flags to mex. Code compiled by MEX is less optimized (slower) than the default 'hv' command-line program <---)
	$(MEX) -v $(MEXFLAGS) -std=c99 $^

DIST_SRC_FILES = $(DIST_OTHER_FILES) $(OBJS:.o=.c) $(HEADERS)
DIST_SRC:= moocore-$(VERSION)-src
dist: DEBUG=0
dist: CDEBUG=
dist: all
	$(call MKDIR,../$(DIST_SRC)
	@(rsync -rlpC --exclude=.svn $(DIST_SRC_FILES) ../$(DIST_SRC)/ \
	&& cd .. \
	&& tar cf - $(DIST_SRC) | gzip -f9 > $(DIST_SRC).tar.gz \
	&& rm -rf ./$(DIST_SRC)/* && rmdir ./$(DIST_SRC)/ \
	&& echo "$(DIST_SRC).tar.gz created." && cd $(PWD) )

TAGS: *.c *.h
	etags *.c *.h
