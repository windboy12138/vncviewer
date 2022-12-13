#pragma once
#if !defined(DISPL)
#define DISPL
#include <Windows.h>
typedef HRESULT(CALLBACK* P_DwmIsCompositionEnabled) (BOOL* pfEnabled);
typedef HRESULT(CALLBACK* P_DwmEnableComposition) (BOOL   fEnable);

class OSVersion
{
public:
	static OSVersion* getInstance() {

		return (!_OSVersion) ?
			_OSVersion = new OSVersion :
			_OSVersion;
	}
	static void releaseInstance() {
		if (_OSVersion)
			delete _OSVersion;
	}

	OSVersion();
	virtual ~OSVersion();
	void SetAeroState();
	bool CaptureAlphaBlending();
	void DisableAero(VOID);
	void ResetAero(VOID);
	bool OS_WIN10;
	bool OS_WIN10_TRANS;
	bool OS_WIN8;
	bool OS_WIN7;
	bool OS_VISTA;
	bool OS_XP;
	bool OS_W2K;
	bool OS_AERO_ON;
	bool OS_LAYER_ON;
	bool OS_NOTSUPPORTED;
	bool OS_BEFOREVISTA;
	bool OS_MINIMUMVISTA;
	bool AeroWasEnabled;
	bool isWINPE(VOID);
	void removeAlpha();
	bool OS_WINPE;
protected:
	HMODULE DMdll;
	void UnloadDM(VOID);
	bool LoadDM(VOID);
	P_DwmIsCompositionEnabled pfnDwmIsCompositionEnabled;
	P_DwmEnableComposition pfnDwmEnableComposition;
	static OSVersion* _OSVersion;
};
#endif