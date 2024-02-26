CC = g++
EXECUTABLE = my-mutex
CFLAGS = -std=c++1y -o3

all: $(EXECUTABLE)

$(EXECUTABLE): my-mutex.cpp
	$(CC) $(CFLAGS) $^ -o $@	

clean:
	rm -f $(EXECUTABLE)