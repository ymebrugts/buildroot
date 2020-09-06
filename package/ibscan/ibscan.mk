################################################################################
#
# ib-scan-ultimate
#
################################################################################

IBSCAN_SITE_METHOD = file

IBSCAN_TARBALL = $(call qstrip,$(BR2_TARGET_IBSCAN_LOCATION))
IBSCAN_SITE = $(patsubst %/,%,$(dir $(IBSCAN_TARBALL)))
IBSCAN_SOURCE = $(notdir $(IBSCAN_TARBALL))
IBSCAN_VERSION = $(patsubst IBScanUltimate_armv7a_%,%,$(patsubst %.tgz,%,$(IBSCAN_SOURCE)))

IBSCAN_INSTALL_STAGING = YES
IBSCAN_DEPENDENCIES = libusb-compat udev

ifeq ($(BR2_ARM_EABIHF),y)
IBSCAN_ARCHABI = arm-linux-gnueabihf
else
IBSCAN_ARCHABI = arm-linux-gnueabi
endif

define IBSCAN_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)/app \
		$(TARGET_CONFIGURE_OPTS) \
		ARCHABI=$(IBSCAN_ARCHABI)
endef

define IBSCAN_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/IBScanUltimate
	cp -dpfr $(@D)/include/* $(STAGING_DIR)/usr/include/IBScanUltimate
	mkdir -p $(STAGING_DIR)/usr/lib
	cp -dpfr $(@D)/lib/$(IBSCAN_ARCHABI)/*.so $(STAGING_DIR)/usr/lib
endef

define IBSCAN_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/lib
	cp -dpfr $(@D)/lib/$(IBSCAN_ARCHABI)/*.so $(TARGET_DIR)/usr/lib
	mkdir -p $(TARGET_DIR)/bin
	cp -dpfr $(@D)/app/bin/* $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
