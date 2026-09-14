#pragma once
#ifdef GTASA
	#include <d3d9.h>
#endif 
typedef bool(__thiscall* FuncState)();

static FuncState ImmediateModeRenderStatesStore = (FuncState)0x700CC0;
static FuncState ImmediateModeRenderStatesSet = (FuncState)0x700D70;
static FuncState ImmediateModeRenderStatesReStore = (FuncState)0x700E00;

static RwIm2DVertex* colorfilterVerts = (RwIm2DVertex*)0xC400D8;
static RwImVertexIndex* colorfilterIndices = (RwImVertexIndex*)0x8D5174;

class GaussianBlur {
public:
	void* blurShader;
	bool bInitShaderAndRasters = false;
	RwRaster* blurRaster;
	RwRaster* lowRaster;

	GaussianBlur();
	~GaussianBlur();

	void CreateRasters();
	void reloadRasters();
	void DrawSimulatedBlur();
	void DrawBlur_SA();
	void DrawBlur_VCorIII();

	void DrawSimulatedBlurStep(RwRaster* raster, float offsetX, float offsetY, unsigned char alpha);
};

extern unsigned char blur_cso[1100];