#!/bin/bash
set -e

# Download Debian packaging metadata and build the Meson project with debuild.
DEB_FNAME="zbar_*.debian.tar.xz"
DEB_URL="http://deb.debian.org/debian/pool/main/z/zbar/"

# Should be the same version as provided by the host OS
COMPAT=12

# Set directories used during the build

ZBARDIR=${PWD}
BUILDDIR=${ZBARDIR}/../build

echo "Generating an origin tarball"

cd "${ZBARDIR}"

VER=$(perl -ne 'print $1 if /^project\('\''zbar'\'',.*version:\s*'\''([\d.]+)'\''/' "${ZBARDIR}/meson.build")
TAR=${ZBARDIR}/../zbar_${VER}.orig.tar.gz

git archive --format tgz -o "${TAR}" HEAD

echo "Retrieving Debian ruleset"
lftp -e "mget -c ${DEB_FNAME}; exit" "${DEB_URL}"

# Ensure to use just one version, in case multiple ones were downloaded
DEB_FNAME=$(ls -1 ${DEB_FNAME} | tail -1)

echo "Preparing build environment"
rm -rf "${BUILDDIR}/" || true
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"

tar xf "${TAR}"
tar xf "${ZBARDIR}/${DEB_FNAME}"

# Ensure that debhelper-compat will use the one expected by the build distro
sed -E "s#debhelper-compat.*,#debhelper-compat (= $COMPAT),#" -i debian/control

# Make debhelper route all standard configure/build/install steps through Meson.
if ! grep -q -- '--buildsystem=meson' debian/rules; then
    sed -i 's/dh \$@/dh $@ --buildsystem=meson/' debian/rules
fi

# Ignore missing SONAME for libs, if any, as it is not a build robot's task
# to update ${DEB_FNAME} ruleset
echo -e "\noverride_dh_shlibdeps:" >> debian/rules
echo -e "\tdh_shlibdeps --dpkg-shlibdeps-params=--ignore-missing-info" >> debian/rules

# We want it to build cleanly - so drop all patches from it
rm -rf debian/patches

# Override the changelog to ensure that it will contain the current version
cat << EOF > debian/changelog
zbar (${VER}) unstable; urgency=medium

  * Upstream version

 -- LinuxTV bot <linuxtv-commits@linuxtv.org>  $(date -R)
EOF

OS_VERSION=$(. /etc/os-release && echo "$ID-$VERSION_ID")

echo "Building ZBar packages for ${OS_VERSION}"
debuild -us -uc
