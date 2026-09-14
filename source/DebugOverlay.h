#pragma once
#include <CRunningScript.h>

class DebugOverlay {
public:
    void Draw();

private:
    void DrawCutsceneInfo();
    void DrawMissionScriptInfo();
    CRunningScript* FindMissionScript();
};

extern DebugOverlay debugOv;