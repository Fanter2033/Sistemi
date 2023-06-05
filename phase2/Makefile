# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX = mipsel-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld

# uMPS3-related paths

# Simplistic search for the umps3 installation prefix.
# If you have umps3 installed on some weird location, set UMPS3_DIR_PREFIX by hand.
ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
else
	UMPS3_DIR_PREFIX = /usr/local
endif

UMPS3_DATA_DIR = $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_INCLUDE_DIR = $(UMPS3_DIR_PREFIX)/include/umps3
UMPS3_TYPE_DIR = ./h/
UMPS3_PHASE1_DIR = ./phase1/

# Compiler options
CFLAGS_LANG = -ffreestanding -ansi
CFLAGS_MIPS = -mips1 -mabi=32 -mno-gpopt -EL -G 0 -mno-abicalls -fno-pic -mfp32 -Wall
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_MIPS) -I$(UMPS3_INCLUDE_DIR) -I$(UMPS3_TYPE_DIR) -I$(UMPS3_PHASE1_DIR) -O0 -std=gnu99

# Linker options
LDFLAGS = -G 0 -T $(UMPS3_DATA_DIR)/umpscore.ldscript -m elf32ltsmip

# Add the location of crt*.S to the search path
VPATH = $(UMPS3_DATA_DIR)

.PHONY : all clean

all : ./kernel/kernel.core.umps

./kernel/kernel.core.umps : ./kernel/kernel
	umps3-elf2umps -k $<

./kernel/kernel : initial.o scheduler.o exceptions.o interrupts.o p2test.04.o ./phase1/pcb.o ./phase1/ash.o ./phase1/ns.o crtso.o libumps.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	-rm -f *.o kernel kernel.*.umps

# Pattern rule for assembly modules
%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<
