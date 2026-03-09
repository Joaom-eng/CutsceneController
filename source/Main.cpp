#include <plugin.h> 
#include <string>
#include <CPostEffects.h>
#include "CutsceneController.h"
#include "blur.h"

using namespace plugin;

struct Main
{
    Main()
    {   
        if (GetGameVersion() != GAME_10US_HOODLUM) return;
        
        // Calls hook 
        injector::MakeCALL(0x5B1947, CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed);
        
        // It's possible to get the subtitles rendered in the cutscene in real time, but the game already shows this in the menu
        //injector::MakeCALL(0x5B187E, AddMessageHookCall, true); 
        
        Events::initRwEvent += []() {
            if (FileExists(PLUGIN_PATH("cuts_vignette.png")))
                CutsceneController::sprite_loader.LoadSpriteFromFolder(PLUGIN_PATH("cuts_vignette.png"));
        };
        
        Events::shutdownRwEvent += [] {
            CutsceneController::sprite_loader.Clear();
        };
        
        // Register event callbacks
        Events::initScriptsEvent += [] {inst.LoadAudio(); };
        Events::gameProcessEvent += [] { inst.Update();};
		Events::drawAfterFadeEvent += [] { inst.DrawInterface(); inst.DrawDebugInterface(); };

        Events::drawingEvent.before += [] {
            if (inst.bCutscenePaused && inst.bBlur) {
                RwRaster* raster = CPostEffects::pRasterFrontBuffer;
                if (!raster) return;

                float blurIntensity = inst.fBlurIntensity;
                unsigned char alpha = inst.blurAlpha; 

                // 4 diagonal passes(simulates Gaussian shader)
                DrawBlurStep(raster, -blurIntensity, -blurIntensity, alpha);
                DrawBlurStep(raster, blurIntensity, -blurIntensity, alpha);
                DrawBlurStep(raster, -blurIntensity, blurIntensity, alpha);
                DrawBlurStep(raster, blurIntensity, blurIntensity, alpha);

                // More cardinal steps to increase intensity
                DrawBlurStep(raster, 0, blurIntensity * 1.5f, alpha);
                DrawBlurStep(raster, 0, -blurIntensity * 1.5f, alpha);
            }
        };
        
        // I believe this is the best method, if something causes the cutscene audio to stop, it will be paused
        Events::onPauseAllSounds += [] {
            if (IS_CUTSCENE_RUNNING && !CutsceneController::bCutscenePaused) {
                inst.ChangeCutscenePause();
            }
        };
    }
} gInstance;
