PTYPE=__SAMD21G18A__

CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AS=arm-none-eabi-as

ELF=$(notdir $(CURDIR)).elf


ASF_ROOT=/home/sandra/xdk-asf-3.42.0
LDSCRIPT = $(ASF_ROOT)/sam0/utils/linker_scripts/samd21/gcc/samd21g18a_flash.ld

INCLUDES= \
          sam0/utils/cmsis/samd21/include \
          sam0/utils/cmsis/samd21/source \
          thirdparty/CMSIS/Include \
          thirdparty/CMSIS/Lib/GC

OBJS = startup_samd21.o system_samd21.o myprintf.o main.o

LDFLAGS+= -T$(LDSCRIPT) -mthumb -mcpu=cortex-m0 -Wl,--gc-sections
CFLAGS+= -mcpu=cortex-m0 -mthumb -g
CFLAGS+= $(INCLUDES:%=-I $(ASF_ROOT)/%) -I .
CFLAGS+= -D$(PTYPE)
CFLAGS+=-pipe -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration \
-Wpointer-arith -std=gnu99 -fno-strict-aliasing -ffunction-sections -fdata-sections \
-Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point \
-Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal \
-Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return \
-Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations \
-Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align \
--param max-inline-insns-single=500

$(ELF):     $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# compile and generate dependency info

%.o:    %.c
	$(CC) -c $(CFLAGS) $< -o $@
	$(CC) -MM $(CFLAGS) $< > $*.d

info:       
	@echo CFLAGS=$(CFLAGS)
	@echo OBJS=$(OBJS)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(ELF) $(CLEANOTHER)

debug:  $(ELF)
	arm-none-eabi-gdb -iex "target extended-remote localhost:3333" $(ELF)

-include $(OBJS:.o=.d)


