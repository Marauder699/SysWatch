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

install: $(TARGET)
	@echo "Installation de SysWatch..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installation du fichier .desktop..."
	sudo cp desktop/syswatch.desktop /usr/share/applications/
	@if [ -f icons/syswatch.png ]; then \
		echo "Installation de l'icône..."; \
		sudo mkdir -p /usr/share/icons/hicolor/256x256/apps; \
		sudo cp icons/syswatch.png /usr/share/icons/hicolor/256x256/apps/; \
		sudo gtk-update-icon-cache /usr/share/icons/hicolor/ -f 2>/dev/null || true; \
	else \
		echo "Aucune icône trouvée (icons/syswatch.png manquant)"; \
		echo "L'application utilisera l'icône système par défaut"; \
	fi
	@echo "Installation terminée!"

uninstall:
	@echo "Désinstallation de SysWatch..."
	sudo rm -f /usr/local/bin/$(TARGET)
	sudo rm -f /usr/share/applications/syswatch.desktop
	sudo rm -f /usr/share/icons/hicolor/256x256/apps/syswatch.png
	sudo gtk-update-icon-cache /usr/share/icons/hicolor/ -f 2>/dev/null || true
	@echo "Désinstallation terminée!"

.PHONY: all clean run install-deps install uninstall
