#include "include/shared_container.h"
#include "include/bmpreader.h"
#include "include/config.h"
#include <stdio.h>
#include <thread>

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

void overlayFrame(BYTE *yFrame, BYTE *uFrame, BYTE *vFrame)
{
	for (int i = 0; i < imageHeight / 2; i++)
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
}

void processingThreadFunction(SharedContainer *container)
{
	while (true)
	{
		BYTE *frame = container->getFrame();

		if (frame == NULL)
		{
			break;
		}

		overlayFrame(frame + yOffset, frame + uOffset, frame + vOffset);
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

	FILE *pInFile;
	FILE *pOutFile;

	pInFile = fopen(yuv_file, "rb");
	pOutFile = fopen(output_file, "wb");

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

	if (numCores == 0 || numCores == 1 || FORCE_SINGLE_THREADING)
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

			overlayFrame(frame + yOffset, frame + uOffset, frame + vOffset);

			fwrite(frame, sizeof(BYTE), bufferSize, pOutFile);
		}
	}
	else
	{
		SharedContainer container(numCores, bufferSize);

		std::thread *processingThreads = new std::thread[numCores];

		for (int i = 0; i < numCores; i++)
		{
			processingThreads[i] = std::thread(processingThreadFunction, &container);
		}

		BYTE *buffer = new BYTE[bufferSize * numCores];
		int bytesNum = fread(buffer, sizeof(BYTE), bufferSize * numCores, pInFile);
		container.changeBuffer(buffer, bytesNum);

		buffer = new BYTE[bufferSize * numCores];
		
		while (true)
		{
			if (feof(pInFile))
			{
				delete [] buffer;
				buffer = container.changeBuffer(buffer, 0);
				fwrite(buffer, sizeof(BYTE), bytesNum, pOutFile);
				delete [] buffer;
				break;
			}

			bytesNum = fread(buffer, sizeof(BYTE), bufferSize * numCores, pInFile);
			buffer = container.changeBuffer(buffer, bytesNum);
			fwrite(buffer, sizeof(BYTE), bufferSize * numCores, pOutFile);
		}

		for (int i = 0; i < numCores; i++)
		{
			processingThreads[i].join();
		}

		delete [] processingThreads;
	}

	fclose(pInFile);
	fclose(pOutFile);

	delete [] y;
	delete [] u;
	delete [] v;

	return 0;
}
