all: bootloader kernel userland image
	@echo ""
	@echo "\033[0;32mBuild complete\033[0m"

bootloader:
	@echo "\033[0;33mBuilding bootloader...\033[0m"
	cd Bootloader; make all

kernel:
	@echo "\033[0;33mBuilding kernel...\033[0m"
	cd Kernel; make all

userland:
	@echo "\033[0;33mBuilding userland...\033[0m"
	cd Userland; make all

image: kernel bootloader userland
	@echo "\033[0;33mCreating bootable image...\033[0m"
	cd Image; make all

buddy:
	@echo "\033[0;33mBuilding with Buddy Memory Manager...\033[0m"
	cd Kernel; make clean
	cd Kernel; make all BUDDY_MM=1
	cd Userland; make clean
	cd Userland; make all
	cd Bootloader; make all
	cd Image; make all
	@echo "\033[0;32mBuddy MM build complete\033[0m"

clean:
	@echo "\033[0;33mCleaning build artifacts...\033[0m"
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean
	@echo "\033[0;32mClean complete\033[0m"

format:
	@echo "\033[0;33mFormatting code with clang-format...\033[0m"
	@find Kernel -name '*.c' -o -name '*.h' | xargs clang-format -i
	@find Userland -name '*.c' -o -name '*.h' | xargs clang-format -i
	@echo "\033[0;32mCode formatted\033[0m"

.PHONY: bootloader image collections kernel userland all clean buddy format
