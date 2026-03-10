#include <plugin.h>
#include <CTimer.h>
#include <Audio.h>
#include <CTheScripts.h>

#include "CutsceneController.h"
#include "blur.h"
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
	
	if (IS_CUTSCENE_RUNNING) {
		CPad* pad = CPad::GetPad(0);
		if (IsPauseButtonPressed(pad) && GetTickCount64() > (m_nLastPausedTime + 1000)) {
			if (bCutscenePaused) {
				if (bResumeAudioExist)
					audioMgr->AddSampleToQueue(resumeAudioVol, resumeAudioFreq, resumeAudioId, false, CVector(0.0f, 0.0f, 0.0f), 8, false);
			}
			else {
				if (bPauseAudioExist)
					audioMgr->AddSampleToQueue(pauseAudioVol, pauseAudioFreq, pauseAudioId, false, CVector(0.0f, 0.0f, 0.0f), 8, false);
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
	if (IS_CUTSCENE_RUNNING && bShowDebugInterface) {
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
	if (IS_CUTSCENE_RUNNING && bShowInterface) {
		Draw_String(pauseText.c_str(), pauseTextVec.x, pauseTextVec.y, pauseTextSizeX, pauseTextSizeY, true, pauseTextFont, eFontAlignment::ALIGN_CENTER);
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
		// error
		return;
	}

	// interface config
	string s = ini.ReadString("Interface", "PauseText", "paused");
	if (!s.empty()) {
		pauseText = s;
	}

	float f = 0.0f;
	f = ini.ReadFloat("Interface", "PauseTextX", 570.0f);
	if (f > 0.0) {
		pauseTextVec.x = f;
	}

	f = ini.ReadFloat("Interface", "PauseTextY", 400.0f);
	if (f > 0.0) {
		pauseTextVec.y = f;
	}

	f = ini.ReadFloat("Interface", "PauseTextSizeX", 1.0f);
	if (f > 0.0) {
		pauseTextSizeX = f;
	}

	f = ini.ReadFloat("Interface", "PauseTextSizeY", 1.0f);
	if (f > 0.0) {
		pauseTextSizeY = f;
	}

	pauseTextBorder = ini.ReadInteger("Interface", "PauseTextBorder", 1);

	f = ini.ReadFloat("Interface", "BlurIntensity", 6.0f);
	fBlurIntensity = f;

	int i2;
	bVignette = ini.ReadBoolean("Interface", "ActiveVignette", true);
	i2 = ini.ReadInteger("Interface", "VignetteAlpha", 255);
	if (i2 > -1 && i2 < 256) {
		vignetteAlpha = (unsigned char)i2;
	}
	bBlur = ini.ReadBoolean("Interface", "ActiveBlur", true);
	bSetShadow = ini.ReadBoolean("Interface", "PauseTextShadow", false);
	bSetBackground = ini.ReadBoolean("Interface", "PauseTextBackground", false);

	// font 
	i2 = ini.ReadInteger("Interface", "PauseTextFont", eFontStyle::FONT_SUBTITLES);
	if (i2 >= 0 && i2 <= 3) pauseTextFont = (eFontStyle)i2;
	else pauseTextFont = eFontStyle::FONT_SUBTITLES;

	// colors
	unsigned char r, g, b, a;
	r = ini.ReadInteger("Interface", "PauseTextRed", 255);
	g = ini.ReadInteger("Interface", "PauseTextGreen", 255);
	b = ini.ReadInteger("Interface", "PauseTextBlue", 255);
	a = ini.ReadInteger("Interface", "PauseTextAlpha", 255);
	pauseTextColor = { r, g, b, a };

	r = ini.ReadInteger("Interface", "PauseTextDropRed", 255);
	g = ini.ReadInteger("Interface", "PauseTextDropGreen", 255);
	b = ini.ReadInteger("Interface", "PauseTextDropBlue", 255);
	a = ini.ReadInteger("Interface", "PauseTextDropAlpha", 255);
	pauseTextDropColor = { r, g, b, a };
	pauseTextDropPos = ini.ReadInteger("Interface", "PauseTextDropPos", 0);

	r = ini.ReadInteger("Interface", "PauseTextBackRed", 255);
	g = ini.ReadInteger("Interface", "PauseTextBackGreen", 255);
	b = ini.ReadInteger("Interface", "PauseTextBackBlue", 255);
	a = ini.ReadInteger("Interface", "PauseTextBackAlpha", 255);
	pauseTextBackColor = { r, g, b, a };

	a = ini.ReadInteger("Interface", "BlurAlpha", 60);
	blurAlpha = a;

	bUseSkipGameKeys = ini.ReadBoolean("Input", "UseSkipGameKeys", true);
	bUseMouseToSkip = ini.ReadBoolean("Input", "UseMouseLeftButtonToSkip", true);
	bSkipInPause = ini.ReadBoolean("Input", "SkipInPause", true);

	// reading custom keys
	static char str[32];
	if (!bUseSkipGameKeys) {
		int n = NULL;
		while (true) {
			sprintf_s(str, "SkipKey%i", n);
			unsigned int key = ini.ReadInteger("Input", str, -1);
			if (key == -1)
				break;
			skipKeys.push_back(key);
			n++;
		}

		n = NULL;
		while (true) {
			sprintf_s(str, "SkipButton%i", n);
			int button = ini.ReadInteger("Input", str, -1);
			if (button == -1)
				break;
			skipButtons.push_back((GamepadButton)button);
			n++;
		}
	}

	s = ini.ReadString("Others", "PauseMethod", "CODE_PAUSE");
	if (s == "USER_PAUSE") {
		pauseMethod = eCutscenePauseMethod::USER_PAUSE;
	}
	else {
		pauseMethod = eCutscenePauseMethod::CODE_PAUSE;
	}

	s = ini.ReadString("Input", "GamepadButtonPause", "None");
	buttonPause = GetButtonFromString(s);

	s = ini.ReadString("Input", "GamepadButtonDebug", "None");
	buttonDebug = GetButtonFromString(s);

	// virtual keys
	int i;
	i = ini.ReadInteger("Input", "PauseKey", VK_F9);
	if (i > -1) {
		pauseKey = i;
	}

	i = ini.ReadInteger("Input", "DebugKey", VK_F9);
	if (i > -1) {
		debugKey = i;
	}

	// audio
	pauseAudioVol = ini.ReadInteger("Audio", "PauseAudioVolume", 127);
	resumeAudioVol = ini.ReadInteger("Audio", "ResumeAudioVolume", 127);
	pauseAudioFreq = ini.ReadInteger("Audio", "PauseFreq", 44100);
	resumeAudioFreq = ini.ReadInteger("Audio", "ResumeFreq", 44100);
}

typedef bool(__thiscall* FuncFW)();
FuncFW isForegroundWindow = (FuncFW)0x746070;

bool __cdecl CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed() {
	// This prevents the value from returning true when the game window is not in focus
	if (!isForegroundWindow())
		return false;

	if (bCutscenePaused && !inst.bSkipInPause)
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