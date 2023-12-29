#ifndef PIXEL_CHUNK_H
#define PIXEL_CHUNK_H

#include <QString>
#include <QDebug>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#define PI 3.1415
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
}

class  pixel_chunk
{
public:
    std::vector<std::vector<float>> pc;
    size_t h = 0;
    size_t w = 0;
    //init as 0
    pixel_chunk() {
        pc.resize(8);
        for (size_t i = 0; i < 8; i += 1) {
            pc[i].resize(8);
        }
    }
    ~pixel_chunk() { pc.clear(); }
};

#endif // PIXEL_CHUNK_H
