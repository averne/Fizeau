export FZ_VERSION =    1.0.0
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

.PHONY: application sysmodule dist clean mrproper

dist: $(DIST_TARGET)
	@:

$(DIST_TARGET): application/out/Fizeau.nro sysmodule/out/Fizeau.nsp misc/default.ini
	@$(MAKE) application sysmodule all --no-print-directory

	@rm -rf out/Fizeau*.zip
	@mkdir -p out/switch/Fizeau
	@cp misc/default.ini out/switch/Fizeau/config.ini
	@cp application/out/Fizeau.nro out/switch/Fizeau/Fizeau.nro

	@mkdir -p out/atmosphere/contents/$(FZ_TID)/flags
	@cp sysmodule/out/Fizeau.nsp out/atmosphere/contents/$(FZ_TID)/exefs.nsp
	@touch out/atmosphere/contents/$(FZ_TID)/flags/boot2.flag

	@7z a $@ ./out/atmosphere ./out/switch >/dev/null
	@rm -r out/atmosphere out/switch
	@echo "Compressed release to $@.zip"

clean:
	@rm -rf out

mrproper: clean

application:
	@$(MAKE) -C $@ $(filter-out $@,$(MAKECMDGOALS)) --no-print-directory

sysmodule:
	@$(MAKE) -C $@ $(filter-out $@,$(MAKECMDGOALS)) --no-print-directory

%:
	@:
