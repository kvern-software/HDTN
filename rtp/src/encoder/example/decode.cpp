#include <iostream>
#include <stdio.h>

extern "C"
{
#include <libavcodec/defs.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define INBUF_SIZE 20000
// c->width = 352;
// c->height = 288;
//Save RGB image as PPM file format
static void ppm_save(char* filename, AVFrame* frame)
{
    FILE* file;
    int i;

    fopen((const char *) file, filename);
    fprintf(file, "P6\n%d %d\n%d\n", frame->width, frame->height, 255);
    for (i = 0; i < frame->height; i++)
        fwrite(frame->data[0] + i * frame->linesize[0], 1, frame->width * 3, file);
    fclose(file);
}

void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const char* outfilePrefix)
{
    std::cout << "decode" << std::endl;

    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        printf("Error sending a packet for decoding\n");
        exit(1);
    }

    std::cout << "decode1" << std::endl;

    int sts;
    ////////////////////////////////////////////////////////////////////////////
    //Create SWS Context for converting from decode pixel format (like YUV420) to RGB
    struct SwsContext* sws_ctx = NULL;
    sws_ctx = sws_getContext(dec_ctx->width,
        dec_ctx->height,
        dec_ctx->pix_fmt,
        dec_ctx->width,
        dec_ctx->height,
        AV_PIX_FMT_RGB24,
        SWS_BICUBIC,
        NULL,
        NULL,
        NULL);
    std::cout << "ctx" << std::endl;

    if (sws_ctx == nullptr)
    {
        return;  //Error!
    }

    //Allocate frame for storing image converted to RGB.
    AVFrame* pRGBFrame = av_frame_alloc();

    pRGBFrame->format = AV_PIX_FMT_RGB24;
    pRGBFrame->width = dec_ctx->width;
    pRGBFrame->height = dec_ctx->height;
    sts = av_frame_get_buffer(pRGBFrame, 0);
    std::cout << "buff" << std::endl;

    if (sts < 0)
    {
        // goto free;
        //return;  //Error!
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        std::cout << ret << std::endl;
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            // goto free;
        //return;
        if (ret < 0) {
            printf("Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3d\n", dec_ctx->frame_number);//
        fflush(stdout);

        //////////////////////////////////////////////////////////////////////////
        //Convert from input format (e.g YUV420) to RGB and save to PPM:
        sts = sws_scale(sws_ctx,    //struct SwsContext* c,
            frame->data,            //const uint8_t* const srcSlice[],
            frame->linesize,        //const int srcStride[],
            0,                      //int srcSliceY, 
            frame->height,          //int srcSliceH,
            pRGBFrame->data,        //uint8_t* const dst[], 
            pRGBFrame->linesize);   //const int dstStride[]);

        snprintf(buf, sizeof(buf), "%s-%d.ppm", outfilePrefix, dec_ctx->frame_number);
        ppm_save(buf, pRGBFrame);
    }


    sws_freeContext(sws_ctx);
    av_frame_free(&pRGBFrame);
}

int main(int argc, char ** argv)
{
    const char* filename, * outfilePrefix, * seqfilename;
    const AVCodec* codec;
    AVCodecParserContext* parser;
    AVCodecContext* codecContext = NULL;
    FILE* file;
    AVFrame* frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data;
    size_t   data_size;
    int ret;
    AVPacket* pkt;


    if (argc <= 2) {
        printf("Usage: %s <input file> <output file>\n"
            "And check your input file is encoded by mpeg1video please.\n", argv[0]);
        exit(0);
    }
    filename = argv[1];
    outfilePrefix = argv[2];


    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    /* find the HEVC video decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!codec) {
        printf("Codec not found\n");
        exit(1);
    }

    std::cout << "codec" << std::endl;

    parser = av_parser_init(codec->id);
    if (!parser) {
        printf("parser not found\n");
        exit(1);
    }
    std::cout << "codec" << std::endl;

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        printf("Could not allocate video codec context\n");
        exit(1);
    }
    std::cout << "codec" << std::endl;

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

       /* open it */
    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        printf( " Could notopen codec\n");
        exit(1);
    }
    std::cout << "codec" << std::endl;

    file = fopen(filename, "rb");
    if (!file) {
        printf( "Could not open %s\n", filename);
        exit(1);
    }
    std::cout << "codec1" << std::endl;

    frame = av_frame_alloc();
    if (!frame) {
        printf( "Could not allocate video frame\n");
        exit(1);
    }
    std::cout << "codec2" << std::endl;

    while (!feof(file)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, file);
        if (!data_size)
            break;
        std::cout << "read" << std::endl;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0)
        {
            ret = av_parser_parse2(parser, codecContext, &pkt->data, &pkt->size,
                data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                printf( "Error while parsing\n");
                exit(1);
            }
            data += ret;
            data_size -= ret;
         std::cout << "data" << std::endl;

            if (pkt->size)
                decode(codecContext, frame, pkt, outfilePrefix);
        }
    }

    /* flush the decoder */
    decode(codecContext, frame, NULL, outfilePrefix);

    fclose(file);

    av_parser_close(parser);
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}