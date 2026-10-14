#!/usr/bin/env python3
"""Drift check for the committed CycloneDX SBOM (sbom/*.cdx.json).

The committed SBOM is a point-in-time record produced out-of-tree
(metadata.tools names the generator); release.yml generates its own
SBOM at release time. What this check guards is the failure mode
that already happened once: the project version moved (CMakeLists
2.0.0 -> 2.0.2) and the committed SBOM silently kept describing the
old release — under CRA, a committed SBOM that does not match the
shipped version is worse than none.

Fails when, for the sbom/ebrowser-<version>.cdx.json record:
  - the file does not parse as CycloneDX JSON;
  - the filename version, metadata.component.version, or any
    first-party component version differs from the project version
    in the root CMakeLists.txt;
  - any component lacks a name or a version.
Third-party components (LVGL, SDL2, ...) keep their own versions.

Stdlib only. Usage: python3 sbom/check_sbom.py [repo-root]
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent


def project_version() -> str:
    text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r"project\s*\([^)]*?VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", text, re.S)
    if not match:
        raise SystemExit("FAIL: cannot find project(VERSION ...) in CMakeLists.txt")
    return match.group(1)


def main() -> int:
    version = project_version()
    records = sorted((ROOT / "sbom").glob("ebrowser-*.cdx.json"))
    if not records:
        print("FAIL: no sbom/ebrowser-*.cdx.json record found")
        return 1
    errors = []
    for record in records:
        if record.name != f"ebrowser-{version}.cdx.json":
            errors.append(
                f"{record.name}: filename version does not match project version {version}"
            )
        try:
            data = json.loads(record.read_text(encoding="utf-8"))
        except ValueError as exc:
            errors.append(f"{record.name}: does not parse as JSON: {exc}")
            continue
        if data.get("bomFormat") != "CycloneDX":
            errors.append(f"{record.name}: bomFormat is not CycloneDX")
        app = data.get("metadata", {}).get("component", {})
        if app.get("version") != version:
            errors.append(
                f"{record.name}: metadata.component.version is "
                f"{app.get('version')!r}, project is {version}"
            )
        for comp in data.get("components", []):
            name = comp.get("name")
            if not name or not comp.get("version"):
                errors.append(f"{record.name}: component without name/version: {comp!r}")
                continue
            if name.startswith("eBrowser") and comp["version"] != version:
                errors.append(
                    f"{record.name}: first-party component {name} is "
                    f"{comp['version']}, project is {version}"
                )
    if errors:
        print("FAIL: SBOM drift detected:")
        for err in errors:
            print(f"  - {err}")
        return 1
    print(f"OK: SBOM record matches project version {version} ({len(records)} file(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
