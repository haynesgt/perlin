all:
	gcc -o makeimage makeimage.c libpng16.a -lz -lm

test:	all
	./makeimage && eog out.png
