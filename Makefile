# Master Makefile for Acadia HAL
# Supports both native and Docker-based builds with Ninja

.PHONY: all build configure clean test format cppcheck doc help
.PHONY: package package-deb package-rpm package-ipk
.PHONY: docker-build docker-configure docker-clean docker-shell docker-install docker-image docker-test
.PHONY: docker-package docker-package-deb docker-package-rpm docker-package-ipk
.PHONY: deploy deploy-a deploy-b run run-a run-b list setup-completion

# Build directories (separate to avoid CMake cache conflicts)
NATIVE_BUILD_DIR := build/native
CROSS_BUILD_DIR := build/cross

# Parallel jobs (override with make JOBS=N)
JOBS := $(shell nproc)
DOCKER_IMAGE := acadia
DOCKER_CROSS_BUILD_DIR := /workspace/build/cross

# Host architecture as shown by file(1) (e.g. x86-64, aarch64)
HOST_ARCH := $(shell uname -m | sed 's/x86_64/x86-64/')

# IP addresses for Raspberry Pi 5 target devices
# Override on the command line: make deploy IP_A=<addr> IP_B=<addr>
IP_A := 192.168.1.3
IP_B := 192.168.1.13

# User and password for SSH/SCP.
# Override on the command line or via environment variables:
#   make deploy USER=pi PASS=<password>
#   HAL_SSH_USER=pi HAL_SSH_PASS=<password> make deploy
# Do NOT commit real credentials here.
USER := $(or $(HAL_SSH_USER),nvidia)
PASS := $(or $(HAL_SSH_PASS),nvidia)
SSHPASS := sshpass -p $(PASS)
SSH := $(SSHPASS) ssh -o StrictHostKeyChecking=no
SCP := $(SSHPASS) scp -o StrictHostKeyChecking=no

# Default target
all: docker-build

# Deploy to remote target: $(1)=IP address
define deploy_remote
	@echo "Deploying to $(1)..."
	$(SSH) $(USER)@$(1) "rm -rf /tmp/acu-test && mkdir -p /tmp/acu-test"
	$(SCP) -r $(CROSS_BUILD_DIR)/lib $(USER)@$(1):/tmp/acu-test/
	$(SCP) -r $(CROSS_BUILD_DIR)/bin $(USER)@$(1):/tmp/acu-test/
endef

# Run apps on remote target: $(1)=IP address
define run_remote
	@APP="$(word 2, $(MAKECMDGOALS))"; \
	ARGS="$(wordlist 3, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))"; \
	if [ -z "$$APP" ]; then \
		echo "Running all test applications on $(1)..."; \
		for name in $$($(SSH) $(USER)@$(1) "ls /tmp/acu-test/bin/"); do \
			echo "--- Running $$name ---"; \
			$(SSH) $(USER)@$(1) "cd /tmp/acu-test && ./bin/$$name" || true; \
		done; \
	else \
		echo "Running $$APP on $(1) with args: $$ARGS"; \
		$(SSH) $(USER)@$(1) "cd /tmp/acu-test && ./bin/$$APP $$ARGS"; \
	fi
endef

define run_local
	@APP="$(word 2, $(MAKECMDGOALS))"; \
	ARGS="$(wordlist 3, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))"; \
	if [ -z "$$APP" ]; then \
		echo "Running all test applications locally..."; \
		for name in $$(ls $(NATIVE_BUILD_DIR)/bin/); do \
			echo "--- Running $$name ---"; \
			$(if $(V),echo "LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$name";) \
			LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$name || true; \
		done; \
	else \
		echo "Running $$APP locally with args: $$ARGS"; \
		$(if $(V),echo "LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$APP $$ARGS";) \
		LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$APP $$ARGS; \
	fi
endef

# =============================================================================
# Native builds (host architecture)
# =============================================================================

configure:
	@echo "Configuring project with Ninja..."
	mkdir -p $(NATIVE_BUILD_DIR)
	cd $(NATIVE_BUILD_DIR) && cmake -GNinja $(CURDIR)

build: configure
	@echo "Building project with Ninja..."
	cd $(NATIVE_BUILD_DIR) && ninja -j$(JOBS)

run: build
	$(call run_local)

test: build
	@echo "Running tests with CTest..."
	cd $(NATIVE_BUILD_DIR) && ctest --output-on-failure -j$(JOBS)

# =============================================================================
# Common for native and Cross
# =============================================================================

clean:
	@echo "Cleaning build directory..."
	rm -rf build

list:
	@echo "Native ($(NATIVE_BUILD_DIR)):"
	@ls -1 $(NATIVE_BUILD_DIR)/bin/ 2>/dev/null | sed 's/^/    /' || echo "    (none - run 'make build' first)"
	@echo "Cross ($(CROSS_BUILD_DIR)):"
	@ls -1 $(CROSS_BUILD_DIR)/bin/ 2>/dev/null | sed 's/^/    /' || echo "    (none - run 'make docker-build' first)"

# =============================================================================
# Cross builds (Docker)
# =============================================================================

# -t: allocate a pseudo-TTY so Ctrl-C (SIGINT) is forwarded to the container
# --init: use tini as PID 1 to properly propagate signals to child processes (e.g. ninja)
# ccache volume: persist compilation cache across docker runs
DOCKER_RUN := docker run --rm -t --init --platform linux/arm64 -v $(PWD):/workspace -v ccache-acu:/home/rpi/.ccache $(DOCKER_IMAGE) bash -c

docker-install:
	@if ! command -v docker > /dev/null 2>&1; then \
		echo "Docker not found. Installing Docker..."; \
		sudo apt update && sudo apt install -y docker.io; \
	else \
		echo "Docker is already installed."; \
	fi

docker-image: docker-install
	@if ! docker image inspect $(DOCKER_IMAGE) >/dev/null 2>&1; then \
		echo "Building Docker image $(DOCKER_IMAGE)..."; \
		make -C docker build; \
	else \
		echo "Docker image $(DOCKER_IMAGE) already exists."; \
	fi

docker-configure: docker-image
	@echo "Configuring project with Docker/Ninja..."
	$(DOCKER_RUN) "mkdir -p $(DOCKER_CROSS_BUILD_DIR) && cd $(DOCKER_CROSS_BUILD_DIR) && cmake -GNinja /workspace"

docker-build: docker-image
	@echo "Building project with Docker/Ninja..."
	$(DOCKER_RUN) "if [ ! -f $(DOCKER_CROSS_BUILD_DIR)/build.ninja ]; then mkdir -p $(DOCKER_CROSS_BUILD_DIR) && cd $(DOCKER_CROSS_BUILD_DIR) && cmake -GNinja /workspace; fi && cd $(DOCKER_CROSS_BUILD_DIR) && ninja -j$(JOBS)"

docker-clean: docker-image
	@echo "Cleaning Docker build directory..."
	$(DOCKER_RUN) "rm -rf $(DOCKER_CROSS_BUILD_DIR)"

docker-shell: docker-image
	@echo "Starting Docker shell for development..."
	docker run -it --rm --platform linux/arm64 -v $(PWD):/workspace -v ccache-acu:/home/rpi/.ccache $(DOCKER_IMAGE) bash

docker-test: docker-build
	@echo "Running tests inside Docker (ARM64 via QEMU)..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ctest --output-on-failure -j$(JOBS)"

docker-package-deb: docker-build
	@echo "Generating DEB package (cross/ARM64)..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && cpack -G DEB"

docker-package-rpm: docker-build
	@echo "Generating RPM package (cross/ARM64, requires rpmbuild in image)..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && cpack -G RPM"

docker-package-ipk: docker-build
	@echo "Generating IPK package (cross/ARM64)..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && cmake --build . --target package_ipk"

docker-package: docker-package-deb docker-package-ipk
	@echo "Cross packages generated in build/cross/"

# =============================================================================
# Deploy & Run (on target device)
# =============================================================================

deploy: deploy-a deploy-b

deploy-a: docker-build
	$(call deploy_remote,$(IP_A))

deploy-b: docker-build
	$(call deploy_remote,$(IP_B))

run-a: deploy-a
	$(call run_remote,$(IP_A))

run-b: deploy-b
	$(call run_remote,$(IP_B))

# =============================================================================
# Code quality & Docs
# =============================================================================

format: configure
	@echo "Formatting C++ files with clang-format..."
	cd $(NATIVE_BUILD_DIR) && ninja format
	@echo "Formatting CMake files..."
	@if command -v cmake-format > /dev/null 2>&1; then \
		find . -name CMakeLists.txt -not -path './build/*' | xargs cmake-format -i; \
	else \
		echo "  cmake-format not found, skipping CMake file formatting."; \
	fi

cppcheck: configure
	@echo "Running cppcheck static analysis..."
	cd $(NATIVE_BUILD_DIR) && ninja cppcheck

doc: configure
	@echo "Generating documentation..."
	cd $(NATIVE_BUILD_DIR) && ninja doc_doxygen

# =============================================================================
# Packaging (DEB / IPK / RPM)
# =============================================================================

package-deb: build
	@echo "Generating DEB package..."
	cd $(NATIVE_BUILD_DIR) && cpack -G DEB
	@echo "Package: $$(ls $(NATIVE_BUILD_DIR)/acadia-hal_*.deb 2>/dev/null | head -1)"

package-rpm: build
	@echo "Generating RPM package (requires rpmbuild: sudo apt-get install rpm)..."
	cd $(NATIVE_BUILD_DIR) && cpack -G RPM
	@echo "Package: $$(ls $(NATIVE_BUILD_DIR)/acadia-hal-*.rpm 2>/dev/null | head -1)"

package-ipk: build
	@echo "Generating IPK package..."
	cd $(NATIVE_BUILD_DIR) && cmake --build . --target package_ipk
	@echo "Package: $$(ls $(NATIVE_BUILD_DIR)/acadia-hal_*.ipk 2>/dev/null | head -1)"

package: package-deb package-ipk
	@echo "All packages generated in $(NATIVE_BUILD_DIR)/"

# =============================================================================
# Misc
# =============================================================================

setup-completion:
	@echo "Setting up bash completion for make..."
	@if ! grep -q "source $(PWD)/make_completion.bash" ~/.bashrc; then \
		echo "source $(PWD)/make_completion.bash" >> ~/.bashrc; \
		echo "Added completion to ~/.bashrc. Restart your shell or run 'source ~/.bashrc' to activate."; \
	else \
		echo "Completion already set up in ~/.bashrc."; \
	fi

# =============================================================================
# Help
# =============================================================================

help:
	@echo "Available targets:"
	@echo "  Native builds (host architecture):"
	@echo "    configure    - Configure the project with CMake/Ninja"
	@echo "    build        - Build the project (configures if needed)"
	@echo "    test         - Run tests with CTest"
	@echo "    run          - Run test app locally (usage: make run [name] [args...], no args = run all)"
	@echo ""
	@echo "  Common (host and docker):"
	@echo "    clean        - Remove build directory"
	@echo "    list         - List available test applications"
	@echo ""
	@echo "  Cross builds (Docker / ARM64):"
	@echo "    all                  - Alias for docker-build (default)"
	@echo "    docker-install       - Install Docker if not present"
	@echo "    docker-image         - Build Docker image if not exists"
	@echo "    docker-configure     - Configure project with Docker/Ninja"
	@echo "    docker-build         - Build project with Docker/Ninja"
	@echo "    docker-test          - Run CTest inside Docker (ARM64 via QEMU)"
	@echo "    docker-clean         - Clean Docker build artifacts"
	@echo "    docker-shell         - Start Docker shell for development"
	@echo ""
	@echo "  Deploy & Run (on target device):"
	@echo "    deploy       - Deploy to all devices (deploy-a + deploy-b)"
	@echo "    deploy-a     - Deploy lib and bin to $(IP_A)"
	@echo "    deploy-b     - Deploy lib and bin to $(IP_B)"
	@echo "    run-a        - Run test app on $(IP_A) (usage: make run-a [name] [args...])"
	@echo "    run-b        - Run test app on $(IP_B) (usage: make run-b [name] [args...])"
	@echo "    Override IPs: make deploy IP_A=<addr> IP_B=<addr>"
	@echo ""
	@echo "  Packaging:"
	@echo "    package-deb          - Generate DEB package (native build)"
	@echo "    package-rpm          - Generate RPM package (requires rpmbuild)"
	@echo "    package-ipk          - Generate IPK package (DEB renamed .ipk)"
	@echo "    package              - Generate DEB + IPK (native build)"
	@echo "    docker-package-deb   - Generate DEB package (cross/ARM64)"
	@echo "    docker-package-rpm   - Generate RPM package (cross/ARM64)"
	@echo "    docker-package-ipk   - Generate IPK package (cross/ARM64)"
	@echo "    docker-package       - Generate DEB + IPK (cross/ARM64)"
	@echo ""
	@echo "  Code quality & Docs:"
	@echo "    format       - Format code with clang-format (and cmake-format if available)"
	@echo "    cppcheck     - Run static analysis with cppcheck"
	@echo "    doc          - Generate Doxygen documentation"
	@echo ""
	@echo "  Misc:"
	@echo "    setup-completion - Set up bash completion for make (adds to ~/.bashrc)"
	@echo ""
	@echo "  Environment variables:"
	@echo "    HAL_SSH_USER  - SSH username for deploy (default: nvidia)"
	@echo "    HAL_SSH_PASS  - SSH password for deploy"
	@echo "    HAL_TLS_CERT  - Path to server TLS certificate"
	@echo "    HAL_TLS_KEY   - Path to server TLS private key"
	@echo "    HAL_TLS_CA    - Path to CA certificate for mutual TLS"
	@echo "    JOBS          - Parallel build jobs (default: nproc=$(JOBS))"
