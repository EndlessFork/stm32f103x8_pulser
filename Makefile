BINARY = main
OPENCM3_DIR = libopencm3
LDSCRIPT = stm32f103c8.ld
#LDSCRIPT = stm32f103c8_RAM.ld
OOCD_INTERFACE = stlink-v2-1
OOCD_BOARD = st_nucleo_f103rb


LIBNAME         = opencm3_stm32f1
DEFS            += -DSTM32F1

FP_FLAGS        ?= -msoft-float
ARCH_FLAGS      = -mthumb -mcpu=cortex-m3 $(FP_FLAGS) -mfix-cortex-m3-ldrd

STLINK_PORT    ?= :4242

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

include Makefile.include

# compile all .c files for our project
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

flash: $(BINARY).bin
        # RAM@0x20000000, FLASH@0x08000000
	stlink/st-flash write $(BINARY) 0x08000000

lss: list

size: $(BINARY).elf
	@arm-none-eabi-size -A -x $(BINARY).elf | python sizer.py


init:
	git submodule init
	git submodule update
	find . -maxdepth 1 -type d -exec make -C \{\} \;
	cd stlink
	bash -c ./autogen.sh
	bash -c ./configure
	make
	cd -

test: test.cc
	gcc -o test test.cc
	./test
	python hist.py pattern_*.dat
