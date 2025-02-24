#-----------------------------------------------------------------------

#  Makefile for moocore

VERSION = 0.16.5$(REVISION)

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

DEBUG?=0
BINDIR?=../bin
BINDIR:=$(abspath $(BINDIR))

## Define source files
SRCS =  avl.c                                                                \
        cmdline.c                                                            \
        dominatedsets.c                                                      \
        eaf3d.c                                                              \
        eaf.c                                                                \
        eafdiff.c                                                            \
        eaf_main.c                                                           \
        epsilon.c                                                            \
        hv3dplus.c                                                           \
        hv4d.c                                                               \
        hv.c                                                                 \
        hvapprox.c                                                           \
        hv_contrib.c                                                         \
        igd.c                                                                \
        io.c                                                                 \
        libutil.c                                                            \
        main-hv.c                                                            \
        main-hvapprox.c                                                      \
        mt19937/mt19937.c                                                    \
        ndsort.c                                                             \
        nondominated.c                                                       \
        pareto.c                                                             \
        rng.c                                                                \
        timer.c                                                              \
        whv.c                                                                \
        whv_hype.c

HDRS =  avl.h                                                                \
        avl_tiny.h                                                           \
        bit_array.h                                                          \
        cmdline.h                                                            \
        common.h                                                             \
        config.h                                                             \
        cvector.h                                                            \
        eaf.h                                                                \
        epsilon.h                                                            \
        gcc_attribs.h                                                        \
        hv.h                                                                 \
        hvapprox.h                                                           \
        hv_priv.h                                                            \
        igd.h                                                                \
        io.h                                                                 \
        io_priv.h                                                            \
        mt19937/mt19937.h                                                    \
        nondominated.h                                                       \
        pow_int.h                                                            \
        rng.h                                                                \
        sort.h                                                               \
        timer.h                                                              \
        whv.h                                                                \
        whv_hype.h                                                           \
        ziggurat_constants.h

# Relative to root folder.
DIST_OTHER_FILES =  git_version *.mk
DIST_ROOT_FILES = Makefile README.md LICENSE
OBJS  = $(SRCS:.c=.o)

EXE_LDFLAGS=-lm

include gitversion.mk

# mex
MEX?=mkoctfile --mex

ifneq ($(uname_S),Cygwin)
CPPFLAGS += -D_GNU_SOURCE
else
CPPFLAGS += -U_GNU_SOURCE
endif

EXE_CFLAGS += $(SANITIZERS) $(OPT_CFLAGS) $(MARCH_FLAGS) $(WARN_CFLAGS) \
	-DDEBUG=$(DEBUG) -DVERSION='"$(VERSION)"' -DMARCH='"$(gcc-guess-march)"'

dominatedsetsexe = $(BINDIR)/dominatedsets$(EXE)
eafexe=$(BINDIR)/eaf$(EXE)
epsilonexe = $(BINDIR)/epsilon$(EXE)
hvapproxexe=$(BINDIR)/hvapprox$(EXE)
hvexe=$(BINDIR)/hv$(EXE)
igdexe = $(BINDIR)/igd$(EXE)
ndsortexe = $(BINDIR)/ndsort$(EXE)
nondominatedexe = $(BINDIR)/nondominated$(EXE)
allexes=$(dominatedsetsexe) $(eafexe) $(epsilonexe) $(hvexe) $(hvapproxexe) $(igdexe) $(ndsortexe) $(nondominatedexe)

.PHONY : all clean default dist dominatedsets eaf epsilon hv hvapprox igd ndsort nondominated test time

default: $(allexes)

all: clean
	$(MAKE) default

%.o : %.c
	$(QUIET_CC)$(CC) $(CPPFLAGS) $(EXE_CFLAGS) $(CFLAGS) -c -o $@ $<

dominatedsets: $(dominatedsetsexe)
eaf: $(eafexe)
epsilon: $(epsilonexe)
hv: $(hvexe)
hvapprox: $(hvapproxexe)
igd: $(igdexe)
ndsort: $(ndsortexe)
nondominated: $(nondominatedexe)

include libhv.mk

$(dominatedsetsexe): dominatedsets.o
$(eafexe): eaf.o eaf_main.o eaf3d.o avl.o
$(epsilonexe): epsilon.o
$(hvapproxexe): main-hvapprox.o timer.o hvapprox.o rng.o mt19937/mt19937.o
$(hvexe): main-hv.o timer.o $(LIBHV_OBJS)
$(igdexe): igd.o
$(ndsortexe): ndsort.o pareto.o hv_contrib.o $(LIBHV_OBJS)
$(nondominatedexe): nondominated.o

dominatedsets.o: cmdline.h io.h nondominated.h
eaf_main.o: cmdline.h io.h eaf.h
epsilon.o: cmdline.h io.h epsilon.h nondominated.h
main-hv.o: cmdline.h io.h hv.h timer.h
ndsort.o: cmdline.h io.h nondominated.h
nondominated.o : cmdline.h io.h nondominated.h
main-hvapprox.o: cmdline.h io.h hvapprox.h
hvapprox.o: hvapprox.h pow_int.h
timer.o: timer.h
eaf.o eaf3d.h: eaf.h
cmdline.o: cmdline.h io.h
io.o: io_priv.h io.h
igd.o : cmdline.h io.h igd.h
avl.o: avl.h
$(OBJS): common.h gcc_attribs.h

$(allexes): cmdline.o io.o
	$(call MKDIR, $(BINDIR)/)
	$(call ECHO,--> Building $@ version $(VERSION) <---)
	$(QUIET_LINK)$(CC) -o $@ $^ $(EXE_CFLAGS) $(CFLAGS) $(EXE_LDFLAGS)

clean:
	@-$(RM) config.status config.log
	@-$(RM) Hypervolume_MEX.mex
	@for exe in $(allexes); do \
		echo "---> Removing $$exe <---"; \
		$(RM) $$exe; \
	done
	$(call ECHO,---> Removing object files: $(OBJS) <---)
	@$(RM) $(OBJS) $(HV_OBJS)

TESTSUITE:="../testsuite"
test: OPT_CFLAGS ?=-Og -g3
test: SANITIZERS ?=
test:
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=1 SANITIZERS="$(SANITIZERS)" OPT_CFLAGS="$(OPT_CFLAGS)"
	$(MAKE) test-eaf test-nondominated test-hv test-igd test-epsilon test-dominatedsets

test-eaf test-nondominated test-hv test-igd test-epsilon test-dominatedsets: target=$(subst test-,,$@)
test-eaf test-nondominated test-hv test-igd test-epsilon test-dominatedsets: OPT_CFLAGS ?=-Og -g3
test-eaf test-nondominated test-hv test-igd test-epsilon test-dominatedsets: SANITIZERS ?=
test-eaf test-nondominated test-hv test-igd test-epsilon test-dominatedsets:
	$(MAKE) $(target) DEBUG=1 SANITIZERS="$(SANITIZERS)" OPT_CFLAGS="$(OPT_CFLAGS)"
	$(call ECHO,---> Testing $(target) for errors (DEBUG=1) <--- )
	@cd $(TESTSUITE)/$(target) && ../regtest.py $(BINDIR)/$(target)$(EXE) && cd $(PWD)

time:
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=0 SANITIZERS=""
	$(MAKE) time-eaf time-nondominated time-hv time-igd time-epsilon time-dominatedsets

time-eaf time-nondominated time-hv time-igd time-epsilon time-dominatedsets: target=$(subst time-,,$@)
time-eaf time-nondominated time-hv time-igd time-epsilon time-dominatedsets:
	$(MAKE) $(target) DEBUG=0 SANITIZERS=""
	$(call ECHO,---> Testing $(target) for speed (DEBUG=0) <--- )
	@cd $(TESTSUITE)/$(target) && ../regtest.py $(BINDIR)/$(target)$(EXE) && cd $(PWD)

mex: Hypervolume_MEX.c $(LIBHV_OBJS)
	$(call ECHO,---> You can use 'make mex MEXFLAGS="-O -std=c99"' to pass flags to mex. Code compiled by mex is less optimized (slower) than the default 'hv' command-line program <---)
	$(MEX) -v $(MEXFLAGS) -std=c99 $^

DIST_SRC_FILES = $(DIST_OTHER_FILES) $(OBJS:.o=.c) $(HDRS)
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

include gcc.mk
