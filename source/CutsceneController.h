#pragma once 
#include <string>
#include <CVector2D.h>
#include <CMenuManager.h>
#include <SpriteLoader.h>
#include <CCutsceneMgr.h>
#include <CSprite2d.h>
#include <CCamera.h>
#include <CFont.h>
#include <CWorld.h>
#include <CRunningScript.h>
#ifdef GTASA
	#include <Audio.h>
#endif 
#include "IniReader/IniReader.h"
#include "InputHelper.h"
#include "DebugOverlay.h"
#include "blur.h"

#define IS_CUTSCENE_RUNNING CCutsceneMgr::ms_running
#define IS_ON_SCRIPTED_CUTSCENE !IS_CUTSCENE_RUNNING && TheCamera.m_bWideScreenOn
#define IS_ON_ANY_CUTSCENE CCutsceneMgr::ms_running || TheCamera.m_bWideScreenOn
#define IS_ON_MISSION CTheScripts::OnAMissionFlag && *(injector::ReadMemory<char*>(0x5D5380, true) + CTheScripts::OnAMissionFlag)

#ifdef GTASA
#define ACTIVE_CAMERA TheCamera.m_aCams[TheCamera.m_nActiveCam]
#define CAM_MODE TheCamera.m_aCams[TheCamera.m_nActiveCam].m_nMode
#define CUTSCENE_POS CCutsceneMgr::ms_cutsceneOffset
#define CUTSCENE_TIMER CCutsceneMgr::ms_cutsceneTimer
#define MISSION_TIMER_PAUSED CUserDisplay::OnscnTimer.m_bPaused
#define SKIP_CUTSCENE CCutsceneMgr::SkipCutscene()
#elif GTAVC
#define ACTIVE_CAMERA TheCamera.m_asCams[TheCamera.m_nActiveCam]
#define CAM_MODE TheCamera.m_asCams[TheCamera.m_nActiveCam].m_nCamMode
#define CUTSCENE_POS ms_cutsceneOffset
#define CUTSCENE_TIMER ms_cutsceneTimer
#define MISSION_TIMER_PAUSED CUserDisplay::OnscnTimer.m_bTimerFeezed
#define SKIP_CUTSCENE SkipCutscene_VC()
#elif GTA3
#define ACTIVE_CAMERA TheCamera.m_asCams[TheCamera.m_nActiveCam]
#define CAM_MODE TheCamera.m_asCams[TheCamera.m_nActiveCam].m_nCamMode
#define CUTSCENE_POS CCutsceneMgr::ms_cutsceneOffset
#define CUTSCENE_TIMER CCutsceneMgr::ms_cutsceneTimer
#define MISSION_TIMER_PAUSED CUserDisplay::OnscnTimer.m_bTimerFeezed
#define SKIP_CUTSCENE CCutsceneMgr::FinishCutscene()
#endif 

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

#if defined(GTAVC) || defined(GTA3) 
	enum eSwitchType : unsigned short 
	{
		SWITCHTYPE_NONE,
		SWITCHTYPE_INTERPOLATION,
		SWITCHTYPE_JUMPCUT
	};
#endif 
#ifdef GTAVC
	// CCutsceneMgr static members(they were not documented in the plugin SDK)
	static float& ms_cutsceneTimer = *(float*)0xA0D9CC;
	static CVector& ms_cutsceneOffset = *(CVector*)0x97F24C;
	static bool& ms_wasCutsceneSkipped = *(bool*)0xA10B37;
	static bool& dataFileLoaded = *(bool*)0x6F7025;
	
	// functions
	typedef void(__thiscall* PauseStreamFn)(void* cSampleM, bool flag, int n);
	inline void PauseStream_VC(void* sampleManager, bool paused, int channel) {
		reinterpret_cast<PauseStreamFn>(0x5D6400)(sampleManager, paused, channel);
	}
#elif GTA3 

	// functions
	typedef void(__thiscall* PauseStreamFn)(void* cSampleM, bool flag, int n);
	inline void PauseStream_III(void* sampleManager, bool paused, int channel) {
		reinterpret_cast<PauseStreamFn>(0x567D30)(sampleManager, paused, channel);
	}

	// The plugin SDK doesn't have this function documented...
	inline void ProcessScripts_III() {
		reinterpret_cast<void(__cdecl*)()>(0x439040)();
	}
#endif

class CutsceneController {
public:
	// interface 
	bool bShowInterface;
	bool bShowDebugInterface;
	bool bShowCamSpeedText;
	bool needBlurCapture = false;
	CSprite2d vig;
	CSprite2d pauseButton;
	
#ifdef GTASA
	BassSampleManager* audioMgr;
#endif
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

	bool bPauseIcon;
	bool bFixAudioDesync;
	CVector2D pauseIconPos;
	float pauseIconSizeX;
	float pauseIconSizeY;

	unsigned int debugKey;
	unsigned int disableBlurInGameKey;

	GamepadButton buttonPause;
	GamepadButton buttonDebug;
	bool bUseMouseToSkip;
	bool bSkipInPause;
	bool bPauseOnlyDuringMissions;
	bool bShowMissionName;
	bool bShowSubtitles;

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
#ifdef GTASA
		TheCamera.TakeControlNoEntity(pos, switchType, eSwitchType::SWITCHTYPE_INTERPOLATION);

#elif defined(GTAVC) || defined(GTA3)
		TheCamera.TakeControlNoEntity(*pos, switchType, eSwitchType::SWITCHTYPE_INTERPOLATION);
#endif 
	}
	
	inline void ChangeCutscenePause() {
#ifdef GTASA
		//if (pauseMethod == eCutscenePauseMethod::CODE_PAUSE) CodePauseMethod();
		//else UserPauseMethod();
		SetGamePaused(!bCutscenePaused, pauseMethod);
#elif GTAVC
		SetGamePaused(!bCutscenePaused, eCutscenePauseMethod::USER_PAUSE);
#elif GTA3
		// In GTA III, changing UserPause during cutscenes partially freezes the audio, whereas CodePause causes issues with scripted cutscenes
		if (IS_CUTSCENE_RUNNING) SetGamePaused(!bCutscenePaused, eCutscenePauseMethod::CODE_PAUSE);
		else SetGamePaused(!bCutscenePaused, eCutscenePauseMethod::USER_PAUSE);
#endif 
	}

	inline void ApplyPausePatches(bool bState) {
#ifdef GTASA 
		if (bState) {
			patch::Nop(0x58FCC2, 4); // 0x58FCC2(2) 0x58FCC4(2)
			patch::Nop(0x58D4BE, 8); // 0x58D4BE(2) 0x58D4C0(6)
			patch::Nop(0x5B17CC, 6); // nop in ++CCutsceneMgr::ms_cutsceneTimer	
			if (!bShowMissionName) patch::Nop(0x58D568, 5); // call _ZN4CHud16DrawMissionTitleEv
			if (!bShowSubtitles) patch::Nop(0x58FCF5, 5); // call _ZN4CHud13DrawSubtitlesE
		}
		else {
			patch::NopRestore(0x58FCC2);
			patch::NopRestore(0x58D4BE);
			patch::NopRestore(0x5B17CC);
			if (!bShowMissionName) patch::NopRestore(0x58D568); // call _ZN4CHud16DrawMissionTitleEv
			if (!bShowSubtitles) patch::NopRestore(0x58FCF5); // call _ZN4CHud13DrawSubtitlesE
		}
#elif GTAVC
		if (bState) {
			patch::Nop(0x55AB4F, 6, true);
			if (!bShowMissionName) patch::Nop(0x55670E, 2, true);
		}
		else {
			patch::NopRestore(0x55AB4F);
			if (!bShowMissionName) patch::NopRestore(0x55670E);
		}
#elif GTA3
		if (bState) {
			patch::Nop(0x5084D0, 6, true);
			if (bShowMissionName) patch::SetUChar(0x50903D, 0xEB, true); // jz -> jmp;
		}
		else {
			patch::NopRestore(0x5084D0);
			if (bShowMissionName) patch::SetUChar(0x50903D, 0x74, true);
		}
#endif
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
	bool CanPauseNow();
	void Update(); // Game process event callback
	void SkipCutscene_VC();
	void PauseAudioStream(bool flag); // only VC and III

	// It requires memory patches to display messages on the screen; m_UserPause is used by the game's menu system and other things, and may be more reliable than m_CodePause
	void UserPauseMethod();
	void CodePauseMethod();
	void SetGamePaused(bool bPaused, eCutscenePauseMethod method);

	// Interface 
	void DrawInterface();

	void ProcessFreeCamera();
	void UpdateFreeCamera(float& posX, float& posY, float& posZ, float& angX, float& angY);

	bool ReadIniOptions();
	CRunningScript* GetCurrentMissionScript();
	void DebugCurrentMissionScript();

	static bool bUseSkipGameKeys;
	static std::list<unsigned int> skipKeys;
	static std::list<GamepadButton> skipButtons;

	static bool bCutscenePaused;
	
	static bool __cdecl Hook_IsCutsceneSkipButtonBeingPressed();

};

extern CutsceneController inst;