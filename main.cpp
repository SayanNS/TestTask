#include "shared_container.h"
#include "bmpreader.h"
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
	yuv.u = (pixel.r * Ur + pixel.g * Ug + pixel.b * Ub) / 4;
	yuv.v = (pixel.r * Vr + pixel.g * Vg + pixel.b * Vb) / 4;

	return yuv;
}

#endif

int imageWidth, imageHeight, videoWidth, videoHeight;
int yOffset, uOffset, vOffset;
BYTE *y, *u, *v;

void overlayFrame(BYTE *yFrame, BYTE *uFrame, BYTE *vFrame, int from, int to)
{
	for (int l = from; l < to; l++)
	{
		for (int k = 0; k < imageHeight / 2; k++)
		{
		}
	}
}

void processingThreadFunction(SharedContainer *container, int from, int to)
{
	int from_div = from / (imageWidth / 2);
	int from_mod = from / (imageWidth / 2);
	int to_div = to / (imageWidth / 2);
	int to_mod = to / (imageWidth / 2);

	if (from_div == to_div)
	{

	}
	else
	{
		while (true)
		{
			BYTE *p = container->getPointer();

			if (p == NULL)
			{
				break;
			}

			BYTE *yFrame = p + yOffset;
			BYTE *uFrame = p + uOffset;
			BYTE *vFrame = p + vOffset;

			for (int j = from_mod; j < imageWidth / 2; j++)
			{
				yFrame[2 * from_div * videoWidth + 2 * j] = y[2 * from_div * imageWidth + 2 * j];
				yFrame[2 * from_div * videoWidth + 2 * j + 1] = y[2 * from_div * imageWidth + 2 * j + 1];
				yFrame[(2 * from_div + 1) * videoWidth + 2 * j] = y[(2 * from_div + 1) * imageWidth + 2 * j];
				yFrame[(2 * from_div + 1) * videoWidth + 2 * j + 1] = y[(2 * from_div + 1) * imageWidth + 2 * j + 1];

				uFrame[from_div * (videoWidth / 2) + j] = u[from_div * (imageWidth / 2) + j];
				vFrame[from_div * (videoWidth / 2) + j] = v[from_div * (imageWidth / 2) + j];
			}

			for (int i = from_mod + 1; i < to_mod; i++)
			{
				for (int j = 0; j < imageWidth / 2; j++)
				{
					yFrame[2 * i * videoWidth + 2 * j] = y[2 * i * imageWidth + 2 * j];
					yFrame[2 * i * videoWidth + 2 * j + 1] = y[2 * i * imageWidth + 2 * j + 1];
					yFrame[(2 * i + 1) * videoWidth + 2 * j] = y[(2 * i + 1) * imageWidth + 2 * j];
					yFrame[(2 * i + 1) * videoWidth + 2 * j + 1] = y[(2 * i + 1) * imageWidth + 2 * j + 1];

					uFrame[i * (videoWidth / 2) + j] = u[i * (imageWidth / 2) + j];
					vFrame[i * (videoWidth / 2) + j] = v[i * (imageWidth / 2) + j];
				}
			}

			for (int j = 0; j < to_mod; j++)
			{
				yFrame[2 * to_div * videoWidth + 2 * j] = y[2 * to_div * imageWidth + 2 * j];
				yFrame[2 * to_div * videoWidth + 2 * j + 1] = y[2 * to_div * imageWidth + 2 * j + 1];
				yFrame[(2 * to_div + 1) * videoWidth + 2 * j] = y[(2 * to_div + 1) * imageWidth + 2 * j];
				yFrame[(2 * to_div + 1) * videoWidth + 2 * j + 1] = y[(2 * to_div + 1) * imageWidth + 2 * j + 1];

				uFrame[to_div * (videoWidth / 2) + j] = u[to_div * (imageWidth / 2) + j];
				vFrame[to_div * (videoWidth / 2) + j] = v[to_div * (imageWidth / 2) + j];
			}

			container->processed();
		}
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
		output_file = "output.yuv";
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

	videoWidth = std::stoi(width_arg);
	videoHeight = std::stoi(height_arg);

	RGB *pixels = loadBitmapImage(bmp_file, &imageWidth, &imageHeight);

	if (pixels == NULL)
	{
		return 0;
	}

	y = new BYTE[imageWidth * imageHeight];
	u = new BYTE[imageWidth * imageHeight / 4];
	v = new BYTE[imageWidth * imageHeight / 4];

	for (int i = 0; i < imageWidth * imageHeight / 4; i++)
	{
		u[i] = 127;
		v[i] = 127;
	}

	for (int i = 0; i < imageHeight; i++)
	{
		for (int j = 0; j < imageWidth; j++)
		{
			YUV yuv = fromRGBToYUV(pixels[i * imageWidth + j]);

			y[i * imageWidth + j] = yuv.y;

			int offset = (i / 2) * (imageWidth / 2) + j / 2;

			u[offset] += yuv.u;
			v[offset] += yuv.v;
		}
	}
	
	delete [] pixels;

	int widthOffset = (videoWidth - imageWidth) / 2;
	widthOffset += (widthOffset & 1);

	int heightOffset = (videoHeight - imageHeight) / 2;
	heightOffset += (heightOffset & 1);

	yOffset = heightOffset * videoWidth + widthOffset;
	uOffset = heightOffset * videoWidth / 4 + widthOffset / 2 + videoWidth * videoHeight;
	vOffset = uOffset + videoWidth * videoHeight / 4;

	int bufferSize = videoWidth * videoHeight + videoWidth * videoHeight / 2;
	int numCores = std::thread::hardware_concurrency();

	if (numCores == 0 || numCores == 1)
	{
		BYTE *frame = new BYTE[bufferSize];
		
		while (true)
		{
			fread(frame, sizeof(BYTE), bufferSize, pInFile);

			if(feof(pInFile))
			{
				delete [] frame;
				break;
			}

			overlayFrame(frame + yOffset, frame + uOffset, frame + vOffset, 0, imageWidth / 2);

			fwrite(frame, sizeof(BYTE), bufferSize, pOutFile);
		}
	}
	else
	{
		SharedStack stack(bufferSize);
		SharedQueue read_queue;
		SharedQueue write_queue;
		SharedContainer container(&read_queue, &write_queue, numCores);

		std::thread *processingThreads = new std::thread[numCores];

		int div = (imageWidth * imageHeight / 4) / numCores;
		int mod = (imageWidth * imageHeight / 4) & numCores;

		for (int i = 0; i < mod; i++)
		{
			processingThreads[i] = std::thread(processingThreadFunction, &container, (div + 1) * i, (div + 1) * (i + 1));
		}

		for (int i = mod; i < numCores; i++)
		{
			processingThreads[i] = std::thread(processingThreadFunction, &container, div * i + mod, div * (i + 1) + mod);
		}

		while (true)
		{
			BYTE *buffer = stack.pop();

			if (buffer == NULL)
			{
				buffer = write_queue.pop();
				fwrite(buffer, sizeof(BYTE), bufferSize, pOutFile);
			}

			fread(buffer, sizeof(BYTE), bufferSize, pInFile);

			if(feof(pInFile))
			{
				read_queue.push(NULL);
				stack.push(buffer);

				while (buffer = write_queue.pop())
				{
					fwrite(buffer, sizeof(BYTE), bufferSize, pOutFile);
					stack.push(buffer);
				}

				break;
			}

			read_queue.push(buffer);
		}

		stack.push(NULL);

		while (BYTE *buffer = stack.pop())
		{
			delete [] buffer;
		}

		for (int i = 0; i < numCores; i++)
		{
			processingThreads[i].join();
		}
	}

	fclose(pInFile);
	fclose(pOutFile);

	delete [] y;
	delete [] u;
	delete [] v;

	return 0;
}
