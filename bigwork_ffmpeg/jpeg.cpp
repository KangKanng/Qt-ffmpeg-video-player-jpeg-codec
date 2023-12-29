#include "jpeg.h"

JPEG::JPEG()
{

}


void JPEG::SaveFrame(AVPicture* picture, int width, int height,  QString mobj) {
    FILE* pFile;
    char szFilename[32];

    // Open file
    sprintf_s(szFilename, mobj.toLocal8Bit().data());
    fopen_s(&pFile, szFilename, "wb");
    if (pFile == NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for (size_t y = 0; y < height; y++)
        fwrite(picture->data[0] + y * picture->linesize[0], 1, width * 3, pFile);

    // Close file
    fclose(pFile);
}

pixel_chunk JPEG::DCT(pixel_chunk f) {
    if (f.pc.size() != 8 || f.pc[0].size() != 8) {
        printf("pixel chunk size is worng.\n");
        getchar();
    }
    pixel_chunk F = pixel_chunk();
    F.h = f.h;
    F.w = f.w;
    for (size_t u = 0; u < 8; u += 1) {
        for (size_t v = 0; v < 8; v += 1) {
            float c_u = (u == 0) ? sqrt(1.0 / 8) : sqrt(2.0 / 8);
            float c_v = (v == 0) ? sqrt(1.0 / 8) : sqrt(2.0 / 8);
            float temp_sum = 0;
            for (size_t ii = 0; ii < 8; ii += 1) {
                for (size_t jj = 0; jj < 8; jj += 1) {
                    temp_sum += f.pc[ii][jj] * cos((2 * ii + 1) * PI / 16.0 * u) * cos((2 * jj + 1) * PI / 16.0 * v);
                }
            }
            F.pc[u][v] = c_u * c_v * temp_sum;
        }
    }
    return F;
}
pixel_chunk JPEG::IDCT(pixel_chunk F) {
    if (F.pc.size() != 8 || F.pc[0].size() != 8) {
        printf("pixel chunk size is worng.\n");
        getchar();
    }
    pixel_chunk f = pixel_chunk();
    f.h = F.h;
    f.w = F.w;
    for (size_t i = 0; i < 8; i += 1) {
        for (size_t j = 0; j < 8; j += 1) {
            float temp_sum = 0;
            for (size_t u = 0; u < 8; u += 1) {
                for (size_t v = 0; v < 8; v += 1) {
                    float c_u = (u == 0) ? sqrt(1.0 / 8) : sqrt(2.0 / 8);
                    float c_v = (v == 0) ? sqrt(1.0 / 8) : sqrt(2.0 / 8);
                    temp_sum += c_u * c_v * F.pc[u][v] * cos((2 * i + 1) * PI / 16.0 * u) * cos((2 * j + 1) * PI / 16.0 * v);
                }
            }
            f.pc[i][j] = temp_sum;
        }
    }
    return f;
}
pixel_chunk JPEG::quantify(pixel_chunk f, int type) {
    if (f.pc.size() != 8 || f.pc[0].size() != 8) {
        printf("pixel chunk size is worng.\n");
        getchar();
    }
    pixel_chunk F = pixel_chunk();
    F.h = f.h;
    F.w = f.w;
    if (type == 1) {
        for (size_t i = 0; i < 8; i += 1) {
            for (size_t j = 0; j < 8; j += 1) {
                F.pc[i][j] = (int)(f.pc[i][j] / (Q_lumi[i][j] * scale) + 0.5);
            }
        }
    }
    else {
        for (size_t i = 0; i < 8; i += 1) {
            for (size_t j = 0; j < 8; j += 1) {
                F.pc[i][j] = (int)(f.pc[i][j] / (Q_chrom[i][j] * scale) + 0.5);
            }
        }
    }

    return F;
}
pixel_chunk JPEG::unquantify(pixel_chunk F, int type) {
    if (F.pc.size() != 8 || F.pc[0].size() != 8) {
        printf("pixel chunk size is worng.\n");
        getchar();
    }
    pixel_chunk f = pixel_chunk();
    f.h = F.h;
    f.w = F.w;
    if (type == 1) {
        for (size_t i = 0; i < 8; i += 1) {
            for (size_t j = 0; j < 8; j += 1) {
                f.pc[i][j] = F.pc[i][j] * Q_lumi[i][j] * scale;
            }
        }
    }
    else {
        for (size_t i = 0; i < 8; i += 1) {
            for (size_t j = 0; j < 8; j += 1) {
                f.pc[i][j] = F.pc[i][j] * Q_lumi[i][j] * scale;
            }
        }
    }

    return f;
}

jpeg_pic JPEG::Encoder_jpeg(AVFrame* pFrame, size_t width, size_t height)
/*Encoder_jpeg aceept a RGB AVframe and executs the fllowing steps:
1. change it from RGB to YUV
2. subsample YUV by 4:2:0(each pixel values at the average of 4 neiboring pixels)
3. split U, V to 8*8 pixel chunks
4. DCT on each pixel chunks
5.quantify DCT result
total time cost: 4.5 hours.
*/
{
    //1. RGB to YUV
    std::vector<std::vector<float>> image_Y(height);
    std::vector<std::vector<float>> image_U(height);
    std::vector<std::vector<float>> image_V(height);
    for (int i = 0; i < height; i += 1) {
        image_Y[i].resize(width);
        image_U[i].resize(width);
        image_V[i].resize(width);
    }

    //pFrame->data[0]:[[rgbrgbrgb...for 3*width] for heigth]
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x += 1) {
            uint8_t cR = *(pFrame->data[0] + y * pFrame->linesize[0] + 3 * x);
            uint8_t cG = *(pFrame->data[0] + y * pFrame->linesize[0] + 3 * x + 1);
            uint8_t cB = *(pFrame->data[0] + y * pFrame->linesize[0] + 3 * x + 2);
            //printf("%d\n", cR);
            image_Y[y][x] = std::min(std::max(0.257 * cR + 0.504 * cG + 0.098 * cB + 16, 16.0), 235.0);
            image_U[y][x] = std::min(std::max(-0.148 * cR + -0.291 * cG + 0.439 * cB + 128, 16.0), 239.0);
            image_V[y][x] = std::min(std::max(0.439 * cR + -0.368 * cG - 0.071 * cB + 128, 16.0), 239.0);
        }
    }
    printf("finish: RGB 2 YUV\n");


    //2. subsampling on U, V
    if (height / 2 - height / 2.0 != 0 || width / 2 - width / 2.0 != 0) {
        printf("[ERROR] h/2:%f	w/2:%f\n", height / 2.0, width / 2.0);
        getchar();
    }
    std::vector<std::vector<float>> image_U_sub(height / 2);
    std::vector<std::vector<float>> image_V_sub(height / 2);
    for (int i = 0; i < height / 2; i += 1) {
        image_U_sub[i].resize(width / 2);
        image_V_sub[i].resize(width / 2);
    }
    for (size_t h = 0; h < height / 2; h += 1) {
        for (size_t w = 0; w < width / 2; w += 1) {
            image_U_sub[h][w] = (image_U[2 * h][2 * w] + image_U[2 * h][2 * w + 1] +
                image_U[2 * h + 1][2 * w] + image_U[2 * h + 1][2 * w + 1]) / 4.0;
            image_V_sub[h][w] = (image_V[2 * h][2 * w] + image_V[2 * h][2 * w + 1] +
                image_V[2 * h + 1][2 * w] + image_V[2 * h + 1][2 * w + 1]) / 4.0;
        }
    }
    printf("finish: subsampling.\n");

    //3. split 8*8 pixel chunks
    if (height / 16 - height / 16.0 != 0 || width / 16 - width / 16.0 != 0) {
        printf("[ERROR] h/16:%f	w/16:%f:\n", height / 16.0, width / 16.0);
        getchar();
    }
    std::vector<pixel_chunk> pxl_chnk_f_U;
    std::vector<pixel_chunk> pxl_chnk_f_V;
    std::vector<pixel_chunk> pxl_chnk_f_Y;
    for (size_t h = 0; h < height / 2; h += 8) {
        for (size_t w = 0; w < width / 2; w += 8) {
            pixel_chunk temp_f_U = pixel_chunk();
            temp_f_U.h = h;
            temp_f_U.w = w;
            pixel_chunk temp_f_V = pixel_chunk();
            temp_f_V.h = h;
            temp_f_V.w = w;
            for (size_t i = 0; i < 8; i += 1) {
                for (size_t j = 0; j < 8; j += 1) {
                    temp_f_U.pc[i][j] = image_U_sub[h + i][w + j];
                    temp_f_V.pc[i][j] = image_V_sub[h + i][w + j];
                }
            }
            pxl_chnk_f_U.push_back(temp_f_U);
            pxl_chnk_f_V.push_back(temp_f_V);
        }
    }
    for (size_t h = 0; h < height; h += 8) {
        for (size_t w = 0; w < width; w += 8) {
            pixel_chunk temp_f_Y = pixel_chunk();
            temp_f_Y.h = h;
            temp_f_Y.w = w;
            for (size_t i = 0; i < 8; i += 1) {
                for (size_t j = 0; j < 8; j += 1) {
                    temp_f_Y.pc[i][j] = image_Y[h + i][w + j];
                }
            }
            pxl_chnk_f_Y.push_back(temp_f_Y);
        }
    }
    printf("finish: split to 8x8 pixel chunks.\n");


    //4. DCT on each pixel chunk
    std::vector<pixel_chunk> DCT_F_U;
    std::vector<pixel_chunk> DCT_F_V;
    std::vector<pixel_chunk> DCT_F_Y;
    for (auto fu : pxl_chnk_f_U) {
        DCT_F_U.push_back(DCT(fu));
    }
    for (auto fv : pxl_chnk_f_V) {
        DCT_F_V.push_back(DCT(fv));
    }
    for (auto fy : pxl_chnk_f_Y) {
        DCT_F_Y.push_back(DCT(fy));
    }
    printf("finish: DCT to each pixel.\n");



    //quantify DCT result
    std::vector<pixel_chunk> DCT_hat_F_U;
    std::vector<pixel_chunk> DCT_hat_F_V;
    std::vector<pixel_chunk> DCT_hat_F_Y;
    for (auto fu : DCT_F_U) {
        DCT_hat_F_U.push_back(quantify(fu, 0));
    }
    for (auto fv : DCT_F_V) {
        DCT_hat_F_V.push_back(quantify(fv, 0));
    }
    for (auto fy : DCT_F_Y) {
        DCT_hat_F_Y.push_back(quantify(fy, 1));
    }
    printf("finish: quantify DCT result.\n");

    jpeg_pic result_picture = jpeg_pic(DCT_hat_F_Y, DCT_hat_F_U, DCT_hat_F_V);
    return result_picture;
}

void JPEG::Decoder_jpeg(jpeg_pic jpeg_p, size_t width, size_t height, QString mobj) {
    /*Decoder_jpeg aceept a self-definition class jpeg_pic which stores all information about a RGB picture
    that will excute thr fllowing steps:
    1. unquantization.
    2. IDCT on each pixel chunk
    3. recover pixel chuks to 2D vectors
    3. upsample UV vector
    4. YUV to RGB
    */
    //1. unquantization
    std::vector<pixel_chunk> DCT_hat_F_U = jpeg_p.DCT_hat_F_U;
    std::vector<pixel_chunk> DCT_hat_F_V = jpeg_p.DCT_hat_F_V;
    std::vector<pixel_chunk> DCT_hat_F_Y = jpeg_p.DCT_hat_F_Y;
    std::vector<pixel_chunk> DCT_F_U;
    std::vector<pixel_chunk> DCT_F_V;
    std::vector<pixel_chunk> DCT_F_Y;
    for (auto fu : DCT_hat_F_U) {
        DCT_F_U.push_back(unquantify(fu, 0));
    }
    for (auto fv : DCT_hat_F_V) {
        DCT_F_V.push_back(unquantify(fv, 0));
    }
    for (auto fy : DCT_hat_F_Y) {
        DCT_F_Y.push_back(unquantify(fy, 1));
    }
    printf("finish: unquantization.\n");
    //2. IDCT on each chunk
    std::vector<pixel_chunk> pxl_chnk_f_U;
    std::vector<pixel_chunk> pxl_chnk_f_V;
    std::vector<pixel_chunk> pxl_chnk_f_Y;
    for (auto fu : DCT_F_U) {
        pxl_chnk_f_U.push_back(IDCT(fu));
    }
    for (auto fv : DCT_F_V) {
        pxl_chnk_f_V.push_back(IDCT(fv));
    }
    for (auto fy : DCT_F_Y) {
        pxl_chnk_f_Y.push_back(IDCT(fy));
    }
    printf("finish: IDCT to each pixel chunk.\n");
    //3. recover pixel chunks to subsampled YUV.
    std::vector<std::vector<float>> image_Y(height);
    std::vector<std::vector<float>> image_U_sub(height / 2);
    std::vector<std::vector<float>> image_V_sub(height / 2);
    for (size_t i = 0; i < height; i++) {
        image_Y[i].resize(width);
    }
    for (size_t i = 0; i < height / 2; i++) {
        image_U_sub[i].resize(width / 2);
        image_V_sub[i].resize(width / 2);
    }
    for (size_t k = 0; k < pxl_chnk_f_U.size(); k += 1) {
        pixel_chunk temp_pc_U = pxl_chnk_f_U[k];
        pixel_chunk temp_pc_V = pxl_chnk_f_V[k];
        for (size_t i = 0; i < 8; i += 1) {
            for (size_t j = 0; j < 8; j += 1) {
                image_U_sub[temp_pc_U.h + i][temp_pc_U.w + j] = temp_pc_U.pc[i][j];
                image_V_sub[temp_pc_V.h + i][temp_pc_V.w + j] = temp_pc_V.pc[i][j];
            }
        }
    }
    for (size_t k = 0; k < pxl_chnk_f_Y.size(); k += 1) {
        pixel_chunk temp_pc_Y = pxl_chnk_f_Y[k];
        for (size_t i = 0; i < 8; i += 1) {
            for (size_t j = 0; j < 8; j += 1) {
                image_Y[temp_pc_Y.h + i][temp_pc_Y.w + j] = temp_pc_Y.pc[i][j];
            }
        }
    }
    printf("finish: picel chunk to 2D vector.\n");
    //4. upsample U V vector.
    std::vector<std::vector<float>> image_U(height);
    std::vector<std::vector<float>> image_V(height);
    for (size_t i = 0; i < height; i++) {
        image_U[i].resize(width);
        image_V[i].resize(width);
    }
    for (size_t h = 0; h < height / 2; h += 1) {
        for (size_t w = 0; w < width / 2; w += 1) {
            image_U[2 * h][2 * w] = image_U_sub[h][w];
            image_U[2 * h + 1][2 * w] = image_U_sub[h][w];
            image_U[2 * h][2 * w + 1] = image_U_sub[h][w];
            image_U[2 * h + 1][2 * w + 1] = image_U_sub[h][w];

            image_V[2 * h][2 * w] = image_V_sub[h][w];
            image_V[2 * h + 1][2 * w] = image_V_sub[h][w];
            image_V[2 * h][2 * w + 1] = image_V_sub[h][w];
            image_V[2 * h + 1][2 * w + 1] = image_V_sub[h][w];
        }
    }
    printf("finish: upsample U V.\n");
    //YUV to RGB
    AVPicture* picture = new AVPicture();
    avpicture_alloc(picture, AV_PIX_FMT_RGB24, width, height);
    for (size_t h = 0; h < height; h += 1) {
        for (size_t w = 0; w < width; w += 1) {
            uint8_t cR = std::min(std::max(1.164 * (image_Y[h][w] - 16) + 1.596 * (image_V[h][w] - 128), 0.0), 255.0);
            uint8_t cB = std::min(std::max(1.164 * (image_Y[h][w] - 16) + 2.018 * (image_U[h][w] - 128), 0.0), 255.0);
            uint8_t cG = std::min(std::max(1.164 * (image_Y[h][w] - 16) + 0.813 * (image_V[h][w] - 128) - 0.391 * (image_U[h][w] - 128), 0.0), 255.0);
            *(picture->data[0] + h * picture->linesize[0] + 3 * w) = cR;
            *(picture->data[0] + h * picture->linesize[0] + 3 * w + 1) = cG;
            *(picture->data[0] + h * picture->linesize[0] + 3 * w + 2) = cB;
        }
    }
    printf("finish: YUV 2 RGB.\n");
    //save as jpeg
    SaveFrame(picture, width, height, mobj);
    printf("finish: save as ppm.\n");
}

int JPEG::JPEG_Decode(QString mpath, QString mobj, int Findex)
{
    AVFormatContext* pFormatCtx;
        int                i, videoindex;
        AVCodecContext* pCodecCtx;
        AVCodec* pCodec;
        AVFrame* pFrame, * pFrameRGB;
        uint8_t* out_buffer;
        AVPacket* packet;
        //int y_size;
        int ret, got_picture;
        struct SwsContext* img_convert_ctx;

        char filepath[] = "D:/street.mov";

        int frame_cnt;

        av_register_all();
        avformat_network_init();
        pFormatCtx = avformat_alloc_context();
        if (avformat_open_input(&pFormatCtx, mpath.toLocal8Bit().data(), NULL, NULL) != 0) {
            printf("Couldn't open input stream.\n");
            getchar();
            return -1;
        }
        if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
            printf("Couldn't find stream information.\n");
            getchar();
            return -1;
        }
        videoindex = -1;
        for (i = 0; i < pFormatCtx->nb_streams; i++) {
            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoindex = i;
                break;
            }
        }
        if (videoindex == -1) {
            printf("Didn't find a video stream.\n");
            getchar();
            return -1;
        }
        pCodecCtx = pFormatCtx->streams[videoindex]->codec;
        pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if (pCodec == NULL) {
            printf("Codec not found.\n");
            getchar();
            return -1;
        }
        if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
            printf("Could not open codec.\n");
            getchar();
            return -1;
        }
        pFrame = av_frame_alloc();
        pFrameRGB = av_frame_alloc();
        out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height));
        avpicture_fill((AVPicture*)pFrameRGB, out_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
        packet = (AVPacket*)av_malloc(sizeof(AVPacket));
        av_dump_format(pFormatCtx, 0, mpath.toLocal8Bit().data(), 0);
        img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

        frame_cnt = Findex;

        packet = (AVPacket*)av_malloc(sizeof(AVPacket));

        while (av_read_frame(pFormatCtx, packet) >= 0) {
            if (packet->stream_index == videoindex) {
                ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
                if (ret < 0) {
                    printf("Decode Error.\n");
                    getchar();
                    return -1;
                }
                if (got_picture) {
                    if (pFrame->pict_type == AV_PICTURE_TYPE_I) {
                        sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                            pFrameRGB->data, pFrameRGB->linesize);
                        printf("Decoded frame index: %d\n", frame_cnt);
                        jpeg_pic jepg = Encoder_jpeg(pFrameRGB, pCodecCtx->width, pCodecCtx->height);
                        Decoder_jpeg(jepg, pCodecCtx->width, pCodecCtx->height, mobj);
                        break;
                    }
                }
            }
        }

        sws_freeContext(img_convert_ctx);
        av_frame_free(&pFrameRGB);
        av_frame_free(&pFrame);
        avcodec_close(pCodecCtx);
        avformat_close_input(&pFormatCtx);

        return 0;
}
