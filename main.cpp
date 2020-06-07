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
	yuv.u = (pixel.r * Ur + pixel.g * Ug + pixel.b * Ub + 127) >> 2;
	yuv.v = (pixel.r * Vr + pixel.g * Vg + pixel.b * Vb + 127) >> 2;

	return yuv;
}

#endif

void fromRGBToYUV420(RGB *pixels, int from, int to, int width, BYTE *y, BYTE *u, BYTE *v)
{
	for (int i = from; i < to; i++)
	{
		YUV yuv = fromRGBToYUV(pixels[i]);

		y[i] = yuv.y;

		int offset = (width >> 1) * ((i / width) >> 1) + ((i % width) >> 1);

		u[offset] += yuv.u;
		v[offset] += yuv.v;
	}
}

void overlayFrame(BYTE *yOffset, BYTE *uOffset, BYTE *vOffset, BYTE *y, BYTE *u, BYTE *v, int videoWidth, int width, int from, int to,int height)
{
	for (int l = from; l < to; l++)
	{
		for (int k = 0; k < height; k++)
		{
			yOffset[2 * k * videoWidth + 2 * l] = y[2 * k * width + 2 * l];
			yOffset[2 * k * videoWidth + 2 * l + 1] = y[2 * k * width + 2 * l + 1];
			yOffset[(2 * k + 1) * videoWidth + 2 * l] = y[(2 * k + 1) * width + 2 * l];
			yOffset[(2 * k + 1) * videoWidth + 2 * l + 1] = y[(2 * k + 1) * width + 2 * l + 1];

			uOffset[k * (videoWidth >> 1) + l] = u[k * (width >> 1) + l];
			vOffset[k * (videoWidth >> 1) + l] = v[k * (width >> 1) + l];
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

	int videoWidth = std::stoi((std::string)width_arg);
	int videoHeight = std::stoi((std::string)height_arg);

	int width;
	int height;

	RGB *pixels = loadBitmapImage(bmp_file, &width, &height);

	if (pixels == NULL)
	{
		return 0;
	}

	int pixelsCount = width * height;

	BYTE *y = new BYTE[pixelsCount];
	BYTE *u = new BYTE[pixelsCount >> 2];
	BYTE *v = new BYTE[pixelsCount >> 2];

	for (int i = 0; i < pixelsCount >> 2; i++)
	{
		u[i] = 0;
		v[i] = 0;
	}

	int numCores = std::thread::hardware_concurrency();

	if (numCores == 0)
	{
		fromRGBToYUV420(pixels, 0, width * height, width, y, u, v);

		delete [] pixels;

		int size = videoWidth * videoHeight + ((videoWidth * videoHeight) >> 1);
		BYTE *frame = new BYTE[size];
		
		while (true)
		{
			fread(frame, sizeof(BYTE), size, pInFile);

			if(feof(pInFile))
			{
				delete [] frame;
				break;
			}
			
			int widthOffset = (videoWidth - width) >> 1;
			widthOffset += (widthOffset & 1);

			int heightOffset = (videoHeight - height) >> 1;
			heightOffset += (heightOffset & 1);

			BYTE *yOffset = frame + heightOffset * videoWidth + widthOffset;
			BYTE *uOffset = frame + ((heightOffset * videoWidth) >> 2) + (widthOffset >> 1) + videoWidth * videoHeight;
			BYTE *vOffset = uOffset + (videoWidth * videoHeight >> 2);

			overlayFrame(yOffset, uOffset, vOffset, y, u, v,
					videoWidth, width, 0, width >> 1, height >> 1);

			fwrite(frame, sizeof(BYTE), size, pOutFile);
		}
	}
	else
	{
		int division = pixelsCount / numCores;
		int modulo = pixelsCount % numCores;

		std::thread *threads = new std::thread[numCores];

		for (int i = 0; i < modulo; i++)
		{
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
		
		delete [] pixels;

		int size = videoWidth * videoHeight + ((videoWidth * videoHeight) >> 1);
		BYTE *frame = new BYTE[size];

		while (true)
		{
			fread(frame, sizeof(BYTE), size, pInFile);

			if(feof(pInFile))
			{
				delete [] frame;
				break;
			}
			
			int widthOffset = (videoWidth - width) >> 1;
			widthOffset += (widthOffset & 1);

			int heightOffset = (videoHeight - height) >> 1;
			heightOffset += (heightOffset & 1);

			division = (width >> 1) / numCores;
			modulo = (width >> 1) % numCores;

			BYTE *yOffset = frame + heightOffset * videoWidth + widthOffset;
			BYTE *uOffset = frame + ((heightOffset * videoWidth) >> 2) + (widthOffset >> 1) + videoWidth * videoHeight;
			BYTE *vOffset = uOffset + (videoWidth * videoHeight >> 2);

			for (int i = 0; i < modulo; i++)
			{
				threads[i] = std::thread(overlayFrame, yOffset, uOffset, vOffset, y, u, v,
						videoWidth, width, (division + 1) * i, (division + 1) * (i + 1), height >> 1);
			}

			for (int i = modulo; i < numCores; i++)
			{
				threads[i] = std::thread(overlayFrame, yOffset, uOffset, vOffset, y, u, v,
						videoWidth, width, division * i + modulo, division * (i + 1) + modulo, height >> 1);
			}

			for (int i = 0; i < modulo; i++)
			{
				threads[i].join();
			}

			for (int i = modulo; (i < numCores) && (division > 0); i++)
			{
				threads[i].join();
			}

			fwrite(frame, sizeof(BYTE), size, pOutFile);
		}
	}

	fclose(pInFile);
	fclose(pOutFile);

	delete [] y;
	delete [] u;
	delete [] v;

	return 0;
}
