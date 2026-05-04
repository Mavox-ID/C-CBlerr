# Produces:  src/bin/CBlerr
#
# Targets:
#   make          → release build: src/bin/CBlerr
#   make clean    → remove build artefacts
#   make install  → copy to /usr/local/bin/CBlerr
# Pls dont makefile me angry :< (Just build it :>) TTT > 07:31 AM AHHHH...

CC      ?= gcc

CFLAGS := \
    -std=c11 \
    -Os \
    -s \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -Wno-missing-field-initializers \
    -Wno-stringop-truncation \
    -Wno-format-truncation \
    -Wno-misleading-indentation \
    -ffunction-sections \
    -fdata-sections \
    -fno-ident \
    -fno-asynchronous-unwind-tables \
    -fno-unwind-tables \
    -fomit-frame-pointer \
    -fno-stack-protector \
    -fmerge-all-constants \
    -fno-math-errno \
    -fvisibility=hidden

LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--strip-all \
    -Wl,--build-id=none \
    -Wl,--no-eh-frame-hdr \
    -Wl,-z,norelro \
    -lm

UPX      := upx
UPXFLAGS := --best --lzma --quiet

SRC_DIR  := src
CORE_DIR := $(SRC_DIR)/core
BIN_DIR  := $(SRC_DIR)/bin

SRCS := \
    $(SRC_DIR)/cblerr.c \
    $(CORE_DIR)/lexer.c \
    $(CORE_DIR)/parser.c \
    $(CORE_DIR)/codegen.c \
    $(CORE_DIR)/module_loader.c \
    $(CORE_DIR)/debugger.c \
    $(CORE_DIR)/type_checker.c \
    $(CORE_DIR)/monomorphizer.c

TARGET := $(BIN_DIR)/CBlerr

.PHONY: all clean

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRCS) | $(BIN_DIR)
	@echo "[CBlerr] Compiling..."
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "[CBlerr] Built: $(TARGET)"
	@SIZE_RAW=$$(stat -c%s $(TARGET)); \
	if command -v $(UPX) >/dev/null 2>&1; then \
	    $(UPX) $(UPXFLAGS) $(TARGET) >/dev/null 2>&1 && \
	    SIZE_UPX=$$(stat -c%s $(TARGET)) && \
	    echo "[CBlerr] UPX: $$SIZE_RAW bytes → $$SIZE_UPX bytes ($$(( SIZE_RAW / 1024 )) KB → $$(( SIZE_UPX / 1024 )) KB)"; \
	else \
	    echo "[CBlerr] UPX not found - Skipping ($$SIZE_RAW bytes)"; \
	fi

clean:
	rm -f $(TARGET)
	rm -rf /tmp/cblerr_standalone /tmp/CBlerr_standalone
