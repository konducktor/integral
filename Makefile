SPEC_FILE ?= in.txt

CC = gcc
ASM = nasm

CFLAGS ?= -O2 -g
CFLAGS += -std=gnu99

CFLAGS += -m32 -no-pie -fno-pie

CFLAGS += -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self \
	-Wswitch-default -Wpointer-arith -Wtype-limits -Wempty-body \
	-Wstrict-prototypes -Wold-style-declaration -Wold-style-definition \
	-Wmissing-parameter-type -Wmissing-field-initializers -Wnested-externs \
	-Wstack-usage=4096 -Wmissing-prototypes -Wfloat-equal -Wabsolute-value

CFLAGS += -fsanitize=undefined -fsanitize-undefined-trap-on-error

LDLIBS = -lm

BISECTION ?= 0
ifeq ($(BISECTION), 1)
    METHOD_FLAGS = -DMETHOD_BISECTION
else
    METHOD_FLAGS =
endif

.PHONY: all
all: integral

# NORMAL NORMAL NORMAL

integral: integral.c functions.o
	$(CC) $(CFLAGS) $(METHOD_FLAGS) -o $@ $^ $(LDLIBS)

# CUSTOM CUSTOM CUSTOM

functions.o: functions.asm
	$(ASM) -f elf32 $< -o $@

assembly-assembler: assembly-assembler.c
	$(CC) $(CFLAGS) -o $@ $<

functions-custom.asm: assembly-assembler
	SPEC_FILE="$(SPEC_FILE)" ./assembly-assembler
	test -f $@

functions-custom.o: functions-custom.asm
	$(ASM) -f elf32 $< -o $@

.PHONY: custom
custom: assembly-assembler
	SPEC_FILE="$(SPEC_FILE)" ./assembly-assembler
	$(ASM) -f elf32 functions-custom.asm -o functions-custom.o
	$(CC) $(CFLAGS) $(METHOD_FLAGS) -DCUSTOM_INPUT_FILE=\"$(SPEC_FILE)\" -o integral integral.c functions-custom.o $(LDLIBS)

# OTHER OTHER OTHER

.PHONY: test
test: integral
	./integral -R 1:2:1.0:1.5:0.001:1.344
	./integral -R 2:3:0.5:1.0:0.001:0.826
	./integral -R 1:3:-1.5:-1.0:0.001:-1.308
	./integral -I 1:-1.308:1.344:0.001:10.048
	./integral -I 2:0.826:1.344:0.001:0.698
	./integral -I 3:-1.308:0.826:0.001:2.758

.PHONY: clean
clean:
	rm -f *.o integral assembly-assembler functions-custom.asm
