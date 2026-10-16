// Uses the launcher's existing checked container reader/writer. Only requested
// graphics values are changed; original containers are backed up first.
#include "Launcher/game_settings.h"
#include "Extension/VR/vr_display_modes.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace graphics = dingosdk::launcher_game_settings;
namespace {
bool game_running() {
    const auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (handle == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot inspect running processes");
    PROCESSENTRY32W process{sizeof(PROCESSENTRY32W)};
    bool running = false;
    for (BOOL next = Process32FirstW(handle, &process); next; next = Process32NextW(handle, &process))
        if (_wcsicmp(process.szExeFile, L"Skate.exe") == 0) running = true;
    CloseHandle(handle);
    return running;
}
}
int main(int argc, char** argv) {
    try {
        int size = 2880;
        bool apply = false;
        const bool interactive = argc == 1;
        if (interactive) {
            std::cout << "ReSkate VR image setup\n\n1  Performance  2304 x 2304\n2  Balanced     2592 x 2592\n3  Sharp        2880 x 2880\n0  Exit\n\nChoose a profile: ";
            int choice = 0;
            std::cin >> choice;
            if (choice < 1 || choice > 3) return 0;
            size = choice == 1 ? 2304 : choice == 2 ? 2592 : 2880;
            apply = true;
        }
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--apply") apply = true;
            else if (argument.starts_with("--size=")) size = std::stoi(argument.substr(7));
            else throw std::runtime_error("Usage: ReSkateVRSetup.exe [--apply] [--size=2304|2592|2880]");
        }
        if (size != 2304 && size != 2592 && size != 2880) throw std::runtime_error("Choose 2304, 2592 or 2880");
        const auto saved = graphics::load(graphics::save_root());
        if (saved.folders.empty()) throw std::runtime_error("Launch ReSkate once to create game settings first");
        const auto modes = dingosdk::vr::display::windowed_catalog(static_cast<unsigned>(size));
        const auto found = std::find(modes.begin(), modes.end(), std::pair{static_cast<unsigned>(size), static_cast<unsigned>(size)});
        if (found == modes.end()) throw std::runtime_error("VR resolution is unavailable");
        const auto index = static_cast<std::int32_t>(std::distance(modes.begin(), found));
        graphics::Values changes{{"Resolution", index}, {"WindowedMode", std::int32_t{0}}};
        if (const auto hdr = saved.values.find("HDR"); hdr != saved.values.end())
            changes["HDR"] = std::holds_alternative<bool>(hdr->second) ? graphics::Value(false) : graphics::Value(std::int32_t{0});
        auto options = graphics::parse_options(std::get<std::string>(saved.values.at(graphics::options_key)));
        const bool performance = size == 2304;
        const std::pair<const char*, const char*> tuning[]{
            {"VSync", "v_sync_off"}, {"Frame Synthesis Mode", "frame_synthesis_mode_none"},
            {"Resolution Scale Enable", "resolution_scaling_off"}, {"Resolution", "resolution_scale_1"},
            {"Motion Blur", "motion_blur_off"}, {"Depth of Field", "depth_of_field_off"},
            {"Target Framerate", "target_uncapped_present_tltr"}, {"Global Graphics Quality", "Custom"},
            {"Texture Quality", "texture_quality_high"}, {"Texture Filtering", "texture_filtering_ultra"},
            {"Shadows", "shadows_medium"}, {"Effects Quality", "effects_quality_medium"},
            {"Post Process", "post_process_low"},
            {"GIBS", "gibs_off"},
            {"GI Grid", performance ? "gi_grid_medium" : "gi_grid_high"},
            {"Lighting", performance ? "lighting_medium" : "lighting_high"},
            {"Mesh Quality", performance ? "mesh_quality_medium" : "mesh_quality_high"},
            {"Ambient Occlusion", "ambient_occlusion_gtao_half"}
        };
        for (auto& option : options) for (const auto& [name, value] : tuning)
            if (option.name == name) option.value = value;
        changes[graphics::options_key] = graphics::join_options(options);
        std::cout << "VR image: " << size << 'x' << size << " (resolution index " << index << ")\n"
            << "Windowed, native resolution, HDR off, motion blur off, depth of field off.\n"
            << "Ray-traced GI off; lighting/mesh " << (performance ? "medium" : "high")
            << "; half-resolution ambient occlusion. Textures high.\n";
        if (!apply) { std::cout << "Inspection only. Add --apply to save these settings.\n"; return 0; }
        if (game_running()) throw std::runtime_error("Close skate. before applying graphics settings");
        const auto backup = graphics::save_root().parent_path().parent_path() / L"VR-settings-backups" /
            std::to_wstring(std::time(nullptr));
        graphics::save(saved, changes, backup);
        std::cout << "Settings saved. Original containers backed up at " << backup << '\n'
            << "Launch with --windowed --width=" << size << " --height=" << size << ".\n";
        if (interactive) {
            std::cout << "Press Enter to close.";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "VR setup: " << error.what() << '\n';
        return 1;
    }
}
