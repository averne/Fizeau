ifeq ($(strip $(DEVKITPRO)),)
    $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR           ?=    $(CURDIR)

# -----------------------------------------------

APP_TITLE         =    Fizeau
APP_TITLEID       =

TARGET            =    Fizeau
EXTENSION         =    nsp
OUT               =    out
BUILD             =    build
SOURCES           =    src ../common
INCLUDES          =    include ../common/ \
                       ../lib/libstratosphere/libstratosphere/libvapours/include
CUSTOM_LIBS       =    ../lib/libstratosphere
NPDM_JSON         =    config.json

DEFINES           =    __SWITCH__
ARCH              =    -march=armv8-a+crc+crypto+simd -mtune=cortex-a57 -mtp=soft -fpie
FLAGS             =    -Wall -pipe -g -O2 -ffunction-sections -fdata-sections
CFLAGS            =    -std=gnu11
CXXFLAGS          =    -std=gnu++17 -fno-rtti -fno-exceptions
ASFLAGS           =
LDFLAGS           =    -Wl,-pie -specs=$(DEVKITPRO)/libnx/switch.specs -g
LINKS             =    -lstratosphere -lnx

PREFIX            =    aarch64-none-elf-
CC                =    $(PREFIX)gcc
CXX               =    $(PREFIX)g++
AS                =    $(PREFIX)as
LD                =    $(PREFIX)g++
NM                =    $(PREFIX)gcc-nm

# -----------------------------------------------

export PATH      :=    $(DEVKITPRO)/tools/bin:$(DEVKITPRO)/devkitA64/bin:$(PORTLIBS)/bin:$(PATH)

PORTLIBS          =    $(DEVKITPRO)/portlibs/switch
LIBNX             =    $(DEVKITPRO)/libnx
LIBS              =    $(CUSTOM_LIBS) $(LIBNX) $(PORTLIBS)

# -----------------------------------------------

CFILES            =    $(shell find $(SOURCES) -name *.c)
CPPFILES          =    $(shell find $(SOURCES) -name *.cpp)
SFILES            =    $(shell find $(SOURCES) -name *.s -or -name *.S)
OFILES            =    $(CFILES:%=$(BUILD)/%.o) $(CPPFILES:%=$(BUILD)/%.o) $(SFILES:%=$(BUILD)/%.o)
DFILES            =    $(OFILES:.o=.d)

LIBS_TARGET       =    $(shell find $(addsuffix /lib,$(CUSTOM_LIBS)) -name "*.a" 2>/dev/null)
NX_TARGET         =    $(if $(OUT:=), $(OUT)/$(TARGET).$(EXTENSION), .$(OUT)/$(TARGET).$(EXTENSION))
ELF_TARGET        =    $(if $(OUT:=), $(OUT)/$(TARGET).elf, .$(OUT)/$(TARGET).elf)
NSO_TARGET        =    $(if $(BUILD:=), $(BUILD)/$(TARGET).nso, .$(BUILD)/$(TARGET).nso)
NPDM_TARGET       =    $(if $(OUT:=), $(OUT)/$(TARGET).npdm, .$(OUT)/$(TARGET).npdm)

DEFINE_FLAGS      =    $(addprefix -D,$(DEFINES))
INCLUDE_FLAGS     =    $(addprefix -I$(CURDIR)/,$(INCLUDES)) $(foreach dir,$(CUSTOM_LIBS),-I$(CURDIR)/$(dir)/include) \
					   $(foreach dir,$(filter-out $(CUSTOM_LIBS),$(LIBS)),-I$(dir)/include)
LIB_FLAGS         =    $(foreach dir,$(LIBS),-L$(dir)/lib)

# -----------------------------------------------

.SUFFIXES:

.PHONY: all libs clean mrproper $(CUSTOM_LIBS)

all: $(NX_TARGET)

libs: $(CUSTOM_LIBS)

$(CUSTOM_LIBS):
	@$(MAKE) -s --no-print-directory -C $@

$(NX_TARGET): $(NSO_TARGET) $(NPDM_TARGET)
	@echo " NSP " $@
	@mkdir -p $(BUILD)/exefs
	@cp $(NSO_TARGET) $(BUILD)/exefs/main
	@cp $(NPDM_TARGET) $(BUILD)/exefs/main.npdm
	@build_pfs0 $(BUILD)/exefs $@

$(NSO_TARGET): $(ELF_TARGET)
	@echo " NSO " $@
	@mkdir -p $(dir $@)
	@elf2nso $< $@

$(ELF_TARGET): $(OFILES) | libs
	@echo " LD  " $@
	@mkdir -p $(dir $@)
	@$(LD) $(ARCH) $(LDFLAGS) -Wl,-Map,$(BUILD)/$(TARGET).map $(LIB_FLAGS) $(OFILES) $(LINKS) -o $@
	@$(NM) -CSn $@ > $(BUILD)/$(TARGET).lst

$(BUILD)/%.c.o: %.c
	@echo " CC  " $@
	@mkdir -p $(dir $@)
	@$(CC) -MMD -MP $(ARCH) $(FLAGS) $(CFLAGS) $(DEFINE_FLAGS) $(INCLUDE_FLAGS) -c $(CURDIR)/$< -o $@

$(BUILD)/%.cpp.o: %.cpp
	@echo " CXX " $@
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP $(ARCH) $(FLAGS) $(CXXFLAGS) $(DEFINE_FLAGS) $(INCLUDE_FLAGS) -c $(CURDIR)/$< -o $@

$(BUILD)/%.s.o: %.s %.S
	@echo " AS  " $@
	@mkdir -p $(dir $@)
	@$(AS) -MMD -MP -x assembler-with-cpp $(ARCH) $(FLAGS) $(ASFLAGS) $(INCLUDE_FLAGS) -c $(CURDIR)/$< -o $@

%.npdm: $(NPDM_JSON)
	@echo " NPDM" $@
	@mkdir -p $(dir $@)
	@npdmtool $(NPDM_JSON) $@

clean:
	@echo Cleaning...
	@rm -rf $(BUILD) $(OUT)

mrproper: clean
	@for dir in $(CUSTOM_LIBS); do $(MAKE) --no-print-directory -C $$dir clean; done

application:
	@:

-include $(DFILES)
