#pragma once
// Every call that touches Geometry Dash / editor internals lives behind this header.
// Calls whose binding I could NOT verify against installed headers are written with
// `if constexpr (requires { ... })` so a missing member degrades the feature (returns
// "unsupported") instead of breaking the build. See docs/API_STATUS.md.
#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include "Util.hpp"

using namespace geode::prelude;

namespace ct::bridge {

struct Info {
    int id = 0, z = 0, zLayer = 0, el1 = 0, el2 = 0, type = 0;
    float x = 0, y = 0, rot = 0, sx = 1, sy = 1;
    int base = -1, detail = -1;       // -1 = API unavailable, 0 = none/default
    std::vector<int> groups;
};

LevelEditorLayer* editorLayer();
EditorUI* editorUI();
std::vector<GameObject*> selectedObjects();
std::vector<GameObject*> allObjects();
Info inspect(GameObject* o);
std::string saveString(GameObject* o);                       // "" if unsupported
bool removableKind(GameObject* o, std::string& whyProtected); // whitelist logic
bool selectOnly(std::vector<GameObject*> const& objs);
bool deleteSelected();                                        // editor's own delete (native undo entry)
int undoDepth();                                              // -1 if unsupported
bool undoLast();

// colours
bool getChannelColor(int ch, ct::RGB& out);
bool setChannelColor(int ch, ct::RGB c);
void setObjectChannels(GameObject* o, int base, int detail);  // <=0 means leave untouched
void refreshColors(std::vector<GameObject*> const& objs);

// grid
bool getGridStep(float& out);
bool setGridStep(float v);
bool moveObject(GameObject* o, CCPoint delta);

} // namespace ct::bridge
