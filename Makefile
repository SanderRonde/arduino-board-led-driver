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

build: mkdir
	$(COMPILER) $(SRCFILES) $(FLAGS) -o $(OUT_FILE) $(LIBS)

run: build
	$(OUT_FILE)

run-verbose: build
	$(OUT_FILE) -v

run-v: run-verbose

gdb: build
	gdb $(OUT_FILE)

gdb-remote: build
	gdbserver :9091 $(OUT_FILE)