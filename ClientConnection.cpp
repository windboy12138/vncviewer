#include "ClientConnection.h"
#include <WinUser.h>
#include <iostream>
#include <queue>

#pragma comment(lib, "imm32.lib") // 为了控制输入法
extern std::queue<rfbPointerEventMsg> pointerQ;
extern std::queue<rfbKeyEventMsg> keyQ;

ClientControlCaptureImpl* control_capture_self_ = nullptr;
HHOOK mouse_hook_ = nullptr;
HHOOK keyboard_hook_ = nullptr;
HIMC hIMC_ = nullptr;

ClientControlCaptureImpl::ClientControlCaptureImpl()
	: key_map_(new KeyMap(this))
{
	control_capture_self_ = this;
	key_map_->Reset();
	rect_client_ = RECT {30, 100, 570, 1060};
}

ClientControlCaptureImpl::~ClientControlCaptureImpl()
{
	control_capture_self_ = nullptr;
	if (mouse_hook_ != nullptr)
	{
		UnintsallMouseHook();
	}
	if (keyboard_hook_ != nullptr)
	{
		UninstallKeyBoardHook();
	}
}

void ClientControlCaptureImpl::SetCaptureWindow(HWND hwnd)
{
	cur_capture_wnd_ = hwnd;
}

bool ClientControlCaptureImpl::InstallMouseHook()
{
	DWORD thread_id;
	DWORD process_id;
	thread_id = GetWindowThreadProcessId(cur_capture_wnd_, &process_id);
	mouse_hook_ = SetWindowsHookEx(WH_MOUSE, MouseProc, NULL, thread_id);

	if (mouse_hook_ != nullptr)
	{
		return true;
	}

	return false;
}

bool ClientControlCaptureImpl::InstallKeyBoardHook()
{
	DWORD thread_id;
	DWORD process_id;
	thread_id = GetWindowThreadProcessId(cur_capture_wnd_, &process_id);
	keyboard_hook_ = SetWindowsHookEx(WH_KEYBOARD, KeyBoardProc, NULL, thread_id);

	hIMC_ = ImmAssociateContext(cur_capture_wnd_, nullptr); // 窗口禁用输入法

	if (keyboard_hook_ != nullptr && hIMC_ != nullptr)
	{
		return true;
	}

	return false;
}

bool ClientControlCaptureImpl::UnintsallMouseHook()
{
	if (mouse_hook_ != nullptr)
	{
		UnhookWindowsHookEx(mouse_hook_);
		return true;
	}

	return false;
}

bool ClientControlCaptureImpl::UninstallKeyBoardHook()
{
	if (keyboard_hook_ != nullptr)
	{
		UnhookWindowsHookEx(keyboard_hook_);
		if (hIMC_ != nullptr)
		{
			ImmAssociateContext(cur_capture_wnd_, hIMC_); // 窗口启用输入法
			return true;
		}
	}

	return false;
}

LRESULT CALLBACK ClientControlCaptureImpl::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0 || control_capture_self_->cur_capture_wnd_ != GetFocus())
	{
		return CallNextHookEx(mouse_hook_, nCode, wParam, lParam);
	}

	return control_capture_self_->ProcessPointerEventMsg(nCode, wParam, lParam);
}

LRESULT ClientControlCaptureImpl::ProcessPointerEventMsg(int nCode, WPARAM wParam, LPARAM lParam)
{
	DWORD state_mask = 0;
	POINT pt_new;
	GetCursorPos(&pt_new);
	ScreenToClient(cur_capture_wnd_, &pt_new);

	// 如果不在有效显示区域内，不处理鼠标消息，传递给下一个hook
	// 必须返回CallNextHookEx，否则鼠标无法通过点击右上角关闭按钮退出程序
	if (pt_new.x < rect_client_.left || pt_new.y < rect_client_.top || pt_new.x > rect_client_.right || pt_new.y > rect_client_.bottom) 
	{
		return CallNextHookEx(mouse_hook_, nCode, wParam, lParam);
	}

	UINT iMsg = wParam;
	MOUSEHOOKSTRUCTEX* pMouseStruct = (MOUSEHOOKSTRUCTEX*)lParam;

	if (wParam == WM_MOUSEWHEEL) {
		DWORD data = pMouseStruct->mouseData;
		printf_s("mouseWheel:%d ", (SHORT)HIWORD(data));
		ProcessMouseWheel((SHORT)HIWORD(data));
		return 1;
	}

	switch (wParam)
	{
	case WM_LBUTTONDOWN:
	{
		state_mask |= MK_LBUTTON;
	}
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	{
		if (wParam == WM_MBUTTONDOWN)
			state_mask |= MK_MBUTTON;
	}
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	{
		if (wParam == WM_RBUTTONDOWN)
			state_mask |= MK_RBUTTON;
	}
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	{
		if (wParam == WM_MOUSEMOVE) {
			SHORT mask = GetKeyState(MK_LBUTTON);
			SHORT mask1 = GetKeyState(MK_RBUTTON);
			// printf_s("maskL:%d, maskR:%d ", HIBYTE(mask), HIBYTE(mask1));

			if (HIBYTE(GetKeyState(MK_LBUTTON)))
				state_mask |= MK_LBUTTON;
			if (HIBYTE(GetKeyState(MK_RBUTTON)))
				state_mask |= MK_RBUTTON;
			if (HIBYTE(GetKeyState(VK_MBUTTON)))
				state_mask |= MK_MBUTTON;
		}
		int x = pt_new.x - rect_client_.left;
		int y = pt_new.y - rect_client_.top;
		printf_s("current mouse position: (%3d, %3d), buttonMask: %d\n", x, y, state_mask);
		ProcessPointerEvent(x, y, state_mask, iMsg);
		return 1; // capture the mouse message, not deliver to next hook
	}
	default:
		break;
	}

	return CallNextHookEx(mouse_hook_, nCode, wParam, lParam);
}

bool ClientControlCaptureImpl::ProcessPointerEvent(int x, int y, DWORD keyflags, UINT msg)
{
	if (msg == WM_MOUSEMOVE) 
	{
		if (prev_pointer_x_ != x || prev_pointer_y_ != y || prev_key_flags_ != keyflags) 
		{
			prev_pointer_x_ = x;
			prev_pointer_y_ = y;
			prev_key_flags_ = keyflags;
		}
		else
		{
			if (block_same_mouse_)
				return false;
		}
	}

	SubProcessPointerEvent(x, y, keyflags);

	return true;
}

void ClientControlCaptureImpl::SubProcessPointerEvent(int x, int y, DWORD keyflags)
{
	int mask;

	mask = (((keyflags & MK_LBUTTON) ? rfbButton1Mask : 0) |
		   ((keyflags & MK_MBUTTON) ? rfbButton2Mask : 0) |
		   ((keyflags & MK_RBUTTON) ? rfbButton3Mask : 0));

	if ((short)HIWORD(keyflags) > 0) 
	{
		mask |= rfbButton4Mask;
	}
	else if ((short)HIWORD(keyflags) < 0) 
	{
		mask |= rfbButton5Mask;
	}

	try
	{
		SendPointerEvent(x, y, mask);

		if ((short)HIWORD(keyflags) != 0) 
		{
			// Immediately send a "button-up" after mouse wheel event.
			mask &= !(rfbButton4Mask | rfbButton5Mask);
			SendPointerEvent(x, y, mask);
		}
	}
	catch (char* str) 
	{
		std::cout << str << std::endl;
	}
}

void ClientControlCaptureImpl::ProcessMouseWheel(int delta)
{
	int wheelMask = rfbWheelUpMask;
	if (delta < 0)
	{
		wheelMask = rfbWheelDownMask;
		delta = -delta;
	}

	while (delta > 0)
	{
		SendPointerEvent(prev_pointer_x_, prev_pointer_y_, prev_button_mask_ | wheelMask);
		SendPointerEvent(prev_pointer_x_, prev_pointer_y_, prev_button_mask_ & ~wheelMask);
		delta -= 120;
	}
}

void ClientControlCaptureImpl::SendPointerEvent(int x, int y, int buttonMask)
{
	prev_button_mask_ = buttonMask;

	rfbPointerEventMsg pe;
	pe.type = rfbPointerEvent;
	pe.buttonMask = buttonMask;

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	pe.x = x;
	pe.y = y;
	pointerQ.push(pe);

	// WriteExactQueue_timeout((char*)&pe, sz_rfbPointerEventMsg, rfbPointerEvent, 5); // sf@2002 - For DSM Plugin
}

LRESULT CALLBACK ClientControlCaptureImpl::KeyBoardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
	{
		return CallNextHookEx(keyboard_hook_, nCode, wParam, lParam);
	}

	control_capture_self_->ProcessKeyEventMsg((int)wParam, (DWORD)lParam);
	return 1;
}

void ClientControlCaptureImpl::ProcessKeyEventMsg(int virtKey, DWORD keyData)
{
	// if virtkey found in mapping table, send X equivalent
	// else
	//   try to convert directly to ascii
	//   if result is in range supported by X keysyms,
	//      raise any modifiers, send it, then restore mods
	//   else
	//      calculate what the ascii would be without mods
	//      send that

	try
	{
		key_map_->ProcessKeyEvent(virtKey, keyData);
	}
	catch (char* str)
	{
		std::cout << str << std::endl;
	}
}

void ClientControlCaptureImpl::SendKeyEvent(uint32_t key, bool down)
{
	rfbKeyEventMsg ke;
	memset(&ke, 0, sizeof(ke));

	ke.type = rfbKeyEvent;
	ke.down = down ? 1 : 0;
	ke.key = key;
	printf_s("KeyEvent, keyCode:%3d, isDown:%d \n", key, down);
	keyQ.push(ke);
	// WriteExactQueue_timeout((char*)&ke, sz_rfbKeyEventMsg, rfbKeyEvent, 5);
}