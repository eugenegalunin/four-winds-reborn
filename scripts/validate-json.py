#!/usr/bin/env python3

import json
import sys
from pathlib import Path


def main() -> int:
    repository = Path(__file__).resolve().parents[1]
    json_files = sorted((repository / "themes").rglob("*.json"))
    if not json_files:
        print("No theme JSON files found.", file=sys.stderr)
        return 1

    failures = 0
    for path in json_files:
        try:
            with path.open("r", encoding="utf-8") as source:
                json.load(source)
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            print(f"{path.relative_to(repository)}: {error}", file=sys.stderr)
            failures += 1

    if failures:
        print(f"Theme JSON validation failed for {failures} file(s).", file=sys.stderr)
        return 1

    print(f"Validated {len(json_files)} theme JSON file(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
