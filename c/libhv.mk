# -*- Makefile-gmake -*-
LIBHV_SRCS    = hv.c
LIBHV_HDRS    = hv.h
LIBHV_OBJS    = $(LIBHV_SRCS:.c=.o)
HV_LIB     = fpli_hv.a

$(HV_LIB): $(LIBHV_OBJS) libutil.o
	@$(RM) $@
	$(QUIET_AR)$(AR) rcs $@ $^

## Dependencies:
$(LIBHV_OBJS): $(LIBHV_HDRS)
