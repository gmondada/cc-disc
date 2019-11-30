
SHELL = bash

include /stm32f4/target/stm32f4discovery/gcc_conf.makefile

SRCS += src/acm.c
SRCS += src/app.c
SRCS += src/cli.c
SRCS += src/cmd.c
SRCS += src/core.c
SRCS += src/easing.c
SRCS += src/gmutil.c
SRCS += src/led.c
SRCS += src/main.c
SRCS += src/mod.c
SRCS += src/ramp.c
SRCS += src/reg.c
SRCS += src/stepper.c
SRCS += src/trace.c
SRCS += src/traj.c
SRCS += src/uart.c
SRCS += src/usb/stm32f4xx_it.c
SRCS += src/usb/usb_bsp.c
SRCS += src/usb/usbd_cdc_vcp.c
SRCS += src/usb/usbd_desc.c
SRCS += src/usb/usbd_usr.c

HDRS += src/acm.h
HDRS += src/app.h
HDRS += src/cli.h
HDRS += src/cmd.h
HDRS += src/core.h
HDRS += src/easing.h
HDRS += src/gmutil.h
HDRS += src/led.h
HDRS += src/mod.h
HDRS += src/ramp.h
HDRS += src/reg.h
HDRS += src/stepper.h
HDRS += src/trace.h
HDRS += src/traj.h
HDRS += src/uart.h

EXECUTABLE = cc-disc
ARCH = arm
OSNAME = stm

BUILDDIR=build/$(OSNAME)-$(ARCH)

F_SRC_TO_OBJ = $(addprefix $(BUILDDIR),$(addsuffix .o,$(basename $(realpath $(1)))))
F_GET_OBJS_BY_EXT = $(call F_SRC_TO_OBJ,$(filter $(1),$(SRCS)))
F_GET_OBJS = $(call F_SRC_TO_OBJ,$(SRCS))


CFLAGS += -std=gnu99 -g -O2 -Wall -fno-strict-aliasing -fwrapv -D__STM32F4DICOVERY__
CFLAGS += -I../libusb/USB_Device/Class/cdc/inc -I../libusb/USB_Device/Core/inc -I../libusb/Conf -I../libusb/USB_OTG/inc
CFLAGS += -Isrc/usb
CFLAGS += $(addprefix -I,$(sort $(dir $(HDRS))))

LDFLAGS += -L../libusb/USB_Device/Core -L../libusb/USB_Device/Class/cdc -L../libusb/USB_OTG
LDFLAGS += -lusbdevcore -lusbdevcdc -lusbcore
LDFLAGS += -Tsrc/stm32_flash.ld

CFLAGS += -DAPP_GIT_VERSION=\"$(shell git describe --abbrev=8 --dirty --always --tags)\"
CFLAGS += -DAPP_BUILD_DATE=\"$(subst $(subst ,, ),\ ,$(shell date '+%Y-%m-%d %H:%M:%S %z'))\"
CFLAGS += -DAPP_BUILD_USER=\"$(USER)\"

OBJS = $(call F_GET_OBJS)

all: $(EXECUTABLE).elf

clean:
	-rm -rf $(BUILDDIR) 2>/dev/null
	-rm $(EXECUTABLE).elf $(EXECUTABLE).bin 2>/dev/null

$(EXECUTABLE).elf: $(OBJS)
	$(CC) $(call F_GET_OBJS) $(LDFLAGS) -Xlinker -Map -Xlinker $(EXECUTABLE).map -o $@
	$(OBJCOPY) -O binary $(EXECUTABLE).elf $(EXECUTABLE).bin

-include $(OBJS:.o=.d)

$(call F_GET_OBJS_BY_EXT,%.c): $(BUILDDIR)/%.o: /%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
	@echo -n $(BUILDDIR)$(dir $<) > $(BUILDDIR)/$*.d.tmp
	@$(CC) -MM $(CFLAGS) $< >> $(BUILDDIR)/$*.d.tmp
	@mv $(BUILDDIR)/$*.d.tmp $(BUILDDIR)/$*.d
