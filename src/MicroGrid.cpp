#include "UiBase.hpp"
#include "Popups.hpp"
#include "Bridge.hpp"
#include <fmt/format.h>
#include <cstdlib>

// Micro Grid. Mechanism (UNVERIFIED): EditorUI::m_gridSize + EditorUI::updateGridNodeSize().
// If either is missing from the bindings, the grid-size controls report "unsupported".
// "Snap selection" is independent: it moves selected objects with EditorUI::moveObject
// (also UNVERIFIED) to the nearest multiple of the chosen step, with its own revert.
namespace ct {
namespace {

struct SnapRec { Ref<GameObject> o; CCPoint delta; };
std::vector<SnapRec> g_snap;

float savedStep() { return Mod::get()->getSavedValue<float>("grid-step", 30.f); }

class MicroGrid : public CTPopup {
    CCLabelBMFont* m_cur = nullptr;
    CCLabelBMFont* m_status = nullptr;
    TextInput* m_input = nullptr;
public:
    static MicroGrid* create() {
        auto r = new MicroGrid();
        if (r && r->init()) { r->autorelease(); return r; }
        delete r; return nullptr;
    }
    bool init() {
        if (!initBase(400, 260, "Micro Grid")) return false;
        m_cur = addLabel("", {0, 85}, .55f);
        addLabel("Grid presets (GD units; 30 = one block)", {0, 62}, .42f);
        static const float steps[] = {30.f, 15.f, 7.5f, 3.75f, 1.875f, 1.f, 0.5f, 60.f};
        for (int i = 0; i < 8; ++i) {
            float x = -135 + (i % 4) * 90, y = 38 - (i / 4) * 26;
            auto b = addBtn(fmt::format("{}", steps[i]).c_str(), {x, y}, menu_selector(MicroGrid::onPreset), .5f);
            b->setUserObject(CCFloat::create(steps[i]));
        }
        addLabel("Custom step:", {-110, -24}, .45f);
        m_input = TextInput::create(70.f, "e.g. 2.5", "bigFont.fnt");
        m_input->setFilter(".0123456789");
        m_input->setPosition(m_center + CCPoint{-35, -24});
        m_mainLayer->addChild(m_input, 6);
        addBtn("Set", {20, -24}, menu_selector(MicroGrid::onCustom), .5f);
        addBtn("Snap Selection To Grid", {-70, -58}, menu_selector(MicroGrid::onSnap), .5f);
        addBtn("Revert Snap", {50, -58}, menu_selector(MicroGrid::onRevert), .5f);
        addBtn("Close", {140, -58}, menu_selector(CTPopup::onClose), .5f);
        m_status = addLabel("", {0, -90}, .4f);
        refresh();
        finishBase();
        return true;
    }
    void say(std::string const& s) { m_status->setString(s.c_str()); }
    void refresh() {
        float v = 0;
        bool ok = bridge::getGridStep(v);
        m_cur->setString((ok ? fmt::format("Current editor grid: {} (saved: {})", v, savedStep())
                             : fmt::format("Grid API unavailable (saved: {})", savedStep())).c_str());
    }
    bool applyStep(float v) {
        if (v < 0.1f || v > 240.f) { say("Step must be between 0.1 and 240."); return false; }
        if (!bridge::setGridStep(v)) { say("Unsupported: m_gridSize/updateGridNodeSize not in bindings."); return false; }
        Mod::get()->setSavedValue<float>("grid-step", v);
        refresh();
        say(fmt::format("Grid set to {}. Persisted; re-applied on editor open if enabled.", v));
        return true;
    }
    void onPreset(CCObject* s) {
        auto f = static_cast<CCFloat*>(static_cast<CCNode*>(s)->getUserObject());
        if (f) applyStep(f->getValue());
    }
    void onCustom(CCObject*) {
        float v = std::strtof(m_input->getString().c_str(), nullptr);
        applyStep(v);
    }
    void onSnap(CCObject*) {
        auto sel = bridge::selectedObjects();
        if (sel.empty()) { say("Select objects first."); return; }
        if (!Mod::get()->getSettingValue<bool>("confirm-optimization")) { doSnap(); return; }
        createQuickPopup("Snap selection",
            fmt::format("Move {} objects to the nearest {} unit grid point?\nThis changes positions (use Revert Snap to undo).", sel.size(), savedStep()),
            "Cancel", "Snap", [this](FLAlertLayer*, bool yes) { if (yes) this->doSnap(); });
    }
    void doSnap() {
        float step = savedStep();
        if (step <= 0) return;
        g_snap.clear();
        int moved = 0;
        for (auto o : bridge::selectedObjects()) {
            auto p = o->getPosition();
            CCPoint t{std::round(p.x / step) * step, std::round(p.y / step) * step};
            CCPoint d{t.x - p.x, t.y - p.y};
            if (std::fabs(d.x) < 1e-4f && std::fabs(d.y) < 1e-4f) continue;
            if (!bridge::moveObject(o, d)) { say("Unsupported: EditorUI::moveObject not in bindings."); return; }
            g_snap.push_back({o, d});
            ++moved;
        }
        say(fmt::format("Snapped {} objects (others already on grid).", moved));
    }
    void onRevert(CCObject*) {
        if (g_snap.empty()) { say("No snap to revert."); return; }
        int n = 0;
        for (auto& r : g_snap) if (bridge::moveObject(r.o, {-r.delta.x, -r.delta.y})) ++n;
        g_snap.clear();
        say(fmt::format("Reverted {} objects.", n));
    }
};
}
void openMicroGrid() { if (auto p = MicroGrid::create()) p->show(); }
}
