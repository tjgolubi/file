# Copyright 2025 Terry Golubiewski, all rights reserved.

export PROJDIR := $(abspath .)
#SWDEV := $(PROJDIR)/SwDev
include $(PROJDIR)/project.mk

APP=$(abspath $(HOME)/App)
GSL=$(APP)/GSL

# Can't use := here because $E will be defined later in $(COMPILER).mk
TARGET1=tjg$(DBGSFX).$E
TARGETS=$(TARGET1)

SRC1 := tjg.cpp 
SOURCE := $(SRC1)

SYSINCL:=$(addsuffix /include, $(SPDLOG) $(GSL))
INCLUDE:=$(PROJDIR)

LIBS:=

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

.ONESHELL:

.PHONY: all clean scour

all: $(TARGETS)

$(TARGET1): $(OBJ1) $(LIBS)
        $(LINK)
