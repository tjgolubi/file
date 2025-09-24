# @file
# @copyright 2025 Terry Golubiewski, all rights reserved.
# @author Terry Golubiewski

export PROJNAME := File
export PROJDIR  := $(abspath ..)
include $(PROJDIR)/project.mk

APP:=$(abspath $(HOME)/App)
GSL:=$(APP)/GSL
GTEST:=$(APP)/googletest

GTEST_LIB:=$(GTEST)/build/lib
GTEST_INC:=$(GTEST)/googletest

PROJ_SRC:=$(PROJDIR)/src
PROJ_LIB:=$(PROJDIR)/src
PROJ_INC:=$(PROJDIR)/src

FILE_TEST_EXE=File_test$(DBGSFX).$E
PIPE_TEST_EXE=Pipe_test$(DBGSFX).$E
TGT1=$(FILE_TEST_EXE)
TGT2=$(PIPE_TEST_EXE)
TARGETS=$(TGT1) $(TGT2)

SRC1 := File_test.cpp
SRC2 := Pipe_test.cpp
SOURCE := $(SRC1) $(SRC2)

SYSINCL:=$(addsuffix /include, $(GSL) $(GTEST_INC))
INCLUDE:=$(PROJ_INC)

LIBS=$(PROJ_LIB)/libFile$(COMPILER)$(DBGSFX).$A

#DEBUG:=1

LIBPATH:=$(GTEST_LIB)

SHELL:=/bin/bash
.ONESHELL:
.SHELLFLAGS:=-eu -o pipefail -c

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

LDLIBS+=-lgtest -lgtest_main

.PHONY: all clean scour test success

all: $(TARGETS)

LOGFILES:=$(patsubst %.exe, log/%.json, $(TARGETS))

log/%.json: %.$E
	@set -v
	./$< --gtest_output=json:$(notdir $@)
	mkdir -p $(dir $@)
	mv $(notdir $@) $@

CLEAN += log

success:
	@echo "All tests passed."

test: depend $(LOGFILES) success

$(TGT1): $(OBJ1) $(LIBS)
	$(LINK)

$(TGT2): $(OBJ2) $(LIBS)
	$(LINK)
