all:
	gcc -o makeimage makeimage.c -lz -lm -lpthread -lpng

test:	all
	./makeimage && eog out.png
