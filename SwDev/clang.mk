# Copyright 2002-2025 Terry Golubiewski, all rights reserved.

CC=gcc
CXX=g++
LD=g++
AR=ar
LEX=lex
YACC=yacc
LINT=lint

LFLAGS =
YFLAGS =
#CDEFS  += -D_REENTRANT
#CFLAGS += -pthread -pipe -fPIC
#CFLAGS += -march=native
CFLAGS += -Wall -Wno-strict-aliasing
CFLAGS += -Werror
#CFLAGS += -Wno-c++0x-compat
CFLAGS += -std=c++23
CFLAGS += -Wno-unused-local-typedefs
#CFLAGS += -fno-diagnostics-show-caret
CFLAGS += -fdiagnostics-color
AFLAGS = /ML
LDFLAGS = -pthread
LDFLAGS =
LDLIBS =
LDLIBS += -lpthread -lrt
LDLIBS += -llog
ARFLAGS = ruv
LINTFLAGS += gcc.lnt

ifdef PATH_OBJ
LINTFLAGS += $(PATH_OBJ)
endif

ifdef DEBUG
CFLAGS += -Og -g
else
CFLAGS += -O2
endif

CXXFLAGS=$(CFLAGS)

GCCSYS :=
GCCINCL = $(addprefix -I,$(strip $(INCLPATH) $(INCLUDE) $(SYSINCL) $(GCCSYS)))

ifdef LIBPATH
LDFLAGS += $(addprefix -L,$(strip $(LIBPATH)))
endif

O=o
A=a
E=exe
D=d

MKDEPSARGS= -MM -MF$@ -MT'$(@:.$D=.$O) $@' $(addprefix -isystem ,$(strip $(SYSINCL) $(GCCSYS))) $(CFLAGS) $(CDEFS) $(GCCINCL) $<


define MKDEPSC
@$(RM) $@
@$(CC) $(MKDEPSARGS)
endef

define MKDEPSCXX
@$(RM) $@
@$(CXX) $(MKDEPSARGS)
endef

define LINK
@echo "Linking: $@"
@$(RM) $@
@$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endef

%.$E: %.$O
	$(LINK)

ifdef PATH_OBJ

$(PATH_OBJ)/%.c: %.l
	$(LEX) $(LFLAGS) -c -o $@ $<

$(PATH_OBJ)/%.c: %.y
	$(YACC) $(YFLAGS) -o $@ $<

$(PATH_OBJ)/%.$O: %.c
	@echo "Module: $<"
	@$(RM) $@
	@$(CC) $(CFLAGS) $(CDEFS) $(GCCINCL) -c -o$@ $<

$(PATH_OBJ)/%.$O: %.cpp
	@echo "Module: $<"
	@$(RM) $@
	@$(CXX) $(CXXFLAGS) $(CDEFS) $(GCCINCL) -c -o$@ $<

$(PATH_OBJ)/%.$D: %.c
	$(MKDEPSC)

$(PATH_OBJ)/%.$D: %.cpp
	$(MKDEPSCXX)

endif

%.c: %.l
	$(LEX) $(LFLAGS) -c -o $@ $<

%.c: %.y
	$(YACC) $(YFLAGS) -o $@ $<

%.$O: %.c
	@echo "Module: $<"
	@$(RM) $@
	@$(CC) $(CFLAGS) $(CDEFS) $(GCCINCL) -c -o$@ $<

%.$O: %.cpp
	@echo "Module: $<"
	@$(RM) $@
	@$(CXX) $(CXXFLAGS) $(CDEFS) $(GCCINCL) -c -o$@ $<

%.$D: %.c
	$(MKDEPSC)

%.$D: %.cpp
	$(MKDEPSCXX)

%.$A:
	@echo "Library: $@"
	@$(AR) $(ARFLAGS) $@ $?

%.$E:
	$(LINK)
