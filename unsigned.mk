# ============================================================================
# UNSIGNED Engine
# ============================================================================

UNSIGNED_DIR ?= external/unsigned

UNSIGNED_SRC_DIR     := $(UNSIGNED_DIR)/src
UNSIGNED_INCLUDE_DIR := $(UNSIGNED_DIR)/include
UNSIGNED_BUILD_DIR   := $(BUILDDIR)/unsigned

UNSIGNED_LIB := $(UNSIGNED_BUILD_DIR)/unsigned.a


# ============================================================================
# Toolchain / optimization
# ============================================================================

# unsigned owns its optimization policy. These flags are only applied to
# unsigned sources, not to the game's sources.
UNSIGNED_CFLAGS := -O2 -flto

# LTO must also be enabled during the final link. Because unsigned.mk is
# included by the game Makefile, the game does not have to enable it itself.
LDFLAGS += -flto

# GCC wrappers ensure the linker plugin can correctly handle LTO objects
# stored inside the static archive. They can be overridden by the parent
# Makefile/toolchain configuration if necessary.
UNSIGNED_AR ?= m68k-neogeo-elf-gcc-ar
UNSIGNED_RANLIB ?= m68k-neogeo-elf-gcc-ranlib


# ============================================================================
# Sources
# ============================================================================

UNSIGNED_SRCS := $(shell find $(UNSIGNED_SRC_DIR) -type f -name '*.c')

UNSIGNED_OBJS := $(patsubst \
	$(UNSIGNED_SRC_DIR)/%.c, \
	$(UNSIGNED_BUILD_DIR)/%.o, \
	$(UNSIGNED_SRCS))

UNSIGNED_DEPS := $(UNSIGNED_OBJS:.o=.d)


# ============================================================================
# Includes
# ============================================================================

CFLAGS += -I$(UNSIGNED_INCLUDE_DIR)


# ============================================================================
# Compilation
# ============================================================================

$(UNSIGNED_BUILD_DIR)/%.o: $(UNSIGNED_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(M68KGCC) $(NGCFLAGS) $(CFLAGS) $(UNSIGNED_CFLAGS) -MMD -MP -c $< -o $@


# ============================================================================
# Library
# ============================================================================

$(UNSIGNED_LIB): $(UNSIGNED_OBJS)
	@mkdir -p $(dir $@)
	$(UNSIGNED_AR) rcs $@ $^
	$(UNSIGNED_RANLIB) $@

# Rebuild affected objects when an included header changes. Missing dependency
# files are expected on the first build; -MP also tolerates removed headers.
-include $(UNSIGNED_DEPS)
