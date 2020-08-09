export FZ_VERSION =    2.0.0
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

.PHONY: all application sysmodule overlay dist clean mrproper

all: application sysmodule overlay
	@:

dist: $(DIST_TARGET)
	@:

$(DIST_TARGET): | all
	@rm -rf $(OUT)/Fizeau*.zip

	@mkdir -p $(OUT)/switch/Fizeau
	@cp misc/default.ini $(OUT)/switch/Fizeau/config.ini
	@cp application/out/Fizeau.nro $(OUT)/switch/Fizeau/Fizeau.nro

	@mkdir -p $(OUT)/switch/.overlays
	@cp overlay/out/Fizeau.ovl $(OUT)/switch/.overlays/Fizeau.ovl

	@mkdir -p $(OUT)/atmosphere/contents/$(FZ_TID)/flags
	@cp sysmodule/out/Fizeau.nsp $(OUT)/atmosphere/contents/$(FZ_TID)/exefs.nsp
	@cp sysmodule/toolbox.json $(OUT)/atmosphere/contents/$(FZ_TID)/toolbox.json
	@touch $(OUT)/atmosphere/contents/$(FZ_TID)/flags/boot2.flag

	@7z a $@ ./$(OUT)/atmosphere ./$(OUT)/switch >/dev/null
	@rm -r $(OUT)/atmosphere $(OUT)/switch
	@echo Compressed release to $@

clean:
	@rm -rf out

mrproper: clean
	@$(MAKE) --no-print-directory -C application mrproper
	@$(MAKE) --no-print-directory -C sysmodule mrproper
	@$(MAKE) --no-print-directory -C overlay mrproper

application:
	@$(MAKE) -s -C $@ $(filter-out $@ dist,$(MAKECMDGOALS)) --no-print-directory

sysmodule:
	@$(MAKE) -s -C $@ $(filter-out $@ dist,$(MAKECMDGOALS)) --no-print-directory

overlay:
	@$(MAKE) -s -C $@ $(filter-out $@ dist,$(MAKECMDGOALS)) --no-print-directory

%:
	@:
