# h264_encoder
dm368 h264 encoder sample app

Usage: video_encode [options]

        Options:
             --benchmark      Print benchmarking information
        -b | --bitrate        Bitrate used to process video stream [variable]
        -c | --codec          Name of codec to use
        -e | --engine         Codec engine containing specified codec
        -h | --help           Print usage information (this message)
        -i | --input_file     Name of input file containing raw YUV
           | --cache          Cache codecs input/output buffers and perform
                              cache maintenance. Useful for local codecs
        -n | --numframes      Number of frames to process [Default: 100]
        -o | --output_file    Name of output file for encoded video
             --recon_file     Name of output file for reconstructed frames
        -r | --resolution     Video resolution ('width'x'height')
             --semiplanar     Read output in 420SP format (for DM6467 and DM365)
        At a minimum the codec name, the resolution and the file names
        *must* be given
