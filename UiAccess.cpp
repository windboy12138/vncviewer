#include "UiAccess.h"
#include "OSVersion.h"
#include <iostream>

int g_lockcode = 0;
bool g_initialized = false;
int errorcounter = 0;
int preventDeadlock = 0;

void keybd_uni_event(_In_  BYTE bVk, _In_  BYTE bScan, _In_  DWORD dwFlags, _In_  ULONG_PTR dwExtraInfo)
{
	if (!OSVersion::getInstance()->OS_WIN8 || g_lockcode != 0 || !g_initialized) {
		// printf_s("run into keybd_event\n");
		keybd_event(bVk, bScan, dwFlags, dwExtraInfo);
		return;
	}
}