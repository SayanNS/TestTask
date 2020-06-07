#include <stdio.h>
#include <stdlib.h>
#include "bmpreader.h"

RGB* loadBitmapImage(char *file, int *width, int *height)
{
	FILE *pFile;
	pFile = fopen(file, "rb");

	if (pFile == NULL)
	{
		printf("Couldn't open bitmap file.\n");
		return 0;
	}

	BITMAPFILEHEADER bitmapFileHeader;
	fread(&bitmapFileHeader, sizeof(bitmapFileHeader), 1, pFile);

	BITMAPINFOHEADER bitmapInfoHeader;
	fread(&bitmapInfoHeader, sizeof(bitmapInfoHeader), 1, pFile);

	if (bitmapInfoHeader.biBitCount != 24)
	{
		printf("Color depth is not rgb.\n");
		return 0;
	}

	if (bitmapInfoHeader.biCompression != 0)
	{
		printf("Image uses compression methods.\n");
		return 0;
	}

	RGB *data = new RGB[bitmapInfoHeader.biWidth * bitmapInfoHeader.biHeight];

	int rowSize = sizeof(RGB) * bitmapInfoHeader.biWidth;
	rowSize += (rowSize ^ 3 + 1) & 3;

	if (bitmapInfoHeader.biHeight > 0)
	{
		for (int i = 0; i < bitmapInfoHeader.biHeight; i++)
		{
			fseek(pFile, bitmapFileHeader.bfOffBits + i * rowSize, SEEK_SET);
			fread(data + (bitmapInfoHeader.biHeight - i - 1) * bitmapInfoHeader.biWidth, sizeof(RGB), bitmapInfoHeader.biWidth, pFile);
		}
	}
	else
	{
		bitmapInfoHeader.biHeight = bitmapInfoHeader.biHeight ^ -1 + 1; //reverse

		for (int i = 0; i < bitmapInfoHeader.biHeight; i++)
		{
			fseek(pFile, bitmapFileHeader.bfOffBits + i * rowSize, SEEK_SET);
			fread(data + i * bitmapInfoHeader.biWidth, sizeof(RGB), bitmapInfoHeader.biWidth, pFile);
		}

	}

	fclose(pFile);

	*width = bitmapInfoHeader.biWidth;
	*height = bitmapInfoHeader.biHeight;

	return data;
}
