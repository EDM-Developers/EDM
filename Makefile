EDM_BUILD_DIR ?= build
EDM_BUILD_CONFIG ?= release
EDM_GPU ?= ON

all: $(EDM_BUILD_DIR)/CMakeCache.txt plugin test cli gbench install

$(EDM_BUILD_DIR)/CMakeCache.txt:
	cmake -B $(EDM_BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=$(EDM_BUILD_CONFIG) -DEDM_WITH_ARRAYFIRE=$(EDM_GPU)

.PHONY: plugin
plugin: $(EDM_BUILD_DIR)/CMakeCache.txt
	cmake --build $(EDM_BUILD_DIR) --config $(EDM_BUILD_CONFIG) --parallel 12 --target edm_plugin

.PHONY: cli
cli: $(EDM_BUILD_DIR)/CMakeCache.txt
	cmake --build $(EDM_BUILD_DIR) --config $(EDM_BUILD_CONFIG) --parallel 12 --target edm_cli

.PHONY: gbench
gbench: $(EDM_BUILD_DIR)/CMakeCache.txt
	cmake --build $(EDM_BUILD_DIR) --config $(EDM_BUILD_CONFIG) --parallel 12 --target gbench

.PHONY: test
test: $(EDM_BUILD_DIR)/CMakeCache.txt
	cmake --build $(EDM_BUILD_DIR) --config $(EDM_BUILD_CONFIG) --parallel 12 --target edm_test

.PHONY: install
install:
	cmake --build $(EDM_BUILD_DIR) --config $(EDM_BUILD_CONFIG) --parallel 12 --target install

.PHONY: clean
clean:
	rm -rf $(EDM_BUILD_DIR)

.PHONY: format
format:
	cmake --build $(EDM_BUILD_DIR) --target format
