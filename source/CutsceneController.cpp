#include <plugin.h>
#include <CTimer.h>
#include <Audio.h>
#include <CTheScripts.h>

#include "CutsceneController.h"
#include "text.h"

SpriteLoader CutsceneController::sprite_loader;
bool CutsceneController::bCutscenePaused = false;
bool CutsceneController::bUseSkipGameKeys;
std::list<unsigned int> CutsceneController::skipKeys;
std::list<GamepadButton> CutsceneController::skipButtons;

CutsceneController inst;

CutsceneController::CutsceneController() {
	ReadIniOptions();
}

bool CutsceneController::LoadAudio() {
	audioMgr = new BassSampleManager();
	if (FileExists(PLUGIN_PATH("pause.wav"))) {
		pauseAudioId = audioMgr->LoadSample(PLUGIN_PATH("pause.wav"), 0, -1);
		bPauseAudioExist = true;
	}
	else {
		bPauseAudioExist = false;
	}

	if (FileExists(PLUGIN_PATH("resume.wav"))) {
		resumeAudioId = audioMgr->LoadSample(PLUGIN_PATH("resume.wav"), 0, -1);
		bResumeAudioExist = true;
	}
	else {
		bResumeAudioExist = false;
	}
	return true;
}

void CutsceneController::Update() {
	if (FrontEndMenuManager.m_bStartUpFrontEndRequested) {
		FrontEndMenuManager.m_bStartUpFrontEndRequested = false; // Prevents the menu from opening after skipping a scene or minimizing and returning to the window
	}
	
	if (IS_CUTSCENE_LOADED) {
		CPad* pad = CPad::GetPad(0);
		if (IsPauseButtonPressed(pad) && GetTickCount64() > (m_nLastPausedTime + 1000)) {
			if (bCutscenePaused) {
				if (bResumeAudioExist)
					audioMgr->AddSampleToQueue(resumeAudioVol, 44100, resumeAudioId, false, CVector(0.0f, 0.0f, 0.0f));
			}
			else {
				if (bPauseAudioExist)
					audioMgr->AddSampleToQueue(pauseAudioVol, 44100, pauseAudioId, false, CVector(0.0f, 0.0f, 0.0f));
			}

			ChangeCutscenePause();
			m_nLastPausedTime = GetTickCount64();
		}

		if (IsDebugButtonPressed(pad) && GetTickCount64() > (m_nLastDebugTime + 1000)) {

			if (bShowDebugInterface) bShowDebugInterface = false;
			else bShowDebugInterface = true;
			m_nLastDebugTime = GetTickCount64();
		}

		if (bCutscenePaused) {
			if (Hook_IsCutsceneSkipButtonBeingPressed()) {
				ChangeCutscenePause();
				CCutsceneMgr::SkipCutscene();
			}
			else {
				CTheScripts::Process(); // The game stop script execution when m_UserPause/m_CodePause are enabled
			}

			
		}

		audioMgr->Process();
	}
}

void CutsceneController::UserPauseMethod() {
	if (CTimer::m_UserPause) {
		SetUserPatchState(false);

		bShowInterface = false;
		CTimer::EndUserPause();
		bCutscenePaused = false;
	}
	else {
		SetUserPatchState(true);

		bShowInterface = true;
		CTimer::StartUserPause();
		bCutscenePaused = true;
	}
}

void CutsceneController::CodePauseMethod() {
	if (CTimer::m_CodePause) {
		bShowInterface = false;
		CTimer::m_CodePause = false;
		patch::NopRestore(0x5B17CC);
		bCutscenePaused = false;
	}
	else {
		bShowInterface = true;
		CTimer::m_CodePause = true;
		patch::Nop(0x5B17CC, 6);
		bCutscenePaused = true;
	}
}

void CutsceneController::DrawDebugInterface() {
	if (IS_CUTSCENE_LOADED && bShowDebugInterface) {
		static char msg[255];
		sprintf_s(msg, "CutsceneName: %s", CCutsceneMgr::ms_cutsceneName);
		Draw_String(msg, 40.0, 15.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

		sprintf_s(msg, "Local: x %.2f  y %.2f z %.2f", CCutsceneMgr::ms_cutsceneOffset.x, CCutsceneMgr::ms_cutsceneOffset.y, CCutsceneMgr::ms_cutsceneOffset.z);
		Draw_String(msg, 40.0, 25.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

		sprintf_s(msg, "CutsceneTimer: %.2f", CCutsceneMgr::ms_cutsceneTimer);
		Draw_String(msg, 40.0, 35.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

		sprintf_s(msg, "CurrentTextOutput: %i", CCutsceneMgr::ms_currTextOutput);
		Draw_String(msg, 40.0, 45.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

		sprintf_s(msg, "CurrentTextPointer: 0x%x", CCutsceneMgr::ms_cTextOutput);
		Draw_String(msg, 40.0, 55.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

		sprintf_s(msg, "NumHiddenEntities: %i", CCutsceneMgr::ms_iNumHiddenEntities);
		Draw_String(msg, 40.0, 65.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

		sprintf_s(msg, "NumParticleEffects: %i", CCutsceneMgr::ms_iNumParticleEffects);
		Draw_String(msg, 40.0, 75.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

		sprintf_s(msg, "NumCutsceneObjs: %i", CCutsceneMgr::ms_numCutsceneObjs);
		Draw_String(msg, 40.0, 85.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);
	}
}

void CutsceneController::DrawInterface() {
	if (IS_CUTSCENE_LOADED && bShowInterface) {
		Draw_String(pauseText.c_str(), pauseTextVec.x, pauseTextVec.y, pauseTextSizeX, pauseTextSizeY, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_CENTER);
		if (bVignette) {
			vig = sprite_loader.GetSprite("cuts_vignette");
			if(vig.m_pTexture != nullptr)
				vig.Draw(CRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), CRGBA(0, 0, 0, vignetteAlpha));
		}
	}
}

void CutsceneController::ReadIniOptions() {
	CIniReader ini("CutsceneController.ini");
	if (ini.data.size() <= 0) {
		// erro
		return;
	}

	// interface config
	string s = ini.ReadString("Config", "PauseText", "paused");
	if (!s.empty()) {
		pauseText = s;
	}

	float f = 0.0f;
	f = ini.ReadFloat("Config", "PauseTextX", 570.0);
	if (f > 0.0) {
		pauseTextVec.x = f;
	}

	f = ini.ReadFloat("Config", "PauseTextY", 400.0);
	if (f > 0.0) {
		pauseTextVec.y = f;
	}

	f = ini.ReadFloat("Config", "PauseTextSizeX", 1.0);
	if (f > 0.0) {
		pauseTextSizeX = f;
	}

	f = ini.ReadFloat("Config", "PauseTextSizeY", 1.0);
	if (f > 0.0) {
		pauseTextSizeY = f;
	}

	int i2; bool b;
	bVignette = ini.ReadBoolean("Config", "ActiveVignette", true);
	bUseSkipGameKeys = ini.ReadBoolean("Config", "UseSkipGameKeys", true);
	bUseMouseToSkip = ini.ReadBoolean("Config", "UseMouseLeftButtonToSkip", true);

	// reading custom keys
	static char str[32];
	if (!bUseSkipGameKeys) {
		int n = NULL;
		while (true) {
			sprintf_s(str, "SkipKey%i", n);
			unsigned int key = ini.ReadInteger("Config", str, -1);
			if (key == -1)
				break;
			skipKeys.push_back(key);
			n++;
		}

		n = NULL;
		while (true) {
			sprintf_s(str, "SkipButton%i", n);
			int button = ini.ReadInteger("Config", str, -1);
			if (button == -1)
				break;
			skipButtons.push_back((GamepadButton)button);
			n++;
		}
	}
	
	i2 = ini.ReadInteger("Config", "VignetteAlpha", 255);
	if (i2 > -1 && i2 < 256) {
		vignetteAlpha = (unsigned char)i2;
	}

	s = ini.ReadString("Config", "PauseMethod", "CODE_PAUSE");
	if (s == "USER_PAUSE") {
		pauseMethod = eCutscenePauseMethod::USER_PAUSE;
	}
	else {
		pauseMethod = eCutscenePauseMethod::CODE_PAUSE;
	}

	s = ini.ReadString("Config", "GamepadButtonPause", "None");
	buttonPause = GetButtonFromString(s);

	s = ini.ReadString("Config", "GamepadButtonDebug", "None");
	buttonDebug = GetButtonFromString(s);

	// virtual keys
	int i;
	i = ini.ReadInteger("Config", "PauseKey", VK_F9);
	if (i > -1) {
		pauseKey = i;
	}

	i = ini.ReadInteger("Config", "DebugKey", VK_F9);
	if (i > -1) {
		debugKey = i;
	}

	// audio
	pauseAudioVol = ini.ReadInteger("Config", "PauseAudioVolume", 127);
	resumeAudioVol = ini.ReadInteger("Config", "ResumeAudioVolume", 127);
}

typedef bool(__thiscall* FuncFW)();
FuncFW isForegroundWindow = (FuncFW)0x746070;

bool __cdecl CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed() {
	// This prevents the value from returning true when the game window is not in focus
	if (!isForegroundWindow())
		return false;

	CPad* Pad = CPad::GetPad(0);

	if (bUseSkipGameKeys) {
		if (!Pad->NewState.ButtonCross || Pad->OldState.ButtonCross)
		{
			if (!inst.IsMouseLeftButtonPressed())
			{
				if ((!CPad::NewKeyState.enter || CPad::OldKeyState.enter)
					&& (!CPad::NewKeyState.extenter || CPad::OldKeyState.extenter))
				{
					if ((!CPad::NewKeyState.standardKeys[VK_SPACE] || CPad::OldKeyState.standardKeys[VK_SPACE]) && isForegroundWindow())
						return false;
				}
			}
		}
	}
	else {
		for (auto& i : skipKeys) {
			if (KeyPressed(i))
				return true;
		}
		
		for (auto& i : skipButtons) {
			if (IsButtonPressed(i, Pad->NewState) && !IsButtonPressed(i, Pad->OldState))
				return true;
		}

		if (inst.IsMouseLeftButtonPressed()) {
			return true;
		}

		return false;
	}
	return true;
}

// Allows plugins/scripts to detect if the cutscene is paused
extern "C" __declspec(dllexport) bool __cdecl IsCutscenePaused() {
	return inst.bCutscenePaused;
}