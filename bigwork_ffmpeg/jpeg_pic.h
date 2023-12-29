#ifndef JPEG_PIC_H
#define JPEG_PIC_H

#include <pixel_chunk.h>

class jpeg_pic {
public:
    std::vector<pixel_chunk> DCT_hat_F_Y;
    std::vector<pixel_chunk> DCT_hat_F_U;
    std::vector<pixel_chunk> DCT_hat_F_V;
    jpeg_pic(std::vector<pixel_chunk> y, std::vector<pixel_chunk> u, std::vector<pixel_chunk>v) :
        DCT_hat_F_Y(y), DCT_hat_F_U(u), DCT_hat_F_V(v) {};
    ~jpeg_pic() { DCT_hat_F_Y.clear(); DCT_hat_F_U.clear(); DCT_hat_F_V.clear(); }
};

#endif // JPEG_PIC_H
