# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Mauro Carvalho Chehab <mchehab+huawei@kernel.org>

BUILD_DIR ?= build
MESON_ARGS ?=

RPM_TOPDIR = $(abspath $(BUILD_DIR))
MOCK_CONFIG ?= fedora-rawhide-x86_64

VERSION = $(shell meson introspect --projectinfo $(BUILD_DIR) -i | \
	    grep '"version"' | cut -d '"' -f 4)
DIST_TARBALL = $(BUILD_DIR)/meson-dist/zbar-$(VERSION).tar.gz

.PHONY: all clean reconfigure install uninstall devenv distclean check tests \
	check-local regress other-tests docs html-local dist archive dist-nsis

all: $(BUILD_DIR)/build.ninja
	rm -f $(BUILD_DIR)/zbar/config.h
	meson compile -C $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	meson setup $(BUILD_DIR) $(MESON_ARGS)

clean: $(BUILD_DIR)/build.ninja
	ninja -C $(BUILD_DIR) clean

reconfigure: $(BUILD_DIR)/build.ninja
	meson setup --reconfigure $(BUILD_DIR) $(MESON_ARGS)

install: $(BUILD_DIR)/build.ninja
	meson install -C $(BUILD_DIR)

uninstall: $(BUILD_DIR)/build.ninja
	ninja -C $(BUILD_DIR) uninstall

devenv: all
	meson devenv -C $(BUILD_DIR)

distclean:
	rm -rf $(BUILD_DIR)

check check-local tests: all
	meson test -C $(BUILD_DIR) --print-errorlogs

regress: all
	meson test -C $(BUILD_DIR) --suite regression --print-errorlogs

other-tests: all
	meson test -C $(BUILD_DIR) --suite other --print-errorlogs

docs: $(BUILD_DIR)/build.ninja
	meson compile -C $(BUILD_DIR) docs

html-local: $(BUILD_DIR)/build.ninja
	meson compile -C $(BUILD_DIR) html

dist archive: $(BUILD_DIR)/build.ninja
	meson dist -C $(BUILD_DIR) --formats=gztar \
		$(if $(DRY_RUN),--no-tests \
		$(shell meson dist --help | sed -n 's/.*--allow-dirty.*/--allow-dirty/p' | head -1))

dist-nsis: all
	python3 tools/build_nsis.py $(BUILD_DIR) \
		--runtime-prefix "$(MINGW_PREFIX)"

srpm: dist
	mkdir -p "$(RPM_TOPDIR)"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
	cp "$(DIST_TARBALL)" "$(RPM_TOPDIR)/SOURCES/"
	rpmbuild --define "_topdir $(RPM_TOPDIR)" -bs "$(BUILD_DIR)/zbar.spec"

mock: srpm
	mock -r "$(MOCK_CONFIG)" --resultdir="./SRPMS" "$(BUILD_DIR)/SRPMS/zbar-$(VERSION)-1.src.rpm"
