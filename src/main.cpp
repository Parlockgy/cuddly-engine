#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

// This initial build adds an editor button and a clear information popup.
// The actual HTTP downloader and audio registration require platform-specific
// implementation and are intentionally not faked in this starter version.

class $modify(LevelTuneEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto menu = this->getChildByID("editor-buttons-menu");
        if (!menu) return true;

        auto spr = CCSprite::createWithSpriteFrameName("GJ_musicBtn_001.png");
        if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
        auto btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(LevelTuneEditorUI::onLevelTune)
        );
        btn->setID("leveltune-importer-button"_spr);
        btn->setScale(0.75f);
        menu->addChild(btn);
        menu->updateLayout();
        return true;
    }

    void onLevelTune(CCObject*) {
        FLAlertLayer::create(
            "LevelTune Importer",
            "<cy>Custom Music Helper</c>\n\n"
            "Paste a direct, authorized audio-file URL (for example, a URL ending in .mp3 or .ogg) "
            "in a future downloader panel to import music.\n\n"
            "<cr>YouTube watch links are not direct audio files.</c> This starter build does not "
            "extract or bypass YouTube download restrictions. Use audio you own, have permission "
            "to download, or a source that explicitly offers downloads.\n\n"
            "<co>Status: editor button prototype</c>\n"
            "The direct-download, format validation, and song-ID registration steps still need "
            "to be implemented and tested before this can install a song into a level.",
            "OK"
        )->show();
    }
};
