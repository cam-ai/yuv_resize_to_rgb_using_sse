#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <future>
#include <immintrin.h>

using namespace std;
using namespace cv;

bool YUYVToRGB_OpenCV(unsigned char *pYUV, unsigned char *pRGB, int height, int width);
bool YUYVToRGB_OpenCV_Resize(unsigned char *pYUV, unsigned char *pRGB, int height, int width, float scale_h, float scale_w);
bool YUYVToRGB_My(unsigned char *pYUV, unsigned char *pRGB, int height, int width);
bool YUYVToRGB_My_Resize(unsigned char *pYUV, unsigned char *pRGB, int height, int width, float scale_h, float scale_w);
bool YUYVToRGB_LUT(unsigned char *pYUV, unsigned char *pRGB, int height, int width);
bool YUYVToRGB_SSE(unsigned char *pYUV, unsigned char *pRGB, int height, int width);
bool YUYVToRGB_SSE_Resize(unsigned char *pYUV, unsigned char *pRGB, int height, int width, float scale_h, float scale_w);