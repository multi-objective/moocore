# -*- Makefile-gmake -*-
LIBHV_SRCS = hv.c hv3dplus.c hv4d.c hvc3d.c hv_contrib.c
LIBHV_HDRS = hv.h hv_priv.h hv3d_priv.h hv4d_priv.h hvc4d_priv.h libmoocore-config.h
LIBHV_OBJS = $(LIBHV_SRCS:.c=.o)
HV_LIB     = fpli_hv.a

$(HV_LIB): $(LIBHV_OBJS) libutil.o
	@$(RM) $@
	$(QUIET_AR)$(AR) rcs $@ $^

$(LIBHV_OBJS): $(LIBHV_HDRS)
