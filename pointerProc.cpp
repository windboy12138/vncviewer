#include "pointerProc.h"
#include <iostream>

bool PointerEventProc::Init()
{
	ZeroMemory(&prev_pointer_event_, sizeof(prev_pointer_event_));
	monitor_list_.resize(kAllMonitorIdx + 1);

	if (GetMonitorsInfo())
	{
		single_monitor_ = num_monitor_ > 1 ? false : true;
		current_monitor_idx_ = kPrimaryMonitorIdx;
		ChooseMonitor(current_monitor_idx_);
		printf_s("current monitor idx %d, monitor offset (%d, %d), screen size (%d, %d)\n", current_monitor_idx_,
			monitor_offset_x_, monitor_offset_y_, screen_width_, screen_height_);
		return true;
	}

	return false;
}

bool PointerEventProc::GetMonitorsInfo()
{
	num_monitor_ = 0;
	int nr = 1;
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);

	for (int i = 0; EnumDisplayDevices(NULL, i, &dd, 0); i++)
	{
		if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) && !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
		{
			DEVMODE devMode;
			devMode.dmSize = sizeof(DEVMODE);
			EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &devMode);
			num_monitor_++;

			if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
			{
				monitor_list_[kPrimaryMonitorIdx].offsetx = devMode.dmPosition.x;
				monitor_list_[kPrimaryMonitorIdx].offsety = devMode.dmPosition.y;
				monitor_list_[kPrimaryMonitorIdx].width = devMode.dmPelsWidth;
				monitor_list_[kPrimaryMonitorIdx].height = devMode.dmPelsHeight;
				monitor_list_[kPrimaryMonitorIdx].depth = devMode.dmBitsPerPel;
			}
			else
			{
				monitor_list_[nr].offsetx = devMode.dmPosition.x;
				monitor_list_[nr].offsety = devMode.dmPosition.y;
				monitor_list_[nr].width = devMode.dmPelsWidth;
				monitor_list_[nr].height = devMode.dmPelsHeight;
				monitor_list_[nr].depth = devMode.dmBitsPerPel;
				nr++;
			}
		}
	}

	monitor_list_[kAllMonitorIdx].offsetx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	monitor_list_[kAllMonitorIdx].offsety = GetSystemMetrics(SM_YVIRTUALSCREEN);
	monitor_list_[kAllMonitorIdx].width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	monitor_list_[kAllMonitorIdx].height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	monitor_list_[kAllMonitorIdx].depth = monitor_list_[kPrimaryMonitorIdx].depth; //depth primary monitor is used

	if (num_monitor_ == 0)
	{
		return false;
	}

	return true;
}

void PointerEventProc::ExcutePointerEvent(rfbPointerEventMsg pe)
{

	DWORD flags = MOUSEEVENTF_ABSOLUTE;
	if (pe.x != prev_pointer_event_.x || pe.y != prev_pointer_event_.y)
	{
		flags |= MOUSEEVENTF_MOVE;
	}

	if ( (pe.buttonMask & rfbButton1Mask) != (prev_pointer_event_.buttonMask & rfbButton1Mask) )
	{
		if ( GetSystemMetrics(SM_SWAPBUTTON) )
			flags |= (pe.buttonMask & rfbButton1Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
		else
			flags |= (pe.buttonMask & rfbButton1Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
	}

	if ( (pe.buttonMask & rfbButton2Mask) != (prev_pointer_event_.buttonMask & rfbButton2Mask) )
	{
		flags |= (pe.buttonMask & rfbButton2Mask) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
	}

	if ( (pe.buttonMask & rfbButton3Mask) != (prev_pointer_event_.buttonMask & rfbButton3Mask) )
	{
		if (GetSystemMetrics(SM_SWAPBUTTON))
			flags |= (pe.buttonMask & rfbButton3Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
		else
			flags |= (pe.buttonMask & rfbButton3Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
	}

	DWORD wheel_movement = 0;
	if (pe.buttonMask & rfbWheelUpMask) 
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheel_movement = WHEEL_DELTA;
	}
	if (pe.buttonMask & rfbWheelDownMask) 
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheel_movement = -WHEEL_DELTA;
	}

	if (single_monitor_)
	{
		unsigned long x = ((pe.x + (monitor_offset_x_)) * 65535) / (screen_width_ - 1);
		unsigned long y = ((pe.y + (monitor_offset_y_)) * 65535) / (screen_height_ - 1);
		::mouse_event(flags, (DWORD)x, (DWORD)y, wheel_movement, 0);
		
		printf_s("mouse event (%d, %d)\n", pe.x, pe.y);
	}
	else
	{
		INPUT evt;
		evt.type = INPUT_MOUSE;
		int xx = pe.x - GetSystemMetrics(SM_XVIRTUALSCREEN) + (monitor_offset_x_ + 0);
		int yy = pe.y - GetSystemMetrics(SM_YVIRTUALSCREEN) + (monitor_offset_y_ + 0);
		
		evt.mi.dx = (xx * 65535) / (GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1);
		evt.mi.dy = (yy * 65535) / (GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1);
		evt.mi.dwFlags = flags | MOUSEEVENTF_VIRTUALDESK;
		evt.mi.dwExtraInfo = 0;
		evt.mi.mouseData = wheel_movement;
		evt.mi.time = 0;
		
		printf_s("recv mouse event:(%3d, %3d), mask:%d\n", pe.x, pe.y, pe.buttonMask);
		SendInput(1, &evt, sizeof(evt));
	}
	
	// save the last point event
	prev_pointer_event_ = pe;
}

bool PointerEventProc::ChooseMonitor(int monitor_idx)
{
	if (monitor_idx > num_monitor_ || monitor_idx < 0)
	{
		return false;
	}
	
	current_monitor_idx_ = monitor_idx;
	monitor_offset_x_ = monitor_list_[monitor_idx].offsetx;
	monitor_offset_y_ = monitor_list_[monitor_idx].offsety;
	screen_width_ = monitor_list_[monitor_idx].width;
	screen_height_ = monitor_list_[monitor_idx].height;
	return true;
}
