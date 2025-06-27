# Copyright 2025 Terry Golubiewski, all rights reserved.

export PROJDIR := $(abspath ..)
#SWDEV := $(PROJDIR)/SwDev
include $(PROJDIR)/project.mk

APP:=$(abspath $(HOME)/App)
GSL:=$(APP)/GSL

DOX      := $(PROJDIR)/dox
DOX_HTML := $(DOX)/html

PROJ_SRC := $(PROJDIR)/src
PROJ_LIB := $(PROJ_SRC)
PROJ_INC := $(PROJ_SRC)

# Can't use := here because $E will be defined later in $(COMPILER).mk
FILE_A=$(PROJ_LIB)/libFile$(COMPILER)$(DBGSFX).$A
TARGET1=$(FILE_A)
TARGETS=$(TARGET1)

SRC1 := File.cpp
SOURCE := $(SRC1)

SCOUR := $(DOX_HTML)

SYSINCL:=$(addsuffix /include, $(SPDLOG) $(GSL))
INCLUDE:=$(PROJ_INC)

LIBS=$(FILE_A)

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

.ONESHELL:

.PHONY: all clean scour doxygen

all: $(TARGETS) doxygen

$(TARGET1): $(OBJ1)

$(DOX_HTML): $(wildcard *.cpp *.h)
	cd $(DOX)
	doxygen

doxygen: $(DOX_HTML)
