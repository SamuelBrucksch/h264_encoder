#ifndef yuv_h_
#define yuv_h_

#include <stdio.h>
#include <string.h>

#include <xdc/std.h>

#include <ti/sdo/dmai/BufTab.h>
#include <ti/sdo/dmai/BufferGfx.h>

Int readFrame420P(Buffer_Handle hBuf, FILE *outFile, Int imageHeight);

Int readFrame420SP(Buffer_Handle hBuf, FILE *outFile, Int imageHeight);

#endif