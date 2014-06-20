#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "png.h"

#define N 4

int random_integer;

double max(double a, double b)
{
	if (a > b) return a;
	else return b;
}

// Calculates a random number.
// Careful: Sometimes every number will be odd. 
// The distribution is still more or less uniform.
double randf(int x)
{
	int a, b, c, d;
	a = 2440355209;
	b = 3530914573;
	c = 3993913333;
	d = 4294967291;
	x += random_integer;
	x = x * ( x * ( x * a + b ) + c ) + d;
	return 1.0 - (x & 0x7fffffff) / 1073741824.0; 

}

unsigned int pair(unsigned int x, unsigned int y)
{
	// Cantor's pairing function
	// Maps N^2 to N
	return ((x+y)*(x+y+1)+y)/2;
}

// Computes a random value using a 3 dimentional point of integer coordinates
double nrand(int a, int b, int c)
{
	int z;
	// Map integers onto natural numbers
	if (a < 0) a = (-2)*a - 1; else a = 2*a;
	if (b < 0) b = (-2)*b - 1; else b = 2*b;
	if (c < 0) c = (-2)*c - 1; else c = 2*c;
	// Pair them up
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
	// z = SUM( noise at x y for each iteration )
	// Each iteration zooms out by a given factor (period_factor), and
	// decreases in intensity by a given factor (invpersistence)
	// The first iteration is smallest_period*smallest_period in size
	int iterations, smallest_period;
	double invpersistence, period_factor;
	int i;
	double z;
	iterations = 10;
	invpersistence = 1.1;
	period_factor = 2;
	smallest_period = 1;
	z = 0;
	for (i = 1; i <= iterations; i++) {
		double c, period, v0, v1, v2, v3;
		double x_anchor, x_transformed, y_anchor, y_transformed;
		int x_i, y_i;
		c = 1 / pow(invpersistence, i);

		// period = how many pixels wide the squares are
		period = smallest_period * pow(period_factor, (iterations-i));

		// x_i and y_i uniquely identify a random value for an iteration
		x_i = x / period;
		x_anchor = x_i * period;
		x_transformed = (x - x_anchor) / period;

		y_i = y / period;
		y_anchor = y_i * period;
		y_transformed = (y - y_anchor) / period;

		// nrand(x, y, i) calculates the noise for iteration i at point (x,y)
		// We get the noise then do diamond interpolation on four points
		v0 = nrand(x_i+0, y_i+0, i) * max(0, 2-x_transformed-y_transformed-1);
		v1 = nrand(x_i+1, y_i+0, i) * max(0, 1+x_transformed-y_transformed-1);
		v2 = nrand(x_i+0, y_i+1, i) * max(0, 1-x_transformed+y_transformed-1);
		v3 = nrand(x_i+1, y_i+1, i) * max(0, 0+x_transformed+y_transformed-1);
		z += c * (v0 + v1 + v2 + v3)/2;
	}
	// The maximum of z is sum(1/invpersistence^i, i=0..inf) = (invpersistence-1)
	// However, since we averaged many random values, z is already (usually) in the right range
	// z *= (invpersistence - 1);
	if (z > 1) z = 1;
	if (z < -1) z = -1;
	return (z+1)/2.0;
}

struct fill_image_data
{
	png_image *image;
	png_bytep buffer;
	size_t start;
	size_t end;
	char finish_message;
};

void* fill_image(void *datap)
{
	struct fill_image_data *data = datap;
	size_t i;
	// Fill each pixel in the image
	for (i = data->start; i < data->end; i++) {
		int t, x, y;
		// There is one channel per pixel, so divide by 1
		t = i / 1;
		x = t % data->image->width;
		y = t / data->image->width;
		// noise2 calculates the noise at integer x,y
		data->buffer[i] = 255 * noise2(x, y);
	}
	// Print completion message and flush output to make it show (because we did not print a \n)
	printf("%c", data->finish_message);
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	png_image image;
	png_bytep buffer;

	// Set the seed (as global variable random_integer)
	if (argc >= 2) random_integer = atoi(argv[1]);
	else {
		FILE* f = fopen("/dev/urandom", "r");
		fread(&random_integer, sizeof(int), 1, f);
		printf("Seed: %d\n", random_integer);
	}

	// Set the image attributes
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;
	image.opaque = NULL;
	image.width = 1024;
	image.height = 1024;
	image.format = PNG_FORMAT_GRAY;
	image.flags = 0;
	image.colormap_entries = 0;

	// Allocate the buffer
	buffer = malloc(PNG_IMAGE_SIZE(image));

	// The status messages are used to indicate the progress through the image.
	// Each thread will spit one 
	const char* statuses = "abcdefghijklmnopqrstuvwxyz";
	int status_len = 26;
	int status_sent = 0;

	// Each chunk will process chunk_len pixel parts, except the last chunk
	// which processes the remaining stuff
	int nchunks = status_len;
	size_t chunk_len = PNG_IMAGE_SIZE(image) / nchunks;
	struct fill_image_data *image_datas;
	image_datas = calloc(nchunks+1, sizeof(struct fill_image_data));

	// Allocate space for the thread pointers
	pthread_t *threads;
	threads = calloc(nchunks+1, sizeof(pthread_t));

	// Display a list of statuses for comparison
	printf("%s\n", statuses);

	// i = chunk index, j = index of data to start processing
	int i, j;
	j = 0;
	for (i = 0; i < nchunks; i++) {
		image_datas[i].image = &image;
		image_datas[i].buffer = buffer;
		image_datas[i].start = j;
		image_datas[i].end = j + chunk_len;
		image_datas[i].finish_message = statuses[i];

		// Start a worker thread
		pthread_create(threads+i, NULL, fill_image, image_datas+i);
		j += chunk_len;
	#if 1 // This limits number of threads. It can make it faster, can make it slower.
		if (i % 8 == 0) pthread_join(threads[i], NULL);
	#endif
	}
	// Process the remaining parts
	image_datas[i].image = &image;
	image_datas[i].buffer = buffer;
	image_datas[i].start = j;
	image_datas[i].end = PNG_IMAGE_SIZE(image);
	image_datas[i].finish_message = statuses[i];
	pthread_create(threads+i, NULL, fill_image, image_datas+i);

	// Wait for completion
	for (i = 0; i < nchunks+1; i++) {
		pthread_join(threads[i], NULL);
	}

	png_image_write_to_file(&image, "out.png", 0, buffer, 0, NULL);

	free(threads);
	free(image_datas);
	free(buffer);

	printf("\n");

	return 0;
}
