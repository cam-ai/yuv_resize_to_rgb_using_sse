#include "convert.h"
#include <bitset>

unsigned char ClampToByte(int Value)
{
  if (Value > 255)
    return 255;
  else if (Value < 0)
    return 0;
  else
    return (unsigned char)Value;
}

bool YUYVToRGB_SSE(unsigned char *pYUV, unsigned char *pRGB, int height, int width)
{
  if (width < 1 || height < 1 || pYUV == NULL || pRGB == NULL)
    return false;

  unsigned char *cur = pYUV;
  // Y、U、V都是8bit，把浮点的系数转换矩阵通过移位变成整数；拆分成多段计算是为了防止乘法的溢出；
  //四舍五入
  const int Shift = 13;
  const int HalfV = 1 << (Shift - 1);
  //转换系数矩阵
  const int B_Y_WT = 1 << Shift, B_U_WT = 1.770f * (1 << Shift), B_V_WT = 0;
  const int G_Y_WT = 1 << Shift, G_U_WT = -0.343f * (1 << Shift), G_V_WT = -0.714f * (1 << Shift);
  const int R_Y_WT = 1 << Shift, R_U_WT = 0, R_V_WT = 1.403f * (1 << Shift);

  //_mm_loadu_si128:把之后16个字节的数据读入到一个SSE寄存器中,这里是8个无符号8bit（0-255）
  //_mm_cvtepu8_epi16:把这16个字节的低64位的8个字节数扩展为8个16位数据（把无符号8bit 填充成 16bit short）
  //_mm_setr_epi16:就是用已知数的8个16位数据来构造一个SSE整型数
  //_mm_mullo_epi16:两个16位的乘法
  __m128i Weight_B_Y = _mm_set1_epi32(B_Y_WT), Weight_B_U = _mm_set1_epi32(B_U_WT), Weight_B_V = _mm_set1_epi32(B_V_WT);
  __m128i Weight_G_Y = _mm_set1_epi32(G_Y_WT), Weight_G_U = _mm_set1_epi32(G_U_WT), Weight_G_V = _mm_set1_epi32(G_V_WT);
  __m128i Weight_R_Y = _mm_set1_epi32(R_Y_WT), Weight_R_U = _mm_set1_epi32(R_U_WT), Weight_R_V = _mm_set1_epi32(R_V_WT);
  __m128i Half = _mm_set1_epi32(HalfV);
  __m128i C128 = _mm_set1_epi32(128);
  __m128i Zero = _mm_setzero_si128();

  const int BlockSize = 16, Block = width / BlockSize;

  for (int i = 0; i < height; i++)
  {
    // cur = pYUV + i * width;
    //  printf("%p\n", &*cur);
    for (int j = 0; j < 2 * width; j = j + 32)
    {
      cur = pYUV + i * 2 * width + j;

      // cout << "cur_i:" << &*cur << " ";
      __m128i Src1, Src2, YUYV_Y, YUYV_U, YUYV_V, Blue, Green, Red, Dest1, Dest2, Dest3;
      Src1 = _mm_loadu_si128((__m128i *)(cur));      //分配一个可以存放16个字节的空间:yuyv*4
      Src2 = _mm_loadu_si128((__m128i *)(cur + 16)); //分配一个可以存放16个字节的空间:yuyv*4
      //把16个连续像素（32个字节）的数据顺序由 [SRC1]: Y1 U1 Y2 V1 Y3 U2 Y4 V2 Y5 U3 Y6 V3 Y7 U4 Y8 V4
      //                                    [SRC2]: Y9 U5 Y10 V5 Y11 U6 Y12 V6 Y13 U7 Y14 V7 Y15 U8 Y16 V8

      //转换目标：
      // YUYV_Y：Y1 Y2 Y3 Y4 Y5 Y6 Y7 Y8 Y9 Y10 Y11 Y12 Y13 Y14 Y15
      // YUYV_U：U1 U1 U2 U2 U3 U3 U4 U4 U5 U5 U6 U6 U7 U7 U8 U8
      // YUYV_V：V1 V1 V2 V2 V3 V3 V4 V4 V5 V5 V6 V6 V7 V7 V8 V8

      YUYV_Y = _mm_shuffle_epi8(Src1, _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1));                        // YUYV_Y：Y1 Y2 Y3 Y4 Y5 Y6 Y7 Y8 0 0 0 0 0 0 0 0
      YUYV_Y = _mm_or_si128(YUYV_Y, _mm_shuffle_epi8(Src2, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14)));  // YUYV_Y：Y1 Y2 Y3 Y4 Y5 Y6 Y7 Y8 Y9 Y10 Y11 Y12 Y13 Y14 Y15
      YUYV_U = _mm_shuffle_epi8(Src1, _mm_setr_epi8(1, 1, 5, 5, 9, 9, 13, 13, -1, -1, -1, -1, -1, -1, -1, -1));                         // YUYV_U：U1 U1 U2 U2 U3 U3 U4 U4 0 0 0 0 0 0 0 0
      YUYV_U = _mm_or_si128(YUYV_U, _mm_shuffle_epi8(Src2, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 5, 5, 9, 9, 13, 13)));   // YUYV_U：U1 U1 U2 U2 U3 U3 U4 U4 U5 U5 U6 U6 U7 U7 U8 U8
      YUYV_V = _mm_shuffle_epi8(Src1, _mm_setr_epi8(3, 3, 7, 7, 11, 11, 15, 15, -1, -1, -1, -1, -1, -1, -1, -1));                       // YUYV_V：V1 V1 V2 V2 V3 V3 V4 V4 0 0 0 0 0 0 0 0
      YUYV_V = _mm_or_si128(YUYV_V, _mm_shuffle_epi8(Src2, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 3, 3, 7, 7, 11, 11, 15, 15))); // YUYV_V：V1 V1 V2 V2 V3 V3 V4 V4 V5 V5 V6 V6 V7 V7 V8 V8
      // cout << "blue:" << Blue[0];

      // 以下操作将三个SSE变量里的字节数据分别提取到12个包含4个int类型的数据的SSE变量里，以便后续的乘积操作不溢出

      __m128i Y16L = _mm_unpacklo_epi8(YUYV_Y, Zero);
      __m128i Y16H = _mm_unpackhi_epi8(YUYV_Y, Zero);
      __m128i Y32LL = _mm_unpacklo_epi16(Y16L, Zero);
      __m128i Y32LH = _mm_unpackhi_epi16(Y16L, Zero);
      __m128i Y32HL = _mm_unpacklo_epi16(Y16H, Zero);
      __m128i Y32HH = _mm_unpackhi_epi16(Y16H, Zero);

      /*
      cout << "YUYV_Y bin:" << bitset<64>(YUYV_Y[0]) << endl; // 01000111  01000000  01000111  01000101  01001011  01000011  01001011  01000000
      cout << "Y16L bin:" << bitset<64>(Y16L[0]) << endl;     // 00000000  01001011  00000000  01000011  00000000  01001011  00000000  01000000
      cout << "Y16H bin:" << bitset<64>(Y16H[1]) << endl;     // 00000000  01001001  00000000  01001001  00000000  01001001  00000000  01001000
      cout << "Y32LL bin:" << bitset<64>(Y32LL[0]) << endl;   // 00000000  00000000  00000000  01001011  00000000  00000000  00000000  01000000
      cout << "Y32LH bin:" << bitset<64>(Y32LH[0]) << endl;   // 00000000  00000000  00000000  01000111  00000000  00000000  00000000  01000101
      cout << "Y32HL bin:" << bitset<64>(Y32HL[0]) << endl;   // 00000000  00000000  00000000  01001001  00000000  00000000  00000000  01001000
      cout << "Y32HH bin:" << bitset<64>(Y32HH[0]) << endl;   // 00000000  00000000  00000000  01000111  00000000  00000000  00000000  01001001
      */

      __m128i U16L = _mm_unpacklo_epi8(YUYV_U, Zero);
      __m128i U16H = _mm_unpackhi_epi8(YUYV_U, Zero);
      __m128i U32LL = _mm_unpacklo_epi16(U16L, Zero);
      __m128i U32LH = _mm_unpackhi_epi16(U16L, Zero);
      __m128i U32HL = _mm_unpacklo_epi16(U16H, Zero);
      __m128i U32HH = _mm_unpackhi_epi16(U16H, Zero);

      U32LL = _mm_sub_epi32(U32LL, C128);
      U32LH = _mm_sub_epi32(U32LH, C128);
      U32HL = _mm_sub_epi32(U32HL, C128);
      U32HH = _mm_sub_epi32(U32HH, C128);

      __m128i V16L = _mm_unpacklo_epi8(YUYV_V, Zero);
      __m128i V16H = _mm_unpackhi_epi8(YUYV_V, Zero);
      __m128i V32LL = _mm_unpacklo_epi16(V16L, Zero);
      __m128i V32LH = _mm_unpackhi_epi16(V16L, Zero);
      __m128i V32HL = _mm_unpacklo_epi16(V16H, Zero);
      __m128i V32HH = _mm_unpackhi_epi16(V16H, Zero);
      V32LL = _mm_sub_epi32(V32LL, C128);
      V32LH = _mm_sub_epi32(V32LH, C128);
      V32HL = _mm_sub_epi32(V32HL, C128);
      V32HH = _mm_sub_epi32(V32HH, C128);

      //系数矩阵乘加计算

      __m128i LL_B = _mm_add_epi32(Y32LL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32LL, Weight_B_U)), Shift));
      __m128i LH_B = _mm_add_epi32(Y32LH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32LH, Weight_B_U)), Shift));
      __m128i HL_B = _mm_add_epi32(Y32HL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32HL, Weight_B_U)), Shift));
      __m128i HH_B = _mm_add_epi32(Y32HH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32HH, Weight_B_U)), Shift));
      Blue = _mm_packus_epi16(_mm_packus_epi32(LL_B, LH_B), _mm_packus_epi32(HL_B, HH_B)); // B1 B2 ...B16

      __m128i LL_G = _mm_add_epi32(Y32LL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32LL), _mm_mullo_epi32(Weight_G_V, V32LL))), Shift));
      __m128i LH_G = _mm_add_epi32(Y32LH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32LH), _mm_mullo_epi32(Weight_G_V, V32LH))), Shift));
      __m128i HL_G = _mm_add_epi32(Y32HL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32HL), _mm_mullo_epi32(Weight_G_V, V32HL))), Shift));
      __m128i HH_G = _mm_add_epi32(Y32HH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32HH), _mm_mullo_epi32(Weight_G_V, V32HH))), Shift));
      Green = _mm_packus_epi16(_mm_packus_epi32(LL_G, LH_G), _mm_packus_epi32(HL_G, HH_G));

      __m128i LL_R = _mm_add_epi32(Y32LL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32LL, Weight_R_V)), Shift));
      __m128i LH_R = _mm_add_epi32(Y32LH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32LH, Weight_R_V)), Shift));
      __m128i HL_R = _mm_add_epi32(Y32HL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32HL, Weight_R_V)), Shift));
      __m128i HH_R = _mm_add_epi32(Y32HH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32HH, Weight_R_V)), Shift));
      Red = _mm_packus_epi16(_mm_packus_epi32(LL_R, LH_R), _mm_packus_epi32(HL_R, HH_R));

      //移位

      Dest1 = _mm_shuffle_epi8(Blue, _mm_setr_epi8(0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1, 5));
      Dest1 = _mm_or_si128(Dest1, _mm_shuffle_epi8(Green, _mm_setr_epi8(-1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1)));
      Dest1 = _mm_or_si128(Dest1, _mm_shuffle_epi8(Red, _mm_setr_epi8(-1, -1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1)));

      Dest2 = _mm_shuffle_epi8(Blue, _mm_setr_epi8(-1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1, 10, -1));
      Dest2 = _mm_or_si128(Dest2, _mm_shuffle_epi8(Green, _mm_setr_epi8(5, -1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1, 10)));
      Dest2 = _mm_or_si128(Dest2, _mm_shuffle_epi8(Red, _mm_setr_epi8(-1, 5, -1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1)));

      Dest3 = _mm_shuffle_epi8(Blue, _mm_setr_epi8(-1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1, -1));
      Dest3 = _mm_or_si128(Dest3, _mm_shuffle_epi8(Green, _mm_setr_epi8(-1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1)));
      Dest3 = _mm_or_si128(Dest3, _mm_shuffle_epi8(Red, _mm_setr_epi8(10, -1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15)));

      _mm_storeu_si128((__m128i *)(pRGB + i * 3 * width + (j / 32) * BlockSize * 3), Dest1);
      _mm_storeu_si128((__m128i *)(pRGB + i * 3 * width + (j / 32) * BlockSize * 3 + BlockSize), Dest2);
      _mm_storeu_si128((__m128i *)(pRGB + i * 3 * width + (j / 32) * BlockSize * 3 + BlockSize * 2), Dest3);
    }
  }
  // Mat dst(height, width, CV_8UC3, pRGB);
  // imwrite("/home/cia5sgh/cjx/opencv/out/mysse.png", dst);
  return true;
}

bool YUYVToRGB_SSE_Resize(unsigned char *pYUV, unsigned char *pRGB, int height, int width, float scale_h, float scale_w)
{
  if (width < 1 || height < 1 || pYUV == NULL || pRGB == NULL)
    return false;

  //先作resize,同时将yuyv排列变成yy...y uu...u vv...v(442-->420)
  unsigned char *yuv420Buf = NULL;
  int height_new = scale_h * height;
  int width_new = scale_w * width;
  int h_w = height_new * width_new;
  yuv420Buf = (unsigned char *)malloc(h_w * 3 * sizeof(unsigned char));
  unsigned char *yData = pYUV;
  unsigned char *uvData = pYUV;
  unsigned char *vData = pYUV;
  // int rgb[3];

  cout << "height_new:" << height_new << endl;
  cout << "width_new:" << width_new << endl;

  int yuv422_yIdx = 0, yuv422_uIdx = 0, yuv422_vIdx = 0, yuv420_yIdx = 0, yuv420_uIdx = 0, yuv420_vIdx = 0;
  float stride_h, stride_w;
  stride_h = 1 / scale_h; // h方向间隔采样
  stride_w = 1 / scale_w; // w方向间隔采样
  // int temp = 0;
  int target_h = 0;
  int target_w = 0;

  yuv420_yIdx = 0;
  yuv420_uIdx = h_w;
  yuv420_vIdx = 2 * h_w;
  for (float i = 0; int(i) < height; i = i + stride_h)
  {
    target_h++;

    for (float j = 0; int(j) < 2 * width - 1; j = j + 2 * stride_w) // 2 * width - 1 是因为最后一个点可能有累计误差，导致最终图像发生偏移！
    {
      if (int(i) == 0)
        target_w++;
      yuv422_yIdx = (int(j) % 2 == 0) ? int(i) * 2 * width + int(j) : int(i) * 2 * width + int(j) - 1;
      yuv422_uIdx = (yuv422_yIdx % 4 == 0) ? yuv422_yIdx + 1 : yuv422_yIdx - 1;
      yuv422_vIdx = yuv422_uIdx + 2;

      yuv420Buf[yuv420_yIdx] = yData[yuv422_yIdx];
      yuv420_yIdx++;
      yuv420Buf[yuv420_uIdx] = yData[yuv422_uIdx];
      yuv420_uIdx++;
      yuv420Buf[yuv420_vIdx] = yData[yuv422_vIdx];
      yuv420_vIdx++;
      //   cout << "yuv422_yIdx:" << yuv422_yIdx << " ";
      //     cout << "yuv420_vIdx:" << yuv420_vIdx << " ";
      //      yuv420_vIdx = yuv420_vIdx + 2;
    }
  }

  unsigned char *cur = yuv420Buf;
  unsigned char *cur_u = yuv420Buf + h_w;
  unsigned char *cur_v = yuv420Buf + 2 * h_w;
  //四舍五入
  const int Shift = 13;
  const int HalfV = 1 << (Shift - 1);
  //转换系数矩阵
  const int B_Y_WT = 1 << Shift, B_U_WT = 1.770f * (1 << Shift), B_V_WT = 0;
  const int G_Y_WT = 1 << Shift, G_U_WT = -0.343f * (1 << Shift), G_V_WT = -0.714f * (1 << Shift);
  const int R_Y_WT = 1 << Shift, R_U_WT = 0, R_V_WT = 1.403f * (1 << Shift);

  //_mm_loadu_si128:把之后16个字节的数据读入到一个SSE寄存器中,这里是8个无符号8bit（0-255）
  //_mm_cvtepu8_epi16:把这16个字节的低64位的8个字节数扩展为8个16位数据（把无符号8bit 填充成 16bit short）
  //_mm_setr_epi16:就是用已知数的8个16位数据来构造一个SSE整型数
  //_mm_mullo_epi16:两个16位的乘法
  __m128i Weight_B_Y = _mm_set1_epi32(B_Y_WT), Weight_B_U = _mm_set1_epi32(B_U_WT), Weight_B_V = _mm_set1_epi32(B_V_WT);
  __m128i Weight_G_Y = _mm_set1_epi32(G_Y_WT), Weight_G_U = _mm_set1_epi32(G_U_WT), Weight_G_V = _mm_set1_epi32(G_V_WT);
  __m128i Weight_R_Y = _mm_set1_epi32(R_Y_WT), Weight_R_U = _mm_set1_epi32(R_U_WT), Weight_R_V = _mm_set1_epi32(R_V_WT);
  __m128i Half = _mm_set1_epi32(HalfV);
  __m128i C128 = _mm_set1_epi32(128);
  __m128i Zero = _mm_setzero_si128();

  const int BlockSize = 16, Block = width_new / BlockSize;

  for (int i = 0; i < height_new; i++)
  {
    for (int j = 0; (j / 16) < Block; j = j + 16)
    {
      cur = yuv420Buf + i * width_new + j;
      cur_u = cur + h_w;
      cur_v = cur_u + h_w;
      // printf("%p\n", &*cur);
      __m128i YUYV_Y, YUYV_U, YUYV_V, Blue, Green, Red, Dest1, Dest2, Dest3;

      //转换目标：
      // YUYV_Y：Y1 Y2 Y3 Y4 Y5 Y6 Y7 Y8 Y9 Y10 Y11 Y12 Y13 Y14 Y15
      // YUYV_U：U1 U1 U2 U2 U3 U3 U4 U4 U5 U5 U6 U6 U7 U7 U8 U8
      // YUYV_V：V1 V1 V2 V2 V3 V3 V4 V4 V5 V5 V6 V6 V7 V7 V8 V8

      YUYV_Y = _mm_loadu_si128((__m128i *)(cur));
      YUYV_U = _mm_loadu_si128((__m128i *)(cur_u));
      YUYV_V = _mm_loadu_si128((__m128i *)(cur_v));
      // cout << YUYV_Y[0] << " ";

      // 以下操作将三个SSE变量里的字节数据分别提取到12个包含4个int类型的数据的SSE变量里，以便后续的乘积操作不溢出

      __m128i Y16L = _mm_unpacklo_epi8(YUYV_Y, Zero);
      __m128i Y16H = _mm_unpackhi_epi8(YUYV_Y, Zero);
      __m128i Y32LL = _mm_unpacklo_epi16(Y16L, Zero);
      __m128i Y32LH = _mm_unpackhi_epi16(Y16L, Zero);
      __m128i Y32HL = _mm_unpacklo_epi16(Y16H, Zero);
      __m128i Y32HH = _mm_unpackhi_epi16(Y16H, Zero);

      __m128i U16L = _mm_unpacklo_epi8(YUYV_U, Zero);
      __m128i U16H = _mm_unpackhi_epi8(YUYV_U, Zero);
      __m128i U32LL = _mm_unpacklo_epi16(U16L, Zero);
      __m128i U32LH = _mm_unpackhi_epi16(U16L, Zero);
      __m128i U32HL = _mm_unpacklo_epi16(U16H, Zero);
      __m128i U32HH = _mm_unpackhi_epi16(U16H, Zero);
      U32LL = _mm_sub_epi32(U32LL, C128);
      U32LH = _mm_sub_epi32(U32LH, C128);
      U32HL = _mm_sub_epi32(U32HL, C128);
      U32HH = _mm_sub_epi32(U32HH, C128);

      __m128i V16L = _mm_unpacklo_epi8(YUYV_V, Zero);
      __m128i V16H = _mm_unpackhi_epi8(YUYV_V, Zero);
      __m128i V32LL = _mm_unpacklo_epi16(V16L, Zero);
      __m128i V32LH = _mm_unpackhi_epi16(V16L, Zero);
      __m128i V32HL = _mm_unpacklo_epi16(V16H, Zero);
      __m128i V32HH = _mm_unpackhi_epi16(V16H, Zero);
      V32LL = _mm_sub_epi32(V32LL, C128);
      V32LH = _mm_sub_epi32(V32LH, C128);
      V32HL = _mm_sub_epi32(V32HL, C128);
      V32HH = _mm_sub_epi32(V32HH, C128);

      //系数矩阵乘加计算

      __m128i LL_B = _mm_add_epi32(Y32LL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32LL, Weight_B_U)), Shift));
      __m128i LH_B = _mm_add_epi32(Y32LH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32LH, Weight_B_U)), Shift));
      __m128i HL_B = _mm_add_epi32(Y32HL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32HL, Weight_B_U)), Shift));
      __m128i HH_B = _mm_add_epi32(Y32HH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(U32HH, Weight_B_U)), Shift));
      Blue = _mm_packus_epi16(_mm_packus_epi32(LL_B, LH_B), _mm_packus_epi32(HL_B, HH_B)); // B1 B2 ...B16

      __m128i LL_G = _mm_add_epi32(Y32LL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32LL), _mm_mullo_epi32(Weight_G_V, V32LL))), Shift));
      __m128i LH_G = _mm_add_epi32(Y32LH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32LH), _mm_mullo_epi32(Weight_G_V, V32LH))), Shift));
      __m128i HL_G = _mm_add_epi32(Y32HL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32HL), _mm_mullo_epi32(Weight_G_V, V32HL))), Shift));
      __m128i HH_G = _mm_add_epi32(Y32HH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_add_epi32(_mm_mullo_epi32(Weight_G_U, U32HH), _mm_mullo_epi32(Weight_G_V, V32HH))), Shift));
      Green = _mm_packus_epi16(_mm_packus_epi32(LL_G, LH_G), _mm_packus_epi32(HL_G, HH_G));

      __m128i LL_R = _mm_add_epi32(Y32LL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32LL, Weight_R_V)), Shift));
      __m128i LH_R = _mm_add_epi32(Y32LH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32LH, Weight_R_V)), Shift));
      __m128i HL_R = _mm_add_epi32(Y32HL, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32HL, Weight_R_V)), Shift));
      __m128i HH_R = _mm_add_epi32(Y32HH, _mm_srai_epi32(_mm_add_epi32(Half, _mm_mullo_epi32(V32HH, Weight_R_V)), Shift));
      Red = _mm_packus_epi16(_mm_packus_epi32(LL_R, LH_R), _mm_packus_epi32(HL_R, HH_R));

      //移位

      Dest1 = _mm_shuffle_epi8(Blue, _mm_setr_epi8(0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1, 5));
      Dest1 = _mm_or_si128(Dest1, _mm_shuffle_epi8(Green, _mm_setr_epi8(-1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1)));
      Dest1 = _mm_or_si128(Dest1, _mm_shuffle_epi8(Red, _mm_setr_epi8(-1, -1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1)));

      Dest2 = _mm_shuffle_epi8(Blue, _mm_setr_epi8(-1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1, 10, -1));
      Dest2 = _mm_or_si128(Dest2, _mm_shuffle_epi8(Green, _mm_setr_epi8(5, -1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1, 10)));
      Dest2 = _mm_or_si128(Dest2, _mm_shuffle_epi8(Red, _mm_setr_epi8(-1, 5, -1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1)));

      Dest3 = _mm_shuffle_epi8(Blue, _mm_setr_epi8(-1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1, -1));
      Dest3 = _mm_or_si128(Dest3, _mm_shuffle_epi8(Green, _mm_setr_epi8(-1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1)));
      Dest3 = _mm_or_si128(Dest3, _mm_shuffle_epi8(Red, _mm_setr_epi8(10, -1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15)));

      _mm_storeu_si128((__m128i *)(pRGB + i * 3 * width_new + (j / 16) * BlockSize * 3), Dest1);
      _mm_storeu_si128((__m128i *)(pRGB + i * 3 * width_new + (j / 16) * BlockSize * 3 + BlockSize), Dest2);
      _mm_storeu_si128((__m128i *)(pRGB + i * 3 * width_new + (j / 16) * BlockSize * 3 + BlockSize * 2), Dest3);
      // temp = temp + 48;
      // cout << "temp:" << temp;
      // cout << "j:" << i * 3 * width_new + (j / 16) * BlockSize * 3 << " ";
    }
    // cout << "i:" << i << endl;

    for (int j = Block * BlockSize; j < width_new; j++)
    {
      // cout << endl;
      // cout << "j:" << i * 3 * width_new + j * 3 << " ";
      int YV = yuv420Buf[i * width_new + j], UV = yuv420Buf[i * width_new + j + h_w] - 128, VV = yuv420Buf[i * width_new + j + 2 * h_w] - 128;
      pRGB[i * 3 * width_new + j * 3 + 0] = ClampToByte(YV + ((B_U_WT * UV + HalfV) >> Shift));
      pRGB[i * 3 * width_new + j * 3 + 1] = ClampToByte(YV + ((G_U_WT * UV + G_V_WT * VV + HalfV) >> Shift));
      pRGB[i * 3 * width_new + j * 3 + 2] = ClampToByte(YV + ((R_V_WT * VV + HalfV) >> Shift));
      // temp = temp + 3;
      // cout << "temp:" << temp;
    }

    // cout << endl;
  }

  // Mat dst(height_new, width_new, CV_8UC3, pRGB);
  Mat dst(target_h, target_w, CV_8UC3, pRGB);
  imwrite("mysse_resize.png", dst);

  return true;
}
