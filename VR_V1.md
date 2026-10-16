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
- [x] VR pose, camera transition, camera cycle, saved graphics settings and multiplayer regressions pass (52 local CTest checks).
- [ ] Stereo disocclusion handling improved and checked in the headset.
- [x] Repeatable packaging includes the pinned x64 OpenXR loader and licenses.
- [x] Separate local install contains the supported game build and VR binaries.
- [x] GitHub fork under damorimf has source and passing build/test/package checks (89a8b45a).
- [ ] Quest 3 / SteamVR starts and displays correct stereo with stable tracking.
- [ ] First/third person, walking, grabs, bails, map changes and recenter tested.
- [ ] Menus, controller input, shutdown and restarting the game tested.
- [ ] Performance and comfort settings validated in actual gameplay.
- [ ] User confirms clear game/menu text and real positional leaning in the headset.

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

## Clarity and usability iteration

The first headset test confirmed head rotation and Touch input but the user
reported blur, a 3DoF feeling, visible body geometry and unreadable/janky menus.
This is not accepted as V1 quality.

- The saved image was 1800x1800 (index 16); the OpenXR recommendation was
  2528x2704 per eye. Motion blur and depth of field were enabled.
- ReSkateVRSetup.exe provides 2304, 2592 and 2880 square profiles. It uses the
  launcher's existing container writer, backs up original settings and sets native
  resolution, windowed mode, motion blur/DOF/HDR/upscaling off. It lowers shadows,
  effects and post processing to leave GPU time for the sharper image.
- The VR menu uses separate Setup, Camera, Image, Input and Advanced pages, with
  fonts rasterized at twice the desktop size and a larger responsive layout.
- Tracking diagnostics report valid/tracked positional input, displacement from
  recenter and translation actually applied to the camera. Position was already
  enabled in the saved profile, so the reported 3DoF feeling requires another test.
  Tracking dropouts now hold the last valid translation instead of resetting it.
- NxRoot FeetOnly 1.31.2 is installed in the local VR copy alongside the user's added mods.
  Source: https://thunderstore.io/c/reskate/p/NxRoot/FeetOnly/
  Download SHA-256: 6096ca422872716f3419f8867e3e84278839457e59df8bd5fa7d1e9e11cd7506.
  The mod is a separate dependency, not bundled into the VR binary package.
- Local test configuration: DualSense (Touch gameplay input off), positional
  tracking on, world scale 1, Feet only on board and on foot, larger menu panel.

## Comfort/performance follow-up

The second Quest 3 test confirmed clearer visuals and working 6DoF, but the user
reported camera shake, stutter, board motion and a partially visible body. This
candidate is not yet headset-validated for comfort or frame pacing.

- Steady camera (enabled by default, `vr.comfort_camera`) follows smoothed travel
  heading and body origin instead of animated head spins/sway. It holds a
  calibrated eye height, filters crouch, and leaves real headset motion untouched.
- Depth stereo reuses completed eye images between new game camera records,
  avoiding repeated depth reprojection while retaining every OpenXR submission.
- FeetOnly uses head-only joint hiding when its costume is available, avoiding
  redundant waist shrinking that could distort descendant joints or the board.
- All 52 local regression tests pass, including animation rejection, immediate
  locomotion/headset lean, and frame-rate-independent crouch filtering.
- Next local test disables bail/grab camera changes, uses Feet only on board and
  on foot, and lowers the requested square image size from 2880 to 2304. Actual
  backbuffer dimensions must be checked: previous requests produced images 25%
  larger, consistent with a DPI scaling issue that remains under investigation.

## Follow camera and rendering diagnostics

The next headset test confirmed that shoes-only now works, but head turns still
stutter. Logs at 2880x2880 show about 60 camera updates/s and 95-107 presents/s,
short of the headset's 120 Hz target. Two-eye image counts increase by about 600
per five seconds, confirming duplicate depth draws are skipped. The first launch
exited during loading without a captured fault; the retry reached gameplay and
later recorded a normal native shutdown. This does not yet establish stability.

- F9 switches first person / third-person follow camera. The VR Setup page also
  has a switch button. `vr.view_key` changes the shortcut (120 = F9, 0 disables).
- FeetOnly applies only in first person; third person restores the original live
  outfit, and switching back reapplies the stored feet-only choice.
- Optional GPU timestamps measure the actual two-eye depth draw, shown on Setup
  and in logs. Readback uses an already-completed allocator fence, adding no new
  CPU wait. The metric excludes the game, copies, depth dilation and compositor.
- A D3D12 WARP integration test runs copy/dilation/two-eye draws over repeated
  allocator reuse and checks completed timing and destroy/recreate behavior.
- Requested 2304 square was verified to produce a physical 2880 square image on
  this PC. The desktop shortcut and launcher now request 2304; the setup helper's
  size semantics still need a portable fix before release.

## Predictable resolution and GPU budget

The installed follow-camera candidate remains under test. A subsequent candidate
uses a fixed square VR resolution catalog shared by setup and DXGI enumeration
when launched windowed with square dimensions. This removes dependence on monitor
mode ordering: 2304 square is index 4, 2592 index 5, and 2880 index 6. Other launches
keep their existing display-mode behavior. Actual dimensions still require a
launch test; the earlier 25% size mismatch did not establish DPI as its cause.

The saved game options also showed ray-traced GI enabled, ultra lighting/mesh,
and full-resolution GTAO. VR setup now disables ray-traced GI, uses medium lighting,
mesh and GI grid for Performance (high for Balanced/Sharp), and half-resolution
GTAO while retaining high textures, native resolution and ultra texture filtering.
These changes need a headset performance/appearance comparison.

A local synthetic run of the actual stereo renderer on the RX 7900 XTX measured
0.79 ms at 2880 square and 0.66 ms at 2304 square for both eye draws. It used
uniform depth 0.02, scale 0.12, maximum shift 0.018, and excludes game rendering,
copies and the compositor. It is not a gameplay timing result.

## DPI follow-up and clean CI evidence

GitHub Actions run [38828708580](https://github.com/damorimf/ReSkateVR/actions/runs/38828708580)
built 89a8b45a on a clean Windows runner, passed all 52 tests, and uploaded the
43,774,888-byte development artifact (artifact ID 12017077496).

The 89a8b45a candidate was installed and its Performance settings verified on disk.
With Quest connected, it reached focused VR gameplay with tracked 6DoF. Actual
image size remained 2880 square despite a 2304-square catalog selection and logical
client size. Live stereo draw timing was usually 0.65-0.9 ms; presents fluctuated
around 60-84/s with about 60 camera updates/s. This remains below the intended
120 Hz budget; no comfort/performance acceptance is claimed.

A subsequent patch establishes per-monitor DPI awareness for square windowed VR
launches while the launcher still holds the game suspended, before HWND creation.
The game manifest declares no DPI awareness. Windows documents that the process
setting must precede window creation. The patch logs success/error and system DPI;
actual game image dimensions and menu appearance require another launch test.
Reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiawarenesscontext

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
