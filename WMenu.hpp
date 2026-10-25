#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

// The "W" icon used by both the main menu and pause menu buttons
cocos2d::CCNode* makeWIcon(float scale);

class WMenu : public geode::Popup {
protected:
    cocos2d::CCLabelBMFont* m_framesLabel = nullptr;

    bool init();
    void onToggle(cocos2d::CCObject* sender);
    void onStep(cocos2d::CCObject* sender);
    void refreshFrames();

public:
    static WMenu* create();
};

void openWMenu();
