all: bootloader kernel userland image
	@echo ""
	@echo "✅ Build complete"

bootloader:
	@echo "🔧 Building bootloader..."
	cd Bootloader; make all

kernel:
	@echo "🔧 Building kernel..."
	cd Kernel; make all

userland:
	@echo "🔧 Building userland..."
	cd Userland; make all

image: kernel bootloader userland
	@echo "📦 Creating bootable image..."
	cd Image; make all

buddy:
	@echo "🔧 Building with Buddy Memory Manager..."
	cd Kernel; make clean
	cd Kernel; make all BUDDY_MM=1
	cd Userland; make clean
	cd Userland; make all
	cd Bootloader; make all
	cd Image; make all
	@echo "✅ Buddy MM build complete"

clean:
	@echo "🧹 Cleaning build artifacts..."
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean
	@echo "✅ Clean complete"

format:
	@echo "Formatting code with clang-format..."
	@find Kernel -name '*.c' -o -name '*.h' | xargs clang-format -i
	@find Userland -name '*.c' -o -name '*.h' | xargs clang-format -i
	@echo "✅ Code formatted"

.PHONY: bootloader image collections kernel userland all clean buddy format
