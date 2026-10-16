
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
    CCSize m_size{0.f, 0.f};
    CCPoint m_center{0.f, 0.f};

    bool initBase(float w, float h, char const* title) {
        if (!FLAlertLayer::init(110)) {
            return false;
        }

        m_noElasticity = true;
        m_size = CCSize{w, h};

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_center = CCPoint{
            win.width / 2.f,
            win.height / 2.f
        };

        if (!m_mainLayer) {
            m_mainLayer = CCLayer::create();
            if (!m_mainLayer) {
                return false;
            }

            this->addChild(m_mainLayer);
        }

        auto bg = extension::CCScale9Sprite::create(
            "GJ_square01.png",
            CCRect{0.f, 0.f, 80.f, 80.f}
        );

        if (!bg) {
            return false;
        }

        bg->setContentSize(m_size);
        bg->setPosition(m_center);
        m_mainLayer->addChild(bg, -1);

        m_body = CCNode::create();

        if (!m_body) {
            return false;
        }

        m_body->setPosition(m_center);
        m_mainLayer->addChild(m_body, 5);

        m_menu = CCMenu::create();

        if (!m_menu) {
            return false;
        }

        m_menu->setPosition(m_center);
        m_menu->setTouchPriority(-504);
        m_mainLayer->addChild(m_menu, 10);

        auto label = CCLabelBMFont::create(
            title ? title : "Creator Toolkit",
            "goldFont.fnt"
        );

        if (label) {
            label->setScale(0.7f);
            label->setPosition(
                m_center + CCPoint{0.f, h / 2.f - 18.f}
            );
            m_mainLayer->addChild(label, 6);
        }

        auto closeSprite =
            CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");

        if (closeSprite) {
            auto closeButton = CCMenuItemSpriteExtra::create(
                closeSprite,
                this,
                menu_selector(CTPopup::onClose)
            );

            if (closeButton) {
                closeButton->setPosition(
                    CCPoint{-w / 2.f + 4.f, h / 2.f - 4.f}
                );

                m_menu->addChild(closeButton);
            }
        }

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
    static void fixPriority(N* node) {
        if constexpr (requires { handleTouchPriority(node); }) {
            handleTouchPriority(node);
        }
    }

    CCMenuItemSpriteExtra* addBtn(
        char const* text,
        CCPoint pos,
        SEL_MenuHandler callback,
        float scale = 0.6f,
        int tag = 0
    ) {
        if (!m_menu || !text || !callback) {
            return nullptr;
        }

        auto label = CCLabelBMFont::create(
            text,
            "bigFont.fnt"
        );

        if (!label) {
            return nullptr;
        }

        label->setScale(scale);

        auto button = CCMenuItemSpriteExtra::create(
            label,
            this,
            callback
        );

        if (!button) {
            return nullptr;
        }

        button->setTag(tag);
        button->setPosition(pos);
        m_menu->addChild(button);

        return button;
    }

    // Supports an optional fourth argument for the font filename.
    CCLabelBMFont* addLabel(
        char const* text,
        CCPoint pos,
        float scale = 0.5f,
        char const* font = "bigFont.fnt"
    ) {
        if (!m_body) {
            return nullptr;
        }

        auto label = CCLabelBMFont::create(
            text ? text : "",
            font ? font : "bigFont.fnt"
        );

        if (!label) {
            return nullptr;
        }

        label->setScale(scale);
        label->setPosition(pos);
        m_body->addChild(label);

        return label;
    }

    void setLines(
        std::vector<std::string> const& lines,
        CCPoint start,
        float lineHeight = 12.f,
        float scale = 0.45f
    ) {
        if (!m_body) {
            return;
        }

        m_body->removeAllChildrenWithCleanup(true);

        float y = start.y;

        for (auto const& line : lines) {
            auto label = CCLabelBMFont::create(
                line.c_str(),
                "bigFont.fnt"
            );

            if (!label) {
                y -= lineHeight;
                continue;
            }

            label->setAnchorPoint(CCPoint{0.f, 0.5f});
            label->setPosition(CCPoint{start.x, y});
            label->setScale(scale);
            m_body->addChild(label);

            y -= lineHeight;
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
