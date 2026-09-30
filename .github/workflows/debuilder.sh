#!/bin/bash
set -euo pipefail

# Keep a compatible packaging baseline instead of choosing arbitrary releases.
DEB_VERSION=0.23.90-1+deb11u1
DEB_FNAME="zbar_${DEB_VERSION}.debian.tar.xz"
DEB_URL="https://deb.debian.org/debian/pool/main/z/zbar/${DEB_FNAME}"
DEB_SHA256=e1b287effc4d0d915c144d5857caa3d7501414897976e6fbc26227fa685ca1ec

ZBARDIR=${PWD}
PACKAGE_DIR=${ZBAR_PACKAGE_DIR:-${ZBARDIR}/..}
WORKDIR=$(mktemp -d)
trap 'rm -rf "$WORKDIR"' EXIT
VER=$(perl -ne 'print $1 if /version:\s*'\''([\d.]+)'\''/' meson.build)
[[ -n "$VER" ]]
TAR="${WORKDIR}/zbar_${VER}.orig.tar.gz"

git archive --format=tgz -o "$TAR" HEAD
if [[ -n "${ZBAR_DEB_METADATA:-}" ]]; then
    cp "$ZBAR_DEB_METADATA" "${WORKDIR}/${DEB_FNAME}"
else
    curl --fail --location --retry 3 "$DEB_URL" \
        --output "${WORKDIR}/${DEB_FNAME}"
fi
printf '%s  %s\n' "$DEB_SHA256" "${WORKDIR}/${DEB_FNAME}" | sha256sum -c -

mkdir "${WORKDIR}/source"
cd "${WORKDIR}/source"
tar xf "$TAR"
tar xf "${WORKDIR}/${DEB_FNAME}"
cp "${ZBARDIR}/.github/workflows/debian.rules" debian/rules
chmod +x debian/rules
rm -rf debian/patches
# The pinned baseline predates the introspection binary package.
printf '%s\n' 'usr/lib/*/girepository-1.0/ZBar-1.0.typelib' \
    > debian/gir1.2-zbar-1.0.install
printf '%s\n' 'usr/share/gir-1.0/ZBar-1.0.gir' \
    >> debian/libzbargtk-dev.install
sed -i '/^Build-Depends:/a\ meson (>= 0.61.0),\n ninja-build,\n gettext,\n gobject-introspection,\n libgirepository1.0-dev,' \
    debian/control
# This package builds the GTK3 variant, including its public dependencies.
# shellcheck disable=SC2016
sed -i '/^Package: libzbargtk-dev$/,/^Description:/ {
    s/libgtk2.0-dev/libgtk-3-dev/
    /^Depends:/a\ gir1.2-zbar-1.0 (= ${binary:Version}),
}' debian/control
cat >> debian/control <<'EOF_CONTROL'

Package: gir1.2-zbar-1.0
Section: introspection
Architecture: any
Multi-Arch: same
Depends:
 ${misc:Depends},
 ${gir:Depends},
Description: GObject introspection data for ZBar
 ZBar scans and decodes bar codes from images and video streams.
 This package provides introspection data for the ZBar GTK3 widget.
EOF_CONTROL
# shellcheck disable=SC1091
DISTRIBUTION=$(. /etc/os-release; printf '%s' "${VERSION_CODENAME:-unstable}")
cat > debian/changelog <<EOF_CHANGELOG
zbar (${VER}-1) ${DISTRIBUTION}; urgency=medium

  * Upstream version

 -- LinuxTV bot <linuxtv-commits@linuxtv.org>  $(date -R)
EOF_CHANGELOG

debuild -us -uc
mkdir -p "$PACKAGE_DIR"
cp "${WORKDIR}"/*.deb "${WORKDIR}"/*.buildinfo \
   "${WORKDIR}"/*.changes "$PACKAGE_DIR/"
