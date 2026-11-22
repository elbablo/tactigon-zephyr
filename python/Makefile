SHELL := /bin/bash

VULCANEXUS_DISTRO ?= jazzy
VENV              := ../.venv
LANGUAGE   ?= auto

# Vulcanexus (include ROS 2) prima del venv: il python3 del venv deve avere la precedenza nel PATH
ENV := . /opt/vulcanexus/$(VULCANEXUS_DISTRO)/setup.bash && . $(VENV)/bin/activate &&

.PHONY: help init build run partial text clean

help:
	@echo "make init                       crea il venv e installa requirements.txt"
	@echo "make build                      colcon build del pacchetto tactigon"
	@echo "make run ADDRESS=XX:XX:... [LANGUAGE=it]   avvia il launch"
	@echo "make partial                   mostra il topic transcription_partial (testo parziale)"
	@echo "make text                      mostra il topic transcription (testo completo)"
	@echo "make clean                      rimuove build/ install/ log/"

init:
	@command -v colcon >/dev/null || { echo "Manca colcon" >&2; exit 1; }
	@test -d $(VENV) || python3 -m venv $(VENV)
	@$(VENV)/bin/pip install -r requirements.txt

build:
	@$(ENV) colcon build

run:
	@test -n "$(ADDRESS)" || { echo "Uso: make run ADDRESS=XX:XX:XX:XX:XX:XX [LANGUAGE=it]" >&2; exit 1; }
	@test -d install || { echo "Esegui prima: make build" >&2; exit 1; }
	@$(ENV) . install/setup.bash && ros2 launch tactigon tactigon.launch.py address:=$(ADDRESS) language:=$(LANGUAGE)

partial:
	@. /opt/vulcanexus/$(VULCANEXUS_DISTRO)/setup.bash && ros2 topic echo /tactigon/transcription_partial std_msgs/msg/String --field data

text:
	@. /opt/vulcanexus/$(VULCANEXUS_DISTRO)/setup.bash && ros2 topic echo /tactigon/transcription std_msgs/msg/String --field data

clean:
	rm -rf build install log
