#pragma once
// Pure C++ (no Geode/GD dependency): colour math, palette generation, serialization.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace ct {

struct RGB { uint8_t r = 0, g = 0, b = 0; };
struct HSV { float h = 0, s = 0, v = 0; }; // h: 0-360, s/v: 0-1

inline float wrapHue(float h) { h = std::fmod(h, 360.f); return h < 0 ? h + 360.f : h; }
inline float clamp01(float x) { return std::clamp(x, 0.f, 1.f); }

inline RGB toRGB(HSV in) {
    float h = wrapHue(in.h), s = clamp01(in.s), v = clamp01(in.v);
    float c = v * s, x = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f)), m = v - c;
    float r = 0, g = 0, b = 0;
    switch (int(h / 60.f) % 6) {
        case 0: r = c; g = x; break;
        case 1: r = x; g = c; break;
        case 2: g = c; b = x; break;
        case 3: g = x; b = c; break;
        case 4: r = x; b = c; break;
        default: r = c; b = x; break;
    }
    auto q = [&](float f) { return uint8_t(std::clamp(int(std::lround((f + m) * 255.f)), 0, 255)); };
    return {q(r), q(g), q(b)};
}

inline HSV toHSV(RGB c) {
    float r = c.r / 255.f, g = c.g / 255.f, b = c.b / 255.f;
    float mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn, h = 0;
    if (d > 0) {
        if (mx == r) h = 60.f * std::fmod((g - b) / d, 6.f);
        else if (mx == g) h = 60.f * ((b - r) / d + 2.f);
        else h = 60.f * ((r - g) / d + 4.f);
    }
    return {wrapHue(h), mx > 0 ? d / mx : 0.f, mx};
}

inline std::string hexStr(RGB c) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "#%02X%02X%02X", c.r, c.g, c.b);
    return buf;
}
inline std::string rgbStr(RGB c) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d, %d, %d", c.r, c.g, c.b);
    return buf;
}
inline bool parseHex(std::string s, RGB& out) {
    s.erase(std::remove_if(s.begin(), s.end(), [](char ch) { return ch == '#' || ch == ' '; }), s.end());
    if (s.size() != 6) return false;
    for (char ch : s) if (!std::isxdigit(static_cast<unsigned char>(ch))) return false;
    auto v = std::strtoul(s.c_str(), nullptr, 16);
    out = {uint8_t((v >> 16) & 255), uint8_t((v >> 8) & 255), uint8_t(v & 255)};
    return true;
}

enum Harmony { Complementary, Analogous, Triadic, SplitComplementary, Tetradic, Monochromatic, HarmonyCount };
inline const char* harmonyName(int i) {
    static const char* n[] = {"Complementary", "Analogous", "Triadic", "Split-compl.", "Tetradic", "Monochromatic"};
    return n[((i % HarmonyCount) + HarmonyCount) % HarmonyCount];
}

struct Preset { const char* name; float hueMin, hueMax, sat, val; bool lightBg; Harmony harmony; };
inline const std::array<Preset, 10>& presets() {
    static const std::array<Preset, 10> p{{
        {"Neon",       150, 330, 0.95f, 1.00f, false, Complementary},
        {"Pastel",       0, 360, 0.35f, 0.98f, true,  Analogous},
        {"Dark",       200, 300, 0.60f, 0.55f, false, Analogous},
        {"Cyberpunk",  280, 330, 0.90f, 0.95f, false, Complementary},
        {"Dreamcore",  280, 340, 0.40f, 0.95f, true,  SplitComplementary},
        {"Liminal",     30,  60, 0.25f, 0.75f, false, Analogous},
        {"Warm",         0,  45, 0.80f, 0.95f, false, Analogous},
        {"Cool",       180, 260, 0.70f, 0.90f, false, Analogous},
        {"Monochrome",   0, 360, 0.08f, 0.85f, false, Monochromatic},
        {"Forest",      90, 150, 0.60f, 0.60f, false, Triadic},
    }};
    return p;
}

constexpr int kRoles = 6;
inline const char* roleName(int i) {
    static const char* n[] = {"Primary", "Secondary", "Background", "Highlight", "Light", "Dark"};
    return n[i];
}

struct Palette { std::string name; std::array<RGB, kRoles> c{}; };

struct Params {
    int harmony = 0, preset = 0;
    float hueShift = 0, satScale = 1, valScale = 1;
    float contrast = 0.6f; // 0 = soft/low contrast, 1 = very dark bg + very light accents
    uint32_t seed = 1;
};

inline Palette generate(Params const& p) {
    auto const& pr = presets()[size_t(p.preset) % presets().size()];
    std::mt19937 rng(p.seed);
    auto rnd = [&](float a, float b) { return a + (b - a) * (float(rng()) / 4294967295.f); };
    float base = wrapHue(rnd(pr.hueMin, pr.hueMax) + p.hueShift);
    float sat = clamp01(pr.sat * p.satScale + rnd(-.05f, .05f));
    float val = clamp01(pr.val * p.valScale + rnd(-.04f, .04f));
    float k = clamp01(p.contrast);
    static const float off[HarmonyCount][2] = {{180, 180}, {30, -30}, {120, 240}, {150, 210}, {90, 180}, {0, 0}};
    int h = ((p.harmony % HarmonyCount) + HarmonyCount) % HarmonyCount;
    bool mono = h == Monochromatic;

    Palette out;
    out.name = std::string(pr.name) + " / " + harmonyName(h);
    out.c[0] = toRGB({base, sat, val});
    out.c[1] = mono ? toRGB({base, sat * .55f, val * .8f}) : toRGB({base + off[h][0], sat * .9f, val * .95f});
    out.c[3] = toRGB({base + off[h][1], mono ? sat * .4f : sat * .75f, 0.88f + 0.12f * k});
    if (pr.lightBg) {
        out.c[2] = toRGB({base, sat * .12f, 0.97f - 0.07f * k});
        out.c[4] = toRGB({base, sat * .15f, 0.99f});
        out.c[5] = toRGB({base, sat * .9f, 0.42f - 0.22f * k});
    } else {
        out.c[2] = toRGB({base, sat * .55f, 0.05f + 0.17f * (1.f - k)});
        out.c[4] = toRGB({base, sat * (0.30f - 0.12f * k), 0.82f + 0.18f * k});
        out.c[5] = toRGB({base, sat * .9f, 0.12f + 0.12f * (1.f - k)});
    }
    return out;
}

// Custom palette persistence: one palette per line, "name<TAB>#RRGGBB,#RRGGBB,..." (6 colours).
inline std::string serialize(std::vector<Palette> const& v) {
    std::string s;
    for (auto const& p : v) {
        std::string n = p.name;
        for (auto& ch : n) if (ch == '\t' || ch == '\n' || ch == '\r') ch = ' ';
        s += n + '\t';
        for (int i = 0; i < kRoles; ++i) s += hexStr(p.c[i]) + (i + 1 < kRoles ? "," : "");
        s += '\n';
    }
    return s;
}
inline std::vector<Palette> deserialize(std::string const& s) {
    std::vector<Palette> out;
    std::istringstream in(s);
    std::string line;
    while (std::getline(in, line)) {
        auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        Palette p; p.name = line.substr(0, tab);
        std::istringstream cs(line.substr(tab + 1));
        std::string tok; int i = 0; bool ok = true;
        while (std::getline(cs, tok, ',') && i < kRoles) { if (!parseHex(tok, p.c[i++])) { ok = false; break; } }
        if (ok && i == kRoles) out.push_back(p);
    }
    return out;
}

} // namespace ct
