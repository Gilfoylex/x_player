#include "ffmpeg_control.h"
#include <QThread>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
}

void Worker::SetFileName(const QString &str_file_path)
{
    m_str_file_path = str_file_path;
}

QString Worker::GetFileName() const
{
    return m_str_file_path;
}

void Worker::doWork(void *p_param)
{
    //auto p_file_path = (static_cast<QString*>(p_param))->toLocal8Bit().constData();
    auto p_file_path = "D:\\test.mkv";//m_str_file_path.toLocal8Bit().constData();
    AVFormatContext* p_format_ctx = nullptr;
    AVCodecContext* p_codec_ctx = nullptr;
    AVCodec* p_codec = nullptr;
    AVFrame* p_frame = nullptr;
    AVFrame* p_frame_rgb = nullptr;
    AVPacket packet;
    uint8_t *p_out_buffer = nullptr;

    struct SwsContext *p_img_convert_ctx = nullptr;
    int n_video_index, n_num_bytes, n_ret;
    n_video_index = n_num_bytes = n_ret = 0;

    //av_register_all();
    //p_format_ctx = avformat_alloc_context();

    if (avformat_open_input(&p_format_ctx, p_file_path, nullptr, nullptr) != 0) {
        return;
    }

    if (avformat_find_stream_info(p_format_ctx, nullptr) < 0) {
        return;
    }

    for (unsigned int i = 0; i < p_format_ctx->nb_streams; ++i) {
        if (p_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            n_video_index = static_cast<int>(i);
            break;
        }
    }

    if (n_video_index == -1) {
        return;
    }

    AVStream *p_video_stream = p_format_ctx->streams[n_video_index];
    //AVRational videoAvgFrameRate = p_video_stream->avg_frame_rate;
    p_codec = avcodec_find_decoder(p_video_stream->codecpar->codec_id);
    if (p_codec == nullptr) {
        return;
    }

    p_codec_ctx = avcodec_alloc_context3(p_codec);
    //AVDictionary *p_options_dict = nullptr;
    avcodec_parameters_to_context(p_codec_ctx, p_video_stream->codecpar);

    if (p_codec_ctx->pix_fmt < 0 || p_codec_ctx->pix_fmt >= AV_PIX_FMT_NB) {
        avcodec_free_context(&p_codec_ctx);
        avformat_close_input(&p_format_ctx);
        return;
    }

    if (avcodec_open2(p_codec_ctx, p_codec, nullptr) < 0) {
        return;
    }

    p_frame = av_frame_alloc();
    p_frame_rgb = av_frame_alloc();

    n_num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, p_codec_ctx->width, p_codec_ctx->height, 1);
    p_out_buffer =  reinterpret_cast<uint8_t*>(av_malloc(static_cast<size_t>(n_num_bytes)));

    av_image_fill_arrays(p_frame_rgb->data,           // dst data[]
                         p_frame_rgb->linesize,       // dst linesize[]
                         p_out_buffer,                    // src buffer
                         AV_PIX_FMT_RGB32,        // pixel format
                         p_codec_ctx->width,        // width
                         p_codec_ctx->height,       // height
                         1                          // align
                         );

    p_img_convert_ctx  = sws_getContext(p_codec_ctx->width, p_codec_ctx->height,
                                        p_codec_ctx->pix_fmt, p_codec_ctx->width, p_codec_ctx->height,
                                        AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr);

    av_dump_format(p_format_ctx, 0, p_file_path, 0);

    // Get duration and interval
    int64_t duration = av_rescale_q(p_format_ctx->duration, AV_TIME_BASE_Q, p_video_stream->time_base);
    double interval = duration / (double)1000;
    double timebaseDouble = av_q2d(p_video_stream->time_base);
    int n_video_fps = av_q2d(p_video_stream->r_frame_rate);

    while (av_read_frame(p_format_ctx, &packet) == 0) {
        if (packet.stream_index == n_video_index) {
            if (avcodec_send_packet(p_codec_ctx, &packet) < 0) {
                break;
            }

            n_ret = avcodec_receive_frame(p_codec_ctx, p_frame);

            if (n_ret < 0) {
                if (n_ret == AVERROR(EAGAIN)) {
                    continue;
                }
                else {
                    break;
                }
            }

            n_ret = sws_scale(p_img_convert_ctx,
                              reinterpret_cast<uint8_t const* const*>(p_frame->data),
                              p_frame->linesize, 0, p_codec_ctx->height, p_frame_rgb->data, p_frame_rgb->linesize);

            if (n_ret < 0) {
                break;
            }

            QImage image_temp(reinterpret_cast<uchar*>(p_out_buffer), p_codec_ctx->width, p_codec_ctx->height, QImage::Format_RGB32);
            emit resultReady(image_temp);
        }

        av_packet_unref(&packet);

        QThread::usleep(60);
    }

    av_free(p_out_buffer);
    av_frame_free(&p_frame);
    av_frame_free(&p_frame_rgb);
    avcodec_close(p_codec_ctx);
    avformat_close_input(&p_format_ctx);
}
