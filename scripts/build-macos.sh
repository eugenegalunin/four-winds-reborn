#!/usr/bin/env bash
set -euo pipefail

install_deps=0
clean=0
run_tests=1
build_type=Release

for option in "$@"; do
    case "$option" in
        --install-deps) install_deps=1 ;;
        --clean) clean=1 ;;
        --no-tests) run_tests=0 ;;
        --debug) build_type=Debug ;;
        *) echo "Unknown option: $option" >&2; exit 2 ;;
    esac
done

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_root/build-macos"

if (( install_deps )); then
    command -v brew >/dev/null 2>&1 || {
        echo "Homebrew is required for --install-deps: https://brew.sh" >&2
        exit 2
    }
    brew install cmake pkg-config sdl2 sdl2_image sdl2_mixer sdl2_ttf zlib
fi

for command_name in git cmake pkg-config; do
    command -v "$command_name" >/dev/null 2>&1 || {
        echo "Missing command: $command_name (rerun with --install-deps)." >&2
        exit 2
    }
done

if command -v brew >/dev/null 2>&1; then
    brew_prefix="$(brew --prefix)"
    zlib_prefix="$(brew --prefix zlib 2>/dev/null || true)"
    export PKG_CONFIG_PATH="${zlib_prefix:+$zlib_prefix/lib/pkgconfig:}$brew_prefix/lib/pkgconfig:$brew_prefix/share/pkgconfig:${PKG_CONFIG_PATH:-}"
fi

for module in sdl2 SDL2_image SDL2_mixer SDL2_ttf zlib; do
    pkg-config --exists "$module" || {
        echo "Missing pkg-config module: $module (rerun with --install-deps)." >&2
        exit 2
    }
done

cd "$repo_root"
git submodule update --init --recursive
if (( clean )); then rm -rf "$build_dir"; fi
testing=OFF
if (( run_tests )); then testing=ON; fi

cmake -S . -B "$build_dir" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DBUILD_TESTING="$testing"
cmake --build "$build_dir" --parallel "$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

if (( run_tests )); then
    ctest --test-dir "$build_dir" --output-on-failure
fi

cp -f "$build_dir/four-winds-reborn" "$repo_root/four-winds-reborn"
echo "Built: $repo_root/four-winds-reborn"
