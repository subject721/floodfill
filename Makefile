
TARGET_NAME = floodfill

#ARCH_PREFIX = $(shell uname -m)-linux-gnu

ifdef ARCH_PREFIX
CC  = $(ARCH_PREFIX)-gcc
CXX = $(ARCH_PREFIX)-g++
LD  = $(ARCH_PREFIX)-g++

OBJ_DIR = obj.$(ARCH_PREFIX)
DEP_DIR = dep.$(ARCH_PREFIX)
BIN_DIR = bin.$(ARCH_PREFIX)

else
CC  = gcc
CXX = g++
LD  = g++

OBJ_DIR = obj
DEP_DIR = dep
BIN_DIR = bin

endif


SRC_DIR = src

SRC = $(shell find $(SRC_DIR) -name '*.cpp')

SRC_BASENAMES = $(basename $(notdir $(SRC)))
SRC_DIRS = $(dir $(SRC))

VPATH = $(SRC_DIRS)

OBJ = $(addsuffix .o,$(SRC_BASENAMES))
DEP = $(addsuffix .d,$(SRC_BASENAMES))
#OBJ += Startup.o

OBJ_FULL = $(addprefix $(OBJ_DIR)/, $(OBJ))
DEP_FULL = $(addprefix $(DEP_DIR)/, $(DEP))

TARGET_FULL = $(addprefix $(BIN_DIR)/,$(TARGET_NAME))

LIBS = -lX11 -lpthread

COMMON_FLAGS = -g -Wall
CXXFLAGS = $(COMMON_FLAGS) -std=c++11 -Iinclude
LINKFLAGS = $(COMMON_FLAGS)

DIRS = $(OBJ_DIR) $(DEP_DIR) $(BIN_DIR)

-include $(DEP_FULL)

.DEFAULT_GOAL := all

.PHONY: default
default: all

.PHONY: all
all: $(TARGET_FULL)

$(TARGET_FULL): $(DIRS) $(OBJ_FULL)
	@printf "%-10s %-12s:%s\n" "linking" "[$(CXX)]" $@
	@$(LD) -o $@ $(LINKFLAGS) $(OBJ_FULL) $(LIBS) -Wl,--no-as-needed

$(OBJ_DIR):
	@if [ ! -e $@ ]; then mkdir $@; fi

$(DEP_DIR):
	@if [ ! -e $@ ]; then mkdir $@; fi

$(BIN_DIR):
	@if [ ! -e $@ ]; then mkdir $@; fi

$(OBJ_DIR)/%.o: %.cpp
	@printf "%-10s %-12s:%s\n" "compiling" "[$(CXX)]" $<
	@$(CXX) $(CXXFLAGS) -c -o $@ -MMD -MF $(addprefix ./$(DEP_DIR)/,$(patsubst %.o, %.d, $(notdir $@))) $<


.PHONY: clean
clean:
	@echo "Cleaning executable"
	@rm -f $(TARGET_FULL)
	@echo "Cleaning objects/dependency files"
	@rm -f $(OBJ_FULL)
	@rm -f $(DEP_FULL)

