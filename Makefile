CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lyaml -ljansson
TARGET = yaml2json
all: $(TARGET)
$(TARGET): $(TARGET).o
	$(CC) $^ -o $@ $(LDFLAGS)
$(TARGET).o: $(TARGET).c
	$(CC) -c $< -o $@ $(CFLAGS)
clean:
	rm -f *.o $(TARGET)
