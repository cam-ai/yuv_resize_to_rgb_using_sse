#include "convert.h"

int main()
{

  char const *yuvFileName = "/home/myn1szh/cjx/opencv/test.yuyv";
  FILE *yuvFile = NULL;
  char *mode = NULL;
  unsigned char *yuv422Buf = NULL;
  unsigned char *rgbBuf = NULL;
  int frameHeight = 2160;   // 2160;
  int frameWidth = 3840;    // 3840;
  float scale_h = 0.414814; //高度缩放因子
  float scale_w = 0.233333; //宽度缩放因子
  bool flag = false;

  yuvFile = fopen(yuvFileName, "rb");
  if (yuvFile == NULL)
  {
    printf("cannot find yuv file\n");
    exit(1);
  }
  else
  {
    cout << "The input yuv file is " << yuvFileName << endl;
  }

  yuv422Buf = (unsigned char *)malloc(frameWidth * frameHeight * 2 * sizeof(unsigned char));
  // rgbBuf = (unsigned char *)malloc(int(frameWidth * scale_w) * int(frameHeight * scale_h) * 3 * sizeof(unsigned char));
  rgbBuf = (unsigned char *)malloc(frameWidth * frameHeight * 3 * sizeof(unsigned char));

  fread(yuv422Buf, 1, frameWidth * frameHeight * 2, yuvFile);
  // printf("%s\n", yuv422Buf);
  unsigned char *pYUV = yuv422Buf;
  unsigned char *pRGB = rgbBuf;

  clock_t start, end; //定义clock_t变量
  start = clock();    //开始时间

  // flag = YUYVToRGB_OpenCV(pYUV, pRGB, frameHeight, frameWidth);
  // flag = YUYVToRGB_OpenCV_Resize(pYUV, pRGB, frameHeight, frameWidth, scale_h, scale_w);
  // flag = YUYVToRGB_My(pYUV, pRGB, frameHeight, frameWidth);
  flag = YUYVToRGB_My_Resize(pYUV, (unsigned char *)&pRGB[0], frameHeight, frameWidth, scale_h, scale_w);
  // flag = YUYVToRGB_LUT(pYUV, pRGB, frameHeight, frameWidth);
  // flag = YUYVToRGB_SSE(pYUV, pRGB, frameHeight, frameWidth);
  // flag = YUYVToRGB_SSE_Resize(pYUV, pRGB, frameHeight, frameWidth, scale_h, scale_w);
  end = clock();                                                                    //结束时间
  cout << "time = " << 1000 * double(end - start) / CLOCKS_PER_SEC << "ms" << endl; //输出时间（单位：ｓ）
  cout << "flag:" << flag << endl;
  return 0;
}
