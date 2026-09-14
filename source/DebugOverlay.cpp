#include <plugin.h>
#include <CTheScripts.h>
#include "DebugOverlay.h"
#include "CutsceneController.h"
#include "text.h"

DebugOverlay debugOv;

void DebugOverlay::Draw() {
	if (inst.bShowDebugInterface) {
		if (IS_ON_ANY_CUTSCENE) {
			DrawMissionScriptInfo();
			if (!IS_ON_SCRIPTED_CUTSCENE) 
				DrawCutsceneInfo();
		}
	}
}

void DebugOverlay::DrawCutsceneInfo() {
	static char msg[128];
	sprintf_s(msg, "CutsceneName: %s", CCutsceneMgr::ms_cutsceneName);
	Draw_String(msg, 40.0, 25.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

	sprintf_s(msg, "Local: x %.2f  y %.2f z %.2f", CUTSCENE_POS.x, CUTSCENE_POS.y, CUTSCENE_POS.z);
	Draw_String(msg, 40.0, 35.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

	sprintf_s(msg, "CutsceneTimer: %.2f", CUTSCENE_TIMER);
	Draw_String(msg, 40.0, 45.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

#ifdef GTASA
	sprintf_s(msg, "CurrentTextOutput: %i", CCutsceneMgr::ms_currTextOutput);
	Draw_String(msg, 40.0, 55.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

	sprintf_s(msg, "CurrentTextPointer: 0x%x", CCutsceneMgr::ms_cTextOutput);
	Draw_String(msg, 40.0, 65.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

	sprintf_s(msg, "NumHiddenEntities: %i", CCutsceneMgr::ms_iNumHiddenEntities);
	Draw_String(msg, 40.0, 75.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

	sprintf_s(msg, "NumParticleEffects: %i", CCutsceneMgr::ms_iNumParticleEffects);
	Draw_String(msg, 40.0, 85.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);

	sprintf_s(msg, "NumCutsceneObjs: %i", CCutsceneMgr::ms_numCutsceneObjs);
	Draw_String(msg, 40.0, 95.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);
#else 
	sprintf_s(msg, "NumCutsceneObjs: %i", CCutsceneMgr::ms_numCutsceneObjs);
	Draw_String(msg, 40.0, 55.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);
#endif 
	
}

CRunningScript* DebugOverlay::FindMissionScript()
{
	for (CRunningScript* script = CTheScripts::pActiveScripts;
		script;
		script = script->m_pNext)
	{
		if (script->m_bIsActive && script->m_bIsMission)
			return script;
	}

	return nullptr;
}

void DebugOverlay::DrawMissionScriptInfo() {
	CRunningScript* script = FindMissionScript();
	if (!script) return;

	
#ifdef GTASA
	static char msg[128];
	const unsigned short rawOpcode =
		*reinterpret_cast<unsigned short*>(script->m_pCurrentIP);
	const unsigned short opcode = rawOpcode & 0x7FFF;

	
	sprintf_s(msg, "Mission script: %.8s~n~Offset: %hu~n~Opcode: %x", script->m_szName, 
		script->m_pCurrentIP - script->m_pBaseIP,
		opcode
	);
#else 
	static char msg[32];
	static char msg1[32];
	static char msg2[32];
	// GTA VC does not support the use of ~n~
	sprintf_s(msg, "Mission script: %.8s", script->m_szName);
	sprintf_s(msg1, "Current Ip: %hu", script->m_nIp);
	sprintf_s(msg2, "WakeTime: %is", (script->m_nWakeTime / 1000));
	Draw_String(msg1, 540.0, 35.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);
	Draw_String(msg2, 540.0, 45.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);
#endif 
	Draw_String(msg, 540.0, 25.0, 0.3, 0.5, true, eFontStyle::FONT_SUBTITLES, eFontAlignment::ALIGN_LEFT);
	
}
