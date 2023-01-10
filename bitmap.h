#pragma once
#include <Windows.h>
#include <dwmapi.h>
#include <thread>
#include "common_utils.h"
extern HWND hWndChild;

void InitMaskBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO* bmpinfo);
void InitColorBitMap(int width, int height, bool top2bottom, int bit_count, BITMAPINFO& bmi);
void print_mask_bitmap(uint8_t* mask_bits);

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

HBITMAP IconToBitmap(HICON hIcon, SIZE* pTargetSize = NULL)
{
    ICONINFO info = { 0 };
    if (hIcon == NULL
        || !GetIconInfo(hIcon, &info)
        //|| !info.fIcon)
        )
    {
        return NULL;
    }

    INT nWidth = 0;
    INT nHeight = 0;
    if (pTargetSize != NULL)
    {
        nWidth = pTargetSize->cx;
        nHeight = pTargetSize->cy;
    }
    else
    {
        if (info.hbmMask != NULL)
        {
            BITMAP bmp = { 0 };
            GetObject(info.hbmMask, sizeof(bmp), &bmp);

            nWidth = bmp.bmWidth;
            nHeight = bmp.bmHeight;
            printf_s("have mask, (w, h) (%d, %d)\n", nWidth, nHeight);
        }
    }

    {
        HDC wnd = GetWindowDC(hWndChild);

        HDC bmp, maskbmp;
        bmp = CreateCompatibleDC(NULL);
        SelectObject(bmp, info.hbmColor);
        
        maskbmp = CreateCompatibleDC(NULL);
        SelectObject(maskbmp, info.hbmMask);

        uint8_t* lpvBits = NULL;
        int nRet = 2;
        BITMAP bm;
        GetObject(info.hbmMask, sizeof(bm), &bm);
        int ncolors = 1 << bm.bmBitsPixel;
        int bmpinfo_size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * ncolors;
        std::vector<uint8_t> buf(bmpinfo_size);
        BITMAPINFO* bmpinfo = (BITMAPINFO*)buf.data();
        bmpinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        nRet = ::GetDIBits(maskbmp, info.hbmMask, 0, 0, NULL, bmpinfo, DIB_RGB_COLORS);

        lpvBits = new uint8_t[bmpinfo->bmiHeader.biSizeImage];

        nRet = ::GetDIBits(maskbmp, info.hbmMask, 0, nHeight, lpvBits, bmpinfo, DIB_RGB_COLORS);

        printf_s("bitmap mask datas \n");
        for (int i = 0; i < bmpinfo->bmiHeader.biSizeImage; i++)
        {
            if (i % 4 == 0) {
                printf_s("\n");
            }
            uint8_t tmp = lpvBits[i];
            for (int j = 0; j < 8; j++)
            {
                bool res = tmp & 0x80;
                tmp = tmp << 1;
                printf_s("%d", res);
            }
        }
        printf_s("\n");


        BitBlt(wnd, 100, 100, 32, 32, maskbmp, 0, 0, SRCAND);
        BitBlt(wnd, 100, 100, 32, 32, bmp, 0, 0, SRCINVERT);
        // MaskBlt(wnd, 100, 100, 32, 32, bmp, 0, 0, info.hbmMask, 0, 0, 0xccaa0000);

        DeleteDC(bmp);
        DeleteDC(maskbmp);
        ReleaseDC(hWndChild, wnd);
    }

    if (info.hbmColor != NULL)
    {
        DeleteObject(info.hbmColor);
        info.hbmColor = NULL;
    }

    if (info.hbmMask != NULL)
    {
        DeleteObject(info.hbmMask);
        info.hbmMask = NULL;
    }

    if (nWidth <= 0
        || nHeight <= 0)
    {
        return NULL;
    }

    INT nPixelCount = nWidth * nHeight;

    HDC dc = GetDC(NULL);
    INT* pData = NULL;
    HDC dcMem = NULL;
    HBITMAP hBmpOld = NULL;
    bool* pOpaque = NULL;
    HBITMAP dib = NULL;
    BOOL bSuccess = FALSE;

    do
    {
        BITMAPINFOHEADER bi = { 0 };
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = nWidth;
        bi.biHeight = -nHeight;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        dib = CreateDIBSection(dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (VOID**)&pData, NULL, 0);
        if (dib == NULL) break;

        memset(pData, 0, nPixelCount * 4);

        dcMem = CreateCompatibleDC(dc);
        if (dcMem == NULL) break;

        hBmpOld = (HBITMAP)SelectObject(dcMem, dib);
        ::DrawIconEx(dcMem, 0, 0, hIcon, nWidth, nHeight, 0, NULL, DI_MASK);

        pOpaque = new(std::nothrow) bool[nPixelCount];
        if (pOpaque == NULL) break;
        for (INT i = 0; i < nPixelCount; ++i)
        {
            pOpaque[i] = !pData[i];
        }

        memset(pData, 0, nPixelCount * 4);
        ::DrawIconEx(dcMem, 0, 0, hIcon, nWidth, nHeight, 0, NULL, DI_NORMAL);

        BOOL bPixelHasAlpha = FALSE;
        UINT* pPixel = (UINT*)pData;

        for (INT i = 0; i < nPixelCount; ++i)
        {
            if (i % 32 == 0)
            {
                printf_s("\n");
            }
            printf_s("%8x ", pData[i]);
        }

        for (INT i = 0; i < nPixelCount; ++i, ++pPixel)
        {
            if ((*pPixel & 0xff000000) != 0)
            {
                bPixelHasAlpha = TRUE;
                break;
            }
        }



        if (!bPixelHasAlpha)
        {
            pPixel = (UINT*)pData;
            for (INT i = 0; i < nPixelCount; ++i, ++pPixel)
            {
                if (pOpaque[i])
                {
                    *pPixel |= 0xFF000000;
                }
                else
                {
                    *pPixel &= 0x00FFFFFF;
                }
            }
        }

        bSuccess = TRUE;

    } while (FALSE);


    if (pOpaque != NULL)
    {
        delete[]pOpaque;
        pOpaque = NULL;
    }

    if (dcMem != NULL)
    {
        SelectObject(dcMem, hBmpOld);
        DeleteDC(dcMem);
    }

    ReleaseDC(NULL, dc);

    if (!bSuccess)
    {
        if (dib != NULL)
        {
            DeleteObject(dib);
            dib = NULL;
        }
    }

    return dib;
}

void cursorToBitmap(CURSORINFO &ci)
{
    HDC hdc = GetDC(NULL);

    HDC mem_dc = CreateCompatibleDC(hdc);

    HBITMAP win_bitmap = CreateCompatibleBitmap(hdc, 32, 32);
    // SelectObject(mem_dc, win_bitmap);
    SetStretchBltMode(hdc, STRETCH_HALFTONE);

    HICON hIcon = CopyIcon(ci.hCursor);
    if (!hIcon)
    {
        return;
    }

    ICONINFO ii;
    if (!GetIconInfo(hIcon, &ii))
    {
        DestroyIcon(hIcon);
        return;
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);    
    bmi.bmiHeader.biWidth = 32;
    bmi.bmiHeader.biHeight = -32;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 32 * 32 / 8;
    memset(bmi.bmiColors, 0, 2 * sizeof(RGBQUAD));

    uint8_t* image_data = nullptr;
    image_data = new(std::nothrow) uint8_t[bmi.bmiHeader.biSizeImage];

    if (image_data != nullptr)
        memset(image_data, 0, bmi.bmiHeader.biSizeImage);

    if (::GetDIBits(mem_dc, ii.hbmMask, 0, 32, image_data, &bmi, DIB_RGB_COLORS))
    {
        for (int i = 0; i < bmi.bmiHeader.biSizeImage; i++)
        {
            if (i % 4 == 0) {
                printf_s("\n");
            }
            uint8_t tmp = image_data[i];
            for (int j = 0; j < 8; j++)
            {
                bool res = tmp & 0x80;
                tmp = tmp << 1;
                printf_s("%d", res);
            }
        }
        printf_s("\n");

        //for (INT i = 0; i < bmi.bmiHeader.biSizeImage / 4; ++i)
        //{
        //    if (i % 32 == 0)
        //    {
        //        printf_s("\n");
        //    }
        //    printf_s("%8x ", image_data[i]);
        //}

        // to-do
        //std::unique_ptr<DibImage> dib_image(new DibImage(&bmi, image_data));
        //dib_image->Save(L"test.bmp");
    }

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
    DestroyIcon(hIcon);
    ReleaseDC(NULL, hdc);
    DeleteDC(mem_dc);
    DeleteObject(win_bitmap);
}

int captureCursorIcon(HICON &hIcon, uint8_t* &mask_bits, uint8_t* &color_bits, HBITMAP &cursor_bmp)
{
    ICONINFO info = { 0 };
    if (!GetIconInfo(hIcon, &info))
    {
        DestroyIcon(hIcon);
        return -1;
    }

    if (info.hbmMask == NULL)
    {
        return -1;
    }

    BITMAP bm = { 0 };
    GetObject(info.hbmMask, sizeof(bm), &bm);
    INT nWidth = bm.bmWidth;
    INT nHeight = bm.bmHeight;

    if (nHeight < 0 || nHeight % 2 != 0)
    {
        return -1;
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

        print_mask_bitmap(mask_bits);
        printf_s("generate mask is:%d\n", generate_mask);
        Sleep(1000);
        
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
    }
    else 
    {
        // not support format
        return -1;
    }

    HCURSOR h1 = NULL;
    HBITMAP mask_bmp = CreateBitmap(nWidth, 32, 1, 1, mask_bits);
    HBITMAP color_bmp;
    if (nHeight == 64)
    {
        color_bmp = CreateBitmap(nWidth, 32, 1, 1, color_bits);
        h1 = CreateCursor(NULL, info.xHotspot, info.yHotspot, 32, 32, mask_bits, color_bits);
    }
    else
    {
        color_bmp = CreateBitmap(nWidth, 32, 1, 32, color_bits);
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
    BitBlt(wnd, 100, 100, 32, 32, mask_dc, 0, 0, SRCAND);
    BitBlt(wnd, 100, 100, 32, 32, color_dc, 0, 0, SRCINVERT);
    // MaskBlt(wnd, 100, 100, 32, 32, color_dc, 0, 0, mask_bmp, 0, 0, 0xccaa0000);
    DeleteDC(color_dc);
    DeleteDC(mask_dc);
    ReleaseDC(hWndChild, wnd);
    
    if (h1 != NULL)
    {
        SetClassLongPtr(hWndChild, GCLP_HCURSOR, (LONG_PTR)(h1));
        PostMessage(hWndChild, WM_SETCURSOR, (UINT)(hWndChild), 33554433);
        DestroyCursor(h1);
    }
    
    DeleteObject(info.hbmMask);
    DeleteObject(info.hbmColor);

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

void print_mask_bitmap(uint8_t *mask_bits)
{
    printf_s("bitmap mask datas \n");
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

void print_bitmap()
{
    int nPixelCount = 32 * 32;
    uint8_t *mask_bits = new uint8_t[nPixelCount / 8];
    

    uint8_t* colorBits = new uint8_t[nPixelCount / 8];
    printf_s("\nXOR mask");
    for (int i = 0; i < nPixelCount / 8; i++)
    {
        if (i % 4 == 0) {
            printf_s("\n");
        }
        uint8_t tmp = colorBits[i];
        for (int j = 0; j < 8; j++)
        {
            bool res = tmp & 0x80;
            tmp = tmp << 1;
            printf_s("%d", res);
        }
    }
    printf_s("\n");
}


