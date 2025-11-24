###
# Makefile to build and release Docker images used in our CI/CD build pipeline and deployments
###

.DEFAULT_GOAL := oci

SHELL               := /bin/bash
RELEASE_TAG         ?= $(shell cat release.tag)
BUILD_ARCH          ?= linux/$(shell uname -m)
OK                  := "[ 👍 ]"
BUILD_LOG           := ./build/build.log

.PHONY: help
help:
	@echo ""
	@echo "Makefile to build Docker base container images"
	@echo ""
	@echo "Targets:"
	@echo "  help:             Show this help."
	@echo ""
	@echo "  deps:             Verify if dependencies to build container images are available."
	@echo ""
	@echo "  Dockerfile:       Generate Dockerfile for the project for local build and development."
	@echo ""
	@echo "  lint:             Linting Dockerfile"
	@echo ""
	@echo "  oci:              Build a local OCI image for a single target architecture for a project."
	@echo "                    You can specify the target architecture with SINGLE_ARCH."
	@echo "                    The default is set to $(BUILD_ARCH)."
	@echo ""
	@echo "  clean:            Remove all build artifacts, i.e. Dockerfile, builder instance and OCI images in the build directory."
	@echo ""

.PHONY: deps
deps:
	@echo -n "👮 Check envsubst available:                 "
	@command -v envsubst >/dev/null 2>&1;
	@echo "$(OK)"
	@echo -n "👮 Check docker and shellcheck available:    "
	@command -v docker >/dev/null 2>&1;
	@echo "$(OK)"
	@echo -n "👮 Check shellcheck available:               "
	@command -v shellcheck >/dev/null 2>&1;
	@echo "$(OK)"
	@echo -n "👩‍🔧 Create build directory:                   "
	@mkdir -p ./build >/dev/null 2>&1;
	@echo "$(OK)"

.PHONY: deps-hadolint
deps-hadolint:
	@command -v hadolint 2>>$(BUILD_LOG) 1>/dev/null;

.PHONY: shellcheck
shellcheck: deps
	@echo -n "👮 Run shellcheck:                           "
	@find . -type f -name '*.sh' | xargs shellcheck --external-sources -e SC2034,SC2155 2>>$(BUILD_LOG) 1>/dev/null;
	@echo "$(OK)"

.PHONY: lint
lint: deps-hadolint Dockerfile
	@echo -n "👮 Run Hadolint on Dockerfile:               "
	@find . -type f -name 'Dockerfile' | xargs hadolint	2>>$(BUILD_LOG) 1>/dev/null;
	@echo "$(OK)"

.PHONY: Dockerfile
Dockerfile: shellcheck
	@echo -n "👩‍🔧 Generating Dockerfile:                    "
	@source ./version-lock.sh && envsubst < "Dockerfile.tpl" > "Dockerfile";
	@echo "$(OK)"

.PHONY: oci
oci: Dockerfile lint
	@echo -n "📦 Build container image:                    "
	@docker build --platform="$(BUILD_ARCH)" --tag $(RELEASE_TAG) . 2>>$(BUILD_LOG) 1>/dev/null;
	@echo "$(OK)"
	@echo ""
	@echo "🚀 Container image available in Docker context with tag: $(RELEASE_TAG)"

clean: deps
	@echo -n "🧼 Remove generated Dockerfile:              "
	@rm -f Dockerfile 2>>$(BUILD_LOG) 1>/dev/null;
	@echo "$(OK)"
	@echo -n "🧼 Remove OCI build artifacts:               "
	@rm -rf ./build/*.oci 2>>$(BUILD_LOG) 1>/dev/null;
	@echo "$(OK)"
	@echo -n "🧼 Remove build logs:                        "
	@rm -rf ./build/*.log 2>>$(BUILD_LOG) 1>/dev/null;
	@echo "$(OK)"
