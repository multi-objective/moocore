# MATLAB API for moocore

DEBUG?=0

# Octave provides mkoctfile, MATLAB provides mex
MEX?=mkoctfile --mex

## Define source files
SRCS =  Hypervolume_MEX.c

S=
## Quiet / verbose output:
MEX_VERBOSE="-v"
ifneq ($(findstring $(MAKEFLAGS),s),s)
  ifdef S
    QUIET_CC   = @echo '   ' CC $@;
    QUIET_CXX  = @echo '   ' CXX $@;
    QUIET_AR   = @echo '   ' AR $@;
    QUIET_LINK = @echo '   ' LINK $@;
    QUIET_RM   = @echo '   ' RM $@;
    QUIET_MEX   = @echo '   ' MEX $@;
    MEX_VERBOSE="--silent"
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

MEX_CFLAGS= -std=c99 -I../c/

OBJS  = $(SRCS:.c=.o)
OBJS := $(OBJS:.cpp=.o)
MEX_FILES:=$(SRCS:.c=.mex)

.PHONY : default all clean

default: $(MEX_FILES)

all: clean
	$(MAKE) default

%.o : %.c
	$(QUIET_MEX)$(MEX) $(MEX_VERBOSE) $(MEX_CFLAGS) $(MEXFLAGS) -c -o $@ $<

include ../c/libhv.mk
LIBHV_SRCS:=$(addprefix ../c/,$(LIBHV_SRCS))

$(MEX_FILES): $(OBJS) $(LIBHV_OBJS)
	$(call ECHO,---> You can use 'make MEXFLAGS="-O -std=c99"' to pass flags to mex. Code compiled by mex is less optimized (slower) than the default 'hv' command-line program <---)
	$(QUIET_MEX)$(MEX) $(MEX_VERBOSE) $(MEX_CFLAGS) $(MEXFLAGS) $^

clean:
	$(MAKE) -C ../c clean
	@-$(RM) $(MEX_FILES) octave-workspace
	$(call ECHO,---> Removing object files: $(OBJS) <---)
	@$(RM) $(OBJS)

test: all
	octave-cli -W -q hypervolume.m
