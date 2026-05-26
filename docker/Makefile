IMAGE_NAME ?= acadia

.PHONY: build run clean delete-images

build:
	@echo "Building Docker image $(IMAGE_NAME) for Raspberry Pi 5 (ARM64)..."
	docker buildx build --platform linux/arm64 --load --build-arg UID=$(shell id -u) --build-arg GID=$(shell id -g) -t $(IMAGE_NAME) .

run:
	# No --gpus flag: Pi 5 has VideoCore VII (not CUDA)
	docker run -it --rm --hostname rpi5 $(IMAGE_NAME)

clean:
	docker rmi $(IMAGE_NAME) || true

delete-images:
	docker rmi $(docker images -q) || true
