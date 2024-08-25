.PHONY: all help

HAS_DEBUG ?= 0
GNU_INSTALL_ROOT ?= $(CURDIR)/nrf-sdk/gcc-arm-none-eabi-6-2017-q2-update


TARGETS := nrf51822 nrf52810

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - build all targets"
	@echo "  clean      - clean all targets"

# Define a recipe to build each target individually
define build_target
.PHONY: $(1)
$(1):
	$$(MAKE) -C $(1)/armgcc \
		GNU_INSTALL_ROOT=$$(if $$(findstring nrf51,$(1)),$(GNU_INSTALL_ROOT)/,$(GNU_INSTALL_ROOT)/bin/) \
		MAX_KEYS=$$(if $$(findstring nrf51,$(1)),50,100) \
		HAS_DEBUG=$(HAS_DEBUG) \
		bin

	mkdir -p ./release
	cp $(1)/armgcc/_build/*.bin ./release/

$(1)-clean:
	$$(MAKE) -C $(1)/armgcc clean \
		GNU_INSTALL_ROOT=$$(if $$(findstring nrf51,$(1)),$(GNU_INSTALL_ROOT)/,$(GNU_INSTALL_ROOT)/bin/) \

endef

# Generate rules for each target in the TARGETS list
$(foreach target,$(TARGETS),$(eval $(call build_target,$(target))))

# Define all target to depend on all individual targets
all: $(TARGETS)

clean: $(foreach target,$(TARGETS),$(target)-clean)
	rm -rf ./release

