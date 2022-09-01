OUT = maya.hdd

.PHONY: all
all: $(OUT)

.PHONY: run run-uefi
run:
	qemu-system-x86_64 -M q35 -m 2G -enable-kvm -serial stdio -cpu host -hda $(OUT) -smp 1

run-uefi:
	qemu-system-x86_64 -M q35 -m 3G -enable-kvm -serial stdio -cpu host -bios ovmf-x64/OVMF.fd -hda $(OUT) -smp 1


ovmf-x64:
	mkdir -p ovmf-x64
	cd ovmf-x64 && curl -o OVMF-X64.zip https://efi.akeo.ie/OVMF/OVMF-X64.zip && 7z x OVMF-X64.zip

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v3.0-branch-binary --depth=1
	make -C limine

.PHONY: kernel
kernel:
	make -C kernel

$(OUT): limine kernel
	rm -f $(OUT)
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(OUT) # allocate drive space
	parted -s $(OUT) mklabel gpt
	parted -s $(OUT) mkpart ESP fat32 2048s 100%
	parted -s $(OUT) set 1 esp on
	limine/limine-deploy $(OUT)
	sudo losetup -Pf --show $(OUT) > loopback_dev
	sudo mkfs.fat -F 32 `cat loopback_dev`p1
	mkdir -p imgmount
	sudo mount `cat loopback_dev`p1 imgmount
	sudo mkdir -p imgmount/EFI/BOOT
	sudo mkdir -p imgmount/boot
	sudo cp -v kernel/maya.elf sysroot/boot/limine.cfg limine/limine.sys imgmount/boot/
	sudo cp -v limine/BOOTX64.EFI imgmount/EFI/BOOT/
	sync
	sudo umount imgmount
	sudo losetup -d `cat loopback_dev`
	rm -rf loopback_dev imgmount

.PHONY: clean
clean:
	rm -rf $(OUT)
	make -C kernel clean
