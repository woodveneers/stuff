/**
 * GD Creator Studio — main.cpp
 * 
 * A comprehensive level creation toolkit for Geometry Dash.
 * Adds custom editor panels for:
 *   - Animation Studio: one-click animation presets
 *   - Color & Design: palettes, gradients, schemes
 *   - Level Analyzer: difficulty estimation, rating checklist
 *   - Quick Tools: common trigger setups, structure helpers
 * 
 * Hooks into EditorUI to add panels alongside the standard
 * Build/Edit/Delete tabs using the EditorTab API (optional)
 * or a custom floating panel system.
 * 
 * Built for Geode v5.0.0-beta.4 / GD 2.2081
 */

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

#include "ui.hpp"
#include "animation.hpp"
#include "design.hpp"
#include "gameplay.hpp"
#include "triggers.hpp"
#include "shapes.hpp"
#include "particles.hpp"

using namespace geode::prelude;

// ============================================================
// State
// ============================================================

static LevelEditorLayer* g_editor = nullptr;
static int g_nextGroupID = 1;   // auto-incrementing group for animations
static int g_selectedAnimPreset = 0;
static int g_selectedPalette = 0;

// ============================================================
// Find the next available group ID
// ============================================================

static int getNextFreeGroup(LevelEditorLayer* editor) {
    // Start from a high number to avoid conflicts with user's groups
    static int counter = 500;
    return counter++;
}

// ============================================================
// Panel: Animation Studio
// ============================================================

class AnimationPanel : public CCLayer {
public:
    static AnimationPanel* create(EditorUI* editorUI) {
        auto ret = new AnimationPanel();
        if (ret && ret->init(editorUI)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    
    bool init(EditorUI* editorUI) {
        if (!CCLayer::init()) return false;
        
        m_editorUI = editorUI;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float panelW = 340.0f;
        float panelH = 220.0f;
        
        this->setContentSize({ panelW, panelH });
        this->setPosition({ winSize.width / 2, winSize.height / 2 + 40.0f });
        this->setAnchorPoint({ 0.5f, 0.5f });
        
        // Background
        auto bg = studio::ui::panelBG(panelW, panelH, { 20, 20, 35, 220 });
        bg->setPosition({ panelW / 2, panelH / 2 });
        this->addChild(bg, -1);
        
        // Title
        auto title = studio::ui::sectionLabel("Animation Studio", 0.5f);
        title->setPosition({ panelW / 2, panelH - 15.0f });
        this->addChild(title);
        
        // Menu for buttons
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu);
        
        // Category buttons
        auto categories = studio::anim::getCategories();
        float catX = 45.0f;
        for (auto& cat : categories) {
            auto btn = studio::ui::textButton(
                cat.c_str(), this, menu_selector(AnimationPanel::onSelectCategory),
                "GJ_button_04.png", 70.0f, 0.4f, 24.0f
            );
            btn->setPosition({ catX, panelH - 40.0f });
            btn->setTag(catX);  // store position as tag for identification
            menu->addChild(btn);
            catX += 80.0f;
        }
        
        // Preset list area
        m_presetLabel = CCLabelBMFont::create("Select a category above", "chatFont.fnt");
        m_presetLabel->setScale(0.4f);
        m_presetLabel->setPosition({ panelW / 2, panelH - 70.0f });
        m_presetLabel->setColor({ 180, 180, 180 });
        this->addChild(m_presetLabel);
        
        // Preset navigation
        auto prevBtn = studio::ui::textButton("<", this, 
            menu_selector(AnimationPanel::onPrevPreset), "GJ_button_01.png", 30.0f, 0.5f, 25.0f);
        prevBtn->setPosition({ 30.0f, panelH / 2 - 10.0f });
        menu->addChild(prevBtn);
        
        auto nextBtn = studio::ui::textButton(">", this,
            menu_selector(AnimationPanel::onNextPreset), "GJ_button_01.png", 30.0f, 0.5f, 25.0f);
        nextBtn->setPosition({ panelW - 30.0f, panelH / 2 - 10.0f });
        menu->addChild(nextBtn);
        
        // Description
        m_descLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_descLabel->setScale(0.35f);
        m_descLabel->setPosition({ panelW / 2, panelH / 2 - 35.0f });
        m_descLabel->setColor({ 150, 200, 150 });
        m_descLabel->setWidth(280.0f);
        this->addChild(m_descLabel);
        
        // Apply button
        auto applyBtn = studio::ui::textButton(
            "Apply to Selection", this, menu_selector(AnimationPanel::onApply),
            "GJ_button_02.png", 130.0f, 0.5f, 30.0f
        );
        applyBtn->setPosition({ panelW / 2, 30.0f });
        menu->addChild(applyBtn);
        
        // Close button
        auto closeBtn = studio::ui::textButton(
            "X", this, menu_selector(AnimationPanel::onClose),
            "GJ_button_06.png", 25.0f, 0.5f, 25.0f
        );
        closeBtn->setPosition({ panelW - 15.0f, panelH - 15.0f });
        menu->addChild(closeBtn);
        
        // Load default category
        loadCategory("Motion");
        
        return true;
    }
    
    void loadCategory(const std::string& category) {
        m_currentPresets = studio::anim::getPresetsByCategory(category);
        m_currentIndex = 0;
        m_currentCategory = category;
        updatePresetDisplay();
    }
    
    void updatePresetDisplay() {
        if (m_currentPresets.empty()) {
            m_presetLabel->setString("No presets in this category");
            m_descLabel->setString("");
            return;
        }
        
        auto& preset = m_currentPresets[m_currentIndex];
        std::string label = fmt::format("{} ({}/{})", 
            preset.name, m_currentIndex + 1, m_currentPresets.size());
        m_presetLabel->setString(label.c_str());
        m_descLabel->setString(preset.description.c_str());
    }
    
    void onSelectCategory(CCObject* sender) {
        // Determine which category based on button text
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        auto categories = studio::anim::getCategories();
        
        // Simple cycling through categories
        static int catIdx = 0;
        catIdx = (catIdx + 1) % categories.size();
        loadCategory(categories[catIdx]);
    }
    
    void onPrevPreset(CCObject*) {
        if (m_currentPresets.empty()) return;
        m_currentIndex = (m_currentIndex - 1 + m_currentPresets.size()) % m_currentPresets.size();
        updatePresetDisplay();
    }
    
    void onNextPreset(CCObject*) {
        if (m_currentPresets.empty()) return;
        m_currentIndex = (m_currentIndex + 1) % m_currentPresets.size();
        updatePresetDisplay();
    }
    
    void onApply(CCObject*) {
        if (m_currentPresets.empty() || !g_editor) return;
        
        auto& preset = m_currentPresets[m_currentIndex];
        
        // Get selected objects from the editor
        auto selectedObjs = m_editorUI->getSelectedObjects();
        if (!selectedObjs || selectedObjs->count() == 0) {
            studio::ui::showInfo("Animation", "Select some objects first, then apply an animation!");
            return;
        }
        
        // Assign a group to selected objects
        int groupID = getNextFreeGroup(g_editor);
        
        // Add the group to all selected objects
        for (int i = 0; i < selectedObjs->count(); i++) {
            auto obj = static_cast<GameObject*>(selectedObjs->objectAtIndex(i));
            if (obj) {
                // Add group to object
                obj->addToGroup(groupID);
            }
        }
        
        // Get the X position of the leftmost selected object (for trigger placement)
        float minX = 999999.0f;
        for (int i = 0; i < selectedObjs->count(); i++) {
            auto obj = static_cast<GameObject*>(selectedObjs->objectAtIndex(i));
            if (obj && obj->getPositionX() < minX) {
                minX = obj->getPositionX();
            }
        }
        
        // Generate the trigger chain
        preset.generate(groupID, minX, g_editor);
        
        studio::ui::showInfo("Animation Applied",
            fmt::format("<cg>{}</c> applied to {} objects!\n\n"
                "Group ID: <cy>{}</c>\n"
                "Triggers placed at x={:.0f}, y=-60\n\n"
                "You can edit the triggers manually for fine-tuning.",
                preset.name, selectedObjs->count(), groupID, minX));
    }
    
    void onClose(CCObject*) {
        this->removeFromParent();
    }
    
private:
    EditorUI* m_editorUI = nullptr;
    CCLabelBMFont* m_presetLabel = nullptr;
    CCLabelBMFont* m_descLabel = nullptr;
    std::vector<studio::anim::AnimPreset> m_currentPresets;
    std::string m_currentCategory;
    int m_currentIndex = 0;
};

// ============================================================
// Panel: Color Palette
// ============================================================

class PalettePanel : public CCLayer {
public:
    static PalettePanel* create() {
        auto ret = new PalettePanel();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    
    bool init() {
        if (!CCLayer::init()) return false;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float panelW = 300.0f;
        float panelH = 250.0f;
        
        this->setContentSize({ panelW, panelH });
        this->setPosition({ winSize.width / 2, winSize.height / 2 + 30.0f });
        this->setAnchorPoint({ 0.5f, 0.5f });
        
        auto bg = studio::ui::panelBG(panelW, panelH, { 20, 20, 35, 220 });
        bg->setPosition({ panelW / 2, panelH / 2 });
        this->addChild(bg, -1);
        
        auto title = studio::ui::sectionLabel("Color Palettes", 0.5f);
        title->setPosition({ panelW / 2, panelH - 15.0f });
        this->addChild(title);
        
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu);
        
        // Palette navigation
        auto prevBtn = studio::ui::textButton("<", this,
            menu_selector(PalettePanel::onPrev), "GJ_button_01.png", 30.0f, 0.5f, 25.0f);
        prevBtn->setPosition({ 25.0f, panelH / 2 });
        menu->addChild(prevBtn);
        
        auto nextBtn = studio::ui::textButton(">", this,
            menu_selector(PalettePanel::onNext), "GJ_button_01.png", 30.0f, 0.5f, 25.0f);
        nextBtn->setPosition({ panelW - 25.0f, panelH / 2 });
        menu->addChild(nextBtn);
        
        // Palette name
        m_nameLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_nameLabel->setScale(0.4f);
        m_nameLabel->setPosition({ panelW / 2, panelH - 40.0f });
        this->addChild(m_nameLabel);
        
        // Style + description
        m_descLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_descLabel->setScale(0.32f);
        m_descLabel->setPosition({ panelW / 2, panelH - 58.0f });
        m_descLabel->setColor({ 180, 180, 180 });
        m_descLabel->setWidth(260.0f);
        this->addChild(m_descLabel);
        
        // Color swatches - 6 colored boxes
        float swatchY = panelH / 2 - 10.0f;
        float swatchSize = 35.0f;
        float swatchSpacing = 42.0f;
        float startX = panelW / 2 - (swatchSpacing * 2.5f);
        
        std::vector<std::string> labels = { "BG", "Ground", "Main", "Accent", "Detail", "Text" };
        
        for (int i = 0; i < 6; i++) {
            auto swatch = CCLayerColor::create({ 100, 100, 100, 255 }, swatchSize, swatchSize);
            swatch->setPosition({ startX + i * swatchSpacing - swatchSize/2, swatchY - swatchSize/2 });
            swatch->setTag(100 + i);
            this->addChild(swatch, 1);
            
            auto lbl = CCLabelBMFont::create(labels[i].c_str(), "chatFont.fnt");
            lbl->setScale(0.25f);
            lbl->setPosition({ startX + i * swatchSpacing, swatchY - swatchSize/2 - 10.0f });
            lbl->setColor({ 150, 150, 150 });
            this->addChild(lbl);
        }
        
        // Color values label
        m_colorInfo = CCLabelBMFont::create("", "chatFont.fnt");
        m_colorInfo->setScale(0.3f);
        m_colorInfo->setPosition({ panelW / 2, 55.0f });
        m_colorInfo->setColor({ 180, 200, 180 });
        m_colorInfo->setWidth(280.0f);
        this->addChild(m_colorInfo);
        
        // Close
        auto closeBtn = studio::ui::textButton(
            "X", this, menu_selector(PalettePanel::onClose),
            "GJ_button_06.png", 25.0f, 0.5f, 25.0f
        );
        closeBtn->setPosition({ panelW - 15.0f, panelH - 15.0f });
        menu->addChild(closeBtn);
        
        // Load first palette
        m_palettes = studio::design::getAllPalettes();
        updateDisplay();
        
        return true;
    }
    
    void updateDisplay() {
        if (m_palettes.empty()) return;
        
        auto& pal = m_palettes[g_selectedPalette];
        m_nameLabel->setString(pal.name.c_str());
        m_descLabel->setString(fmt::format("[{}] {}", pal.style, pal.description).c_str());
        
        // Update swatches
        studio::design::Color3 colors[] = { pal.bg, pal.ground, pal.primary, pal.secondary, pal.detail, pal.text };
        for (int i = 0; i < 6; i++) {
            auto swatch = static_cast<CCLayerColor*>(this->getChildByTag(100 + i));
            if (swatch) {
                auto c = colors[i].toCC();
                swatch->setColor(c);
            }
        }
        
        // Show RGB values
        m_colorInfo->setString(fmt::format(
            "BG({},{},{}) Gnd({},{},{}) Main({},{},{}) Acc({},{},{})",
            pal.bg.r, pal.bg.g, pal.bg.b,
            pal.ground.r, pal.ground.g, pal.ground.b,
            pal.primary.r, pal.primary.g, pal.primary.b,
            pal.secondary.r, pal.secondary.g, pal.secondary.b
        ).c_str());
    }
    
    void onPrev(CCObject*) {
        g_selectedPalette = (g_selectedPalette - 1 + m_palettes.size()) % m_palettes.size();
        updateDisplay();
    }
    
    void onNext(CCObject*) {
        g_selectedPalette = (g_selectedPalette + 1) % m_palettes.size();
        updateDisplay();
    }
    
    void onClose(CCObject*) {
        this->removeFromParent();
    }
    
private:
    CCLabelBMFont* m_nameLabel = nullptr;
    CCLabelBMFont* m_descLabel = nullptr;
    CCLabelBMFont* m_colorInfo = nullptr;
    std::vector<studio::design::ColorPalette> m_palettes;
};

// ============================================================
// Panel: Particle Effects
// ============================================================

class ParticlePanel : public CCLayer {
public:
    static ParticlePanel* create(EditorUI* editorUI) {
        auto ret = new ParticlePanel();
        if (ret && ret->init(editorUI)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    
    bool init(EditorUI* editorUI) {
        if (!CCLayer::init()) return false;
        m_editorUI = editorUI;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float panelW = 360.0f;
        float panelH = 250.0f;
        
        this->setContentSize({ panelW, panelH });
        this->setPosition({ winSize.width / 2, winSize.height / 2 + 30.0f });
        this->setAnchorPoint({ 0.5f, 0.5f });
        
        auto bg = studio::ui::panelBG(panelW, panelH, { 25, 15, 35, 225 });
        bg->setPosition({ panelW / 2, panelH / 2 });
        this->addChild(bg, -1);
        
        auto title = studio::ui::sectionLabel("Particle Effects", 0.5f);
        title->setPosition({ panelW / 2, panelH - 15.0f });
        this->addChild(title);
        
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu);
        
        // Category tabs
        auto cats = studio::particles::getCategories();
        float catX = 35.0f;
        for (int i = 0; i < (int)cats.size(); i++) {
            auto btn = studio::ui::textButton(
                cats[i].c_str(), this, menu_selector(ParticlePanel::onCatBtn),
                "GJ_button_04.png", 50.0f, 0.33f, 20.0f
            );
            btn->setPosition({ catX, panelH - 38.0f });
            btn->setTag(i);
            menu->addChild(btn);
            catX += 58.0f;
        }
        
        // Preset name + nav
        m_name = CCLabelBMFont::create("", "bigFont.fnt");
        m_name->setScale(0.4f);
        m_name->setPosition({ panelW / 2, panelH - 62.0f });
        this->addChild(m_name);
        
        m_desc = CCLabelBMFont::create("", "chatFont.fnt");
        m_desc->setScale(0.32f);
        m_desc->setPosition({ panelW / 2, panelH - 78.0f });
        m_desc->setColor({ 170, 200, 170 });
        m_desc->setWidth(300.0f);
        this->addChild(m_desc);
        
        auto prevBtn = studio::ui::textButton("<", this,
            menu_selector(ParticlePanel::onPrev), "GJ_button_01.png", 28.0f, 0.5f, 24.0f);
        prevBtn->setPosition({ 25.0f, panelH - 68.0f });
        menu->addChild(prevBtn);
        
        auto nextBtn = studio::ui::textButton(">", this,
            menu_selector(ParticlePanel::onNext), "GJ_button_01.png", 28.0f, 0.5f, 24.0f);
        nextBtn->setPosition({ panelW - 25.0f, panelH - 68.0f });
        menu->addChild(nextBtn);
        
        // Stats display
        m_stats = CCLabelBMFont::create("", "chatFont.fnt");
        m_stats->setScale(0.28f);
        m_stats->setPosition({ panelW / 2, panelH / 2 - 20.0f });
        m_stats->setColor({ 150, 180, 220 });
        m_stats->setWidth(320.0f);
        this->addChild(m_stats);
        
        // Color preview swatches (start → end)
        auto startLbl = CCLabelBMFont::create("Start", "chatFont.fnt");
        startLbl->setScale(0.25f);
        startLbl->setPosition({ panelW / 2 - 50.0f, 80.0f });
        startLbl->setColor({ 150, 150, 150 });
        this->addChild(startLbl);
        
        m_startSwatch = CCLayerColor::create({ 255, 255, 255, 255 }, 25.0f, 25.0f);
        m_startSwatch->setPosition({ panelW / 2 - 62.0f, 55.0f });
        this->addChild(m_startSwatch, 1);
        
        auto arrow = CCLabelBMFont::create("->", "chatFont.fnt");
        arrow->setScale(0.35f);
        arrow->setPosition({ panelW / 2, 67.0f });
        arrow->setColor({ 150, 150, 150 });
        this->addChild(arrow);
        
        auto endLbl = CCLabelBMFont::create("End", "chatFont.fnt");
        endLbl->setScale(0.25f);
        endLbl->setPosition({ panelW / 2 + 50.0f, 80.0f });
        endLbl->setColor({ 150, 150, 150 });
        this->addChild(endLbl);
        
        m_endSwatch = CCLayerColor::create({ 255, 255, 255, 0 }, 25.0f, 25.0f);
        m_endSwatch->setPosition({ panelW / 2 + 38.0f, 55.0f });
        this->addChild(m_endSwatch, 1);
        
        // Place button
        auto placeBtn = studio::ui::textButton(
            "Place at Camera", this, menu_selector(ParticlePanel::onPlace),
            "GJ_button_02.png", 130.0f, 0.5f, 30.0f
        );
        placeBtn->setPosition({ panelW / 2, 25.0f });
        menu->addChild(placeBtn);
        
        // Close
        auto closeBtn = studio::ui::textButton(
            "X", this, menu_selector(ParticlePanel::onClose),
            "GJ_button_06.png", 25.0f, 0.5f, 25.0f
        );
        closeBtn->setPosition({ panelW - 15.0f, panelH - 15.0f });
        menu->addChild(closeBtn);
        
        loadCategory("Ambient");
        return true;
    }
    
    void loadCategory(const std::string& cat) {
        m_presets = studio::particles::getPresetsByCategory(cat);
        m_idx = 0;
        updateDisplay();
    }
    
    void updateDisplay() {
        if (m_presets.empty()) {
            m_name->setString("No effects");
            m_desc->setString("");
            m_stats->setString("");
            return;
        }
        
        auto& p = m_presets[m_idx];
        auto& cfg = p.config;
        
        m_name->setString(fmt::format("{} ({}/{})", p.name, m_idx + 1, m_presets.size()).c_str());
        m_desc->setString(p.description.c_str());
        
        std::string blend = cfg.additive ? "Additive (glow)" : "Normal (solid)";
        std::string mode = cfg.emitterMode == 0 ? "Gravity" : "Radius (spiral)";
        m_stats->setString(fmt::format(
            "Rate: {:.0f}/s | Max: {} | Life: {:.1f}s | Speed: {:.0f}\n"
            "Size: {:.0f}->{:.0f} | Angle: {:.0f} +/-{:.0f} | Blend: {} | {}",
            cfg.emissionRate, cfg.maxParticles, cfg.lifetime, cfg.speed,
            cfg.startSize, cfg.endSize, cfg.angle, cfg.angleVar, blend, mode
        ).c_str());
        
        // Update color swatches
        m_startSwatch->setColor({ 
            (GLubyte)cfg.startColor.r, (GLubyte)cfg.startColor.g, (GLubyte)cfg.startColor.b });
        m_startSwatch->setOpacity(cfg.startColor.a);
        m_endSwatch->setColor({
            (GLubyte)cfg.endColor.r, (GLubyte)cfg.endColor.g, (GLubyte)cfg.endColor.b });
        m_endSwatch->setOpacity(std::max(30, cfg.endColor.a));  // min opacity so you can see it
    }
    
    void onCatBtn(CCObject* sender) {
        int tag = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
        auto cats = studio::particles::getCategories();
        if (tag >= 0 && tag < (int)cats.size()) loadCategory(cats[tag]);
    }
    
    void onPrev(CCObject*) {
        if (m_presets.empty()) return;
        m_idx = (m_idx - 1 + m_presets.size()) % m_presets.size();
        updateDisplay();
    }
    
    void onNext(CCObject*) {
        if (m_presets.empty()) return;
        m_idx = (m_idx + 1) % m_presets.size();
        updateDisplay();
    }
    
    void onPlace(CCObject*) {
        if (m_presets.empty() || !g_editor) return;
        
        auto& preset = m_presets[m_idx];
        
        // Place at camera center
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float cx = winSize.width / 2;
        float cy = winSize.height / 2;
        auto editorPos = g_editor->m_objectLayer->convertToNodeSpace(
            CCDirector::sharedDirector()->convertToGL({ cx, cy }));
        
        auto obj = studio::particles::placeParticle(g_editor, preset, editorPos.x, editorPos.y);
        
        studio::ui::showInfo("Particle Placed",
            fmt::format("<cg>{}</c> placed!\n\n"
                "Position: ({:.0f}, {:.0f})\n"
                "Emission: {:.0f} particles/sec\n"
                "Blend: {}\n\n"
                "Select the particle object to edit\n"
                "its properties in GD's particle editor.\n"
                "The preset values are applied as a starting point.",
                preset.name, editorPos.x, editorPos.y,
                preset.config.emissionRate,
                preset.config.additive ? "Additive (glow)" : "Normal (solid)"
            ));
    }
    
    void onClose(CCObject*) {
        this->removeFromParent();
    }
    
private:
    EditorUI* m_editorUI = nullptr;
    CCLabelBMFont* m_name = nullptr;
    CCLabelBMFont* m_desc = nullptr;
    CCLabelBMFont* m_stats = nullptr;
    CCLayerColor* m_startSwatch = nullptr;
    CCLayerColor* m_endSwatch = nullptr;
    std::vector<studio::particles::ParticlePreset> m_presets;
    int m_idx = 0;
};

// ============================================================
// Panel: Shape Builder
// ============================================================

class ShapePanel : public CCLayer {
public:
    static ShapePanel* create(EditorUI* editorUI) {
        auto ret = new ShapePanel();
        if (ret && ret->init(editorUI)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    
    bool init(EditorUI* editorUI) {
        if (!CCLayer::init()) return false;
        m_editorUI = editorUI;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float panelW = 360.0f;
        float panelH = 280.0f;
        
        this->setContentSize({ panelW, panelH });
        this->setPosition({ winSize.width / 2, winSize.height / 2 + 20.0f });
        this->setAnchorPoint({ 0.5f, 0.5f });
        
        auto bg = studio::ui::panelBG(panelW, panelH, { 20, 20, 40, 225 });
        bg->setPosition({ panelW / 2, panelH / 2 });
        this->addChild(bg, -1);
        
        auto title = studio::ui::sectionLabel("Shape Builder", 0.5f);
        title->setPosition({ panelW / 2, panelH - 15.0f });
        this->addChild(title);
        
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu);
        
        // Category tabs
        auto cats = studio::shapes::getShapeCategories();
        float catX = 50.0f;
        for (int i = 0; i < (int)cats.size(); i++) {
            auto btn = studio::ui::textButton(
                cats[i].c_str(), this, menu_selector(ShapePanel::onCatBtn),
                "GJ_button_04.png", 70.0f, 0.38f, 22.0f
            );
            btn->setPosition({ catX, panelH - 38.0f });
            btn->setTag(i);
            menu->addChild(btn);
            catX += 85.0f;
        }
        
        // Shape name + nav
        m_shapeName = CCLabelBMFont::create("", "bigFont.fnt");
        m_shapeName->setScale(0.4f);
        m_shapeName->setPosition({ panelW / 2, panelH - 65.0f });
        this->addChild(m_shapeName);
        
        m_shapeDesc = CCLabelBMFont::create("", "chatFont.fnt");
        m_shapeDesc->setScale(0.32f);
        m_shapeDesc->setPosition({ panelW / 2, panelH - 82.0f });
        m_shapeDesc->setColor({ 160, 180, 160 });
        this->addChild(m_shapeDesc);
        
        auto prevBtn = studio::ui::textButton("<", this,
            menu_selector(ShapePanel::onPrev), "GJ_button_01.png", 28.0f, 0.5f, 24.0f);
        prevBtn->setPosition({ 25.0f, panelH - 72.0f });
        menu->addChild(prevBtn);
        
        auto nextBtn = studio::ui::textButton(">", this,
            menu_selector(ShapePanel::onNext), "GJ_button_01.png", 28.0f, 0.5f, 24.0f);
        nextBtn->setPosition({ panelW - 25.0f, panelH - 72.0f });
        menu->addChild(nextBtn);
        
        // Size selector
        m_sizeLabel = CCLabelBMFont::create("Size: Medium", "chatFont.fnt");
        m_sizeLabel->setScale(0.35f);
        m_sizeLabel->setPosition({ panelW / 2 - 70.0f, panelH / 2 - 10.0f });
        m_sizeLabel->setColor({ 200, 200, 255 });
        this->addChild(m_sizeLabel);
        
        auto sizeBtn = studio::ui::textButton("Size", this,
            menu_selector(ShapePanel::onCycleSize), "GJ_button_01.png", 50.0f, 0.4f, 24.0f);
        sizeBtn->setPosition({ panelW / 2 + 60.0f, panelH / 2 - 10.0f });
        menu->addChild(sizeBtn);
        
        // Fill toggle
        m_fillLabel = CCLabelBMFont::create("Mode: Filled", "chatFont.fnt");
        m_fillLabel->setScale(0.35f);
        m_fillLabel->setPosition({ panelW / 2 - 70.0f, panelH / 2 - 35.0f });
        m_fillLabel->setColor({ 200, 255, 200 });
        this->addChild(m_fillLabel);
        
        auto fillBtn = studio::ui::textButton("Fill", this,
            menu_selector(ShapePanel::onToggleFill), "GJ_button_01.png", 50.0f, 0.4f, 24.0f);
        fillBtn->setPosition({ panelW / 2 + 60.0f, panelH / 2 - 35.0f });
        menu->addChild(fillBtn);
        
        // Block type selector
        m_blockLabel = CCLabelBMFont::create("Block: Default Block", "chatFont.fnt");
        m_blockLabel->setScale(0.33f);
        m_blockLabel->setPosition({ panelW / 2 - 50.0f, panelH / 2 - 60.0f });
        m_blockLabel->setColor({ 255, 200, 150 });
        this->addChild(m_blockLabel);
        
        auto blockBtn = studio::ui::textButton("Block", this,
            menu_selector(ShapePanel::onCycleBlock), "GJ_button_05.png", 55.0f, 0.4f, 24.0f);
        blockBtn->setPosition({ panelW / 2 + 70.0f, panelH / 2 - 60.0f });
        menu->addChild(blockBtn);
        
        // Preview info
        m_previewLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_previewLabel->setScale(0.3f);
        m_previewLabel->setPosition({ panelW / 2, 65.0f });
        m_previewLabel->setColor({ 150, 150, 180 });
        this->addChild(m_previewLabel);
        
        // Place button
        auto placeBtn = studio::ui::textButton(
            "Place at Camera", this, menu_selector(ShapePanel::onPlace),
            "GJ_button_02.png", 130.0f, 0.5f, 30.0f
        );
        placeBtn->setPosition({ panelW / 2, 30.0f });
        menu->addChild(placeBtn);
        
        // Close
        auto closeBtn = studio::ui::textButton(
            "X", this, menu_selector(ShapePanel::onClose),
            "GJ_button_06.png", 25.0f, 0.5f, 25.0f
        );
        closeBtn->setPosition({ panelW - 15.0f, panelH - 15.0f });
        menu->addChild(closeBtn);
        
        m_blocks = studio::shapes::getBlockChoices();
        loadCategory("Basic");
        updatePreview();
        return true;
    }
    
    void loadCategory(const std::string& cat) {
        m_shapes = studio::shapes::getShapesByCategory(cat);
        m_shapeIdx = 0;
        updateDisplay();
    }
    
    void updateDisplay() {
        if (m_shapes.empty()) {
            m_shapeName->setString("No shapes");
            m_shapeDesc->setString("");
            return;
        }
        auto& s = m_shapes[m_shapeIdx];
        m_shapeName->setString(fmt::format("{} ({}/{})", s.name, m_shapeIdx + 1, m_shapes.size()).c_str());
        m_shapeDesc->setString(s.description.c_str());
        updatePreview();
    }
    
    void updatePreview() {
        if (m_shapes.empty()) return;
        auto& s = m_shapes[m_shapeIdx];
        float scale = studio::shapes::sizeMultiplier(m_size);
        auto pts = s.generate(scale, m_filled);
        m_previewLabel->setString(
            fmt::format("Preview: ~{} blocks | {} | {}", 
                pts.size(), studio::shapes::sizeName(m_size),
                m_filled ? "Filled" : "Outline").c_str());
    }
    
    void onCatBtn(CCObject* sender) {
        int tag = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
        auto cats = studio::shapes::getShapeCategories();
        if (tag >= 0 && tag < (int)cats.size()) loadCategory(cats[tag]);
    }
    
    void onPrev(CCObject*) {
        if (m_shapes.empty()) return;
        m_shapeIdx = (m_shapeIdx - 1 + m_shapes.size()) % m_shapes.size();
        updateDisplay();
    }
    
    void onNext(CCObject*) {
        if (m_shapes.empty()) return;
        m_shapeIdx = (m_shapeIdx + 1) % m_shapes.size();
        updateDisplay();
    }
    
    void onCycleSize(CCObject*) {
        m_size = static_cast<studio::shapes::ShapeSize>(
            ((int)m_size + 1) % 4);
        m_sizeLabel->setString(
            fmt::format("Size: {}", studio::shapes::sizeName(m_size)).c_str());
        updatePreview();
    }
    
    void onToggleFill(CCObject*) {
        m_filled = !m_filled;
        m_fillLabel->setString(
            fmt::format("Mode: {}", m_filled ? "Filled" : "Outline").c_str());
        updatePreview();
    }
    
    void onCycleBlock(CCObject*) {
        m_blockIdx = (m_blockIdx + 1) % m_blocks.size();
        m_blockLabel->setString(
            fmt::format("Block: {}", m_blocks[m_blockIdx].name).c_str());
    }
    
    void onPlace(CCObject*) {
        if (m_shapes.empty() || !g_editor) return;
        
        // Place at the editor camera position
        auto editorLayer = g_editor;
        auto camera = editorLayer->m_editorUI->getGroupCenter(
            editorLayer->m_objects, false);
        
        // Fallback: use center of visible area
        float cx = CCDirector::sharedDirector()->getWinSize().width / 2;
        float cy = CCDirector::sharedDirector()->getWinSize().height / 2;
        
        // Convert screen to editor coordinates
        auto editorPos = editorLayer->m_objectLayer->convertToNodeSpace(
            CCDirector::sharedDirector()->convertToGL({ cx, cy }));
        
        auto& shape = m_shapes[m_shapeIdx];
        int blockID = m_blocks[m_blockIdx].id;
        
        auto result = studio::shapes::placeShape(
            g_editor, shape, editorPos.x, editorPos.y,
            m_size, m_filled, blockID, 0.0f
        );
        
        studio::ui::showInfo("Shape Placed",
            fmt::format("<cg>{}</c> placed!\n\n"
                "{} blocks placed\n"
                "Group: <cy>{}</c>\n"
                "Block: {}\n\n"
                "Select all (group {}) to move, color, or animate.",
                shape.name, result.objectsPlaced,
                result.groupID, m_blocks[m_blockIdx].name,
                result.groupID
            ));
    }
    
    void onClose(CCObject*) {
        this->removeFromParent();
    }
    
private:
    EditorUI* m_editorUI = nullptr;
    CCLabelBMFont* m_shapeName = nullptr;
    CCLabelBMFont* m_shapeDesc = nullptr;
    CCLabelBMFont* m_sizeLabel = nullptr;
    CCLabelBMFont* m_fillLabel = nullptr;
    CCLabelBMFont* m_blockLabel = nullptr;
    CCLabelBMFont* m_previewLabel = nullptr;
    std::vector<studio::shapes::ShapeTemplate> m_shapes;
    std::vector<studio::shapes::BlockChoice> m_blocks;
    studio::shapes::ShapeSize m_size = studio::shapes::ShapeSize::Medium;
    bool m_filled = true;
    int m_shapeIdx = 0;
    int m_blockIdx = 0;
};

// ============================================================
// Panel: Quick Triggers
// ============================================================

class TriggersPanel : public CCLayer {
public:
    static TriggersPanel* create(EditorUI* editorUI) {
        auto ret = new TriggersPanel();
        if (ret && ret->init(editorUI)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    
    bool init(EditorUI* editorUI) {
        if (!CCLayer::init()) return false;
        m_editorUI = editorUI;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float panelW = 350.0f;
        float panelH = 260.0f;
        
        this->setContentSize({ panelW, panelH });
        this->setPosition({ winSize.width / 2, winSize.height / 2 + 30.0f });
        this->setAnchorPoint({ 0.5f, 0.5f });
        
        auto bg = studio::ui::panelBG(panelW, panelH, { 25, 15, 30, 225 });
        bg->setPosition({ panelW / 2, panelH / 2 });
        this->addChild(bg, -1);
        
        auto title = studio::ui::sectionLabel("Quick Triggers", 0.5f);
        title->setPosition({ panelW / 2, panelH - 15.0f });
        this->addChild(title);
        
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu);
        
        // Category tabs across the top
        auto cats = studio::triggers::getCategories();
        float catX = 50.0f;
        for (int i = 0; i < (int)cats.size(); i++) {
            auto btn = studio::ui::textButton(
                cats[i].c_str(), this, menu_selector(TriggersPanel::onCategoryBtn),
                "GJ_button_04.png", 70.0f, 0.38f, 22.0f
            );
            btn->setPosition({ catX, panelH - 38.0f });
            btn->setTag(i);
            menu->addChild(btn);
            catX += 85.0f;
        }
        
        // Action display
        m_actionName = CCLabelBMFont::create("", "bigFont.fnt");
        m_actionName->setScale(0.38f);
        m_actionName->setPosition({ panelW / 2, panelH - 65.0f });
        this->addChild(m_actionName);
        
        m_actionDesc = CCLabelBMFont::create("", "chatFont.fnt");
        m_actionDesc->setScale(0.33f);
        m_actionDesc->setPosition({ panelW / 2, panelH - 82.0f });
        m_actionDesc->setColor({ 170, 200, 170 });
        m_actionDesc->setWidth(300.0f);
        this->addChild(m_actionDesc);
        
        // Nav buttons
        auto prevBtn = studio::ui::textButton("<", this,
            menu_selector(TriggersPanel::onPrev), "GJ_button_01.png", 28.0f, 0.5f, 24.0f);
        prevBtn->setPosition({ 25.0f, panelH / 2 - 15.0f });
        menu->addChild(prevBtn);
        
        auto nextBtn = studio::ui::textButton(">", this,
            menu_selector(TriggersPanel::onNext), "GJ_button_01.png", 28.0f, 0.5f, 24.0f);
        nextBtn->setPosition({ panelW - 25.0f, panelH / 2 - 15.0f });
        menu->addChild(nextBtn);
        
        // Info box: shows what will happen
        m_infoLabel = CCLabelBMFont::create("Select objects, then apply a trigger action.\nTriggers are auto-placed below your objects.", "chatFont.fnt");
        m_infoLabel->setScale(0.3f);
        m_infoLabel->setPosition({ panelW / 2, panelH / 2 - 40.0f });
        m_infoLabel->setColor({ 150, 150, 180 });
        m_infoLabel->setWidth(300.0f);
        this->addChild(m_infoLabel);
        
        // Apply button
        auto applyBtn = studio::ui::textButton(
            "Apply to Selection", this, menu_selector(TriggersPanel::onApply),
            "GJ_button_02.png", 130.0f, 0.5f, 30.0f
        );
        applyBtn->setPosition({ panelW / 2 - 60.0f, 30.0f });
        menu->addChild(applyBtn);
        
        // Group info button
        auto groupBtn = studio::ui::textButton(
            "View Groups", this, menu_selector(TriggersPanel::onViewGroups),
            "GJ_button_05.png", 90.0f, 0.45f, 28.0f
        );
        groupBtn->setPosition({ panelW / 2 + 80.0f, 30.0f });
        menu->addChild(groupBtn);
        
        // Close
        auto closeBtn = studio::ui::textButton(
            "X", this, menu_selector(TriggersPanel::onClose),
            "GJ_button_06.png", 25.0f, 0.5f, 25.0f
        );
        closeBtn->setPosition({ panelW - 15.0f, panelH - 15.0f });
        menu->addChild(closeBtn);
        
        loadCategory("Motion");
        return true;
    }
    
    void loadCategory(const std::string& cat) {
        m_currentActions = studio::triggers::getActionsByCategory(cat);
        m_currentCat = cat;
        m_idx = 0;
        updateDisplay();
    }
    
    void updateDisplay() {
        if (m_currentActions.empty()) {
            m_actionName->setString("No actions");
            m_actionDesc->setString("");
            return;
        }
        auto& a = m_currentActions[m_idx];
        m_actionName->setString(fmt::format("{} ({}/{})", a.name, m_idx + 1, m_currentActions.size()).c_str());
        m_actionDesc->setString(a.description.c_str());
        
        std::string info;
        if (a.category == "Template") {
            info = "TEMPLATE: Places multiple triggers at once.\nSelect objects first, then apply.";
        } else {
            info = fmt::format("Places 1 trigger (ID {}). Auto-assigns group.\nSelect objects, then apply.", a.triggerID);
        }
        m_infoLabel->setString(info.c_str());
    }
    
    void onCategoryBtn(CCObject* sender) {
        int tag = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
        auto cats = studio::triggers::getCategories();
        if (tag >= 0 && tag < (int)cats.size()) {
            loadCategory(cats[tag]);
        }
    }
    
    void onPrev(CCObject*) {
        if (m_currentActions.empty()) return;
        m_idx = (m_idx - 1 + m_currentActions.size()) % m_currentActions.size();
        updateDisplay();
    }
    
    void onNext(CCObject*) {
        if (m_currentActions.empty()) return;
        m_idx = (m_idx + 1) % m_currentActions.size();
        updateDisplay();
    }
    
    void onApply(CCObject*) {
        if (m_currentActions.empty() || !g_editor || !m_editorUI) return;
        
        auto selected = m_editorUI->getSelectedObjects();
        if (!selected || selected->count() == 0) {
            studio::ui::showInfo("Quick Triggers", "Select some objects first!");
            return;
        }
        
        auto& action = m_currentActions[m_idx];
        
        // Assign group
        int groupID = studio::triggers::assignGroupToSelection(m_editorUI, g_editor);
        if (groupID < 0) return;
        
        // Place trigger below the selection
        float trigX = studio::triggers::getSelectionMinX(m_editorUI);
        float trigY = -60.0f;  // below level
        
        auto obj = g_editor->createObject(action.triggerID, { trigX, trigY }, false);
        auto eff = typeinfo_cast<EffectGameObject*>(obj);
        
        if (eff && action.configure) {
            action.configure(eff, groupID, g_editor);
        }
        
        studio::ui::showInfo("Trigger Applied",
            fmt::format("<cg>{}</c> applied!\n\n"
                "Group: <cy>{}</c> ({} objects)\n"
                "Trigger at ({:.0f}, {:.0f})\n\n"
                "Tip: You can select the trigger and\nedit it normally to fine-tune values.",
                action.name, groupID, selected->count(), trigX, trigY));
    }
    
    void onViewGroups(CCObject*) {
        if (!g_editor) return;
        
        auto groups = studio::triggers::scanGroups(g_editor);
        
        if (groups.empty()) {
            studio::ui::showInfo("Groups", "No groups used in this level yet.");
            return;
        }
        
        std::string report = fmt::format("<cg>{} groups found:</c>\n\n", groups.size());
        
        int shown = 0;
        for (auto& g : groups) {
            if (shown >= 15) {
                report += fmt::format("\n...and {} more groups", groups.size() - shown);
                break;
            }
            report += fmt::format("G{}: {} objs, {} triggers\n", g.id, g.objectCount, g.triggerCount);
            shown++;
        }
        
        // Show next free group
        int nextFree = studio::triggers::findNextFreeGroup(g_editor);
        report += fmt::format("\n<cy>Next free group: {}</c>", nextFree);
        
        FLAlertLayer::create("Group Manager", report.c_str(), "OK")->show();
    }
    
    void onClose(CCObject*) {
        this->removeFromParent();
    }
    
private:
    EditorUI* m_editorUI = nullptr;
    CCLabelBMFont* m_actionName = nullptr;
    CCLabelBMFont* m_actionDesc = nullptr;
    CCLabelBMFont* m_infoLabel = nullptr;
    std::vector<studio::triggers::QuickAction> m_currentActions;
    std::string m_currentCat;
    int m_idx = 0;
};

// ============================================================
// Hooks: LevelEditorLayer — track the editor instance
// ============================================================

class $modify(StudioEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool p1) {
        if (!LevelEditorLayer::init(level, p1)) return false;
        g_editor = this;
        return true;
    }
};

// ============================================================
// Hooks: EditorUI — add Studio buttons to the editor
// ============================================================

class $modify(StudioEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editor) {
        if (!EditorUI::init(editor)) return false;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        // Create a menu for our studio buttons (top-right of editor)
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu, 100);
        
        float btnX = winSize.width - 25.0f;
        float btnY = winSize.height / 2 + 60.0f;
        float spacing = 35.0f;
        
        // Animation button
        auto animSpr = ButtonSprite::create("Anim", 40, true, "bigFont.fnt", 
            "GJ_button_02.png", 25.0f, 0.4f);
        auto animBtn = CCMenuItemSpriteExtra::create(
            animSpr, this, menu_selector(StudioEditorUI::onAnimStudio));
        animBtn->setPosition({ btnX, btnY });
        menu->addChild(animBtn);
        
        // Color palette button
        auto colorSpr = ButtonSprite::create("Color", 40, true, "bigFont.fnt",
            "GJ_button_05.png", 25.0f, 0.4f);
        auto colorBtn = CCMenuItemSpriteExtra::create(
            colorSpr, this, menu_selector(StudioEditorUI::onColorPalette));
        colorBtn->setPosition({ btnX, btnY - spacing });
        menu->addChild(colorBtn);
        
        // Analyze button
        auto analyzeSpr = ButtonSprite::create("Check", 40, true, "bigFont.fnt",
            "GJ_button_04.png", 25.0f, 0.4f);
        auto analyzeBtn = CCMenuItemSpriteExtra::create(
            analyzeSpr, this, menu_selector(StudioEditorUI::onAnalyze));
        analyzeBtn->setPosition({ btnX, btnY - spacing * 2 });
        menu->addChild(analyzeBtn);
        
        // Quick Triggers button
        auto trigSpr = ButtonSprite::create("Trig", 40, true, "bigFont.fnt",
            "GJ_button_01.png", 25.0f, 0.4f);
        auto trigBtn = CCMenuItemSpriteExtra::create(
            trigSpr, this, menu_selector(StudioEditorUI::onQuickTriggers));
        trigBtn->setPosition({ btnX, btnY - spacing * 3 });
        menu->addChild(trigBtn);
        
        // Shape Builder button
        auto shapeSpr = ButtonSprite::create("Shape", 40, true, "bigFont.fnt",
            "GJ_button_03.png", 25.0f, 0.4f);
        auto shapeBtn = CCMenuItemSpriteExtra::create(
            shapeSpr, this, menu_selector(StudioEditorUI::onShapeBuilder));
        shapeBtn->setPosition({ btnX, btnY - spacing * 4 });
        menu->addChild(shapeBtn);
        
        // Particle FX button
        auto fxSpr = ButtonSprite::create("FX", 40, true, "bigFont.fnt",
            "GJ_button_01.png", 25.0f, 0.45f);
        auto fxBtn = CCMenuItemSpriteExtra::create(
            fxSpr, this, menu_selector(StudioEditorUI::onParticleFX));
        fxBtn->setPosition({ btnX, btnY - spacing * 5 });
        menu->addChild(fxBtn);
        
        log::info("[Creator Studio] Editor UI initialized — 6 panels available");
        
        return true;
    }
    
    // ---- Open Animation Studio ----
    void onAnimStudio(CCObject*) {
        // Remove existing panel if open
        if (auto existing = this->getChildByTag(9001)) {
            existing->removeFromParent();
            return;
        }
        
        auto panel = AnimationPanel::create(this);
        if (panel) {
            panel->setTag(9001);
            this->addChild(panel, 200);
        }
    }
    
    // ---- Open Color Palette ----
    void onColorPalette(CCObject*) {
        if (auto existing = this->getChildByTag(9002)) {
            existing->removeFromParent();
            return;
        }
        
        auto panel = PalettePanel::create();
        if (panel) {
            panel->setTag(9002);
            this->addChild(panel, 200);
        }
    }
    
    // ---- Run Level Analysis ----
    void onAnalyze(CCObject*) {
        if (!g_editor) return;
        
        auto analysis = studio::gameplay::analyzeLevel(g_editor);
        auto report = studio::gameplay::formatAnalysis(analysis);
        
        FLAlertLayer::create(
            "Level Analysis",
            report.c_str(),
            "OK"
        )->show();
    }
    
    // ---- Open Quick Triggers ----
    void onQuickTriggers(CCObject*) {
        if (auto existing = this->getChildByTag(9003)) {
            existing->removeFromParent();
            return;
        }
        
        auto panel = TriggersPanel::create(this);
        if (panel) {
            panel->setTag(9003);
            this->addChild(panel, 200);
        }
    }
    
    // ---- Open Shape Builder ----
    void onShapeBuilder(CCObject*) {
        if (auto existing = this->getChildByTag(9004)) {
            existing->removeFromParent();
            return;
        }
        
        auto panel = ShapePanel::create(this);
        if (panel) {
            panel->setTag(9004);
            this->addChild(panel, 200);
        }
    }
    
    // ---- Open Particle FX ----
    void onParticleFX(CCObject*) {
        if (auto existing = this->getChildByTag(9005)) {
            existing->removeFromParent();
            return;
        }
        
        auto panel = ParticlePanel::create(this);
        if (panel) {
            panel->setTag(9005);
            this->addChild(panel, 200);
        }
    }
};

// ============================================================
// Hooks: EditorPauseLayer — add analysis to pause menu too
// ============================================================

class $modify(StudioPauseLayer, EditorPauseLayer) {
    void customSetup() {
        EditorPauseLayer::customSetup();
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu, 100);
        
        // Quick analysis button in pause menu
        auto spr = ButtonSprite::create("Analyze Level", 100, true,
            "bigFont.fnt", "GJ_button_04.png", 28.0f, 0.45f);
        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(StudioPauseLayer::onQuickAnalyze));
        btn->setPosition({ winSize.width - 90.0f, 35.0f });
        menu->addChild(btn);
    }
    
    void onQuickAnalyze(CCObject*) {
        if (!g_editor) return;
        
        auto analysis = studio::gameplay::analyzeLevel(g_editor);
        auto report = studio::gameplay::formatAnalysis(analysis);
        
        FLAlertLayer::create("Level Analysis", report.c_str(), "OK")->show();
    }
};
