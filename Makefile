# To use: make `launch.json` entry that launches and attaches GDB to `build/main`
# Mock data is at the top of `manual.cpp`

COMPILER := gcc
MKDIR_P = mkdir -p
DEBUG_FLAGS := -g -ggdb
FLAGS := -DMOCK=1 -Wall $(DEBUG_FLAGS)
LIBS := -lstdc++

MAIN_SRCFILES := \
	src/*.cpp

MOCK_SRCFILES := \
	mock/*.cpp

OUT_DIR := build
FILENAME := main
OUT_FILE := $(OUT_DIR)/$(FILENAME)


SRCFILES := $(MAIN_SRCFILES) $(MOCK_SRCFILES)

mkdir:
	$(MKDIR_P) $(OUT_DIR)

.PHONY: build
build: mkdir
	$(COMPILER) $(SRCFILES) $(FLAGS) -o $(OUT_FILE) $(LIBS)

.PHONY: run
run: build
	$(OUT_FILE)

.PHONY: run-verbose
run-verbose: build
	$(OUT_FILE) -v

.PHONY: run-v
run-v: run-verbose

.PHONY: gdb
gdb: build
	gdb $(OUT_FILE)

.PHONY: gdb-remote
gdb-remote: build
	gdbserver :9091 $(OUT_FILE)