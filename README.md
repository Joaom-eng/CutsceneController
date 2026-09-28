# CutsceneController

<img width="1918" height="1079" alt="image" src="https://github.com/user-attachments/assets/be40c3ba-3553-47fa-ab45-05e987e11a2e" />

Cutscene Controller is an ASI plugin that adds improvements and fixes to the game's cutscene system:
- Allows cutscenes to be paused
- Allows enabling or disabling mouse skip input
- Adds customizable pause and skip buttons, with gamepad support
- Fixes a major issue where minimizing the game during a cutscene would desynchronize the audio
- Fixes skip input detection that could trigger even when the game window was minimized or out of focus
- Allows custom audio to be played when pausing or resuming cutscenes
- Ensures CLEO scripts continue executing while cutscenes are paused, preventing interruptions caused by the game's `CTimer::m_UserPause` or `CTimer::m_CodePause` states
- Provides an exported function `IsCutscenePaused` that allows external plugins or CLEO scripts to detect whether a cutscene is currently paused
- Adds a shader-free simulated blur effect when a cutscene is paused
- GTA San Andreas also supports a real Gaussian pixel shader
- Adds a simple pause interface
- It allows you to move the camera freely during the scene, something like [this](https://www.mixmods.com.br/2018/11/assistir-cutscenes-em-outros-angulos-mod/) but without bugs

## Supported Games

| Game | Supported executable |
|---|---|
| GTA San Andreas | 1.0 US Hoodlum |
| GTA Vice City | 1.0 English |
| GTA III | 1.0 English |

## Feature Compatibility

| Feature | GTA SA | GTA VC | GTA III |
|---|---:|---:|---:|
| Cutscene pause | Yes | Yes | Yes |
| Free camera | Yes | Yes | Yes |
| Simulated blur | Yes | Yes | Yes |
| Gaussian pixel shader | Yes | No | No |
| Custom pause/resume sounds | Yes | No | No |

## Controls

| Action | Default control | INI option |
|---|---|---|
| Pause or resume the cutscene | `F9` | `PauseKey` |
| Enable or disable free camera | `SHIFT` | `ToggleKey` |
| Move camera forward | `W` | `FrontKey` |
| Move camera backward | `S` | `BackKey` |
| Look around | Mouse movement | `Sensitivity` |
| Increase or decrease camera speed | Mouse wheel | `InitialSpeed` |
| Skip the cutscene | Game controls or configured keys | `SkipKey` / `SkipButton` |
| Temporarily disable blur | Not assigned | `DisableBlurInGameKey` |
> [!NOTE]
> Controls use Windows virtual-key codes in `CutsceneController.ini`.
> See the [Microsoft virtual-key code reference](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) when assigning keyboard controls.
>
> Some cutscene behavior depends on the original game scripts and may vary between missions

## Installation
1. Extract the `CutsceneController` folder into the game's `modloader` directory.
2. Choose the English or Portuguese configuration file.
3. Edit `CutsceneController.ini` to customize the controls and interface.

This mod uses the [plugin SDK](https://github.com/DK22Pac/plugin-sdk), thanks to all the contributors
