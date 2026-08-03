#!/usr/bin/env python3

import argparse
import hashlib
import json
import pathlib
import re
import sys

parser = argparse.ArgumentParser()

parser.add_argument("--package", required=True)
parser.add_argument("--repo", required=True)
parser.add_argument("--version", required=True)
parser.add_argument("--ref", help="Commit SHA to pin to. Defaults to v{version} (tag).")
parser.add_argument("--archive", required=True)
parser.add_argument("--root-dir", required=True)
parser.add_argument("--conan-dir", required=True)
parser.add_argument("--vcpkg-dir", required=True)
parser.add_argument(
    "--submodule",
    action="append",
    default=[],
    metavar="NAME=REPO:SHA:ARCHIVE",
    help="Internal submodule to pin in portfile.cmake. Repeatable.",
)

args = parser.parse_args()

package = args.package
repo = args.repo
version = args.version

archive = pathlib.Path(args.archive)

if not archive.exists():
    sys.exit(f"Archive not found: {archive}")

data = archive.read_bytes()

sha256 = hashlib.sha256(data).hexdigest()
sha512 = hashlib.sha512(data).hexdigest()

main_ref = args.ref or f"v{version}"

print(f"Version : {version}")
print(f"Ref     : {main_ref}")
print(f"SHA256  : {sha256}")
print(f"SHA512  : {sha512}")

# Maps a REPO value as it appears in portfile.cmake (e.g.
# "privateMwb/MiniDatabase") to the (ref, sha512) it should be pinned
# to. Starts with the main package; --submodule entries add to it.
ref_sha_map = {repo: (main_ref, sha512)}

for entry in args.submodule:
    name, _, rest = entry.partition("=")
    sub_repo, sub_sha, sub_archive_path = rest.split(":", 2)

    sub_archive = pathlib.Path(sub_archive_path)
    if not sub_archive.exists():
        sys.exit(f"Submodule archive not found for {name}: {sub_archive}")

    sub_sha512 = hashlib.sha512(sub_archive.read_bytes()).hexdigest()
    ref_sha_map[sub_repo] = (sub_sha, sub_sha512)

    print(f"  {name:<14} ({sub_repo}) ref={sub_sha} sha512={sub_sha512[:16]}...")

# ---------------------------------------------------------------------
# Root CMakeLists.txt
# ---------------------------------------------------------------------

root_cmakelists = pathlib.Path(args.root_dir) / "CMakeLists.txt"

text = root_cmakelists.read_text(encoding="utf-8")


def _bump_project_version(match: re.Match) -> str:
    # Substitute only within the matched project(...) call, so this
    # can't touch an unrelated VERSION elsewhere in the file (e.g.
    # cmake_minimum_required(VERSION ...)).
    return re.sub(
        r"VERSION\s+[0-9A-Za-z.\-_]+",
        f"VERSION {version}",
        match.group(0),
        count=1,
    )


new_text, count = re.subn(
    r"project\s*\([^)]*\)",
    _bump_project_version,
    text,
    count=1,
    flags=re.DOTALL,
)

if count == 0:
    sys.exit(f"No project() call found in {root_cmakelists}")

root_cmakelists.write_text(new_text, encoding="utf-8")

print("✓ Updated CMakeLists.txt")

# ---------------------------------------------------------------------
# Conan
# ---------------------------------------------------------------------

recipe_dir = pathlib.Path(args.conan_dir)

# conanfile.py

conanfile = recipe_dir / "conanfile.py"

text = conanfile.read_text(encoding="utf-8")

text = re.sub(
    r'version\s*=\s*"[^"]+"',
    f'version = "{version}"',
    text,
)

conanfile.write_text(text, encoding="utf-8")

print("✓ Updated conanfile.py")

# conandata.yml

conandata = recipe_dir / "conandata.yml"

if args.ref:
    conandata_url = f"https://github.com/{repo}/archive/{main_ref}.tar.gz"
else:
    conandata_url = f"https://github.com/{repo}/archive/refs/tags/{main_ref}.tar.gz"

conandata.write_text(
f'''sources:
  "{version}":
    url: "{conandata_url}"
    sha256: "{sha256}"
''',
encoding="utf-8"
)

print("✓ Updated conandata.yml")

# ---------------------------------------------------------------------
# vcpkg
# ---------------------------------------------------------------------

port_dir = pathlib.Path(args.vcpkg_dir)

# portfile.cmake

portfile = port_dir / "portfile.cmake"

text = portfile.read_text(encoding="utf-8")

# Each vcpkg_from_github(...) block is matched independently and its
# REF/SHA512 are only substituted using the (ref, sha512) pinned for
# that block's own REPO -- a single global substitution would blindly
# overwrite every block with the main package's ref/hash, which is
# wrong for any port with submodule fetch blocks (like minidb).
block_pattern = re.compile(
    r"(REPO\s+)(\S+)(\s*REF\s+)(\S+)(\s*SHA512\s+)(\S+)"
)


def _replace_block(match: re.Match) -> str:
    block_repo = match.group(2)
    pin = ref_sha_map.get(block_repo)
    if pin is None:
        # Not one of the repos this run knows about (e.g. a port with
        # unrelated fetch blocks) -- leave it untouched.
        return match.group(0)
    block_ref, block_sha512 = pin
    return (
        match.group(1) + match.group(2)
        + match.group(3) + block_ref
        + match.group(5) + block_sha512
    )


new_text, block_count = block_pattern.subn(_replace_block, text)

if block_count == 0:
    sys.exit(f"No vcpkg_from_github REPO/REF/SHA512 blocks found in {portfile}")

unmatched = set(ref_sha_map) - {
    m.group(2) for m in block_pattern.finditer(text)
}
if unmatched:
    sys.exit(f"REPO(s) not found in {portfile}: {', '.join(sorted(unmatched))}")

portfile.write_text(new_text, encoding="utf-8")

print("✓ Updated portfile.cmake")

# vcpkg.json

vcpkg_json = port_dir / "vcpkg.json"

data = json.loads(vcpkg_json.read_text(encoding="utf-8"))

data["version"] = version

vcpkg_json.write_text(
    json.dumps(data, indent=2) + "\n",
    encoding="utf-8",
)

print("✓ Updated vcpkg.json")