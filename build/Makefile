# $Id$
# DCID Makefile
#
# Daughter Card ID
#

export CNPLATFORM=ironforge
export TARGET=arm-linux
export PLATFORM_TARGET=$(TARGET)-$(CNPLATFORM)

ifeq (${METAPROJECT_ROOT},)
export METAPROJECT_ROOT:=$(abspath ../..)
endif

include ${METAPROJECT_ROOT}/config/config.mk

# make command
MAKECMD  = $(MAKE) $(MAKEOPTS) --no-print-directory

# compiler flags
CFLAGS   = -Wall -g -I../src -I../include -I../import/chumby_accel/all/include -DCNPLATFORM_$(CNPLATFORM)

# linker flags
LDFLAGS  = 

# write disabled binaries
WDBIN     = ../bin/write-disabled/dcid
WDLIB     = ../bin/write-disabled/libdcid.a
EXP_WDBIN = ../export/write-disabled/$(PLATFORM_TARGET)/bin/dcid
EXP_WDLIB = ../export/write-disabled/$(PLATFORM_TARGET)/lib/libdcid.a

# write enabled binaries
WEBIN     = ../bin/write-enabled/dcid
WELIB     = ../bin/write-enabled/libdcid.a
EXP_WE_DIR = ../export/write-enabled/${PLATFORM_TARGET}
EXP_WEBIN = ../export/write-enabled/$(PLATFORM_TARGET)/bin/dcid
EXP_WELIB = ../export/write-enabled/$(PLATFORM_TARGET)/lib/libdcid.a
EXP_WE_DIRS = $(addprefix ${EXP_WE_DIR}/,bin lib)

# write disabled target flags and binaries
write-disabled: WEFLAGS  =
write-disabled: OUT_BIN  = $(WDBIN)
write-disabled: OUT_LIB  = $(WDLIB)
write-disabled: EXP_BIN  = $(EXP_WDBIN)
write-disabled: EXP_LIB  = $(EXP_WDLIB)

# write enabled target flags and binaries
write-enabled: WEFLAGS   = -D DCID_ALLOW_WRITE
write-enabled: OUT_BIN   = $(WEBIN)
write-enabled: OUT_LIB   = $(WELIB)
write-enabled: EXP_BIN   = $(EXP_WEBIN)
write-enabled: EXP_LIB   = $(EXP_WELIB)

# test utility binary
TST_UTL_BIN = ../bin/test-util

# test interface binary
TST_INT_BIN = ../bin/test-interface

# dummy values that make seems to want for whatever reason
WEFLAGS  =
OUT_BIN  = $(WDBIN)
OUT_LIB  = $(WDLIB)
OUT_INC  = ../include/*.h
OUT_DOC  = ../doc/doxygen
CFG_DOC  = ../doc/doxygen.conf
EXP_WD_DIR = ../export/write-disabled/${PLATFORM_TARGET}
EXP_BIN  = ../export/write-disabled/$(PLATFORM_TARGET)/bin/dcid
EXP_LIB  = ../export/write-disabled/$(PLATFORM_TARGET)/lib/libdcid.a
EXP_INC  = ../export/all/all/include/
EXP_DOC  = ../export/all/all/doc/
EXP_WD_DIRS = $(addprefix ${EXP_WD_DIR}/,bin lib)

SOURCES  = $(wildcard ../src/*.c)
OBJS     = $(SOURCES:.c=.o)
CC       = $(TARGET)-gcc
STRIP    = $(TARGET)-strip
AR       = $(TARGET)-ar
DOXYGEN  = doxygen

# build all targets, excluding exports
all:
	@$(MAKECMD) clean-objs clean-objs-main
	@$(MAKECMD) write-enabled
	@$(MAKECMD) clean-objs clean-objs-main
	@$(MAKECMD) write-disabled
	@$(MAKECMD) clean-objs
	@$(MAKECMD) test-util
	@$(MAKECMD) test-interface

# build write enabled binaries
write-enabled: $(OUT_BIN) $(OUT_LIB)

# build write disabled binaries
write-disabled: $(OUT_BIN) $(OUT_LIB)

# build test utility
test-util: $(TST_UTL_BIN)

# build test interface
test-interface: $(TST_INT_BIN);

.c.o:
	@echo "  C $<"
	@$(CC) $(CFLAGS) $(WEFLAGS) -c $< -o $@

$(OUT_BIN): $(OBJS) ../src/main/main.o
	@echo "  B $(OUT_BIN)"
	@$(CC) ../src/*.o ../src/main/main.o $(LDFLAGS) -o $(OUT_BIN)
	@$(STRIP) -d $(OUT_BIN)

$(OUT_LIB): $(OBJS)
	@echo "  A $(OUT_LIB)"
	@$(AR) rcs $(OUT_LIB) $(OBJS)

$(TST_UTL_BIN): $(OBJS) ../src/test-util/test-util.o
	@echo "  B $(TST_UTL_BIN)"
	@$(CC) ../src/*.o ../src/test-util/test-util.o $(LDFLAGS) -o $(TST_UTL_BIN)
	@$(STRIP) -d $(TST_UTL_BIN)

$(TST_INT_BIN): $(OBJS) ../src/test-interface/test-interface.o
	@echo "  B $(TST_INT_BIN)"
	@$(CC) ../src/*.o ../src/test-interface/test-interface.o $(LDFLAGS) -o $(TST_INT_BIN)
	@$(STRIP) -d $(TST_INT_BIN)

doxygen: ${OUT_DOC}
	@echo "  D $(CFG_DOC)"
	@$(DOXYGEN) $(CFG_DOC) 1 > /dev/null

clean: clean-objs clean-objs-main clean-objs-test-util test-util-clean clean-objs-test-interface test-interface-clean
	@$(MAKECMD) write-enabled-clean
	@$(MAKECMD) write-disabled-clean
	@$(MAKECMD) test-util-clean
	@$(MAKECMD) test-interface-clean

clean-objs:
	@echo "  X ../src/*.o"
	@-rm -rf ../src/*.o

clean-objs-main:
	@echo "  X ../src/main/*.o"
	@-rm -rf ../src/main/*.o

clean-objs-test-util:
	@echo "  X ../src/test-util/*.o"
	@-rm -rf ../src/test-util/*.o
	
clean-objs-test-interface:
	@echo "  X ../src/test-interface/*.o"
	@-rm -rf ../src/test-interface/*.o

write-enabled-clean:
	@echo "  X $(WEBIN)"
	@-rm -rf $(WEBIN)

write-disabled-clean:
	@echo "  X $(WDBIN)"
	@-rm -rf $(WDBIN)

test-util-clean:
	@echo "  X $(TST_UTL_BIN)"
	@-rm -rf $(TST_UTL_BIN)

test-interface-clean:
	@echo "  X $(TST_INT_BIN)"
	@-rm -rf $(TST_INT_BIN)

${EXP_WD_DIRS} ${EXP_WE_DIRS} ${OUT_DOC}:
	mkdir -p $@

exports: all exports-bin exports-lib exports-doxygen exports-scripts

exports-bin: ${EXP_WD_DIRS} ${EXP_WE_DIRS}
	@echo "  E  $(EXP_WDBIN)"
	@-cp $(WDBIN) $(EXP_WDBIN)
	@echo "  E  $(EXP_WEBIN)"
	@-cp $(WEBIN) $(EXP_WEBIN)

exports-lib:
	@echo "  E  $(EXP_WDLIB)"
	@-cp $(WDLIB) $(EXP_WDLIB)
	@echo "  E  $(EXP_WELIB)"
	@-cp $(WELIB) $(EXP_WELIB)
	@echo "  E  $(EXP_INC)"
	@-cp $(OUT_INC) $(EXP_INC)

exports-doxygen: doxygen
	@echo "  E $(EXP_DOC)"
	@-cp -r $(OUT_DOC) $(EXP_DOC)

exports-scripts:
	@echo "  E  install.sh"
	cp install.sh ../export

exports-clean: exports-clean-bin exports-clean-lib exports-clean-doc

exports-clean-bin:
	@echo "  X $(EXP_BIN)"
	@-rm -rf $(EXP_BIN)

exports-clean-lib:
	@echo "  X $(EXP_LIB)"
	@-rm -rf $(EXP_LIB)

exports-clean-doc:
	@echo "  X $(OUT_DOC)"
	@-rm -rf $(OUT_DOC)

# Build all targets required for production
build-all:
	$(MAKE) all exports PRODUCTION=1 TARGET=$(TARGET)

# Clean all production targets
clean-all:
	$(MAKE) clean TARGET=$(TARGET)
	$(MAKE) exports-clean TARGET=$(TARGET)

commit:
	export COMMIT_TIME="$(shell date +'%d-%b-%Y %H%M %Z')" ; cd ../export ; echo "Auto-commit Production=$(PRODUCTION) $${COMMIT_TIME}" >>autocommit.log ; svn commit -m"{auto} Automated export checkin by build process at $${COMMIT_TIME}"

.PHONY : all write-enabled write-disabled write-enabled-clean write-disabled-clean clean exports exports-clean \
	exports-scripts

