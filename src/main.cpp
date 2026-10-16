#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "Popups.hpp"
#include "Bridge.hpp"

using namespace geode::prelude;

class $modify(CreatorToolkitEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        // Re-apply the saved grid step (setting-controlled). Silently ignored if unsupported.
        if (Mod::get()->getSettingValue<bool>("restore-grid-on-open")) {
            float step = Mod::get()->getSavedValue<float>("grid-step", 30.f);
            if (step != 30.f) ct::bridge::setGridStep(step);
        }

        if (!Mod::get()->getSettingValue<bool>("show-editor-button")) return true;
        auto menu = this->getChildByID("editor-buttons-menu");
        if (!menu) { log::warn("Creator Toolkit: editor-buttons-menu not found."); return true; }
        if (menu->getChildByID("pumpkinman.creator-toolkit/button")) return true;

        auto spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
        if (!spr) return true;
        auto button = CCMenuItemSpriteExtra::create(spr, this, menu_selector(CreatorToolkitEditorUI::onCreatorToolkit));
        if (!button) return true;
        button->setID("pumpkinman.creator-toolkit/button");
        button->setScale(0.72f);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onCreatorToolkit(CCObject*) { ct::openHub(); }
};
