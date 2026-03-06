#pragma once
/**
 * gameplay.hpp — Level Analysis & Gameplay Helper
 * 
 * Scans the current level and provides actionable feedback:
 * - Object density per section (are there dead zones?)
 * - Game mode distribution (variety check)
 * - Trigger complexity score
 * - Estimated difficulty
 * - Rating potential checklist
 * - Common issues detection
 */

#include <Geode/Geode.hpp>
#include <vector>
#include <map>
#include <cmath>
using namespace geode::prelude;

namespace studio::gameplay {

    // ============================================================
    // Analysis result
    // ============================================================

    struct SectionInfo {
        float startX;
        float endX;
        int objectCount;
        int hazardCount;
        int triggerCount;
        int decorationCount;
        float density;      // objects per 30 units
        std::string dominantMode; // most common gamemode portal in this section
    };

    struct LevelAnalysis {
        // Basic stats
        int totalObjects;
        int totalTriggers;
        int totalHazards;
        int totalDecorations;
        int totalPortals;
        float levelLength;      // in editor units
        int groupsUsed;
        int colorsUsed;
        
        // Sections (every ~10% of the level)
        std::vector<SectionInfo> sections;
        
        // Game mode usage
        std::map<std::string, int> modePortalCounts;
        
        // Scores (0-100)
        int gameplayScore;      // variety, flow, difficulty curve
        int decorationScore;    // object density, visual variety
        int triggerScore;       // trigger usage and complexity
        int overallScore;       // weighted average
        
        // Issues found
        std::vector<std::string> issues;
        
        // Tips for improvement
        std::vector<std::string> tips;
        
        // Estimated difficulty
        std::string estimatedDifficulty;
    };

    // ============================================================
    // Object type detection helpers
    // ============================================================

    inline bool isHazard(GameObject* obj) {
        if (!obj) return false;
        auto type = obj->m_objectType;
        if (type == GameObjectType::Hazard || type == GameObjectType::AnimatedHazard)
            return true;
        int id = obj->m_objectID;
        return (id == 8 || id == 39 || id == 103 || id == 9 || id == 61 ||
                id == 88 || id == 89 || id == 98 || id == 243 || id == 244 ||
                (id >= 363 && id <= 368) || (id >= 397 && id <= 399) ||
                (id >= 1705 && id <= 1711));
    }

    inline bool isTrigger(GameObject* obj) {
        if (!obj) return false;
        int id = obj->m_objectID;
        return (id == 901 || id == 1346 || id == 2067 || id == 1006 ||
                id == 1007 || id == 1049 || id == 899 || id == 1347 ||
                id == 1520 || id == 1268 || id == 1595 || id == 1611 ||
                id == 1612 || id == 1613 || id == 1616 || id == 1811 ||
                id == 1812 || id == 1814 || id == 1815 || id == 1817 ||
                id == 1818 || id == 1819 || id == 2015 || id == 2016 ||
                id == 2062 || id == 2067 || id == 2068 || id == 2903);
    }

    inline bool isPortal(GameObject* obj) {
        if (!obj) return false;
        int id = obj->m_objectID;
        return (id == 10 || id == 11 || id == 12 || id == 13 || id == 45 ||
                id == 46 || id == 47 || id == 99 || id == 101 || id == 111 ||
                id == 200 || id == 201 || id == 202 || id == 203 ||
                id == 286 || id == 287 || id == 660 || id == 745 ||
                id == 1331 || id == 1334 || id == 1933);
    }

    inline std::string portalModeName(int id) {
        switch (id) {
            case 12: return "Cube";
            case 13: return "Ship";
            case 47: return "Ball";
            case 111: return "UFO";
            case 660: return "Wave";
            case 745: return "Robot";
            case 1331: return "Spider";
            case 1933: return "Swing";
            default: return "";
        }
    }

    inline bool isDecoration(GameObject* obj) {
        if (!obj) return false;
        return obj->m_objectType == GameObjectType::Decoration ||
               obj->m_objectType == GameObjectType::Special;
    }

    // ============================================================
    // Main analysis function
    // ============================================================

    inline LevelAnalysis analyzeLevel(LevelEditorLayer* editor) {
        LevelAnalysis result = {};
        
        auto objects = editor->m_objects;
        if (!objects || objects->count() == 0) {
            result.issues.push_back("Level is empty!");
            return result;
        }
        
        // Pass 1: Find level bounds and count objects
        float minX = 999999.0f, maxX = -999999.0f;
        std::set<int> usedGroups;
        std::set<int> usedColors;
        
        for (int i = 0; i < objects->count(); i++) {
            auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (!obj) continue;
            
            float x = obj->getPositionX();
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            
            result.totalObjects++;
            
            if (isHazard(obj)) result.totalHazards++;
            else if (isTrigger(obj)) result.totalTriggers++;
            else if (isPortal(obj)) {
                result.totalPortals++;
                std::string mode = portalModeName(obj->m_objectID);
                if (!mode.empty()) result.modePortalCounts[mode]++;
            }
            else if (isDecoration(obj)) result.totalDecorations++;
            
            // Track groups
            for (int g : obj->m_groups) {
                if (g > 0) usedGroups.insert(g);
            }
        }
        
        result.levelLength = maxX - minX;
        result.groupsUsed = usedGroups.size();
        
        // Pass 2: Section analysis (divide level into 10 sections)
        int numSections = 10;
        float sectionWidth = result.levelLength / numSections;
        
        for (int s = 0; s < numSections; s++) {
            SectionInfo section;
            section.startX = minX + s * sectionWidth;
            section.endX = section.startX + sectionWidth;
            section.objectCount = 0;
            section.hazardCount = 0;
            section.triggerCount = 0;
            section.decorationCount = 0;
            
            for (int i = 0; i < objects->count(); i++) {
                auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (!obj) continue;
                float x = obj->getPositionX();
                if (x < section.startX || x >= section.endX) continue;
                
                section.objectCount++;
                if (isHazard(obj)) section.hazardCount++;
                if (isTrigger(obj)) section.triggerCount++;
                if (isDecoration(obj)) section.decorationCount++;
            }
            
            section.density = (sectionWidth > 0) ? 
                (section.objectCount / (sectionWidth / 30.0f)) : 0;
            
            result.sections.push_back(section);
        }
        
        // ---- Scoring ----
        
        // Gameplay score: mode variety, hazard distribution, flow
        int modeVariety = result.modePortalCounts.size();
        result.gameplayScore = std::min(100, 
            (modeVariety * 15) +                               // variety bonus
            (result.totalHazards > 10 ? 20 : result.totalHazards * 2) + // has gameplay
            (result.totalPortals > 5 ? 15 : result.totalPortals * 3) +  // has transitions
            (result.totalObjects > 500 ? 20 : result.totalObjects / 25)  // substantial
        );
        
        // Decoration score: density, visual richness
        float decoRatio = (result.totalObjects > 0) ?
            (float)result.totalDecorations / result.totalObjects : 0;
        result.decorationScore = std::min(100,
            (int)(decoRatio * 120) +           // high deco ratio is good
            (result.totalObjects > 2000 ? 30 : result.totalObjects / 70) + // lots of detail
            (result.groupsUsed > 20 ? 20 : result.groupsUsed)  // uses groups
        );
        
        // Trigger score: complexity and usage
        float triggerRatio = (result.totalObjects > 0) ?
            (float)result.totalTriggers / result.totalObjects : 0;
        result.triggerScore = std::min(100,
            (int)(triggerRatio * 200) +         // trigger usage
            (result.totalTriggers > 100 ? 30 : result.totalTriggers / 3) +
            (result.groupsUsed > 30 ? 20 : result.groupsUsed / 2)
        );
        
        // Overall weighted score
        result.overallScore = (
            result.gameplayScore * 35 +
            result.decorationScore * 40 +   // deco matters most for rating
            result.triggerScore * 25
        ) / 100;
        
        // ---- Difficulty estimation ----
        float hazardDensity = (result.levelLength > 0) ?
            result.totalHazards / (result.levelLength / 300.0f) : 0;
        
        if (hazardDensity < 3) result.estimatedDifficulty = "Easy/Normal";
        else if (hazardDensity < 6) result.estimatedDifficulty = "Hard";
        else if (hazardDensity < 10) result.estimatedDifficulty = "Harder";
        else if (hazardDensity < 15) result.estimatedDifficulty = "Insane";
        else if (hazardDensity < 25) result.estimatedDifficulty = "Easy Demon";
        else if (hazardDensity < 40) result.estimatedDifficulty = "Medium Demon";
        else if (hazardDensity < 60) result.estimatedDifficulty = "Hard Demon";
        else if (hazardDensity < 80) result.estimatedDifficulty = "Insane Demon";
        else result.estimatedDifficulty = "Extreme Demon";
        
        // ---- Issue detection ----
        
        // Check for empty sections
        for (int s = 0; s < (int)result.sections.size(); s++) {
            if (result.sections[s].objectCount < 5) {
                result.issues.push_back(
                    fmt::format("Section {} ({}-{}%) has very few objects — feels empty",
                        s + 1, s * 10, (s + 1) * 10));
            }
        }
        
        // Check decoration ratio
        if (decoRatio < 0.3f && result.totalObjects > 100) {
            result.issues.push_back("Low decoration ratio — level may look bare. Aim for 50%+ decoration objects.");
        }
        
        // Check trigger usage
        if (result.totalTriggers < 10 && result.totalObjects > 200) {
            result.issues.push_back("Very few triggers — consider adding move/pulse/color triggers for visual polish.");
        }
        
        // Check mode variety
        if (modeVariety < 2 && result.levelLength > 2000) {
            result.issues.push_back("Only one game mode used — add variety with ship, wave, or ball portals.");
        }
        
        // Check for consistent difficulty
        float maxDensity = 0, minDensity = 999;
        for (auto& s : result.sections) {
            if (s.hazardCount > 0) {
                if (s.density > maxDensity) maxDensity = s.density;
                if (s.density < minDensity) minDensity = s.density;
            }
        }
        if (maxDensity > minDensity * 5 && minDensity > 0) {
            result.issues.push_back("Large difficulty spikes — some sections are much harder than others.");
        }
        
        // ---- Tips ----
        
        if (result.overallScore < 30) {
            result.tips.push_back("Focus on adding decoration before submitting for rating.");
            result.tips.push_back("Use air decoration (glow, particles, lines) to fill empty space.");
        }
        if (result.overallScore >= 30 && result.overallScore < 60) {
            result.tips.push_back("Add pulse triggers synced to the music for visual rhythm.");
            result.tips.push_back("Use color triggers to create gradual color transitions.");
            result.tips.push_back("Add a custom background with parallax movement.");
        }
        if (result.overallScore >= 60) {
            result.tips.push_back("Strong foundation! Polish with particle effects and screen shake.");
            result.tips.push_back("Consider adding a custom art intro or transition.");
        }
        
        // Rating checklist
        if (result.totalObjects < 1000)
            result.tips.push_back("Rated levels typically have 1000+ objects. Currently: " + 
                std::to_string(result.totalObjects));
        if (result.totalTriggers < 50)
            result.tips.push_back("Rated levels use 50+ triggers for polish. Currently: " + 
                std::to_string(result.totalTriggers));
        
        return result;
    }

    // Format analysis as readable string
    inline std::string formatAnalysis(const LevelAnalysis& a) {
        std::string out;
        
        out += fmt::format(
            "<cg>Objects:</c> {} | <cy>Triggers:</c> {} | <cr>Hazards:</c> {} | <cp>Deco:</c> {}\n"
            "<cg>Groups:</c> {} | <cy>Portals:</c> {} | <cp>Length:</c> {:.0f}\n\n"
            "<cg>Gameplay:</c> {}/100 | <cy>Decoration:</c> {}/100 | <cp>Triggers:</c> {}/100\n"
            "<cj>Overall: {}/100</c> | Est. difficulty: <cr>{}</c>\n",
            a.totalObjects, a.totalTriggers, a.totalHazards, a.totalDecorations,
            a.groupsUsed, a.totalPortals, a.levelLength,
            a.gameplayScore, a.decorationScore, a.triggerScore,
            a.overallScore, a.estimatedDifficulty
        );
        
        if (!a.modePortalCounts.empty()) {
            out += "\n<cg>Game modes:</c> ";
            for (auto& [mode, count] : a.modePortalCounts) {
                out += fmt::format("{}({}) ", mode, count);
            }
            out += "\n";
        }
        
        if (!a.issues.empty()) {
            out += "\n<cr>Issues:</c>\n";
            for (auto& issue : a.issues) {
                out += "- " + issue + "\n";
            }
        }
        
        if (!a.tips.empty()) {
            out += "\n<cg>Tips:</c>\n";
            for (auto& tip : a.tips) {
                out += "- " + tip + "\n";
            }
        }
        
        return out;
    }

} // namespace studio::gameplay
