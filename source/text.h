#pragma once

#include <CFont.h>
#include <CMessages.h>

static void FixAspectRatio(float* x, float* y)
{
	float resX = (float)RsGlobal.maximumWidth;
	float resY = (float)RsGlobal.maximumHeight;
	resY *= 1.33333333f;
	resX /= resY;

	*x /= resX;
	*y /= 1.07142857f;
}

static void Draw_String(const char* m_text, float m_fX, float m_fY, float m_fWidth, float m_fHeight, bool m_nFixAR, eFontStyle m_nStyle, eFontAlignment m_nAlign) {
	char finalString[400];
	//CRGBA m_Color = CRGBA(255, 255, 255 , 230);
	float sizX = m_fWidth;
	float sizY = m_fHeight;

	if (m_nFixAR) {
		FixAspectRatio(&sizX, &sizY);
	}

	float magicResolutionWidth = RsGlobal.maximumWidth * 0.0015625f;
	float magicResolutionHeight = RsGlobal.maximumHeight * 0.002232143f;

	CFont::SetScale(sizX * magicResolutionWidth, sizY * magicResolutionHeight);
	CFont::SetFontStyle(m_nStyle);
	CFont::SetProportional(true);
	CFont::SetJustify(false);
	CFont::SetOrientation(m_nAlign);
	CFont::SetEdge(inst.pauseTextBorder);

	if (inst.bSetShadow) {
		CFont::SetDropColor(inst.pauseTextDropColor);
		CFont::SetDropShadowPosition(inst.pauseTextDropPos);
	}
	
	CFont::SetBackground(false, false);

	/*
	if (m_nOutline || m_nShadow)
	{
		if (m_nOutline)

		else
			CFont::SetDropShadowPosition(m_nShadow);
		CFont::SetDropColor(m_DropColor);
	}
	else
	{
		CFont::SetEdge(0);
		CFont::SetDropShadowPosition(0);
	}
	if (m_nBackground)
	{
		CFont::SetBackground(print->m_nBackground, false);
		CFont::SetBackgroundColor(print->m_BackColor);
	}
	*/

	CFont::SetColor(inst.pauseTextColor);
	switch (m_nAlign)
	{
	case ALIGN_LEFT:
		CFont::SetWrapx(640.0f * magicResolutionWidth); break;
	case ALIGN_CENTER:
		CFont::SetCentreSize(640.0f * magicResolutionWidth); break;
	case ALIGN_RIGHT:
		CFont::SetRightJustifyWrap(640.0f * magicResolutionWidth); break;
	}

	CMessages::InsertNumberInString(m_text, -1, -1, -1, -1, -1, -1, finalString); // this is required to use InsertPlayerControlKeysInString, idkw
	CMessages::InsertPlayerControlKeysInString(finalString);
	CFont::PrintString(m_fX * magicResolutionWidth, m_fY * magicResolutionHeight, m_text);
}