# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Mauro Carvalho Chehab <mchehab+huawei@kernel.org>

BUILD_DIR ?= build
MESON_ARGS ?=

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
