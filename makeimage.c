#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "png.h"

#define N 4

int random_integer;

double max(double a, double b)
{
	if (a > b) return a;
	else return b;
}

double randf(int x)
{
	x += random_integer;
	x = (x<<13) ^ x;
	return ( 1.0 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0); 
}

int pair(int x, int y)
{
	return ((x+y)*(x+y+1)+y)/2;
}

double nrand(int a, int b, int c)
{
	int z;
	if (a < 0) a = (-2)*a - 1; else a = 2*a;
	if (b < 0) b = (-2)*b - 1; else b = 2*b;
	if (c < 0) c = (-2)*c - 1; else c = 2*c;
	z = pair(c, pair(b, a));
	return randf(z);
}

double interpolate_cos(double a, double b, double x)
{
	double ft, f;
	ft = x * 3.141592654589;
	f = (1 - cos(ft)) * .5;
	return  a*(1-f) + b*f;
}

double interpolate_cubic(double v0, double v1, double v2, double v3, double x)
{
	double P, Q, R, S;
	P = (v3 - v2) - (v0 - v1);
	Q = (v0 - v1) - P;
	R = v2 - v0;
	S = v1;
	return P*x*x*x + Q*x*x + R*x + S;
}

double noise1(int x)
{
	int iterations, invpersistence, period_factor, smallest_period;
	int i;
	double y;
	iterations = 8;
	invpersistence = 2;
	period_factor = 2;
	smallest_period = 1;
	y = 0;
	for (i = 1; i <= iterations; i++) {
		double c, period, x_anchor, v0, v1, v2, v3, x_transformed;
		int x_i;
		c = 1 / pow(invpersistence, i);

		period = smallest_period * pow(period_factor, (iterations-i));
		x_i = x / period;
		x_anchor = x_i * period;
		x_transformed = (x - x_anchor) / period;

		v0 = nrand(x_i-1, 0, i);
		v1 = nrand(x_i+0, 0, i);
		v2 = nrand(x_i+1, 0, i);
		v3 = nrand(x_i+2, 0, i);
		y += c * interpolate_cubic(v0, v1, v2, v3, x_transformed);
	}
	y *= (invpersistence - 1);
	return (y+1)/2;
}

double noise2(int x, int y)
{
	int iterations, smallest_period;
	double invpersistence, period_factor;
	int i;
	double z;
	iterations = 15;
	invpersistence = 1.01;
	period_factor = 2;
	smallest_period = 1;
	z = 0;
	for (i = 1; i <= iterations; i++) {
		double c, period, v0, v1, v2, v3;
		double x_anchor, x_transformed, y_anchor, y_transformed;
		int x_i, y_i;
		c = 1 / pow(invpersistence, i);

		period = smallest_period * pow(period_factor, (iterations-i));

		x_i = x / period;
		x_anchor = x_i * period;
		x_transformed = (x - x_anchor) / period;

		y_i = y / period;
		y_anchor = y_i * period;
		y_transformed = (y - y_anchor) / period;

		v0 = nrand(x_i+0, y_i+0, i) * max(0, 2-x_transformed-y_transformed-1);
		v1 = nrand(x_i+1, y_i+0, i) * max(0, 1+x_transformed-y_transformed-1);
		v2 = nrand(x_i+0, y_i+1, i) * max(0, 1-x_transformed+y_transformed-1);
		v3 = nrand(x_i+1, y_i+1, i) * max(0, 0+x_transformed+y_transformed-1);
		z += c * (v0 + v1 + v2 + v3)/2;
	}
	// z *= (invpersistence - 1);
	if (z > 1) z = 1;
	if (z < -1) z = -1;
	return (z+1)/2.0;
}

int main(int argc, char *argv[])
{
	png_image image;
	png_bytep buffer;

	if (argc >= 2) random_integer = atoi(argv[1]);
	else {
		FILE* f = fopen("/dev/urandom", "r");
		fread(&random_integer, sizeof(int), 1, f);
		printf("Seed: %d\n", random_integer);
	}

	memset(&image, 0, (sizeof image));

	image.version = PNG_IMAGE_VERSION;
	image.opaque = NULL;
	image.width = 8192;
	image.height = 8192;
	image.format = PNG_FORMAT_RGB;
	image.flags = 0;
	image.colormap_entries = 0;

	buffer = malloc(PNG_IMAGE_SIZE(image));

	const char* statuses = "a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.w.x.y.z.";
	int status_len = 52;
	int status_sent = 0;

	int i, j;
	printf("%s\n", statuses);
	for (i = 0; i < PNG_IMAGE_SIZE(image); i+=1) {
		if (status_sent < status_len * i /  PNG_IMAGE_SIZE(image)) {
			printf("%c", statuses[status_sent]);
			fflush(stdout);
			status_sent += 1;
		}
	#if N==1
		int t,w,x,y,z;
		z = i/3;
		w = (sqrtl(8*z+1)-1)/2;
		t = (w*w+w)/2;
		y = z-t;
		x = w-y;
		if (x > 255) x = 255;
		if (y > 255) y = 255;
		if (i % 3 == 0) buffer[i] = x;
		else if (i % 3 == 1) buffer[i] = y;
		else buffer[i] = 0;
	#elif N==2
		int t, x, y, z;
		t = i / 3;
		x = t % image.width;
		y = t / image.width;
		z = ((x+y)*(x+y+1)+y)/2;
		buffer[i] = z;
	#elif N==3
		int t, x, y;
		t = i / 3;
		x = t % image.width;
		y = t / image.width;
		if (x == 0) j = noise1(y) * image.width;
		buffer[i] = 255 * (x > j);
	#elif N==4
		int t, x, y;
		t = i / 3;
		x = t % image.width;
		y = t / image.width;
		buffer[i] = 255 * noise2(x, y);
	#endif
	}
	printf("%c", statuses[status_sent]);

	png_image_write_to_file(&image, "out.png", 0, buffer, 0, NULL);

	printf("\n");

	return 0;
}
