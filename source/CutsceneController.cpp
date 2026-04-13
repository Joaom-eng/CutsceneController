#include <plugin.h> // Plugin-SDK version 1002 from 2025-12-09 23:18:09
#include <CTheScripts.h>
#include <CTimer.h>
#include <CPostEffects.h>
#include <CUserDisplay.h>

#include "CutsceneController.h"
#include "text.h"

#define ACTIVE_CAMERA TheCamera.m_aCams[TheCamera.m_nActiveCam]

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

void CutsceneController::UpdateFreeCamera(float& posX, float& posY, float& posZ, float& angX, float& angY) {
	if (bCutscenePaused) TheCamera.Process(); // Having the m_UserPause and m_CodePause flags active prevents TheCamera.Process() from being called
	
	CPad* pad = CPad::GetPad(0);

	if (pad->NewMouseControllerState.wheelUp) {
		camSpeed += 0.1f;
	}
	if (pad->NewMouseControllerState.wheelDown) {
		camSpeed -= 0.1f;
		if (camSpeed < 0.0f) camSpeed = 0.1f;
	}

	auto mouse = pad->NewMouseControllerState; // Mouse movement
	if (injector::ReadMemory<bool>(0xBA6745)) {
		mouse.y *= -1.0f;
	}

	float accel;
	if (camSensi == 0.0f) accel = TheCamera.m_fMouseAccelHorzntal;
	else accel = camSensi;

	angX += mouse.x * accel; // Sensitivity
	angY -= mouse.y * accel;

	if (angY > 1.5f) angY = 1.5f;
	if (angY < -1.5f) angY = -1.5f;

	float dirX = cos(angY) * sin(angX);
	float dirY = cos(angY) * cos(angX);
	float dirZ = sin(angY);

	if (KeyPressed(frontKey)) {
		posX += dirX * camSpeed;
		posY += dirY * camSpeed;
		posZ += dirZ * camSpeed;
	}

	if (KeyPressed(backKey)) {
		posX -= dirX * camSpeed;
		posY -= dirY * camSpeed;
		posZ -= dirZ * camSpeed;
	}

	CVector camPos = { posX, posY, posZ };
	CVector lookAt = { posX + dirX, posY + dirY, posZ + dirZ };
	CVector angle = {0.0f, 0.0f, 0.0f};

	TheCamera.SetCamPositionForFixedMode(&camPos, &angle);
	PointCameraAtPoint(&lookAt, eSwitchType::SWITCHTYPE_JUMPCUT);
}

void CutsceneController::ProcessFreeCamera() {
	static float angY, angX;
	static float posX, posY, posZ;

	CVector camPos;

	static CVector vecPoint;
	unsigned int ms = static_cast<unsigned int>(CCutsceneMgr::ms_cutsceneTimer * 1000.0f);

	eCamMode mode = ACTIVE_CAMERA.m_nMode;
	
	ACTIVE_CAMERA.m_nMode = eCamMode::MODE_FLYBY; // Necessary to call GetCutSceneFinishTime
	if (ms >= TheCamera.GetCutSceneFinishTime() && bFixedCam && !IS_ON_SCRIPTED_CUTSCENE) {
		mode = MODE_FLYBY;
		CCutsceneMgr::SkipCutscene(); // Fixes the scene not finishing with bFixedCam active
		bFixedCam = false;
	}
	ACTIVE_CAMERA.m_nMode = mode;

	if (bFixedCam) {
		UpdateFreeCamera(posX, posY, posZ, angX, angY);
	}

	static ULONGLONG lastKeyTime;

	if (KeyPressed(toggleCamKey) && GetTickCount64() > (lastKeyTime + 1000)) {
		if (bFixedCam) {
			if (IS_ON_SCRIPTED_CUTSCENE) {
				ACTIVE_CAMERA.m_nMode = eCamMode::MODE_FIXED;
				TheCamera.SetCamPositionForFixedMode(&lastFixedCamPos, &lastFixedCamAngle);
				TheCamera.m_vecFixedModeVector = vecPoint; // This vector is modified by the opcode POINT_CAMERA_AT_POINT
			}
			else {
				// Resynchronizes the timer used in the CCam::Process_FlyBy function
				ACTIVE_CAMERA.m_fTimeElapsedFloat = CCutsceneMgr::ms_cutsceneTimer * 1000.0f;

				ACTIVE_CAMERA.m_nMode = eCamMode::MODE_FLYBY;
				TheCamera.TakeControlWithSpline(eSwitchType::SWITCHTYPE_JUMPCUT);
			}
			bFixedCam = false;
		}
		else {
			camPos = ACTIVE_CAMERA.m_vecSource;

			posX = camPos.x;
			posY = camPos.y;
			posZ = camPos.z;

			lastFixedCamPos = TheCamera.m_vecFixedModeSource;
			lastFixedCamAngle = TheCamera.m_vecFixedModeUpOffSet;
			vecPoint = TheCamera.m_vecFixedModeVector;
			bFixedCam = true;
		}
		lastKeyTime = GetTickCount64();
	}
}

void CutsceneController::Update() {
	if (FrontEndMenuManager.m_bStartUpFrontEndRequested) {
		FrontEndMenuManager.m_bStartUpFrontEndRequested = false; // Prevents the menu from opening after skipping a scene or minimizing and returning to the window
	}

	if (IS_ON_ANY_CUTSCENE) {
		CPad* pad = CPad::GetPad(0);
		
		inst.ProcessFreeCamera();

		if (IsPauseButtonPressed(pad) && GetTickCount64() > (m_nLastPausedTime + 1000)) {
			if (bCutscenePaused) {
				if (bResumeAudioExist)
					audioMgr->AddSampleToQueue(resumeAudioVol, resumeAudioFreq, resumeAudioId, false, CVector(0.0f, 0.0f, 0.0f), 8, false);
			}
			else {
				if (bPauseAudioExist)
					audioMgr->AddSampleToQueue(pauseAudioVol, pauseAudioFreq, pauseAudioId, false, CVector(0.0f, 0.0f, 0.0f), 8, false);
			}
			
			RwRasterPushContext(gsBlur->blurRaster);
			RwRasterRenderFast(CPostEffects::pRasterFrontBuffer, 0, 0);
			RwRasterPopContext();

			// Pausing the mission timers, as they are updated even when paused
			if (IS_ON_SCRIPTED_CUTSCENE) {
				if (bCutscenePaused) {
					CUserDisplay::OnscnTimer.m_bPaused = false; // same as FREEZE_ONSCREEN_TIMER opcode
					injector::WriteMemory<char>(0x58B325, 1, true);
				}
				else {
					CUserDisplay::OnscnTimer.m_bPaused = true;
					injector::WriteMemory<char>(0x58B325, 0, true);
				}
			}

			ChangeCutscenePause();
			m_nLastPausedTime = GetTickCount64();
		}

		if (IsDebugButtonPressed(pad) && GetTickCount64() > (m_nLastDebugTime + 1000)) {
			if (bShowDebugInterface) bShowDebugInterface = false;
			else bShowDebugInterface = true;
			m_nLastDebugTime = GetTickCount64();
		}

		if (bCutscenePaused && !bFixedCam) {
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
	if ((IS_ON_ANY_CUTSCENE) && bShowInterface) { // pause interface
		Draw_String(pauseText.c_str(), pauseTextVec.x, pauseTextVec.y, pauseTextSizeX, pauseTextSizeY, true, pauseTextFont, eFontAlignment::ALIGN_CENTER);
		if (bVignette) {
			vig = sprite_loader.GetSprite("cuts_vignette");
			if(vig.m_pTexture != nullptr)
				vig.Draw(CRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), CRGBA(0, 0, 0, vignetteAlpha));
		}
	}
	if ((IS_ON_ANY_CUTSCENE) && bFixedCam && bShowCamSpeedText) {
		char msg[64];
		sprintf_s(msg, "Camera speed: %.1f", camSpeed);
		Draw_String(msg, 580.0f, 15.0f, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_CENTER); // 640 480
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

	f = ini.ReadFloat("Interface", "BlurIntensity", 2.0f);
	fBlurIntensity = f;

	int i2;
	bVignette = ini.ReadBoolean("Interface", "ActiveVignette", true);
	i2 = ini.ReadInteger("Interface", "VignetteAlpha", 255);
	if (i2 > -1 && i2 < 256) {
		vignetteAlpha = (unsigned char)i2;
	}

	bBlur = ini.ReadBoolean("Interface", "ActiveBlur", true);
	bUsePixelShader = ini.ReadBoolean("Interface", "UseGaussianShader", false);

	bSetShadow = ini.ReadBoolean("Interface", "PauseTextShadow", false);

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

	// main keys
	pauseKey = ini.ReadInteger("Input", "PauseKey", VK_F9);
	debugKey = ini.ReadInteger("Input", "DebugKey", VK_TAB);
	disableBlurInGameKey = ini.ReadInteger("Input", "DisableBlurInGameKey", 0);

	// audio
	pauseAudioVol = ini.ReadInteger("Audio", "PauseAudioVolume", 127);
	resumeAudioVol = ini.ReadInteger("Audio", "ResumeAudioVolume", 127);

	pauseAudioFreq = ini.ReadInteger("Audio", "PauseFreq", 44100);
	resumeAudioFreq = ini.ReadInteger("Audio", "ResumeFreq", 44100);

	// camera
	toggleCamKey = ini.ReadInteger("Camera", "ToggleKey", 0);
	camSensi = ini.ReadFloat("Camera", "Sensitivity", 0.0f);
	camSpeed = ini.ReadFloat("Camera", "InitialSpeed", 0.3f);
	bShowCamSpeedText = ini.ReadBoolean("Camera", "ShowSpeedNumber", true);
	frontKey = ini.ReadInteger("Camera", "FrontKey", 87); 
	backKey = ini.ReadInteger("Camera", "BackKey", 83);

}

typedef bool(__thiscall* FuncFW)();
FuncFW isForegroundWindow = (FuncFW)0x746070;

bool __cdecl CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed() {
	// This prevents the value from returning true when the game window is not in focus
	if (!isForegroundWindow())
		return false;

	if (bCutscenePaused && !inst.bSkipInPause)
		return false;

	CPad* pad = CPad::GetPad(0);

	if (bUseSkipGameKeys) {
		if (!pad->NewState.ButtonCross || pad->OldState.ButtonCross)
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
			if (IsButtonPressed(i, pad->NewState) && !IsButtonPressed(i, pad->OldState))
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