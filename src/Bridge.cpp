#include "Bridge.hpp"

namespace ct::bridge {

namespace {
    // ---- optional-API helpers: each returns "unsupported" when the member is missing ----
    template <class O> void groupsOf(O* o, std::vector<int>& out) {
        if constexpr (requires { (*o->m_groups)[0]; o->m_groupCount; }) { // UNVERIFIED member names
            if (o->m_groups) for (int i = 0; i < int(o->m_groupCount) && i < 10; ++i) out.push_back((*o->m_groups)[i]);
        }
    }
    template <class O> int baseOf(O* o) {
        if constexpr (requires { o->m_baseColor->m_colorID; }) return o->m_baseColor ? int(o->m_baseColor->m_colorID) : 0; // UNVERIFIED
        else return -1;
    }
    template <class O> int detailOf(O* o) {
        if constexpr (requires { o->m_detailColor->m_colorID; }) return o->m_detailColor ? int(o->m_detailColor->m_colorID) : 0; // UNVERIFIED
        else return -1;
    }
    template <class O> void setBase(O* o, int ch) {
        if constexpr (requires { o->m_baseColor->m_colorID = ch; }) { if (o->m_baseColor) o->m_baseColor->m_colorID = ch; }
    }
    template <class O> void setDetail(O* o, int ch) {
        if constexpr (requires { o->m_detailColor->m_colorID = ch; }) { if (o->m_detailColor) o->m_detailColor->m_colorID = ch; }
    }
    template <class O, class L> std::string saveStr(O* o, L* l) {
        if constexpr (requires { std::string(o->getSaveString(l)); }) return std::string(o->getSaveString(l)); // UNVERIFIED signature
        else return {};
    }
    template <class L> int undoCount(L* l) {
        if constexpr (requires { l->m_undoObjects->count(); }) return l && l->m_undoObjects ? int(l->m_undoObjects->count()) : -1; // UNVERIFIED
        else return -1;
    }
    template <class M> bool paintOn(M* m, int ch, ct::RGB c) {
        if constexpr (requires { m->getColorAction(ch)->m_color; }) { // UNVERIFIED
            auto a = m->getColorAction(ch);
            if (!a) return false;
            a->m_color = {c.r, c.g, c.b};
            if constexpr (requires { a->m_fromColor; }) a->m_fromColor = {c.r, c.g, c.b};
            return true;
        } else return false;
    }
    template <class M> bool readOn(M* m, int ch, ct::RGB& c) {
        if constexpr (requires { m->getColorAction(ch)->m_color.r; }) { // UNVERIFIED
            auto a = m->getColorAction(ch);
            if (!a) return false;
            c = {a->m_color.r, a->m_color.g, a->m_color.b};
            return true;
        } else return false;
    }
    template <class L> bool paintChannel(L* l, int ch, ct::RGB c) {
        bool ok = false;
        if constexpr (requires { l->m_effectManager; }) ok = paintOn(l->m_effectManager, ch, c) || ok;
        // The level's saved start colours may live on the settings object's own effect manager.
        if constexpr (requires { l->m_levelSettings->m_effectManager; }) {
            if (l->m_levelSettings && l->m_levelSettings->m_effectManager
                && (!requires { l->m_effectManager; } || l->m_levelSettings->m_effectManager != l->m_effectManager))
                ok = paintOn(l->m_levelSettings->m_effectManager, ch, c) || ok;
        }
        return ok;
    }
    template <class L> bool readChannel(L* l, int ch, ct::RGB& c) {
        if constexpr (requires { l->m_effectManager; }) return readOn(l->m_effectManager, ch, c);
        else return false;
    }
    template <class L> void refreshOn(L* l, CCArray* arr) {
        if constexpr (requires { l->updateObjectColors(arr); }) l->updateObjectColors(arr); // UNVERIFIED
    }
    template <class U> bool gridGet(U* ui, float& v) {
        if constexpr (requires { v = ui->m_gridSize; }) { v = ui->m_gridSize; return true; } // UNVERIFIED
        else return false;
    }
    template <class U> bool gridSet(U* ui, float v) {
        if constexpr (requires { ui->m_gridSize = v; ui->updateGridNodeSize(); }) { // UNVERIFIED
            ui->m_gridSize = v; ui->updateGridNodeSize(); return true;
        } else return false;
    }
    template <class U, class O> bool moveOn(U* ui, O* o, CCPoint d) {
        if constexpr (requires { ui->moveObject(o, d); }) { ui->moveObject(o, d); return true; } // UNVERIFIED
        else return false;
    }
}

LevelEditorLayer* editorLayer() { return LevelEditorLayer::get(); }

EditorUI* editorUI() {
    auto l = editorLayer();
    return l ? l->m_editorUI : nullptr;
}

std::vector<GameObject*> selectedObjects() {
    std::vector<GameObject*> v;
    auto ui = editorUI();
    if (!ui) return v;
    if (auto arr = ui->getSelectedObjects())
        for (auto o : CCArrayExt<GameObject*>(arr)) if (o) v.push_back(o);
    return v;
}

std::vector<GameObject*> allObjects() {
    std::vector<GameObject*> v;
    auto l = editorLayer();
    if (!l || !l->m_objects) return v;
    for (auto o : CCArrayExt<GameObject*>(l->m_objects)) if (o) v.push_back(o);
    return v;
}

Info inspect(GameObject* o) {
    Info i;
    i.id = o->m_objectID;
    auto p = o->getPosition();
    i.x = p.x; i.y = p.y;
    i.rot = o->getRotation();
    i.sx = o->getScaleX(); i.sy = o->getScaleY();
    i.z = o->m_zOrder;
    i.zLayer = int(o->m_zLayer);
    i.el1 = o->m_editorLayer; i.el2 = o->m_editorLayer2;
    i.type = int(o->m_objectType);
    i.base = baseOf(o); i.detail = detailOf(o);
    groupsOf(o, i.groups);
    return i;
}

std::string saveString(GameObject* o) { return saveStr(o, editorLayer()); }

bool removableKind(GameObject* o, std::string& why) {
    int id = o->m_objectID;
    if (id == 31) { why = "Start position"; return false; }
    if (id == 1329 || id == 142) { why = "Coins"; return false; }
    auto t = o->m_objectType;
    if (t == GameObjectType::Solid || t == GameObjectType::Hazard || t == GameObjectType::Decoration) return true;
    // Triggers whose repeated activation is idempotent (they SET a state, not add to it):
    // Color (899), Alpha (1007), Toggle (1049). Move/Pulse/Spawn/Count/etc. are NOT idempotent.
    if (id == 899 || id == 1007 || id == 1049) return true;
    why = "Interactive/trigger/special (not provably safe)";
    return false;
}

bool selectOnly(std::vector<GameObject*> const& objs) {
    auto ui = editorUI();
    if (!ui) return false;
    auto arr = CCArray::create();
    for (auto o : objs) arr->addObject(o);
    ui->deselectAll();
    ui->selectObjects(arr, true);
    return true;
}

bool deleteSelected() {
    auto ui = editorUI();
    if (!ui) return false;
    ui->onDeleteSelected(nullptr);
    return true;
}

int undoDepth() { return undoCount(editorLayer()); }

bool undoLast() {
    auto ui = editorUI();
    if (!ui) return false;
    ui->undoLastAction(nullptr);
    return true;
}

bool getChannelColor(int ch, ct::RGB& out) { auto l = editorLayer(); return l && readChannel(l, ch, out); }
bool setChannelColor(int ch, ct::RGB c) { auto l = editorLayer(); return l && paintChannel(l, ch, c); }

void setObjectChannels(GameObject* o, int base, int detail) {
    if (base > 0) setBase(o, base);
    if (detail > 0) setDetail(o, detail);
}

void refreshColors(std::vector<GameObject*> const& objs) {
    auto l = editorLayer();
    if (!l) return;
    auto arr = CCArray::create();
    for (auto o : objs) arr->addObject(o);
    refreshOn(l, arr);
}

bool getGridStep(float& out) { auto ui = editorUI(); return ui && gridGet(ui, out); }
bool setGridStep(float v) { auto ui = editorUI(); return ui && gridSet(ui, v); }
bool moveObject(GameObject* o, CCPoint d) { auto ui = editorUI(); return ui && moveOn(ui, o, d); }

} // namespace ct::bridge
