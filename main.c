#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

#include <xdc/std.h>

#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/CERuntime.h>

#include <ti/sdo/dmai/Dmai.h>
#include <ti/sdo/dmai/Ccv.h>
#include <ti/sdo/dmai/Cpu.h>
#include <ti/sdo/dmai/Time.h>
#include <ti/sdo/dmai/BufTab.h>
#include <ti/sdo/dmai/ColorSpace.h>
#include <ti/sdo/dmai/Capture.h>
#include <ti/sdo/dmai/Framecopy.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/ce/Venc1.h>

#include "yuv.h"

#define DEFAULT_ENGINE_NAME "encode"
#define MAX_ENGINE_NAME_SIZE 30

#define MAX_CODEC_NAME_SIZE 30

#define MAX_FILE_NAME_SIZE 100

/* Align buffers to this cache line size (in bytes)*/
#define BUFSIZEALIGN            128

/* The input buffer height restriction */
#define CODECHEIGHTALIGN       16

/* vbuf size that has been selected based on size/performance tradeoff */
#define VBUFSIZE 20480

static Char vbufferIn[VBUFSIZE];
static Char vbufferOut[VBUFSIZE];

typedef enum
{
   ArgID_BENCHMARK = 256,
   ArgID_BITRATE,
   ArgID_CODEC,
   ArgID_ENGINE,
   ArgID_HELP,
   ArgID_INPUT_FILE,
   ArgID_CACHE,
   ArgID_NUMFRAMES,
   ArgID_OUTPUT_FILE,
   ArgID_RECON_FILE,
   ArgID_RESOLUTION,
   ArgID_SP,
   ArgID_NUMARGS
} ArgID;

/* Arguments for app */
typedef struct Args {
    Int   numFrames;
    Int   width;
    Int   height;
    Int   bitRate;
    Int   benchmark;
    Bool  cache;
    Bool  writeReconFrames;
    Bool  sp;
    Char  codecName[MAX_CODEC_NAME_SIZE];
    Char  inFile[MAX_FILE_NAME_SIZE];
    Char  outFile[MAX_FILE_NAME_SIZE];
    Char  reconFile[MAX_FILE_NAME_SIZE];
    Char  engineName[MAX_ENGINE_NAME_SIZE];
} Args;

#define DEFAULT_ARGS { 100, 0, 0, -1, FALSE, FALSE, FALSE, FALSE }

static Void usage(void)
{
    fprintf(stderr, "Usage: video_encode_io1_<platform> [options]\n\n"
        "Options:\n"
        "-b | --bitrate        Bitrate used to process video stream [variable]\n"
        "-c | --codec          Name of codec to use\n"
        "-e | --engine         Codec engine containing specified codec\n"
        "-h | --help           Print usage information (this message)\n"
        "-i | --input_file     Name of input file containing raw YUV\n"
        "-n | --numframes      Number of frames to process [Default: 100]\n"
        "-o | --output_file    Name of output file for encoded video\n"
        "-r | --resolution     Video resolution ('width'x'height')\n"
        "\n"
        "At a minimum the codec name, the resolution and the file names "
        "*must* be given\n\n");
}

static Void parseArgs(Int argc, Char *argv[], Args *argsp)
{
    const char shortOptions[] = "b:c:e:hi:n:o:r:";

    const struct option longOptions[] = {
        {"benchmark",       no_argument,       NULL, ArgID_BENCHMARK   },
        {"bitrate",         required_argument, NULL, ArgID_BITRATE     },
        {"codec",           required_argument, NULL, ArgID_CODEC       },
        {"engine",          required_argument, NULL, ArgID_ENGINE      },
        {"help",            no_argument,       NULL, ArgID_HELP        },
        {"input_file",      required_argument, NULL, ArgID_INPUT_FILE  },
        {"cache",           no_argument,       NULL, ArgID_CACHE       },
        {"numframes",       required_argument, NULL, ArgID_NUMFRAMES   },
        {"output_file",     required_argument, NULL, ArgID_OUTPUT_FILE },
        {"recon_file",      required_argument, NULL, ArgID_RECON_FILE  },
        {"resolution",      required_argument, NULL, ArgID_RESOLUTION  },
        {"semiplanar",      no_argument,       NULL, ArgID_SP          },
        {0, 0, 0, 0}
    };

    Int  codec   = FALSE;
    Int  inFile  = FALSE;
    Int  outFile = FALSE;
    Int  index;
    Int  argID;

    strncpy(argsp->engineName, DEFAULT_ENGINE_NAME, MAX_ENGINE_NAME_SIZE);

    for (;;) {
        argID = getopt_long(argc, argv, shortOptions, longOptions, &index);

        if (argID == -1) {
            break;
        }

        switch (argID) {

            case ArgID_BENCHMARK:
                argsp->benchmark = TRUE;
                break;

            case ArgID_BITRATE:
            case 'b':
                argsp->bitRate = atoi(optarg);
                break;

            case ArgID_CODEC:
            case 'c':
                strncpy(argsp->codecName, optarg, MAX_CODEC_NAME_SIZE);
                codec = TRUE;
                break;

            case ArgID_ENGINE:
            case 'e':
                strncpy(argsp->engineName, optarg, MAX_ENGINE_NAME_SIZE);
                break;

            case ArgID_HELP:
            case 'h':
                usage();
                exit(EXIT_SUCCESS);

            case ArgID_INPUT_FILE:
            case 'i':
                strncpy(argsp->inFile, optarg, MAX_FILE_NAME_SIZE);
                inFile = TRUE;
                break;

            case ArgID_CACHE:
                argsp->cache = TRUE;
                break;
                
            case ArgID_NUMFRAMES:
            case 'n':
                argsp->numFrames = atoi(optarg);
                break;

            case ArgID_OUTPUT_FILE:
            case 'o':
                strncpy(argsp->outFile, optarg, MAX_FILE_NAME_SIZE);
                outFile = TRUE;
                break;

            case ArgID_RECON_FILE:
                strncpy(argsp->reconFile, optarg, MAX_FILE_NAME_SIZE);
                argsp->writeReconFrames = TRUE;
                break;

            case ArgID_SP:
                argsp->sp = TRUE;
                break;

            case ArgID_RESOLUTION:
            case 'r':
                if (sscanf(optarg, "%dx%d", &argsp->width,
                                            &argsp->height) != 2) {
                    fprintf(stderr, "Invalid resolution supplied (%s)\n",
                            optarg);
                    usage();
                    exit(EXIT_FAILURE);
                }

                if (argsp->width % 8 || argsp->height % 8) {
                    fprintf(stderr, "Width and height must be multiple of 8\n");                }
                break;

            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        usage();
        exit(EXIT_FAILURE);
    }

    if (!codec || !inFile || !outFile || !argsp->width || !argsp->height) {
        usage();
        exit(EXIT_FAILURE);
    }
}

Int main(Int argc, Char *argv[])
{
    Args args = DEFAULT_ARGS;

	VIDENC1_Params params = Venc1_Params_DEFAULT;
    VIDENC1_DynamicParams dynParams = Venc1_DynamicParams_DEFAULT;
    BufferGfx_Attrs gfxAttrs = BufferGfx_Attrs_DEFAULT;
    Buffer_Attrs bAttrs = Buffer_Attrs_DEFAULT;
    Venc1_Handle hVe1 = NULL;
    FILE *outFile   = NULL;
    FILE *inFile    = NULL;
    Engine_Handle hEngine   = NULL;
    Bool flushed   = FALSE;
    Bool mustExit  = FALSE;
    BufTab_Handle hBufTab   = NULL;
    Buffer_Handle hOutBuf   = NULL;
    Buffer_Handle hFreeBuf  = NULL;                           
    Buffer_Handle hInBuf    = NULL;
    Int numFrame  = 0;
    Int flushCntr = 1;
    Int bufIdx;
    Int inBufSize, outBufSize;
    Int numBufs;
    ColorSpace_Type colorSpace;
    Int ret = Dmai_EOK;

    /* Parse the arguments given to the app */
    parseArgs(argc, argv, &args);    

    printf("Starting application...\n");

	/* Initialize the codec engine run time */
    CERuntime_init();
    
    /* Initialize DMAI */
    Dmai_init();

    /* Open the input file with raw yuv data */
    inFile = fopen(args.inFile, "rb");

    if (inFile == NULL) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to open input file %s\n", args.inFile);
        goto cleanup;
    }

    /* Using a larger vbuf to enhance performance of file i/o */
    if (setvbuf(inFile, vbufferIn, _IOFBF, sizeof(vbufferIn)) != 0) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to setvbuf on input file descriptor\n");
        goto cleanup;   
    }

	/* Open the output file where to put encoded data */
    outFile = fopen(args.outFile, "wb");

    if (outFile == NULL) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to open output file %s\n", args.outFile);
        goto cleanup;
    }

    /* Using a larger vbuf to enhance performance of file i/o */
    if (setvbuf(outFile, vbufferOut, _IOFBF, sizeof(vbufferOut)) != 0) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to setvbuf on output file descriptor\n");
        goto cleanup;   
    }

	/* Open the codec engine */
    hEngine = Engine_open(args.engineName, NULL, NULL);

	/* Set up codec parameters depending on bit rate */
    if (args.bitRate < 0) {
        /* Variable bit rate */
        params.rateControlPreset = IVIDEO_NONE;

        /* 
         * If variable bit rate use a bogus bit rate value (> 0)
         * since it will be ignored.
         */
        params.maxBitRate = 2000000;
    }
    else {
        /* Constant bit rate */
        params.rateControlPreset = IVIDEO_LOW_DELAY;
        params.maxBitRate = args.bitRate;
    }

    params.inputChromaFormat = XDM_YUV_420SP;
	params.reconChromaFormat = XDM_YUV_420SP;
	params.maxWidth = args.width;
    params.maxHeight = args.height;
    params.maxInterFrameInterval = 1;
    dynParams.targetBitRate = params.maxBitRate;
    dynParams.inputWidth = params.maxWidth;
    dynParams.inputHeight = params.maxHeight;

    /* Create the video encoder */
    hVe1 = Venc1_create(hEngine, args.codecName, &params, &dynParams);

    if (hVe1 == NULL) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to create video encoder: %s\n", args.codecName);
        goto cleanup;
    }

    /* Ask the codec how much input data it needs */
    inBufSize = Venc1_getInBufSize(hVe1);
    if (inBufSize < 0) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to calculate buffer attributes\n");
        goto cleanup;
    }

    /* Ask the codec how much space it needs for output data */
    outBufSize = Venc1_getOutBufSize(hVe1);

    colorSpace = ColorSpace_YUV420PSEMI;

    /* Align buffers to cache line boundary */    
    gfxAttrs.bAttrs.memParams.align = bAttrs.memParams.align = BUFSIZEALIGN;    
    gfxAttrs.dim.width = args.width;
    gfxAttrs.dim.height = args.height;
	gfxAttrs.dim.height = Dmai_roundUp(gfxAttrs.dim.height, CODECHEIGHTALIGN);
    gfxAttrs.dim.lineLength = BufferGfx_calcLineLength(args.width, colorSpace);
    gfxAttrs.colorSpace = colorSpace;
    
	/* Number of input buffers required */
    if(params.maxInterFrameInterval > 1) {
	    /* B frame support */
        numBufs = params.maxInterFrameInterval;
    }
    else {
        numBufs = 1;
    }

    /* Create a table of input buffers of the size requested by the codec */
    hBufTab = BufTab_create(numBufs, Dmai_roundUp(inBufSize, BUFSIZEALIGN), BufferGfx_getBufferAttrs(&gfxAttrs));  
    if (hBufTab == NULL) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to allocate contiguous buffers\n");
        goto cleanup;
    }

    /* Set input buffer table */
    Venc1_setBufTab(hVe1, hBufTab);

    /* Create the output buffer for encoded video data */
    hOutBuf = Buffer_create(Dmai_roundUp(outBufSize, BUFSIZEALIGN), &bAttrs);
    if (hOutBuf == NULL) {
        ret = Dmai_EFAIL;
        fprintf(stderr,"Failed to create contiguous buffer\n");
        goto cleanup;
    }

    while (1) {

    	/* Get a buffer for input */
        hInBuf = BufTab_getFreeBuf(hBufTab);
        if (hInBuf == NULL) {
            ret = Dmai_EFAIL;
            fprintf(stderr,"Failed to get a free contiguous buffer from BufTab\n");
            BufTab_print(hBufTab);
            goto cleanup;
        }

		/* Read a yuv input frame */
        printf("\n Frame %d: ", numFrame);
        if (readFrame420SP(hInBuf, inFile, args.height) < 0) {
			ret = Dmai_EFAIL;
			goto cleanup;
		}

        if (++numFrame == args.numFrames || mustExit == TRUE){
            if(!(params.maxInterFrameInterval>1)) {
                /* No B-frame support */
                printf("... exiting \n");
                goto cleanup;
            }

            /* 
             * When encoding a stream with B-frames, ending the processing
             * requires to free the buffer held by the encoder. This is done by
             * flushing the encoder and performing a last process() call
             * with a dummy input buffer.
             */             
            printf("\n... exiting with flush (B-frame stream) \n"); 
            flushCntr = params.maxInterFrameInterval-1;
            flushed = TRUE;
            Venc1_flush(hVe1);
        }

        for(bufIdx = 0; bufIdx < flushCntr; bufIdx++) { 

        	/* Make sure the whole buffer is used for input */
            BufferGfx_resetDimensions(hInBuf);

            /* Encode the video buffer */
            if (Venc1_process(hVe1, hInBuf, hOutBuf) < 0) {
                ret = Dmai_EFAIL;
                fprintf(stderr,"Failed to encode video buffer\n");
                goto cleanup;
            }

            /* if encoder generated output content, free released buffer */
            if (Buffer_getNumBytesUsed(hOutBuf)>0) {
                /* Get free buffer */
                hFreeBuf = Venc1_getFreeBuf(hVe1);
                /* Free buffer */
                BufTab_freeBuf(hFreeBuf);
            }
            /* if encoder did not generate output content */
            else {
                /* if non B frame sequence */
                /* encoder skipped frame probably exceeding target bitrate */
                if (params.maxInterFrameInterval<=1) {
                    /* free buffer */
                    printf(" Encoder generated 0 size frame\n");
                    BufTab_freeBuf(hInBuf);
                }
            }

            /* Write the encoded frame to the file system */
            if (Buffer_getNumBytesUsed(hOutBuf)) {
                if (fwrite(Buffer_getUserPtr(hOutBuf),
                  Buffer_getNumBytesUsed(hOutBuf), 1, outFile) != 1) {
                    ret = Dmai_EFAIL;
                    fprintf(stderr,"Failed to write encoded video data to file\n");
                    goto cleanup;
                }
            }         
        }

		/* If the codec flushing completed, exit main thread */
        if (flushed) {
            /* Free dummy input buffer used for flushing process() calls */               
            printf("freeing dummy input buffer ... \n");
            BufTab_freeBuf(hInBuf);
            break;
        }
    }

cleanup:
    /* Clean up the application */
    if (hOutBuf) {
        Buffer_delete(hOutBuf);
    }

    if (hVe1) {
        Venc1_delete(hVe1);
    }

    if (hBufTab) {
        BufTab_delete(hBufTab);
    }

    if (hEngine) {
        Engine_close(hEngine);
    }

    if (inFile) {
        fclose(inFile);
    }
   
    if (outFile) {
        fclose(outFile);
    }

    printf("End of application.\n");

    if (ret == Dmai_EFAIL)
        return 1;
    else    
        return 0;
}
