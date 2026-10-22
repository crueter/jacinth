#!/bin/sh -e

# Windows: msvc, clang-cl, msys2 gcc (amd64 only) + clang
# Linux: ubuntu, arch; gcc, clang
# macOS: appleclang, homebrew clang

matrix() {
	name="$1"
	shift

	n=0
	os="["
	for i in "$@"; do
		if [ "$n" -gt 0 ]; then
			os="$os, "
		fi

		os="$os\"$i\""
		n=$((n + 1))
	done

	os="$os]"

	printf '"name": "%s", "runs-on": %s' "$name" "$os"
}

msys() {
	name="$1"
	msystem="$2"
	shift 2
	matrix "$name" "$@"

	printf ', "msystem": "%s"' "$msystem"
}

container() {
	name="$1"
	container="$2"
	shift 2
	matrix "$name" "$@"

	printf ', "container": "%s"' "$container"
}

WIN="$(matrix "Windows" windows-latest windows-11-arm)"
UBUNTU="$(matrix "Ubuntu" ubuntu-26.04 ubuntu-26.04-arm)"
ARCH="$(container "Arch Linux" ghcr.io/pkgforge-dev/archlinux:latest ubuntu-26.04 ubuntu-26.04-arm)"
MACOS="$(matrix "macOS" macos-latest)"

UCRT64="$(msys "MSYS2 UCRT64" UCRT64 windows-latest)"
CLANGARM64="$(msys "MSYS2 CLANGARM64" CLANGARM64 windows-11-arm)"

CL='["msvc", "clang-cl"]'
GCC='["gcc"]'
GCC_CLANG='["gcc", "clang"]'
CLANG='["clang"]'
APPLE_CLANG='["appleclang", "brew-clang"]'

# only used by recent GCC
REFL='[true, false]'

n=0

target() {
	if [ "$n" -gt 0 ]; then
		printf ", "
	fi

	printf '{%s, "program": %s, "refl": %s}' "$1" "$2" "${3:-false}"
	n=$((n + 1))
}

printf '['

# windows
target "$WIN" "$CL"
target "$UCRT64" "$GCC" "$REFL"
target "$UCRT64" "$CLANG"
target "$CLANGARM64" "$CLANG"

# ubuntu
target "$UBUNTU" "$GCC_CLANG"

# arch
target "$ARCH" "$CLANG"
target "$ARCH" "$GCC" "$REFL"

# macos
target "$MACOS" "$APPLE_CLANG"

echo ']'
