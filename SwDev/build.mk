# Copyright 2025 Terry Golubiewski, all rights reserved.

CDEFS += -DVER=$(VERSION)

ifdef DEBUG
CDEFS += -DDEBUG=$(DEBUG)
else
CDEFS += -DNDEBUG
endif

BASENAMES := $(notdir $(basename $(sort $(SOURCE))))
OBJ    := $(addsuffix .$O, $(BASENAMES))
DFILES := $(addsuffix .$D, $(BASENAMES))
LOB    := $(addsuffix .lob, $(BASENAMES))

OBJ1 := $(addsuffix .$O, $(notdir $(basename $(SRC1))))
OBJ2 := $(addsuffix .$O, $(notdir $(basename $(SRC2))))
OBJ3 := $(addsuffix .$O, $(notdir $(basename $(SRC3))))
OBJ4 := $(addsuffix .$O, $(notdir $(basename $(SRC4))))
OBJ5 := $(addsuffix .$O, $(notdir $(basename $(SRC5))))
OBJ6 := $(addsuffix .$O, $(notdir $(basename $(SRC6))))
OBJ7 := $(addsuffix .$O, $(notdir $(basename $(SRC7))))
OBJ8 := $(addsuffix .$O, $(notdir $(basename $(SRC8))))
OBJ9 := $(addsuffix .$O, $(notdir $(basename $(SRC9))))

ifdef PATH_OBJ

OBJ    := $(addprefix $(PATH_OBJ)/, $(OBJ))
DFILES := $(addprefix $(PATH_OBJ)/, $(DFILES))
LOB    := $(addprefix $(PATH_OBJ)/, $(LOB))
CLEAN  += $(PATH_OBJ)

OBJ1 := $(addprefix $(PATH_OBJ)/, $(OBJ1))
OBJ2 := $(addprefix $(PATH_OBJ)/, $(OBJ2))
OBJ3 := $(addprefix $(PATH_OBJ)/, $(OBJ3))
OBJ4 := $(addprefix $(PATH_OBJ)/, $(OBJ4))
OBJ5 := $(addprefix $(PATH_OBJ)/, $(OBJ5))
OBJ6 := $(addprefix $(PATH_OBJ)/, $(OBJ6))
OBJ7 := $(addprefix $(PATH_OBJ)/, $(OBJ7))
OBJ8 := $(addprefix $(PATH_OBJ)/, $(OBJ8))
OBJ9 := $(addprefix $(PATH_OBJ)/, $(OBJ9))

endif

.PHONY: default lint clean scour depend

default: depend $(TARGETS)

lint: $(PATH_OBJ) $(LOB)

ifdef PATH_OBJ

$(PATH_OBJ):
	@-mkdir --parents $(PATH_OBJ)

endif

clean:
	@$(RM) -r $(CLEAN)

scour: clean
	@$(RM) -r $(SCOUR) $(TARGETS)

depend: $(PATH_OBJ) $(DFILES)

ifdef DFILES
  ifeq "$(filter scour clean, $(MAKECMDGOALS))" ""
    ifneq "$(wildcard *.d, $(DFILES))" ""
      -include $(DFILES)
    else
      $(info Auto-dependency files not loaded.)
    endif
  else
    ifneq "$(filter-out scour clean, $(MAKECMDGOALS))" ""
      $(info Auto-dependency files not loaded.)
    endif
  endif
endif
