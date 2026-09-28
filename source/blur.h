#pragma once
#ifdef GTASA
	#include <d3d9.h>
#endif 

static RwIm2DVertex* colorfilterVerts = (RwIm2DVertex*)0xC400D8;
static RwImVertexIndex* colorfilterIndices = (RwImVertexIndex*)0x8D5174;

class GaussianBlur {
public:
	void* blurShader = nullptr;
	bool bInitShaderAndRasters = false;
	RwRaster* blurRaster = nullptr;
	RwRaster* lowRaster = nullptr;
	RwRaster* pingPongRaster = nullptr;
	RwCamera* lowCamera = nullptr;
	RwCamera* pingPongCamera = nullptr;
	RpWorld* auxiliaryCameraWorld = nullptr;

	GaussianBlur();
	~GaussianBlur();

	void CreateRasters();
	void DestroyRasters();
	void reloadRasters();
	void DrawBlur_SA();
	void DrawBlur_VCorIII();

	void DrawRasterToCurrentTarget(RwRaster* raster, float targetWidth, float targetHeight,
		float uOffset, float vOffset, unsigned char alpha, bool additive,
		float maxU = 1.0f, float maxV = 1.0f);
	void DrawPingPongBlur();
};

extern unsigned char blur_cso[1100];
