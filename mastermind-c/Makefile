# Makefile pour Mastermind (jeu de base)

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11
INCLUDE := -Iinclude

SRC_DIR := src
OBJ_DIR := build

SOURCES := $(SRC_DIR)/main.c \
           $(SRC_DIR)/game.c \
           $(SRC_DIR)/feedback.c \
           $(SRC_DIR)/parse.c \
           $(SRC_DIR)/colors.c \
           $(SRC_DIR)/utils.c

OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

TARGET  := mastermind

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
    $(CC) $(CFLAGS) $(OBJECTS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
    $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJ_DIR):
    mkdir -p $(OBJ_DIR)

clean:
    rm -rf $(OBJ_DIR) $(TARGET)
