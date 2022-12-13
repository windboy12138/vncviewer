#pragma once
#ifndef _G_UIACCESS_H_
#define _G_UIACCESS_H_

#include <windows.h>
#include <sddl.h>
#pragma warning (disable:6387)
#include <shellapi.h>

struct keyEventdata
{
	BYTE bVk;
	BYTE bScan;
	DWORD dwflags;
};

void keybd_uni_event(_In_  BYTE bVk, _In_  BYTE bScan, _In_  DWORD dwFlags, _In_  ULONG_PTR dwExtraInfo);
#endif