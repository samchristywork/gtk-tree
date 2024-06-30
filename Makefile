all: build/main

build/main: src/main.c
	mkdir -p build
	gcc -g `pkg-config --cflags gtk+-3.0` -o build/main src/main.c `pkg-config --libs gtk+-3.0` -lm

run: build/main
	./build/main

clean:
	rm -rf build
