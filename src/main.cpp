#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <algorithm>
#include <unordered_map>

using namespace geode::prelude;

namespace {
    using Clock = std::chrono::steady_clock;

    ccColor3B colorForFPS(float fps, float greenThreshold, float yellowThreshold) {
        if (fps >= greenThreshold) return { 70, 255, 100 };
        if (fps >= yellowThreshold) return { 255, 220, 55 };
        return { 255, 65, 65 };
    }

    void processParticles(CCNode* node, bool reduce,
                          std::unordered_map<CCNode*, bool>& originalVisibility) {
        if (!node) return;
        if (auto particle = dynamic_cast<CCParticleSystemQuad*>(node)) {
            if (reduce) {
                if (originalVisibility.find(node) == originalVisibility.end())
                    originalVisibility.emplace(node, particle->isVisible());
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
        for (unsigned int i = 0; i < children->count(); ++i)
            processParticles(static_cast<CCNode*>(children->objectAtIndex(i)),
                             reduce, originalVisibility);
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
        double particleScanSeconds = 0.0;
        Clock::time_point sampleStart = Clock::now();
        std::unordered_map<CCNode*, bool> particleVisibility;
        bool bgWasVisible = true;
        bool groundWasVisible = true;
        bool ground2WasVisible = true;
        bool layersCaptured = false;
    };

    void onEnter() {
        PlayLayer::onEnter();
        m_fields->sampledFrames = 0;
        m_fields->particleScanSeconds = 0.0;
        m_fields->sampleStart = Clock::now();
        m_fields->layersCaptured = false;
    }

    void applyLayerVisibility(bool hideBg, bool hideGround, bool hideGround2) {
        if (!m_fields->layersCaptured) {
            if (m_background) m_fields->bgWasVisible = m_background->isVisible();
            if (m_groundLayer) m_fields->groundWasVisible = m_groundLayer->isVisible();
            if (m_groundLayer2) m_fields->ground2WasVisible = m_groundLayer2->isVisible();
            m_fields->layersCaptured = true;
        }
        if (m_background) m_background->setVisible(hideBg ? false : m_fields->bgWasVisible);
        if (m_groundLayer) m_groundLayer->setVisible(hideGround ? false : m_fields->groundWasVisible);
        if (m_groundLayer2) m_groundLayer2->setVisible(hideGround2 ? false : m_fields->ground2WasVisible);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        bool lowGraphics = Mod::get()->getSettingValue<bool>("low-graphics-mode");
        bool hideBg = lowGraphics || Mod::get()->getSettingValue<bool>("hide-background");
        bool hideGround = lowGraphics || Mod::get()->getSettingValue<bool>("hide-ground");
        bool hideGround2 = lowGraphics || Mod::get()->getSettingValue<bool>("hide-ground-2");
        applyLayerVisibility(hideBg, hideGround, hideGround2);

        bool reduceParticles = lowGraphics || Mod::get()->getSettingValue<bool>("reduce-particles");
        bool practiceAllowed = Mod::get()->getSettingValue<bool>("hide-particles-in-practice");
        if (m_isPracticeMode && !practiceAllowed && !lowGraphics)
            reduceParticles = false;

        m_fields->particleScanSeconds += dt;
        if (m_fields->particleScanSeconds >= 0.5) {
            m_fields->particleScanSeconds = 0.0;
            processParticles(this, reduceParticles, m_fields->particleVisibility);
        }

        const bool showFPS = Mod::get()->getSettingValue<bool>("show-fps");
        if (!showFPS) {
            if (m_fields->fpsLabel) {
                m_fields->fpsLabel->removeFromParentAndCleanup(true);
                m_fields->fpsLabel = nullptr;
            }
            return;
        }

        if (!m_fields->fpsLabel) {
            m_fields->fpsLabel = CCLabelBMFont::create("FPS: --", "bigFont.fnt");
            if (m_fields->fpsLabel) {
                m_fields->fpsLabel->setAnchorPoint({ 0.f, 1.f });
                auto size = CCDirector::sharedDirector()->getWinSize();
                m_fields->fpsLabel->setPosition({ 10.f, size.height - 10.f });
                m_fields->fpsLabel->setScale(
                    Mod::get()->getSettingValue<double>("fps-counter-scale")
                );
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
                float fps = elapsed > 0.0 ? static_cast<float>(m_fields->sampledFrames / elapsed) : 0.f;
                double frameMs = fps > 0.0 ? 1000.0 / fps : 0.0;
                bool showMs = Mod::get()->getSettingValue<bool>("show-frame-time");
                bool showStatus = Mod::get()->getSettingValue<bool>("show-performance-status");
                bool adaptiveColors = Mod::get()->getSettingValue<bool>("adaptive-fps-colors");
                float greenThreshold = static_cast<float>(Mod::get()->getSettingValue<int>("fps-green-threshold"));
                float yellowThreshold = static_cast<float>(Mod::get()->getSettingValue<int>("fps-yellow-threshold"));
                if (yellowThreshold >= greenThreshold) yellowThreshold = std::max(10.f, greenThreshold - 5.f);

                std::string label = fmt::format("FPS: {:.0f}", fps);
                if (showMs) label += fmt::format(" | {:.1f} ms", frameMs);
                if (showStatus) {
                    if (fps >= greenThreshold) label += " | GOOD";
                    else if (fps >= yellowThreshold) label += " | FAIR";
                    else label += " | LOW";
                }
                m_fields->fpsLabel->setString(label.c_str());
                if (adaptiveColors) m_fields->fpsLabel->setColor(colorForFPS(fps, greenThreshold, yellowThreshold));
                else m_fields->fpsLabel->setColor({255, 255, 255});
                m_fields->fpsLabel->setScale(
                    Mod::get()->getSettingValue<double>("fps-counter-scale")
                );
                m_fields->sampledFrames = 0;
                m_fields->sampleStart = now;
            }
        }
    }

    void onExit() {
        restoreParticles(this, m_fields->particleVisibility);
        // Restore layer visibility before leaving the level.
        if (m_fields->layersCaptured) {
            if (m_background) m_background->setVisible(m_fields->bgWasVisible);
            if (m_groundLayer) m_groundLayer->setVisible(m_fields->groundWasVisible);
            if (m_groundLayer2) m_groundLayer2->setVisible(m_fields->ground2WasVisible);
        }
        if (m_fields->fpsLabel) {
            m_fields->fpsLabel->removeFromParentAndCleanup(true);
            m_fields->fpsLabel = nullptr;
        }
        PlayLayer::onExit();
    }
};
