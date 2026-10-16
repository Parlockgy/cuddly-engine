#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

namespace {
    void showToolkit() {
        FLAlertLayer::create(
            "Creator Toolkit",
            "<cy>LEVEL CREATOR TOOLBOX</c>\n\n"
            "<cg>1. Object Optimizer</c>\n"
            "Scan selected static objects, find identical candidates, and preview safe merge groups.\n\n"
            "<cg>2. Palette Sync</c>\n"
            "Choose a base color and preview coordinated highlight/shadow tones while preserving existing channel IDs.\n\n"
            "<cg>3. Color Slot Pages</c>\n"
            "Plan for 50+ saved palette presets with pages, labels, and quick apply.\n\n"
            "<co>Prototype status:</c> this build adds the editor entry point and feature panel only. It does NOT merge objects, rewrite channels, or create extra native GD color channels yet. Those operations need careful editor-data handling and in-game tests to avoid corrupting levels.\n\n"
            "No level data has been changed.",
            "Close"
        )->show();
    }
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
            spr,
            this,
            menu_selector(CreatorToolkitEditorUI::onCreatorToolkit)
        );
        if (!button) return true;
        button->setID("pumpkinman.creator-toolkit/button");
        button->setScale(0.72f);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onCreatorToolkit(CCObject*) {
        showToolkit();
    }
};
