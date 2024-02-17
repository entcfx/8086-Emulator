CC = g++
ASM = nasm
ROOT = .
SRC_DIR = $(ROOT)/src
BUILD_DIR = $(ROOT)/build

# Define ANSI escape codes for colors
GREEN = \033[0;32m
YELLOW = \033[0;33m
NC = \033[0m # No Color

# List all source files in the emulator directory
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
# Generate corresponding object file names
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))

.PHONY: all emulator clean

all: $(BUILD_DIR) emulator

emulator: $(OBJ_FILES) $(ROOT)/x86


$(ROOT)/x86: $(OBJ_FILES)
	@echo -e "$(GREEN)Linking $@$(NC)"
	$(CC) -o $@ $^


# Pattern rule to compile .cpp files to .o files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo -e "$(GREEN)Compiling $@$(NC)"
	$(CC) -c -o $@ $<


clean:
	rm -rf $(ROOT)/x86 $(BUILD_DIR)
	@clear

reset:
	$(MAKE) clean
	$(MAKE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
