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

	yuv.y = BYTE(pixel.r * Yr + pixel.g * Yg + pixel.b * Yb);
	yuv.u = BYTE((pixel.r * Ur + pixel.g * Ug + pixel.b * Ub) / 4);
	yuv.v = BYTE((pixel.r * Vr + pixel.g * Vg + pixel.b * Vb) / 4);

	return yuv;
}

#endif

void overlayFrame(BYTE *yFrame, BYTE *uFrame, BYTE *vFrame, BYTE *y, BYTE *u, BYTE *v, int imageWidth, int imageHeight, int videoWidth, int videoHeight)
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

void processingThreadFunction(SharedContainer *container, BYTE *y, BYTE *u, BYTE *v, int imageWidth, int imageHeight, int videoWidth, int videoHeight, int yOffset, int uOffset, int vOffset)
{
	while (true)
	{
		BYTE *frame = container->getFrame();

		if (frame == NULL)
		{
			break;
		}

		overlayFrame(frame + yOffset, frame + uOffset, frame + vOffset, y, u, v, imageWidth, imageHeight, videoWidth, videoHeight);
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

	if (pInFile == NULL)
	{
		printf("YUV file not found");
		return 1;
	}

	int imageWidth, imageHeight;
	int videoWidth = atoi(width_arg);
	int videoHeight = atoi(height_arg);

	RGB *pixels = loadBitmapImage(bmp_file, &imageWidth, &imageHeight);

	if (pixels == NULL)
	{
		return 1;
	}

	BYTE *y = new BYTE[imageWidth * imageHeight];
	BYTE *u = new BYTE[imageWidth * imageHeight / 4];
	BYTE *v = new BYTE[imageWidth * imageHeight / 4];

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

	int yOffset = heightOffset * videoWidth + widthOffset;
	int uOffset = heightOffset * videoWidth / 4 + widthOffset / 2 + videoWidth * videoHeight;
	int vOffset = uOffset + videoWidth * videoHeight / 4;

	int frameSize = videoWidth * videoHeight + videoWidth * videoHeight / 2;
	int numCores = std::thread::hardware_concurrency();

	if (numCores == 0 || numCores == 1)
	{
		BYTE *frame = new BYTE[frameSize];
		
		while (true)
		{
			fread(frame, sizeof(BYTE), frameSize, pInFile);

			if(feof(pInFile))
			{
				delete [] frame;
				break;
			}

			overlayFrame(frame + yOffset, frame + uOffset, frame + vOffset, y, u, v, imageWidth, imageHeight, videoWidth, videoHeight);
			fwrite(frame, sizeof(BYTE), frameSize, pOutFile);
		}
	}
	else
	{
		// Wrapping class for sensitive data accessed by main and processing threads
		SharedContainer container(numCores, frameSize);

		std::thread *processingThreads = new std::thread[numCores];

		for (int i = 0; i < numCores; i++)
		{
			processingThreads[i] = std::thread(processingThreadFunction, &container, y, u, v, imageWidth, imageHeight, videoWidth, videoHeight, yOffset, uOffset, vOffset);
		}

		// buffer for storing data of as many frames as many threads
		BYTE *buffer = new BYTE[frameSize * numCores];
		// read as many frames as many cores on machine.
		int bytesNum = fread(buffer, sizeof(BYTE), frameSize * numCores, pInFile);
		// set buffer on processing
		container.changeBuffer(buffer, bytesNum);
		// second buffer for reading next chunk of data when first buffer is being processed
		buffer = new BYTE[frameSize * numCores];
		
		while (true)
		{
			if (feof(pInFile))
			{
				delete [] buffer;
				buffer = container.changeBuffer(nullptr, 0);
				fwrite(buffer, sizeof(BYTE), bytesNum, pOutFile);
				delete [] buffer;
				break;
			}

			bytesNum = fread(buffer, sizeof(BYTE), frameSize * numCores, pInFile);
			// gets processed buffer and sets readed buffer on processing
			buffer = container.changeBuffer(buffer, bytesNum);
			fwrite(buffer, sizeof(BYTE), frameSize * numCores, pOutFile);
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
