CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Iinclude
LIBS   = -lz -lzip
TARGET = pngprobe
SRCS   = src/main.c src/pngscanner.c src/base64.c src/zip_reader.c src/tensor.c

PNG ?= test.png

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

debug:
	$(CC) -Wall -Wextra -std=c11 -g -O0 -Iinclude -o $(TARGET) $(SRCS) $(LIBS)

valgrind: debug
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(TARGET) $(PNG)

valgrind-log: debug
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 --log-file=valgrind.log ./$(TARGET) $(PNG)

clean:
	rm -f $(TARGET) valgrind.log

.PHONY: all debug valgrind valgrind-log clean

