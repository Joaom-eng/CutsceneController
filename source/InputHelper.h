#pragma once 
#include <plugin.h>
#include <string>
#include <unordered_map>

using namespace std;

enum GamepadButton
{
	None,
	Cross, 
	Circle,
	Square,
	Triangle,
	L1,
	R1,
	L2,
	R2,
	Start,
	Select,
	DPadUp,
	DPadDown,
	DPadLeft,
	DPadRight
};

static unordered_map<string, GamepadButton> buttonMap = {
	{"Cross", GamepadButton::Cross},  
	{"Circle", GamepadButton::Circle},
	{"Square", GamepadButton::Square},
	{"Triangle", GamepadButton::Triangle},
	{"L1", GamepadButton::L1},
	{"L2", GamepadButton::L2},
	{"R1", GamepadButton::R1},
	{"R2", GamepadButton::R2},
	{"Start", GamepadButton::Start},
	{"Select", GamepadButton::Select},
	{"DPadUp", GamepadButton::DPadUp},
	{"DPadDown", GamepadButton::DPadDown},
	{"DPadLeft", GamepadButton::DPadLeft},
	{"DPadRight", GamepadButton::DPadRight}
};

static GamepadButton GetButtonFromString(const std::string& str)
{
	auto it = buttonMap.find(str);
	if (it != buttonMap.end())
		return it->second;

	return GamepadButton::None;
}

static signed short IsButtonPressed(GamepadButton btn, CControllerState state)
{
	switch (btn)
	{
	case GamepadButton::Cross:		return state.ButtonCross;
	case GamepadButton::Circle:		return state.ButtonCircle;
	case GamepadButton::Square:		return state.ButtonSquare;
	case GamepadButton::Triangle:	return state.ButtonTriangle;
	case GamepadButton::L1:			return state.LeftShoulder1;
	case GamepadButton::L2:			return state.LeftShoulder2;
	case GamepadButton::R1:			return state.RightShoulder1;
	case GamepadButton::R2:			return state.RightShoulder2;
	case GamepadButton::Start:		return state.Start;
	case GamepadButton::Select:		return state.Select;
	case GamepadButton::DPadUp:     return state.DPadUp;
	case GamepadButton::DPadDown:     return state.DPadDown;
	case GamepadButton::DPadRight:     return state.DPadRight;
	case GamepadButton::DPadLeft:     return state.DPadLeft;
	default: return false;
	}
}