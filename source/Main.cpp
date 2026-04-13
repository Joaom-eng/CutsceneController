#include <plugin.h> 
#include <string>

#include "CutsceneController.h"
#include "blur.h"

using namespace plugin;

struct Main
{
    Main()
    {   
        if (GetGameVersion() != GAME_10US_HOODLUM) return;
        
        injector::MakeJMP(0x4D5D10, CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed, true);

        // It's possible to get the subtitles rendered in the cutscene in real time, but the game already shows this in the menu
        //injector::MakeCALL(0x5B187E, AddMessageHookCall, true); 
        
        Events::initRwEvent += []() {
            if (FileExists(PLUGIN_PATH("cuts_vignette.png")))
                CutsceneController::sprite_loader.LoadSpriteFromFolder(PLUGIN_PATH("cuts_vignette.png"));
            inst.gsBlur = new GaussianBlur(); // init rasters
            RwD3D9CreatePixelShader((RwUInt32*)&blur_cso, (void**)&inst.gsBlur->blurShader);
        };
        
        Events::shutdownRwEvent += [] {
            CutsceneController::sprite_loader.Clear();
        };
        
        // Register event callbacks
        Events::initScriptsEvent += [] { inst.LoadAudio();  };

        Events::gameProcessEvent += [] { inst.Update();};
        
		Events::drawAfterFadeEvent += [] { 
            if (!FrontEndMenuManager.m_bMenuActive) {
                inst.DrawInterface(); 
                inst.DrawDebugInterface();
            }
        };

        Events::drawingEvent.before += [=] {
            if (inst.bCutscenePaused && !inst.bFixedCam) {
                if (inst.bBlur) {
                    inst.gsBlur->DrawBlur();
                }
                else {
                    // Renders the "copy" of the raster front made before pausing the cutscene
                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)inst.gsBlur->blurRaster);
                    for (int i = 0; i < 4; i++) {
                        colorfilterVerts[i].emissiveColor = 0xFFFFFFFF;
                    }
                    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, colorfilterVerts, 4, colorfilterIndices, 6);
                }
            }
        };
        
        // I believe this is the best method, if something causes the cutscene audio to stop, it will be paused
        Events::onPauseAllSounds += [] {
            if ((IS_ON_ANY_CUTSCENE) && !CutsceneController::bCutscenePaused) {
                inst.ChangeCutscenePause();
            }
        };
    }
} gInstance;
