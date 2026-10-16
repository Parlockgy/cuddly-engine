#include "UiBase.hpp"
#include "Popups.hpp"
#include "Bridge.hpp"
#include "Util.hpp"
#include <fmt/format.h>
#include <random>
#include <sstream>
#include <tuple>

namespace ct {
namespace {

struct State {
    bool loaded = false;
    Params p;
    Palette cur;
    std::vector<Palette> history; int hpos = -1;
    std::vector<Palette> custom;  int cpos = -1;
    int sel = 0, channel = 1, prop = 0; // prop: 0 primary, 1 secondary, 2 both
} S;

struct ApplyUndo {
    bool valid = false, hadColor = false, painted = false;
    int ch = 0; ct::RGB old;
    std::vector<std::tuple<Ref<GameObject>, int, int>> objs; // object, old base, old detail
} g_undo;

void saveState() {
    auto m = Mod::get();
    m->setSavedValue<std::string>("palette-state", fmt::format("{} {} {} {} {} {} {} {} {} {}",
        S.p.harmony, S.p.preset, S.p.hueShift, S.p.satScale, S.p.valScale, S.p.contrast, S.p.seed, S.channel, S.prop, S.sel));
    m->setSavedValue<std::string>("custom-palettes", serialize(S.custom));
}
void regenerate(bool newSeed) {
    static std::mt19937 rd{std::random_device{}()};
    if (newSeed) S.p.seed = rd();
    S.cur = generate(S.p);
    if (newSeed) {
        S.history.push_back(S.cur);
        if (S.history.size() > 60) S.history.erase(S.history.begin());
        S.hpos = int(S.history.size()) - 1;
    } else if (S.hpos >= 0) S.history[S.hpos] = S.cur;
}
void ensureLoaded() {
    if (S.loaded) return;
    S.loaded = true;
    auto m = Mod::get();
    std::istringstream in(m->getSavedValue<std::string>("palette-state", ""));
    in >> S.p.harmony >> S.p.preset >> S.p.hueShift >> S.p.satScale >> S.p.valScale >> S.p.contrast >> S.p.seed >> S.channel >> S.prop >> S.sel;
    if (in.fail()) { S.p = Params{}; S.channel = 1; S.prop = 0; S.sel = 0; }
    S.p.preset = std::clamp(S.p.preset, 0, int(presets().size()) - 1);
    S.p.harmony = std::clamp(S.p.harmony, 0, int(HarmonyCount) - 1);
    S.sel = std::clamp(S.sel, 0, kRoles - 1);
    S.prop = std::clamp(S.prop, 0, 2);
    S.channel = std::clamp(S.channel, 1, 999);
    S.custom = deserialize(m->getSavedValue<std::string>("custom-palettes", ""));
    S.cpos = S.custom.empty() ? -1 : 0;
    S.cur = generate(S.p);
    S.history = {S.cur}; S.hpos = 0;
}

class PaletteLab : public CTPopup {
    CCLabelBMFont *m_harm, *m_preset, *m_hue, *m_sat, *m_val, *m_con, *m_hist, *m_cust, *m_status, *m_prop, *m_marker;
    std::array<CCLayerColor*, kRoles> m_sw{};
    std::array<CCLabelBMFont*, kRoles> m_hex{}, m_rgb{}, m_role{};
    TextInput *m_chan = nullptr, *m_hexIn = nullptr;

    CCLabelBMFont* row(CCPoint c, SEL_MenuHandler sel) {
        addBtn("<", {c.x - 78, c.y}, sel, .4f, -1);
        addBtn(">", {c.x + 78, c.y}, sel, .4f, +1);
        return addLabel("", c, .45f);
    }
public:
    static PaletteLab* create() {
        auto r = new PaletteLab();
        if (r && r->init()) { r->autorelease(); return r; }
        delete r; return nullptr;
    }
    bool init() {
        if (!initBase(480, 296, "Palette Lab")) return false;
        ensureLoaded();
        for (int i = 0; i < kRoles; ++i) {
            float x = -155 + i * 62;
            auto layer = CCLayerColor::create({0, 0, 0, 255}, 54, 36);
            layer->ignoreAnchorPointForPosition(false);
            m_sw[i] = layer;
            auto b = CCMenuItemSpriteExtra::create(layer, this, menu_selector(PaletteLab::onSwatch));
            b->setTag(i); b->setPosition({x, 100}); m_menu->addChild(b);
            m_role[i] = addLabel(roleName(i), {x, 74}, .3f, "bigFont.fnt");
            m_hex[i] = addLabel("", {x, 64}, .42f);
            m_rgb[i] = addLabel("", {x, 55}, .34f);
        }
        m_marker = addLabel("^", {0, 80}, .4f, "bigFont.fnt");

        m_harm   = row({-120, 36}, menu_selector(PaletteLab::onHarmony));
        m_preset = row({-120, 16}, menu_selector(PaletteLab::onPreset));
        m_hue    = row({-120, -4}, menu_selector(PaletteLab::onHue));
        m_sat    = row({-120, -24}, menu_selector(PaletteLab::onSat));
        m_val    = row({ 120, 36}, menu_selector(PaletteLab::onVal));
        m_con    = row({ 120, 16}, menu_selector(PaletteLab::onContrast));
        m_hist   = row({ 120, -4}, menu_selector(PaletteLab::onHistory));
        m_cust   = row({ 120, -24}, menu_selector(PaletteLab::onCustom));

        addBtn("Regenerate", {-190, -50}, menu_selector(PaletteLab::onRegen), .5f);
        addBtn("Save Custom", {-115, -50}, menu_selector(PaletteLab::onSave), .5f);
        addBtn("Delete Custom", {-35, -50}, menu_selector(PaletteLab::onDelete), .5f);
        addLabel("HEX:", {55, -50}, .42f);
        m_hexIn = TextInput::create(70.f, "#RRGGBB", "bigFont.fnt");
        m_hexIn->setFilter("#0123456789abcdefABCDEF");
        m_hexIn->setPosition(m_center + CCPoint{110, -50});
        m_mainLayer->addChild(m_hexIn, 6);
        addBtn("Set Swatch", {185, -50}, menu_selector(PaletteLab::onSetHex), .5f);

        addLabel("Channel (1-999):", {-175, -80}, .42f);
        m_chan = TextInput::create(50.f, "1", "bigFont.fnt");
        m_chan->setFilter("0123456789");
        m_chan->setString(std::to_string(S.channel));
        m_chan->setPosition(m_center + CCPoint{-110, -80});
        m_mainLayer->addChild(m_chan, 6);
        auto pb = addBtn("Property", {-45, -80}, menu_selector(PaletteLab::onProp), .45f);
        (void)pb;
        m_prop = addLabel("", {-45, -97}, .36f);
        addBtn("Paint Channel", {30, -80}, menu_selector(PaletteLab::onPaint), .5f, 0);
        addBtn("Assign + Paint", {115, -80}, menu_selector(PaletteLab::onPaint), .5f, 1);
        addBtn("Undo Apply", {190, -80}, menu_selector(PaletteLab::onUndoApply), .5f);

        m_status = addLabel("", {0, -122}, .4f);
        refresh();
        finishBase();
        return true;
    }

    void say(std::string const& s) { m_status->setString(s.c_str()); }

    void refresh() {
        for (int i = 0; i < kRoles; ++i) {
            auto c = S.cur.c[i];
            m_sw[i]->setColor({c.r, c.g, c.b});
            m_hex[i]->setString(hexStr(c).c_str());
            m_rgb[i]->setString(rgbStr(c).c_str());
        }
        m_marker->setPositionX(m_center.x - 155 + S.sel * 62);
        m_harm->setString(fmt::format("Harmony: {}", harmonyName(S.p.harmony)).c_str());
        m_preset->setString(fmt::format("Preset: {}", presets()[S.p.preset].name).c_str());
        m_hue->setString(fmt::format("Hue shift: {:.0f}", S.p.hueShift).c_str());
        m_sat->setString(fmt::format("Saturation: {:.1f}x", S.p.satScale).c_str());
        m_val->setString(fmt::format("Brightness: {:.1f}x", S.p.valScale).c_str());
        m_con->setString(fmt::format("Contrast: {:.1f}", S.p.contrast).c_str());
        m_hist->setString(fmt::format("History: {}/{}", S.hpos + 1, S.history.size()).c_str());
        m_cust->setString(S.custom.empty() ? "Custom: none"
            : fmt::format("Custom {}/{}: {}", S.cpos + 1, S.custom.size(), S.custom[S.cpos].name).c_str());
        static const char* pn[] = {"Primary color", "Secondary color", "Both"};
        m_prop->setString(fmt::format("Apply to: {}", pn[S.prop]).c_str());
        saveState();
    }
    static int dir(CCObject* s) { return static_cast<CCNode*>(s)->getTag(); }
    void changed() { regenerate(false); refresh(); }

    void onSwatch(CCObject* s) { S.sel = dir(s); refresh(); }
    void onHarmony(CCObject* s) { S.p.harmony = (S.p.harmony + dir(s) + HarmonyCount) % HarmonyCount; changed(); }
    void onPreset(CCObject* s) {
        int n = int(presets().size());
        S.p.preset = (S.p.preset + dir(s) + n) % n;
        S.p.harmony = presets()[S.p.preset].harmony;
        changed();
    }
    void onHue(CCObject* s) { S.p.hueShift = wrapHue(S.p.hueShift + 15.f * dir(s)); changed(); }
    void onSat(CCObject* s) { S.p.satScale = std::clamp(S.p.satScale + .1f * dir(s), .2f, 1.5f); changed(); }
    void onVal(CCObject* s) { S.p.valScale = std::clamp(S.p.valScale + .1f * dir(s), .4f, 1.3f); changed(); }
    void onContrast(CCObject* s) { S.p.contrast = std::clamp(S.p.contrast + .1f * dir(s), 0.f, 1.f); changed(); }
    void onRegen(CCObject*) { regenerate(true); refresh(); say("New palette generated."); }
    void onHistory(CCObject* s) {
        S.hpos = std::clamp(S.hpos + dir(s), 0, int(S.history.size()) - 1);
        S.cur = S.history[S.hpos]; refresh();
    }
    void onCustom(CCObject* s) {
        if (S.custom.empty()) { say("No custom palettes saved yet."); return; }
        int n = int(S.custom.size());
        S.cpos = (S.cpos + dir(s) + n) % n;
        S.cur = S.custom[S.cpos]; refresh();
    }
    void onSave(CCObject*) {
        Palette p = S.cur;
        p.name = fmt::format("Custom {}", S.custom.size() + 1);
        S.custom.push_back(p); S.cpos = int(S.custom.size()) - 1;
        refresh(); say("Saved as " + p.name + ".");
    }
    void onDelete(CCObject*) {
        if (S.custom.empty()) { say("Nothing to delete."); return; }
        S.custom.erase(S.custom.begin() + S.cpos);
        S.cpos = S.custom.empty() ? -1 : std::clamp(S.cpos, 0, int(S.custom.size()) - 1);
        refresh(); say("Custom palette deleted.");
    }
    void onSetHex(CCObject*) {
        RGB c;
        if (!parseHex(m_hexIn->getString(), c)) { say("Invalid HEX. Use 6 hex digits, e.g. #FF3366."); return; }
        S.cur.c[S.sel] = c;
        if (S.hpos >= 0) S.history[S.hpos] = S.cur;
        refresh(); say(fmt::format("{} set to {}.", roleName(S.sel), hexStr(c)));
    }
    void onProp(CCObject*) { S.prop = (S.prop + 1) % 3; refresh(); }

    void onPaint(CCObject* s) {
        bool assign = static_cast<CCNode*>(s)->getTag() == 1;
        int ch = std::atoi(m_chan->getString().c_str());
        if (ch < 1 || ch > 999) { say("Channel must be 1-999 (custom channels only)."); return; }
        S.channel = ch;
        auto color = S.cur.c[S.sel];
        ApplyUndo u; u.ch = ch;
        size_t applied = 0, skipped = 0;
        if (assign) {
            auto sel = bridge::selectedObjects();
            if (sel.empty()) { say("Select objects in the editor first."); return; }
            for (auto o : sel) {
                auto i = bridge::inspect(o);
                if (i.base < 0) { say("Unsupported: object color channel members not in bindings."); return; }
                bool wantBase = S.prop == 0 || S.prop == 2;
                bool wantDetail = (S.prop == 1 || S.prop == 2) && i.detail > 0; // no secondary => skip
                if (!wantBase && !wantDetail) { ++skipped; continue; }
                u.objs.emplace_back(o, i.base, i.detail);
                bridge::setObjectChannels(o, wantBase ? ch : 0, wantDetail ? ch : 0);
                ++applied;
            }
            bridge::refreshColors(sel);
        }
        ct::RGB old;
        u.hadColor = bridge::getChannelColor(ch, old);
        u.old = old;
        u.painted = bridge::setChannelColor(ch, color);
        u.valid = u.painted || !u.objs.empty();
        g_undo = std::move(u);
        std::string msg = assign ? fmt::format("Assigned channel {} to {} objects ({} skipped). ", ch, applied, skipped) : "";
        msg += g_undo.painted ? fmt::format("Channel {} painted {}.", ch, hexStr(color))
                              : fmt::format("Channel {} not painted: missing in level or API unsupported (create it in the color menu).", ch);
        say(msg);
        saveState();
    }
    void onUndoApply(CCObject*) {
        if (!g_undo.valid) { say("Nothing to undo."); return; }
        std::vector<GameObject*> touched;
        for (auto& [o, b, d] : g_undo.objs) { bridge::setObjectChannels(o, b, d); touched.push_back(o.data()); }
        if (g_undo.painted && g_undo.hadColor) bridge::setChannelColor(g_undo.ch, g_undo.old);
        bridge::refreshColors(touched);
        say(fmt::format("Reverted {} objects{}.", touched.size(),
                        g_undo.painted && !g_undo.hadColor ? "; channel color could not be restored (unknown previous value)" : ""));
        g_undo = ApplyUndo{};
    }
};
}
void openPaletteLab() { if (auto p = PaletteLab::create()) p->show(); }
}
