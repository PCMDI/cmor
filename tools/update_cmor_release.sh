#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  tools/update_cmor_release.sh --minor N --patch N
  tools/update_cmor_release.sh --version X.Y.Z

Updates CMOR minor/patch version numbers and sets the release date to today.
Also regenerates ./configure by running autoconf after updating configure.ac.

Options:
  --minor N       Set minor version (integer)
  --patch N       Set patch version (integer)
  --version V     Set full version (X.Y.Z); major must match current major
  --dry-run       Show intended changes, do not modify files
  -h, --help      Show this help
EOF
}

die() { echo "error: $*" >&2; exit 1; }

perl_inplace_or_die() {
  local file="$1"
  local perl_expr="$2"
  local desc="${3:-edit}"

  perl -0777 -i -pe "${perl_expr}" "${file}" || die "failed to ${desc} in ${file}"
}

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

minor=""
patch=""
full_version=""
dry_run="0"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --minor)
      [[ $# -ge 2 ]] || die "--minor requires a value"
      minor="$2"
      shift 2
      ;;
    --patch)
      [[ $# -ge 2 ]] || die "--patch requires a value"
      patch="$2"
      shift 2
      ;;
    --version)
      [[ $# -ge 2 ]] || die "--version requires a value"
      full_version="$2"
      shift 2
      ;;
    --dry-run)
      dry_run="1"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown argument: $1 (use --help)"
      ;;
  esac
done

configure_ac="${repo_root}/configure.ac"
configure_sh="${repo_root}/configure"
cmor_h="${repo_root}/include/cmor.h"
setup_py_in="${repo_root}/setup.py.in"
citation="${repo_root}/CITATION.cff"

[[ -f "${configure_ac}" ]] || die "missing file: ${configure_ac}"
[[ -f "${configure_sh}" ]] || die "missing file: ${configure_sh}"
[[ -f "${cmor_h}" ]] || die "missing file: ${cmor_h}"
[[ -f "${setup_py_in}" ]] || die "missing file: ${setup_py_in}"
[[ -f "${citation}" ]] || die "missing file: ${citation}"

current_version="$(perl -ne 'if (/^AC_INIT\(\[cmor\],\[(\d+)\.(\d+)\.(\d+)\]/) { print "$1.$2.$3\n"; exit }' "${configure_ac}")"
[[ -n "${current_version}" ]] || die "could not parse current version from ${configure_ac}"

current_major="${current_version%%.*}"
rest="${current_version#*.}"
current_minor="${rest%%.*}"
current_patch="${current_version##*.}"

if [[ -n "${full_version}" ]]; then
  if [[ "${full_version}" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
    new_major="${BASH_REMATCH[1]}"
    minor="${BASH_REMATCH[2]}"
    patch="${BASH_REMATCH[3]}"
    [[ "${new_major}" == "${current_major}" ]] || die "--version major (${new_major}) must match current major (${current_major})"
  else
    die "--version must be X.Y.Z"
  fi
fi

[[ -n "${minor}" ]] || die "missing --minor (or use --version)"
[[ -n "${patch}" ]] || die "missing --patch (or use --version)"
[[ "${minor}" =~ ^[0-9]+$ ]] || die "--minor must be an integer"
[[ "${patch}" =~ ^[0-9]+$ ]] || die "--patch must be an integer"

new_version="${current_major}.${minor}.${patch}"
release_date="$(date +%Y-%m-%d)"

echo "CMOR version: ${current_version} -> ${new_version}"
echo "Release date: ${release_date}"

if [[ "${dry_run}" == "1" ]]; then
  echo "Dry run; no files modified."
  exit 0
fi

command -v autoconf >/dev/null 2>&1 || die "autoconf not found in PATH"

backup_dir="$(mktemp -d "${repo_root}/.cmor_release_backup.XXXXXX")"
cleanup() {
  local status=$?
  if [[ $status -ne 0 ]]; then
    cp -p "${backup_dir}/configure.ac" "${configure_ac}" 2>/dev/null || true
    cp -p "${backup_dir}/configure" "${configure_sh}" 2>/dev/null || true
    cp -p "${backup_dir}/cmor.h" "${cmor_h}" 2>/dev/null || true
    cp -p "${backup_dir}/setup.py.in" "${setup_py_in}" 2>/dev/null || true
    cp -p "${backup_dir}/CITATION.cff" "${citation}" 2>/dev/null || true
  fi
  rm -rf "${backup_dir}" 2>/dev/null || true
}
trap cleanup EXIT

cp -p "${configure_ac}" "${backup_dir}/configure.ac"
cp -p "${configure_sh}" "${backup_dir}/configure"
cp -p "${cmor_h}" "${backup_dir}/cmor.h"
cp -p "${setup_py_in}" "${backup_dir}/setup.py.in"
cp -p "${citation}" "${backup_dir}/CITATION.cff"

export CMOR_NEW_VERSION="${new_version}"
export CMOR_NEW_MINOR="${minor}"
export CMOR_NEW_PATCH="${patch}"
export CMOR_NEW_DATE="${release_date}"

perl_inplace_or_die "${configure_ac}" \
  'BEGIN{$c=0} $c += s/^(\s*AC_INIT\(\[cmor\],\[)[^\]]+(\].*)$/$1 . $ENV{CMOR_NEW_VERSION} . $2/mse; END{exit($c?0:2)}' \
  "set AC_INIT version"

perl_inplace_or_die "${cmor_h}" \
  'BEGIN{$c=0} $c += s/^(\s*#define\s+CMOR_VERSION_MINOR\s+)[0-9]+\s*$/$1 . $ENV{CMOR_NEW_MINOR}/mse; END{exit($c?0:2)}' \
  "set CMOR_VERSION_MINOR"

perl_inplace_or_die "${cmor_h}" \
  'BEGIN{$c=0} $c += s/^(\s*#define\s+CMOR_VERSION_PATCH\s+)[0-9]+\s*$/$1 . $ENV{CMOR_NEW_PATCH}/mse; END{exit($c?0:2)}' \
  "set CMOR_VERSION_PATCH"

perl_inplace_or_die "${setup_py_in}" \
  'BEGIN{$c=0} $c += s/(\bversion=\x27)[0-9]+\.[0-9]+\.[0-9]+(\x27)/$1 . $ENV{CMOR_NEW_VERSION} . $2/gse; END{exit($c?0:2)}' \
  "set setup.py.in version"

perl_inplace_or_die "${citation}" \
  'BEGIN{$c=0} $c += s/^(version:\s*)[0-9]+\.[0-9]+\.[0-9]+\s*$/$1 . $ENV{CMOR_NEW_VERSION}/mse; END{exit($c?0:2)}' \
  "set CITATION.cff version"

perl_inplace_or_die "${citation}" \
  'BEGIN{$c=0} $c += s/^(date-released:\s*)[0-9]{4}-[0-9]{2}-[0-9]{2}\s*$/$1 . $ENV{CMOR_NEW_DATE}/mse; END{exit($c?0:2)}' \
  "set CITATION.cff date-released"

(
  cd -- "${repo_root}"
  autoconf || die "autoconf failed; revert changes with: git checkout -- configure.ac configure include/cmor.h setup.py.in CITATION.cff"

)

echo "Updated:"
echo "  ${configure_ac}"
echo "  ${configure_sh}"
echo "  ${cmor_h}"
echo "  ${setup_py_in}"
echo "  ${citation}"
