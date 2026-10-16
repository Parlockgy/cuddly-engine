
#include "UiBase.hpp"
#include "Popups.hpp"
#include "Bridge.hpp"

#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

// Duplicate Cleaner / Optimizer.
//
// Geometry Dash does not provide a general operation that fuses two
// objects into one. This tool never claims to merge objects.
//
// It only removes objects whose complete serialized state is identical
// to another object that is kept, and only for object kinds that
// removableKind() explicitly considers safe.
//
// Benefit: reduced object count. No FPS improvement is guaranteed.

namespace ct {
namespace {

struct Rep {
    bool valid = false;
    bool selectionScope = true;

    size_t examined = 0;
    size_t protectedCount = 0;
    size_t uniqueKinds = 0;
    size_t removable = 0;
    size_t groups = 0;
    size_t triggers = 0;
    size_t sameSpot = 0;

    std::map<std::string, size_t> why;
    std::map<int, size_t> perId;
    std::vector<Ref<GameObject>> toRemove;
};

Rep g_rep;
std::vector<Ref<GameObject>> g_origSel;

struct UndoInfo {
    bool valid = false;
    int depthAfter = -1;
    size_t before = 0;
};

UndoInfo g_undo;

Rep analyze(bool selectionOnly) {
    using namespace bridge;

    Rep r;
    r.selectionScope = selectionOnly;

    if (!editorLayer()) {
        return r;
    }

    r.valid = true;

    auto objs = selectionOnly ? selectedObjects() : allObjects();
    r.examined = objs.size();

    std::unordered_map<std::string, std::vector<GameObject*>> byKey;

    std::map<
        std::tuple<int, long, long>,
        std::set<std::string>
    > spots;

    for (auto o : objs) {
        if (!o) {
            continue;
        }

        std::string why;

        if (!removableKind(o, why)) {
            r.protectedCount++;
            r.why[why]++;
            continue;
        }

        auto key = saveString(o);

        if (key.empty()) {
            r.protectedCount++;
            r.why["Save string API unavailable"]++;
            continue;
        }

        auto p = o->getPosition();

        spots[{
            o->m_objectID,
            std::lround(p.x * 100),
            std::lround(p.y * 100)
        }].insert(key);

        byKey[key].push_back(o);
    }

    r.uniqueKinds = byKey.size();

    for (auto& [key, objects] : byKey) {
        if (objects.size() < 2) {
            continue;
        }

        r.groups++;

        // Keep the first object and mark the remaining exact duplicates.
        for (size_t i = 1; i < objects.size(); ++i) {
            r.toRemove.push_back(objects[i]);

            int id = objects[i]->m_objectID;
            r.perId[id]++;

            if (id == 899 || id == 1007 || id == 1049) {
                r.triggers++;
            }
        }
    }

    r.removable = r.toRemove.size();

    for (auto& [position, keys] : spots) {
        if (keys.size() > 1) {
            r.sameSpot++;
        }
    }

    return r;
}

std::vector<std::string> describe(Rep const& r) {
    std::vector<std::string> lines;

    if (!r.valid) {
        lines.push_back(
            "No analysis yet (editor not available or not run)."
        );
        return lines;
    }

    lines.push_back(fmt::format(
        "Scope: {}",
        r.selectionScope ? "current selection" : "whole level"
    ));

    lines.push_back(fmt::format(
        "Objects examined: {}",
        r.examined
    ));

    lines.push_back(fmt::format(
        "Distinct removable-kind objects kept: {}",
        r.uniqueKinds
    ));

    lines.push_back(fmt::format(
        "Protected, never touched: {}",
        r.protectedCount
    ));

    for (auto& [reason, count] : r.why) {
        lines.push_back(fmt::format(
            "   - {}: {}",
            reason,
            count
        ));
    }

    lines.push_back(fmt::format(
        "Exact duplicates removable: {} (in {} groups)",
        r.removable,
        r.groups
    ));

    lines.push_back(fmt::format(
        "   of which redundant Color/Alpha/Toggle triggers: {}",
        r.triggers
    ));

    lines.push_back(fmt::format(
        "Same-spot objects with different properties (kept): {}",
        r.sameSpot
    ));

    std::vector<std::pair<int, size_t>> top(
        r.perId.begin(),
        r.perId.end()
    );

    std::sort(
        top.begin(),
        top.end(),
        [](auto const& a, auto const& b) {
            return a.second > b.second;
        }
    );

    std::string summary = "Duplicates by ID: ";

    for (size_t i = 0; i < top.size() && i < 6; ++i) {
        summary += fmt::format(
            "{}x{}  ",
            top[i].first,
            top[i].second
        );
    }

    if (!top.empty()) {
        lines.push_back(summary);
    }

    lines.push_back(fmt::format(
        "Expected result: {} -> {} objects",
        r.examined,
        r.examined >= r.removable
            ? r.examined - r.removable
            : 0
    ));

    lines.push_back(
        "Limits: no object fusion exists in GD; benefit = lower object count only."
    );

    lines.push_back(
        "No FPS gain is claimed. Move/Pulse/Spawn/etc. triggers are never removed."
    );

    return lines;
}

void saveReport(std::vector<std::string> const& lines) {
    std::string text;

    for (auto const& line : lines) {
        text += line + "\n";
    }

    Mod::get()->setSavedValue<std::string>(
        "last-report",
        text
    );

    std::ofstream file(
        Mod::get()->getSaveDir() / "last-optimizer-report.txt"
    );

    if (file) {
        file << text;
    }
}

bool sameSet(Rep const& a, Rep const& b) {
    if (a.toRemove.size() != b.toRemove.size()) {
        return false;
    }

    std::set<GameObject*> objects;

    for (auto& object : a.toRemove) {
        objects.insert(object.data());
    }

    for (auto& object : b.toRemove) {
        if (!objects.count(object.data())) {
            return false;
        }
    }

    return true;
}

class Optimizer : public CTPopup {
    CCLabelBMFont* m_status = nullptr;

public:
    static Optimizer* create() {
        auto popup = new Optimizer();

        if (popup && popup->init()) {
            popup->autorelease();
            return popup;
        }

        delete popup;
        return nullptr;
    }

    bool init() {
        if (!initBase(
            440,
            290,
            "Optimizer / Duplicate Cleaner"
        )) {
            return false;
        }

        addBtn(
            "Analyze Selection",
            {-130, 112},
            menu_selector(Optimizer::onAnalyze),
            .5f,
            1
        );

        addBtn(
            "Analyze Level",
            {-30, 112},
            menu_selector(Optimizer::onAnalyze),
            .5f,
            0
        );

        addBtn(
            "Copy Report",
            {60, 112},
            menu_selector(Optimizer::onCopy),
            .5f
        );

        addBtn(
            "Preview",
            {-165, -100},
            menu_selector(Optimizer::onPreview),
            .55f
        );

        addBtn(
            "Restore Selection",
            {-95, -100},
            menu_selector(Optimizer::onRestoreSel),
            .5f
        );

        addBtn(
            "Apply",
            {-15, -100},
            menu_selector(Optimizer::onApply),
            .6f
        );

        addBtn(
            "Undo",
            {45, -100},
            menu_selector(Optimizer::onUndo),
            .55f
        );

        addBtn(
            "Close",
            {110, -100},
            menu_selector(CTPopup::onClose),
            .55f
        );

        m_status = addLabel(
            "",
            {0, -125},
            .42f
        );

        showReport(g_rep);

        finishBase();
        return true;
    }

    void say(std::string const& message) {
        if (m_status) {
            m_status->setString(message.c_str());
        }
    }

    // This is deliberately not named show(), because CTPopup already
    // declares show() with no arguments.
    void showReport(Rep const& report) {
        auto lines = describe(report);

        setLines(
            lines,
            {-205, 90},
            11.f,
            .45f
        );

        saveReport(lines);
    }

    void onAnalyze(CCObject* sender) {
        if (!sender) {
            say("Unable to determine analysis scope.");
            return;
        }

        bool selectionOnly =
            static_cast<CCNode*>(sender)->getTag() == 1;

        g_rep = analyze(selectionOnly);

        if (selectionOnly && g_rep.examined == 0) {
            say(
                "Nothing selected. Select objects first, or use Analyze Level."
            );
        } else {
            say(
                g_rep.valid
                    ? "Analysis done. Nothing has been changed."
                    : "Editor not available."
            );
        }

        showReport(g_rep);
    }

    void onCopy(CCObject*) {
        std::string text;

        for (auto const& line : describe(g_rep)) {
            text += line + "\n";
        }

        geode::utils::clipboard::write(text);
        say("Report copied to clipboard.");
    }

    void onPreview(CCObject*) {
        if (!g_rep.valid || g_rep.toRemove.empty()) {
            say("Run an analysis with candidates first.");
            return;
        }

        g_origSel.clear();

        for (auto object : bridge::selectedObjects()) {
            g_origSel.push_back(object);
        }

        std::vector<GameObject*> objects;

        for (auto& object : g_rep.toRemove) {
            if (object) {
                objects.push_back(object.data());
            }
        }

        if (objects.empty()) {
            say("No valid duplicate objects remain to preview.");
            return;
        }

        if (!bridge::selectOnly(objects)) {
            say("Could not select duplicate objects.");
            return;
        }

        this->onClose(nullptr);

        Notification::create(
            fmt::format(
                "{} duplicates selected (preview). Reopen Optimizer to apply.",
                objects.size()
            ),
            NotificationIcon::Info
        )->show();
    }

    void onRestoreSel(CCObject*) {
        std::vector<GameObject*> objects;

        for (auto& object : g_origSel) {
            if (object) {
                objects.push_back(object.data());
            }
        }

        if (!bridge::selectOnly(objects)) {
            say("Could not restore the previous selection.");
            return;
        }

        say(fmt::format(
            "Selection restored ({} objects).",
            objects.size()
        ));
    }

    void onApply(CCObject*) {
        if (!g_rep.valid || g_rep.toRemove.empty()) {
            say("Nothing to apply. Analyze first.");
            return;
        }

        if (!Mod::get()->getSettingValue<bool>(
            "confirm-optimization"
        )) {
            doApply();
            return;
        }

        createQuickPopup(
            "Confirm removal",
            fmt::format(
                "Remove {} exact duplicate objects ({} -> {})?\n\nUndo is available afterwards.",
                g_rep.removable,
                g_rep.examined,
                g_rep.examined >= g_rep.removable
                    ? g_rep.examined - g_rep.removable
                    : 0
            ),
            "Cancel",
            "Remove",
            [this](FLAlertLayer*, bool yes) {
                if (yes) {
                    this->doApply();
                }
            }
        );
    }

    void doApply() {
        auto fresh = analyze(g_rep.selectionScope);

        if (!fresh.valid || !sameSet(fresh, g_rep)) {
            g_rep = fresh;
            showReport(g_rep);

            say(
                "Level changed since analysis. Report refreshed; review it and Apply again."
            );
            return;
        }

        size_t before = bridge::allObjects().size();

        std::vector<GameObject*> objects;

        for (auto& object : g_rep.toRemove) {
            if (object) {
                objects.push_back(object.data());
            }
        }

        if (objects.empty()) {
            say("No valid duplicate objects remain.");
            return;
        }

        if (!bridge::selectOnly(objects)) {
            say("Could not select duplicate objects. Nothing was deleted.");
            return;
        }

        if (!bridge::deleteSelected()) {
            say("Editor deletion failed. Check the level before continuing.");
            return;
        }

        size_t after = bridge::allObjects().size();

        g_undo = {
            true,
            bridge::undoDepth(),
            before
        };

        bool success =
            after <= before &&
            before - after == objects.size();

        g_rep = analyze(g_rep.selectionScope);
        showReport(g_rep);

        if (success) {
            say(fmt::format(
                "Removed {} duplicates ({} -> {}). Verified.",
                objects.size(),
                before,
                after
            ));
        } else {
            say(fmt::format(
                "WARNING: expected {} objects, found {}. Check the editor and Undo.",
                before >= objects.size()
                    ? before - objects.size()
                    : 0,
                after
            ));
        }
    }

    void onUndo(CCObject*) {
        if (!g_undo.valid) {
            say("Nothing to undo from this tool.");
            return;
        }

        int depth = bridge::undoDepth();

        if (depth < 0 || depth != g_undo.depthAfter) {
            say(
                "Cannot verify the last editor action is ours. Use the editor's own Undo button."
            );
            return;
        }

        if (!bridge::undoLast()) {
            say("The editor could not undo the last action.");
            return;
        }

        size_t now = bridge::allObjects().size();
        size_t expected = g_undo.before;

        g_undo.valid = false;
        g_rep = Rep{};

        showReport(g_rep);

        if (now == expected) {
            say(fmt::format(
                "Undone: back to {} objects. Verified.",
                now
            ));
        } else {
            say(fmt::format(
                "Undo ran, but count is {} (expected {}). Check the editor.",
                now,
                expected
            ));
        }
    }
};

} // namespace

void openOptimizer() {
    if (auto popup = Optimizer::create()) {
        popup->show();
    }
}

} // namespace ct

