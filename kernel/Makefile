KERNEL = maya.elf
CC ?= gcc
LD ?= ld
CFLAGS ?= -O2 -g -Wall -Wextra -pipe
# preprocessor (*not* C++)
CPPFLAGS ?= 
NASMFLAGS ?= -F dwarf -g

LDFLAGS ?=
CFLAGS +=  					\
	-I. 					\
	-Isrc/include 			\
	-std=gnu11 				\
	-ffreestanding 			\
	-fno-stack-protector    \
	-fno-stack-check 		\
	-fno-pie 				\
	-fno-pic 				\
	-m64 					\
	-masm=intel				\
	-march=x86-64 			\
	-mabi=sysv 				\
	-mno-80387 				\
	-mno-mmx 				\
	-mno-sse 				\
	-mno-sse2 				\
	-mno-red-zone 			\
	-mcmodel=kernel 		\
	-MMD

LDFLAGS += -nostdlib -static -z max-page-size=0x1000 -T src/linker.ld

NASMFLAGS += -felf64

CFILES := $(shell find src -type f -name '*.c')
ASFILES := $(shell find src -type f -name '*.S')
NASMFILES := $(shell find src -type f -name '*.asm')
OBJ := $(CFILES:.c=.o) $(ASFILES:.S=.o) $(NASMFILES:.asm=.o)
HEADER_DEPS := $(CFILES:.c=.d) $(ASFILES:.S=.d)

.PHONY: all
all: $(KERNEL)

src/include/limine.h:
	curl https://raw.githubusercontent.com/limine-bootloader/limine/master/limine.h -o $@

$(KERNEL): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@ #link initial
	# 'src/gensyms.sh' $@ > symtab.map # export symbol table from kernel
	# $(CC) $(CPPFLAGS) $(CFLAGS) -x c -c symtab.map -o symtab.o # compile symbol table to object
	# $(LD) $(OBJ) symtab.o $(LDFLAGS) -o $@ # link against symbol table once again


-include: $(HEADER_DEPS)

%.o: %.c src/include/limine.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.S stivale2.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm $(NASMFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(KERNEL) $(OBJ) $(HEADER_DEPS)
