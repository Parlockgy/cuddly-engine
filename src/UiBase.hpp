#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace ct {

class CTPopup : public FLAlertLayer {
protected:
    CCMenu* m_menu = nullptr;
    CCNode* m_body = nullptr;

    CCSize m_size = CCSize(0.0f, 0.0f);
    CCPoint m_center = CCPoint(0.0f, 0.0f);

    bool initBase(float w, float h, char const* title) {
        if (!FLAlertLayer::init(110)) {
            return false;
        }

        m_noElasticity = true;

        m_size = CCSize(w, h);

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_center = CCPoint(
            win.width / 2.0f,
            win.height / 2.0f
        );

        if (!m_mainLayer) {
            m_mainLayer = CCLayer::create();
            this->addChild(m_mainLayer);
        }

        auto bg = extension::CCScale9Sprite::create(
            "GJ_square01.png",
            CCRect(0.0f, 0.0f, 80.0f, 80.0f)
        );

        bg->setContentSize(m_size);
        bg->setPosition(m_center);
        m_mainLayer->addChild(bg, -1);

        m_body = CCNode::create();
        m_mainLayer->addChild(m_body, 5);

        m_menu = CCMenu::create();
        m_menu->setPosition(m_center);
        m_menu->setTouchPriority(-504);
        m_mainLayer->addChild(m_menu, 10);

        auto t = CCLabelBMFont::create(
            title,
            "goldFont.fnt"
        );

        t->setScale(0.7f);
        t->setPosition(
            m_center + CCPoint(0.0f, h / 2.0f - 18.0f)
        );

        m_mainLayer->addChild(t, 6);

        auto closeSpr = CCSprite::createWithSpriteFrameName(
            "GJ_closeBtn_001.png"
        );

        auto close = CCMenuItemSpriteExtra::create(
            closeSpr,
            this,
            menu_selector(CTPopup::onClose)
        );

        close->setPosition(
            CCPoint(-w / 2.0f + 4.0f, h / 2.0f - 4.0f)
        );

        m_menu->addChild(close);

        this->setKeypadEnabled(true);
        this->setTouchEnabled(true);

        return true;
    }

    void finishBase() {
        if (m_menu) {
            m_menu->setTouchPriority(-504);
        }

        fixPriority(this);
    }

    template <class N>
    static void fixPriority(N* n) {
        if constexpr (requires { handleTouchPriority(n); }) {
            handleTouchPriority(n);
        }
    }

    CCMenuItemSpriteExtra* addBtn(
        char const* text,
        CCPoint pos,
        SEL_MenuHandler sel,
        float scale = 0.55f,
        int tag = 0
    ) {
        auto spr = ButtonSprite::create(text);
        spr->setScale(scale);

        auto b = CCMenuItemSpriteExtra::create(
            spr,
            this,
            sel
        );

        b->setPosition(pos);
        b->setTag(tag);
        m_menu->addChild(b);

        return b;
    }

    CCLabelBMFont* addLabel(
        std::string const& text,
        CCPoint pos,
        float scale = 0.5f,
        char const* font = "chatFont.fnt",
        CCPoint anchor = CCPoint(0.5f, 0.5f)
    ) {
        auto l = CCLabelBMFont::create(
            text.empty() ? " " : text.c_str(),
            font
        );

        l->setScale(scale);
        l->setAnchorPoint(anchor);
        l->setPosition(m_center + pos);

        m_mainLayer->addChild(l, 6);

        return l;
    }

    void setLines(
        std::vector<std::string> const& lines,
        CCPoint topLeft,
        float lineH = 10.5f,
        float scale = 0.45f
    ) {
        m_body->removeAllChildren();

        float y = topLeft.y;

        for (auto const& s : lines) {
            auto l = CCLabelBMFont::create(
                s.empty() ? " " : s.c_str(),
                "chatFont.fnt"
            );

            l->setAnchorPoint(CCPoint(0.0f, 0.5f));
            l->setScale(scale);

            l->setPosition(
                m_center + CCPoint(topLeft.x, y)
            );

            m_body->addChild(l);

            y -= lineH;
        }
    }

public:
    void onClose(CCObject*) {
        this->setKeypadEnabled(false);
        this->removeFromParentAndCleanup(true);
    }

    void keyBackClicked() override {
        this->onClose(nullptr);
    }
};

} // namespace ct
