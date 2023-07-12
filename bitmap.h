#pragma once
#include <Windows.h>
#include <dwmapi.h>
#include <thread>
#include "common_utils.h"
#include "work_thread.h"
#include "libyuv\scale_argb.h"
#include "libyuv\planar_functions.h"
extern HWND hWndChild;

void InitMaskBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO* bmpinfo);
void InitColorBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO& bmi);
void PrintMaskBitmap(uint8_t* mask_bits);
void PrintColorBitmap(uint8_t* color_bits);

bool YUVBlend(uint8_t* dst_y,
              uint8_t* dst_u,
              uint8_t* dst_v,
              int dst_width,
              int dst_height,
              int dst_xpos,
              int dst_ypos,
              const uint8_t* src_y,
              const uint8_t* src_u,
              const uint8_t* src_v,
              int src_width,
              int src_height);

void InitBitMapInfoHeader(int width, int height, bool top2bottom, int bit_count, BITMAPINFOHEADER* bih)
{
    memset(bih, 0, sizeof(BITMAPINFOHEADER));
    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = width;
    bih->biHeight = top2bottom ? -height : height;
    bih->biPlanes = 1;
    bih->biBitCount = bit_count;
    bih->biCompression = BI_RGB;
    bih->biSizeImage = (width * height * bit_count) / 8;
}

void InitBitMapInfo(int width, int height, bool top2bottom, int bit_count, BITMAPINFO* bmi)
{
    InitBitMapInfoHeader(width, height, top2bottom, bit_count, &bmi->bmiHeader);
    memset(bmi->bmiColors, 0, sizeof(RGBQUAD));
}

int CaptureCursorBitmap(HCURSOR hCursor, uint8_t* &mask_bits, uint8_t* &color_bits)
{
    HICON hIcon = CopyIcon(hCursor);
    ICONINFO info = { 0 };
    bool success = false;
    INT nWidth = 0;
    INT nHeight = 0;

    do
    {
        if (!hIcon)
        {
            printf_s("copyIcon failed, error code:%d!\n", GetLastError());
            break;
        }

        if (!GetIconInfo(hIcon, &info))
        {
            printf_s("GetIconInfo failed, error code:%d!\n", GetLastError());
            break;
        }

        if (info.hbmMask == NULL)
        {
            printf_s("hbmMask is empty!\n");
            break;
        }

        BITMAP bm = { 0 };
        GetObject(info.hbmMask, sizeof(bm), &bm);
        nWidth = bm.bmWidth;
        nHeight = bm.bmHeight;

        if (nHeight < 0 || nHeight % 2 != 0)
        {
            break;
        }

        int ncolors = 1 << bm.bmBitsPixel;
        int bmpinfo_size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * ncolors;
        std::vector<uint8_t> buf(bmpinfo_size);
        BITMAPINFO* bmpinfo = (BITMAPINFO*)buf.data();
        InitMaskBitMap(nWidth, nHeight, true, bm.bmBitsPixel, bmpinfo);

        INT nPixelCount = nWidth * 32;

        if (nHeight == 32)
        {
            // 彩色光标处理
            // Capture AND mask bmp data
            HDC mask_dc = CreateCompatibleDC(NULL);
            bool generate_mask = true;

            do
            {
                mask_bits = new uint8_t[nPixelCount / 8];
                if (mask_bits == nullptr)
                {
                    // request memory failed
                    break;
                }
                memset(mask_bits, 0, nPixelCount / 8);

                if (!GetDIBits(mask_dc, info.hbmMask, 0, 32, mask_bits, bmpinfo, DIB_RGB_COLORS))
                {
                    // failed
                    break;
                }

                // If AND mask is standard mask bitmap, there is no need to generate mask
                for (int i = 0; i < nPixelCount / 8; i++)
                {
                    if (mask_bits[i] == 0)
                    {
                        continue;
                    }
                    else
                    {
                        generate_mask = false;
                        break;
                    }
                }

            } while (0);

            // PrintMaskBitmap(mask_bits);
            printf_s("cursor handle:%d\n", hCursor);
            printf_s("generate mask is:%d\n", generate_mask);


            DeleteDC(mask_dc);

            ///////////////////////////
            //Capture Color bmp data
            HDC color_dc = CreateCompatibleDC(NULL);
            BITMAPINFO bmi = { 0 };
            InitColorBitMap(nWidth, nHeight, true, 32, bmi);

            do
            {
                color_bits = new(std::nothrow) uint8_t[bmi.bmiHeader.biSizeImage];
                if (color_bits == nullptr)
                {
                    // request memory failed
                    break;
                }
                memset(color_bits, 0, bmi.bmiHeader.biSizeImage);

                if (!GetDIBits(color_dc, info.hbmColor, 0, 32, color_bits, &bmi, DIB_RGB_COLORS))
                {
                    // failed
                    break;
                }
                PrintColorBitmap(color_bits);

                if (generate_mask)
                {
                    INT byte_idx = 0;
                    INT bit_idx = 0;
                    for (INT i = 0; i < nPixelCount; i++)
                    {
                        // 在高度设为负值后，bmp位图正向显示，位图数据也变为了RGBA的顺序，alpha通道在最后一个字节
                        if (color_bits[i * 4 + 3] == 0)
                        {
                            byte_idx = i / 8;
                            bit_idx = i % 8;
                            mask_bits[byte_idx] |= (0x80 >> bit_idx);
                        }
                    }
                }
            } while (0);

            DeleteDC(color_dc);
        }
        else if (nHeight == 64)
        {
            // 黑白光标处理
            // Capture AND mask bmp data
            HDC mask_dc = CreateCompatibleDC(NULL);

            do
            {
                mask_bits = new uint8_t[nPixelCount / 8];
                if (mask_bits == nullptr)
                {
                    // failed
                    break;
                }
                memset(mask_bits, 0, nPixelCount / 8);

                // AND mask 位于下半部分
                if (!GetDIBits(mask_dc, info.hbmMask, 32, 64, mask_bits, bmpinfo, DIB_RGB_COLORS))
                {
                    // failed
                    break;
                }
            } while (0);

            DeleteDC(mask_dc);


            ///////////////////////////
            //Capture XOR mask bmp data
            HDC color_dc = CreateCompatibleDC(NULL);
            do
            {
                color_bits = new uint8_t[nPixelCount / 8];
                if (color_bits == nullptr)
                {
                    break;
                }
                memset(color_bits, 0, nPixelCount / 8);

                // XOR mask 位于上半部分
                if (!GetDIBits(color_dc, info.hbmMask, 0, 32, color_bits, bmpinfo, DIB_RGB_COLORS))
                {
                    break;
                }
            } while (0);

            DeleteDC(color_dc);

            PrintMaskBitmap(mask_bits);
            PrintMaskBitmap(color_bits);
        }
        else
        {
            // not support format
            break;
        }
        success = true;
    } while (0);
    
    if (success)
    {
        uint8_t* dst_argb = new uint8_t[10 * 10 * 4];

        int res = libyuv::ARGBScale(color_bits, 32 * 4, 32, 32,
                                    dst_argb, 10 * 4, 10, 10,
                                    libyuv::kFilterBox);

        //uint8_t* ori_argb = new uint8_t[32 * 32 * 4];
        //res = libyuv::ARGBScale(dst_argb, 26 * 4, 26, 26,
        //                        ori_argb, 32 * 4, 32, 32,
        //                        libyuv::kFilterBox);

        // res = libyuv::ARGBBlend(color_bits, 32 * 4, dst_argb, 26 * 4, ori_argb, 32 * 4, 32, 32);

        HCURSOR h1 = NULL;
        HBITMAP mask_bmp;
        HBITMAP color_bmp;
        if (nHeight == 64)
        {
            /*mask_bmp = CreateBitmap(nWidth, 32, 1, 1, mask_bits);
            color_bmp = CreateBitmap(nWidth, 32, 1, 1, color_bits);
            h1 = CreateCursor(NULL, info.xHotspot, info.yHotspot, 32, 32, mask_bits, color_bits);*/

            mask_bmp = CreateBitmap(nWidth, 32, 1, 1, mask_bits_global);
            color_bmp = CreateBitmap(nWidth, 32, 1, 32, color_bits_global);
            DeleteObject(info.hbmMask);
            DeleteObject(info.hbmColor);
            info.hbmMask = mask_bmp;
            info.hbmColor = color_bmp;
            h1 = ::CreateIconIndirect(&info);
        }
        else
        {
            mask_bmp = CreateBitmap(nWidth, 32, 1, 1, mask_bits);
            color_bmp = CreateBitmap(nWidth, 32, 1, 32, color_bits);
            // color_bmp = CreateBitmap(nWidth, 32, 1, 32, ori_argb);
            DeleteObject(info.hbmMask);
            DeleteObject(info.hbmColor);
            info.hbmMask = mask_bmp;
            info.hbmColor = color_bmp;
            h1 = ::CreateIconIndirect(&info);
        }

        HDC wnd = GetWindowDC(hWndChild);
        HDC color_dc = CreateCompatibleDC(NULL);
        SelectObject(color_dc, color_bmp);
        HDC mask_dc = CreateCompatibleDC(NULL);
        SelectObject(mask_dc, mask_bmp);
        BitBlt(wnd, 100, 100, 32, 32, NULL, 0, 0, WHITENESS);
        //BitBlt(wnd, 100, 100, 32, 32, mask_dc, 0, 0, SRCAND);
        //BitBlt(wnd, 100, 100, 32, 32, color_dc, 0, 0, SRCINVERT);
        BLENDFUNCTION ftn = { 0 };
        ftn.BlendOp = AC_SRC_OVER;
        ftn.SourceConstantAlpha = 255;
        ftn.AlphaFormat = AC_SRC_ALPHA;
        AlphaBlend(wnd, 100, 100, 32, 32, color_dc, 0, 0, 32, 32, ftn);

        DeleteDC(color_dc);
        DeleteDC(mask_dc);
        ReleaseDC(hWndChild, wnd);

        if (h1 != NULL)
        {
            SetClassLongPtr(hWndChild, GCLP_HCURSOR, (LONG_PTR)(h1));
            PostMessage(hWndChild, WM_SETCURSOR, (UINT)(hWndChild), 33554433);
            DestroyCursor(h1);
            Sleep(1000);
        }
    }

    if (hIcon)
    {
        DestroyIcon(hIcon);
    }

    if (info.hbmMask)
    {
        DeleteObject(info.hbmMask);
    }

    if (info.hbmColor)
    {
        DeleteObject(info.hbmColor);
    }

    return 0;
}

void InitMaskBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO *bmpinfo)
{
    bmpinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo->bmiHeader.biWidth = width;
    bmpinfo->bmiHeader.biHeight = top2bottom  ? -height : height;
    bmpinfo->bmiHeader.biPlanes = 1;
    bmpinfo->bmiHeader.biBitCount = bit_count;
    bmpinfo->bmiHeader.biCompression = BI_RGB;
    bmpinfo->bmiHeader.biSizeImage = (width * height * bit_count) / 8;
}

void InitColorBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO &bmi)
{
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = top2bottom ? -height : height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = bit_count;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = (width * height * bit_count) / 8;
    memset(bmi.bmiColors, 0, sizeof(RGBQUAD));
}

void PrintMaskBitmap(uint8_t *mask_bits)
{
    printf_s("bitmap mask data\n");
    for (int i = 0; i < 32 * 32 / 8; i++)
    {
        if (i % 4 == 0) {
            printf_s("\n");
        }
        uint8_t tmp = mask_bits[i];
        for (int j = 0; j < 8; j++)
        {
            bool res = tmp & 0x80;
            tmp = tmp << 1;
            printf_s("%d", res);
        }
    }
    printf_s("\n");
}

void PrintColorThumbnail(uint8_t* color_bits)
{
    uint32_t* color = (uint32_t*)color_bits;
    printf_s("\ncolor Thumbnail\n");
    for (int i = 0; i < 32 * 32; i++)
    {
        if (i % 32 == 0)
        {
            printf_s("\n");
        }
        uint32_t value = *(color++);
        if (value != 0)
        {
            printf_s("1");
        }
        else
        {
            printf_s("0");
        }
    }
    printf_s("\n");
}

void PrintColorBitmap(uint8_t* color_bits)
{
    PrintColorThumbnail(color_bits);
    printf_s("\ncolor mask data\n");
    for (int i = 0; i < 32 * 32; i++)
    {
        if (i % 32 == 0)
        {
            printf_s("\n");
        }
        for (int j = 0; j < 4; j++)
        {
            uint8_t tmp = color_bits[i * 4 + j];
            if (tmp == 0)
            {
                printf_s("00");
            }
            else
            {
                printf_s("%02x", tmp);
            }
        }
        printf_s("\t");
    }
    printf_s("\n");
}

bool YUVBlend(uint8_t* dst_y,
              uint8_t* dst_u,
              uint8_t* dst_v,
              int dst_width,
              int dst_height,
              int dst_xpos,
              int dst_ypos,
              const uint8_t* src_y,
              const uint8_t* src_u,
              const uint8_t* src_v,
              int src_width,
              int src_height)
{
    if (src_width > dst_width || src_height > dst_height)
    {
        return false;
    }

    int off_size = 0;
    for (int i = 0; i < src_height; i++)
    {
        off_size = dst_width * (dst_ypos + i) + dst_xpos;
        memcpy(dst_y + off_size, src_y + src_width * i, src_width);
    }

    int dst_uv_x_off = dst_xpos / 2;
    int dst_uv_y_off = dst_ypos / 2;
    int src_half_width = (src_width + 1) / 2;
    int src_half_height = (src_height + 1) / 2;
    int dst_half_width = (dst_width + 1) / 2;
    int dst_half_height = (dst_height + 1) / 2;

    for (int i = 0; i < src_half_height; i++)
    {
        off_size = dst_half_width * (dst_uv_y_off + i) + dst_uv_x_off;
        memcpy(dst_u + off_size, src_u + src_half_width * i, src_half_width);
        memcpy(dst_v + off_size, src_v + src_half_width * i, src_half_width);
    }

    return true;
}


//uint8_t mask_bits_global[] =
//{
//    0xff, 0xff, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff,
//    0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff,
//    0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff,
//    0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff,
//    0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
//};
//
//uint32_t color_bits_global[] =
//{
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0xffffffff, 0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0xffffffff, 0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
//};