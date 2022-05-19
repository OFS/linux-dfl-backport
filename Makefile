# Copyright(c) 2022, Intel Corporation

KERNEL ?= $(shell uname -r)
KERNELDIR ?= /lib/modules/$(KERNEL)/build
LINUXINCLUDE := -I$(src)/include -I$(src)/include/uapi $(LINUXINCLUDE)

# make Kconfig conditionals always work
include $(KERNELDIR)/.config

ifeq ($(DEBUG),1)
DYNDBG = dyndbg=+p
endif

RPMBUILDOPTS = -bb --build-in-place \
               --define '_topdir /tmp/rpmbuild' \
               --define '_rpmdir .' \
               --define '_build_name_fmt %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm' \
               --define '_modules $(reverse)'

ifeq ($(BACKPORT_VERSION),)
ifneq ($(wildcard .git),)
date := $(shell date +%Y.%m.%d)
version_string := $(shell git describe --first-parent --long --match 'intel*' \
		      --always --tags --dirty | \
		      sed -E "s/^intel-//;s/^v//;s/[^-]+-g/${date}-g/")
BACKPORT_VERSION := $(shell echo ${version_string} | sed -E "s/-.*//")
BACKPORT_RELEASE := $(shell echo ${version_string} | sed -E "s/^[^-]+-//;s/-/./g")
endif
endif

ifeq ($(BACKPORT_RELEASE),)
BACKPORT_RELEASE := 1
endif

export BACKPORT_VERSION
export BACKPORT_RELEASE

ifndef CONFIG_REGMAP_MMIO
obj-m += regmap-mmio.o
endif

# modules to build (in insmod order)
ifndef CONFIG_REGMAP_SPI_AVMM
obj-m += regmap-spi-avmm.o
endif

# The module order matters; it determines the module order for
# both the insmod and rmmod targets. The module order is also
# leveraged for install packages by the dkms.conf file.
obj-m += fpga-mgr.o
obj-m += fpga-bridge.o
obj-m += fpga-region.o
obj-m += fpga-image-load.o
obj-m += dfl.o
obj-m += dfl-fme.o
obj-m += dfl-afu.o
obj-m += dfl-intel-s10-iopll.o
obj-m += dfl-fme-mgr.o
obj-m += dfl-fme-region.o
obj-m += dfl-fme-br.o
obj-m += dfl-hssi.o
obj-m += dfl-n3000-nios.o
obj-m += dfl-emif.o
obj-m += uio-dfl.o
obj-m += spi-altera-core.o
obj-m += spi-altera-dfl.o
obj-m += regmap-indirect-register.o
obj-m += qsfp-mem.o
obj-m += 8250_dfl.o
obj-m += intel-s10-phy.o
obj-m += intel-m10-bmc.o
obj-m += intel-m10-bmc-spi.o
obj-m += intel-m10-bmc-pmci.o
obj-m += intel-m10-bmc-log.o
obj-m += intel-m10-bmc-hwmon.o
obj-m += intel-m10-bmc-sec-update.o
obj-m += ptp_dfl_tod.o
obj-m += s10hssi.o
obj-m += n5010-phy.o
obj-m += n5010-hssi.o
obj-m += dfl-pci.o

ptp_dfl_tod-y := drivers/ptp/ptp_dfl_tod.o
regmap-mmio-y := drivers/base/regmap/regmap-mmio.o
regmap-spi-avmm-y := drivers/base/regmap/regmap-spi-avmm.o
dfl-y := drivers/fpga/dfl.o

dfl-afu-y := drivers/fpga/dfl-afu-main.o
dfl-afu-y += drivers/fpga/dfl-afu-region.o
dfl-afu-y += drivers/fpga/dfl-afu-dma-region.o
dfl-afu-y += drivers/fpga/dfl-afu-error.o

dfl-fme-y := drivers/fpga/dfl-fme-main.o
dfl-fme-y += drivers/fpga/dfl-fme-pr.o
dfl-fme-y += drivers/fpga/dfl-fme-perf.o
dfl-fme-y += drivers/fpga/dfl-fme-error.o

dfl-fme-br-y := drivers/fpga/dfl-fme-br.o
dfl-fme-mgr-y := drivers/fpga/dfl-fme-mgr.o
dfl-fme-region-y := drivers/fpga/dfl-fme-region.o
dfl-intel-s10-iopll-y := drivers/fpga/dfl-intel-s10-iopll.o
dfl-pci-y := drivers/fpga/dfl-pci.o
spi-altera-core-y := drivers/spi/spi-altera-core.o
spi-altera-dfl-y := drivers/spi/spi-altera-dfl.o
dfl-hssi-y := drivers/fpga/dfl-hssi.o
dfl-n3000-nios-y := drivers/fpga/dfl-n3000-nios.o
dfl-emif-y := drivers/memory/dfl-emif.o
fpga-bridge-y := drivers/fpga/fpga-bridge.o
fpga-mgr-y := drivers/fpga/fpga-mgr.o
fpga-region-y := drivers/fpga/fpga-region.o
fpga-image-load-y := drivers/fpga/fpga-image-load.o
qsfp-mem-y := drivers/net/phy/qsfp-mem.o
intel-s10-phy-y := drivers/net/phy/intel-s10-phy.o
intel-m10-bmc-y := drivers/mfd/intel-m10-bmc-core.o
intel-m10-bmc-spi-y += drivers/mfd/intel-m10-bmc-spi.o
intel-m10-bmc-pmci-y += drivers/mfd/intel-m10-bmc-pmci.o
intel-m10-bmc-log-y += drivers/mfd/intel-m10-bmc-log.o
intel-m10-bmc-hwmon-y := drivers/hwmon/intel-m10-bmc-hwmon.o
intel-m10-bmc-sec-update-y := drivers/fpga/intel-m10-bmc-sec-update.o
s10hssi-y := drivers/net/ethernet/intel/s10hssi.o
regmap-indirect-register-y := drivers/base/regmap/regmap-indirect-register.o
n5010-phy-y := drivers/net/ethernet/silicom/n5010-phy.o
n5010-hssi-y := drivers/net/ethernet/silicom/n5010-hssi.o
uio-dfl-y := drivers/uio/uio_dfl.o
8250_dfl-y := drivers/tty/serial/8250/8250_dfl.o

# intermediates used when reversing modules list
count := $(words $(obj-m))
indices := $(shell seq $(count) -1 1)

# module names without .o suffix
modules := $(basename $(obj-m))

# module names in reverse order with _ instead of -
reverse := $(foreach i,$(indices),$(word $i,$(modules)))
reverse := $(subst -,_,$(basename $(reverse)))

# explicit rules used to call insmod and rmmod for all modules
rules_rmmod := $(addprefix rmmod_,$(reverse))
rules_insmod := $(addprefix insmod_,$(modules))

# compile all modules
all:
	@$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

# install modules to /lib/modules/$(KERNEL)/extra
install:
	@$(MAKE) -C $(KERNELDIR) M=$(CURDIR) INSTALL_MOD_DIR=extra modules_install

# remove build artifacts
clean:
	@$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean
	@-rm -f *.rpm

$(rules_rmmod): rmmod_%:
	@if lsmod | grep -qE '\<$*\>'; then rmmod $*; fi

$(rules_insmod): insmod_%:
	@if ! lsmod | grep -q $* && test -f $*.ko; then \
		[ $* = uio-dfl ] && modprobe uio; \
		insmod $*.ko $(DYNDBG); \
	fi

# The rmmod target is also invoked directly by dkms
rmmod: $(rules_rmmod)
insmod: $(rules_insmod)
reload: rmmod insmod

# helper used to generate dynamic dkms config based on kernel config
dkms:
	@echo $(modules)

# build rpm packages
rpm: build/rpm/intel-fpga-dfl.spec clean
	@rpmbuild $(RPMBUILDOPTS) $<

deb: clean
	python build/debian/deb-changelog.py \
		$(BACKPORT_VERSION)-$(BACKPORT_RELEASE) > build/debian/changelog
	cd build && yes | debuild -uc -us

help:
	@echo "Build Usage:"
	@echo " make all	Build kernel modules"
	@echo " make install	Install kernel modules to /lib/modules/$$(uname -r)/extra"
	@echo " make clean	Remove build artifacts"
	@echo ""
	@echo "Package Usage:"
	@echo " make rpm	Build rpm package from source"
	@echo ""
	@echo "Test Usage:"
	@echo " make rmmod	Remove modules from running kernel"
	@echo " make insmod	Insert modules to running kernel"
	@echo " make reload	Combination of 'make rmmod' and 'make insmod'"
	@echo ""
	@echo "Build Arguments:"
	@echo " KERNEL		Kernel version to build against ($$(uname -r))"
	@echo " KERNELDIR	Path to kernel build dir (/lib/modules/<KERNEL>/build)"
	@echo ""
	@echo "Test Arguments:"
	@echo " DEBUG=<0|1>	Toggle dynamic debugging when inserting modules (0)"

.PHONY: all install clean rmmod insmod reload rpm help dkms
