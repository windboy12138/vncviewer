#pragma once
#include <iostream>
#include <Windows.h>
#include <mutex>

std::mutex clip_mutex;
extern HWND message_wnd_;

bool ProcessLocalClipboardChange();
bool UpdateLocalClipboard();


bool ProcessLocalClipboardChange()
{
    std::lock_guard<std::mutex> auto_lock(clip_mutex);
    printf_s("Clipboard data updated!\n");
    HANDLE clip;
    std::string clip_text = "";

    if (!IsClipboardFormatAvailable(CF_TEXT))
    {
        printf_s("Not text data\n");
        return false;
    }

    if (!OpenClipboard(message_wnd_))
    {
        printf_s("Can't open clipboard\n");
        return false;
    }
    clip = GetClipboardData(CF_TEXT);
    clip_text = (char*)clip;
    printf_s("data: %s \n", (char*)clip);

    CloseClipboard();
    return true;
}

bool UpdateLocalClipboard()
{
    std::lock_guard<std::mutex> auto_lock(clip_mutex);
    if (!OpenClipboard(message_wnd_))
    {
        printf_s("Can't open clipboard\n");
        return false;
    }

    if (!::EmptyClipboard())
    {
        CloseClipboard();
        return false;
    }
    char text[] = "SetClipboardData";
    int len = strlen(text) + 1;
    HGLOBAL hglb_copy = GlobalAlloc(GMEM_DDESHARE, len);
    if (hglb_copy != NULL)
    {
        LPSTR str_copy = (LPSTR)GlobalLock(hglb_copy);
        if (str_copy != NULL)
        {
            memcpy(str_copy, text, len);
            str_copy[len - 1] = 0;
            GlobalUnlock(hglb_copy);

            SetClipboardData(CF_TEXT, hglb_copy);
        }
    }
    CloseClipboard();
    return true;
}