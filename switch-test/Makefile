#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Prebuilt SpiderMonkey pieces (built in WSL by switch-build/). Override via env.
#   MOZ_DIST   : dir with include/ of mozjs headers
#   JSLIBS     : dir with lib/{libjs_static,libjsrust,libswitchextra}.a
#---------------------------------------------------------------------------------
MOZ      ?= $(TOPDIR)/../mozjs-128.13.0
JSLIBS   ?= $(TOPDIR)/../switch-build/libs
SHIMS    := $(TOPDIR)/../switch-shims
MOZ_DIST ?= $(TOPDIR)/../switch-build/dist

TARGET		:=	SpiderMonkey-NX
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	source
ROMFS		:=	romfs

APP_TITLE	:=	Spidermonkey-NX
APP_AUTHOR	:=	Souldbminer
APP_VERSION	:=	1.0.0

#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

DEFINES	:=	-D__SWITCH__ -D__linux__ -D__gnu_linux__ -D_GNU_SOURCE -DMOZ_SWITCH_NO_SIGNAL
# Opt-in self-hosting/zlib debug trace (pairs with mozjs SWITCH_DEBUG_SELFHOST=1).
# Build with: make DEBUG_SELFHOST=1   (default off -> clean build)
ifeq ($(DEBUG_SELFHOST),1)
DEFINES	+=	-DSWITCH_DEBUG_SELFHOST
endif

CFLAGS	:=	-g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES) \
			-I$(MOZ_DIST)/include -I$(SHIMS)
CFLAGS	+=	$(INCLUDE)

CXXFLAGS	:= $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions \
			-include $(SHIMS)/switch_pthread_compat.h

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-z,notext -Wl,-z,nopack-relative-relocs -Wl,--wrap=pthread_create -Wl,-Map,$(notdir $*.map)

# circular refs between libjs_static <-> libswitchextra(mozglue/mfbt) <-> libjsrust
LIBS	:= -Wl,--start-group -ljs_static -ljsrust -lswitchextra -Wl,--end-group -lz -lnx

LIBDIRS	:= $(JSLIBS) $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all
all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
#---------------------------------------------------------------------------------
else
.PHONY:	all
DEPENDS	:=	$(OFILES:.o=.d)
all	:	$(OUTPUT).nro
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf	:	$(OFILES)
$(OFILES_SRC)	: $(HFILES_BIN)
-include $(DEPENDS)
endif
#---------------------------------------------------------------------------------------
