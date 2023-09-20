#include "video_reader.h"

//
// Created by Hector Mejia on 9/20/23.
//
bool video_reader_open_file(VideoReaderState *state, const char *filename) {
    // Open the file using libavformat
    state->av_format_ctx = avformat_alloc_context();
    if (!state->av_format_ctx) {
        printf("Couldn't created AVFormatContext\n");
        return false;
    }

    if (avformat_open_input(&state->av_format_ctx, filename, NULL, NULL) != 0) {
        printf("Couldn't open video file\n");
        return false;
    }

    // Find the first valid video stream inside the file
    state->video_stream_index = -1;
    AVCodecParameters *av_codec_params;
    AVCodec *av_codec;
    for (int i = 0; i < state->av_format_ctx->nb_streams; ++i) {
        av_codec_params = state->av_format_ctx->streams[i]->codecpar;
        av_codec = const_cast<AVCodec *>(avcodec_find_decoder(av_codec_params->codec_id));
        if (!av_codec) {
            continue;
        }
        if (av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
            state->video_stream_index = i;
            state->width = av_codec_params->width;
            state->height = av_codec_params->height;
            break;
        }
    }
    if (state->video_stream_index == -1) {
        printf("Couldn't find valid video stream inside file\n");
        return false;
    }

    // Set up a codec context for the decoder
    state->av_codec_ctx = avcodec_alloc_context3(av_codec);
    if (!state->av_codec_ctx) {
        printf("Couldn't create AVCodecContext\n");
        return false;
    }
    if (avcodec_parameters_to_context(state->av_codec_ctx, av_codec_params) < 0) {
        printf("Couldn't initialize AVCodecContext\n");
        return false;
    }
    if (avcodec_open2(state->av_codec_ctx, av_codec, NULL) < 0) {
        printf("Couldn't open codec\n");
        return false;
    }

    state->av_frame = av_frame_alloc();
    if (!state->av_frame) {
        printf("Couldn't allocate AVFrame\n");
        return false;
    }
    state->av_packet = av_packet_alloc();
    if (!state->av_packet) {
        printf("Couldn't allocate AVPacket\n");
        return false;
    }
    return true;
}

bool video_reader_read_frame(VideoReaderState *state, uint8_t *frame_buffer) {
    // Decode one frame
    int response;
    while (av_read_frame(state->av_format_ctx, state->av_packet) >= 0) {
        if (state->av_packet->stream_index != state->video_stream_index) {
            av_packet_unref(state->av_packet);
            continue;
        }

        response = avcodec_send_packet(state->av_codec_ctx, state->av_packet);
        if (response < 0) {
            printf("Failed to decode packet: %s\n", av_err2str(response));
            return false;
        }

        response = avcodec_receive_frame(state->av_codec_ctx, state->av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            av_packet_unref(state->av_packet);
            continue;
        } else if (response < 0) {
            printf("Failed to decode packet: %s\n", av_err2str(response));
            return false;
        }

        av_packet_unref(state->av_packet);
        break;
    }


    // Set up sws scaler
    if (!state->sws_scaler_ctx) {
//        auto source_pix_fmt = correct_for_deprecated_pixel_format(av_codec_ctx->pix_fmt);
        state->sws_scaler_ctx = sws_getContext(state->width, state->height, state->av_codec_ctx->pix_fmt,
                                               state->width, state->height, AV_PIX_FMT_RGB0,
                                               SWS_BILINEAR, NULL, NULL, NULL);
    }
    if (!state->sws_scaler_ctx) {
        printf("Couldn't initialize sw scaler\n");
        return false;
    }

    printf("scalar ctx nv_slice_ctx: %p\n", state->sws_scaler_ctx);

    // its a packed buffer, so we will have 4 bytes per pixel, all in a flat array
    // this will mirror the data contained within the av_frame.
    uint8_t *dest[4] = {frame_buffer, NULL, NULL, NULL};
    int dest_linesize[4] = {state->width * 4, 0, 0, 0};
    int out = sws_scale(
            state->sws_scaler_ctx,
            state->av_frame->data,
            state->av_frame->linesize,
            0,
            state->av_frame->height,
            dest,
            dest_linesize
    );
    printf("sws_scale returned %d\n", out);


    return true;
}

void video_reader_close(VideoReaderState *state) {
    // freeing all the memory allocated for the codec parameters
    sws_freeContext(state->sws_scaler_ctx); // not needed after this.
    avformat_close_input(&state->av_format_ctx);
    avformat_free_context(state->av_format_ctx);
    avcodec_free_context(&state->av_codec_ctx);
    av_packet_free(&state->av_packet);
    av_frame_free(&state->av_frame);
}

