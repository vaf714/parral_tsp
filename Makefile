INPUT_DIR=input_files
EXE_DIR=bin
SOURCE_DIR=source

CC=gcc
CFLAGS=-g -Wall #-D DEBUG

TSP_OBJECTS=tsp.c timer.h
TSP_OBJ:=$(foreach obj,$(TSP_OBJECTS),$(SOURCE_DIR)/$(obj))

tsp: $(TSP_OBJ)
	@echo building tsp...
	@if [ ! -d $(EXE_DIR) ]; then mkdir -p $(EXE_DIR); fi;
	@$(CC) $(CFLAGS) -o $(EXE_DIR)/tsp $(SOURCE_DIR)/tsp.c
	@echo build finished!


PARRAL_TSP_OBJECTS=parral_tsp.c timer.h
PARRAL_TSP_OBJ:=$(foreach obj,$(PARRAL_TSP_OBJECTS),$(SOURCE_DIR)/$(obj))

parral_tsp: $(PARRAL_TSP_OBJ)
	@echo building parral_tsp...
	@if [ ! -d $(EXE_DIR) ]; then mkdir -p $(EXE_DIR); fi;
	@$(CC) $(CFLAGS) -o $(EXE_DIR)/parral_tsp $(SOURCE_DIR)/parral_tsp.c -lpthread 
	@echo build finished!

clean:
	@if [ -d $(EXE_DIR) ]; rm $(EXE_DIR)/*
	@echo clean finished!
