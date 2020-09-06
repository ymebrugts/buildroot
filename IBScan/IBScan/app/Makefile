################################################################################
# COMPILE IBScanUltimate APPLICATION
################################################################################

#
# ARCHITECTURE-DEPENDENT SETTINGS
#
ifeq ($(ARCHABI),arm-linux-gnueabihf)
	ARCHFLAGS = -mfloat-abi=hard -mthumb -march=armv7-a
else
	ifeq ($(ARCHABI),arm-linux-gnueabi)
		ARCHFLAGS = -mfloat-abi=softfp -mthumb -march=armv7-a
	else
		ARCHFLAGS = -mfloat-abi=hard -mthumb -march=armv7-a
	endif
endif

#
# GENERAL SETTINGS
# CROSS_COMPILE MUST HAVE BEEN DEFINED AND EXPORTED
#
CC           = $(CROSS_COMPILE)""gcc
INCLUDES     = -I ../include
WARNINGS     = -Wall
OPTIMIZATION = -O2
LIBRARIES    = -lusb -lstdc++ -lpthread -L ../lib/$(ARCHABI) -l IBScanUltimate 
DEFINES      = -DBSD -D__linux__ -D_MULTI_THREADED $(ARCHDEFINES)
FLAGSCC      = -fPIC $(ARCHFLAGS)
FLAGSAR      = -Wl,--no-as-needed $(ARCHFLAGS)

#
# DIRECTORIES AND NAMES OF OBJECTS TO BE CREATED
#
BINDIR = bin

OBJDIR = obj
OBJS_  = IBScan.o
OBJS   = $(patsubst %,$(OBJDIR)/%,$(OBJS_))

#
# TOP-LEVEL RULES
#
all: $(BINDIR)/IBScan

clean:
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)

#
# RULES FOR COMPILING DIRECTORIES OF OBJECTS
#
$(OBJS): | $(OBJDIR)
$(OBJDIR):
	mkdir $(OBJDIR)
$(OBJDIR)/%.o: %.cpp
	$(CC) $(OPTIMIZATION) $(INCLUDES) $(WARNINGS) $(DEFINES) $(FLAGSCC) -c -o $@ $<

#
# RULES FOR CREATING BINARIES
#
$(BINDIR)/IBScan: $(OBJS) | $(BINDIR)
	$(CC) $(OPTIMIZATION) $(FLAGSAR) $(LIBRARIES) -o $@ $^
$(BINDIR):
	mkdir $(BINDIR)

