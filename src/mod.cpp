#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include "openrac/mod.hpp"
#include "logic.hpp"

namespace {
openrac::Mod* g = nullptr;
std::vector<std::uint8_t> snapshot;
std::uint32_t snapshot_loads = std::numeric_limits<std::uint32_t>::max();
std::uint32_t last_loads = std::numeric_limits<std::uint32_t>::max();
std::uint32_t settled = 0;
std::uint16_t previous = 0;
bool request_save = false;
bool request_load = false;
bool auto_level_start = true;

void notice(const std::string& s) {
    if (g != nullptr && g->has_notice()) g->play_notice(s);
}
void capture(const openrac_play_status& status) {
    if (status.phase != OPENRAC_PLAY_PLAYING) return;
    openrac_pose pose{}; pose.size = sizeof pose;
    if (!g->hero_pose(pose) || pose.health <= 0) return;
    const auto size = g->savestate_size();
    if (size == 0) return;
    snapshot.resize(size);
    g->savestate_save(snapshot.data(), snapshot.size());
    snapshot_loads = status.loads;
    notice("Checkpoint saved");
}
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac1") throw openrac::Error("Checkpoint Plus is for Ratchet & Clank");
    if (!mod.has_savestates() || !mod.has_play()) throw openrac::Error("Checkpoint Plus requires OpenRAC interface 1.8+");
    g = &mod;
    auto_level_start = mod.setting<bool>("auto_level_start");

    mod.on_pad([](int port, std::uint8_t* report, int size) {
        if (port != 0 || report == nullptr || size < 2) return;
        const auto now = qol::pad_buttons(report);
        const auto save_mask = static_cast<std::uint16_t>(OPENRAC_PAD_L1 | OPENRAC_PAD_R1 | OPENRAC_PAD_SQUARE);
        const auto load_mask = static_cast<std::uint16_t>(OPENRAC_PAD_L1 | OPENRAC_PAD_R1 | OPENRAC_PAD_TRIANGLE);
        if (qol::combo_pressed(now, previous, save_mask)) request_save = true;
        if (qol::combo_pressed(now, previous, load_mask)) request_load = true;
        previous = now;
    });

    mod.on_tick_boundary([](std::uint32_t) {
        const auto status = g->play_status();
        if (status.loads != last_loads) {
            last_loads = status.loads; settled = 0; snapshot.clear();
            snapshot_loads = std::numeric_limits<std::uint32_t>::max();
        }
        if (status.phase == OPENRAC_PLAY_PLAYING) ++settled;
        if (request_save) { request_save = false; capture(status); }
        if (auto_level_start && snapshot.empty() && settled == 120) capture(status);
        if (request_load) {
            request_load = false;
            if (snapshot.empty() || snapshot_loads != status.loads) notice("No checkpoint for this level");
            else {
                auto copy = snapshot;
                g->savestate_load(copy.data(), copy.size());
            }
        }
    });
    mod.info("L1+R1+Square saves; L1+R1+Triangle restores");
}
OPENRAC_MOD_CPP("checkpoint-plus", "0.1.0", load)
