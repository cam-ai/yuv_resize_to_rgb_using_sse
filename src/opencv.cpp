#include "convert.h"
bool YUYVToRGB_OpenCV(unsigned char *pYUV, unsigned char *pRGB, int height, int width)
{
  if (height < 1 || width < 1 || pYUV == NULL || pRGB == NULL)
    return false;

  Mat src(height, width, CV_8UC2, pYUV); // why CV_8UC2?
  Mat dst(height, width, CV_8UC3, pRGB);

  cvtColor(src, dst, COLOR_YUV2BGR_YUYV); // why YUV2BGR?
  // imwrite("opencv.png", dst);

  return true;
}

bool YUYVToRGB_OpenCV_Resize(unsigned char *pYUV, unsigned char *pRGB, int height, int width, float scale_h, float scale_w)
{
  if (height < 1 || width < 1 || pYUV == NULL || pRGB == NULL)
    return false;

  Mat src(height, width, CV_8UC2, pYUV);
  Mat image_mini;
  Mat dst(height, width, CV_8UC3, pRGB);

  //先resize，再转换色彩空间,会发生颜色错位
  //先转换色彩空间，再resize
  cvtColor(src, dst, COLOR_YUV2BGR_YUYV);
  resize(dst, image_mini, cv::Size(), scale_h, scale_w);
  // imwrite("opencv_resize.png", image_mini);
  return true;
}
