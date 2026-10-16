#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <algorithm>
#include <unordered_map>

using namespace geode::prelude;

namespace {
    using Clock = std::chrono::steady_clock;

    ccColor3B colorForFPS(float fps) {
        if (fps >= 50.f) return { 70, 255, 100 };   // green
        if (fps >= 30.f) return { 255, 220, 55 };   // yellow
        return { 255, 65, 65 };                     // red
    }

    void processParticles(CCNode* node, bool reduce,
                          std::unordered_map<CCNode*, bool>& originalVisibility) {
        if (!node) return;

        if (auto particle = dynamic_cast<CCParticleSystemQuad*>(node)) {
            if (reduce) {
                if (originalVisibility.find(node) == originalVisibility.end()) {
                    originalVisibility.emplace(node, particle->isVisible());
                }
                particle->setVisible(false);
            } else {
                auto it = originalVisibility.find(node);
                if (it != originalVisibility.end()) {
                    particle->setVisible(it->second);
                    originalVisibility.erase(it);
                }
            }
        }

        auto children = node->getChildren();
        if (!children) return;

        for (unsigned int i = 0; i < children->count(); ++i) {
            auto child = static_cast<CCNode*>(children->objectAtIndex(i));
            processParticles(child, reduce, originalVisibility);
        }
    }

    void restoreParticles(CCNode* node,
                          std::unordered_map<CCNode*, bool>& originalVisibility) {
        processParticles(node, false, originalVisibility);
        originalVisibility.clear();
    }
}

class $modify(FOPPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* fpsLabel = nullptr;
        int sampledFrames = 0;
        double sampledSeconds = 0.0;
        double particleScanSeconds = 0.0;
        Clock::time_point sampleStart = Clock::now();
        std::unordered_map<CCNode*, bool> particleVisibility;
        bool particleModeWasEnabled = false;
    };

    void onEnter() {
        PlayLayer::onEnter();
        m_fields->sampledFrames = 0;
        m_fields->sampledSeconds = 0.0;
        m_fields->particleScanSeconds = 0.0;
        m_fields->sampleStart = Clock::now();
        m_fields->particleModeWasEnabled = false;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        const bool showFPS = Mod::get()->getSettingValue<bool>("show-fps");
        if (showFPS) {
            if (!m_fields->fpsLabel) {
                m_fields->fpsLabel = CCLabelBMFont::create("FPS: --", "bigFont.fnt");
                if (m_fields->fpsLabel) {
                    m_fields->fpsLabel->setAnchorPoint({ 0.f, 1.f });
                    auto size = CCDirector::sharedDirector()->getWinSize();
                    m_fields->fpsLabel->setPosition({ 10.f, size.height - 10.f });
                    m_fields->fpsLabel->setScale(0.45f);
                    m_fields->fpsLabel->setZOrder(9999);
                    this->addChild(m_fields->fpsLabel);
                }
            }

            if (m_fields->fpsLabel) {
                ++m_fields->sampledFrames;
                auto now = Clock::now();
                double elapsed = std::chrono::duration<double>(now - m_fields->sampleStart).count();
                double interval = Mod::get()->getSettingValue<double>("fps-update-interval");
                interval = std::max(0.1, std::min(1.0, interval));

                if (elapsed >= interval) {
                    float fps = elapsed > 0.0
                        ? static_cast<float>(m_fields->sampledFrames / elapsed)
                        : 0.f;
                    auto text = fmt::format("FPS: {:.0f}", fps);
                    m_fields->fpsLabel->setString(text.c_str());
                    m_fields->fpsLabel->setColor(colorForFPS(fps));
                    m_fields->sampledFrames = 0;
                    m_fields->sampleStart = now;
                }
            }
        } else if (m_fields->fpsLabel) {
            m_fields->fpsLabel->removeFromParentAndCleanup(true);
            m_fields->fpsLabel = nullptr;
        }

        const bool reduceParticles = Mod::get()->getSettingValue<bool>("reduce-particles");
        m_fields->particleScanSeconds += dt;

        // Scan at a low frequency to keep the optimizer itself lightweight.
        if (m_fields->particleScanSeconds >= 0.5) {
            m_fields->particleScanSeconds = 0.0;
            processParticles(this, reduceParticles, m_fields->particleVisibility);
            m_fields->particleModeWasEnabled = reduceParticles;
        }
    }

    void onExit() {
        restoreParticles(this, m_fields->particleVisibility);
        if (m_fields->fpsLabel) {
            m_fields->fpsLabel = nullptr;
        }
        PlayLayer::onExit();
    }
};
