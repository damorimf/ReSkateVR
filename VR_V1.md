# ReSkate VR V1

Target: an installed, reproducible VR build for Quest 3 through SteamVR, based on
cesarcc2/ReSkateVR and current upstream ReSkate. This is a development candidate,
not a verified V1 release.

## Source baseline

- VR source: cesar/feature/openxr-vr, original tip a4f79ef.
- Upstream integration base: reskate-upstream/main, 719cba1.
- Original VR history retained locally at archive/cesar-vr-base.
- Development branch: codex/vr-v1.
- Game contract: Steam build 25414733; never bypass executable verification.

## Build and validation

Use Visual Studio 2022 Build Tools with the C++ desktop workload:

```powershell
cmake --preset vr-v1
cmake --build --preset vr-v1-release --parallel 4
ctest --test-dir build/vr-v1 -C Release --output-on-failure
```

This preset builds the VR and multiplayer regression tests, disables binary
auto-update for local development and disables uploads to upstream's crash account.
It explicitly disables the optional Rust Spotify Connect device so a clean VR
build does not depend on a pre-populated Cargo cache; external-player audio remains.
The VR source is already part of the Windows build; the historical
RESKATE_ENABLE_OPENXR_VR option is not used by this source tree.

## V1 completion gates

- [x] Rebased integration compiles with warnings as errors (local MSVC Release build).
- [x] VR pose, camera transition, camera cycle and multiplayer regressions pass (50/50 local CTest checks).
- [ ] Stereo disocclusion handling improved and checked in the headset.
- [x] Repeatable packaging includes the pinned x64 OpenXR loader and licenses.
- [x] Separate local install contains the supported game build and VR binaries.
- [ ] GitHub fork under damorimf has the development source and build checks.
- [ ] Quest 3 / SteamVR starts and displays correct stereo with stable tracking.
- [ ] First/third person, walking, grabs, bails, map changes and recenter tested.
- [ ] Menus, controller input, shutdown and restarting the game tested.
- [ ] Performance and comfort settings validated in actual gameplay.

## Changes under development

- Integrate all four original VR commits with current upstream features.
- Keep VR camera ownership through on-foot transitions and the R3 cycle.
- Smooth viewpoint changes before the existing ground/slope camera guard; headset
  tracking is applied after smoothing. Reset on teleports/recenter/tracking gaps.
- Fill stereo disocclusion from the background adjacent to the silhouette rather
  than unrelated farther scenery. A software-GPU test runs the actual shader for
  both eyes and fails against the original implementation.
- Repair upstream standalone probe and in-memory multiplayer test link dependencies.
- Track asset files rather than directory dependencies, avoiding repeated parallel
  CMake regeneration in Visual Studio builds.

## Local setup findings

Steam skate. is installed at C:\Program Files (x86)\Steam\steamapps\common\Skate.
Its executable does not match the supported game contract. An existing ReSkate
copy at C:\Program Files (x86)\Steam\steamapps\content\app_3354750\ReSkate has the
exact supported executable hash. Its game data was copied into
C:\Users\Daniel\Games\ReSkateVR, excluding its mod settings and mod folders.
SteamVR is the registered OpenXR runtime. GitHub account requested: damorimf.
Runtime playtesting requires the user's connected Quest 3.

## Runtime evidence (October 15, 2026)

The installed candidate passed exact game and Steam API identity checks, loaded
ReSkate.dll, initialized the DX12 overlay and created a live local skater.
With the user's Quest 3 connected, SteamVR/OpenXR 2.18.3 entered a focused session,
recognized Touch bindings and submitted depth-stereo eye images. Logs showed
about 60 game-camera updates/s and typically 115–120 presents/s at an 1800x1800
game image, with occasional dips. These counters do not establish visual comfort
or 120 unique game-camera frames/s. User testing of image quality, transitions,
recenter, controls, bails, map changes and sustained performance remains pending.

The installer was exercised against an unsupported-game fixture, a tampered
binary and a repeated install: rejection and backup checks passed.

## Packaging and installation

```powershell
./contrib/Package-VR.ps1
# Use the generated package directory printed by the packaging command:
./contrib/Install-VR.ps1 -PackageDirectory '<package directory>' -InstallDirectory "$env:USERPROFILE/Games/ReSkateVR" -PrepareGame
```

The package pins OpenXR loader 1.1.63 by SHA-256, includes licenses, and records
checksums for its files. The installer checks them before copying and backs up
existing mod binaries. `-PrepareGame` allows staging beside an unsupported game
so the launcher can repair it through Steam; it does not make that game compatible.
After the launcher obtains build 25414733, run the installer without that switch
to verify the game's executable too. Local builds keep automatic updates disabled.
