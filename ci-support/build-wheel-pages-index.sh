#!/usr/bin/env bash

set -euo pipefail

WHEEL_DIR=${1:?usage: build-wheel-pages-index.sh <wheel-dir> <site-dir>}
SITE_DIR=${2:?usage: build-wheel-pages-index.sh <wheel-dir> <site-dir>}
PACKAGE_NAME=cmor
PACKAGES_DIR="${SITE_DIR}/packages"
PACKAGE_DIR="${SITE_DIR}/${PACKAGE_NAME}"
SIMPLE_DIR="${SITE_DIR}/simple"
SIMPLE_PACKAGE_DIR="${SIMPLE_DIR}/${PACKAGE_NAME}"

shopt -s nullglob
wheels=("${WHEEL_DIR}"/*.whl)
shopt -u nullglob

if [ "${#wheels[@]}" -eq 0 ]; then
    echo "No wheel files found in ${WHEEL_DIR}" >&2
    exit 1
fi

rm -rf "${SITE_DIR}"
mkdir -p "${PACKAGES_DIR}" "${PACKAGE_DIR}" "${SIMPLE_PACKAGE_DIR}"
: > "${SITE_DIR}/.nojekyll"

for wheel_path in "${wheels[@]}"; do
    cp "${wheel_path}" "${PACKAGES_DIR}/"
done

wheel_files=()
for wheel_path in "${PACKAGES_DIR}"/*.whl; do
    wheel_files+=("$(basename "${wheel_path}")")
done

IFS=$'\n' wheel_files=($(printf '%s\n' "${wheel_files[@]}" | sort))
unset IFS

write_package_index() {
    local target_dir=$1
    local package_href_prefix=$2

    {
        cat <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>cmor wheels</title>
</head>
<body>
EOF

        for wheel_file in "${wheel_files[@]}"; do
            local wheel_sha256

            wheel_sha256=$(sha256sum "${PACKAGES_DIR}/${wheel_file}" | awk '{print $1}')
            printf '  <a href="%spackages/%s#sha256=%s">%s</a><br>\n' \
                "${package_href_prefix}" "${wheel_file}" "${wheel_sha256}" "${wheel_file}"
        done

        cat <<EOF
</body>
</html>
EOF
    } > "${target_dir}/index.html"
}

cat > "${SITE_DIR}/index.html" <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>CMOR wheel index</title>
</head>
<body>
  <p>Install the latest main-branch wheels with <code>pip install cmor --extra-index-url https://pcmdi.github.io/cmor</code>.</p>
  <p><a href="./cmor/">Package index</a></p>
  <p><a href="./simple/cmor/">Simple API mirror</a></p>
</body>
</html>
EOF

cat > "${SIMPLE_DIR}/index.html" <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>simple</title>
</head>
<body>
  <a href="./cmor/">cmor</a>
</body>
</html>
EOF

write_package_index "${PACKAGE_DIR}" "../"
write_package_index "${SIMPLE_PACKAGE_DIR}" "../../"
