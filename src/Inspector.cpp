#include "UiBase.hpp"
#include "Popups.hpp"
#include "Bridge.hpp"
#include <map>
#include <set>
#include <fmt/format.h>

namespace ct {
namespace {

std::string joinInts(std::set<int> const& s, size_t cap = 14) {
    std::string out; size_t n = 0;
    for (int v : s) { if (n++) out += ","; if (n > cap) { out += "..."; break; } out += std::to_string(v); }
    return out.empty() ? "-" : out;
}
std::string chanStr(int v) { return v < 0 ? "n/a" : std::to_string(v); }

std::vector<std::string> buildReport() {
    using namespace bridge;
    auto sel = selectedObjects();
    std::vector<std::string> L;
    if (sel.empty()) { L.push_back("Nothing selected. Select objects in the editor, then reopen."); return L; }
    if (sel.size() == 1) {
        auto i = inspect(sel[0]);
        L.push_back(fmt::format("Object ID {}  (type {})", i.id, i.type));
        L.push_back(fmt::format("Position: {:.2f}, {:.2f}", i.x, i.y));
        L.push_back(fmt::format("Rotation: {:.2f}   Scale: {:.2f} x {:.2f}", i.rot, i.sx, i.sy));
        L.push_back(fmt::format("Z-order: {}   Z-layer: {}", i.z, i.zLayer));
        L.push_back(fmt::format("Editor layers: {} / {}", i.el1, i.el2));
        std::set<int> g(i.groups.begin(), i.groups.end());
        L.push_back("Groups: " + joinInts(g));
        L.push_back(fmt::format("Color channels: primary {}  secondary {}", chanStr(i.base), chanStr(i.detail)));
        return L;
    }
    std::map<int, int> ids; std::set<int> groups, bases, details, el1, zs;
    float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
    for (auto o : sel) {
        auto i = inspect(o);
        ids[i.id]++; groups.insert(i.groups.begin(), i.groups.end());
        bases.insert(i.base); details.insert(i.detail); el1.insert(i.el1); zs.insert(i.z);
        minX = std::min(minX, i.x); maxX = std::max(maxX, i.x); minY = std::min(minY, i.y); maxY = std::max(maxY, i.y);
    }
    L.push_back(fmt::format("{} objects selected, {} distinct IDs", sel.size(), ids.size()));
    std::vector<std::pair<int,int>> top(ids.begin(), ids.end());
    std::sort(top.begin(), top.end(), [](auto& a, auto& b) { return a.second > b.second; });
    std::string t = "Top IDs: ";
    for (size_t k = 0; k < top.size() && k < 6; ++k) t += fmt::format("{}x{}  ", top[k].first, top[k].second);
    L.push_back(t);
    L.push_back(fmt::format("Bounds X {:.1f}..{:.1f}  Y {:.1f}..{:.1f}", minX, maxX, minY, maxY));
    L.push_back(fmt::format("Size: {:.1f} x {:.1f}", maxX - minX, maxY - minY));
    L.push_back("Z-orders used: " + joinInts(zs));
    L.push_back("Editor layer 1 values: " + joinInts(el1));
    L.push_back("Groups used: " + joinInts(groups));
    L.push_back("Primary channels: " + joinInts(bases));
    L.push_back("Secondary channels: " + joinInts(details));
    return L;
}

class Inspector : public CTPopup {
    std::vector<std::string> m_last;
public:
    static Inspector* create() {
        auto r = new Inspector();
        if (r && r->init()) { r->autorelease(); return r; }
        delete r; return nullptr;
    }
    bool init() {
        if (!initBase(380, 250, "Object Inspector")) return false;
        addBtn("Refresh", {-70, -105}, menu_selector(Inspector::onRefresh));
        addBtn("Copy", {10, -105}, menu_selector(Inspector::onCopy));
        addBtn("Close", {80, -105}, menu_selector(CTPopup::onClose));
        onRefresh(nullptr);
        finishBase();
        return true;
    }
    void onRefresh(CCObject*) {
        m_last = buildReport();
        setLines(m_last, {-170, 82}, 12.f, .5f);
    }
    void onCopy(CCObject*) {
        std::string s;
        for (auto& l : m_last) s += l + "\n";
        geode::utils::clipboard::write(s);
        Notification::create("Inspector report copied", NotificationIcon::Success)->show();
    }
};
}
void openInspector() { if (auto p = Inspector::create()) p->show(); }
}
