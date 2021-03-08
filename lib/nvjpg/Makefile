TARGET            =    lib$(notdir $(CURDIR))
EXTENSION         =    a
OUT               =    lib

# -----------------------------------------------

LIB_TARGET        =    $(if $(OUT:=), $(OUT)/$(TARGET).$(EXTENSION), .$(OUT)/$(TARGET).$(EXTENSION))

# -----------------------------------------------

.SUFFIXES:

.PHONY: all clean

all: $(LIB_TARGET)

$(LIB_TARGET):
	@$(MAKE) -s --no-print-directory -C oss-nvjpg all
	@mkdir -p $(OUT)
	@cp oss-nvjpg/out/* $(OUT)
	@echo "Built" $(notdir $@)

clean:
	@$(MAKE) -s --no-print-directory -C oss-nvjpg clean
	@rm -rf $(OUT)
