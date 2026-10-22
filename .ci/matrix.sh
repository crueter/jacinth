#!/bin/sh -e

# Windows: msvc, clang-cl, msys2 gcc (amd64 only) + clang
# Linux: ubuntu, arch; gcc, clang
# macOS: appleclang, homebrew clang

matrix() {
	printf '"name": "%s"' "$1"
}

msys() {
	matrix "$1"

	printf ', "msystem": "%s"' "$2"
}

container() {
	matrix "$1"

	printf ', "container": "%s"' "$2"
}

WIN="$(matrix "Windows")"
UBUNTU="$(matrix "Ubuntu")"
ARCH="$(container "Arch Linux" ghcr.io/pkgforge-dev/archlinux:latest)"
MACOS="$(matrix "macOS")"

UCRT64="$(msys "MSYS2 UCRT64" UCRT64)"
CLANGARM64="$(msys "MSYS2 CLANGARM64" CLANGARM64)"

CL='msvc clang-cl'
GCC='gcc'
GCC_CLANG='gcc clang'
CLANG='clang'
APPLE_CLANG='brew-clang'

# broken, appleclang does not yet support modules
# APPLE_CLANG='appleclang brew-clang'

# only used by recent GCC
REFL='true false'

# runs-on=arch
WIN_RO="windows-latest=amd64 windows-11-arm=aarch64"
UBUNTU_RO="ubuntu-26.04=amd64 ubuntu-26.04-arm=aarch64"
ARCH_RO="$UBUNTU_RO"
UCRT_RO="windows-latest=amd64"
CLANGARM_RO="windows-11-arm=aarch64"
MACOS_RO="macos-latest=aarch64"

n=0

# data, runner, arch, program, refl
target() {
	if [ "$n" -gt 0 ]; then
		printf ", "
	fi

	printf '{%s, "runs-on": "%s", "arch": "%s", "program": "%s", "refl": %s}' "$1" "$2" "$3" "$4" "$5"
	n=$((n + 1))
}

programs() {
	data="$1"
	runs="$2"
	progs="$3"
	refl="${4:-false}"

	for o in $runs; do
		runner="${o%%=*}"
		arch="${o##*=}"

		for p in $progs; do
			for r in $refl; do
				target "$data" "$runner" "$arch" "$p" "$r"
			done
		done
	done
}

printf '['

# windows
programs "$WIN" "$WIN_RO" "$CL"
programs "$UCRT64" "$UCRT_RO" "$GCC" "$REFL"
programs "$UCRT64" "$UCRT_RO" "$CLANG"
programs "$CLANGARM64" "$CLANGARM_RO" "$CLANG"

# ubuntu
programs "$UBUNTU" "$UBUNTU_RO" "$GCC_CLANG"

# arch
programs "$ARCH" "$ARCH_RO" "$CLANG"
programs "$ARCH" "$ARCH_RO" "$GCC" "$REFL"

# macos
programs "$MACOS" "$MACOS_RO" "$APPLE_CLANG"

echo ']'
