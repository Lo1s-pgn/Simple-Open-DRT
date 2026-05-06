SHELL := /bin/bash

BUILD_DIR := build
SOURCE_DIR := source
RELEASE_DIR := release

CMAKE ?= cmake
CMAKE_GENERATOR ?= Ninja
CMAKE_BUILD_TYPE ?= Release
CMAKE_OSX_ARCHITECTURES ?= x86_64;arm64
CMAKE_OSX_DEPLOYMENT_TARGET ?= 13.0

.PHONY: all configure build clean rebuild install

all: build

configure:
	$(CMAKE) -S $(SOURCE_DIR) -B $(BUILD_DIR) -G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DCMAKE_OSX_ARCHITECTURES="$(CMAKE_OSX_ARCHITECTURES)" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET=$(CMAKE_OSX_DEPLOYMENT_TARGET)

build: configure
	$(CMAKE) --build $(BUILD_DIR) --config $(CMAKE_BUILD_TYPE)

clean:
	rm -rf "$(BUILD_DIR)" "$(RELEASE_DIR)"

rebuild: clean build

install:
	./tools/install_simple_open_drt_ofx.command
