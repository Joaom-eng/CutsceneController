#include <plugin.h>
#include <CPostEffects.h>
#include <CScene.h>

#include "CutsceneController.h"
#include "blur.h"
#include "text.h"

// blur simulation
void GaussianBlur::DrawSimulatedBlurStep(RwRaster* raster, float offsetX, float offsetY, unsigned char alpha)
{
	float width = (float)RwRasterGetWidth(raster);
	float height = (float)RwRasterGetHeight(raster);
	float d3dOffset = -0.5f;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwIm2DVertex verts[4];
	float z = 0.0f;
	float rhw = 1.0f;

	// Coordinates with the blur offset applied over the D3D offset
	float x1 = 0.0f + d3dOffset + offsetX;
	float y1 = 0.0f + d3dOffset + offsetY;
	float x2 = width + d3dOffset + offsetX;
	float y2 = height + d3dOffset + offsetY;

	// Moving vertices
	RwIm2DVertexSetScreenX(&verts[0], x1);
	RwIm2DVertexSetScreenY(&verts[0], y1);
	RwIm2DVertexSetU(&verts[0], 0.0f, rhw);
	RwIm2DVertexSetV(&verts[0], 0.0f, rhw);

	RwIm2DVertexSetScreenX(&verts[1], x2);
	RwIm2DVertexSetScreenY(&verts[1], y1);
	RwIm2DVertexSetU(&verts[1], 1.0f, rhw);
	RwIm2DVertexSetV(&verts[1], 0.0f, rhw);

	RwIm2DVertexSetScreenX(&verts[2], x1);
	RwIm2DVertexSetScreenY(&verts[2], y2);
	RwIm2DVertexSetU(&verts[2], 0.0f, rhw);
	RwIm2DVertexSetV(&verts[2], 1.0f, rhw);

	RwIm2DVertexSetScreenX(&verts[3], x2);
	RwIm2DVertexSetScreenY(&verts[3], y2);
	RwIm2DVertexSetU(&verts[3], 1.0f, rhw);
	RwIm2DVertexSetV(&verts[3], 1.0f, rhw);

	for (int i = 0; i < 4; i++) {
		RwIm2DVertexSetScreenZ(&verts[i], z);
		RwIm2DVertexSetRecipCameraZ(&verts[i], rhw);
		RwIm2DVertexSetIntRGBA(&verts[i], 255, 255, 255, alpha);
	}

	RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts, 4);
}

GaussianBlur::GaussianBlur() {
	RwRaster* camRaster = RwCameraGetRaster(Scene.m_pCamera);
	float w = RwRasterGetWidth(CPostEffects::pRasterFrontBuffer);
	float h = RwRasterGetHeight(CPostEffects::pRasterFrontBuffer);

	blurRaster = RwRasterCreate(
		w,
		h,
		camRaster->depth,
		rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888
	);

	float x = 0.7f, y = 0.7f;
	FixAspectRatio(&x, &y);

	int lowW = ((int)(w * x) >> 4) << 4;
	int lowH = ((int)(h * y) >> 4) << 4;
	lowRaster = RwRasterCreate(
		lowW,
		lowH,
		camRaster->depth,
		rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888
	);

	blurTexture = RwTextureCreate(blurRaster);
	bInitShaderAndRasters = true;
}

void GaussianBlur::reloadRasters() {
	RwRasterDestroy(blurRaster);
	RwRasterDestroy(lowRaster);
	GaussianBlur();
}

void GaussianBlur::DrawBlur() {
	if (KeyPressed(inst.disableBlurInGameKey) || !CPostEffects::pRasterFrontBuffer || !Scene.m_pCamera)
		return;

	RwRaster* camRaster = RwCameraGetRaster(Scene.m_pCamera);
	float w = RwRasterGetWidth(CPostEffects::pRasterFrontBuffer);
	float h = RwRasterGetHeight(CPostEffects::pRasterFrontBuffer);

	if (w != blurRaster->width || h != blurRaster->height || camRaster->depth != blurRaster->depth) {
		reloadRasters();
		return;
	}

	ImmediateModeRenderStatesStore();
	ImmediateModeRenderStatesSet();
	if (inst.bUsePixelShader) {
		if (bInitShaderAndRasters) {
			RwRect rect = { 0, 0, lowRaster->width, lowRaster->height };
		
			RwRGBA clearColor = { 0, 0, 0, 255 };
			RwCameraClear(RwCameraGetCurrentCamera(), &clearColor, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

			RwD3D9SetIm2DPixelShader((void*)blurShader);
			
			float imgSize[4] = { lowRaster->width, lowRaster->height, 0, 0 };
			RwD3D9SetPixelShaderConstant(0, &imgSize, 1);

			RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)lowRaster);
			RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
			RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
			
			for (int i = 0; i < 4; i++) {
				colorfilterVerts[i].emissiveColor = 0xFFFFFFFF; // It's necessary to configure the colors, not doing so will leave the screen black for some people (yes, not everyone, which is kind of strange)
			}

			// horizontal pass
			float offH[4] = { inst.fBlurIntensity, 0.0f, 0.0f, 0.0f };
			RwD3D9SetPixelShaderConstant(1, &offH, 1);

			RwRasterPushContext(lowRaster);
			RwRasterRenderScaled(blurRaster, &rect);
			RwRasterPopContext();

			// vertical pass
			float offV[4] = { 0.0f, inst.fBlurIntensity, 0.0f, 0.0f };
			RwD3D9SetPixelShaderConstant(1, &offV, 1);

			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, colorfilterVerts, 4, colorfilterIndices, 6);
			RwD3D9SetIm2DPixelShader(nullptr);
		}
		else {
			reloadRasters();
		}
	}
	else {
		if (bInitShaderAndRasters) {
			float blurIntensity = inst.fBlurIntensity;
			unsigned char alpha = inst.blurAlpha;

			RwRGBA clearColor = { 0, 0, 0, 255 };
			RwCameraClear(RwCameraGetCurrentCamera(), &clearColor, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

			// 4 diagonal passes(simulates Gaussian shader)
			DrawSimulatedBlurStep(blurRaster, -blurIntensity, -blurIntensity, alpha);
			DrawSimulatedBlurStep(blurRaster, blurIntensity, -blurIntensity, alpha);
			DrawSimulatedBlurStep(blurRaster, -blurIntensity, blurIntensity, alpha);
			DrawSimulatedBlurStep(blurRaster, blurIntensity, blurIntensity, alpha);

			// More cardinal steps to increase intensity
			DrawSimulatedBlurStep(blurRaster, 0, blurIntensity * 1.5f, alpha);
			DrawSimulatedBlurStep(blurRaster, 0, -blurIntensity * 1.5f, alpha);
		}
	}
	ImmediateModeRenderStatesReStore();
}