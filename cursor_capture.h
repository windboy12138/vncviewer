#pragma once
#include <Windows.h>
#include <vector>
#include <stdint.h>

HCURSOR last_cursor_ = nullptr;

void InitMaskBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO* bmpinfo);
void InitColorBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO& bmi);

bool HasCursorChanged()
{
    CURSORINFO ci = { 0 };
    ci.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&ci);

    if (!(ci.flags & CURSOR_SHOWING))
    {
        // to-do: cursor not show, change magnifier?
        return false;
    }

    if (last_cursor_ == ci.hCursor)
    {
        return false;
    }

    // to-do: save cursor position
    int x = ci.ptScreenPos.x;
    int y = ci.ptScreenPos.y;

    last_cursor_ = ci.hCursor;
    return true;
}

bool CaptureCursorBitmap(HCURSOR hCursor, uint8_t*& mask_bits, uint8_t*& color_bits)
{
    HICON hIcon = CopyIcon(hCursor);
    ICONINFO info = { 0 };
    bool success = false;
    INT width = 0;
    INT height = 0;

    do
    {
        if (!hIcon)
        {
            // printf_s("copyIcon failed, error code:%d!\n", GetLastError());
            break;
        }

        if (!GetIconInfo(hIcon, &info))
        {
            // printf_s("GetIconInfo failed, error code:%d!\n", GetLastError());
            break;
        }

        if (info.hbmMask == NULL)
        {
            // printf_s("hbmMask is empty!\n");
            break;
        }

        BITMAP bm = { 0 };
        GetObject(info.hbmMask, sizeof(bm), &bm);
        width = bm.bmWidth;
        height = bm.bmHeight;

        if (height < 0 || height % 2 != 0)
        {
            break;
        }

        int ncolors = 1 << bm.bmBitsPixel;
        int bmpinfo_size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * ncolors;
        std::vector<uint8_t> buf(bmpinfo_size);
        BITMAPINFO* bmpinfo = (BITMAPINFO*)buf.data();
        //InitMaskBitMap(width, height, true, bm.bmBitsPixel, bmpinfo);
        InitMaskBitMap(width, height, false, bm.bmBitsPixel, bmpinfo);

        INT pixel_count = width * 32;

        if (height == 32)
        {
            // 彩色光标处理
            // Capture AND mask bmp data
            HDC mask_dc = CreateCompatibleDC(NULL);
            bool generate_mask = true;

            do
            {
                mask_bits = new uint8_t[pixel_count / 8];
                if (mask_bits == nullptr)
                {
                    // request memory failed
                    break;
                }
                memset(mask_bits, 0, pixel_count / 8);

                if (!GetDIBits(mask_dc, info.hbmMask, 0, 32, mask_bits, bmpinfo, DIB_RGB_COLORS))
                {
                    // failed
                    break;
                }

                // If AND mask is standard mask bitmap, there is no need to generate mask
                for (int i = 0; i < pixel_count / 8; i++)
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

            DeleteDC(mask_dc);

            ///////////////////////////
            //Capture Color bmp data
            HDC color_dc = CreateCompatibleDC(NULL);
            BITMAPINFO bmi = { 0 };
            InitColorBitMap(width, height, false, 32, bmi);

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

                if (generate_mask)
                {
                    INT byte_idx = 0;
                    INT bit_idx = 0;
                    for (INT i = 0; i < pixel_count; i++)
                    {
                        // 在高度设为负值后，bmp位图正向显示，位图数据也变为了BGRA的顺序，alpha通道在最后一个字节
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
        else if (height == 64)
        {
            // 黑白光标
            mask_bits = new uint8_t[pixel_count / 8];
            if (mask_bits == nullptr)
            {
                // request memory failed
                break;
            }
            memcpy(mask_bits, mask_bits_global, pixel_count / 8);

            color_bits = new(std::nothrow) uint8_t[pixel_count * 4];
            if (color_bits == nullptr)
            {
                // request memory failed
                break;
            }
            memcpy(color_bits, color_bits_global, pixel_count * 4);
            
        }
        else
        {
            // not support format
            break;
        }

        success = true;

    } while (0);

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

    return success;
}

void InitMaskBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO* bmpinfo)
{
    bmpinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo->bmiHeader.biWidth = width;
    bmpinfo->bmiHeader.biHeight = top2bottom ? -height : height;
    bmpinfo->bmiHeader.biPlanes = 1;
    bmpinfo->bmiHeader.biBitCount = bit_count;
    bmpinfo->bmiHeader.biCompression = BI_RGB;
    bmpinfo->bmiHeader.biSizeImage = (width * height * bit_count) / 8;
}

void InitColorBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO& bmi)
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


uint8_t mask_bits_global[] =
{
    0xff, 0xff, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff,
    0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff,
    0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff,
    0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff,
    0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

uint32_t color_bits_global[] =
{
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff, 0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xffffffff, 0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xff000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xffffffff, 0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
    0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,        0x0,
};
