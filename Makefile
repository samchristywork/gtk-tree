all:
	mkdir -p build
	gcc `pkg-config --cflags gtk+-3.0` -o build/main src/main.c `pkg-config --libs gtk+-3.0`
