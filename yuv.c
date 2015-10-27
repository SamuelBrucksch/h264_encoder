#include <stdio.h>
#include <string.h>

#include <xdc/std.h>

#include <ti/sdo/dmai/BufTab.h>
#include <ti/sdo/dmai/BufferGfx.h>

#include "yuv.h"

Int readFrame420P(Buffer_Handle hBuf, FILE *outFile, Int imageHeight)
{
    Int8 *yPtr = Buffer_getUserPtr(hBuf);
    Int8 *cbcrPtr;
    Int y, x;

    BufferGfx_Dimensions dim;

    BufferGfx_getDimensions(hBuf, &dim);

    /* Write Y plane */
    for (y = 0; y < imageHeight; y++) {
        if (fread(yPtr, dim.width, 1, outFile) != 1) {
            fprintf(stderr,"Failed to read data from file\n");
            return -1;
        }

        yPtr += dim.lineLength;
    }

    /* Join Cb to CbCr interleaved */
    cbcrPtr = Buffer_getUserPtr(hBuf) + Buffer_getSize(hBuf) * 2 / 3;
    for (y = 0; y < imageHeight / 2; y++) {
      for (x = 0; x < dim.width; x += 2) {
        if (fread(&cbcrPtr[x], 1, 1, outFile) != 1) {
            fprintf(stderr,"Failed to read data from file\n");
            return -1;
        }
      }
      cbcrPtr += dim.lineLength;
    }

    /* Join Cr to CbCr interleaved */
    cbcrPtr = Buffer_getUserPtr(hBuf) + Buffer_getSize(hBuf) * 2 / 3;
    for (y = 0; y < imageHeight / 2; y++) {
      for (x = 1; x < dim.width; x += 2) {
        if (fread(&cbcrPtr[x], 1, 1, outFile) != 1) {
            fprintf(stderr,"Failed to read data from file\n");
            return -1;
        }
      }
      cbcrPtr += dim.lineLength;
    }

    printf("Read 420P frame size %d (%dx%d) from file\n",
           (Int) (dim.width * 3 / 2 * imageHeight),
           (Int) dim.width, (Int) imageHeight);

    Buffer_setNumBytesUsed(hBuf, Buffer_getSize(hBuf));

    return 0;
}


Int readFrame420SP(Buffer_Handle hBuf, FILE *outFile, Int imageHeight)
{
    Int8 *yPtr = Buffer_getUserPtr(hBuf);
    Int8 *cbcrPtr;
    Int y;

    BufferGfx_Dimensions dim;

    BufferGfx_getDimensions(hBuf, &dim);

    /* Write Y plane */
    for (y = 0; y < imageHeight; y++) {
        if (fread(yPtr, dim.width, 1, outFile) != 1) {
            fprintf(stderr,"Failed to read data from file\n");
            return -1;
        }

        yPtr += dim.lineLength;
    }

    /* Join Cb to CbCr interleaved */
    cbcrPtr = Buffer_getUserPtr(hBuf) + Buffer_getSize(hBuf) * 2 / 3;
    for (y = 0; y < imageHeight / 2; y++) {
        if (fread(cbcrPtr, dim.width, 1, outFile) != 1) {
            fprintf(stderr,"Failed to read data from file\n");
            return -1;
        }
        cbcrPtr += dim.lineLength;
    }

    printf("Read 420SP frame size %d (%dx%d) from file\n",
           (Int) (dim.width * 3 / 2 * imageHeight),
           (Int) dim.width, (Int) imageHeight);

    Buffer_setNumBytesUsed(hBuf, Buffer_getSize(hBuf));

    return 0;
}

