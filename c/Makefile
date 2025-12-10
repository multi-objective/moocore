#-----------------------------------------------------------------------
#  Makefile for moocore
#-----------------------------------------------------------------------
VERSION = 0.18$(REVISION)

S=
## Quiet / verbose output:
ifneq ($(findstring $(MAKEFLAGS),s),s)
  ifdef S
    QUIET_CC   = @echo '   ' CC $@;
    QUIET_CXX  = @echo '   ' CXX $@;
    QUIET_AR   = @echo '   ' AR $@;
    QUIET_LINK = @echo '   ' LINK $@;
    QUIET_RM   = @echo '   ' RM $@;
    ECHO       =
  else
    ECHO       = @echo "$(1)"
  endif
endif

ifeq ($(OS),Windows_NT)
  uname_S:=Windows
else
  uname_S:=$(shell uname -s || echo unknown)
endif
$(info Building on '$(uname_S)')

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
        hvc3d.c                                                              \
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
        hv3d_priv.h                                                          \
        hv4d_priv.h                                                          \
        igd.h                                                                \
        io.h                                                                 \
        io_priv.h                                                            \
        libmoocore-config.h	                                             \
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
OBJS := $(OBJS:.cpp=.o)

LDLIBS=-lm
EXE_LDFLAGS=$(LDLIBS)

include gitversion.mk

ifneq ($(uname_S),Cygwin)
CPPFLAGS += -D_GNU_SOURCE
else
CPPFLAGS += -U_GNU_SOURCE
endif

SHLIB_CFLAGS = -fvisibility=hidden -DMOOCORE_SHARED_LIB $(OPT_CFLAGS) $(MARCH_FLAGS)
SHLIB_LDFLAGS = -shared

# Platform-specific linker flags
ifeq ($(uname_S),Linux)
  SHLIB_CFLAGS += -fPIC
  SHLIB_LDFLAGS += $(LDLIBS) -Wl,--no-undefined
  SHLIB_EXT=so
endif
ifeq ($(uname_S),Darwin)
  SHLIB_CFLAGS += -fPIC
  SHLIB_LDFLAGS += $(LDLIBS)
  SHLIB_EXT=dylib
endif
ifeq ($(uname_S),Windows)
  # No LTO for Windows.
  OPT_CFLAGS = -DNDEBUG -O2
  MARCH_FLAGS=
  SHLIB_EXT=dll
  SHLIB_LDFLAGS += -fuse-ld=lld
endif

EXE_CFLAGS += $(SANITIZERS) $(OPT_CFLAGS) $(MARCH_FLAGS) $(WARN_CFLAGS) \
	-DDEBUG=$(DEBUG) -DVERSION='"$(VERSION)"' -DMARCH='"$(gcc-guess-march)"'


# List of test target base names
TEST_NAMES := eaf nondominated hv hvapprox igd epsilon dominatedsets ndsort
TEST_TARGETS := $(addprefix test-,$(TEST_NAMES))
TIME_TARGETS := $(addprefix time-,$(TEST_NAMES))
ALL_NAMES := $(TEST_NAMES)

EXE_FILES := $(addsuffix $(EXE), $(addprefix $(BINDIR)/,$(ALL_NAMES)))

SHLIB=libmoocore.$(SHLIB_EXT)
CXXSHLIB=libmoocorepp.$(SHLIB_EXT)

.PHONY : all clean default dist $(ALL_NAMES) test time shlibs

default: $(EXE_FILES)

all: clean
	$(MAKE) default

%.o : %.cpp
	$(QUIET_CXX)$(CXX) $(CPPFLAGS) $(EXE_CFLAGS) $(CFLAGS) -c -o $@ $<

%.o : %.c
	$(QUIET_CC)$(CC) $(CPPFLAGS) $(EXE_CFLAGS) $(CFLAGS) -c -o $@ $<

$(ALL_NAMES): %: $(BINDIR)/%$(EXE)

include libhv.mk

$(BINDIR)/dominatedsets$(EXE): dominatedsets.o
$(BINDIR)/eaf$(EXE): eaf.o eaf_main.o eaf3d.o avl.o
$(BINDIR)/epsilon$(EXE): epsilon.o
$(BINDIR)/hv$(EXE): main-hv.o timer.o $(LIBHV_OBJS)
$(BINDIR)/hvapprox$(EXE): main-hvapprox.o timer.o hvapprox.o rng.o mt19937/mt19937.o
$(BINDIR)/igd$(EXE): igd.o
$(BINDIR)/ndsort$(EXE): ndsort.o pareto.o $(LIBHV_OBJS)
$(BINDIR)/nondominated$(EXE): nondominated.o timer.o

$(EXE_FILES): cmdline.o io.o
	$(call MKDIR, $(BINDIR)/)
	$(call ECHO,--> Building $@ version $(VERSION) <---)
	$(QUIET_LINK)$(CC) -o $@ $^ $(EXE_CFLAGS) $(CFLAGS) $(EXE_LDFLAGS) $(LDFLAGS)

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

shlibs: clean $(SHLIB) $(CXXSHLIB)

$(SHLIB): CFLAGS += $(SHLIB_CFLAGS)
$(SHLIB): $(LIBHV_OBJS)
	$(QUIET_CC)$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(SHLIB_LDFLAGS)

$(CXXSHLIB): CFLAGS += $(SHLIB_CFLAGS)
$(CXXSHLIB): $(LIBHV_OBJS)
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(SHLIB_LDFLAGS)

clean:
	@-$(RM) *.s *.i *.gcda mt19937/*.gcda
	@-$(RM) config.status config.log
	@-$(RM) $(SHLIB) $(CXXSHLIB)
	@for exe in $(EXE_FILES); do \
		echo "---> Removing $$exe <---"; \
		$(RM) $$exe; \
	done
	$(call ECHO,---> Removing object files: $(OBJS) <---)
	@$(RM) $(OBJS) $(HV_OBJS) $(HV_LIB)

TESTSUITE:="../testsuite"
test: OPT_CFLAGS ?=-Og -g3
test: SANITIZERS ?=
test:
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=1 SANITIZERS="$(SANITIZERS)" OPT_CFLAGS="$(OPT_CFLAGS)"
	$(MAKE) $(TEST_TARGETS)

$(TEST_TARGETS): target=$(subst test-,,$@)
$(TEST_TARGETS): OPT_CFLAGS ?=-Og -g3
$(TEST_TARGETS): SANITIZERS ?=
$(TEST_TARGETS):
	$(MAKE) $(target) DEBUG=1 SANITIZERS="$(SANITIZERS)" OPT_CFLAGS="$(OPT_CFLAGS)"
	$(call ECHO,---> Testing $(target) for errors (DEBUG=1) <--- )
	@cd $(TESTSUITE)/$(target) && ../regtest.py $(BINDIR)/$(target)$(EXE) && cd $(PWD)

time:
	@if ! test -d $(TESTSUITE); then  		    	         \
	    echo "Error: Testsuite not found in $(TESTSUITE)" && exit 1; \
	fi
	$(MAKE) all DEBUG=0 SANITIZERS=""
	$(MAKE) $(TIME_TARGETS)

$(TIME_TARGETS): target=$(subst time-,,$@)
$(TIME_TARGETS):
	$(MAKE) $(target) DEBUG=0 SANITIZERS=""
	$(call ECHO,---> Testing $(target) for speed (DEBUG=0) <--- )
	@cd $(TESTSUITE)/$(target) && ../regtest.py $(BINDIR)/$(target)$(EXE) && cd $(PWD)

DIST_SRC_FILES = $(DIST_OTHER_FILES) $(SRCS) $(HDRS)
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
