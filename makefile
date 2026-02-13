CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
LIBS = -lraylib -lm -lpthread -ldl -lrt -lX11

SRC_DIR = src
TARGET = vslzr
ASSETS_DIR = assets

SRC = $(SRC_DIR)/vslzr.c

# Build
build: $(ASSETS_DIR)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

$(ASSETS_DIR):
	mkdir -p $(ASSETS_DIR)

clean:
	rm -f $(TARGET)
