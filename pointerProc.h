#ifndef POINTER_PROC_H
#define POINTER_PROC_H

#include <Windows.h>
#include <memory>
#include <vector>
#include "messageProto.h"

constexpr int kPrimaryMonitorIdx = 0;
constexpr int kAllMonitorIdx = 9;

class PointerEventProc
{
public:
	PointerEventProc() {};
	~PointerEventProc() {};

	bool Init();
	bool GetMonitorsInfo(); // 获得被控端显示器的信息
	void ExcutePointerEvent(rfbPointerEventMsg pe);
	bool ChooseMonitor(int monitor_idx); // 根据要传输的显示器画面设置显示器ID，0表示主显示器

private:
	struct MonitorInfo
	{
		int width;
		int height;
		int depth;
		int offsetx;
		int offsety;
	};
	int num_monitor_ = -1;
	int current_monitor_idx_ = -1;
	bool single_monitor_ = true;
	int monitor_offset_x_ = 0;
	int monitor_offset_y_ = 0;
	int screen_width_ = 0;
	int screen_height_ = 0;
	std::vector<MonitorInfo> monitor_list_;
	rfbPointerEventMsg prev_pointer_event_;
};
#endif