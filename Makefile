export FZ_VERSION =    2.1.9
export FZ_COMMIT  =    $(shell git rev-parse --short HEAD)
export FZ_TID     =    0100000000000F12
export FZ_CHL_TID =    010000000000CF12

# Vscode fails to parse non-english error codes
export LANG       =    en

export TOPDIR     =    $(CURDIR)
OUT               =    out

# -----------------------------------------------

ifneq (, $(strip $(shell git status --porcelain 2>/dev/null)))
export FZ_COMMIT    :=    $(addsuffix -dirty,$(FZ_COMMIT))
endif

DIST_TARGET       =    $(OUT)/Fizeau-$(FZ_VERSION)-$(FZ_COMMIT).zip
DIST_CHL_TARGET   =    $(OUT)/Fizeau-chl-$(FZ_VERSION)-$(FZ_COMMIT).zip

# -----------------------------------------------

MODULES           =    application sysmodule overlay chainloader

.PHONY: all dist dist-chl clean mrproper $(MODULES)

all: $(MODULES)
	@:

dist: $(DIST_TARGET)
	@:

$(DIST_TARGET): | application overlay sysmodule
	@rm -rf $(OUT)/Fizeau*.zip

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

	@7z a $@ ./$(OUT)/atmosphere ./$(OUT)/config ./$(OUT)/switch >/dev/null
	@rm -r $(OUT)/atmosphere $(OUT)/config $(OUT)/switch
	@echo Compressed release to $@

dist-chl: $(DIST_CHL_TARGET)
	@:

$(DIST_CHL_TARGET): | all
	@rm -rf $(OUT)/Fizeau-chl*.zip

	@mkdir -p $(OUT)/config/Fizeau
	@mkdir -p $(OUT)/switch/Fizeau
	@cp misc/default.ini $(OUT)/config/Fizeau/config.ini
	@cp application/out/Fizeau.nro $(OUT)/switch/Fizeau/Fizeau.nro

	@mkdir -p $(OUT)/switch/.overlays
	@cp overlay/out/Fizeau.ovl $(OUT)/switch/.overlays/Fizeau.ovl

	@mkdir -p $(OUT)/atmosphere/contents/$(FZ_TID)
	@cp sysmodule/out/Fizeau.nsp $(OUT)/atmosphere/contents/$(FZ_TID)/exefs.nsp

	@mkdir -p $(OUT)/atmosphere/contents/$(FZ_CHL_TID)/flags
	@cp chainloader/out/Fizeau-chl.nsp $(OUT)/atmosphere/contents/$(FZ_CHL_TID)/exefs.nsp
	@cp chainloader/toolbox.json $(OUT)/atmosphere/contents/$(FZ_TID)/toolbox.json
	@touch $(OUT)/atmosphere/contents/$(FZ_CHL_TID)/flags/boot2.flag

	@7z a $@ ./$(OUT)/atmosphere ./$(OUT)/config ./$(OUT)/switch >/dev/null
	@rm -r $(OUT)/atmosphere $(OUT)/config $(OUT)/switch
	@echo Compressed release to $@

clean:
	@rm -rf out

mrproper: clean
	@for dir in $(MODULES); do $(MAKE) --no-print-directory -C $$dir mrproper; done

application:
	@$(MAKE) -s -C $@ $(filter-out $(MODULES) dist dist-chl,$(MAKECMDGOALS)) --no-print-directory

sysmodule:
	@$(MAKE) -s -C $@ $(filter-out $(MODULES) dist dist-chl,$(MAKECMDGOALS)) --no-print-directory

overlay:
	@$(MAKE) -s -C $@ $(filter-out $(MODULES) dist dist-chl,$(MAKECMDGOALS)) --no-print-directory

chainloader:
	@$(MAKE) -s -C $@ $(filter-out $(MODULES) dist dist-chl,$(MAKECMDGOALS)) --no-print-directory

%:
	@:
