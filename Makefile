# --- Toolchain ---------------------------------------------------------------
CC ?= cc

# --- Project layout ----------------------------------------------------------
SRC_DIR := src
OUT_DIR := output
OBJ_DIR := $(OUT_DIR)/obj
BIN     := $(OUT_DIR)/rogue-lite

# --- Vendored raylib ---------------------------------------------------------
RAYLIB_DIR := vendor/raylib-6.2_linux_amd64
RAYLIB_INC := $(RAYLIB_DIR)/include
RAYLIB_LIB := $(RAYLIB_DIR)/lib/libraylib.a

# --- Flags ---------------------------------------------------------------
CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra
CPPFLAGS += -I$(RAYLIB_INC) -MMD -MP

# raylib's Linux runtime dependencies (static lib, so they must be explicit)
LDLIBS += $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11

# --- Sources -------------------------------------------------------------
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# --- Rules ---------------------------------------------------------------
all: $(BIN)

# --- Linking rule --------------------------------------------------------
# Link the .o files into the executable
$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# --- Compilation rule ----------------------------------------------------
# Compile each .c file in the src folder to the respective .o file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# --- Execution rule ------------------------------------------------------
run: $(BIN)
	./$(BIN)

# --- Cleanup rule --------------------------------------------------------
clean:
	rm -rf $(OUT_DIR)/*

# --- Editor rule --------------------------------------------------------
# Zed uses clangd as LSP and that does not read Makefile, so to avoid linting
# errors, we need to generate this file
compile_flags.txt: Makefile
	@printf '%s\n' $(foreach f,$(CPPFLAGS) $(CFLAGS),'$(f)') > $@

all: $(BIN) compile_flags.txt

.PHONY: all run clean compile_flags.txt

-include $(DEPS)
