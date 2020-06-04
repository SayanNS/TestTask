#include "bmpreader.h"
#include "shared_container.h"
#include <stdio.h>
#include <thread>

//#define SDTV
#define HDTV

#ifdef SDTV

#define Yr 0.299
#define Yg 0.587
#define Yb 0.114
#define Ur -0.169
#define Ug -0.331
#define Ub 0.5
#define Vr 0.5
#define Vg -0.419
#define Vb -0.081

#endif

#ifdef HDTV

#define Yr 0.213
#define Yg 0.715
#define Yb 0.072
#define Ur -0.115
#define Ug -0.385
#define Ub 0.5
#define Vr 0.5
#define Vg -0.454
#define Vb -0.046

#endif

struct YUV
{
	BYTE y;
	BYTE u;
	BYTE v;
};

#ifdef __SSE__

#include <xmmintrin.h>

YUV fromRGBToYUV(RGB pixel)
{
	__m128 _yuv, _rgb, _a;

	_rgb = _mm_set1_ps(pixel.r);
	_a = _mm_set_ps(Yr, Ur /4, Vr / 4, 0);
	_yuv = _mm_mul_ps(_rgb, _a);

	_rgb = _mm_set1_ps(pixel.g);
	_a = _mm_set_ps(Yg, Ug / 4, Vg / 4, 0);
	_a = _mm_mul_ps(_rgb, _a);
	_yuv = _mm_add_ps(_yuv, _a);

	_rgb = _mm_set1_ps(pixel.b);
	_a = _mm_set_ps(Yb, Ub / 4, Vb / 4, 0);
	_a = _mm_mul_ps(_rgb, _a);
	_yuv = _mm_add_ps(_yuv, _a);

	_a = _mm_set_ps(0, 127 / 4, 127 / 4, 0);
	_yuv = _mm_add_ps(_yuv, _a);

	float res[4];
	_mm_store_ps(res, _yuv);

	YUV yuv;
	yuv.y = (BYTE)res[3];
	yuv.u = (BYTE)res[2];
	yuv.v = (BYTE)res[1];

	return yuv;
}

#else

YUV fromRGBToYUV(RGB pixel)
{
	YUV yuv;

	yuv.y = pixel.r * Yr + pixel.g * Yg + pixel.b * Yb;
	yuv.u = (pixel.r * Ur + pixel.g * Ug + pixel.b * Ub + 127) / 4;
	yuv.v = (pixel.r * Vr + pixel.g * Vg + pixel.b * Vb + 127) / 4;

	return yuv;
}

#endif

void fromRGBToYUV420(RGB *pixels, int from, int to, int width, BYTE *y, BYTE *u, BYTE *v)
{
	for (int i = from; i < to; i++)
	{
		YUV yuv = fromRGBToYUV(pixels[i]);

		y[i] = yuv.y;

		int offset = ((width + 1) >> 1) * ((i / width) >> 1) + ((i % width) >> 1);

		u[offset] += yuv.u;
		v[offset] += yuv.v;
	}
}

void avgArrays(BYTE *a, BYTE *b, int aWidth, int aHeight, int bWidth, int bHeight)
{
        for (int i = 0; i < bHeight; i++)
        {
                for (int j = 0; j < bWidth; j++)
                {
                        a[i * aWidth + j] = a[i * aWidth + j] / 2 + b[i * bWidth + j] / 2;
                }
        }
}

int parseNumber(char *number_str)
{
	int number = 0;

	for (int i = 0; number_str[i]; i++)
	{
		number *= 10;
		number += number_str[i] - 48;
	}

	return number;
}

void readThreadFunction(SharedPool *Pop, SharedContainer *Push, int bufferSize, FILE *pFile)
{
	int i = 0;

	while (true)
	{
		printf("reading frame: %d\n", i++);

		Node *node = Pop->pop();

		if (node == NULL)
		{
			node = new Node();
			node->buffer = new unsigned char[bufferSize];
		}

		fread(node->buffer, sizeof(unsigned char), bufferSize, pFile);

		if (feof(pFile))
		{
			Push->push(NULL);
			break;
		}
		
		Push->push(node);
	}
}

void processThreadFunction(SharedContainer *Pop, SharedContainer *Push, int videoWidth, int videoHeight, int width, int height, BYTE *y, BYTE *u, BYTE *v)
{
	int i = 0;

	int offset = (videoWidth - width) >> 1;
        offset += (offset & 1);

        int heightOffset = (videoHeight - height) >> 1;
        heightOffset += (heightOffset & 1);

	while (true)
	{
		printf("processing frame: %d\n", i++);

		Node *node = Pop->pop();

		if (node == NULL)
		{
			Push->push(NULL);
			break;
		}

		avgArrays(node->buffer + offset + heightOffset * videoWidth, y, videoWidth, videoHeight, width, height);
                avgArrays(node->buffer + videoWidth * videoHeight + (offset >> 1) + (heightOffset >> 1) * (videoWidth >> 1),
                                u, videoWidth >> 1, videoHeight, width >> 1, height >> 1);
                avgArrays(node->buffer + videoWidth * videoHeight + (videoWidth >> 1) * (videoHeight >> 1) + (offset >> 1) + (heightOffset >> 1) * (videoWidth >> 1),
                                v, videoWidth >> 1, videoHeight, width >> 1, height >> 1);
		Push->push(node);
	}
}

void writeThreadFunction(SharedContainer *Pop, SharedPool *Push, int bufferSize, FILE *pFile)
{
	int i = 0;

	while (true)
	{
		printf("writing frame: %d\n", i++);

		Node *node = Pop->pop();

		if (node == NULL)
		{
			break;
		}

		fwrite(node->buffer, sizeof(unsigned char), bufferSize, pFile);		
		Push->push(node);
	}
}

int main(int argc, char **argv)
{
	char *bmp_file, *yuv_file, *output_file, *width_arg, *height_arg;
	bmp_file = yuv_file = output_file = width_arg = height_arg = NULL;

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
		{
			continue;
		}

		switch (argv[i][1])
		{
		case 'b':
			bmp_file = argv[++i];
			break;
		case 'y':
			yuv_file = argv[++i];
			break;
		case 'o':
			output_file = argv[++i];
			break;
		case 'h':
			height_arg = argv[++i];
			break;
		case 'w':
			width_arg = argv[++i];
			break;
		}
	}

	if (bmp_file == NULL)
	{
		printf("BMP file is not specified\n");
		return 0;
	}

	if (yuv_file == NULL)
	{
		printf("YUV file is not specified\n");
		return 0;
	}
	
	if (width_arg == NULL)
	{
		printf("Width is not specified\n");
		return 0;
	}

	if (height_arg == NULL)
	{
		printf("Height is not specified\n");
		return 0;
	}

	if (output_file == NULL)
	{
		output_file == "output.yuv";
	}

	FILE *pInFile;
	FILE *pOutFile;

	pInFile = fopen(yuv_file, "rb");
	pOutFile = fopen(output_file, "wb");

	if (pInFile == NULL)
	{
		printf("YUV file is not found\n");
		return 0;
	}

	int videoWidth = parseNumber(width_arg);
	int videoHeight = parseNumber(height_arg);

	int width;
	int height;

	RGB *pixels = loadBitmapImage(bmp_file, &width, &height);

	if (pixels == NULL)
	{
		return 0;
	}

	int uvCount = ((width + 1) >> 1) * ((height + 1) >> 1);
	int pixelsCount = width * height;

	BYTE *y = new BYTE[width * height];
	BYTE *u = new BYTE[uvCount];
	BYTE *v = new BYTE[uvCount];

	for (int i = 0; i < uvCount; i++)
	{
		u[i] = 0;
		v[i] = 0;
	}

	int numCores = std::thread::hardware_concurrency();

	if (numCores == 0)
	{
		fromRGBToYUV420(pixels, 0, width * height, width, y, u, v);
	}
	else
	{
		std::thread *threads = new std::thread[numCores];

		int division = pixelsCount / numCores;
		int modulo = pixelsCount % numCores;

		for (int i = 0; i < modulo; i++)
		{
			int row = (division + 1) * i / (width >> 1);
			int pos = (division + 1) * i % (width >> 1);

			threads[i] = std::thread(fromRGBToYUV420, pixels, (division + 1) * i, (division + 1) * (i + 1), width, y, u, v);
		}

		for (int i = modulo; (i < numCores) && (division > 0); i++)
		{
			threads[i] = std::thread(fromRGBToYUV420, pixels, division * i + modulo, division * (i + 1) + modulo, width, y, u, v); 
		}

		for (int i = 0; i < modulo; i++)
		{
			threads[i].join();
		}

		for (int i = modulo; (i < numCores) && (division > 0); i++)
		{
			threads[i].join();
		}

		delete [] threads;
	}

	delete [] pixels;

	{
		SharedPool a;
		SharedContainer b;
		SharedContainer c;

		int size = videoWidth * videoHeight + 2 * ((videoWidth + 1) >> 1) * ((videoHeight + 1) >> 1);

		std::thread read_thread(readThreadFunction, &a, &b, size, pInFile);
		std::thread process_thread(processThreadFunction, &b, &c, videoWidth, videoHeight, width, height, y, u, v);
		std::thread write_thread(writeThreadFunction, &c, &a, size, pOutFile);

		read_thread.join();
		process_thread.join();
		write_thread.join();
	}

	fwrite(y, sizeof(BYTE), width * height, pOutFile);
	fwrite(u, sizeof(BYTE), uvCount, pOutFile);
	fwrite(v, sizeof(BYTE), uvCount, pOutFile);

	fclose(pInFile);
	fclose(pOutFile);

	delete [] y;
	delete [] u;
	delete [] v;

	return 0;
}
