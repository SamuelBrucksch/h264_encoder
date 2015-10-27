#ifndef yuv_h_
#define yuv_h_

#include <stdio.h>
#include <string.h>

#include <xdc/std.h>

#include <ti/sdo/dmai/BufTab.h>
#include <ti/sdo/dmai/BufferGfx.h>

Int readFrame420P(Buffer_Handle hBuf, FILE *outFile, Int imageHeight);

Int readFrame420SP(Buffer_Handle hBuf, FILE *outFile, Int imageHeight);

Int readFrameUYVY(Buffer_Handle hBuf, FILE *outFile);

Void processReconData(IVIDEO1_BufDesc* reconBufs, Buffer_Handle hSrcBuf, Buffer_Handle hDstBuf);

#endif