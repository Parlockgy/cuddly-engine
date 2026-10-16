#include "UiBase.hpp"
#include "Popups.hpp"
#include "Bridge.hpp"
#include <fmt/format.h>
#include <fstream>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>

// Duplicate Cleaner / Optimizer.
// GD has no operation that fuses two objects into one, so objects are NEVER "merged".
// The only transformation implemented is removing objects whose full serialized state
// (GameObject save string: ID, position, rotation, scale, flips, groups, colors, z-order/layer,
// editor layers, ...) is byte-identical to another object that is kept, and only for
// object kinds whose duplicate is provably redundant. Benefit: lower object count only.
namespace ct {
namespace {

struct Rep {
    bool valid = false, selectionScope = true;
    size_t examined = 0, protectedCount = 0, uniqueKinds = 0, removable = 0, groups = 0, triggers = 0, sameSpot = 0;
    std::map<std::string, size_t> why;
    std::map<int, size_t> perId;
    std::vector<Ref<GameObject>> toRemove;
};

Rep g_rep;
std::vector<Ref<GameObject>> g_origSel;
struct UndoInfo { bool valid = false; int depthAfter = -1; size_t before = 0; } g_undo;

Rep analyze(bool selectionOnly) {
    using namespace bridge;
    Rep r;
    r.selectionScope = selectionOnly;
    if (!editorLayer()) return r;
    r.valid = true;
    auto objs = selectionOnly ? selectedObjects() : allObjects();
    r.examined = objs.size();

    std::unordered_map<std::string, std::vector<GameObject*>> byKey;
    std::map<std::tuple<int, long, long>, std::set<std::string>> spots;
    for (auto o : objs) {
        std::string why;
        if (!removableKind(o, why)) { r.protectedCount++; r.why[why]++; continue; }
        auto key = saveString(o);
        if (key.empty()) { r.protectedCount++; r.why["Save string API unavailable"]++; continue; }
        auto p = o->getPosition();
        spots[{o->m_objectID, std::lround(p.x * 100), std::lround(p.y * 100)}].insert(key);
        byKey[key].push_back(o);
    }
    r.uniqueKinds = byKey.size();
    for (auto& [k, v] : byKey) {
        if (v.size() < 2) continue;
        r.groups++;
        for (size_t i = 1; i < v.size(); ++i) {
            r.toRemove.push_back(v[i]);
            int id = v[i]->m_objectID;
            r.perId[id]++;
            if (id == 899 || id == 1007 || id == 1049) r.triggers++;
        }
    }
    r.removable = r.toRemove.size();
    for (auto& [k, s] : spots) if (s.size() > 1) r.sameSpot++;
    return r;
}

std::vector<std::string> describe(Rep const& r) {
    std::vector<std::string> L;
    if (!r.valid) { L.push_back("No analysis yet (editor not available or not run)."); return L; }
    L.push_back(fmt::format("Scope: {}", r.selectionScope ? "current selection" : "whole level"));
    L.push_back(fmt::format("Objects examined: {}", r.examined));
    L.push_back(fmt::format("Distinct removable-kind objects kept: {}", r.uniqueKinds));
    L.push_back(fmt::format("Protected, never touched: {}", r.protectedCount));
    for (auto& [w, n] : r.why) L.push_back(fmt::format("   - {}: {}", w, n));
    L.push_back(fmt::format("Exact duplicates removable: {}  (in {} groups)", r.removable, r.groups));
    L.push_back(fmt::format("   of which redundant Color/Alpha/Toggle triggers: {}", r.triggers));
    L.push_back(fmt::format("Same-spot objects with different properties (kept): {}", r.sameSpot));
    std::vector<std::pair<int, size_t>> top(r.perId.begin(), r.perId.end());
    std::sort(top.begin(), top.end(), [](auto& a, auto& b) { return a.second > b.second; });
    std::string t = "Duplicates by ID: ";
    for (size_t k = 0; k < top.size() && k < 6; ++k) t += fmt::format("{}x{}  ", top[k].first, top[k].second);
    if (!top.empty()) L.push_back(t);
    L.push_back(fmt::format("Expected result: {} -> {} objects", r.examined, r.examined - r.removable));
    L.push_back("Limits: no object fusion exists in GD; benefit = lower object count only.");
    L.push_back("No FPS gain is claimed. Move/Pulse/Spawn/etc. triggers are never removed.");
    return L;
}

void saveReport(std::vector<std::string> const& lines) {
    std::string text;
    for (auto& l : lines) text += l + "\n";
    Mod::get()->setSavedValue<std::string>("last-report", text);
    std::ofstream f(Mod::get()->getSaveDir() / "last-optimizer-report.txt");
    if (f) f << text;
}

bool sameSet(Rep const& a, Rep const& b) {
    if (a.toRemove.size() != b.toRemove.size()) return false;
    std::set<GameObject*> s;
    for (auto& o : a.toRemove) s.insert(o.data());
    for (auto& o : b.toRemove) if (!s.count(o.data())) return false;
    return true;
}

class Optimizer : public CTPopup {
    CCLabelBMFont* m_status = nullptr;
public:
    static Optimizer* create() {
        auto r = new Optimizer();
        if (r && r->init()) { r->autorelease(); return r; }
        delete r; return nullptr;
    }
    bool init() {
        if (!initBase(440, 290, "Optimizer / Duplicate Cleaner")) return false;
        addBtn("Analyze Selection", {-130, 112}, menu_selector(Optimizer::onAnalyze), .5f, 1);
        addBtn("Analyze Level", {-30, 112}, menu_selector(Optimizer::onAnalyze), .5f, 0);
        addBtn("Copy Report", {60, 112}, menu_selector(Optimizer::onCopy), .5f);
        addBtn("Preview", {-165, -100}, menu_selector(Optimizer::onPreview), .55f);
        addBtn("Restore Selection", {-95, -100}, menu_selector(Optimizer::onRestoreSel), .5f);
        addBtn("Apply", {-15, -100}, menu_selector(Optimizer::onApply), .6f);
        addBtn("Undo", {45, -100}, menu_selector(Optimizer::onUndo), .55f);
        addBtn("Close", {110, -100}, menu_selector(CTPopup::onClose), .55f);
        m_status = addLabel("", {0, -125}, .42f);
        setLines(describe(g_rep), {-205, 90}, 11.f, .45f);
        finishBase();
        return true;
    }
    void say(std::string const& s) { m_status->setString(s.c_str()); }
    void show(Rep const& r) {
        auto lines = describe(r);
        setLines(lines, {-205, 90}, 11.f, .45f);
        saveReport(lines);
    }

    void onAnalyze(CCObject* s) {
        bool sel = static_cast<CCNode*>(s)->getTag() == 1;
        g_rep = analyze(sel);
        if (sel && g_rep.examined == 0) say("Nothing selected. Select objects first, or use Analyze Level.");
        else say(g_rep.valid ? "Analysis done. Nothing has been changed." : "Editor not available.");
        show(g_rep);
    }
    void onCopy(CCObject*) {
        std::string t;
        for (auto& l : describe(g_rep)) t += l + "\n";
        geode::utils::clipboard::write(t);
        say("Report copied to clipboard.");
    }
    void onPreview(CCObject*) {
        if (!g_rep.valid || g_rep.toRemove.empty()) { say("Run an analysis with candidates first."); return; }
        g_origSel.clear();
        for (auto o : bridge::selectedObjects()) g_origSel.push_back(o);
        std::vector<GameObject*> v;
        for (auto& o : g_rep.toRemove) v.push_back(o.data());
        bridge::selectOnly(v);
        this->onClose(nullptr); // so the highlighted objects are visible; reopen to Apply
        Notification::create(fmt::format("{} duplicates selected (preview). Reopen Optimizer to apply.", v.size()),
                             NotificationIcon::Info)->show();
    }
    void onRestoreSel(CCObject*) {
        std::vector<GameObject*> v;
        for (auto& o : g_origSel) v.push_back(o.data());
        bridge::selectOnly(v);
        say(fmt::format("Selection restored ({} objects).", v.size()));
    }
    void onApply(CCObject*) {
        if (!g_rep.valid || g_rep.toRemove.empty()) { say("Nothing to apply. Analyze first."); return; }
        if (!Mod::get()->getSettingValue<bool>("confirm-optimization")) { doApply(); return; }
        createQuickPopup("Confirm removal",
            fmt::format("Remove {} exact duplicate objects ({} -> {})?\n\nUndo is available afterwards.",
                        g_rep.removable, g_rep.examined, g_rep.examined - g_rep.removable),
            "Cancel", "Remove",
            [this](FLAlertLayer*, bool yes) { if (yes) this->doApply(); });
    }
    void doApply() {
        // Re-analyze: the level may have changed since the report. Abort on any mismatch.
        auto fresh = analyze(g_rep.selectionScope);
        if (!fresh.valid || !sameSet(fresh, g_rep)) {
            g_rep = fresh; show(g_rep);
            say("Level changed since analysis. Report refreshed; review it and Apply again.");
            return;
        }
        size_t before = bridge::allObjects().size();
        std::vector<GameObject*> v;
        for (auto& o : g_rep.toRemove) v.push_back(o.data());
        bridge::selectOnly(v);
        bridge::deleteSelected();        // editor's own delete => native undo entry
        size_t after = bridge::allObjects().size();
        g_undo = {true, bridge::undoDepth(), before};
        bool ok = after == before - v.size();
        g_rep = analyze(g_rep.selectionScope);
        show(g_rep);
        say(ok ? fmt::format("Removed {} duplicates ({} -> {}). Verified.", v.size(), before, after)
               : fmt::format("WARNING: expected {} objects, found {}. Use Undo.", before - v.size(), after));
    }
    void onUndo(CCObject*) {
        if (!g_undo.valid) { say("Nothing to undo from this tool."); return; }
        int d = bridge::undoDepth();
        if (d < 0 || d != g_undo.depthAfter) {
            say("Cannot verify the last editor action is ours. Use the editor's own Undo button.");
            return;
        }
        bridge::undoLast();
        size_t now = bridge::allObjects().size();
        g_undo.valid = false;
        g_rep = Rep{};
        show(g_rep);
        say(now == g_undo.before ? fmt::format("Undone: back to {} objects. Verified.", now)
                                 : fmt::format("Undo ran, but count is {} (expected {}). Check the editor.", now, g_undo.before));
    }
};
}
void openOptimizer() { if (auto p = Optimizer::create()) p->show(); }
}
