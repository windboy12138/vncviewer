#pragma once
constexpr int kPrimaryMonitorIdx = 0;
constexpr int kAllMonitorIdx = 9;

struct Monitor
{
	int width;
	int height;
	int depth;
	int offsetx;
	int offsety;
};

class MonitorInfo 
{
public:
	bool Init();
	bool GetMonitorsInfo(); // 获得被控端显示器的信息
	bool ChooseMonitor(int monitor_idx); // 根据要传输的显示器画面设置显示器ID，0表示主显示器

public:
	int num_monitor_;
	int current_monitor_idx_;
	bool single_monitor_;
	int monitor_offset_x_;
	int monitor_offset_y_;
	int screen_width_;
	int screen_height_;
	Monitor monitor_list_[10];
};