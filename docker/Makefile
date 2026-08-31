IMAGE_NAME ?= zion

.PHONY: build run clean delete-images

build:
	@echo "Building Docker image $(IMAGE_NAME) for Zynq UltraScale+ MPSoC (ARM64)..."
	docker buildx build --platform linux/arm64 --load --build-arg UID=$(shell id -u) --build-arg GID=$(shell id -g) -t $(IMAGE_NAME) .

run:
	# No --gpus flag: Zion platform uses Zynq UltraScale+ (not CUDA)
	docker run -it --rm --hostname "zion" $(IMAGE_NAME)

clean:
	docker rmi $(IMAGE_NAME) || true

delete-images:
	docker rmi $(docker images -q) || true
