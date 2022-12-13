#include "multiMonitor.h"
#include <Windows.h>
#include <iostream>

bool MonitorInfo::GetMonitorsInfo()
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

bool MonitorInfo::Init()
{
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

bool MonitorInfo::ChooseMonitor(int monitor_idx)
{
	if (monitor_idx >= num_monitor_ || num_monitor_ == 0)
	{
		return false;
	}
	else
	{
		current_monitor_idx_ = monitor_idx;
		monitor_offset_x_ = monitor_list_[monitor_idx].offsetx;
		monitor_offset_y_ = monitor_list_[monitor_idx].offsety;
		screen_width_ = monitor_list_[monitor_idx].width;
		screen_height_ = monitor_list_[monitor_idx].height;
		return true;
	}
}
