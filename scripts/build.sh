#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "$(uname -s)" in
    Darwin) exec "$script_dir/build-macos.sh" "$@" ;;
    Linux)  exec "$script_dir/build-linux.sh" "$@" ;;
    *)
        echo "Unsupported Unix platform. On Windows run scripts/build-windows.ps1." >&2
        exit 2
        ;;
esac
