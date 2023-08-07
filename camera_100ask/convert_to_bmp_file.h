#ifndef CONVERT_TO_BMP_FILE_H
#define CONVERT_TO_BMP_FILE_H

#include "types.h"

int CvtRgb2BMPFileFrmFrameBuffer(unsigned char * pRgb, UINT32 dwWidth, UINT32 dwHeight, UINT32 dwBpp, char *outfilename);

#endif /*CONVERT_TO_BMP_FILE_H*/
