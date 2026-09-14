#include <plugin.h> 
#include <string>
#include "CutsceneController.h"
#include <CTxdStore.h>
#include <CScene.h>

using namespace plugin;

struct Main
{
    Main()
    {   
		if (!inst.ReadIniOptions()) return;
#ifdef GTASA
        if (GetGameVersion() != GAME_10US_HOODLUM) return;
        injector::MakeJMP(0x4D5D10, CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed, true);

        // GTA VC and III do not have the IsCutsceneSkipButtonBeingPressed function
#elif GTAVC
        if (GetGameVersion() != GAME_10EN) return;

        
        injector::MakeInline<0x406242, 0x406244 + 5>([](injector::reg_pack& regs) {
            if (CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed()) {
                regs.eip = 0x406331;
            }
            else {
                regs.eip = 0x4063A0;
            }
        });
#elif GTA3
        if (GetGameVersion() != GAME_10EN) return;

        injector::MakeInline<0x405042, 0x405044 + 5>([](injector::reg_pack& regs) {
        if (CutsceneController::Hook_IsCutsceneSkipButtonBeingPressed()) {
            regs.eip = 0x405131;
        }
        else {
            regs.eip = 0x405136;
        }
        });
#endif 

        // It's possible to get the subtitles rendered in the cutscene in real time, but the game already shows this in the menu
        //injector::MakeCALL(0x5B187E, AddMessageHookCall, true); // for GTA SA

        static int txdSlot = -1;

        Events::initRwEvent += []() {
            txdSlot = CTxdStore::AddTxdSlot("cc_icons");

            if (CTxdStore::LoadTxd(txdSlot, PLUGIN_PATH("icons\\icons.txd"))) {
                CTxdStore::AddRef(txdSlot);
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(txdSlot);
                
                inst.pauseButton.SetTexture((char*)std::to_string(inst.pauseKey).c_str());
                if (inst.pauseButton.m_pTexture == nullptr) {
                    inst.pauseButton.SetTexture((char*)"120");
                }
				inst.vig.SetTexture((char*)"cuts_vignette");
                CTxdStore::PopCurrentTxd();
            }
            else {
                
            }

            inst.gsBlur = new GaussianBlur(); // init rasters
#ifdef GTASA
            RwD3D9CreatePixelShader((RwUInt32*)&blur_cso, (void**)&inst.gsBlur->blurShader);
#endif
        };
        
        Events::shutdownRwEvent += [] {
            inst.pauseButton.Delete();
            inst.vig.Delete();

            if (txdSlot != -1) {
                CTxdStore::RemoveTxdSlot(txdSlot);
                txdSlot = -1;
            }
			delete inst.gsBlur;
        };
        
        Events::gameProcessEvent += [] { inst.Update(); };
        
#ifdef GTASA
        Events::initScriptsEvent += [] { inst.LoadAudio(); };

        // I believe this is the best method, if something causes the cutscene audio to stop, it will be paused
        Events::onPauseAllSounds += [] {
            if ((IS_ON_ANY_CUTSCENE) && !CutsceneController::bCutscenePaused && inst.bFixAudioDesync) {
                inst.ChangeCutscenePause();
            }
        };

		Events::drawAfterFadeEvent += [] { 
            if (!FrontEndMenuManager.m_bMenuActive) {
                inst.DrawInterface();
            }
        };

        Events::drawingEvent.before += [=] {
            if (inst.bCutscenePaused && !inst.bFixedCam) {
                if (inst.bBlur) {
                    inst.gsBlur->DrawBlur_SA();
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
#endif 
       
#if defined(GTAVC) || defined(GTA3)
        static bool wasFocused = true;
        Events::gameProcessEvent += [] {
            if (!inst.bFixAudioDesync) return;

            bool focused = GetForegroundWindow() == RsGlobal.ps->window;

            if (!focused && !inst.bCutscenePaused && (IS_ON_ANY_CUTSCENE)) {
                inst.ChangeCutscenePause();
            }

            if (!focused && wasFocused) {
                inst.needBlurCapture = false;
            }

            if (focused && !wasFocused) {
                if (inst.bCutscenePaused) {
                    inst.needBlurCapture = true;
                }
            }
            wasFocused = focused;
        };
        
        Events::drawingEvent.after += [] {
            if (!FrontEndMenuManager.m_bMenuActive && IS_ON_ANY_CUTSCENE) {
				if (inst.needBlurCapture && inst.bBlur) {
                    RwCameraEndUpdate(Scene.m_pCamera);
                    RwRasterPushContext(inst.gsBlur->blurRaster);
                    RwRasterRenderFast(RwCameraGetRaster(Scene.m_pCamera), 0, 0);
                    RwRasterPopContext();
                    RwCameraBeginUpdate(Scene.m_pCamera);
					inst.needBlurCapture = false;
				}
                
                if (inst.bCutscenePaused && inst.bBlur && !inst.bFixedCam) {
                    inst.gsBlur->DrawBlur_VCorIII();
                }
                inst.DrawInterface();
            }
        };
#endif 
        
       
    }
} gInstance;
