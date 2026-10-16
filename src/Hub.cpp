#include "UiBase.hpp"
#include "Popups.hpp"

namespace ct {
namespace {
class Hub : public CTPopup {
public:
    static Hub* create() {
        auto r = new Hub();
        if (r && r->init()) { r->autorelease(); return r; }
        delete r; return nullptr;
    }
    bool init() {
        if (!initBase(300, 230, "Creator Toolkit")) return false;
        addBtn("Palette Lab", {0, 62}, menu_selector(Hub::onPalette), .75f);
        addBtn("Optimizer / Duplicate Cleaner", {0, 26}, menu_selector(Hub::onOpt), .6f);
        addBtn("Object Inspector", {0, -8}, menu_selector(Hub::onInspect), .75f);
        addBtn("Micro Grid / Grid Presets", {0, -42}, menu_selector(Hub::onGrid), .65f);
        setLines({
            "Status: source-complete; NOT yet compiled or tested in-game.",
            "Features that depend on unverified bindings report",
            "'unsupported' at runtime instead of failing silently.",
        }, {-130, -78}, 10.f, .4f);
        finishBase();
        return true;
    }
    void onPalette(CCObject*) { openPaletteLab(); }
    void onOpt(CCObject*) { openOptimizer(); }
    void onInspect(CCObject*) { openInspector(); }
    void onGrid(CCObject*) { openMicroGrid(); }
};
}
void openHub() { if (auto p = Hub::create()) p->show(); }
}
