
ifeq ($(OS), Windows_NT)
	BUILD_COMMAND = build.bat
else
	BUILD_COMMAND = ./build.sh
endif

all:
	$(BUILD_COMMAND)

release:
	$(BUILD_COMMAND) release
