#pragma once 
#include <string>
#include <CVector2D.h>
#include <CMenuManager.h>
#include <SpriteLoader.h>
#include <CCutsceneMgr.h>

#include <CSprite2d.h>
#include <CCamera.h>
#include <Audio.h>
#include <CFont.h>
#include <CWorld.h>

#include "IniReader/IniReader.h"
#include "InputHelper.h"
#include "blur.h"

#define IS_CUTSCENE_RUNNING CCutsceneMgr::ms_running
#define IS_ON_SCRIPTED_CUTSCENE !IS_CUTSCENE_RUNNING && TheCamera.m_bWideScreenOn
#define IS_ON_ANY_CUTSCENE CCutsceneMgr::ms_running || TheCamera.m_bWideScreenOn

using namespace plugin;
using namespace std;

enum eCutscenePauseMethod {
	USER_PAUSE = 1,
	CODE_PAUSE
};

enum eCutsceneLoadStatus {
	STATUS_LOADING = 1,
	STATUS_LOADED
};

class CutsceneController {
public:
	// interface 
	bool bShowInterface;
	bool bShowDebugInterface;
	bool bShowCamSpeedText;
	CSprite2d vig;

	BassSampleManager* audioMgr;

	// cam settings
	unsigned int toggleCamKey;
	unsigned int frontKey;
	unsigned int backKey;
	float camSensi;
	float camSpeed;
	bool bFixedCam = false;
	CVector camPos;
	CVector lastFixedCamPos;
	CVector lastFixedCamAngle;

	eCutscenePauseMethod pauseMethod;
	ULONGLONG m_nLastPausedTime = NULL;
	ULONGLONG m_nLastDebugTime = NULL;
	
	// ini props 
	unsigned int pauseKey;
	unsigned int debugKey;
	unsigned int disableBlurInGameKey;

	GamepadButton buttonPause;
	GamepadButton buttonDebug;
	bool bUseMouseToSkip;
	bool bSkipInPause;

	// effects
	GaussianBlur* gsBlur;
	bool bVignette;
	bool bBlur;
	bool bUsePixelShader;
	unsigned char vignetteAlpha;
	unsigned char blurAlpha;
	float fBlurIntensity;
	
	// text //
	string pauseText;
	eFontStyle pauseTextFont;

	CVector2D pauseTextVec;
	float pauseTextSizeX;
	float pauseTextSizeY;
	CRGBA pauseTextColor;
	CRGBA pauseTextDropColor;
	short pauseTextBorder;
	short pauseTextDropPos;
	bool bSetShadow;
	
	// audio
	uint32_t pauseAudioId;
	bool bPauseAudioExist;
	uint8_t pauseAudioVol;
	uint32_t pauseAudioFreq;

	uint32_t resumeAudioId;
	bool bResumeAudioExist;
	uint8_t resumeAudioVol;
	uint32_t resumeAudioFreq;

	CutsceneController();

	inline void PointCameraAtPoint(CVector* pos, eSwitchType switchType) {
		if (pos->z <= -100.0f) pos->z = CWorld::FindGroundZForCoord(pos->x, pos->y);
		TheCamera.TakeControlNoEntity(pos, switchType, eSwitchType::SWITCHTYPE_INTERPOLATION);
	}

	inline void ChangeCutscenePause() {
		if (pauseMethod == eCutscenePauseMethod::CODE_PAUSE) CodePauseMethod();
		else UserPauseMethod();
	}

	inline void SetUserPatchState(bool bState) {
		if (bState) {
			patch::Nop(0x58FCC2, 4); // 0x58FCC2(2) 0x58FCC4(2)
			patch::Nop(0x58D4BE, 8); // 0x58D4BE(2) 0x58D4C0(6)
			patch::Nop(0x5B17CC, 6); // nop in ++CCutsceneMgr::ms_cutsceneTimer	
		}
		else {
			patch::NopRestore(0x58FCC2);
			patch::NopRestore(0x58D4BE);
			patch::NopRestore(0x5B17CC);
		}
	}

	// It checks if the left mouse button was clicked, but only if it was enabled in the.ini file
	inline bool IsMouseLeftButtonPressed() {
		if (bUseMouseToSkip) return CPad::NewMouseControllerState.lmb && !CPad::OldMouseControllerState.lmb;
		else return false;
	}

	inline bool IsPauseButtonPressed(CPad* pad) {
		if (IsButtonPressed(buttonPause, pad->NewState) && !IsButtonPressed(buttonPause, pad->OldState))
			return true;
		if (KeyPressed(pauseKey))
			return true;
		return false;
	}

	inline bool IsDebugButtonPressed(CPad* pad) {
		if (IsButtonPressed(buttonDebug, pad->NewState) && !IsButtonPressed(buttonDebug, pad->OldState))
			return true;
		if (KeyPressed(debugKey))
			return true;
		return false;
	}

	bool LoadAudio();
	void Update(); // Game process event callback

	// It requires memory patches to display messages on the screen; m_UserPause is used by the game's menu system and other things, and may be more reliable than m_CodePause
	void UserPauseMethod();
	void CodePauseMethod();

	// Interface 
	void DrawInterface();
	void DrawDebugInterface();

	void ProcessFreeCamera();
	void UpdateFreeCamera(float& posX, float& posY, float& posZ, float& angX, float& angY);

	void ReadIniOptions();

	static bool bUseSkipGameKeys;
	static std::list<unsigned int> skipKeys;
	static std::list<GamepadButton> skipButtons;

	static bool bCutscenePaused;
	static SpriteLoader sprite_loader;

	static bool __cdecl Hook_IsCutsceneSkipButtonBeingPressed();

};

extern CutsceneController inst;