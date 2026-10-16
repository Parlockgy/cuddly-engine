#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

namespace {
    // Persistent delegate: pressing OK advances through the information pages.
    class ToolkitAlertDelegate : public FLAlertLayerProtocol {
    public:
        void FLAlert_Clicked(FLAlertLayer*, bool) override {
            if (m_nextPage == 2) {
                m_nextPage = 0;
                showPage2();
            } else if (m_nextPage == 3) {
                m_nextPage = 0;
                showPage3();
            } else {
                m_nextPage = 0;
            }
        }

        void showPage1() {
            m_nextPage = 2;
            FLAlertLayer::create(
                this,
                "Creator Toolkit (1/3)",
                "<cy>LEVEL CREATOR TOOLBOX</c>\n\n"
                "<cg>1. Object Optimizer</c>\n"
                "The planned optimizer scans a selected area and identifies identical static objects. "
                "It will show candidate groups and estimated object-count savings before applying anything.\n\n"
                "<cg>Safety first</c>\n"
                "Triggers, animated objects, objects with different Z layers, groups, editor-only settings, "
                "or other meaningful differences must not be merged. The tool should skip uncertain cases.\n\n"
                "<co>Prototype note:</c> object scanning and merging are not active in this build.",
                "OK", nullptr, 300.f
            )->show();
        }

    private:
        int m_nextPage = 0;

        void showPage2() {
            m_nextPage = 3;
            FLAlertLayer::create(
                this,
                "Creator Toolkit (2/3)",
                "<cg>2. Palette Sync</c>\n"
                "Choose a base color and preview coordinated shadow, midtone, and highlight colors. "
                "The preview is intended to preserve existing channel IDs unless you explicitly choose "
                "to assign a different channel.\n\n"
                "<cg>3. Color Slot Pages</c>\n"
                "Browse saved palette presets across multiple pages, with labels and quick-apply controls. "
                "The target is 50+ saved presets; this does not create new native Geometry Dash color channels.\n\n"
                "<co>Prototype note:</c> palette generation and applying presets are not active yet.",
                "OK", nullptr, 300.f
            )->show();
        }

        void showPage3() {
            m_nextPage = 0;
            FLAlertLayer::create(
                this,
                "Creator Toolkit (3/3)",
                "<cy>Current build status</c>\n\n"
                "<cg>Available now</c>\n"
                "- Creator Toolkit button in the editor\n"
                "- Three short information pages that fit on screen\n\n"
                "<co>Not implemented yet</c>\n"
                "- Actual object merging / trigger cleanup\n"
                "- Automatic palette generation and channel edits\n"
                "- Saving and paging through 50+ presets\n\n"
                "This information panel does not modify your level. Close this page to return to the editor.",
                "OK", nullptr, 300.f
            )->show();
        }
    };

    ToolkitAlertDelegate g_alertDelegate;
}

class $modify(CreatorToolkitEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;
        if (!Mod::get()->getSettingValue<bool>("show-editor-button")) return true;

        auto menu = this->getChildByID("editor-buttons-menu");
        if (!menu) {
            log::warn("Creator Toolkit: editor-buttons-menu was not found; skipping button injection.");
            return true;
        }
        if (menu->getChildByID("pumpkinman.creator-toolkit/button")) return true;

        auto spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
        if (!spr) {
            log::warn("Creator Toolkit: couldn't load a button sprite.");
            return true;
        }

        auto button = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(CreatorToolkitEditorUI::onCreatorToolkit)
        );
        if (!button) return true;
        button->setID("pumpkinman.creator-toolkit/button");
        button->setScale(0.72f);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onCreatorToolkit(CCObject*) {
        g_alertDelegate.showPage1();
    }
};
