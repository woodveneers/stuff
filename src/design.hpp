#pragma once
/**
 * design.hpp — Color Palettes & Design Templates
 * 
 * Provides pre-built color schemes and design helpers:
 * - Named color palettes (rated level styles)
 * - Color harmony generator (complementary, analogous, triadic)
 * - Gradient generation between two colors
 * - Quick-apply to selected color channels
 */

#include <Geode/Geode.hpp>
#include <vector>
#include <cmath>
using namespace geode::prelude;

namespace studio::design {

    struct Color3 {
        int r, g, b;
        ccColor3B toCC() const { return { (GLubyte)r, (GLubyte)g, (GLubyte)b }; }
    };

    struct ColorPalette {
        std::string name;
        std::string style;       // "Dark", "Neon", "Nature", "Pastel", "Fire", etc.
        std::string description;
        Color3 bg;               // Background color
        Color3 ground;           // Ground color
        Color3 primary;          // Main object color
        Color3 secondary;        // Accent color
        Color3 detail;           // Detail/glow color
        Color3 text;             // Text/line color
    };

    // ============================================================
    // Pre-built palettes inspired by rated level styles
    // ============================================================

    inline std::vector<ColorPalette> getAllPalettes() {
        return {
            {
                "Abyss", "Dark",
                "Deep dark theme — blacks, deep blues, red accents",
                { 5, 5, 15 }, { 10, 10, 30 }, { 20, 40, 80 },
                { 180, 20, 40 }, { 100, 10, 30 }, { 200, 200, 220 }
            },
            {
                "Neon Rush", "Neon",
                "Vibrant neon on dark backgrounds — classic GD style",
                { 10, 5, 20 }, { 15, 10, 30 }, { 0, 255, 200 },
                { 255, 0, 150 }, { 100, 0, 255 }, { 255, 255, 255 }
            },
            {
                "Sunset", "Warm",
                "Orange-to-purple gradient feel — warm and atmospheric",
                { 40, 10, 30 }, { 60, 20, 40 }, { 255, 120, 30 },
                { 255, 60, 80 }, { 180, 40, 120 }, { 255, 220, 180 }
            },
            {
                "Forest", "Nature",
                "Greens and earth tones — organic, natural feel",
                { 10, 20, 10 }, { 30, 50, 20 }, { 60, 140, 50 },
                { 100, 200, 60 }, { 40, 80, 30 }, { 200, 230, 180 }
            },
            {
                "Arctic", "Cold",
                "Ice blues and whites — clean, cold aesthetic",
                { 10, 15, 25 }, { 20, 30, 50 }, { 100, 180, 255 },
                { 200, 230, 255 }, { 60, 120, 200 }, { 240, 250, 255 }
            },
            {
                "Inferno", "Fire",
                "Reds, oranges, yellows — aggressive, hot theme",
                { 20, 5, 0 }, { 40, 10, 0 }, { 255, 80, 0 },
                { 255, 200, 0 }, { 200, 30, 0 }, { 255, 255, 200 }
            },
            {
                "Void", "Minimal",
                "Monochrome with single color accent — sleek and modern",
                { 5, 5, 5 }, { 15, 15, 15 }, { 50, 50, 50 },
                { 0, 200, 255 }, { 30, 30, 30 }, { 180, 180, 180 }
            },
            {
                "Sakura", "Pastel",
                "Soft pinks and lavenders — gentle, beautiful aesthetic",
                { 30, 15, 25 }, { 50, 25, 40 }, { 255, 150, 180 },
                { 200, 130, 220 }, { 255, 200, 210 }, { 255, 240, 245 }
            },
            {
                "Cyber", "Tech",
                "Matrix greens on black — digital/tech theme",
                { 0, 5, 0 }, { 5, 15, 5 }, { 0, 255, 60 },
                { 0, 180, 255 }, { 0, 120, 40 }, { 150, 255, 150 }
            },
            {
                "Galaxy", "Space",
                "Deep purples and starlight — cosmic atmosphere",
                { 10, 5, 25 }, { 20, 10, 40 }, { 120, 60, 200 },
                { 255, 180, 255 }, { 60, 30, 120 }, { 200, 200, 255 }
            },
            {
                "Bloodbath", "Hell",
                "Classic hell theme — dark reds and blacks",
                { 15, 0, 0 }, { 30, 5, 5 }, { 180, 0, 0 },
                { 255, 40, 0 }, { 100, 0, 0 }, { 200, 150, 150 }
            },
            {
                "Ocean", "Water",
                "Deep sea blues and teals — underwater atmosphere",
                { 5, 15, 30 }, { 10, 25, 50 }, { 30, 120, 180 },
                { 0, 200, 200 }, { 20, 80, 130 }, { 180, 220, 255 }
            },
        };
    }

    // ============================================================
    // Color math helpers
    // ============================================================

    // Linear interpolation between two colors
    inline Color3 lerpColor(Color3 a, Color3 b, float t) {
        return {
            (int)(a.r + (b.r - a.r) * t),
            (int)(a.g + (b.g - a.g) * t),
            (int)(a.b + (b.b - a.b) * t)
        };
    }

    // Generate a gradient of N colors between start and end
    inline std::vector<Color3> gradient(Color3 start, Color3 end, int steps) {
        std::vector<Color3> colors;
        for (int i = 0; i < steps; i++) {
            float t = (float)i / (float)(steps - 1);
            colors.push_back(lerpColor(start, end, t));
        }
        return colors;
    }

    // HSV to RGB conversion
    inline Color3 hsvToRgb(float h, float s, float v) {
        float c = v * s;
        float x = c * (1 - std::abs(std::fmod(h / 60.0f, 2) - 1));
        float m = v - c;
        float r, g, b;
        
        if (h < 60)       { r = c; g = x; b = 0; }
        else if (h < 120) { r = x; g = c; b = 0; }
        else if (h < 180) { r = 0; g = c; b = x; }
        else if (h < 240) { r = 0; g = x; b = c; }
        else if (h < 300) { r = x; g = 0; b = c; }
        else              { r = c; g = 0; b = x; }
        
        return { (int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255) };
    }

    // Get complementary color
    inline Color3 complement(Color3 c) {
        return { 255 - c.r, 255 - c.g, 255 - c.b };
    }

    // Generate harmonious colors from a base hue
    inline std::vector<Color3> analogousScheme(float baseHue, float sat = 0.8f, float val = 0.9f) {
        return {
            hsvToRgb(std::fmod(baseHue - 30, 360), sat, val),
            hsvToRgb(baseHue, sat, val),
            hsvToRgb(std::fmod(baseHue + 30, 360), sat, val),
        };
    }

    inline std::vector<Color3> triadicScheme(float baseHue, float sat = 0.8f, float val = 0.9f) {
        return {
            hsvToRgb(baseHue, sat, val),
            hsvToRgb(std::fmod(baseHue + 120, 360), sat, val),
            hsvToRgb(std::fmod(baseHue + 240, 360), sat, val),
        };
    }

    inline std::vector<Color3> splitComplementScheme(float baseHue, float sat = 0.8f, float val = 0.9f) {
        return {
            hsvToRgb(baseHue, sat, val),
            hsvToRgb(std::fmod(baseHue + 150, 360), sat, val),
            hsvToRgb(std::fmod(baseHue + 210, 360), sat, val),
        };
    }

    // Get palettes filtered by style
    inline std::vector<ColorPalette> getPalettesByStyle(const std::string& style) {
        auto all = getAllPalettes();
        std::vector<ColorPalette> filtered;
        for (auto& p : all) {
            if (p.style == style) filtered.push_back(p);
        }
        return filtered;
    }

    inline std::vector<std::string> getStyles() {
        return { "Dark", "Neon", "Warm", "Nature", "Cold", "Fire", 
                 "Minimal", "Pastel", "Tech", "Space", "Hell", "Water" };
    }

} // namespace studio::design
