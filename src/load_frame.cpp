//
// Created by Hector Mejia on 9/19/23.
//

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <inttypes.h>
}

bool loadFrame(const char *filename, int *width_out, int *height_out, unsigned char **data_out) {
    AVFormatContext *format_context = avformat_alloc_context();
    if (!format_context) {
        printf("failed to allocate format context\n");
        return false;
    }
    if (avformat_open_input(&format_context, filename, nullptr, nullptr) != 0) {
        printf("failed to open input file\n");
        avformat_free_context(format_context);
        return false;
    }

    int video_stream_index = -1;
    AVCodecParameters *avCodecParameters;
    const AVCodec *avCodec;
    for (int i = 0; i < format_context->nb_streams; i++) {
        avCodecParameters = format_context->streams[i]->codecpar;
        avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
        if (!avCodec) {
            printf("failed to find decoder\n");
            avformat_close_input(&format_context);
            avformat_free_context(format_context);
            return false;
        }
        if (avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        printf("failed to find video stream\n");
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
        return false;
    }

    // set up codec context for the decoder
    // allocates space where the codec can store some internal data for decoding
    AVCodecContext *codec_context = avcodec_alloc_context3(avCodec);
    if (!codec_context) {
        printf("failed to allocate codec context\n");
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
        return false;
    }
    // place information of avformat into the codec for work
    if (avcodec_parameters_to_context(codec_context, avCodecParameters) < 0) {
        printf("failed to copy codec params to codec context\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
        return false;
    }
    if (avcodec_open2(codec_context, avCodec, nullptr) < 0) {
        printf("failed to open codec\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
        return false;
    }

    // allocate a frame to store decoded data
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        printf("failed to allocate frame\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
        return false;
    }
    // allocate a packet to store the raw data
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        printf("failed to allocate packet\n");
        av_frame_free(&frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
        return false;
    }

    while (av_read_frame(format_context, packet) >= 0) {

        // if this doesnt belong to the video stream we found,
        // then we skip it
        if (packet->stream_index != video_stream_index) {
            continue;
        }
        int res = avcodec_send_packet(codec_context, packet);
        if (res < 0) {
            printf("failed to send packet for decoding: %s\n", av_err2str(res));
            av_packet_free(&packet);
            av_frame_free(&frame);
            avcodec_free_context(&codec_context);
            avformat_close_input(&format_context);
            avformat_free_context(format_context);
            return false;
        }

        res = avcodec_receive_frame(codec_context, frame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            continue;
        } else if (res < 0) {
            printf("failed to decode packet: %s\n", av_err2str(res));
            av_packet_free(&packet);
            av_frame_free(&frame);
            avcodec_free_context(&codec_context);
            avformat_close_input(&format_context);
            avformat_free_context(format_context);
            return false;
        }

        // resets the package allocated for the frame
        av_packet_unref(packet);
    }

    // transform the frame from its YUV format to RGB
    SwsContext *swsContext = sws_getContext(
            frame->width,
            frame->height,
            codec_context->pix_fmt,
            frame->width,
            frame->height,
            AV_PIX_FMT_RGB0, // RGBA
            SWS_BILINEAR, // used when actually scaling
            nullptr,
            nullptr,
            nullptr
    );
    if (!swsContext) {
        printf("failed to create sws context\n");
        av_packet_free(&packet);
        av_frame_free(&frame);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
        return false;
    }

    // its a packed buffer, so we will have 4 bytes per pixel, all in a flat array
    // this will mirror the data contained within the frame.
    auto *data = new uint8_t[frame->width * frame->height * 4];
    int destLineSize[4] = { frame->width * 4, 0, 0, 0 };
    uint8_t *const dest[4] = { data, nullptr, nullptr, nullptr };
    sws_scale(
            swsContext,
            frame->data,
            frame->linesize,
            0,
            frame->height,
            dest,
            destLineSize
    );
    sws_freeContext(swsContext); // not needed after this.

    *width_out = frame->width;
    *height_out = frame->height;
    *data_out = data;

    // freeing all the memory allocated for the codec parameters
    avformat_close_input(&format_context);
    avformat_free_context(format_context);
    avcodec_free_context(&codec_context);
    av_packet_free(&packet);
    av_frame_free(&frame);
    return true;
}
