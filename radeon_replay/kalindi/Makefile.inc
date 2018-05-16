#!/bin/make

KALINDI_SRCS= \
	backlight.c \
	crtc.c \
	dp_aux.c \
	encoder.c \
	init_asic.c \
	init_display_path.c \
	init_misc.c \
	init_coreboot_helpers.c \
	misc.c \
	pll.c \
	transmitter.c

KALINDI_OBJS = $(sort ${KALINDI_SRCS:.c=.o})

KALINDI_TARGET=libkalindi.a


$(KALINDI_TARGET): $(KALINDI_OBJS)
	$(AR) -rcs $@ $^

clean-kalindi:
	-$(RM) $(KALINDI_OBJS) $(KALINDI_TARGET)
