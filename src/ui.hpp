#pragma once
/**
 * ui.hpp — Shared UI components for Creator Studio panels
 * 
 * Provides helper functions to create consistent, clean editor
 * panel interfaces without repeating boilerplate.
 */

#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace studio::ui {

    // Create a section header label
    inline CCLabelBMFont* sectionLabel(const char* text, float scale = 0.4f) {
        auto label = CCLabelBMFont::create(text, "goldFont.fnt");
        label->setScale(scale);
        return label;
    }

    // Create a body text label
    inline CCLabelBMFont* bodyLabel(const char* text, float scale = 0.35f) {
        auto label = CCLabelBMFont::create(text, "chatFont.fnt");
        label->setScale(scale);
        label->setColor({ 200, 200, 200 });
        return label;
    }

    // Create a clickable button with text
    inline CCMenuItemSpriteExtra* textButton(
        const char* text, CCObject* target, SEL_MenuHandler callback,
        const char* bg = "GJ_button_01.png", float width = 80.0f,
        float scale = 0.6f, float height = 30.0f
    ) {
        auto spr = ButtonSprite::create(text, width, true, "bigFont.fnt", bg, height, scale);
        return CCMenuItemSpriteExtra::create(spr, target, callback);
    }

    // Create a small icon button
    inline CCMenuItemSpriteExtra* iconButton(
        const char* spriteName, CCObject* target, SEL_MenuHandler callback,
        float scale = 0.8f
    ) {
        auto spr = CCSprite::createWithSpriteFrameName(spriteName);
        spr->setScale(scale);
        return CCMenuItemSpriteExtra::create(spr, target, callback);
    }

    // Create a scrollable panel container
    inline CCLayer* scrollPanel(float width, float height) {
        auto layer = CCLayer::create();
        layer->setContentSize({ width, height });
        layer->setAnchorPoint({ 0.5f, 0.5f });
        return layer;
    }

    // Create a colored background box
    inline CCLayerColor* panelBG(float w, float h, ccColor4B color = { 0, 0, 0, 120 }) {
        auto bg = CCLayerColor::create(color, w, h);
        bg->setAnchorPoint({ 0.5f, 0.5f });
        bg->ignoreAnchorPointForPosition(false);
        return bg;
    }

    // Arrange nodes vertically with spacing
    inline void layoutVertical(CCNode* parent, const std::vector<CCNode*>& children,
                               float startY, float spacing) {
        float y = startY;
        for (auto child : children) {
            child->setPositionY(y);
            parent->addChild(child);
            y -= spacing;
        }
    }

    // Show a quick info popup
    inline void showInfo(const char* title, const std::string& body) {
        FLAlertLayer::create(title, body.c_str(), "OK")->show();
    }

    // Show a confirmation popup  
    inline void showConfirm(const char* title, const std::string& body,
                            const char* yesText, const char* noText,
                            CCObject* target, SEL_MenuHandler yesCallback) {
        auto alert = FLAlertLayer::create(target, title, body.c_str(), noText, yesText);
        alert->show();
    }

} // namespace studio::ui
