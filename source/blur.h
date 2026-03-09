#pragma once

static void DrawBlurStep(RwRaster* raster, float offsetX, float offsetY, unsigned char alpha)
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
