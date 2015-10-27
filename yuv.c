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

Int readFrameUYVY(Buffer_Handle hBuf, FILE *outFile)
{
    Int8 *ptr = Buffer_getUserPtr(hBuf);
    Int y;

    BufferGfx_Dimensions dim;

    BufferGfx_getDimensions(hBuf, &dim);

    for (y = 0; y < dim.height; y++) {
        if (fread(ptr, dim.width * 2, 1, outFile) != 1) {
            fprintf(stderr,"Failed to read data from file\n");
            return -1;
        }

        ptr += dim.lineLength;
    }

    printf("Read UYVY frame size %d (%dx%d) from file\n",
           (Int) (dim.width * 2 * dim.height),
           (Int) dim.width, (Int) dim.height);

    Buffer_setNumBytesUsed(hBuf, dim.width * 2 * dim.height);

    return 0;
}

Void processReconData(IVIDEO1_BufDesc* reconBufs, Buffer_Handle hSrcBuf, Buffer_Handle hDstBuf)
{
    Int16                 mbSizeY;
    Int16                 mbSizeX;
    Uint32                lumaColLength;
    Uint32                chromaColSize;
    Uint32                UYVYRowLength;
    Uint32                UYVYMbSize;
    UInt8                *lum_buf;
    UInt8                *chr_buf;
    UInt8                *curr_mb;
    UInt8                *curr_lum_mb;
    UInt8                *curr_chroma_mb;
    Int16                 i, j, k, l;
    BufferGfx_Dimensions  dim;

    /*
     * A single Master Block is 16x16 pixels.  Get our master block dimensions
     * by divding the pixel height and width by 16.
     */
    BufferGfx_getDimensions(hSrcBuf, &dim);

    mbSizeY = dim.height >> 4;
    mbSizeX = dim.width  >> 4;

    /*
     * Our luma buffer is a series of 16x16 byte blocks, and our chroma buffer
     * is a series of 16x8 byte blocks.  Each block corresponds to information
     * for one master block.  The first block in each buffer contains header
     * information.  Set lum_buf and chr_buf to the first block after the
     * header.
     */
    lum_buf = (UInt8*) (reconBufs->bufDesc[0].buf + 16 * 16);
    chr_buf = (UInt8*) (reconBufs->bufDesc[1].buf + 16 * 8);

    /*
     * The luma and chroma buffers are constructed in column-major order.
     * The blocks for a single column are followed by two padding blocks
     * before the next column starts.  Set lumaColLength and chromaColSize
     * to the number of bytes that must be skipped over to get to the next
     * column in the corresponding buffer.
     */
    lumaColLength = (16*16) * (mbSizeY + 2);
    chromaColSize = (16*8)  * (mbSizeY + 2);

    /*
     * Calculate the number of bytes that must be skipped over to go to the
     * next row in the reconstructed UYVY frame.  Also calculate how many
     * bytes in the UYVY file are needed to represent a single master block.
     */
    UYVYRowLength = 32 * mbSizeX;
    UYVYMbSize    = 32 * 16;

    /*
     * Copy the reconstructed buffer information into a UYVY frame.
     */
    for (i = 0; i < mbSizeX; i++) {
        for (j = 0; j < mbSizeY; j++) {

            /* Calculate input and output buffer offsets for the current */
            /* master block                                              */
            curr_lum_mb    = lum_buf + (lumaColLength * i) + (256 * j);
            curr_chroma_mb = chr_buf + (chromaColSize * i) + (128 * j);
            curr_mb        = (UInt8 *) Buffer_getUserPtr(hDstBuf) +
                                 (j * (UYVYMbSize * mbSizeX)) + (i * 32);

            /* Copy Luma information */
            for (k = 0; k < 16; k++) {
                for (l = 0; l < 16; l++) {
                    curr_mb[(k * UYVYRowLength) + (l * 2) + 1] =
                        curr_lum_mb[k * 16 + l];
                }
            }

            /* Copy Chroma information */
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 16; l++) {
                    curr_mb[((k * 2) * UYVYRowLength) + (l * 2)] =
                        curr_chroma_mb[k * 16 + l];
                    curr_mb[((k * 2 + 1) * UYVYRowLength) + (l * 2)] =
                        curr_chroma_mb[k * 16 + l];
                }
            }
        }
    }

    Buffer_setNumBytesUsed(hDstBuf, dim.width * dim.height * 2);
}