FMOD is already installed at:
C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\

The Visual Studio project (persona battle.vcxproj) is configured to use that path.

Build steps:
1. Open persona battle.slnx in Visual Studio
2. Set platform to Win32 (x86) and configuration to Debug
3. Build the project
4. After build, fmod.dll is copied automatically next to persona battle.exe

If music does not play:
- Make sure assets/sound/battle_music.mp3 exists
- In project Properties -> Debugging -> Working Directory, set to:
  $(ProjectDir)
  so relative asset paths work when running from Visual Studio

If FMOD is installed somewhere else, edit FMODDir in persona battle.vcxproj.
