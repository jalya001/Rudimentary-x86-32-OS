CC = g++
LD = ld

TOOLS = tools
BOOT = boot
BUILD = build
KERNEL = kernel

CCOPTS = -std=gnu++17 -O2 -m32 -Wall -g -c -fno-builtin -fno-stack-protector \
         -march=i386 -ffreestanding -nostdlib -fno-pic -fno-pie \
         -fno-exceptions -fno-rtti

LDOPTS = -melf_i386 -nostdlib -N

all: $(BUILD) bootblock createimage kernel.elf image

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/kernel.o: $(KERNEL)/kernel.cpp | $(BUILD)
	$(CC) $(CCOPTS) -o $@ $<

kernel.elf: $(BUILD)/kernel.o | $(BUILD)
	$(LD) $(LDOPTS) -T $(KERNEL)/kernel.ld -o $(BUILD)/$@ $<

$(BUILD)/bootblock.o: ${BOOT}/bootblock.s | $(BUILD)
	$(CC) $(CCOPTS) -o $@ $<

bootblock: $(BUILD)/bootblock.o | $(BUILD)
	$(LD) $(LDOPTS) -Ttext 0x0 -o $(BUILD)/bootblock.elf $<
	objcopy -O binary $(BUILD)/bootblock.elf $(BUILD)/bootblock

$(BUILD)/createimage.o: $(TOOLS)/createimage.cpp | $(BUILD)
	$(CC) -c -o $@ $<

createimage: $(BUILD)/createimage.o
	$(CC) -o $(BUILD)/createimage $<

image: bootblock createimage kernel.elf
	./build/createimage --extended ./build/bootblock ./build/kernel.elf
	mv image build/image

boot: image
	dd if=/dev/zero of=boot_img_test bs=512 count=2880
	dd if=./build/image of=boot_img_test bs=512 conv=notrunc

clean:
	rm -rf build boot_img_test