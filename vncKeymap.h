#pragma once

class vncKeymap {
public:
	static void KeyEvent(unsigned int keysym, bool down, bool jap, bool unicode);
	static void ClearShiftKeys();
};