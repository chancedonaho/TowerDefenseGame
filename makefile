CC = g++
CFLAGS = -std=c++17 -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
SRC = main.cpp
OUT = game

all:
	$(CC) $(SRC) -o $(OUT) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(OUT)