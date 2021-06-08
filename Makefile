.PHONY: release-build debug-build
debug-build: | debug
	ninja -C debug
release-build: | release
	ninja -C release

debug:
	env CFLAGS="-fsanitize=address,undefined" meson setup debug --buildtype debug
release:
	meson setup release --buildtype release
install: | release
	ninja -C release install
clean: | debug release
	ninja -C release clean
	ninja -C debug clean
