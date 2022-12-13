#pragma once
#include <Windows.h>
#include <minwindef.h>
#include <memory>
#include "KeyMap.h"
#include "messageProto.h"

// to-do clean follow code
/* client -> server */

#define rfbSetPixelFormat 0
#define rfbFixColourMapEntries 1	/* not currently supported */
#define rfbSetEncodings 2
#define rfbFramebufferUpdateRequest 3
#define rfbKeyEvent 4
#define rfbPointerEvent 5
#define rfbClientCutText 6
#define rfbFileTransfer 7     // Modif sf@2002 - actually bidirectionnal
#define rfbSetScale 8 // Modif sf@2002
#define rfbSetServerInput	9 // Modif rdv@2002
#define rfbSetSW	10// Modif rdv@2002
#define rfbTextChat	11// Modif sf@2002 - TextChat - Bidirectionnal
#define rfbKeepAlive 13 // 16 July 2008 jdp -- bidirectional
#define rfbPalmVNCSetScaleFactor 0xF // PalmVNC 1.4 & 2.0 SetScale Factor message


class ClientControlCaptureImpl {
public:
	ClientControlCaptureImpl();
	virtual ~ClientControlCaptureImpl();

	void SetCaptureWindow(HWND hwnd);

	bool InstallMouseHook();
	bool InstallKeyBoardHook();

	bool UnintsallMouseHook();
	bool UninstallKeyBoardHook();

	static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK KeyBoardProc(int nCode, WPARAM wParam, LPARAM lParam);

	LRESULT ProcessPointerEventMsg(int nCode, WPARAM wParam, LPARAM lParam);
	bool ProcessPointerEvent(int x, int y, DWORD keyflags, UINT msg);
	void SubProcessPointerEvent(int x, int y, DWORD keyflags);
	void ProcessMouseWheel(int delta);
	void SendPointerEvent(int x, int y, int buttonMask);

	void ProcessKeyEventMsg(int virtKey, DWORD keyData);
	void SendKeyEvent(uint32_t key, bool down);

private:
	RECT rect_client_ = { 0 };
	double scale_factor_x_ = 1.0;
	double scale_factor_y_ = 1.0;
	int prev_pointer_x_ = 0;
	int prev_pointer_y_ = 0;
	DWORD prev_key_flags_ = 0;
	int prev_button_mask_ = 0;
	bool block_same_mouse_ = true;
	HWND cur_capture_wnd_ = nullptr;
	std::unique_ptr<KeyMap> key_map_; // to-do: unique_prt
};