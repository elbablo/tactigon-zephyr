SHELL := /bin/bash

# Carica l'ambiente Zephyr (venv, ZEPHYR_BASE, PATH) prima di ogni comando
ENV := . scripts/env.sh &&

.PHONY: help init build rebuild upload flash monitor menuconfig clean

help:
	@echo "make init        scarica e installa Zephyr, SDK e tool (una tantum)"
	@echo "make build       compila il firmware"
	@echo "make rebuild     compila da zero (build pristine)"
	@echo "make upload      copia build/zephyr/zephyr.uf2 sulla scheda (bootloader UF2)"
	@echo "make flash       build + upload"
	@echo "make monitor     apre la console seriale (SERIAL_PORT=... BAUD=...)"
	@echo "make menuconfig  apre menuconfig"
	@echo "make clean       rimuove la cartella build"

init:
	@./scripts/init.sh

build:
	@$(ENV) west build -b "$$BOARD" .

rebuild:
	@$(ENV) west build -p always -b "$$BOARD" .

upload:
	@./scripts/upload.sh

flash: build upload

monitor:
	@./scripts/monitor.sh

menuconfig:
	@$(ENV) west build -b "$$BOARD" . -t menuconfig

clean:
	rm -rf build
