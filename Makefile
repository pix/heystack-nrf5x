.PHONY: all help

HAS_DEBUG ?= 0
GNU_INSTALL_ROOT ?= $(CURDIR)/nrf-sdk/gcc-arm-none-eabi-6-2017-q2-update


TARGETS := nrf51822_xxac nrf52810_xxaa nrf52832_xxaa nrf52832_yj17024

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - build all targets"
	@echo "  clean      - clean all targets"

# Define a recipe to build each target individually
define build_target
.PHONY: $(1)
DIR_$(1) := $(shell echo $(1) | cut -d'_' -f1)
$(1):
	$$(MAKE) -C $$(DIR_$(1))/armgcc \
		GNU_INSTALL_ROOT=$$(if $$(findstring nrf51,$$(DIR_$(1))),$$(GNU_INSTALL_ROOT)/,$$(GNU_INSTALL_ROOT)/bin/) \
		MAX_KEYS=500 \
		HAS_DEBUG=$(HAS_DEBUG) \
		$(1) bin_$(1)

	mkdir -p ./release
	cp $$(DIR_$(1))/armgcc/_build/*_s???.bin ./release/

$(1)-clean:
	$$(MAKE) -C $$(DIR_$(1))/armgcc clean \
		GNU_INSTALL_ROOT=$$(if $$(findstring nrf51,$$(DIR_$(1))),$$(GNU_INSTALL_ROOT)/,$$(GNU_INSTALL_ROOT)/bin/)

endef

# Generate rules for each target in the TARGETS list
$(foreach target,$(TARGETS),$(eval $(call build_target,$(target))))

# Define all target to depend on all individual targets
all: $(TARGETS)

clean: $(foreach target,$(TARGETS),$(target)-clean)
	rm -rf ./release
