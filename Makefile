export FZ_VERSION =    2.8.0
export FZ_COMMIT  =    $(shell git rev-parse --short HEAD)
export FZ_TID     =    0100000000000F12

# Vscode fails to parse non-english error codes
export LANG       =    en

export TOPDIR     =    $(CURDIR)
OUT               =    out

# -----------------------------------------------

ifneq (, $(strip $(shell git status --porcelain 2>/dev/null)))
export FZ_COMMIT    :=    $(addsuffix -dirty,$(FZ_COMMIT))
endif

DIST_TARGET       =    $(OUT)/Fizeau-$(FZ_VERSION)-$(FZ_COMMIT).zip

# -----------------------------------------------

MODULES           =    application sysmodule overlay

.PHONY: all dist clean mrproper $(MODULES)

all: $(MODULES)
	@:

dist: $(DIST_TARGET)
	@:

$(DIST_TARGET): | all
	@rm -rf $(OUT)/Fizeau-*-*.zip

	@mkdir -p $(OUT)/config/Fizeau
	@mkdir -p $(OUT)/switch/Fizeau
	@cp misc/default.ini $(OUT)/config/Fizeau/config.ini
	@cp application/out/Fizeau.nro $(OUT)/switch/Fizeau/Fizeau.nro

	@mkdir -p $(OUT)/switch/.overlays
	@cp overlay/out/Fizeau.ovl $(OUT)/switch/.overlays/Fizeau.ovl

	@mkdir -p $(OUT)/atmosphere/contents/$(FZ_TID)/flags
	@cp sysmodule/out/Fizeau.nsp $(OUT)/atmosphere/contents/$(FZ_TID)/exefs.nsp
	@cp sysmodule/toolbox.json $(OUT)/atmosphere/contents/$(FZ_TID)/toolbox.json
	@touch $(OUT)/atmosphere/contents/$(FZ_TID)/flags/boot2.flag

	@mkdir -p $(OUT)/atmosphere/exefs_patches/nvnflinger_cmu
	@cp misc/patches/*.ips $(OUT)/atmosphere/exefs_patches/nvnflinger_cmu || :

	@7z a $@ ./$(OUT)/atmosphere ./$(OUT)/config ./$(OUT)/switch >/dev/null
	@rm -r $(OUT)/atmosphere $(OUT)/config $(OUT)/switch
	@echo Compressed release to $@

clean:
	@rm -rf out

mrproper: clean
	@for dir in $(MODULES); do $(MAKE) --no-print-directory -C $$dir mrproper; done

application:
	@$(MAKE) -s -C $@ $(filter-out $(MODULES) dist,$(MAKECMDGOALS)) --no-print-directory

sysmodule:
	@$(MAKE) -s -C $@ $(filter-out $(MODULES) dist,$(MAKECMDGOALS)) --no-print-directory

overlay:
	@$(MAKE) -s -C $@ $(filter-out $(MODULES) dist,$(MAKECMDGOALS)) --no-print-directory

%:
	@:
