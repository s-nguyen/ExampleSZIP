/*
 * ExampleSZIP.c
 *
 *  Created on: May 6, 2017
 *      Author: yorha
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <szlib.h>

#define w 800
#define h 800

long test_encoding(int bits_per_pixel, char *in, long size, char *out, long buffer_size);
long test_decoding(int bits_per_pixel, char *in, long size, char *out, long out_size, long buffer_size);

int main() {
	FILE *image = fopen("/home/yorha/workspace/ExampleSZIP/white.raw", "rb");
	fseek(image, 0, SEEK_END);
	int sz = ftell(image);
	rewind(image);
	char ibuf[sz];
	char obuf[sz];
	char dbuf[sz+15];
	char* dbufOffset = dbuf+15;
	fread(ibuf, 1, sz, image);
	fclose(image);
	printf("Input %d bytes\n", sz);

	long int osz;
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);
	osz = test_encoding(8, ibuf, sz, obuf, 1);
	clock_gettime(CLOCK_MONOTONIC, &stop);
	printf("Comprerssed %ld Bytes. Time taken: %ld.%09ld\n", osz, (stop.tv_sec - start.tv_sec), (stop.tv_nsec - start.tv_nsec));

	printf("Decompressed %ld Bytes\n", test_decoding(8, obuf, osz, dbufOffset, sizeof(dbuf), 1));
	image = fopen("decompressed.pgm", "wb");
	snprintf(dbuf, 15, "P5\n800\n800\n255\n");
	fwrite(dbuf, 1, sizeof(dbuf), image);
	fclose(image);

	printf("Done\n");
	return 0;
}

long test_encoding(int bits_per_pixel, char *in, long size, char *out, long buffer_size){
	sz_stream c_stream;
	int bytes_per_pixel;
	int err;
	int len;

	c_stream.hidden = 0;

	c_stream.options_mask = SZ_RAW_OPTION_MASK | SZ_NN_OPTION_MASK
			| SZ_MSB_OPTION_MASK;
	c_stream.bits_per_pixel = bits_per_pixel;
	c_stream.pixels_per_block = 8;
	c_stream.pixels_per_scanline = 16;

	bytes_per_pixel = (bits_per_pixel + 7) / 8;
	if (bytes_per_pixel == 3)
		bytes_per_pixel = 4;

	c_stream.image_pixels = size / bytes_per_pixel;

	len = size;

	c_stream.next_in = in;
	c_stream.total_in = 0;

	c_stream.next_out = out;
	c_stream.total_out = 0;

	err = SZ_CompressInit(&c_stream);
	if (err != SZ_OK) {
		fprintf(stderr, "SZ_CompressInit error: %d\n", err);
		exit(1);
	}


	for (;;) {
		c_stream.avail_out = buffer_size;
		err = SZ_Compress(&c_stream, SZ_FINISH);

		//printf("output byte=%02X\n", c_stream.next_out[-1]);

		if (err == SZ_STREAM_END)
			break;

		if (err != SZ_OK) {
			fprintf(stderr, "SZ_Compress error: %d\n", err);
			exit(1);
		}
	}

	err = SZ_CompressEnd(&c_stream);
	if (err != SZ_OK) {
		fprintf(stderr, "SZ_CompressEnd error: %d\n", err);
		exit(1);
	}

	{
		int i;

		if (c_stream.total_out < 30) {
			printf("total_out=%ld\n", c_stream.total_out);
			for (i = 0; i < c_stream.total_out; i++)
				printf("%02X", out[i]);

			printf("\n");
		}
	}

	return c_stream.total_out;
}


long test_decoding(int bits_per_pixel, char *in, long size, char *out, long out_size, long buffer_size){
	int bytes_per_pixel;
	int err;
	sz_stream d_stream;

	strcpy((char*)out, "garbage");

	d_stream.hidden = 0;

	d_stream.next_in  = in;
	d_stream.next_out = out;

	d_stream.avail_in = 0;
	d_stream.avail_out = 0;

	d_stream.total_in = 0;
	d_stream.total_out = 0;

	d_stream.options_mask = SZ_RAW_OPTION_MASK | SZ_NN_OPTION_MASK | SZ_MSB_OPTION_MASK;
	d_stream.bits_per_pixel = bits_per_pixel;
	d_stream.pixels_per_block = 8;
	d_stream.pixels_per_scanline = 16;

	bytes_per_pixel = (bits_per_pixel + 7)/8;
	if (bytes_per_pixel == 3)
		bytes_per_pixel = 4;

	d_stream.image_pixels = out_size/bytes_per_pixel;

	err = SZ_DecompressInit(&d_stream);
	if (err != SZ_OK)
		{
		fprintf(stderr, "SZ_DecompressEnd error: %d\n", err);
		exit(1);
		}

	while (d_stream.total_in < size)
		{
		d_stream.avail_in = d_stream.avail_out = buffer_size;
		if (d_stream.avail_in + d_stream.total_in > size)
			d_stream.avail_in = size - d_stream.total_in;

		err = SZ_Decompress(&d_stream, SZ_NO_FLUSH);
		if (err == SZ_STREAM_END)
			break;

		if (err != SZ_OK)
			{
			fprintf(stderr, "SZ_Decompress error: %d\n", err);
			exit(1);
			}
		}

	while (d_stream.total_out < out_size)
		{
		d_stream.avail_out = buffer_size;
		err = SZ_Decompress(&d_stream, SZ_FINISH);
		if (err == SZ_STREAM_END)
			break;

		if (err != SZ_OK)
			{
			fprintf(stderr, "SZ_Decompress error: %d\n", err);
			exit(1);
			}
		}

	err = SZ_DecompressEnd(&d_stream);
	if (err != SZ_OK)
		{
		fprintf(stderr, "SZ_DecompressEnd error: %d\n", err);
		exit(1);
		}

	return d_stream.total_out;
}
