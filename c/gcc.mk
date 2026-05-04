# -*- Makefile-gmake -*-
DEFAULT_UBSAN_FLAGS=-fsanitize=undefined,float-cast-overflow,float-divide-by-zero,bounds  -fno-omit-frame-pointer -fno-common
DEFAULT_ASAN_FLAGS=-fsanitize=address,pointer-subtract -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-common
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2:abort_on_error=1

WERROR=
ifdef WERROR
WERROR_FLAG:=-Werror
endif
WARN_CFLAGS = -pedantic -Wall -Wextra -Wvla -Wconversion -Wno-sign-conversion -Wstrict-prototypes -Wundef $(WERROR_FLAG)
ifeq ($(DEBUG), 0)
  SANITIZERS ?=
  OPT_CFLAGS ?= -DNDEBUG -O3 -flto
  # GCC options that improve performance:
  # -ffinite-math-only -fno-signed-zeros
  # GCC options that may or may not improve performance:
  # -ftree-loop-im -ftree-loop-ivcanon -fivopts -ftree-vectorize -funroll-loops -fipa-pta
  #
else
  SANITIZERS ?= $(DEFAULT_ASAN_FLAGS) # With DEBUG=1, use ASAN by default.
  OPT_CFLAGS ?= -g3 -O0
endif

ifdef march
  MARCH=$(march)
endif
ifndef MARCH
  MARCH=native
endif
ifneq ($(MARCH),none)
 MARCH_FLAGS = -march=$(MARCH)
 # GCC prints -mcpu=CPU in MacOS and -march=CPU otherwise. Clang prints "-target-cpu" "CPU".
 gcc-guess-march = $(strip $(shell $(CC) $(CFLAGS) $(OPT_CFLAGS) $(MARCH_FLAGS) -x c -S -\#\#\# - < /dev/null 2>&1 | \
                            sed -n '/cc1/{s/.*-march=\([^"'"'"' ]*\).*/\1/p;s/.*-target-cpu" "\([^"]*\)".*/\1/p;s/.*-mcpu=\([^"'"'"' ]*\).*/\1/p;q;}'))
 ifeq ($(gcc-guess-march),)
   gcc-guess-march=unknown
 endif
endif
