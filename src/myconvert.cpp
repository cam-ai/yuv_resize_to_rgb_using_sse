#include "convert.h"

bool YUYVToRGB_My(unsigned char *pYUV, unsigned char *pRGB, int height, int width)
{
  if (width < 1 || height < 1 || pYUV == NULL || pRGB == NULL)
    return false;
  // yuyv
  // const long len = 3 * width * height;
  unsigned char *yData = pYUV;
  unsigned char *uvData = pYUV;
  // unsigned char *vData = pYUV;

  // cout << &yData << endl;
  // cout << &uData << endl;
  // unsigned char *rgbBuf = NULL;
  // rgbBuf = (unsigned char *)malloc(height * width * 3 * sizeof(unsigned char));

  int rgb[3];
  int yIdx = 0, uIdx = 0, vIdx = 0, idx = 0;

  // int stride_y,stride_uv;
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < 2 * width; j = j + 2)
    {
      yIdx = i * 2 * width + j;
      uIdx = (j % 4 == 0) ? yIdx + 1 : yIdx - 1;
      vIdx = uIdx + 2;
      /*
      rgb[0] = (int)(yData[yIdx] + 1.370705 * (uvData[uIdx] - 128));                                   // r分量
      rgb[1] = (int)(yData[yIdx] - 0.698001 * (uvData[uIdx] - 128) - 0.703125 * (uvData[vIdx] - 128)); // g分量
      rgb[2] = (int)(yData[yIdx] + 1.732446 * (uvData[vIdx] - 128));                                   // b分量
      */
      // 公式先按bgr存的，不然opencv存出来图通道不对劲
      rgb[0] = (int)(yData[yIdx] + 1.770 * (uvData[uIdx] - 128));                                // b分量
      rgb[1] = (int)(yData[yIdx] - 0.343 * (uvData[uIdx] - 128) - 0.714 * (uvData[vIdx] - 128)); // g分量
      rgb[2] = (int)(yData[yIdx] + 1.403 * (uvData[vIdx] - 128));                                // r分量

      for (int k = 0; k < 3; k++)
      {
        idx = (i * width + (j / 2)) * 3 + k;
        if (rgb[k] >= 0 && rgb[k] <= 255)
        {
          pRGB[idx] = rgb[k];
        }
        else
        {
          pRGB[idx] = (rgb[k] < 0) ? 0 : 255;
        }
      }
    }
  }

  return true;
}

bool YUYVToRGB_My_Resize(unsigned char *pYUV, unsigned char *pRGB, int height, int width, float scale_h, float scale_w)
{
  if (width < 1 || height < 1 || pYUV == NULL || pRGB == NULL)
    return false;
  // yuyv
  // const long len = 3 * width * height;
  unsigned char *yData = pYUV;
  unsigned char *uvData = pYUV;
  unsigned char *vData = pYUV;

  // cout << &yData << endl;
  // cout << &uData << endl;
  // unsigned char *rgbBuf = NULL;
  // rgbBuf = (unsigned char *)malloc(height * width * 3 * sizeof(unsigned char));

  int rgb[3];
  int yIdx = 0, uIdx = 0, vIdx = 0, idx = 0;
  float stride_h, stride_w;
  stride_h = 1 / scale_h; // h方向间隔采样
  stride_w = 1 / scale_w; // w方向间隔采样
  int target_h = 0;
  int target_w = 0;

  // int stride_y,stride_uv;
  for (float i = 0; int(i) < height; i = i + stride_h)
  {
    target_h++;
    // cout << target_h << " ";
    target_w = 0;
    for (float j = 0; int(j) < 2 * width - 1; j = j + 2 * stride_w) // 2 * width - 1 是因为最后一个点可能有累计误差，导致最终图像发生偏移！
    {
      target_w++;
      yIdx = (int(j) % 2 == 0) ? int(i) * 2 * width + int(j) : int(i) * 2 * width + int(j) - 1;
      uIdx = (yIdx % 4 == 0) ? yIdx + 1 : yIdx - 1;
      vIdx = uIdx + 2;
      // cout << "i:" << i << " "
      //      << "j:" << j << " ";
      // cout << "yIdx:" << yIdx << " "
      //      << "uIdx:" << uIdx << " "
      //      << "vIdx:" << vIdx << " ";
      /*
      rgb[0] = (int)(yData[yIdx] + 1.370705 * (uvData[uIdx] - 128));                                   // r分量
      rgb[1] = (int)(yData[yIdx] - 0.698001 * (uvData[uIdx] - 128) - 0.703125 * (uvData[vIdx] - 128)); // g分量
      rgb[2] = (int)(yData[yIdx] + 1.732446 * (uvData[vIdx] - 128));                                   // b分量
      */
      // 公式先按bgr存的，不然opencv存出来图通道不对劲
      rgb[0] = (int)(yData[yIdx] + 1.770 * (uvData[uIdx] - 128));                                // b分量
      rgb[1] = (int)(yData[yIdx] - 0.343 * (uvData[uIdx] - 128) - 0.714 * (uvData[vIdx] - 128)); // g分量
      rgb[2] = (int)(yData[yIdx] + 1.403 * (uvData[vIdx] - 128));                                // r分量
      // cout << int(vData[vIdx]) << " ";
      //  std::cout << rgb[1] << " ";
      for (int k = 0; k < 3; k++)
      {
        // idx = ((i / 2) * width + (j / 2) / 2) * 3 + k;
        //  cout << "i:" << i << "j:" << j << "k:" << k << "idx:" << idx << "/";
        // cout << idx << ",";
        if (rgb[k] >= 0 && rgb[k] <= 255)
        {
          pRGB[idx] = rgb[k];
          idx++;
          // cout << pRGB[idx] << ",";
          //  cout << int(pRGB[idx]) << ",";
          //   cout << rgb[k] << ',';
        }
        else
        {
          pRGB[idx] = (rgb[k] < 0) ? 0 : 255;
          // cout << int(pRGB[idx]) << ",";
          //  cout << int(pRGB[idx]) << ",";
          //   cout << rgb[k] << ',';
          idx++;
        }
        // cout << idx << " ";
      }
      // cout << "j:" << j;
    }
  }
  cout << target_h << endl;
  cout << target_w << endl;
  cout << idx << endl;
  //   opencv 保存图片
  Mat dst(896, 896, CV_8UC3, pRGB);
  imwrite("myconvert_resize.png", dst);
  return true;
}
