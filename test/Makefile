# Copyright 2025 Terry Golubiewski, all rights reserved.

export PROJDIR := $(abspath ..)
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
TARGET1=$(FILE_TEST_EXE)
TARGETS=$(TARGET1)

SRC1 := File_test.cpp
SOURCE := $(SRC1)

SYSINCL:=$(addsuffix /include, $(GTEST_INC) $(GSL))
INCLUDE:=$(PROJ_INC)

LIBS=$(PROJ_LIB)/libFile$(COMPILER)$(DBGSFX).$A

LIBPATH=$(GTEST_LIB)
LDLIBS=-lgtest -lgtest_main

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

.ONESHELL:

.PHONY: all clean scour run

all: $(TARGETS)

run: $(FILE_TEST_EXE)
	./$(FILE_TEST_EXE)

$(TARGET1): $(OBJ1) $(LIBS)
	$(LINK)
