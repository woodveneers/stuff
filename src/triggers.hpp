#pragma once
/**
 * triggers.hpp — Quick Trigger System
 * 
 * The pain point this solves: GD's trigger system is powerful but
 * the workflow is painful. You have to manually place a trigger,
 * open its settings, type in the group ID, set duration, set easing,
 * close the menu, position it correctly... for EVERY trigger.
 * 
 * Quick Triggers lets you:
 * 1. Select objects that already exist in your gameplay
 * 2. Pick a trigger action from a simple menu
 * 3. It auto-assigns groups, places triggers, and configures them
 * 
 * Also includes:
 * - Group manager (see what's in each group, rename, merge)
 * - Trigger templates (common multi-trigger patterns)
 * - Bulk operations (add trigger to everything in a section)
 */

#include <Geode/Geode.hpp>
#include <vector>
#include <set>
#include <map>
using namespace geode::prelude;

namespace studio::triggers {

    // Trigger object IDs
    constexpr int MOVE      = 901;
    constexpr int ROTATE    = 1346;
    constexpr int SCALE     = 2067;
    constexpr int PULSE     = 1006;
    constexpr int ALPHA     = 1007;
    constexpr int TOGGLE    = 1049;
    constexpr int COLOR     = 899;
    constexpr int FOLLOW    = 1347;
    constexpr int SHAKE     = 1520;
    constexpr int SPAWN     = 1268;
    constexpr int STOP      = 1616;
    constexpr int TOUCH     = 1595;
    constexpr int COUNT     = 1611;
    constexpr int PICKUP    = 1817;
    constexpr int COLLISION = 1815;
    constexpr int ON_DEATH  = 1812;

    // ============================================================
    // Quick trigger action — what the user wants to do
    // ============================================================
    
    struct QuickAction {
        std::string name;
        std::string description;
        std::string category;     // "Motion", "Visual", "Logic", "Template"
        int triggerID;            // primary trigger used
        
        // Configure the trigger after placement
        // params: trigger object, target group, editor
        std::function<void(EffectGameObject*, int group, LevelEditorLayer*)> configure;
    };

    // ============================================================
    // Group management helpers
    // ============================================================

    // Find the next unused group ID by scanning all objects
    inline int findNextFreeGroup(LevelEditorLayer* editor, int startFrom = 1) {
        std::set<int> usedGroups;
        
        auto objects = editor->m_objects;
        if (objects) {
            for (int i = 0; i < objects->count(); i++) {
                auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (!obj) continue;
                for (int g : obj->m_groups) {
                    if (g > 0) usedGroups.insert(g);
                }
            }
        }
        
        int id = startFrom;
        while (usedGroups.count(id)) id++;
        return id;
    }

    // Assign a group to selected objects, returns the group ID used
    inline int assignGroupToSelection(EditorUI* editorUI, LevelEditorLayer* editor, int groupID = -1) {
        auto selected = editorUI->getSelectedObjects();
        if (!selected || selected->count() == 0) return -1;
        
        if (groupID < 0) {
            groupID = findNextFreeGroup(editor);
        }
        
        for (int i = 0; i < selected->count(); i++) {
            auto obj = static_cast<GameObject*>(selected->objectAtIndex(i));
            if (obj) {
                obj->addToGroup(groupID);
            }
        }
        
        return groupID;
    }

    // Get all groups used by selected objects
    inline std::set<int> getGroupsFromSelection(EditorUI* editorUI) {
        std::set<int> groups;
        auto selected = editorUI->getSelectedObjects();
        if (!selected) return groups;
        
        for (int i = 0; i < selected->count(); i++) {
            auto obj = static_cast<GameObject*>(selected->objectAtIndex(i));
            if (!obj) continue;
            for (int g : obj->m_groups) {
                if (g > 0) groups.insert(g);
            }
        }
        return groups;
    }

    // Get the X position of the leftmost selected object
    inline float getSelectionMinX(EditorUI* editorUI) {
        auto selected = editorUI->getSelectedObjects();
        if (!selected || selected->count() == 0) return 0;
        
        float minX = 999999.0f;
        for (int i = 0; i < selected->count(); i++) {
            auto obj = static_cast<GameObject*>(selected->objectAtIndex(i));
            if (obj && obj->getPositionX() < minX) {
                minX = obj->getPositionX();
            }
        }
        return minX;
    }

    // Get the average Y position of selection (for vertical centering)
    inline float getSelectionCenterY(EditorUI* editorUI) {
        auto selected = editorUI->getSelectedObjects();
        if (!selected || selected->count() == 0) return 0;
        
        float totalY = 0;
        int count = 0;
        for (int i = 0; i < selected->count(); i++) {
            auto obj = static_cast<GameObject*>(selected->objectAtIndex(i));
            if (obj) {
                totalY += obj->getPositionY();
                count++;
            }
        }
        return count > 0 ? totalY / count : 0;
    }

    // ============================================================
    // Place a trigger and configure it
    // ============================================================

    inline EffectGameObject* placeAndConfigure(
        LevelEditorLayer* editor, int triggerID,
        float x, float y, int targetGroup
    ) {
        auto obj = editor->createObject(triggerID, { x, y }, false);
        auto eff = typeinfo_cast<EffectGameObject*>(obj);
        if (eff) {
            eff->m_targetGroupID = targetGroup;
        }
        return eff;
    }

    // ============================================================
    // All quick actions
    // ============================================================

    inline std::vector<QuickAction> getAllActions() {
        std::vector<QuickAction> actions;

        // ========== MOTION ==========

        actions.push_back({
            "Move Up", "Move selected objects up by 30 units over 0.5s",
            "Motion", MOVE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 0.5f;
                t->m_easingType = 13;  // SineInOut
                t->m_moveOffsetY = 30.0f;
            }
        });

        actions.push_back({
            "Move Down", "Move selected objects down by 30 units over 0.5s",
            "Motion", MOVE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 0.5f;
                t->m_easingType = 13;
                t->m_moveOffsetY = -30.0f;
            }
        });

        actions.push_back({
            "Move Left", "Move selected objects left by 30 units",
            "Motion", MOVE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 0.5f;
                t->m_easingType = 13;
                t->m_moveOffsetX = -30.0f;
            }
        });

        actions.push_back({
            "Move Right", "Move selected objects right by 30 units",
            "Motion", MOVE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 0.5f;
                t->m_easingType = 13;
                t->m_moveOffsetX = 30.0f;
            }
        });

        actions.push_back({
            "Rotate 90°", "Spin selected objects 90 degrees",
            "Motion", ROTATE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 0.5f;
                t->m_easingType = 3;  // EaseOut
                t->m_rotateDegrees = 90.0f;
            }
        });

        actions.push_back({
            "Rotate 360°", "Full spin — continuous rotation",
            "Motion", ROTATE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 2.0f;
                t->m_easingType = 0;  // None (linear)
                t->m_rotateDegrees = 360.0f;
            }
        });

        actions.push_back({
            "Scale Up 1.5x", "Grow objects to 1.5x size",
            "Motion", SCALE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 0.4f;
                t->m_easingType = 6;  // ElasticOut (bouncy feel)
            }
        });

        actions.push_back({
            "Follow Player", "Objects follow the player's Y position",
            "Motion", FOLLOW,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 10.0f;
                // Follow Y = 1.0, Follow X = 0
            }
        });

        // ========== VISUAL ==========

        actions.push_back({
            "Pulse White", "Quick white flash on the beat",
            "Visual", PULSE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_fadeInTime = 0.0f;
                t->m_holdTime = 0.05f;
                t->m_fadeOutTime = 0.3f;
                // Pulse to white
            }
        });

        actions.push_back({
            "Pulse Red", "Red color flash — impact/danger feel",
            "Visual", PULSE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_fadeInTime = 0.0f;
                t->m_holdTime = 0.08f;
                t->m_fadeOutTime = 0.4f;
            }
        });

        actions.push_back({
            "Fade In", "Objects appear gradually over 1 second",
            "Visual", ALPHA,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 1.0f;
                t->m_easingType = 14;  // SineIn
                // Fade from 0 to 1
                t->m_opacity = 1.0f;
            }
        });

        actions.push_back({
            "Fade Out", "Objects disappear over 1 second",
            "Visual", ALPHA,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_duration = 1.0f;
                t->m_easingType = 15;  // SineOut
                t->m_opacity = 0.0f;
            }
        });

        actions.push_back({
            "Toggle Off", "Hide selected objects instantly",
            "Visual", TOGGLE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_activateGroup = false;
            }
        });

        actions.push_back({
            "Toggle On", "Show previously hidden objects",
            "Visual", TOGGLE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_activateGroup = true;
            }
        });

        actions.push_back({
            "Screen Shake", "Quick camera shake — impact effect",
            "Visual", SHAKE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_duration = 0.3f;
                t->m_shakeStrength = 4.0f;
                t->m_shakeInterval = 0.02f;
            }
        });

        actions.push_back({
            "Big Shake", "Heavy camera shake — boss/explosion feel",
            "Visual", SHAKE,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_duration = 0.6f;
                t->m_shakeStrength = 10.0f;
                t->m_shakeInterval = 0.015f;
            }
        });

        // ========== LOGIC ==========

        actions.push_back({
            "Spawn Trigger", "Spawn a group of triggers after a delay",
            "Logic", SPAWN,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_spawnDelay = 0.5f;
            }
        });

        actions.push_back({
            "Stop Trigger", "Stop all triggers acting on the selected group",
            "Logic", STOP,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
            }
        });

        actions.push_back({
            "Touch Trigger", "Activates when player touches the trigger area",
            "Logic", TOUCH,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
                t->m_holdMode = false;
                t->m_touchToggle = 0;  // on touch
            }
        });

        actions.push_back({
            "On Death", "Triggers when the player dies",
            "Logic", ON_DEATH,
            [](EffectGameObject* t, int g, LevelEditorLayer*) {
                t->m_targetGroupID = g;
            }
        });

        // ========== TEMPLATES (multi-trigger) ==========

        actions.push_back({
            "Appear on Beat", "Objects start hidden, pulse in on the beat, then stay visible",
            "Template", TOGGLE,
            [](EffectGameObject* t, int g, LevelEditorLayer* editor) {
                float x = t->getPositionX();
                float y = t->getPositionY();
                
                // Toggle off (instant, at level start)
                t->m_targetGroupID = g;
                t->m_activateGroup = false;
                
                // Toggle on at this position
                auto toggleOn = editor->createObject(TOGGLE, { x + 30, y }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(toggleOn)) {
                    eff->m_targetGroupID = g;
                    eff->m_activateGroup = true;
                }
                
                // Pulse effect
                auto pulse = editor->createObject(PULSE, { x + 30, y - 30 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(pulse)) {
                    eff->m_targetGroupID = g;
                    eff->m_fadeInTime = 0.0f;
                    eff->m_holdTime = 0.05f;
                    eff->m_fadeOutTime = 0.5f;
                }
                
                // Scale pop
                auto scale = editor->createObject(SCALE, { x + 30, y - 60 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(scale)) {
                    eff->m_targetGroupID = g;
                    eff->m_duration = 0.3f;
                    eff->m_easingType = 6;  // ElasticOut
                }
            }
        });

        actions.push_back({
            "Disappear", "Objects fade out + shrink + toggle off",
            "Template", ALPHA,
            [](EffectGameObject* t, int g, LevelEditorLayer* editor) {
                float x = t->getPositionX();
                float y = t->getPositionY();
                
                // Alpha fade
                t->m_targetGroupID = g;
                t->m_duration = 0.5f;
                t->m_opacity = 0.0f;
                t->m_easingType = 15;  // SineOut
                
                // Scale down
                auto scale = editor->createObject(SCALE, { x, y - 30 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(scale)) {
                    eff->m_targetGroupID = g;
                    eff->m_duration = 0.5f;
                    eff->m_easingType = 2;  // EaseIn
                }
                
                // Toggle off after fade completes
                auto toggle = editor->createObject(TOGGLE, { x + 60, y }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(toggle)) {
                    eff->m_targetGroupID = g;
                    eff->m_activateGroup = false;
                }
            }
        });

        actions.push_back({
            "Platform Move", "Platform moves right, pauses, returns. Classic moving platform.",
            "Template", MOVE,
            [](EffectGameObject* t, int g, LevelEditorLayer* editor) {
                float x = t->getPositionX();
                float y = t->getPositionY();
                
                // Move right
                t->m_targetGroupID = g;
                t->m_duration = 1.5f;
                t->m_easingType = 1;  // EaseInOut
                t->m_moveOffsetX = 120.0f;
                
                // Move back (spawned after delay)
                auto back = editor->createObject(MOVE, { x + 90, y }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(back)) {
                    eff->m_targetGroupID = g;
                    eff->m_duration = 1.5f;
                    eff->m_easingType = 1;
                    eff->m_moveOffsetX = -120.0f;
                }
                
                // Spawn trigger to loop
                auto loop = editor->createObject(SPAWN, { x + 180, y }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(loop)) {
                    eff->m_spawnDelay = 3.5f;  // total cycle time
                }
            }
        });

        actions.push_back({
            "Saw Spin", "Continuous rotation + slight floating. Instant saw setup.",
            "Template", ROTATE,
            [](EffectGameObject* t, int g, LevelEditorLayer* editor) {
                float x = t->getPositionX();
                float y = t->getPositionY();
                
                // Continuous rotation
                t->m_targetGroupID = g;
                t->m_duration = 2.0f;
                t->m_easingType = 0;  // Linear
                t->m_rotateDegrees = 360.0f;
                t->m_lockObjectRotation = true;
                
                // Slight vertical bob
                auto bobUp = editor->createObject(MOVE, { x, y - 30 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(bobUp)) {
                    eff->m_targetGroupID = g;
                    eff->m_duration = 1.0f;
                    eff->m_easingType = 13;  // SineInOut
                    eff->m_moveOffsetY = 10.0f;
                }
                
                auto bobDown = editor->createObject(MOVE, { x + 30, y - 30 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(bobDown)) {
                    eff->m_targetGroupID = g;
                    eff->m_duration = 1.0f;
                    eff->m_easingType = 13;
                    eff->m_moveOffsetY = -10.0f;
                }
            }
        });

        actions.push_back({
            "Boss Entrance", "Dramatic entrance: shake + scale pop + pulse. For mini-bosses or big moments.",
            "Template", SHAKE,
            [](EffectGameObject* t, int g, LevelEditorLayer* editor) {
                float x = t->getPositionX();
                float y = t->getPositionY();
                
                // Big shake
                t->m_duration = 0.8f;
                t->m_shakeStrength = 12.0f;
                t->m_shakeInterval = 0.02f;
                
                // Scale pop on the group
                auto scale = editor->createObject(SCALE, { x, y - 30 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(scale)) {
                    eff->m_targetGroupID = g;
                    eff->m_duration = 0.4f;
                    eff->m_easingType = 6;  // ElasticOut
                }
                
                // Pulse white
                auto pulse = editor->createObject(PULSE, { x, y - 60 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(pulse)) {
                    eff->m_targetGroupID = g;
                    eff->m_fadeInTime = 0.0f;
                    eff->m_holdTime = 0.15f;
                    eff->m_fadeOutTime = 0.6f;
                }
                
                // Toggle on (in case it was hidden)
                auto toggle = editor->createObject(TOGGLE, { x, y - 90 }, false);
                if (auto eff = typeinfo_cast<EffectGameObject*>(toggle)) {
                    eff->m_targetGroupID = g;
                    eff->m_activateGroup = true;
                }
            }
        });

        actions.push_back({
            "Color Transition", "Smooth color change over 2 seconds. Place at transition points.",
            "Template", COLOR,
            [](EffectGameObject* t, int g, LevelEditorLayer* editor) {
                t->m_targetGroupID = g;
                t->m_duration = 2.0f;
                // Color target is set by user after placement
                // This just creates the trigger framework
            }
        });

        return actions;
    }

    // Get actions by category
    inline std::vector<QuickAction> getActionsByCategory(const std::string& cat) {
        auto all = getAllActions();
        std::vector<QuickAction> out;
        for (auto& a : all) {
            if (a.category == cat) out.push_back(a);
        }
        return out;
    }

    inline std::vector<std::string> getCategories() {
        return { "Motion", "Visual", "Logic", "Template" };
    }

    // ============================================================
    // Group info for the group manager
    // ============================================================

    struct GroupInfo {
        int id;
        int objectCount;
        int triggerCount;        // triggers targeting this group
        float minX, maxX;       // spatial range
        std::string summary;
    };

    inline std::vector<GroupInfo> scanGroups(LevelEditorLayer* editor) {
        std::map<int, GroupInfo> groupMap;
        
        auto objects = editor->m_objects;
        if (!objects) return {};
        
        for (int i = 0; i < objects->count(); i++) {
            auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (!obj) continue;
            
            float x = obj->getPositionX();
            
            for (int g : obj->m_groups) {
                if (g <= 0) continue;
                
                auto& info = groupMap[g];
                info.id = g;
                info.objectCount++;
                
                if (info.objectCount == 1) {
                    info.minX = info.maxX = x;
                } else {
                    if (x < info.minX) info.minX = x;
                    if (x > info.maxX) info.maxX = x;
                }
            }
            
            // Check if this is a trigger targeting a group
            if (auto eff = typeinfo_cast<EffectGameObject*>(obj)) {
                int target = eff->m_targetGroupID;
                if (target > 0) {
                    groupMap[target].triggerCount++;
                }
            }
        }
        
        std::vector<GroupInfo> result;
        for (auto& [id, info] : groupMap) {
            info.summary = fmt::format("Group {}: {} objs, {} triggers, x=[{:.0f}-{:.0f}]",
                info.id, info.objectCount, info.triggerCount, info.minX, info.maxX);
            result.push_back(info);
        }
        
        // Sort by group ID
        std::sort(result.begin(), result.end(),
            [](const GroupInfo& a, const GroupInfo& b) { return a.id < b.id; });
        
        return result;
    }

} // namespace studio::triggers
