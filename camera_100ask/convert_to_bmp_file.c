#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "convert_to_bmp_file.h"

typedef struct tagBITMAPFILEHEADER { 
  UINT16   bfType; 
  UINT32   bfSize; 
  UINT16   bfReserved1; 
  UINT16   bfReserved2; 
  UINT32   bfOffBits; 
} __attribute__ ((packed)) BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
  UINT32   biSize; 
  UINT32   biwidth; 
  UINT32   biheight; 
  UINT16   biPlanes; 
  UINT16   biBitCount; 
  UINT32   biCompression; 
  UINT32   biSizeImage; 
  UINT32   biXPelsPerMeter; 
  UINT32   biYPelsPerMeter; 
  UINT32   biClrUsed; 
  UINT32   biClrImportant; 
} __attribute__ ((packed)) BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  UINT8    rgbBlue; 
  UINT8    rgbGreen; 
  UINT8    rgbRed; 
  UINT8    rgbReserved; 
} __attribute__ ((packed)) RGBQUAD;




int CvtRgb2BMPFileFrmFrameBuffer(unsigned char * pRgb, UINT32 dwWidth, UINT32 dwHeight, UINT32 dwBpp, char *outfilename)
{
    BITMAPFILEHEADER tBmpFileHead;
    BITMAPINFOHEADER tBmpInfoHead;

    UINT32 dwSize;

    unsigned char *pPos = 0;

	FILE * fout;

    memset(&tBmpFileHead, 0, sizeof(BITMAPFILEHEADER));
    memset(&tBmpInfoHead, 0, sizeof(BITMAPINFOHEADER));
    
    fout = fopen(outfilename, "w");

    if (!fout)
    {
        printf("Can't create output file %s\n", outfilename);
        return -2;
    }

    tBmpFileHead.bfType     = 0x4d42;
    tBmpFileHead.bfSize     = 0x36 + dwWidth * dwHeight * (dwBpp / 8);
    tBmpFileHead.bfOffBits  = 0x00000036;

    tBmpInfoHead.biSize     = 0x00000028;
    tBmpInfoHead.biwidth    = dwWidth;
    tBmpInfoHead.biheight   = dwHeight;
    tBmpInfoHead.biPlanes   = 0x0001;
    tBmpInfoHead.biBitCount = dwBpp;
    tBmpInfoHead.biCompression  = 0;
    tBmpInfoHead.biSizeImage    = dwWidth * dwHeight * (dwBpp / 8);
    tBmpInfoHead.biXPelsPerMeter    = 0;
    tBmpInfoHead.biYPelsPerMeter    = 0;
    tBmpInfoHead.biClrUsed  = 0;
    tBmpInfoHead.biClrImportant     = 0;

    //printf("dwBpp=%d, dwWidth=%d, dwHeight=%d\n", dwBpp, dwWidth, dwHeight);

    if (fwrite(&tBmpFileHead, 1, sizeof(tBmpFileHead), fout) != sizeof(tBmpFileHead))
    {
        printf("Can't write BMP File Head to %s\n", outfilename);
        return -3;
    }

    if (fwrite(&tBmpInfoHead, 1, sizeof(tBmpInfoHead), fout) != sizeof(tBmpInfoHead))
    {
        printf("Can't write BMP File Info Head to %s\n", outfilename);
        return -4;
    }

    dwSize = dwWidth * dwBpp / 8;
    pPos   = pRgb + (dwHeight - 1) * dwSize;

    while (pPos >= pRgb)
    {
        if (fwrite(pPos, 1, dwSize, fout) != dwSize)
        {
            printf("Can't write date to BMP File %s\n", outfilename);
            return -5;
        }    
        pPos -= dwSize;
    }
    
    fclose(fout);
        
    return 0;
}

