ifeq ($(strip $(DEVKITPRO)),)
    $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR           ?=    $(CURDIR)

# -----------------------------------------------

APP_TITLE         =    Fizeau
APP_AUTHOR        =    averne
APP_ICON          =    $(CURDIR)/icon.jpg
APP_VERSION       =    $(FZ_VERSION)
APP_TITLEID       =

TARGET            =    Fizeau
EXTENSION         =    nro
OUT               =    out
BUILD             =    build
SOURCES           =    src
INCLUDES          =    include ../lib/libstratosphere/libstratosphere/include ../lib/nvjpg/oss-nvjpg/include
CUSTOM_LIBS       =    ../common ../lib/imgui-nx ../lib/imgui ../lib/inih ../lib/nvjpg
ROMFS             =    res

DEFINES           =    __SWITCH__ VERSION=\"$(FZ_VERSION)\" COMMIT=\"$(FZ_COMMIT)\" \
                       APPLICATION NXLINK
ARCH              =    -march=armv8-a+crc+crypto+simd -mtune=cortex-a57 -mtp=soft -fpie
FLAGS             =    -Wall -pipe -g -O2 -ffunction-sections -fdata-sections
CFLAGS            =    -std=gnu11
CXXFLAGS          =    -std=gnu++20 -fno-rtti -fno-exceptions
ASFLAGS           =
LDFLAGS           =    -Wl,-pie -specs=$(DEVKITPRO)/libnx/switch.specs -g
LINKS             =    -lcommon -limgui-nx -limgui -lnvjpg -linih -ldeko3d -lnx

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
NACP_TARGET       =    $(if $(OUT:=), $(OUT)/$(TARGET).nacp, .$(OUT)/$(TARGET).nacp)

DEFINE_FLAGS      =    $(addprefix -D,$(DEFINES))
INCLUDE_FLAGS     =    $(addprefix -I$(CURDIR)/,$(INCLUDES)) $(foreach dir,$(CUSTOM_LIBS),-I$(CURDIR)/$(dir)/include) \
                       $(foreach dir,$(filter-out $(CUSTOM_LIBS),$(LIBS)),-I$(dir)/include)
LIB_FLAGS         =    $(foreach dir,$(LIBS),-L$(dir)/lib)

# -----------------------------------------------

ifeq ($(strip $(APP_TITLE)),)
    APP_TITLE     =    $(TARGET)
endif

ifeq ($(strip $(APP_AUTHOR)),)
    APP_AUTHOR    =    Unspecified
endif

ifeq ($(strip $(APP_VERSION)),)
    APP_VERSION   =    Unspecified
endif

ifneq ($(APP_TITLEID),)
    NACPFLAGS    +=    --titleid=$(strip $(APP_TITLEID))
endif

ifeq ($(strip $(APP_ICON)),)
    APP_ICON      =    $(LIBNX)/default_icon.jpg
endif

NROFLAGS          =    --icon=$(strip $(APP_ICON)) --nacp=$(strip $(NACP_TARGET))

ifneq ($(ROMFS),)
    NROFLAGS     +=    --romfsdir=$(strip $(ROMFS))
    ROMFS_TARGET +=    $(shell find $(ROMFS) -type 'f')
endif

# -----------------------------------------------

.SUFFIXES:

.PHONY: all libs run clean mrproper $(CUSTOM_LIBS)

all: $(NX_TARGET)

run: all
	@echo "Running" $(NX_TARGET)
	@nxlink -r 100 -p Fizeau/Fizeau.nro -s $(NX_TARGET)

libs: $(CUSTOM_LIBS)

shaders: ../lib/imgui-nx
	@mkdir -p $(ROMFS)
	$(shell find ../lib/imgui-nx/lib/ -type f -name '*.dksh' -exec rsync -u {} $(ROMFS) \;)

$(CUSTOM_LIBS):
	@$(MAKE) -s --no-print-directory -C $@

$(NX_TARGET): $(ROMFS_TARGET) $(APP_ICON) $(NACP_TARGET) $(ELF_TARGET) | shaders
	@echo " NRO " $@
	@mkdir -p $(dir $@)
	@elf2nro $(ELF_TARGET) $@ $(NROFLAGS) > /dev/null
	@echo "Built" $(notdir $@)

$(ELF_TARGET): $(OFILES) $(LIBS_TARGET) | libs
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

%.nacp:
	@echo " NACP" $@
	@mkdir -p $(dir $@)
	@nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@ $(NACPFLAGS)

clean:
	@echo Cleaning...
	@rm -rf $(BUILD) $(OUT)

mrproper: clean
	@for dir in $(CUSTOM_LIBS); do $(MAKE) --no-print-directory -C $$dir clean; done

-include $(DFILES)
