BUILD_DIR=build
EXE_DIR=bin
SOURCE_DIR=source

CC=gcc
#CFLAGS=-g -Wall -D DEBUG
CFLAGS=-g -Wall

OBJECTS=tsp.c timer.h
OBJ:=$(foreach obj,$(OBJECTS),$(SOURCE_DIR)/$(obj))

all: tsp.o
	@echo building...
	@if [ ! -d $(EXE_DIR) ]; then mkdir -p $(EXE_DIR); fi;
	@$(CC) -o $(EXE_DIR)/tsp $(BUILD_DIR)/tsp.o
	@echo build finished!
tsp.o: $(OBJ)
	@if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;
	@$(CC) -c $(CFLAGS) $(SOURCE_DIR)/tsp.c -o $(BUILD_DIR)/tsp.o

clean:
	rm build/*.o bin/tsp
