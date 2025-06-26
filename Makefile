# Copyright 2025 Terry Golubiewski, all rights reserved.

export PROJDIR := $(abspath .)
#SWDEV := $(PROJDIR)/SwDev
include $(PROJDIR)/project.mk

APP=$(abspath $(HOME)/App)
GSL=$(APP)/GSL

# Can't use := here because $E will be defined later in $(COMPILER).mk
TARGET1=tjg$(DBGSFX).$E
TARGETS=$(TARGET1)

SRC1 := tjg.cpp File.cpp
SOURCE := $(SRC1)

SCOUR := dox/html

SYSINCL:=$(addsuffix /include, $(SPDLOG) $(GSL))
INCLUDE:=$(PROJDIR)

LIBS:=

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

.ONESHELL:

.PHONY: all clean scour doxygen

all: $(TARGETS) doxygen

$(TARGET1): $(OBJ1) $(LIBS)
        $(LINK)

dox/html: $(wildcard *.cpp *.h)
	cd $(PROJDIR)/dox
	doxygen

doxygen: dox/html
