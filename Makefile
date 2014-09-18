all:
	gcc -o main main.c -lxcb -lm `pkg-config glib-2.0 --cflags --libs` -g

