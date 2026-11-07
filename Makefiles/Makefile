# Makefiles/Makefile
# Inker Project
# Copyright (C) 2026 Inker Developer kljj04

.DEFAULT_GOAL := All

MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

RWildcard = $(foreach Dir,$(wildcard $1*),$(call RWildcard,$(Dir)/,$2) $(filter $(subst *,%,$2),$(Dir)))

CXX := clang++
LD := clang++
CLANG_FORMAT := clang-format

SOURCE_DIR := Inker
INCLUDE_DIR := $(SOURCE_DIR)/Include

BUILD_DIR := Build
OBJECT_DIR := $(BUILD_DIR)/Objects
TARGET := $(BUILD_DIR)/inker
INKERD_TARGET := $(BUILD_DIR)/inkerd

CXXFLAGS := -std=c++23 -O2 -Wall -Wextra -Wpedantic -IInker/Common -IInker/Include
CXXFLAGS += -I$(SOURCE_DIR) -I$(INCLUDE_DIR) -IInkerd/Include

LDFLAGS :=
LDLIBS :=

SOURCES := $(sort $(call RWildcard,$(SOURCE_DIR)/,*.cpp))
OBJECTS := $(patsubst $(SOURCE_DIR)/%.cpp,$(OBJECT_DIR)/%.o,$(SOURCES))
COMMON_OBJECTS := $(filter-out $(OBJECT_DIR)/Source/Inker.o,$(OBJECTS))
INKERD_SOURCES := $(sort $(call RWildcard,Inkerd/,*.cpp))
INKERD_OBJECTS := $(patsubst Inkerd/%.cpp,$(OBJECT_DIR)/Inkerd/%.o,$(INKERD_SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d) $(INKERD_OBJECTS:.o=.d)

FORMAT_FILES := $(sort $(call RWildcard,$(SOURCE_DIR)/,*.cpp) $(call RWildcard,$(SOURCE_DIR)/,*.hpp) $(call RWildcard,$(SOURCE_DIR)/,*.h) $(call RWildcard,Inkerd/,*.cpp) $(call RWildcard,Inkerd/,*.hpp))

.PHONY: All Inker Inkerd Clean Run Format Rebuild

All: $(TARGET) $(INKERD_TARGET)

Inker: $(TARGET)

Inkerd: $(INKERD_TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	@$(LD) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(INKERD_TARGET): $(COMMON_OBJECTS) $(INKERD_OBJECTS)
	@mkdir -p $(dir $@)
	@$(LD) $(LDFLAGS) $(COMMON_OBJECTS) $(INKERD_OBJECTS) $(LDLIBS) -o $@

$(OBJECT_DIR)/Inkerd/%.o: Inkerd/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

Clean:
	@rm -rf $(BUILD_DIR)

Run: $(TARGET)
	@"$(TARGET)" $(ARGS)

Format:
	@$(CLANG_FORMAT) -i $(FORMAT_FILES)

Rebuild:
	@$(MAKE) -s -f "$(MAKEFILE_DIR)/Makefile" Clean
	@$(MAKE) -s -f "$(MAKEFILE_DIR)/Makefile" All

-include $(DEPENDENCIES)