CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c17
TARGET = cnew
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/
	@chmod +x /usr/local/bin/$(TARGET)
	@echo "$(TARGET) installed successfully."

uninstall: $(TARGET)
	@echo "Uninstalling $(TARGET) from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/$(TARGET)
	@echo "$(TARGET) uninstalled successfully."

clean: 
	@echo "Cleaning up..."
	@rm -f $(TARGET) *.o

.PHONY: all install uninstall clean
