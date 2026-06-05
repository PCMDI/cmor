#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  tools/rebuild_notebook_output.sh [options] NOTEBOOK.ipynb [...]
  tools/rebuild_notebook_output.sh [options] --all

Executes CMOR example notebooks in place, then strips notebook metadata while
keeping the regenerated outputs.

Options:
  --all              Rebuild every notebook in ./notebooks
  --kernel NAME      Kernel name to execute with (default: python3)
  --timeout SECONDS  Cell execution timeout; -1 disables it (default: -1)
  -h, --help         Show this help
EOF
}

die() { echo "error: $*" >&2; exit 1; }

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"
notebooks_dir="${repo_root}/notebooks"

kernel_name="python3"
timeout="-1"
all_notebooks="0"
notebook_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --all)
      all_notebooks="1"
      shift
      ;;
    --kernel)
      [[ $# -ge 2 ]] || die "--kernel requires a value"
      kernel_name="$2"
      shift 2
      ;;
    --timeout)
      [[ $# -ge 2 ]] || die "--timeout requires a value"
      timeout="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      notebook_args+=("$@")
      break
      ;;
    -*)
      die "unknown argument: $1 (use --help)"
      ;;
    *)
      notebook_args+=("$1")
      shift
      ;;
  esac
done

command -v jupyter >/dev/null 2>&1 || die "jupyter not found in PATH"
command -v nbstripout >/dev/null 2>&1 || die "nbstripout not found in PATH"
[[ -d "${notebooks_dir}" ]] || die "missing notebooks directory: ${notebooks_dir}"
[[ "${timeout}" =~ ^-?[0-9]+$ ]] || die "--timeout must be an integer"

if [[ "${all_notebooks}" == "1" && ${#notebook_args[@]} -gt 0 ]]; then
  die "use either --all or explicit notebook paths, not both"
fi

notebooks=()
if [[ "${all_notebooks}" == "1" ]]; then
  shopt -s nullglob
  notebooks=("${notebooks_dir}"/*.ipynb)
  shopt -u nullglob
else
  [[ ${#notebook_args[@]} -gt 0 ]] || die "missing NOTEBOOK.ipynb path (or use --all)"
  for notebook_arg in "${notebook_args[@]}"; do
    if [[ "${notebook_arg}" = /* ]]; then
      notebook_path="${notebook_arg}"
    else
      notebook_path="${PWD}/${notebook_arg}"
    fi

    [[ -f "${notebook_path}" ]] || die "not a file: ${notebook_arg}"
    [[ "${notebook_path}" == *.ipynb ]] || die "not a notebook: ${notebook_arg}"
    notebooks+=("${notebook_path}")
  done
fi

[[ ${#notebooks[@]} -gt 0 ]] || die "no notebooks found"

for notebook in "${notebooks[@]}"; do
  echo "Rebuilding ${notebook#${repo_root}/}"
  jupyter nbconvert \
    --to notebook \
    --execute \
    --inplace \
    --ExecutePreprocessor.cwd="${notebooks_dir}" \
    --ExecutePreprocessor.kernel_name="${kernel_name}" \
    --ExecutePreprocessor.timeout="${timeout}" \
    "${notebook}"

  nbstripout \
    --keep-output \
    --extra-keys "metadata.kernelspec metadata.language_info metadata.widgets" \
    "${notebook}"
done

echo "Rebuilt ${#notebooks[@]} notebook(s)."
