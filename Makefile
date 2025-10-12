NASM = nasm
GCC  = gcc
LD   = ld
OBJCOPY = objcopy
AR   = ar
DD   = dd
VBX  = VBoxManage
MKFS = mkfs.fat
MCOPY = mcopy

STAGE1 = stage1.asm
STAGE2 = stage2.asm
PARSER_SRC = parser.c
KERNEL_SRC_DIR = kernel/src
KERNEL_INC_DIR = kernel/include
KERNEL_SRC = $(wildcard $(KERNEL_SRC_DIR)/*.c)
KERNEL_OBJ = $(KERNEL_SRC:.c=.o)
LIBK_SRC_DIR = libk/src
LIBK_INC_DIR = libk/include
LIBK_SRC = $(wildcard $(LIBK_SRC_DIR)/*.c)
LIBK_OBJ = $(LIBK_SRC:.c=.o)
DRIVERS_SRC_DIR = drivers/src
DRIVERS_INC_DIR = drivers/include
DRIVERS_SRC = $(wildcard $(DRIVERS_SRC_DIR)/*.c)
DRIVERS_OBJ = $(DRIVERS_SRC:.c=.o)
LIBK = libk.a

STAGE1_BIN = stage1.bin
STAGE2_BIN = stage2.bin
PARSER_ELF = parser.elf
PARSER_BIN = parser.bin
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin
IMG        = os.img
VDI        = os.vdi

SECTOR_SIZE = 512
BOOT_START_SECTOR = 7

CFLAGS = -ffreestanding -m64 -I$(KERNEL_INC_DIR) -I$(LIBK_INC_DIR) -I$(DRIVERS_INC_DIR) -fno-stack-protector -c -O2
PARSER_CFLAGS = -ffreestanding -m64 -I$(LIBK_INC_DIR) -I$(DRIVERS_INC_DIR) -c -fno-pie -fno-stack-protector -fno-builtin -O2

all: $(VDI)

$(STAGE1_BIN): $(STAGE1)
	$(NASM) -f bin $< -o $@

$(STAGE2_BIN): $(STAGE2)
	$(NASM) -f bin $< -o $@

$(LIBK_OBJ): $(LIBK_SRC_DIR)/%.o : $(LIBK_SRC_DIR)/%.c
	$(GCC) $(CFLAGS) $< -o $@

$(DRIVERS_OBJ): $(DRIVERS_SRC_DIR)/%.o : $(DRIVERS_SRC_DIR)/%.c
	$(GCC) $(CFLAGS) $< -o $@

$(LIBK): $(LIBK_OBJ) $(DRIVERS_OBJ)
	$(AR) rcs $@ $^

$(PARSER_SRC:.c=.o): $(PARSER_SRC) $(LIBK)
	$(GCC) $(PARSER_CFLAGS) $< -o $@

$(PARSER_ELF): $(PARSER_SRC:.c=.o) $(LIBK)
	$(LD) -m elf_x86_64 -T parser.ld -o $@ $(PARSER_SRC:.c=.o) $(LIBK)

$(PARSER_BIN): $(PARSER_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_OBJ): $(KERNEL_SRC_DIR)/%.o : $(KERNEL_SRC_DIR)/%.c $(LIBK)
	$(GCC) $(CFLAGS) $< -o $@

$(KERNEL_ELF): $(KERNEL_OBJ) $(LIBK)
	$(LD) -T kernel.ld -o $@ $(KERNEL_OBJ) $(LIBK)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

STAGE2_SIZE = $(shell stat -c%s $(STAGE2_BIN) 2>/dev/null || echo 0)
STAGE2_SECTORS = $(shell expr $$(( ( $(STAGE2_SIZE) + $(SECTOR_SIZE) - 1 ) / $(SECTOR_SIZE) )) )

PARSER_SIZE = $(shell stat -c%s $(PARSER_BIN) 2>/dev/null || echo 0)
PARSER_SECTORS = $(shell expr $$(( ( $(PARSER_SIZE) + $(SECTOR_SIZE) - 1 ) / $(SECTOR_SIZE) )) )

RESERVED_SECTORS = $(shell expr $$(( $(BOOT_START_SECTOR) + $(STAGE2_SECTORS) + $(PARSER_SECTORS) )) )

$(IMG): $(STAGE1_BIN) $(STAGE2_BIN) $(PARSER_BIN) $(KERNEL_BIN)
	$(DD) if=/dev/zero of=$(IMG) bs=1M count=256
	$(MKFS) -F 32 -R $(RESERVED_SECTORS) $(IMG)
	$(DD) if=$(IMG) of=bpb.bin bs=1 skip=11 count=79 conv=notrunc
	cp $(STAGE1_BIN) stage1_patched.bin
	$(DD) if=bpb.bin of=stage1_patched.bin bs=1 seek=11 count=79 conv=notrunc
	$(DD) if=stage1_patched.bin of=$(IMG) bs=$(SECTOR_SIZE) count=1 conv=notrunc
	$(DD) if=$(STAGE2_BIN) of=$(IMG) bs=$(SECTOR_SIZE) seek=$(BOOT_START_SECTOR) conv=notrunc
	$(DD) if=$(PARSER_BIN) of=$(IMG) bs=$(SECTOR_SIZE) seek=$$(( $(BOOT_START_SECTOR) + $(STAGE2_SECTORS) )) conv=notrunc
	$(DD) if=$(IMG) bs=$(SECTOR_SIZE) count=1 of=$(IMG) seek=6 conv=notrunc
	$(MCOPY) -i $(IMG) $(KERNEL_BIN) ::kernel.bin

$(VDI): $(IMG)
	$(VBX) convertfromraw $(IMG) $(VDI) --format VDI --variant Standard

sectors: $(STAGE1_BIN) $(STAGE2_BIN) $(PARSER_BIN) $(KERNEL_BIN)
	@for f in $(STAGE1_BIN) $(STAGE2_BIN) $(PARSER_BIN) $(KERNEL_BIN); do \
		if [ -f "$$f" ]; then \
			SIZE=$$(stat -c%s "$$f"); \
			SECTORS=$$(( ($$SIZE + $(SECTOR_SIZE) - 1) / $(SECTOR_SIZE) )); \
			echo "$$f: $$SIZE bytes, $$SECTORS sectors"; \
		else \
			echo "$$f: file not found"; \
		fi \
	done
	@echo "Reserved sectors: $(RESERVED_SECTORS)"

clean:
	rm -f *.bin *.elf $(IMG) $(VDI) $(LIBK) $(LIBK_SRC_DIR)/*.o $(KERNEL_SRC_DIR)/*.o $(DRIVERS_SRC_DIR)/*.o *.o bpb.bin stage1_patched.bin

