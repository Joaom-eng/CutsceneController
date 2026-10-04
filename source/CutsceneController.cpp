#include <plugin.h> // Plugin-SDK version 1002 from 2025-12-09 23:18:09
#include <CTheScripts.h>
#include <CTimer.h>
#ifdef GTASA
	#include <CPostEffects.h>
#endif

#if defined(GTAVC) || defined(GTA3)
	#include <cSampleManager.h>
	#include <cDMAudio.h>
#endif 

#include <CUserDisplay.h>
#include <CGame.h>
#include <CScene.h>
#include <cmath>

#include "CutsceneController.h"
#include "text.h"

bool CutsceneController::bCutscenePaused = false;
bool CutsceneController::bUseSkipGameKeys;
std::list<unsigned int> CutsceneController::skipKeys;
std::list<GamepadButton> CutsceneController::skipButtons;
CutsceneController inst;

CutsceneController::CutsceneController() {
	
}

bool CutsceneController::LoadAudio() {
#ifdef GTASA
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
#endif
}

void CutsceneController::SkipCutscene_VC() {
#ifdef GTAVC
	ms_wasCutsceneSkipped = true;

	if (dataFileLoaded) {
		ms_cutsceneTimer = (float)TheCamera.GetCutSceneFinishTime() * 0.001f;
		TheCamera.FinishCutscene();
	}

	FindPlayerPed()->bIsVisible = true;
	CWorld::Players[CWorld::PlayerInFocus].MakePlayerSafe(false);
#endif
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

#ifdef GTASA
	if (injector::ReadMemory<bool>(0xBA6745, true)) mouse.y *= -1.0f;
#elif GTAVC 
	if (!injector::ReadMemory<bool>(0xA10A4C, true)) mouse.y *= -1.0f;
#elif GTA3 
	if (!injector::ReadMemory<bool>(0x95CC8C, true)) mouse.y *= -1.0f;
#endif

	float accel;
	if (camSensi == 0.0f) accel = TheCamera.m_fMouseAccelHorzntal;
	else accel = camSensi;

	static CVector cameraVelocity = { 0.0f, 0.0f, 0.0f };
	static float angularVelocityX = 0.0f;
	static float angularVelocityY = 0.0f;
	static ULONGLONG previousUpdateTime = 0;

	constexpr float referenceFrameRate = 60.0f;
	constexpr float maxDeltaTime = 0.05f;

	ULONGLONG currentUpdateTime = GetTickCount64();
	float deltaTime = 1.0f / referenceFrameRate;

	if (previousUpdateTime != 0) {
		ULONGLONG elapsedMilliseconds = currentUpdateTime - previousUpdateTime;

		if (elapsedMilliseconds > 250) {
			cameraVelocity = { 0.0f, 0.0f, 0.0f };
			angularVelocityX = 0.0f;
			angularVelocityY = 0.0f;
		}
		else {
			deltaTime = static_cast<float>(elapsedMilliseconds) * 0.001f;
			if (deltaTime < 0.001f) deltaTime = 0.001f;
			if (deltaTime > maxDeltaTime) deltaTime = maxDeltaTime;
		}
	}

	previousUpdateTime = currentUpdateTime;

	float targetAngularVelocityX = mouse.x * accel / deltaTime;
	float targetAngularVelocityY = -mouse.y * accel / deltaTime;
	float rotationSmoothing = 1.0f - std::exp(-rotationResponse * deltaTime);

	angularVelocityX += (targetAngularVelocityX - angularVelocityX) * rotationSmoothing;
	angularVelocityY += (targetAngularVelocityY - angularVelocityY) * rotationSmoothing;

	angX += angularVelocityX * deltaTime;
	angY += angularVelocityY * deltaTime;

	if (angY > 1.5f) {
		angY = 1.5f;
		if (angularVelocityY > 0.0f) angularVelocityY = 0.0f;
	}
	if (angY < -1.5f) {
		angY = -1.5f;
		if (angularVelocityY < 0.0f) angularVelocityY = 0.0f;
	}

	float dirX = cos(angY) * sin(angX);
	float dirY = cos(angY) * cos(angX);
	float dirZ = sin(angY);

	float forwardInput = 0.0f;
	float sideInput = 0.0f;
	if (KeyPressed(frontKey)) forwardInput += 1.0f;
	if (KeyPressed(backKey)) forwardInput -= 1.0f;
	if (KeyPressed(rightKey)) sideInput += 1.0f;
	if (KeyPressed(leftKey)) sideInput -= 1.0f;

	float inputLength = std::sqrt(forwardInput * forwardInput + sideInput * sideInput);
	if (inputLength > 1.0f) {
		forwardInput /= inputLength;
		sideInput /= inputLength;
	}

	float rightX = cos(angX);
	float rightY = -sin(angX);

	CVector targetVelocity = {
		(dirX * forwardInput + rightX * sideInput) * camSpeed,
		(dirY * forwardInput + rightY * sideInput) * camSpeed,
		dirZ * forwardInput * camSpeed
	};

	float movementSmoothing = 1.0f - std::exp(-movementResponse * deltaTime);
	cameraVelocity.x += (targetVelocity.x - cameraVelocity.x) * movementSmoothing;
	cameraVelocity.y += (targetVelocity.y - cameraVelocity.y) * movementSmoothing;
	cameraVelocity.z += (targetVelocity.z - cameraVelocity.z) * movementSmoothing;

	float frameScale = deltaTime * referenceFrameRate;
	posX += cameraVelocity.x * frameScale;
	posY += cameraVelocity.y * frameScale;
	posZ += cameraVelocity.z * frameScale;

	CVector camPos = { posX, posY, posZ };
	CVector lookAt = { posX + dirX, posY + dirY, posZ + dirZ };
	CVector angle = { 0.0f, 0.0f, 0.0f };

#ifdef GTASA
	TheCamera.SetCamPositionForFixedMode(&camPos, &angle);
#elif defined(GTAVC) || defined(GTA3) 
	TheCamera.SetCamPositionForFixedMode(camPos, angle);
#endif 
	

	PointCameraAtPoint(&lookAt, eSwitchType::SWITCHTYPE_JUMPCUT);
}

void CutsceneController::ProcessFreeCamera() {
	static float angY, angX;
	static float posX, posY, posZ;

	CVector camPos;
	
	static CVector vecPoint;

	unsigned int ms = static_cast<unsigned int>(CUTSCENE_TIMER * 1000.0f);


	eCamMode mode = (eCamMode)CAM_MODE;
	CAM_MODE = eCamMode::MODE_FLYBY; // Necessary to call GetCutSceneFinishTime
	if (ms >= TheCamera.GetCutSceneFinishTime() && bFixedCam && !IS_ON_SCRIPTED_CUTSCENE) {
		mode = MODE_FLYBY;
		SKIP_CUTSCENE; // Fixes the scene not finishing with bFixedCam active
		bFixedCam = false;
	}
	CAM_MODE = mode;

	if (bFixedCam) {
		UpdateFreeCamera(posX, posY, posZ, angX, angY);
	}

	static ULONGLONG lastKeyTime;

	if (KeyPressed(toggleCamKey) && GetTickCount64() > (lastKeyTime + 1000)) {
		if (bFixedCam) {
			if (IS_ON_SCRIPTED_CUTSCENE) {
				CAM_MODE = eCamMode::MODE_FIXED;
#ifdef GTASA
				TheCamera.SetCamPositionForFixedMode(&lastFixedCamPos, &lastFixedCamAngle);
#endif 
#if defined(GTAVC) || defined(GTA3)
				TheCamera.SetCamPositionForFixedMode(lastFixedCamPos, lastFixedCamAngle);
#endif 
				TheCamera.m_vecFixedModeVector = vecPoint; // This vector is modified by the opcode POINT_CAMERA_AT_POINT
			}
			else {
				// Resynchronizes the timer used in the CCam::Process_FlyBy function
				ACTIVE_CAMERA.m_fTimeElapsedFloat = CUTSCENE_TIMER * 1000.0f;

				CAM_MODE = eCamMode::MODE_FLYBY;
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

bool CutsceneController::CanPauseNow() {
#ifdef GTASA
	if (bPauseOnlyDuringMissions && !IS_ON_MISSION)
		return false;
#endif

	return true;
}

void CutsceneController::Update() {
	if (FrontEndMenuManager.m_bStartUpFrontEndRequested) {
		FrontEndMenuManager.m_bStartUpFrontEndRequested = false; // Prevents the menu from opening after skipping a scene or minimizing and returning to the window
	}

	if (IS_ON_ANY_CUTSCENE) {
		CPad* pad = CPad::GetPad(0);

		inst.ProcessFreeCamera();

		if (IsPauseButtonPressed(pad) && GetTickCount64() > (m_nLastPausedTime + 1000)) {
			if (CanPauseNow()) {

#ifdef GTASA
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
						MISSION_TIMER_PAUSED = false; // same as FREEZE_ONSCREEN_TIMER opcode
						injector::WriteMemory<char>(0x58B325, 1, true);
					}
					else {
						MISSION_TIMER_PAUSED = true;
						injector::WriteMemory<char>(0x58B325, 0, true);
					}
				}
#endif
				if(!bCutscenePaused) needBlurCapture = true;
				ChangeCutscenePause();
				
				m_nLastPausedTime = GetTickCount64();
			}
		}
		
		if (IsDebugButtonPressed(pad) && GetTickCount64() > (m_nLastDebugTime + 1000)) {
			if (bShowDebugInterface) bShowDebugInterface = false;
			else bShowDebugInterface = true;
			m_nLastDebugTime = GetTickCount64();
		}

		if (bCutscenePaused) {
			if (Hook_IsCutsceneSkipButtonBeingPressed()) {
				ChangeCutscenePause();
				SKIP_CUTSCENE;
			}
			else {
#if defined(GTASA) || defined(GTAVC)
				CTheScripts::Process(); // The game stop script execution when m_UserPause/m_CodePause are enabled
#elif GTA3 
				ProcessScripts_III();
#endif 
			}
		}
#ifdef GTASA 
		audioMgr->Process();
#endif 
	}
}

void CutsceneController::SetGamePaused(bool paused, eCutscenePauseMethod method) {
	
	bShowInterface = paused;
	PauseAudioStream(paused);

	if (method == eCutscenePauseMethod::USER_PAUSE) CTimer::m_UserPause = paused;
	else CTimer::m_CodePause = paused;

	ApplyPausePatches(paused);
	bCutscenePaused = paused;
}

static unsigned char effectsFadeVolumeBeforePause = 127;
static bool effectsMutedByCodePause = false;

void CutsceneController::PauseAudioStream(bool flag) {
#ifdef GTA3 
	PauseStream_III(&SampleManager, flag, 0);
	if (flag && !effectsMutedByCodePause) {
		effectsFadeVolumeBeforePause = SampleManager.m_nEffectsFadeVolume;
		DMAudio.SetEffectsFadeVol(0);
		effectsMutedByCodePause = true;
	}
	else if (!flag && effectsMutedByCodePause) {
		DMAudio.SetEffectsFadeVol(effectsFadeVolumeBeforePause);
		effectsMutedByCodePause = false;
	}
#elif GTAVC 
	PauseStream_VC(&SampleManager, flag, 0);
#endif 
}

void CutsceneController::DrawInterface() {
	debugOv.Draw();

	if ((IS_ON_ANY_CUTSCENE) && bShowInterface) { // pause interface
		if (bVignette) {
			if (vig.m_pTexture != nullptr) 
				vig.Draw(CRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), CRGBA(0, 0, 0, vignetteAlpha));
		}
		if (bPauseIcon) {
			std::string spriteName = std::to_string(inst.pauseKey);
			
			float magicResolutionWidth = RsGlobal.maximumWidth * 0.0015625f;
			float magicResolutionHeight = RsGlobal.maximumHeight * 0.002232143f;

			float x = pauseIconPos.x * magicResolutionWidth;
			float y = pauseIconPos.y * magicResolutionHeight;
			float w = pauseIconSizeX * magicResolutionWidth;
			float h = pauseIconSizeY * magicResolutionHeight;
			FixAspectRatio(&w, &h);

			static char msg[128];
			
			
			if (pauseButton.m_pTexture != nullptr) {
				pauseButton.Draw(CRect(x, y, x + w, y + h), CRGBA(255, 255, 255, 255));
			}
		}

		Draw_String(pauseText.c_str(), pauseTextVec.x, pauseTextVec.y, pauseTextSizeX, pauseTextSizeY, true, pauseTextFont, eFontAlignment::ALIGN_CENTER);
		
		
	}
	if ((IS_ON_ANY_CUTSCENE) && bFixedCam && bShowCamSpeedText) {
		char msg[64];
		sprintf_s(msg, "Camera speed: %.1f", camSpeed);
		Draw_String(msg, 580.0f, 15.0f, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_CENTER);
	}
}

bool CutsceneController::ReadIniOptions() {
	CIniReader ini("CutsceneController.ini");
	if (ini.data.empty()) {
		MessageBoxA(nullptr, "Failed to read CutsceneController.ini", "CutsceneController", MB_ICONERROR);
		return false;
	}

	try {
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

#ifdef GTASA
		bUsePixelShader = ini.ReadBoolean("Interface", "UseGaussianShader", false);
#endif 

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

#ifdef GTASA
		s = ini.ReadString("Others", "PauseMethod", "CODE_PAUSE");
		if (s == "USER_PAUSE") {
			pauseMethod = eCutscenePauseMethod::USER_PAUSE;
		}
		else {
			pauseMethod = eCutscenePauseMethod::CODE_PAUSE;
		}

		bPauseOnlyDuringMissions = ini.ReadBoolean("Others", "PauseOnlyDuringMissions", true);
#endif
		bFixAudioDesync = ini.ReadBoolean("Others", "FixAudioDesync", true);
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
		leftKey = ini.ReadInteger("Camera", "LeftKey", 65);
		rightKey = ini.ReadInteger("Camera", "RightKey", 68);

		movementResponse = ini.ReadFloat("Camera", "MovementResponse", 4.0f);
		if (movementResponse <= 0.0f) movementResponse = 4.0f;

		rotationResponse = ini.ReadFloat("Camera", "RotationResponse", 4.0f);
		if (rotationResponse <= 0.0f) rotationResponse = 4.0f;

		bPauseIcon = ini.ReadBoolean("Interface", "PauseSprite", true);
		pauseIconPos.x = ini.ReadFloat("Interface", "PauseSpriteX", 0.0f);
		pauseIconPos.y = ini.ReadFloat("Interface", "PauseSpriteY", 0.0f);

		pauseIconSizeX = ini.ReadFloat("Interface", "PauseSpriteSizeX", 0.0f);
		pauseIconSizeY = ini.ReadFloat("Interface", "PauseSpriteSizeY", 0.0f);

		bShowMissionName = ini.ReadBoolean("Interface", "ShowMissionName", false);
		bShowSubtitles = ini.ReadBoolean("Interface", "ShowSubtitles", true);
	}
	catch(std::exception& e){
		MessageBoxA(nullptr, e.what(), "CutsceneController", MB_ICONERROR);
		return false;
	}

	return true;
}

static bool isForegroundWindow() {
	return GetForegroundWindow() == RsGlobal.ps->window;
}

bool __cdecl CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed() {
	// This prevents the value from returning true when the game window is not in focus
	if (!isForegroundWindow())
		return false;
	
	if (bCutscenePaused && !inst.bSkipInPause)
		return false;
	
	CPad* pad = CPad::GetPad(0);
	
#if defined(GTAVC) || defined(GTA3)
	if (CGame::playingIntro)
	{
		if (pad->NewState.Start && !pad->OldState.Start)
			return true;
	}
#endif 

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
