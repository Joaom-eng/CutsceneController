# CutsceneController

<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/5d19b7af-6924-4604-8029-1e98accf7425" />

Cutscene Controller is an ASI plugin that adds improvements and fixes to the game's cutscene system:
- Allows cutscenes to be paused
- Allows enabling or disabling mouse skip input
- Adds customizable pause and skip buttons, with gamepad support
- Fixes a major issue where minimizing the game during a cutscene would desynchronize the audio
- Fixes skip input detection that could trigger even when the game window was minimized or out of focus
- Allows custom audio to be played when pausing or resuming cutscenes
- Ensures CLEO scripts continue executing while cutscenes are paused, preventing interruptions caused by the game's `CTimer::m_UserPause` or `CTimer::m_CodePause` states
- Provides an exported function `IsCutscenePaused` that allows external plugins or CLEO scripts to detect whether a cutscene is currently paused
- Adds a simple pause interface

This mod uses the [plugin SDK](https://github.com/DK22Pac/plugin-sdk), thanks to all the contributors
