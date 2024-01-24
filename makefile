SRC_DIR = src
OBJ_DIR = ./obj
EXE=main.elf

SRC=$(wildcard *.cpp $(foreach fd, $(SRC_DIR), $(fd)/*.cpp)) 
OBJS=$(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))


CXX ?= g++
SDLCONFIG ?= sdl-config
CXXFLAGS ?= -Os -Wall -Wextra
LDFLAGS ?= -lSDL_image -lSDL_ttf -lSDL_mixer -lmikmod -lSDL_gfx -lm

ifdef DEBUG
CXXFLAGS += -g
endif

ifdef TARGET
include $(TARGET).mk
endif

CXXFLAGS += `$(SDLCONFIG) --cflags`
LDFLAGS += `$(SDLCONFIG) --libs`

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@ 

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(RM) -rv *~ $(OBJS) $(EXE)