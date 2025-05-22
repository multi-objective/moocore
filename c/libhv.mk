# -*- Makefile-gmake -*-
LIBHV_SRCS    = hv.c hv3dplus.c hv4d.c
LIBHV_HDRS    = hv.h hv_priv.h
LIBHV_OBJS    = $(LIBHV_SRCS:.c=.o)
HV_LIB     = fpli_hv.a

$(HV_LIB): $(LIBHV_OBJS) libutil.o
	@$(RM) $@
	$(QUIET_AR)$(AR) rcs $@ $^

## Dependencies:
$(LIBHV_OBJS): $(LIBHV_HDRS)
