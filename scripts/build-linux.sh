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
build_dir="$repo_root/build-linux"

if (( install_deps )); then
    command -v apt-get >/dev/null 2>&1 || {
        echo "Automatic dependency installation currently supports Debian/Ubuntu (apt-get)." >&2
        exit 2
    }
    sudo apt-get update
    sudo apt-get install -y \
        build-essential cmake git pkg-config \
        libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev \
        zlib1g-dev
fi

for command_name in git cmake pkg-config c++; do
    command -v "$command_name" >/dev/null 2>&1 || {
        echo "Missing command: $command_name (rerun with --install-deps)." >&2
        exit 2
    }
done

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
cmake --build "$build_dir" --parallel "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

if (( run_tests )); then
    ctest --test-dir "$build_dir" --output-on-failure
fi

cp -f "$build_dir/four-winds-reborn" "$repo_root/four-winds-reborn"
echo "Built: $repo_root/four-winds-reborn"
