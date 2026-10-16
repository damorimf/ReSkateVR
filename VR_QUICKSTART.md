# ReSkate VR

Development build for Quest 3 through SteamVR. Head tracking, positional leaning
and feet-only first person have been confirmed in the headset. Performance,
follow-camera switching and sustained stability are still being validated.

## Install

Use a separate ReSkate game folder containing supported Steam build **25414733**.
Close that copy of the game and launcher, extract the VR package, then run:

```powershell
./Install-VR.ps1 -InstallDirectory 'C:\Users\YourName\Games\ReSkateVR'
```

The installer checks the game and package hashes and backs up replaced binaries
under `VR-backups`. Your `Mods` folder is preserved. The package contains the VR
runtime, launcher, OpenXR loader and setup helper; it does not contain game data.
If you need the supported game build, use `-PrepareGame` to stage the launcher,
run it to obtain the supported build through Steam, then run the installer again.

## First launch

1. Close the game and run **ReSkateVRSetup.exe**. Start with **Performance**.
   It selects native square rendering, high textures, lighter lighting/geometry,
   and disables motion blur, depth of field and ray-traced global illumination.
   Graphics settings are shared by ReSkate installations; original containers are
   backed up under `%LOCALAPPDATA%\ReSkate\Game\VR-settings-backups`.
2. Connect the Quest through Steam Link and leave SteamVR running.
3. Start the VR copy with a square, windowed launch:

   ```powershell
   ./ReSkateLauncher.exe --no-update --windowed --width=2304 --height=2304
   ```

4. Open the ReSkate menu with **Insert**, select **VR**, and enable VR on Setup.
   On Input, choose **Gamepad / DualSense** or **Touch**. Close menus to skate.
5. Face forward in a comfortable position and press **F8** to recenter. Leaning
   should move the view independently of the skater. Setup shows tracking status
   and the actual image dimensions; check these after changing image presets.

## Camera controls

| Control | Action |
| --- | --- |
| F8 | Recenter your head position and direction |
| F9 | Switch first person / third-person follow camera |
| VR Setup switch button | Switch first person / follow camera from the menu |
| VR Camera | Adjust follow distance/height, steadiness and body visibility |

Keep the game focused for keyboard shortcuts. Third person follows the skater's
travel direction and shows their outfit. Returning to first person restores the
chosen body visibility. Real headset movement remains active in both modes.

For shoes-only first person, install the separate
[NxRoot FeetOnly mod](https://thunderstore.io/c/reskate/p/NxRoot/FeetOnly/) in this
copy's `Mods` folder. On VR Camera, select **Feet only** for both board and on-foot
visibility. The mod changes the live costume for first person and restores the
outfit for third person. It does not replace the saved outfit.

## If head turns stutter

Start with Performance and check the actual game image dimensions on VR Setup.
The game-camera update rate and headset submission rate are separate; a high
submission rate does not mean the game has that many unique camera frames.
Setup also reports GPU time for the mod's two-eye draw; game rendering, copies
and the compositor add to that time. Pairing and FOV calibration remain under
validation for this development build.

Logs are in the VR game folder at `logs\ReSkate.log`. They record the image size,
camera/present rates, tracking, stereo GPU time and errors. If VR cannot start,
check that SteamVR sees the connected Quest rather than a disconnected headset.

Development evidence and remaining release gates: [VR_V1.md](VR_V1.md).
