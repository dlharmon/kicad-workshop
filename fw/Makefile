BINARY = kicad-workshop
DEVICE = EFM32HG309F64
OBJS += kicad-workshop.o usb_descriptors.o
CFLAGS += -Os -ggdb3 -Wall -Wextra -std=c++17 -fno-exceptions
CPPFLAGS += -MD $(CFLAGS)
LDFLAGS += -static -nostartfiles
LDLIBS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: clean all flash size

all: $(BINARY).elf size

size: $(BINARY).elf
	size $(BINARY).elf

clean:
	rm -rf *.elf *.hex *.o *.d *~ *.ld

flash: $(BINARY).elf
	openocd -f flash.cfg

debug: $(BINARY).elf
	openocd -f debug.cfg

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
