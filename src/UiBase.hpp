#pragma once
// Minimal popup base built directly on FLAlertLayer (independent of geode::Popup's
// template/signature changes between SDK majors). UNVERIFIED against SDK 5.x headers:
// FLAlertLayer::init(int), m_mainLayer, m_noElasticity, keyBackClicked.
#include <Geode/Geode.hpp>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace ct {

class CTPopup : public FLAlertLayer {
protected:
    CCMenu* m_menu = nullptr;
    CCNode* m_body = nullptr;
    CCSize m_size{0, 0};
    CCPoint m_center{0, 0};

    bool initBase(float w, float h, char const* title) {
        if (!FLAlertLayer::init(110)) return false;
        m_noElasticity = true;
        m_size = {w, h};
        auto win = CCDirector::sharedDirector()->getWinSize();
        m_center = {win.width / 2, win.height / 2};
        if (!m_mainLayer) { m_mainLayer = CCLayer::create(); this->addChild(m_mainLayer); }

        auto bg = extension::CCScale9Sprite::create("GJ_square01.png", {0, 0, 80, 80});
        bg->setContentSize(m_size);
        bg->setPosition(m_center);
        m_mainLayer->addChild(bg, -1);

        m_body = CCNode::create();
        m_mainLayer->addChild(m_body, 5);

        m_menu = CCMenu::create();
        m_menu->setPosition(m_center);
        m_menu->setTouchPriority(-504);
        m_mainLayer->addChild(m_menu, 10);

        auto t = CCLabelBMFont::create(title, "goldFont.fnt");
        t->setScale(.7f);
        t->setPosition(m_center + CCPoint{0, h / 2 - 18});
        m_mainLayer->addChild(t, 6);

        auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto close = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(CTPopup::onClose));
        close->setPosition({-w / 2 + 4, h / 2 - 4});
        m_menu->addChild(close);

        this->setKeypadEnabled(true);
        this->setTouchEnabled(true);
        return true;
    }

    // Call after all children (menus, text inputs) exist.
    void finishBase() {
        m_menu->setTouchPriority(-504);
        fixPriority(this);
    }
    template <class N> static void fixPriority(N* n) {
        if constexpr (requires { handleTouchPriority(n); }) handleTouchPriority(n);
    }

    CCMenuItemSpriteExtra* addBtn(char const* text, CCPoint pos, SEL_MenuHandler sel, float scale = .55f, int tag = 0) {
        auto spr = ButtonSprite::create(text);
        spr->setScale(scale);
        auto b = CCMenuItemSpriteExtra::create(spr, this, sel);
        b->setPosition(pos);
        b->setTag(tag);
        m_menu->addChild(b);
        return b;
    }

    CCLabelBMFont* addLabel(std::string const& text, CCPoint pos, float scale = .5f,
                            char const* font = "chatFont.fnt", CCPoint anchor = {.5f, .5f}) {
        auto l = CCLabelBMFont::create(text.empty() ? " " : text.c_str(), font);
        l->setScale(scale);
        l->setAnchorPoint(anchor);
        l->setPosition(m_center + pos);
        m_mainLayer->addChild(l, 6);
        return l;
    }

    // Replace the text block in m_body (one chatFont label per line).
    void setLines(std::vector<std::string> const& lines, CCPoint topLeft, float lineH = 10.5f, float scale = .45f) {
        m_body->removeAllChildren();
        float y = topLeft.y;
        for (auto const& s : lines) {
            auto l = CCLabelBMFont::create(s.empty() ? " " : s.c_str(), "chatFont.fnt");
            l->setAnchorPoint({0, .5f});
            l->setScale(scale);
            l->setPosition(m_center + CCPoint{topLeft.x, y});
            m_body->addChild(l);
            y -= lineH;
        }
    }

public:
    void onClose(CCObject*) {
        this->setKeypadEnabled(false);
        this->removeFromParentAndCleanup(true);
    }
    void keyBackClicked() override { this->onClose(nullptr); }
};

} // namespace ct
