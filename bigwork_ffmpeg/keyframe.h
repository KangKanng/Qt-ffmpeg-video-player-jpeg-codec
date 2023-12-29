#ifndef KEYFRAME_H
#define KEYFRAME_H
#include <QString>
#include <windows.h>
#include <WinGDI.h>
#include <iostream>
#include <iosfwd>
#include <fstream>
extern "C"
{
    #include <libavcodec/avcodec.h>//处理原始音频和视频流的解码
    #include <libavutil/opt.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/common.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/samplefmt.h>
    #include <libavformat/avformat.h>//处理解析视频文件并将包含在其中的流分离出来
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/parseutils.h>
}

class KeyFrame
{
public:
    KeyFrame();

    void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, int index, int bpp, QString outfile)
    {
        char buf[5] = { 0 };   //bmp头
        BITMAPFILEHEADER bmpheader;
        BITMAPINFOHEADER bmpinfo;
        FILE *fp;
        char *filename = new char[255];
        sprintf_s(filename, 255, "%s_%d.bmp", outfile.toLocal8Bit().data(), index);
        if ((fp = fopen(filename, "wb+")) == NULL) {
            printf("open file failed!\n");
            return;
        }
        bmpheader.bfType = 0x4d42;
        bmpheader.bfReserved1 = 0;
        bmpheader.bfReserved2 = 0;
        bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp / 8;
        bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
        bmpinfo.biWidth = width;
        bmpinfo.biHeight = -height;
        bmpinfo.biPlanes = 1;
        bmpinfo.biBitCount = bpp;
        bmpinfo.biCompression = BI_RGB;
        bmpinfo.biSizeImage = (width*bpp + 31) / 32 * 4 * height;
        bmpinfo.biXPelsPerMeter = 100;
        bmpinfo.biYPelsPerMeter = 100;
        bmpinfo.biClrUsed = 0;
        bmpinfo.biClrImportant = 0;
        fwrite(&bmpheader, sizeof(bmpheader), 1, fp);
        fwrite(&bmpinfo, sizeof(bmpinfo), 1, fp);
        fwrite(pFrameRGB->data[0], width*height*bpp / 8, 1, fp);
        fclose(fp);
    }

    int keyget(QString filepath, QString outfile)
    {
        //printf("%d\n", avcodec_version());

        unsigned int i = 0, videoStream = -1;
        AVFormatContext *pFormatCtx;
        AVCodecContext *pCodecCtx;
        AVCodec *pCodec;
        AVFrame *pFrame, *pFrameRGB;
        struct SwsContext *pSwsCtx;
        char *filename = filepath.toLocal8Bit().data();
        int frameFinished;
        int PictureSize;
        AVPacket packet;
        uint8_t *buf;
        //注册解码器
        av_register_all();
        avformat_network_init();
        pFormatCtx = avformat_alloc_context();


        //AVInputFormat *pInputFormt = av_find_input_format("dshow");
        if (avformat_open_input(&pFormatCtx, filepath.toLocal8Bit().data(), NULL, NULL) != 0) {
            printf("%s\n", "failed");
            return -1;
        }
        //获取视频流信息
        if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
            printf("%s\n", "couldn`t find stream info");
            return -1;
        }

        //获取视频数据
        for (int i = 0; i<pFormatCtx->nb_streams; i++)

            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                //AVMEDIA_TYPE_VIDEO
                //AV_CODEC_ID_H264
                videoStream = i;
            }

        if (videoStream == -1) {
            printf("%s\n", "find video stream failed");
            return -1;
        }
        pCodecCtx = pFormatCtx->streams[videoStream]->codec;
        pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

        if (pCodec == NULL) {
            printf("%d\n", "avcode find decoder failed!");
            return -1;
        }
        //打开解码器
        if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
            printf("avcode open failed!\n");
            return -1;
        }
        //为每帧图像分配内存
        pFrame = av_frame_alloc();
        pFrameRGB = av_frame_alloc();

        if (pFrame == NULL || pFrameRGB == NULL) {
            printf("av frame alloc failed!\n");
            return -1;
        }
        //获得帧图大小
        PictureSize = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
        buf = (uint8_t*)av_malloc(PictureSize);
        if (buf == NULL) {
            printf("av malloc failed!\n");
            return -1;
        }
        avpicture_fill((AVPicture *)pFrameRGB, buf, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
        //设置图像转换上下文
        pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
        i = 0;
        while (av_read_frame(pFormatCtx, &packet) >= 0) {
            if (packet.stream_index == videoStream) {
                //解码
                avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
                if (frameFinished) {
                    if (pFrame->key_frame) {
                        //转换图像格式，将解压出来的YUV420P的图像转换为BRG24的图像
                        sws_scale(pSwsCtx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                        //保存为bmp图
                        SaveAsBMP(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i, 24, outfile);
                        i++;
                    }
                }
                av_free_packet(&packet);
            }
        }
        sws_freeContext(pSwsCtx);
        av_free(pFrame);
        av_free(pFrameRGB);
        avcodec_close(pCodecCtx);
        avformat_close_input(&pFormatCtx);
        printf("关键帧已保存在设置路径！\n");
        return 0;
    }
};



#endif // KEYFRAME_H
