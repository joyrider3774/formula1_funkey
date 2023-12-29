SRC_DIR = src
OBJ_DIR = ./obj
EXE=main.elf

SRC=$(wildcard *.cpp $(foreach fd, $(SRC_DIR), $(fd)/*.cpp)) 
OBJS=$(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))


CC = g++
PREFIX = /usr
OPT_LEVEL = -O2 
CFLAGS = -Wall -Wextra -I$(PREFIX)/include -I$(PREFIX)/include/SDL 
LDFLAGS = -L$(PREFIX)/lib -lSDL -lSDL_image -lSDL_ttf -lSDL_mixer -lmikmod -lSDL_gfx -lm

ifdef DEBUG
CFLAGS += -g
endif

ifdef TARGET
include $(TARGET).mk
endif

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXE) $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(RM) -rv *~ $(OBJS) $(EXE)