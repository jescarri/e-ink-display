# Uncomment lines below if you have problems with $PATH
#SHELL := /bin/bash
#PATH := /usr/local/bin:$(PATH)

# Public key for OTA signature verification (same as lora-sensor)
IDENTITYLABS_PUB_KEY ?= a206eb8f630dbe913481fee5e91b19cd338247187bea975187b545b178ade8c1

# Export for platformio
export IDENTITYLABS_PUB_KEY

.PHONY: all upload clean program uploadfs update release build-cli

all:
	@pio -f -c vim run

upload:
	@pio -f -c vim run --target upload

clean:
	@pio -f -c vim run --target clean

program:
	@pio -f -c vim run --target program

uploadfs:
	@pio -f -c vim run --target uploadfs

update:
	@pio -f -c vim update

# Build release firmware
release:
	@echo "Building release firmware..."
	@platformio run

# Build CLI tool
build-cli:
	@echo "Building e-paper CLI tool..."
	cd cli && go build -o e-paper-cli .
	@echo "CLI tool built: cli/e-paper-cli"
