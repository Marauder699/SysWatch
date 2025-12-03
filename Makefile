# Makefile pour SysWatch
# Application de monitoring pour Raspberry Pi 500+

# Version et auteur de l'application
VERSION = 1.0.0
AUTHOR = "Stephane Corriveau"

CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -Wextra -Iinclude -g -DAPP_VERSION=$(VERSION) -DAPP_AUTHOR=$(AUTHOR)
LIBS = `pkg-config --libs gtk+-3.0`
TARGET = syswatch

# Fichiers sources et objets
SRC_DIR = src
OBJ_DIR = obj
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

# Créer le répertoire obj s'il n'existe pas
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Compiler les fichiers objets
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Lier l'exécutable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

install-deps:
	@echo "Installation des dépendances GTK3..."
	sudo apt-get update
	sudo apt-get install -y libgtk-3-dev pkg-config

.PHONY: all clean run install-deps
