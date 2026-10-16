
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include "Popups.hpp"
#include "Bridge.hpp"

using namespace geode::prelude;

class $modify(CreatorToolkitEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        auto mod = Mod::get();

        // Restore the saved grid step when the setting is enabled.
        if (mod->getSettingValue<bool>("restore-grid-on-open")) {
            float step = mod->getSavedValue<float>("grid-step", 30.f);

            if (step != 30.f) {
                ct::bridge::setGridStep(step);
            }
        }

        // Respect the editor-button visibility setting.
        if (!mod->getSettingValue<bool>("show-editor-button")) {
            return true;
        }

        auto menu = this->getChildByID("editor-buttons-menu");

        if (!menu) {
            log::warn(
                "Creator Toolkit: editor-buttons-menu not found."
            );
            return true;
        }

        // Avoid adding the button more than once.
        if (menu->getChildByID(
            "pumpkinman.creator-toolkit/button"
        )) {
            return true;
        }

        auto sprite =
            CCSprite::createWithSpriteFrameName(
                "GJ_optionsBtn_001.png"
            );

        if (!sprite) {
            sprite =
                CCSprite::createWithSpriteFrameName(
                    "GJ_plusBtn_001.png"
                );
        }

        if (!sprite) {
            log::warn(
                "Creator Toolkit: could not create the editor button sprite."
            );
            return true;
        }

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(
                CreatorToolkitEditorUI::onCreatorToolkit
            )
        );

        if (!button) {
            log::warn(
                "Creator Toolkit: could not create the editor button."
            );
            return true;
        }

        button->setID(
            "pumpkinman.creator-toolkit/button"
        );

        button->setScale(0.72f);

        menu->addChild(button);
        menu->updateLayout();

        return true;
    }

    void onCreatorToolkit(CCObject*) {
        ct::openHub();
    }
};

