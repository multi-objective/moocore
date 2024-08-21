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

S=1
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

DEBUG?=0
BINDIR?=../bin
BINDIR:=$(abspath $(BINDIR))

## Define source files
SRCS  = igd.c epsilon.c dominatedsets.c nondominated.c io.c ndsort.c hv_contrib.c hv.c pareto.c whv.c whv_hype.c \
	eaf.c eafdiff.c eaf_main.c eaf3d.c avl.c cmdline.c libutil.c main-hv.c timer.c \
	rng.c mt19937/mt19937.c
HEADERS = io.h io_priv.h common.h gcc_attribs.h igd.h epsilon.h nondominated.h hv.h cmdline.h whv.h whv_hype.h \
	eaf.h cvector.h avl.h bit_array.h \
	rng.h ziggurat_constants.h mt19937/mt19937.h

# Relative to root folder.
DIST_OTHER_FILES =  git_version *.mk
DIST_ROOT_FILES = Makefile README.md LICENSE
OBJS  = $(SRCS:.c=.o)

EXE_LDFLAGS=-lm

include gitversion.mk
include gcc.mk

# mex
MEX?=mkoctfile --mex

ifneq ($(uname_S),Cygwin)
CPPFLAGS += -D_GNU_SOURCE
else
CPPFLAGS += -U_GNU_SOURCE
endif

EXE_CFLAGS += $(OPT_CFLAGS) $(WARN_CFLAGS) \
	-DDEBUG=$(DEBUG) -DVERSION='"$(VERSION)"' -DMARCH='"$(gcc-guess-march)"'

igdexe = $(BINDIR)/igd$(EXE)
epsilonexe = $(BINDIR)/epsilon$(EXE)
dominatedsetsexe = $(BINDIR)/dominatedsets$(EXE)
nondominatedexe = $(BINDIR)/nondominated$(EXE)
ndsortexe = $(BINDIR)/ndsort$(EXE)
eafexe=$(BINDIR)/eaf$(EXE)
hvexe=$(BINDIR)/hv$(EXE)


.PHONY : all clean dist default test time eaf hv nondominated

default: $(igdexe) $(epsilonexe) $(dominatedsetsexe) $(nondominatedexe) $(ndsortexe) $(eafexe) $(hvexe)

igd: $(igdexe)
epsilon: $(epsilonexe)
dominatedsets: $(dominatedsetsexe)
nondominated: $(nondominatedexe)
ndsort: $(ndsortexe)
eaf: $(eafexe)
hv: $(hvexe)

include libhv.mk

$(igdexe): OBJS=igd.o io.o cmdline.o
$(igdexe): HEADERS=igd.h nondominated.h cmdline.h

$(epsilonexe): OBJS=epsilon.o io.o cmdline.o
$(epsilonexe): HEADERS=epsilon.h nondominated.h cmdline.h

$(dominatedsetsexe): OBJS=dominatedsets.o io.o cmdline.o
$(dominatedsetsexe): HEADERS=nondominated.h cmdline.h

$(nondominatedexe): OBJS=nondominated.o io.o cmdline.o
$(nondominatedexe): HEADERS= epsilon.h nondominated.h cmdline.h

$(eafexe): OBJS=eaf.o io.o eaf_main.o eaf3d.o avl.o cmdline.o
$(eafexe): HEADERS=eaf.h

$(ndsortexe): OBJS=ndsort.o pareto.o io.o hv_contrib.c $(LIBHV_OBJS) cmdline.o
$(ndsortexe): HEADERS=nondominated.h cmdline.h

$(hvexe): OBJS=main-hv.o io.o timer.o cmdline.o $(LIBHV_OBJS)
$(hvexe): HEADERS=hv.h timer.h

io.o: io_priv.h io.h

$(igdexe) $(epsilonexe) $(nondominatedexe) $(dominatedsetsexe) $(ndsortexe) $(eafexe) $(hvexe): $(OBJS) $(HEADERS)
	$(call MKDIR, $(BINDIR)/)
	$(call ECHO,--> Building $@ version $(VERSION) <---)
	$(QUIET_LINK)$(CC) -o $@  $(OBJS) $(EXE_CFLAGS) $(CFLAGS) $(EXE_LDFLAGS)

$(OBJS): $(HEADERS) common.h gcc_attribs.h

clean:
	@-$(RM) config.status config.log
	@-$(RM) Hypervolume_MEX.mex

	@for exe in $(igdexe) $(epsilonexe) $(dominatedsetsexe) $(nondominatedexe) $(ndsortexe) $(eafexe) $(hvexe); do \
		echo "---> Removing $$exe <---"; \
		$(RM) $$exe; \
	done

	$(call ECHO,---> Removing object files: $(OBJS) <---)
	@$(RM) $(OBJS) $(HV_OBJS)

TESTSUITE:="../testsuite"
test: clean
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=1 SANITIZERS=""
	$(MAKE) test-nondominated test-eaf test-hv

test-nondominated:
	$(MAKE) $(nondominatedexe) DEBUG=1 SANITIZERS=""
	$(call ECHO,---> Testing $(nondominatedexe) for errors (DEBUG=1) <--- )
	@cd $(TESTSUITE)/nondominated && ../regtest.py $(nondominatedexe) && cd $(PWD)

test-eaf:
	$(MAKE) eaf DEBUG=1 SANITIZERS=""
	$(call ECHO,---> Testing $(eafexe) for errors (DEBUG=1) <--- )
	@cd $(TESTSUITE)/eaf && ../regtest.py $(eafexe) && cd $(PWD)

test-hv:
	$(MAKE) $(hvexe) DEBUG=1 SANITIZERS=""
	$(call ECHO,---> Testing $(hvexe) for errors (DEBUG=1) <--- )
	@cd $(TESTSUITE)/hv && ../regtest.py $(hvexe) && cd $(PWD)

time: clean
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=0 SANITIZERS=""
	$(MAKE) time-nondominated time-eaf time-hv

time-nondominated:
	$(MAKE) $(nondominatedexe) DEBUG=0 SANITIZERS=""
	$(call ECHO,---> Testing $(nondominatedexe) for speed (DEBUG=0) <--- )
	@cd $(TESTSUITE)/nondominated && ../regtest.py $(nondominatedexe) && cd $(PWD)

time-eaf:
	$(MAKE) eaf DEBUG=0 SANITIZERS=""
	$(call ECHO,---> Testing $(eafexe) for speed (DEBUG=0) <--- )
	@cd $(TESTSUITE)/eaf && ../regtest.py $(eafexe) && cd $(PWD)

time-hv:
	$(MAKE) $(hvexe) DEBUG=0 SANITIZERS=""
	$(call ECHO,---> Testing $(hvexe) for speed (DEBUG=0) <--- )
	@cd $(TESTSUITE)/hv && ../regtest.py $(hvexe) && cd $(PWD)

all: clean
	$(MAKE) default

%.o : %.c
	$(QUIET_CC)$(CC) $(CPPFLAGS) $(EXE_CFLAGS) $(CFLAGS) -c -o $@ $<

mex: Hypervolume_MEX.c $(LIBHV_OBJS)
	$(call ECHO,---> You can use 'make mex MEXFLAGS="-O -std=c99"' to pass flags to mex. Code compiled by mex is less optimized (slower) than the default 'hv' command-line program <---)
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
