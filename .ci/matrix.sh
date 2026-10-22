#!/bin/sh -e

# Windows: msvc, clang-cl, msys2 gcc (amd64 only) + clang
# Linux: ubuntu, arch; gcc, clang, clang-19
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
WIN2022="$(matrix "Windows 2022")"

UBUNTU_LTS="$(container "Ubuntu 26.04" ubuntu:26.04)"
UBUNTU_STABLE="$(container "Ubuntu 26.10" ubuntu:26.10)"

ARCH="$(container "Arch Linux" ghcr.io/pkgforge-dev/archlinux:latest)"
MACOS="$(matrix "macOS")"

UCRT64="$(msys "MSYS2 UCRT64" UCRT64)"
CLANGARM64="$(msys "MSYS2 CLANGARM64" CLANGARM64)"

CL='msvc clang-cl'
CLANG_CL='clang-cl'
GCC='gcc'
GCC16='gcc-16'
GCC_CLANG='gcc clang'
CLANG='clang'
CLANG19='clang-19'
APPLE_CLANG='brew-clang'

# broken, appleclang does not yet support modules
# APPLE_CLANG='appleclang brew-clang'

# only used by recent GCC
REFL='true false'

# runs-on=arch
WIN2022_RO="windows-2022=amd64"
WIN_RO="windows-latest=amd64 windows-11-arm=aarch64"
UBUNTU_RO="ubuntu-26.04=amd64 ubuntu-26.04-arm=aarch64"

# ALARM is stuck on GCC 16.1.1 which has a severe bug that makes compilation impossible
ARCH_REFL_RO="ubuntu-26.04=amd64"
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
programs "$WIN2022" "$WIN2022_RO" "$CLANG_CL"

# msys
programs "$UCRT64" "$UCRT_RO" "$GCC" "$REFL"
programs "$UCRT64" "$UCRT_RO" "$CLANG"
programs "$CLANGARM64" "$CLANGARM_RO" "$CLANG"

# ubuntu
programs "$UBUNTU_LTS" "$UBUNTU_RO" "$CLANG19"
programs "$UBUNTU_LTS" "$UBUNTU_RO" "$CLANG"
programs "$UBUNTU_STABLE" "$UBUNTU_RO" "$GCC16" "$REFL"
programs "$UBUNTU_STABLE" "$UBUNTU_RO" "$CLANG"

# arch
programs "$ARCH" "$ARCH_RO" "$CLANG"
programs "$ARCH" "$ARCH_REFL_RO" "$GCC" "$REFL"

# macos
programs "$MACOS" "$MACOS_RO" "$APPLE_CLANG"

echo ']'
