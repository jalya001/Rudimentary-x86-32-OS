CC = g++
LD = ld

TOOLS = tools
BOOT = boot
BUILD = build
KERNEL = kernel
USER = user
LIB = lib

CCOPTS = -std=gnu++17 -O2 -m32 -Wall -g -c -fno-builtin -fno-stack-protector \
         -march=i386 -ffreestanding -nostdlib -nostdinc -Ilib -fno-pic -fno-pie \
         -fno-exceptions -fno-rtti

LDOPTS = -melf_i386 -nostdlib -N

LIB_CXX := $(shell find $(LIB) -name '*.cpp')
LIB_CXX_OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(LIB_CXX))
KERNEL_CXX := $(shell find $(KERNEL) -name '*.cpp')
KERNEL_S := $(shell find $(KERNEL) -name '*.S')
KERNEL_CXX_OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(KERNEL_CXX))
KERNEL_S_OBJ := $(patsubst %.S,$(BUILD)/%.o,$(KERNEL_S))
KERNEL_OBJS := $(KERNEL_CXX_OBJ) $(KERNEL_S_OBJ) $(LIB_CXX_OBJ)
USER_CXX := $(shell find $(USER) -name '*.cpp')
USER_CXX_OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(USER_CXX))
USER_BINS := $(patsubst user/%.cpp,$(BUILD)/user/%,$(USER_CXX))

all: $(BUILD) bootblock createimage kernel.elf user_programs lib image

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CCOPTS) -o $@ $<

$(BUILD)/%.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(CCOPTS) -o $@ $<

kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDOPTS) -T $(KERNEL)/kernel.ld \
		-o $(BUILD)/$@ $(KERNEL_OBJS)

$(BUILD)/bootblock.o: ${BOOT}/bootblock.s | $(BUILD)
	$(CC) $(CCOPTS) -o $@ $<

bootblock: $(BUILD)/bootblock.o | $(BUILD)
	$(LD) $(LDOPTS) -Ttext 0x0 -o $(BUILD)/bootblock.elf $<
	objcopy -O binary $(BUILD)/bootblock.elf $(BUILD)/bootblock

lib: $(LIB_CXX_OBJ)

user_programs: $(USER_BINS)

$(BUILD)/user/%: $(BUILD)/user/%.o $(LIB_CXX_OBJ)
	$(LD) $(LDOPTS) -Ttext 0x0 -o $@ $^

$(BUILD)/createimage.o: $(TOOLS)/createimage.cpp | $(BUILD)
	$(CC) -c -o $@ $<

createimage: $(BUILD)/createimage.o
	$(CC) -o $(BUILD)/createimage $<

image: bootblock createimage kernel.elf user_programs lib
	./build/createimage --extended ./build/bootblock ./build/kernel.elf $(USER_BINS)
	mv image build/image

boot: image
	dd if=/dev/zero of=boot_img_test bs=512 count=2880
	dd if=./build/image of=boot_img_test bs=512 conv=notrunc

clean:
	rm -rf build boot_img_test