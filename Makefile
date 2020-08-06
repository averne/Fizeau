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

.PHONY: all application sysmodule dist clean mrproper

all: application sysmodule
	@:

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
	@cp sysmodule/toolbox.json out/atmosphere/contents/$(FZ_TID)/toolbox.json
	@touch out/atmosphere/contents/$(FZ_TID)/flags/boot2.flag

	@7z a $@ ./out/atmosphere ./out/switch >/dev/null
	@rm -r out/atmosphere out/switch
	@echo Compressed release to $@

clean:
	@rm -rf out

mrproper: clean
	@$(MAKE) --no-print-directory -C application mrproper
	@$(MAKE) --no-print-directory -C sysmodule mrproper

application:
	@$(MAKE) -C $@ $(filter-out $@,$(MAKECMDGOALS)) --no-print-directory

sysmodule:
	@$(MAKE) -C $@ $(filter-out $@,$(MAKECMDGOALS)) --no-print-directory

%:
	@:
