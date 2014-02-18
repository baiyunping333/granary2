# Copyright 2014 Peter Goodman, all rights reserved.

include Makefile.inc

.PHONY: all all_objects clean clean_generated install user_tool kernel_tool

# Compile all files. This passes in `GRANARY_SRC_DIR` through to all sub-
# invocations of `make`.
all_objects:
	@echo "Entering $(GRANARY_SRC_DIR)/dependencies/$(GRANARY_DRIVER)"
	$(MAKE) -C $(GRANARY_SRC_DIR)/dependencies/$(GRANARY_DRIVER) \
		$(MFLAGS) GRANARY_SRC_DIR=$(GRANARY_SRC_DIR) all
	@echo "Entering $(GRANARY_SRC_DIR)/dependencies/xxhash"
	$(MAKE) -C $(GRANARY_SRC_DIR)/dependencies/xxhash \
		$(MFLAGS) GRANARY_SRC_DIR=$(GRANARY_SRC_DIR) all
	@echo "Entering $(GRANARY_SRC_DIR)/granary"
	$(MAKE) -C $(GRANARY_SRC_DIR)/granary \
		$(MFLAGS) GRANARY_SRC_DIR=$(GRANARY_SRC_DIR) all
	@echo "Entering $(GRANARY_WHERE_DIR)"
	$(MAKE) -C $(GRANARY_WHERE_DIR) \
		$(MFLAGS) GRANARY_SRC_DIR=$(GRANARY_SRC_DIR) all

# Make a final object that tools can link against for getting arch-specific
# implementations of built-in compiler functions that are also sometimes
# synthesized by optimizing compilers (e.g. memset).
user_tool: all_objects
	@echo "Building object $(GRANARY_BIN_DIR)/granary/breakpoint.o"
	@$(GRANARY_CXX) -c $(GRANARY_BIN_DIR)/granary/breakpoint.ll \
    	-o $(GRANARY_BIN_DIR)/granary/breakpoint.o
    	
	@echo "Loading user space $(GRANARY_BIN_DIR)/tool.o"
	@$(GRANARY_LD) -r \
    	$(GRANARY_BIN_DIR)/granary/arch/$(GRANARY_ARCH)/asm/string.o \
    	$(GRANARY_BIN_DIR)/granary/breakpoint.o \
    	-o $(GRANARY_BIN_DIR)/tool.o
	
# We handle the equivalent of `user_tool` in `granary/kernel/Took.mk`.
kernel_tool: all_objects
    	
all: $(GRANARY_WHERE)_tool
	@echo "Done."

# Clean up all executable / binary files.
clean:
	@echo "Removing all previously compiled files."
	@-rm $(GRANARY_BIN_DIR)/grr > /dev/null 2>&1 ||:  # User space injecter
	@find $(GRANARY_BIN_DIR) -type f -name \*.so -execdir rm {} \;
	@find $(GRANARY_BIN_DIR) -type f -name \*.ll -execdir rm {} \;
	@find $(GRANARY_BIN_DIR) -type f -name \*.o -execdir rm {} \;
	@find $(GRANARY_BIN_DIR) -type f -name \*.o_shipped -execdir rm {} \;
	@find $(GRANARY_BIN_DIR) -type f -name \*.o.cmd -execdir rm {} \;
	@find $(GRANARY_BIN_DIR) -type f -name \*.S -execdir rm {} \;
	@find $(GRANARY_BIN_DIR) -type f -name \*.out -execdir rm {} \;
	@find $(GRANARY_BIN_DIR) -type f -name \*.a -execdir rm {} \;

# Clean up all auto-generated files.
clean_generated:
	@echo "Removing all previously auto-generated files."
	@find $(GRANARY_GEN_SRC_DIR) -type f -execdir rm {} \;

# Make a header file that external tools can use to define tools.
headers:
	@mkdir -p $(GRANARY_EXPORT_HEADERS_DIR)
	@$(GRANARY_PYTHON) $(GRANARY_SRC_DIR)/scripts/generate_export_headers.py \
		$(GRANARY_WHERE) $(GRANARY_SRC_DIR) $(GRANARY_EXPORT_HEADERS_DIR)

# Install libgranary.so onto the OS.
install: all headers
	cp $(GRANARY_BIN_DIR)/libgranary.so $(GRANARY_EXPORT_LIB_DIR)

# Compile one or more specific tools. For example:
# `make tools GRANARY_TOOLS=bbcount`.
tools:
	$(MAKE) -C $(GRANARY_SRC_DIR)/granary/$(GRANARY_WHERE) -f Tool.mk \
		$(MFLAGS) \
		GRANARY_SRC_DIR=$(GRANARY_SRC_DIR) \
		GRANARY_TOOL_DIR=$(GRANARY_TOOL_DIR) all

# Clean one or more specific tools. For example:
# `make clean_tools GRANARY_TOOLS=bbcount`.
clean_tools:
	$(MAKE) -C $(GRANARY_SRC_DIR)/granary/$(GRANARY_WHERE) -f Tool.mk \
		$(MFLAGS) \
		GRANARY_SRC_DIR=$(GRANARY_SRC_DIR) \
		GRANARY_TOOL_DIR=$(GRANARY_TOOL_DIR) clean
