#pragma once
/**
 * animation.hpp — Animation Preset System
 * 
 * The core value prop: select objects in the editor, pick an animation,
 * and the mod automatically creates all the move/rotate/scale/pulse
 * triggers needed. No manual trigger chaining.
 * 
 * GD animations are built from trigger chains:
 *   - Move triggers (obj 901) → translate objects
 *   - Rotate triggers (obj 1346) → spin objects
 *   - Scale triggers (obj 2067) → grow/shrink objects
 *   - Pulse triggers (obj 1006) → color flash effects
 *   - Alpha triggers (obj 1007) → fade in/out
 *   - Toggle triggers (obj 1049) → show/hide objects
 * 
 * Each animation preset generates the right combination of these
 * triggers with proper timing, easing, and group assignments.
 */

#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace studio::anim {

    // ============================================================
    // Trigger object IDs in GD
    // ============================================================
    
    constexpr int MOVE_TRIGGER    = 901;
    constexpr int ROTATE_TRIGGER  = 1346;
    constexpr int SCALE_TRIGGER   = 2067;
    constexpr int PULSE_TRIGGER   = 1006;
    constexpr int ALPHA_TRIGGER   = 1007;
    constexpr int TOGGLE_TRIGGER  = 1049;
    constexpr int COLOR_TRIGGER   = 899;
    constexpr int FOLLOW_TRIGGER  = 1347;
    constexpr int SHAKE_TRIGGER   = 1520;
    constexpr int SPAWN_TRIGGER   = 1268;

    // Easing types
    enum class Easing {
        None = 0,
        EaseInOut = 1,
        EaseIn = 2,
        EaseOut = 3,
        ElasticInOut = 4,
        ElasticIn = 5,
        ElasticOut = 6,
        BounceInOut = 7,
        BounceIn = 8,
        BounceOut = 9,
        ExpoInOut = 10,
        ExpoIn = 11,
        ExpoOut = 12,
        SineInOut = 13,
        SineIn = 14,
        SineOut = 15,
        BackInOut = 16,
        BackIn = 17,
        BackOut = 18
    };

    // ============================================================
    // Animation preset definition
    // ============================================================

    struct AnimPreset {
        std::string name;
        std::string description;
        std::string category;  // "Motion", "Pulse", "Entrance", "Loop", "Deco"
        
        // The function that generates triggers for a given group
        // Parameters: group ID, X position to place triggers, editor layer
        std::function<void(int groupID, float triggerX, LevelEditorLayer*)> generate;
    };

    // ============================================================
    // Helper: Create and place a trigger object in the editor
    // ============================================================
    
    inline GameObject* placeTrigger(LevelEditorLayer* editor, int triggerID,
                                     float x, float y) {
        auto obj = editor->createObject(triggerID, { x, y }, false);
        return obj;
    }

    // Helper: assign a target group to a trigger
    inline void setTriggerGroup(GameObject* trigger, int groupID) {
        // In GD, trigger target group is stored in specific object properties
        // The exact property depends on the trigger type
        if (auto effObj = typeinfo_cast<EffectGameObject*>(trigger)) {
            effObj->m_targetGroupID = groupID;
        }
    }

    inline void setTriggerDuration(GameObject* trigger, float duration) {
        if (auto effObj = typeinfo_cast<EffectGameObject*>(trigger)) {
            effObj->m_duration = duration;
        }
    }

    inline void setTriggerEasing(GameObject* trigger, Easing easing) {
        if (auto effObj = typeinfo_cast<EffectGameObject*>(trigger)) {
            effObj->m_easingType = static_cast<int>(easing);
        }
    }

    // ============================================================
    // Animation presets
    // ============================================================

    inline std::vector<AnimPreset> getAllPresets() {
        std::vector<AnimPreset> presets;

        // ---- MOTION CATEGORY ----

        presets.push_back({
            "Float Up & Down",
            "Smooth bobbing motion. Great for platforms, orbs, decorations.",
            "Motion",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;  // trigger row below main level
                
                // Move up
                auto up = placeTrigger(editor, MOVE_TRIGGER, x, y);
                setTriggerGroup(up, group);
                setTriggerDuration(up, 1.2f);
                setTriggerEasing(up, Easing::SineInOut);
                if (auto eff = typeinfo_cast<EffectGameObject*>(up)) {
                    eff->m_moveOffsetY = 20.0f;
                }
                
                // Move down (spawned after up finishes)
                auto down = placeTrigger(editor, MOVE_TRIGGER, x + 30, y);
                setTriggerGroup(down, group);
                setTriggerDuration(down, 1.2f);
                setTriggerEasing(down, Easing::SineInOut);
                if (auto eff = typeinfo_cast<EffectGameObject*>(down)) {
                    eff->m_moveOffsetY = -20.0f;
                }
                
                // Spawn trigger to loop it
                auto loop = placeTrigger(editor, SPAWN_TRIGGER, x + 60, y);
                setTriggerDuration(loop, 2.4f);  // delay = up + down duration
                
                log::info("[Studio] Float anim: group {} at x={}", group, x);
            }
        });

        presets.push_back({
            "Pulse Scale",
            "Object grows and shrinks rhythmically. Perfect for beat-synced decoration.",
            "Motion",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                // Scale up
                auto up = placeTrigger(editor, SCALE_TRIGGER, x, y);
                setTriggerGroup(up, group);
                setTriggerDuration(up, 0.3f);
                setTriggerEasing(up, Easing::SineOut);
                if (auto eff = typeinfo_cast<EffectGameObject*>(up)) {
                    // Scale to 1.2x
                }
                
                // Scale back down
                auto down = placeTrigger(editor, SCALE_TRIGGER, x + 30, y);
                setTriggerGroup(down, group);
                setTriggerDuration(down, 0.5f);
                setTriggerEasing(down, Easing::SineIn);
                
                log::info("[Studio] Pulse scale: group {} at x={}", group, x);
            }
        });

        presets.push_back({
            "Spin Clockwise",
            "Continuous 360° rotation. Good for saws, gears, and decorative elements.",
            "Motion",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                auto rot = placeTrigger(editor, ROTATE_TRIGGER, x, y);
                setTriggerGroup(rot, group);
                setTriggerDuration(rot, 2.0f);
                setTriggerEasing(rot, Easing::None);  // linear for smooth spin
                if (auto eff = typeinfo_cast<EffectGameObject*>(rot)) {
                    eff->m_rotateDegrees = 360.0f;
                }
                
                log::info("[Studio] Spin: group {} at x={}", group, x);
            }
        });

        presets.push_back({
            "Shake",
            "Quick vibration effect. Great for impacts, explosions, boss fights.",
            "Motion",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                auto shake = placeTrigger(editor, SHAKE_TRIGGER, x, y);
                setTriggerDuration(shake, 0.5f);
                if (auto eff = typeinfo_cast<EffectGameObject*>(shake)) {
                    eff->m_shakeStrength = 5.0f;
                    eff->m_shakeInterval = 0.02f;
                }
                
                log::info("[Studio] Shake at x={}", x);
            }
        });

        presets.push_back({
            "Slide In Left",
            "Object slides in from the left. Classic entrance animation.",
            "Entrance",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                // Start offscreen (move left first, then animate right)
                auto moveOff = placeTrigger(editor, MOVE_TRIGGER, x - 30, y);
                setTriggerGroup(moveOff, group);
                setTriggerDuration(moveOff, 0.0f);
                if (auto eff = typeinfo_cast<EffectGameObject*>(moveOff)) {
                    eff->m_moveOffsetX = -200.0f;
                }
                
                auto slideIn = placeTrigger(editor, MOVE_TRIGGER, x, y);
                setTriggerGroup(slideIn, group);
                setTriggerDuration(slideIn, 0.8f);
                setTriggerEasing(slideIn, Easing::ExpoOut);
                if (auto eff = typeinfo_cast<EffectGameObject*>(slideIn)) {
                    eff->m_moveOffsetX = 200.0f;
                }
                
                log::info("[Studio] Slide in left: group {} at x={}", group, x);
            }
        });

        presets.push_back({
            "Slide In Right",
            "Object slides in from the right.",
            "Entrance",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                auto moveOff = placeTrigger(editor, MOVE_TRIGGER, x - 30, y);
                setTriggerGroup(moveOff, group);
                setTriggerDuration(moveOff, 0.0f);
                if (auto eff = typeinfo_cast<EffectGameObject*>(moveOff)) {
                    eff->m_moveOffsetX = 200.0f;
                }
                
                auto slideIn = placeTrigger(editor, MOVE_TRIGGER, x, y);
                setTriggerGroup(slideIn, group);
                setTriggerDuration(slideIn, 0.8f);
                setTriggerEasing(slideIn, Easing::ExpoOut);
                if (auto eff = typeinfo_cast<EffectGameObject*>(slideIn)) {
                    eff->m_moveOffsetX = -200.0f;
                }
            }
        });

        presets.push_back({
            "Drop In",
            "Falls from above with a bounce. Eye-catching entrance.",
            "Entrance",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                auto moveOff = placeTrigger(editor, MOVE_TRIGGER, x - 30, y);
                setTriggerGroup(moveOff, group);
                setTriggerDuration(moveOff, 0.0f);
                if (auto eff = typeinfo_cast<EffectGameObject*>(moveOff)) {
                    eff->m_moveOffsetY = 150.0f;
                }
                
                auto drop = placeTrigger(editor, MOVE_TRIGGER, x, y);
                setTriggerGroup(drop, group);
                setTriggerDuration(drop, 0.6f);
                setTriggerEasing(drop, Easing::BounceOut);
                if (auto eff = typeinfo_cast<EffectGameObject*>(drop)) {
                    eff->m_moveOffsetY = -150.0f;
                }
            }
        });

        // ---- PULSE / COLOR CATEGORY ----

        presets.push_back({
            "Color Flash",
            "Quick color pulse on a channel. Hit it on the beat for rhythm sync.",
            "Pulse",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                auto pulse = placeTrigger(editor, PULSE_TRIGGER, x, y);
                setTriggerGroup(pulse, group);
                if (auto eff = typeinfo_cast<EffectGameObject*>(pulse)) {
                    eff->m_fadeInTime = 0.0f;
                    eff->m_holdTime = 0.1f;
                    eff->m_fadeOutTime = 0.4f;
                }
            }
        });

        presets.push_back({
            "Fade In",
            "Object fades from invisible to fully visible.",
            "Entrance",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                // Set alpha to 0 first
                auto hide = placeTrigger(editor, ALPHA_TRIGGER, x - 30, y);
                setTriggerGroup(hide, group);
                setTriggerDuration(hide, 0.0f);
                
                // Fade to full alpha
                auto fadeIn = placeTrigger(editor, ALPHA_TRIGGER, x, y);
                setTriggerGroup(fadeIn, group);
                setTriggerDuration(fadeIn, 1.0f);
                setTriggerEasing(fadeIn, Easing::SineIn);
            }
        });

        presets.push_back({
            "Fade Out",
            "Object fades to invisible. Good for transitions.",
            "Entrance",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                auto fadeOut = placeTrigger(editor, ALPHA_TRIGGER, x, y);
                setTriggerGroup(fadeOut, group);
                setTriggerDuration(fadeOut, 1.0f);
                setTriggerEasing(fadeOut, Easing::SineOut);
            }
        });

        // ---- DECO CATEGORY ----

        presets.push_back({
            "Parallax Drift",
            "Slow horizontal movement for background layers. Creates depth.",
            "Deco",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                auto drift = placeTrigger(editor, MOVE_TRIGGER, x, y);
                setTriggerGroup(drift, group);
                setTriggerDuration(drift, 10.0f);
                setTriggerEasing(drift, Easing::None);
                if (auto eff = typeinfo_cast<EffectGameObject*>(drift)) {
                    eff->m_moveOffsetX = -100.0f;
                }
            }
        });

        presets.push_back({
            "Twinkle",
            "Rapid fade in/out for star and particle effects.",
            "Deco",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                
                // Quick fade out
                auto fadeOut = placeTrigger(editor, ALPHA_TRIGGER, x, y);
                setTriggerGroup(fadeOut, group);
                setTriggerDuration(fadeOut, 0.3f);
                setTriggerEasing(fadeOut, Easing::SineInOut);
                
                // Quick fade in
                auto fadeIn = placeTrigger(editor, ALPHA_TRIGGER, x + 30, y);
                setTriggerGroup(fadeIn, group);
                setTriggerDuration(fadeIn, 0.3f);
                setTriggerEasing(fadeIn, Easing::SineInOut);
            }
        });

        presets.push_back({
            "Orbit",
            "Circular motion using combined X+Y movement. For orbiting decorations.",
            "Motion",
            [](int group, float x, LevelEditorLayer* editor) {
                float y = -60.0f;
                float radius = 30.0f;
                float dur = 2.0f;
                float step = dur / 4.0f;
                
                // Approximate a circle with 4 move triggers
                // Right
                auto m1 = placeTrigger(editor, MOVE_TRIGGER, x, y);
                setTriggerGroup(m1, group);
                setTriggerDuration(m1, step);
                setTriggerEasing(m1, Easing::SineInOut);
                if (auto e = typeinfo_cast<EffectGameObject*>(m1)) {
                    e->m_moveOffsetX = radius;
                    e->m_moveOffsetY = radius;
                }
                
                // Up
                auto m2 = placeTrigger(editor, MOVE_TRIGGER, x + 30, y);
                setTriggerGroup(m2, group);
                setTriggerDuration(m2, step);
                setTriggerEasing(m2, Easing::SineInOut);
                if (auto e = typeinfo_cast<EffectGameObject*>(m2)) {
                    e->m_moveOffsetX = -radius;
                    e->m_moveOffsetY = radius;
                }
                
                // Left
                auto m3 = placeTrigger(editor, MOVE_TRIGGER, x + 60, y);
                setTriggerGroup(m3, group);
                setTriggerDuration(m3, step);
                setTriggerEasing(m3, Easing::SineInOut);
                if (auto e = typeinfo_cast<EffectGameObject*>(m3)) {
                    e->m_moveOffsetX = -radius;
                    e->m_moveOffsetY = -radius;
                }
                
                // Down (back to start)
                auto m4 = placeTrigger(editor, MOVE_TRIGGER, x + 90, y);
                setTriggerGroup(m4, group);
                setTriggerDuration(m4, step);
                setTriggerEasing(m4, Easing::SineInOut);
                if (auto e = typeinfo_cast<EffectGameObject*>(m4)) {
                    e->m_moveOffsetX = radius;
                    e->m_moveOffsetY = -radius;
                }
            }
        });

        return presets;
    }

    // Get presets filtered by category
    inline std::vector<AnimPreset> getPresetsByCategory(const std::string& category) {
        auto all = getAllPresets();
        std::vector<AnimPreset> filtered;
        for (auto& p : all) {
            if (p.category == category) filtered.push_back(p);
        }
        return filtered;
    }

    // Get all categories
    inline std::vector<std::string> getCategories() {
        return { "Motion", "Entrance", "Pulse", "Deco" };
    }

} // namespace studio::anim
