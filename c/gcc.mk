# -*- Makefile-gmake -*-
WERROR=
ifdef WERROR
WERROR_FLAG:=-Werror
endif
WARN_CFLAGS = -pedantic -Wall -Wextra -Wvla -Wconversion -Wno-sign-conversion -Wstrict-prototypes $(WERROR_FLAG)
SANITIZERS= -fsanitize=undefined -fsanitize=address
ifeq ($(DEBUG), 0)
  OPT_CFLAGS ?= -DNDEBUG -O3 -flto
# Options -funroll-loops -ffast-math -msse -mfpmath=sse improve performance but are not portable.
# Options -fstandard-precision=fast -ftree-vectorize are not well supported
# in some versions/architectures.
else
  OPT_CFLAGS += -g3 -O0 $(SANITIZERS)
endif

ifdef march
MARCH=$(march)
endif
ifndef MARCH
 MARCH=native
endif
gcc-guess-march = $(strip $(shell $(CC) -march=$(MARCH) $(CFLAGS)  -x c -S -\#\#\# - < /dev/null 2>&1 | \
	 	            grep -m 1 -e cc1 | grep -o -e "march=[^'\"]*" | head -n 1 | sed 's,march=,,'))
ifeq ($(gcc-guess-march),)
gcc-guess-march=unknown
endif
ifneq ($(MARCH),none)
  OPT_CFLAGS += -march=$(MARCH)
endif
