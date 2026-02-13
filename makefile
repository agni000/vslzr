CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
LIBS = -lraylib -lm -lpthread -ldl -lrt -lX11

SRC_DIR = src
TARGET = vslzr

SRC = $(SRC_DIR)/vslzr.c

# Build
build:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

